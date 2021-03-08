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
enum ExecuteResult { EXECUTE_SUCCESS, EXECUTE_TABLE_FULL, EXECUTE_DUPLICATE_KEY };

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
    uint32_t num_pages;
    void* pages[TABLE_MAX_PAGES];
};

struct Table {
    Table() : root_page_num(0), pager(nullptr) {}
    ~Table() { delete pager; }

    // uint32_t num_rows;
    uint32_t root_page_num;
    Pager* pager;
};

struct Statement {
    StatementType type;
    Row row_to_insert;
};

struct Cursor {
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table;  // the position one past the last element
};

enum NodeType { NODE_INTERNAL, NODE_LEAF };
// common node header layout
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

// leaf node header layout
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

// leaf node body layout
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

class LitDatabase {
public:
    void PrintPrompt();

    void ReadInput();
    ParseMetaResult ParseMeta(Table*);
    ParseStatementResult ParseStatement(Statement*);
    ParseStatementResult ParseInsert(Statement*);
    ExecuteResult ExecuteStatement(Statement* statement, Table* table);

    Table* DbOpen(const char* filename);
    Pager* PagerOpen();
    void* GetPage(Pager* pager, uint32_t page_num);
    void DbClose(Table* table);
    void PagerFlush(Pager* pager, uint32_t page_num);

    Cursor* TableStart(Table* table);
    // Cursor* TableEnd(Table* table);
    Cursor* TableFind(Table* table, uint32_t key);
    void* CursorValue(Cursor* cursor);
    void CursorAdvance(Cursor* cursor);

    uint32_t* LeafNodeNumCells(void* node);
    void* LeafNodeCell(void* node, uint32_t cell_num);
    uint32_t* LeafNodeKey(void* node, uint32_t cell_num);
    void* LeafNodeValue(void* node, uint32_t cell_num);
    void LeafNodeInsert(Cursor* cursor, uint32_t key, const Row& value);
    void InitializeLeafNode(void* node);
    Cursor* LeafNodeFind(Table* table, uint32_t page_num, uint32_t key);

    NodeType get_node_type(void* node);
    void set_node_type(void* node, NodeType type);

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

    void PrintConstants();
    void PrintLeafNode(void* node);
};

#endif
