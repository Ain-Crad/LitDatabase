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

// LitDatabase::Parse() {
//     assert(cur != nullptr);

//     ParseWhitespace();

//     if (*cur == '\0') {
//         std::cout << "Empty input" << std::endl;
//     } else if (*cur == '.') {
//         switch (ParseMeta()) {
//             case PARSE_META_SUCCESS: break;
//             case PARSE_STATEMENT_NEGATIVE_ID: break;
//         }
//     } else {
//         Statement statement;
//         switch (ParseStatement(&statement)) {
//             case PARSE_STATEMENT_SUCCESS: break;
//             case PARSE_STATEMENT_SYNTAX_ERROR:
//         }
//     }
// }

void LitDatabase::ParseWhitespace() {
    assert(cur != nullptr);
    while (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == '\r') ++cur;
}

ParseMetaResult LitDatabase::ParseMeta() {
    assert(cur != nullptr);

    if (strcmp(cur, ".exit") == 0) {
        input_buffer.clear();
        cur = nullptr;
        exit(EXIT_SUCCESS);
    } else {
        std::cout << "Unrecognized meta command" << std::endl;
        input_buffer.clear();
        cur = nullptr;
        return PARSE_META_UNRECOGNIZED;
    }

    return PARSE_META_SUCCESS;
}

ParseStatementResult LitDatabase::ParseStatement(Statement* statement) {
    if (strncmp(cur, "insert", 6) == 0) {
        statement->type = STATEMENT_INSERT;
        return PARSE_STATEMENT_SUCCESS;
    } else if (strncmp(cur, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;
        return PARSE_STATEMENT_SUCCESS;
    } else {
        std::cout << "Unrecognized keyword" << std::endl;
        return PARSE_STATEMENT_UNRECOGNIZED;
    }
}

ExecuteResult LitDatabase::ExecuteStatement(Statement* statement, Table* table) {
    switch (statement->type) {
        case STATEMENT_INSERT: return ExecuteInsert(statement, table);
        case STATEMENT_SELECT: return ExecuteSelect(statement, table);
    }
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

void LitDatabase::PrintRow(const Row& row) {
    // std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")" << std::endl;
}

void LitDatabase::SerializeRow(const Row& source, void* destination) {
    memcpy(static_cast<char*>(destination) + ID_OFFSET, &(source.id), ID_SIZE);
    memcpy(static_cast<char*>(destination) + USERNAME_OFFSET, &(source.username), USERNAME_SIZE);
    memcpy(static_cast<char*>(destination) + EMAIL_OFFSET, &(source.email), EMAIL_SIZE);
}

void LitDatabase::DeserializeRow(void* source, Row* destination) {
    memcpy(&(destination->id), static_cast<char*>(source) + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), static_cast<char*>(source) + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy((&destination->email), static_cast<char*>(source) + EMAIL_OFFSET, EMAIL_SIZE);
}

void* LitDatabase::RowSlot(Table* table, uint32_t row_num) {
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void* page = table->pages[page_num];
    if (page == nullptr) {
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return static_cast<void*>(static_cast<char*>(page) + byte_offset);
}