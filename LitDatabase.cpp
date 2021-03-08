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
    if (table->num_rows >= TABLE_MAX_ROWS) return EXECUTE_TABLE_FULL;

    Row* row = &(statement->row_to_insert);
    SerializeRow(*row, RowSlot(table, table->num_rows));
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}

ExecuteResult LitDatabase::ExecuteSelect(Statement* statement, Table* table) {
    Row row;
    for (uint32_t i = 0; i < table->num_rows; ++i) {
        DeserializeRow(RowSlot(table, i), &row);
        PrintRow(row);
    }
    return EXECUTE_SUCCESS;
}

void LitDatabase::PrintRow(const Row& row) { printf("(%d, %s, %s)\n", row.id, row.username, row.email); }

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

void* LitDatabase::RowSlot(Table* table, uint32_t row_num) {
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void* page = GetPage(table->pager, page_num);
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return static_cast<void*>(static_cast<unsigned char*>(page) + byte_offset);
}

Table* LitDatabase::DbOpen(const char* filename) {
    file_name = filename;
    Pager* pager = PagerOpen();
    uint32_t num_rows = pager->file_length / ROW_SIZE;

    Table* table = new Table();
    table->pager = pager;
    table->num_rows = num_rows;

    return table;
}

Pager* LitDatabase::PagerOpen() {
    assert(file_name != nullptr);
    std::fstream* fd = new std::fstream();
    fd->open(file_name, std::fstream::in);
    if (fd->fail()) {
        std::cout << "Create new database file." << std::endl;
        fd->close();
        fd->clear();

        fd->open(file_name, std::fstream::out);
        fd->close();
        fd->clear();

        fd->open(file_name, std::fstream::in);
    } else {
        std::cout << "Load database file." << std::endl;
    }

    fd->seekg(0, std::fstream::end);
    int file_length = fd->tellp();
    fd->close();
    fd->clear();

    Pager* pager = new Pager();
    pager->fd = fd;
    pager->file_length = file_length;

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
            pager->fd->open(file_name, std::fstream::in);
            pager->fd->seekg(page_num * PAGE_SIZE, std::fstream::beg);
            pager->fd->read(static_cast<char*>(page), PAGE_SIZE);
            // if (pager->fd->fail()) {
            //     std::cout << "Error reading file" << std::endl;
            //     exit(EXIT_FAILURE);
            // }
            pager->fd->close();
            pager->fd->clear();
        }
        pager->pages[page_num] = page;
    }

    return pager->pages[page_num];
}

void LitDatabase::DbClose(Table* table) {
    Pager* pager = table->pager;
    uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

    for (uint32_t i = 0; i < num_full_pages; ++i) {
        if (pager->pages[i] == nullptr) continue;
        PagerFlush(pager, i, PAGE_SIZE);
        free(pager->pages[i]);
        pager->pages[i] = nullptr;
    }

    // deal with partial page
    uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    if (num_additional_rows > 0) {
        uint32_t page_num = num_full_pages;
        if (pager->pages[page_num] != nullptr) {
            PagerFlush(pager, page_num, num_additional_rows * ROW_SIZE);
            free(pager->pages[page_num]);
            pager->pages[page_num] = nullptr;
        }
    }

    if (pager->fd->is_open()) pager->fd->close();
    if (pager->fd->fail()) {
        std::cout << "Error closing db file" << std::endl;
        exit(EXIT_FAILURE);
    }

    delete table;
}

void LitDatabase::PagerFlush(Pager* pager, uint32_t page_num, uint32_t sz) {
    pager->fd->open(file_name, std::fstream::out);
    if (pager->pages[page_num] == nullptr) {
        std::cout << "Tried to flush null page." << std::endl;
        exit(EXIT_FAILURE);
    }

    pager->fd->seekp(page_num * PAGE_SIZE, std::fstream::beg);
    if (pager->fd->fail()) {
        std::cout << "Error seeking." << std::endl;
        exit(EXIT_FAILURE);
    }
    pager->fd->write(static_cast<char*>(pager->pages[page_num]), sz);
    if (pager->fd->fail()) {
        std::cout << "Error writing." << std::endl;
        exit(EXIT_FAILURE);
    }
}