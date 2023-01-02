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

    enum VarAccess {
        Dereference,
        Write,
    };

    class Variable {
        std::string name;
        std::string type;
        std::string internalMutableFrom;
        bool isConst;
        bool isInternalMut;
        bool isMut;
    public:
        Variable(std::string name, std::string type) : Variable(name, type, false, "") {}
        Variable(std::string name, std::string type, bool isConst, bool isMut) : Variable(name, type, isConst, isMut, "") {}
        Variable(std::string name, std::string type, std::string memberType) : Variable(name, type, false, false, memberType) {}
        Variable(std::string name, std::string type, bool isConst, bool isMut, std::string memberType) {
            this->name = name;
            this->type = type;
            this->isMut = isMut;
            this->isConst = isConst;
            this->internalMutableFrom = memberType;
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
        bool isWritableFrom(Function* f, VarAccess accessType);
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
