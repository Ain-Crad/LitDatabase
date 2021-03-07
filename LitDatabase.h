#ifndef LITDATABASE_H
#define LITDATABASE_H

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

constexpr uint32_t COLUMN_USERNAME_SIZE = 32;
constexpr uint32_t COLUMN_EMAIL_SIZE = 255;

enum ParseMetaResult { PARSE_META_SUCCESS, PARSE_META_UNRECOGNIZED };
enum ParseStatementResult {
    PARSE_STATEMENT_SUCCESS,
    PARSE_STATEMENT_UNRECOGNIZED,
    PARSE_STATEMENT_STRING_TOO_LONG,
    PARSE_STATEMENT_SYNTAX_ERROR,
    PARSE_STATEMENT_NEGATIVE_ID
};
enum StatementType { STATEMENT_INSERT, STATEMENT_SELECT };
enum ExecuteResult { EXECUTE_SUCCESS, EXECUTE_TABLE_FULL };

struct Row {
    uint32_t id;
    // std::string username;
    // std::string email;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
};

constexpr uint32_t ID_SIZE = sizeof(uint32_t);
constexpr uint32_t USERNAME_SIZE = COLUMN_USERNAME_SIZE + 1;
constexpr uint32_t EMAIL_SIZE = COLUMN_EMAIL_SIZE + 1;
constexpr uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

constexpr uint32_t PAGE_SIZE = 4096;
constexpr uint32_t TABLE_MAX_PAGES = 100;
constexpr uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

struct Table {
    Table() {
        num_rows = 0;
        for (int i = 0; i < TABLE_MAX_PAGES; ++i) {
            pages[i] = nullptr;
        }
    }
    ~Table() {
        for (int i = 0; i < TABLE_MAX_PAGES; ++i) {
            if (pages[i]) free(pages[i]);
        }
    }

    uint32_t num_rows;
    void* pages[TABLE_MAX_PAGES];
};

struct Statement {
    StatementType type;
    Row row_to_insert;
};

class LitDatabase {
public:
    // LitDatabase() { statement.type = STATEMENT_INVALID; }

    void PrintPrompt();
    void ReadInput();
    void Parse();
    ParseMetaResult ParseMeta();
    ParseStatementResult ParseStatement(Statement*);
    ExecuteResult ExecuteStatement(Statement* statement, Table* table);

    void* RowSlot(Table* table, uint32_t row_num);

    std::string get_input_buffer() { return input_buffer; }

    const char* cur = nullptr;

private:
    std::string input_buffer;
    // Statement statement;

    void ParseWhitespace();

    void PrintRow(const Row& row);
    void SerializeRow(const Row& source, void* destination);
    void DeserializeRow(void* source, Row* destination);
    ExecuteResult ExecuteInsert(Statement* statement, Table* table);
    ExecuteResult ExecuteSelect(Statement* statement, Table* table);
};

#endif
