#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace sclc {
    struct Enum {
        std::string name;
        std::unordered_map<std::string, long> members;
        std::unordered_map<long, std::string> membersReverse;
        long nextValue = 0;
        Token* name_token;
        static const size_t npos = -1;

        Enum(std::string name) {
            this->name = name;
        }
        void addMember(std::string member, long value) {
            if (membersReverse.find(value) != membersReverse.end()) {
                throw std::runtime_error("Enum member '" + member + "' has already been defined with value " + std::to_string(value) + " in enum '" + name + "'");
            }
            members[member] = value;
            membersReverse[value] = member;
            nextValue = value + 1;
        }
        size_t indexOf(std::string name) {
            return members.find(name) != members.end() ? members[name] : npos;
        }
    };
} // namespace sclc
