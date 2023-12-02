#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace sclc {
    struct Enum {
        std::string name;
        std::unordered_map<std::string, long> members;
        long nextValue = 0;
        Token* name_token;
        static const size_t npos = -1;

        Enum(std::string name) {
            this->name = name;
        }
        void addMember(std::string member, long value) {
            members[member] = value;
            nextValue = value + 1;
        }
        size_t indexOf(std::string name) {
            return members.find(name) != members.end() ? members[name] : npos;
        }
        bool hasMember(std::string name) {
            return members.find(name) != members.end();
        }
    };
} // namespace sclc
