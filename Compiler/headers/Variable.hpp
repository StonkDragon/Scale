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
    struct Token;

    enum VarAccess {
        Dereference,
        Write,
    };

    std::string removeTypeModifiers(std::string t);

    template<typename T>
    struct Also {
        virtual T& also(std::function<void(T&)> func) = 0;
    };

    bool typeCanBeNil(std::string s, bool doRemoveMods);
    bool typeIsReadonly(std::string s);
    bool typeIsConst(std::string s);

    struct Variable : public Also<Variable> {
        std::string name;
        std::string type;
        std::string internalMutableFrom;
        bool isConst;
        bool isInternalMut;
        bool isReadonly;
        bool isVirtual;
        bool isExtern;
        Token* name_token;
        bool isPrivate;
        bool canBeNil;
        std::string typeFromTemplate;
        Variable(std::string name, std::string type);
        Variable(std::string name, std::string type, std::string memberType);
        virtual ~Variable();

        bool isWritableFrom(Function* f) const;
        bool isAccessible(Function* f) const;
        bool operator==(const Variable& other) const;
        bool operator!=(const Variable& other) const;
        virtual Variable& also(std::function<void(Variable&)> f);

        static Variable& emptyVar();
    };
} // namespace sclc
