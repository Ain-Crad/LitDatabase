#ifndef LITDATABASE_H
#define LITDATABASE_H

#include <iostream>
#include <string>

class LitDatabase {
public:
    void PrintPrompt();
    void ReadInput();
    void Parse();

private:
    std::string input_buffer;
};

#endif
