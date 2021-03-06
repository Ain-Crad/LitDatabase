#include "LitDatabase.h"

void LitDatabase::PrintPrompt() { std::cout << "LitDb > "; }

void LitDatabase::ReadInput() {
    if (!std::getline(std::cin, input_buffer)) {
        std::cout << "Error reading input" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void LitDatabase::Parse() {
    if (input_buffer == ".exit") {
        input_buffer.clear();
        exit(EXIT_SUCCESS);
    } else {
        std::cout << "Unrecognized command: " << input_buffer << std::endl;
    }
}