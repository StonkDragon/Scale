#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <map>

#include "Common.hpp"

namespace sclc
{
    struct Struct {
        std::string name;
        std::string super;
        Token name_token;
        int flags;
        std::vector<Variable> members;
        std::vector<bool> memberInherited;
        std::unordered_set<std::string> interfaces;
        std::unordered_set<std::string> toImplementFunctions;
        size_t required_typed_arguments;
        std::map<std::string, Token> templates;
        bool templateInstance = false;
        bool usedInStdLib = false;
        static Struct Null;

        Struct(std::string name);
        Struct(std::string name, Token t);
        ~Struct();
        void addMember(Variable member, bool inherited = false);
        void clearMembers();
        bool hasMember(const std::string& member) const;
        bool hasMember(const std::string&& member) const;
        const Variable& getMember(std::string&& name) const;
        const Variable& getMember(const std::string& name) const;
        bool implements(const std::string& name) const;
        void implement(const std::string& interface);
        void addTemplateArgument(std::string name, Token type);
        bool isSealed() const;
        bool isStatic() const;
        bool isFinal() const;
        bool isExtern() const;
        void toggleFinal();
        void addModifier(std::string m);
        bool operator==(const Struct& other) const;
        bool operator!=(const Struct& other) const;
        std::vector<Variable> getDefinedMembers() const;
    };

    struct Layout {
        std::string name;
        Token name_token;
        std::vector<Variable> members;
    
        Layout(std::string name);
        Layout(std::string name, Token t);
        void addMember(Variable member);
        bool hasMember(std::string member);
        Variable getMember(std::string name);

        bool operator==(const Layout& other) const;
        bool operator!=(const Layout& other) const;
    };
} // namespace sclc
