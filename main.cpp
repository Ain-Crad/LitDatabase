#include <iostream>
#include <string>

#include "LitDatabase.h"

int main() {
    LitDatabase lit_db;
    while (true) {
        lit_db.PrintPrompt();
        lit_db.ReadInput();
        // lit_db.Parse();

        assert(lit_db.cur != nullptr);
        if (*lit_db.cur == '\0') {
            std::cout << "Empty input" << std::endl;
            continue;
        }

        if (*lit_db.cur == '.') {
            switch (lit_db.ParseMeta()) {
                case PARSE_META_SUCCESS: continue;
                case PARSE_META_UNRECOGNIZED:
                    std::cout << "Unrecognized command: " << lit_db.get_input_buffer() << std::endl;
                    continue;
            }
        }

        Statement statement;
        Table table;
        switch (lit_db.ParseStatement(&statement)) {
            case PARSE_STATEMENT_SUCCESS: break;
            case PARSE_STATEMENT_SYNTAX_ERROR:
                std::cout << "Syntax error. Could not parse statement." << std::endl;
                continue;
            case PARSE_STATEMENT_UNRECOGNIZED:
                std::cout << "Unrecognized keyword at start of " << lit_db.get_input_buffer() << std::endl;
                continue;
        }

        switch (lit_db.ExecuteStatement(&statement, &table)) {
            case EXECUTE_SUCCESS: std::cout << "Executed." << std::endl; break;
            case EXECUTE_TABLE_FULL: std::cout << "Error: Table full." << std::endl; break;
        }
    }
}