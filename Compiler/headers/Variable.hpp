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
    struct Function;

    enum VarAccess {
        Dereference,
        Write,
    };

    std::string removeTypeModifiers(std::string t);

    template<typename T>
    struct Also {
        virtual T& also(std::function<void(T&)> func) = 0;
    };

    bool typeCanBeNil(std::string s);
    bool typeIsReadonly(std::string s);
    bool typeIsConst(std::string s);
    bool typeIsMut(std::string s);

    struct Variable : public Also<Variable> {
        struct Hasher {
            size_t operator()(const Variable& v) const {
                return std::hash<std::string>()(v.name) ^ std::hash<std::string>()(v.type);
            }
        };

        struct Equator {
            bool operator()(const Variable& v1, const Variable& v2) const {
                return v1 == v2;
            }
        };

        std::string name;
        std::string type;
        std::string internalMutableFrom;
        bool isConst;
        bool isInternalMut;
        bool isMut;
        bool isReadonly;
        bool isVirtual;
        Token* name_token;
        bool isPrivate;
        bool canBeNil;
        std::string typeFromTemplate;
        Variable(std::string name, std::string type) : Variable(name, type, "") {}
        Variable(std::string name, std::string type, std::string memberType) {
            this->name = name;
            this->type = type;
            this->isMut = typeIsMut(type);
            this->isConst = typeIsConst(type);
            this->isReadonly = typeIsReadonly(type);
            this->canBeNil = typeCanBeNil(type);
            this->internalMutableFrom = memberType;
            this->isInternalMut = memberType.size() != 0;
            this->isPrivate = false;
            this->typeFromTemplate = "";
            this->isVirtual = false;
        }
        virtual ~Variable() {}

        bool isWritableFrom(Function* f) const;
        bool isAccessible(Function* f) const;
        inline bool operator==(const Variable& other) const {
            if (this->type == "?" || other.type == "?") {
                return this->name == other.name;
            }
            return this->name == other.name && removeTypeModifiers(this->type) == removeTypeModifiers(other.type);
        }
        inline bool operator!=(const Variable& other) const {
            return !((*this) == other);
        }
        virtual Variable& also(std::function<void(Variable&)> f) {
            f(*this);
            return *this;
        }

        static Variable& emptyVar() {
            static Variable empty("", "");
            return empty;
        }
    };
} // namespace sclc
