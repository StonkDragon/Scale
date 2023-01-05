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
    class Function
    {
        std::string name;
        std::string file;
        std::string return_type;
        std::vector<Token> body;
        std::vector<std::string> modifiers;
        std::vector<Variable> args;
        Variable namedReturnValue;
        Token nameToken;
    public:
        bool isMethod;
        bool isExternC;
        bool isPrivate;
        bool hasNamedReturnValue;
        Function(std::string name, Token nameToken);
        Function(std::string name, bool isMethod, Token nameToken);
        virtual ~Function() {}
        virtual std::string getName();
        virtual std::vector<Token> getBody();
        virtual void addToken(Token token);
        virtual void addModifier(std::string modifier);
        virtual std::vector<std::string> getModifiers();
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

        virtual bool operator==(const Function& other) const;
    };
    
    class Method : public Function {
        std::string member_type;
        bool force_add;
    public:
        Method(std::string member_type, std::string name, Token& nameToken) : Function(name, true, nameToken) {
            this->member_type = member_type;
            this->isMethod = true;
            this->force_add = false;
        }
        std::string getMemberType() {
            return member_type;
        }
        void setMemberType(std::string member_type) {
            this->member_type = member_type;
        }
        bool addAnyway() {
            return force_add;
        }
        void forceAdd(bool force) {
            force_add = force;
        }
    };
} // namespace sclc
