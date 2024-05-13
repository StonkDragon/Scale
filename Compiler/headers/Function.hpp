#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include <gc_cpp.h>

#include "Variable.hpp"
#include "Token.hpp"

namespace sclc
{
    typedef std::unordered_map<std::string, std::string> Deprecation;

    struct Function : public gc_cleanup {
        size_t lambdaIndex;
        std::string name;
        std::string name_without_overload;
        std::string return_type;
        std::vector<Token> body;
        std::vector<std::string> modifiers;
        std::vector<Variable> args;
        Variable namedReturnValue;
        std::string member_type;
        Token name_token;
        bool isMethod;
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
        long has_foreign;
        long has_overrides;
        long has_binary_inherited;
        long has_nonvirtual;
        long has_async;
        long has_reified;
        long has_inline;
        
        std::vector<Variable> captures;
        std::vector<Variable> ref_captures;
        std::vector<std::string> reified_parameters;

        Function* container;

        Function(std::string name, Token name_token);
        Function(std::string name, bool isMethod, Token name_token);
        virtual ~Function();
        virtual std::vector<Token>& getBody();
        virtual void addToken(Token token);
        virtual void addModifier(std::string modifier);
        // IMPORTANT: Function takes index starting at 1
        virtual const std::string& getModifier(size_t index);
        virtual void addArgument(Variable arg);
        virtual bool belongsToType(std::string typeName);
        virtual void clearArgs();
        virtual bool isCVarArgs();
        virtual Variable& varArgsParam();
        virtual Function* clone();

        virtual bool operator==(const Function& other) const;
        virtual bool operator!=(const Function& other) const;
        virtual bool operator==(const Function* other) const;
        virtual bool operator!=(const Function* other) const;
    };
    
    struct Method : public Function {
        bool force_add;
        Method(std::string member_type, std::string name, Token name_token);
        Method* cloneAs(std::string memberType);
    };
} // namespace sclc
