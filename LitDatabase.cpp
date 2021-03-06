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

void LitDatabase::Parse() {
    assert(cur != nullptr);

    ParseWhitespace();
    switch (*cur) {
        case '\0': std::cout << "Empty input" << std::endl; break;
        case '.': ParseMeta(); break;
        default:
            if (ParseStatement() == PARSE_STATEMENT_SUCCESS) ExecuteStatement();
            break;
    }
}

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

ParseStatementResult LitDatabase::ParseStatement() {
    if (strncmp(cur, "insert", 6) == 0) {
        statement.type = STATEMENT_INSERT;
        return PARSE_STATEMENT_SUCCESS;
    } else if (strncmp(cur, "select", 6) == 0) {
        statement.type = STATEMENT_SELECT;
        return PARSE_STATEMENT_SUCCESS;
    } else {
        std::cout << "Unrecognized keyword" << std::endl;
        return PARSE_STATEMENT_UNRECOGNIZED;
    }
}

void LitDatabase::ExecuteStatement() {
    switch (statement.type) {
        case STATEMENT_INSERT: std::cout << "execute insert" << std::endl; break;
        case STATEMENT_SELECT: std::cout << "execute select" << std::endl; break;
        case STATEMENT_INVALID: std::cout << "invalid statement" << std::endl; break;
    }
}