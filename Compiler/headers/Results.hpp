#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include "Function.hpp"
#include "Interface.hpp"
#include "Variable.hpp"
#include "Container.hpp"
#include "Struct.hpp"

namespace sclc
{
    class FPResult {
    public:
        bool success;
        bool isNote;
        std::string message;
        std::string in;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::string value;
        int column;
        int line;
        TokenType type;
    };

    class TPResult {
    public:
        std::vector<Function*> functions;
        std::vector<Function*> extern_functions;
        std::vector<Interface*> interfaces;
        std::vector<Variable> extern_globals;
        std::vector<Variable> globals;
        std::vector<Container> containers;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::vector<Struct> structs;
        std::unordered_map<std::string, std::string> typealiases;
    };
} // namespace sclc
