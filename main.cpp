#include <iostream>
#include <string>

#include "LitDatabase.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Must supply a database filename." << std::endl;
        exit(EXIT_FAILURE);
    }

    char* filename = argv[1];

    LitDatabase lit_db;
    Table* table = lit_db.DbOpen(filename);

    while (true) {
        lit_db.PrintPrompt();
        lit_db.ReadInput();

        assert(lit_db.cur != nullptr);
        if (*lit_db.cur == '\0') {
            std::cout << "Empty input" << std::endl;
            continue;
        }

        if (*lit_db.cur == '.') {
            switch (lit_db.ParseMeta(table)) {
                case PARSE_META_SUCCESS: continue;
                case PARSE_META_UNRECOGNIZED:
                    std::cout << "Unrecognized command: " << lit_db.get_input_buffer() << std::endl;
                    continue;
            }
        }

        Statement statement;
        switch (lit_db.ParseStatement(&statement)) {
            case PARSE_STATEMENT_SUCCESS: break;
            case PARSE_STATEMENT_NEGATIVE_ID: std::cout << "ID must be positive." << std::endl;
            case PARSE_STATEMENT_STRING_TOO_LONG: std::cout << "String is too long." << std::endl; continue;
            case PARSE_STATEMENT_SYNTAX_ERROR:
                std::cout << "Syntax error. Could not parse statement." << std::endl;
                continue;
            case PARSE_STATEMENT_UNRECOGNIZED:
                std::cout << "Unrecognized keyword at start of " << lit_db.get_input_buffer() << std::endl;
                continue;
        }

        switch (lit_db.ExecuteStatement(&statement, table)) {
            case EXECUTE_SUCCESS: std::cout << "Executed." << std::endl; break;
            case EXECUTE_TABLE_FULL: std::cout << "Error: Table full." << std::endl; break;
            case EXECUTE_DUPLICATE_KEY: std::cout << "Error: Duplicate key." << std::endl; break;
        }
    }
}