#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "Token.hpp"

namespace sclc {
    struct Enum {
        std::string name;
        std::unordered_map<std::string, long> members;
        long nextValue = 0;
        Token* name_token;
        static const size_t npos;

        Enum(std::string name);
        void addMember(std::string member, long value);
        size_t indexOf(std::string name);
        bool hasMember(std::string name);
    };
} // namespace sclc
