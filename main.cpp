#include <iostream>
#include <string>

#include "LitDatabase.h"

int main() {
    LitDatabase lit_db;
    while (true) {
        lit_db.PrintPrompt();
        lit_db.ReadInput();
        lit_db.Parse();
    }
}