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

    std::string removeTypeModifiers(std::string t);

    class Variable {
        std::string name;
        std::string type;
        std::string internalMutableFrom;
        bool isConst;
        bool isInternalMut;
        bool isMut;
    public:
        bool isPrivate;
        bool canBeNil;
        std::string typeFromTemplate;
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
            this->canBeNil = false;
            this->isPrivate = false;
            this->typeFromTemplate = "";
        }
        ~Variable() {}
        std::string getName() {
            return name;
        }
        std::string getType() const {
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
            if (this->getType() == "?" || other.getType() == "?") {
                return this->name == other.name;
            }
            return this->name == other.name && removeTypeModifiers(this->getType()) == removeTypeModifiers(other.getType());
        }
        inline bool operator!=(const Variable& other) const {
            return !((*this) == other);
        }
    };
} // namespace sclc
