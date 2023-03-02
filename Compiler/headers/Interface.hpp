#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include "Function.hpp"

namespace sclc
{
    class Interface {
        std::string name;
        std::vector<Function*> to_implement;
    public:
        Interface(std::string name) {
            this->name = name;
        }
        bool hasToImplement(std::string func) {
            for (Function* f : to_implement) {
                if (f->getName() == func) {
                    return true;
                }
            }
            return false;
        }
        Function* getToImplement(std::string func) {
            for (Function* f : to_implement) {
                if (f->getName() == func) {
                    return f;
                }
            }
            return nullptr;
        }
        std::vector<Function*> getImplements() {
            return to_implement;
        }
        void addToImplement(Function* func) {
            to_implement.push_back(func);
        }
        std::string getName() {
            return name;
        }
        void setName(std::string name) {
            this->name = name;
        }

        inline bool operator==(const Interface& other) const {
            return this->name == other.name;
        }
        inline bool operator!=(const Interface& other) const {
            return !((*this) == other);
        }
    };
} // namespace sclc
