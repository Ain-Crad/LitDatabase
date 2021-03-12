#include "LitDatabase.h"

void LitDatabase::InitializeLeafNode(void* node) {
    set_node_type(node, NODE_LEAF);
    set_node_root(node, false);
    *LeafNodeNumCells(node) = 0;
    *LeafNodeNextLeaf(node) = 0;
}

void LitDatabase::LeafNodeSplitAndInsert(Cursor* cursor, uint32_t key, const Row& value) {
    void* old_node = GetPage(cursor->table->pager, cursor->page_num);
    uint32_t old_max = GetNodeMaxKey(old_node);
    uint32_t new_page_num = GetUnusedPageNum(cursor->table->pager);
    void* new_node = GetPage(cursor->table->pager, new_page_num);
    InitializeLeafNode(new_node);
    *NodeParent(new_node) = *NodeParent(old_node);
    *LeafNodeNextLeaf(new_node) = *LeafNodeNextLeaf(old_node);
    *LeafNodeNextLeaf(old_node) = new_page_num;

    for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; --i) {
        void* destination_node;
        if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
            destination_node = new_node;
        } else {
            destination_node = old_node;
        }
        uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
        void* destination = LeafNodeCell(destination_node, index_within_node);

        if (i == cursor->cell_num) {
            // SerializeRow(value, destination);
            SerializeRow(value, LeafNodeValue(destination_node, index_within_node));
            *LeafNodeKey(destination_node, index_within_node) = key;
        } else if (i > cursor->cell_num) {
            memcpy(destination, LeafNodeCell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
        } else {
            memcpy(destination, LeafNodeCell(old_node, i), LEAF_NODE_CELL_SIZE);
        }
    }

    *LeafNodeNumCells(old_node) = LEAF_NODE_LEFT_SPLIT_COUNT;
    *LeafNodeNumCells(new_node) = LEAF_NODE_RIGHT_SPLIT_COUNT;

    if (is_node_root(old_node)) {
        return CreateNewRoot(cursor->table, new_page_num);
    } else {
        // std::cout << "Need to implement updating parent after split" << std::endl;
        // exit(EXIT_FAILURE);
        uint32_t parent_page_num = *NodeParent(old_node);
        uint32_t new_max = GetNodeMaxKey(old_node);
        void* parent = GetPage(cursor->table->pager, parent_page_num);

        UpdateInternalNodeKey(parent, old_max, new_max);
        InternalNodeInsert(cursor->table, parent_page_num, new_page_num);
        return;
    }
}

uint32_t* LitDatabase::LeafNodeNextLeaf(void* node) {
    return static_cast<uint32_t*>(static_cast<void*>(static_cast<unsigned char*>(node) + LEAF_NODE_NEXT_LEAF_OFFSET));
}

Cursor* LitDatabase::LeafNodeFind(Table* table, uint32_t page_num, uint32_t key) {
    void* node = GetPage(table->pager, page_num);
    uint32_t num_cells = *LeafNodeNumCells(node);

    Cursor* cursor = static_cast<Cursor*>(malloc(sizeof(Cursor)));
    cursor->table = table;
    cursor->page_num = page_num;

    // binary search
    uint32_t min_index = 0;
    uint32_t one_past_max_index = num_cells;
    while (one_past_max_index != min_index) {
        uint32_t index = (min_index + one_past_max_index) / 2;
        uint32_t key_at_index = *LeafNodeKey(node, index);
        if (key == key_at_index) {
            cursor->cell_num = index;
            return cursor;
        } else if (key < key_at_index) {
            one_past_max_index = index;
        } else {
            min_index = index + 1;
        }
    }
    cursor->cell_num = min_index;
    return cursor;
}

uint32_t* LitDatabase::LeafNodeKey(void* node, uint32_t cell_num) {
    return static_cast<uint32_t*>(LeafNodeCell(node, cell_num));
}

void* LitDatabase::LeafNodeValue(void* node, uint32_t cell_num) {
    return static_cast<void*>(static_cast<unsigned char*>(LeafNodeCell(node, cell_num)) + LEAF_NODE_KEY_SIZE);
}

void LitDatabase::LeafNodeInsert(Cursor* cursor, uint32_t key, const Row& value) {
    void* node = GetPage(cursor->table->pager, cursor->page_num);

    uint32_t num_cells = *LeafNodeNumCells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        LeafNodeSplitAndInsert(cursor, key, value);
        return;
    }

    if (cursor->cell_num < num_cells) {
        // make room for new cell
        for (uint32_t i = num_cells; i > cursor->cell_num; --i) {
            memcpy(LeafNodeCell(node, i), LeafNodeCell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(LeafNodeNumCells(node)) += 1;
    *(LeafNodeKey(node, cursor->cell_num)) = key;
    SerializeRow(value, LeafNodeValue(node, cursor->cell_num));
}

uint32_t* LitDatabase::LeafNodeNumCells(void* node) {
    return static_cast<uint32_t*>(static_cast<void*>(static_cast<unsigned char*>(node) + LEAF_NODE_NUM_CELLS_OFFSET));
}

void* LitDatabase::LeafNodeCell(void* node, uint32_t cell_num) {
    return static_cast<void*>(static_cast<unsigned char*>(node) + LEAF_NODE_HEADER_SIZE +
                              cell_num * LEAF_NODE_CELL_SIZE);
}