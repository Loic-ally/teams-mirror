#include "parser.hpp"
#include <iostream>
#include <sstream>
#include <string>

namespace client {
    ParsedInput::ParsedInput() {
        std::string line;
        std::getline(std::cin, line);
        _arguments.str(line);
        _arguments >> std::ws >> _command;
    }
}