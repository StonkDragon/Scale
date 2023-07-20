#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

namespace sclc
{
    class Container {
        std::string name;
        std::vector<Variable> members;
    public:
        Token* name_token;
        Container(std::string name) {
            this->name = name;
        }
        void addMember(Variable member) {
            members.push_back(member);
        }
        bool hasMember(std::string member) {
            for (Variable& m : members) {
                if (m.getName() == member) {
                    return true;
                }
            }
            return false;
        }
        std::string getName() { return name; }
        std::vector<Variable> getMembers() { return members; }
        std::string getMemberType(std::string member) {
            for (Variable& m : members) {
                if (m.getName() == member) {
                    return m.getType();
                }
            }
            return "";
        }
        Variable getMember(std::string member) {
            for (Variable& m : members) {
                if (m.getName() == member) {
                    return m;
                }
            }
            return Variable("", "");
        }
    };
} // namespace sclc
