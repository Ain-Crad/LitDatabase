#include "LitDatabase.h"

void LitDatabase::PrintPrompt() { std::cout << "LitDb > "; }

void LitDatabase::ReadInput() {
    if (!std::getline(std::cin, input_buffer)) {
        std::cout << "Error reading input" << std::endl;
        exit(EXIT_FAILURE);
    }

    input_buffer.push_back('\0');
    cur = &input_buffer[0];
}

void LitDatabase::ParseWhitespace() {
    assert(cur != nullptr);
    while (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == '\r') ++cur;
}

ParseMetaResult LitDatabase::ParseMeta(Table* table) {
    assert(cur != nullptr);

    if (strcmp(cur, ".exit") == 0) {
        DbClose(table);
        exit(EXIT_SUCCESS);
    } else if (strcmp(cur, ".constants") == 0) {
        std::cout << "Constants: " << std::endl;
        PrintConstants();
        return PARSE_META_SUCCESS;
    } else if (strcmp(cur, ".btree") == 0) {
        std::cout << "Tree: " << std::endl;
        PrintTree(table->pager, 0, 0);
        return PARSE_META_SUCCESS;
    } else {
        cur = nullptr;
        return PARSE_META_UNRECOGNIZED;
    }

    return PARSE_META_SUCCESS;
}

ParseStatementResult LitDatabase::ParseStatement(Statement* statement) {
    if (strncmp(cur, "insert", 6) == 0) {
        return ParseInsert(statement);
    } else if (strncmp(cur, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;
        return PARSE_STATEMENT_SUCCESS;
    } else {
        return PARSE_STATEMENT_UNRECOGNIZED;
    }
}

ParseStatementResult LitDatabase::ParseInsert(Statement* statement) {
    statement->type = STATEMENT_INSERT;

    char* keyword = strtok(cur, " ");
    char* id_string = strtok(nullptr, " ");
    char* username = strtok(nullptr, " ");
    char* email = strtok(nullptr, " ");

    if (id_string == nullptr || username == nullptr || email == nullptr) {
        return PARSE_STATEMENT_SYNTAX_ERROR;
    }

    int id = atoi(id_string);
    if (id < 0) {
        return PARSE_STATEMENT_NEGATIVE_ID;
    }

    if (strlen(username) > COLUMN_USERNAME_SIZE) {
        return PARSE_STATEMENT_STRING_TOO_LONG;
    }
    if (strlen(email) > COLUMN_EMAIL_SIZE) {
        return PARSE_STATEMENT_STRING_TOO_LONG;
    }

    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email, email);

    return PARSE_STATEMENT_SUCCESS;
}

ExecuteResult LitDatabase::ExecuteStatement(Statement* statement, Table* table) {
    switch (statement->type) {
        case STATEMENT_INSERT: return ExecuteInsert(statement, table);
        case STATEMENT_SELECT: return ExecuteSelect(statement, table);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult LitDatabase::ExecuteInsert(Statement* statement, Table* table) {
    void* node = GetPage(table->pager, table->root_page_num);

    uint32_t num_cells = *LeafNodeNumCells(node);

    Row row = statement->row_to_insert;
    uint32_t key_to_insert = row.id;
    Cursor* cursor = TableFind(table, key_to_insert);
    if (cursor->cell_num < num_cells) {
        uint32_t key_at_index = *LeafNodeKey(node, cursor->cell_num);
        if (key_at_index == key_to_insert) {
            return EXECUTE_DUPLICATE_KEY;
        }
    }

    LeafNodeInsert(cursor, row.id, row);

    free(cursor);

    return EXECUTE_SUCCESS;
}

ExecuteResult LitDatabase::ExecuteSelect(Statement* statement, Table* table) {
    Cursor* cursor = TableStart(table);
    Row row;
    while (!(cursor->end_of_table)) {
        DeserializeRow(CursorValue(cursor), &row);
        PrintRow(row);
        CursorAdvance(cursor);
    }

    free(cursor);

    return EXECUTE_SUCCESS;
}

void LitDatabase::PrintRow(const Row& row) { printf("(%d, %s, %s)\n", row.id, row.username, row.email); }

void* LitDatabase::CursorValue(Cursor* cursor) {
    uint32_t page_num = cursor->page_num;
    void* page = GetPage(cursor->table->pager, page_num);
    return LeafNodeValue(page, cursor->cell_num);
}

void LitDatabase::SerializeRow(const Row& source, void* destination) {
    memcpy(static_cast<unsigned char*>(destination) + ID_OFFSET, &(source.id), ID_SIZE);
    memcpy(static_cast<unsigned char*>(destination) + USERNAME_OFFSET, &(source.username), USERNAME_SIZE);
    memcpy(static_cast<unsigned char*>(destination) + EMAIL_OFFSET, &(source.email), EMAIL_SIZE);
}

void LitDatabase::DeserializeRow(void* source, Row* destination) {
    memcpy(&(destination->id), static_cast<unsigned char*>(source) + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), static_cast<unsigned char*>(source) + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy((&destination->email), static_cast<unsigned char*>(source) + EMAIL_OFFSET, EMAIL_SIZE);
}

Table* LitDatabase::DbOpen(const char* filename) {
    file_name = filename;
    Pager* pager = PagerOpen();

    Table* table = new Table();
    table->pager = pager;
    table->root_page_num = 0;

    if (pager->num_pages == 0) {
        void* root_node = GetPage(pager, 0);
        InitializeLeafNode(root_node);
        set_node_root(root_node, true);
    }

    return table;
}

Pager* LitDatabase::PagerOpen() {
    // windows
    // assert(file_name != nullptr);
    // std::fstream* fd = new std::fstream();
    // fd->open(file_name, std::fstream::in | std::fstream::binary);
    // if (fd->fail()) {
    //     std::cout << "Create new database file." << std::endl;
    //     fd->close();
    //     fd->clear();

    //     fd->open(file_name, std::fstream::out);
    //     fd->close();
    //     fd->clear();

    //     fd->open(file_name, std::fstream::in);
    // } else {
    //     std::cout << "Load database file." << std::endl;
    //     fd->seekg(0, std::fstream::end);
    //     int file_length = fd->tellg();
    //     std::cout << "file_length: " << file_length << std::endl;
    // }

    // fd->seekg(0, std::fstream::end);
    // int file_length = fd->tellg();
    // fd->close();
    // fd->clear();

    // Pager* pager = new Pager();
    // pager->fd = fd;
    // pager->file_length = file_length;
    // pager->num_pages = file_length / PAGE_SIZE;
    // if (file_length % PAGE_SIZE != 0) {
    //     std::cout << "file_length: " << file_length << " PAGE_SIZE: " << PAGE_SIZE << std::endl;
    //     std::cout << "Db file is not a whole number of pages. Corrupt file." << std::endl;
    //     exit(EXIT_FAILURE);
    // }

    // return pager;

    // linux
    int file_descriptor = open(file_name, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (file_descriptor == -1) {
        printf("unable to open file\n");
        exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(file_descriptor, 0, SEEK_END);
    if (file_length == 0) {
        std::cout << "Create new database file." << std::endl;
    } else {
        std::cout << "Load database file." << std::endl;
        std::cout << "File size: " << file_length << std::endl;
    }

    Pager* pager = new Pager();
    pager->file_descriptor = file_descriptor;
    pager->file_length = file_length;
    pager->num_pages = (file_length / PAGE_SIZE);

    if (file_length % PAGE_SIZE != 0) {
        printf("db file is not a whole number of pages\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; ++i) {
        pager->pages[i] = nullptr;
    }

    return pager;
}

void* LitDatabase::GetPage(Pager* pager, uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        std::cout << "Tried to fetch page number out of bounds." << std::endl;
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == nullptr) {
        // Cache miss, allocate memory and load from file
        void* page = malloc(PAGE_SIZE);
        uint32_t num_pages = pager->file_length / PAGE_SIZE;
        if (pager->file_length % PAGE_SIZE) {
            num_pages += 1;
        }

        if (page_num < num_pages) {
            // windows
            // pager->fd->open(file_name, std::fstream::in);
            // pager->fd->seekg(page_num * PAGE_SIZE, std::fstream::beg);
            // pager->fd->read(static_cast<char*>(page), PAGE_SIZE);
            // if (pager->fd->fail()) {
            //     std::cout << "Read failed" << std::endl;
            // }

            // pager->fd->close();
            // pager->fd->clear();

            // linux
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytes_read == -1) {
                printf("Error reading file\n");
                exit(EXIT_FAILURE);
            }
        }

        pager->pages[page_num] = page;

        if (page_num >= pager->num_pages) {
            pager->num_pages = page_num + 1;
        }
    }

    return pager->pages[page_num];
}

void LitDatabase::DbClose(Table* table) {
    Pager* pager = table->pager;

    for (uint32_t i = 0; i < pager->num_pages; ++i) {
        if (pager->pages[i] == nullptr) continue;
        PagerFlush(pager, i);
    }

    // windows
    // if (pager->fd->is_open()) pager->fd->close();
    // if (pager->fd->fail()) {
    //     std::cout << "Error closing db file" << std::endl;
    //     exit(EXIT_FAILURE);
    // }

    // linux
    int result = close(pager->file_descriptor);
    if (result == -1) {
        printf("Error closing db file\n");
        exit(EXIT_FAILURE);
    }

    delete table;
}

void LitDatabase::PagerFlush(Pager* pager, uint32_t page_num) {
    // windows--bugs here!!!
    // pager->fd->open(file_name, std::fstream::out | std::fstream::binary);
    // if (pager->pages[page_num] == nullptr) {
    //     std::cout << "Tried to flush null page." << std::endl;
    //     exit(EXIT_FAILURE);
    // }

    // pager->fd->seekp(page_num * PAGE_SIZE, std::fstream::beg);
    // if (pager->fd->fail()) {
    //     std::cout << "Error seeking." << std::endl;
    //     exit(EXIT_FAILURE);
    // }

    // pager->fd->write(static_cast<char*>(pager->pages[page_num]), PAGE_SIZE);
    // if (pager->fd->fail()) {
    //     std::cout << "Error writing." << std::endl;
    //     exit(EXIT_FAILURE);
    // }
    // pager->fd->close();

    // linux
    if (pager->pages[page_num] == nullptr) {
        std::cout << "Tried to flush null page" << std::endl;
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);

    if (offset == -1) {
        printf("Error seeking\n");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);

    if (bytes_written == -1) {
        printf("Error writing\n");
        exit(EXIT_FAILURE);
    }
}

Cursor* LitDatabase::TableStart(Table* table) {
    Cursor* cursor = TableFind(table, 0);

    void* node = GetPage(table->pager, cursor->page_num);
    uint32_t num_cells = *LeafNodeNumCells(node);
    cursor->end_of_table = (num_cells == 0);

    return cursor;
}

// return the position of the given key
Cursor* LitDatabase::TableFind(Table* table, uint32_t key) {
    uint32_t root_page_num = table->root_page_num;
    void* root_node = GetPage(table->pager, root_page_num);
    if (get_node_type(root_node) == NODE_LEAF) {
        return LeafNodeFind(table, root_page_num, key);
    } else {
        return InternalNodeFind(table, root_page_num, key);
    }
}

void LitDatabase::CursorAdvance(Cursor* cursor) {
    uint32_t page_num = cursor->page_num;
    void* node = GetPage(cursor->table->pager, page_num);

    cursor->cell_num += 1;
    if (cursor->cell_num >= (*LeafNodeNumCells(node))) {
        // cursor->end_of_table = true;
        uint32_t next_page_num = *LeafNodeNextLeaf(node);
        if (next_page_num == 0) {
            cursor->end_of_table = true;
        } else {
            cursor->page_num = next_page_num;
            cursor->cell_num = 0;
        }
    }
}

void LitDatabase::PrintConstants() {
    std::cout << "ROW_SIZE: " << ROW_SIZE << std::endl;
    std::cout << "COMMON_NODE_HEADER_SIZE: " << static_cast<uint32_t>(COMMON_NODE_HEADER_SIZE) << std::endl;
    std::cout << "LEAF_NODE_HEADER_SIZE: " << LEAF_NODE_HEADER_SIZE << std::endl;
    std::cout << "LEAF_NODE_CELL_SIZE: " << LEAF_NODE_CELL_SIZE << std::endl;
    std::cout << "LEAF_NODE_MAX_CELLS: " << LEAF_NODE_MAX_CELLS << std::endl;
}

void LitDatabase::Indent(uint32_t level) {
    for (uint32_t i = 0; i < level; ++i) {
        std::cout << " ";
    }
}
void LitDatabase::PrintTree(Pager* pager, uint32_t page_num, uint32_t indentation_level) {
    void* node = GetPage(pager, page_num);
    uint32_t num_keys, child;
    switch (get_node_type(node)) {
        case NODE_LEAF:
            num_keys = *LeafNodeNumCells(node);
            Indent(indentation_level);
            std::cout << "- leaf (size " << num_keys << ")" << std::endl;
            for (uint32_t i = 0; i < num_keys; ++i) {
                Indent(indentation_level + 1);
                std::cout << "- " << *LeafNodeKey(node, i) << std::endl;
            }
            break;
        case NODE_INTERNAL:
            num_keys = *InternalNodeNumKeys(node);
            Indent(indentation_level);
            std::cout << "- internal (size " << num_keys << ")" << std::endl;
            for (uint32_t i = 0; i < num_keys; ++i) {
                child = *InternalNodeChild(node, i);
                PrintTree(pager, child, indentation_level + 1);

                Indent(indentation_level + 1);
                std::cout << "- key " << *InternalNodeKey(node, i) << std::endl;
            }
            child = *InternalNodeRightChild(node);
            PrintTree(pager, child, indentation_level + 1);
            break;
    }
}

NodeType LitDatabase::get_node_type(void* node) {
    uint8_t value = *static_cast<uint8_t*>(static_cast<void*>(static_cast<unsigned char*>(node) + NODE_TYPE_OFFSET));
    return static_cast<NodeType>(value);
}
void LitDatabase::set_node_type(void* node, NodeType type) {
    uint8_t value = type;
    *static_cast<uint8_t*>(static_cast<void*>(static_cast<unsigned char*>(node) + NODE_TYPE_OFFSET)) = value;
}

void LitDatabase::CreateNewRoot(Table* table, uint32_t right_child_page_num) {
    void* root = GetPage(table->pager, table->root_page_num);
    void* right_child = GetPage(table->pager, right_child_page_num);
    uint32_t left_child_page_num = GetUnusedPageNum(table->pager);
    void* left_child = GetPage(table->pager, left_child_page_num);

    memcpy(left_child, root, PAGE_SIZE);
    set_node_root(left_child, false);

    InitializeInternalNode(root);
    set_node_root(root, true);
    *InternalNodeNumKeys(root) = 1;
    *InternalNodeChild(root, 0) = left_child_page_num;
    uint32_t left_child_max_key = GetNodeMaxKey(left_child);
    *InternalNodeKey(root, 0) = left_child_max_key;
    *InternalNodeRightChild(root) = right_child_page_num;

    *NodeParent(left_child) = table->root_page_num;
    *NodeParent(right_child) = table->root_page_num;
}

uint32_t LitDatabase::GetNodeMaxKey(void* node) {
    switch (get_node_type(node)) {
        case NODE_INTERNAL: return *InternalNodeKey(node, *InternalNodeNumKeys(node) - 1);
        case NODE_LEAF: return *LeafNodeKey(node, *LeafNodeNumCells(node) - 1);
    }
}

uint32_t LitDatabase::GetUnusedPageNum(Pager* pager) { return pager->num_pages; }

bool LitDatabase::is_node_root(void* node) {
    uint8_t value = *static_cast<uint8_t*>(static_cast<void*>(static_cast<unsigned char*>(node) + IS_ROOT_OFFSET));
    return value;
}
void LitDatabase::set_node_root(void* node, bool is_root) {
    uint8_t value = is_root;
    *static_cast<uint8_t*>(static_cast<void*>(static_cast<unsigned char*>(node) + IS_ROOT_OFFSET)) = value;
}