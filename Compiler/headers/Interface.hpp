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
        std::vector<Ptr<Function>> toImplement;
        std::vector<Ptr<Method>> defaultImplementations;
        Token name_token;
        Interface(std::string name);
        bool hasToImplement(std::string func);
        bool hasDefaultImplementation(std::string func);
        Ptr<Function> getToImplement(std::string func);
        Ptr<Method> getDefaultImplementation(std::string func);
        void addToImplement(Ptr<Function> func);
        void addDefaultImplementation(Ptr<Method> func);

        bool operator==(const Interface& other) const;
        bool operator!=(const Interface& other) const;
    };
} // namespace sclc
