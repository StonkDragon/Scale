#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

namespace sclc
{
    struct Container {
        std::string name;
        std::vector<Variable> members;
        Token* name_token;
        Container(std::string name) {
            this->name = name;
        }
        void addMember(Variable member) {
            members.push_back(member);
        }
        bool hasMember(std::string member) {
            for (Variable& m : members) {
                if (m.name == member) {
                    return true;
                }
            }
            return false;
        }
        std::string getMemberType(std::string member) {
            for (Variable& m : members) {
                if (m.name == member) {
                    return m.type;
                }
            }
            return "";
        }
        Variable getMember(std::string member) {
            for (Variable& m : members) {
                if (m.name == member) {
                    return m;
                }
            }
            return Variable("", "");
        }

        bool operator==(const Container& other) const {
            return name == other.name;
        }

        bool operator!=(const Container& other) const {
            return name != other.name;
        }
    };
} // namespace sclc
