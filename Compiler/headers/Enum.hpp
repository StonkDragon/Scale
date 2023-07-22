#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace sclc {
    class Enum {
        std::string name;
        std::unordered_map<std::string, long> members;
        std::unordered_map<long, std::string> membersReverse;
    public:
        long nextValue = 0;
        Token* name_token;
        static const size_t npos = -1;

        Enum(std::string name) {
            this->name = name;
        }
        std::unordered_map<std::string, long> getMembers() {
            return members;
        }
        void addMember(std::string member, long value) {
            if (membersReverse.find(value) != membersReverse.end()) {
                throw std::runtime_error("Enum member '" + member + "' has already been defined with value " + std::to_string(value) + " in enum '" + name + "'");
            }
            members[member] = value;
            membersReverse[value] = member;
            nextValue = value + 1;
        }
        std::string getName() {
            return name;
        }
        void setName(std::string name) {
            this->name = name;
        }
        size_t indexOf(std::string name) {
            return members.find(name) != members.end() ? members[name] : npos;
        }
    };
} // namespace sclc
