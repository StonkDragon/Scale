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
    class Function;

    class Variable {
        std::string name;
        std::string type;
        std::string memberType;
        bool isConst;
        bool isInternalMut;
    public:
        Variable(std::string name, std::string type) : Variable(name, type, false, "") {}
        Variable(std::string name, std::string type, bool isConst) : Variable(name, type, isConst, "") {}
        Variable(std::string name, std::string type, std::string memberType) : Variable(name, type, false, memberType) {}
        Variable(std::string name, std::string type, bool isConst, std::string memberType) {
            this->name = name;
            this->type = type;
            this->isConst = isConst;
            this->memberType = memberType;
            this->isInternalMut = memberType.size() != 0;
        }
        ~Variable() {}
        std::string getName() {
            return name;
        }
        std::string getType() {
            return type;
        }
        void setName(std::string name) {
            this->name = name;
        }
        void setType(std::string type) {
            this->type = type;
        }
        bool isWritable() {
            return !isConst;
        }
        bool isWritableFrom(Function* f);
        inline bool operator==(const Variable& other) const {
            if (this->type == "?" || other.type == "?") {
                return this->name == other.name;
            }
            return this->name == other.name && this->type == other.type;
        }
        inline bool operator!=(const Variable& other) const {
            return !((*this) == other);
        }
    };
} // namespace sclc
