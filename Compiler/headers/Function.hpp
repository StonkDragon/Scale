#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include "Variable.hpp"

namespace sclc
{
    typedef std::unordered_map<std::string, std::string> Deprecation;

    struct Function {
        std::string name;
        std::string file;
        std::string return_type;
        std::vector<Token> body;
        std::vector<std::string> modifiers;
        std::vector<Variable> args;
        Variable namedReturnValue;
        std::string member_type;
        Token nameToken;
        bool isMethod;
        bool isExternC;
        bool isPrivate;
        bool hasNamedReturnValue;
        Deprecation deprecated;
        std::vector<Function*> overloads;
        std::string templateArg;

        long has_expect;
        long has_export;
        long has_private;
        long has_construct;
        long has_final;
        long has_constructor;
        long has_cdecl;
        long has_intrinsic;
        long has_lambda;
        long has_asm;
        long has_sealed;
        long has_unsafe;
        long has_operator;
        long has_restrict;
        long has_getter;
        long has_setter;

        Function(std::string name, Token nameToken);
        Function(std::string name, bool isMethod, Token nameToken);
        virtual ~Function() {}
        virtual std::string getName();
        virtual std::string finalName();
        virtual std::vector<Token> getBody();
        virtual std::vector<Token>& getBodyRef();
        virtual void addToken(Token token);
        virtual void addModifier(std::string modifier);
        virtual std::vector<std::string> getModifiers();
        // IMPORTANT: Function takes index starting at 1
        virtual std::string& getModifier(size_t index);
        virtual void addArgument(Variable arg);
        virtual std::vector<Variable> getArgs();
        virtual std::string getFile();
        virtual void setFile(std::string file);
        virtual void setName(std::string name);
        virtual std::string getReturnType();
        virtual void setReturnType(std::string type);
        virtual Token getNameToken();
        virtual void setNameToken(Token t);
        virtual Variable getNamedReturnValue();
        virtual void setNamedReturnValue(Variable v);
        virtual bool belongsToType(std::string typeName);
        virtual void clearArgs();
        virtual bool isCVarArgs();
        virtual Variable& varArgsParam();
        virtual std::string getMemberType() {
            return member_type;
        }
        virtual void setMemberType(std::string member_type) {
            this->member_type = member_type;
        }

        virtual bool operator==(const Function& other) const;
        virtual bool operator!=(const Function& other) const;
        virtual bool operator==(const Function* other) const;
        virtual bool operator!=(const Function* other) const;
    };
    
    class Method : public Function {
        bool force_add;
    public:
        Method(std::string member_type, std::string name, Token nameToken) : Function(name, true, nameToken) {
            this->member_type = member_type;
            this->isMethod = true;
            this->force_add = false;
        }
        bool addAnyway() {
            return force_add;
        }
        void forceAdd(bool force) {
            force_add = force;
        }
        Method* cloneAs(std::string memberType) {
            Method* m = new Method(memberType, getName(), nameToken);
            m->isMethod = isMethod;
            m->force_add = force_add;
            m->setReturnType(getReturnType());
            for (auto s : getModifiers()) {
                m->addModifier(s);
            }
            m->setFile(getFile());
            m->setReturnType(getReturnType());
            for (Token t : this->getBody()) {
                m->addToken(t);
            }
            for (Variable v : getArgs()) {
                m->addArgument(v);
            }
            if (hasNamedReturnValue) {
                m->setNamedReturnValue(getNamedReturnValue());
            }
            return m;
        }
    };
} // namespace sclc
