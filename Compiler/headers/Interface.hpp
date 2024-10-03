#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include <Function.hpp>

namespace sclc
{
    struct Interface {
        std::string name;
        std::vector<Function*> toImplement;
        std::vector<Method*> defaultImplementations;
        Token* name_token;
        Interface(std::string name);
        bool hasToImplement(std::string func);
        bool hasDefaultImplementation(std::string func);
        Function* getToImplement(std::string func);
        Method* getDefaultImplementation(std::string func);
        void addToImplement(Function* func);
        void addDefaultImplementation(Method* func);

        bool operator==(const Interface& other) const;
        bool operator!=(const Interface& other) const;
    };
} // namespace sclc
