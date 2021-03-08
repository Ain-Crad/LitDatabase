#ifndef LITDATABASE_H
#define LITDATABASE_H

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
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
    uint32_t id = -1;
    char username[COLUMN_USERNAME_SIZE + 1] = {'\0'};
    char email[COLUMN_EMAIL_SIZE + 1] = {'\0'};
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

struct Pager {
    Pager() : fd(nullptr), file_length(0) {
        for (uint32_t i = 0; i < TABLE_MAX_PAGES; ++i) {
            pages[i] = nullptr;
        }
    }

    Pager(std::fstream* _fd, uint32_t _len) : fd(_fd), file_length(_len) {
        for (uint32_t i = 0; i < TABLE_MAX_PAGES; ++i) {
            pages[i] = nullptr;
        }
    }

    ~Pager() {
        delete fd;
        for (uint32_t i = 0; i < TABLE_MAX_PAGES; ++i) {
            if (pages[i]) free(pages[i]);
        }
    }

    std::fstream* fd;
    uint32_t file_length;
    void* pages[TABLE_MAX_PAGES];
};

struct Table {
    Table() : num_rows(0), pager(nullptr) {}
    ~Table() { delete pager; }

    uint32_t num_rows;
    Pager* pager;
};

struct Statement {
    StatementType type;
    Row row_to_insert;
};

struct Cursor {
    Table* table;
    uint32_t row_num;
    bool end_of_table;  // the position one past the last element
};

class LitDatabase {
public:
    // LitDatabase() { statement.type = STATEMENT_INVALID; }

    void PrintPrompt();
    void ReadInput();
    ParseMetaResult ParseMeta(Table*);
    ParseStatementResult ParseStatement(Statement*);
    ParseStatementResult ParseInsert(Statement*);
    ExecuteResult ExecuteStatement(Statement* statement, Table* table);

    // void* RowSlot(Table* table, uint32_t row_num);
    Table* DbOpen(const char* filename);
    Pager* PagerOpen();
    void* GetPage(Pager* pager, uint32_t page_num);
    void DbClose(Table* table);
    void PagerFlush(Pager* pager, uint32_t page_num, uint32_t sz);

    Cursor* TableStart(Table* table);
    Cursor* TableEnd(Table* table);
    void* CursorValue(Cursor* cursor);
    void CursorAdvance(Cursor* cursor);

    std::string get_input_buffer() { return input_buffer; }

    char* cur = nullptr;
    const char* file_name = nullptr;

private:
    std::string input_buffer;

    void ParseWhitespace();

    void PrintRow(const Row& row);
    void SerializeRow(const Row& source, void* destination);
    void DeserializeRow(void* source, Row* destination);
    ExecuteResult ExecuteInsert(Statement* statement, Table* table);
    ExecuteResult ExecuteSelect(Statement* statement, Table* table);
};

#endif
