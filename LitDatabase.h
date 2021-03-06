#ifndef LITDATABASE_H
#define LITDATABASE_H

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

enum ParseMetaResult { PARSE_META_SUCCESS, PARSE_META_UNRECOGNIZED };
enum ParseStatementResult { PARSE_STATEMENT_SUCCESS, PARSE_STATEMENT_UNRECOGNIZED };
enum StatementType { STATEMENT_INVALID, STATEMENT_INSERT, STATEMENT_SELECT };

struct Statement {
    StatementType type;
};

class LitDatabase {
public:
    LitDatabase() { statement.type = STATEMENT_INVALID; }

    void PrintPrompt();
    void ReadInput();
    void Parse();
    void ExecuteStatement();

private:
    std::string input_buffer;
    const char* cur = nullptr;
    Statement statement;

    void ParseWhitespace();
    ParseMetaResult ParseMeta();
    ParseStatementResult ParseStatement();
};

#endif
