#include "LitDatabase.h"

void LitDatabase::InitializeInternalNode(void* node) {
    set_node_type(node, NODE_INTERNAL);
    set_node_root(node, false);
    *InternalNodeNumKeys(node) = 0;
}

void LitDatabase::UpdateInternalNodeKey(void* node, uint32_t old_key, uint32_t new_key) {
    uint32_t old_child_index = InternalNodeFindChild(node, old_key);
    *InternalNodeKey(node, old_child_index) = new_key;
}

uint32_t* LitDatabase::InternalNodeNumKeys(void* node) {
    return static_cast<uint32_t*>(
        static_cast<void*>(static_cast<unsigned char*>(node) + INTERNAL_NODE_NUM_KEYS_OFFSET));
}
uint32_t* LitDatabase::InternalNodeRightChild(void* node) {
    return static_cast<uint32_t*>(
        static_cast<void*>(static_cast<unsigned char*>(node) + INTERNAL_NODE_RIGHT_CHILD_OFFSET));
}
uint32_t* LitDatabase::InternalNodeCell(void* node, uint32_t cell_num) {
    return static_cast<uint32_t*>(static_cast<void*>(static_cast<unsigned char*>(node) + INTERNAL_NODE_HEADER_SIZE +
                                                     cell_num * INTERNAL_NODE_CELL_SIZE));
}
uint32_t* LitDatabase::InternalNodeChild(void* node, uint32_t child_num) {
    uint32_t num_keys = *InternalNodeNumKeys(node);
    if (child_num > num_keys) {
        std::cout << "try to access unvalid child_num" << std::endl;
        exit(EXIT_FAILURE);
    } else if (child_num == num_keys) {
        return InternalNodeRightChild(node);
    } else {
        return InternalNodeCell(node, child_num);
    }
}
uint32_t* LitDatabase::InternalNodeKey(void* node, uint32_t key_num) {
    return static_cast<uint32_t*>(static_cast<void*>(
        static_cast<unsigned char*>(static_cast<void*>(InternalNodeCell(node, key_num))) + INTERNAL_NODE_CHILD_SIZE));
}

uint32_t LitDatabase::InternalNodeFindChild(void* node, uint32_t key) {
    uint32_t num_keys = *InternalNodeNumKeys(node);

    // binary search
    uint32_t min_index = 0;
    uint32_t max_index = num_keys;
    while (min_index != max_index) {
        uint32_t index = (min_index + max_index) / 2;
        uint32_t key_to_right = *InternalNodeKey(node, index);
        if (key_to_right >= key) {
            max_index = index;
        } else {
            min_index = index + 1;
        }
    }

    return min_index;
}

Cursor* LitDatabase::InternalNodeFind(Table* table, uint32_t page_num, uint32_t key) {
    void* node = GetPage(table->pager, page_num);
    uint32_t child_index = InternalNodeFindChild(node, key);
    uint32_t child_num = *InternalNodeChild(node, child_index);
    void* child = GetPage(table->pager, child_num);
    switch (get_node_type(child)) {
        case NODE_LEAF: return LeafNodeFind(table, child_num, key);
        case NODE_INTERNAL: return InternalNodeFind(table, child_num, key);
    }
}

void LitDatabase::InternalNodeInsert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    void* parent = GetPage(table->pager, parent_page_num);
    void* child = GetPage(table->pager, child_page_num);
    uint32_t child_max_key = GetNodeMaxKey(child);
    uint32_t index = InternalNodeFindChild(parent, child_max_key);

    uint32_t original_num_keys = *InternalNodeNumKeys(parent);
    *InternalNodeNumKeys(parent) = original_num_keys + 1;

    if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
        std::cout << "Need to implement splitting internal node" << std::endl;
        exit(EXIT_FAILURE);
    }

    uint32_t right_child_page_num = *InternalNodeRightChild(parent);
    void* right_child = GetPage(table->pager, right_child_page_num);
    if (child_max_key > GetNodeMaxKey(right_child)) {
        // replace right child
        *InternalNodeChild(parent, original_num_keys) = right_child_page_num;
        *InternalNodeKey(parent, original_num_keys) = GetNodeMaxKey(right_child);
        *InternalNodeRightChild(parent) = child_page_num;
    } else {
        // make room for the new cell
        for (uint32_t i = original_num_keys; i > index; --i) {
            void* destination = InternalNodeCell(parent, i);
            void* source = InternalNodeCell(parent, i - 1);
            memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
        }
        *InternalNodeChild(parent, index) = child_page_num;
        *InternalNodeKey(parent, index) = child_max_key;
    }
}

uint32_t* LitDatabase::NodeParent(void* node) {
    return static_cast<uint32_t*>(static_cast<void*>(static_cast<unsigned char*>(node) + PARENT_POINTER_OFFSET));
}