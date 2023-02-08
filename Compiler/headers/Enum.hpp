#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace sclc {
    class Enum {
        std::string name;
        std::vector<std::string> members;
    public:
        static const size_t npos = -1;

        Enum(std::string name) {
            this->name = name;
        }
        std::vector<std::string> getMembers() {
            return members;
        }
        void addMember(std::string member) {
            members.push_back(member);
        }
        std::string getName() {
            return name;
        }
        void setName(std::string name) {
            this->name = name;
        }
        size_t indexOf(std::string name) {
            for (size_t i = 0; i < members.size(); i++) {
                if (members[i] == name) {
                    return i;
                }
            }
            
            return npos;
        }
    };
} // namespace sclc
