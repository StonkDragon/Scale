#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <map>

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
        std::map<std::string, std::string> templates;
        static Struct Null;

        Struct(std::string name) : Struct(name, Token(tok_identifier, name, 0, "")) {}
        Struct(std::string name, Token t) {
            this->name = name;
            this->name_token = t;
            this->flags = 0;
            this->required_typed_arguments = 0;
        }
        Struct(const Struct& other) {
            this->name = other.name;
            this->name_token = other.name_token;
            this->flags = other.flags;
            this->members = other.members;
            this->memberInherited = other.memberInherited;
            this->interfaces = other.interfaces;
            this->super = other.super;
            this->toImplementFunctions = other.toImplementFunctions;
            this->required_typed_arguments = other.required_typed_arguments;
            this->templates = other.templates;
        }
        Struct(Struct&& other) {
            this->name = std::move(other.name);
            this->name_token = std::move(other.name_token);
            this->flags = std::move(other.flags);
            this->members = std::move(other.members);
            this->memberInherited = std::move(other.memberInherited);
            this->interfaces = std::move(other.interfaces);
            this->super = std::move(other.super);
            this->toImplementFunctions = std::move(other.toImplementFunctions);
            this->required_typed_arguments = std::move(other.required_typed_arguments);
            this->templates = std::move(other.templates);
        }
        Struct& operator=(const Struct& other) {
            this->name = other.name;
            this->name_token = other.name_token;
            this->flags = other.flags;
            this->members = other.members;
            this->memberInherited = other.memberInherited;
            this->interfaces = other.interfaces;
            this->super = other.super;
            this->toImplementFunctions = other.toImplementFunctions;
            this->required_typed_arguments = other.required_typed_arguments;
            this->templates = other.templates;
            return *this;
        }
        Struct& operator=(Struct&& other) {
            this->name = std::move(other.name);
            this->name_token = std::move(other.name_token);
            this->flags = std::move(other.flags);
            this->members = std::move(other.members);
            this->memberInherited = std::move(other.memberInherited);
            this->interfaces = std::move(other.interfaces);
            this->super = std::move(other.super);
            this->toImplementFunctions = std::move(other.toImplementFunctions);
            this->required_typed_arguments = std::move(other.required_typed_arguments);
            this->templates = std::move(other.templates);
            return *this;
        }
        ~Struct() = default;
        void addMember(Variable member, bool inherited = false) {
            if (inherited) {
                members.insert(members.begin(), member);
                memberInherited.insert(memberInherited.begin(), true);
                return;
            }
            members.push_back(member);
            memberInherited.push_back(false);
        }
        void clearMembers() {
            members.clear();
            memberInherited.clear();
        }
        bool hasMember(const std::string& member) const {
            for (const Variable& v : members) {
                if (v.name == member) {
                    return true;
                }
            }
            return false;
        }
        bool hasMember(const std::string&& member) const {
            for (const Variable& v : members) {
                if (v.name == member) {
                    return true;
                }
            }
            return false;
        }
        Variable& getMember(std::string&& name) const {
            for (const Variable& v : members) {
                if (v.name == name) {
                    return __CAST_AWAY_QUALIFIER(v, const, Variable&);
                }
            }
            return Variable::emptyVar();
        }
        Variable& getMember(std::string& name) const {
            for (const Variable& v : members) {
                if (v.name == name) {
                    return __CAST_AWAY_QUALIFIER(v, const, Variable&);
                }
            }
            return Variable::emptyVar();
        }
        bool implements(const std::string& name) const {
            return interfaces.find(name) != interfaces.end();
        }
        void implement(const std::string& interface) {
            interfaces.insert(interface);
        }
        void addTemplateArgument(std::string name, std::string type) {
            templates[name] = type;
        }
        bool isSealed() const {
            return (flags & 0b00000100) != 0;
        }
        bool isStatic() const {
            return (flags & 0b00010000) != 0;
        }
        bool isFinal() const {
            return (flags & 0b00100000) != 0;
        }
        void toggleFinal() {
            flags ^= 0b00100000;
        }
        void addModifier(std::string m) {
            if (m == "final") {
                flags |= 0b00100000;
            } else if (m == "sealed") {
                flags |= 0b00000100;
            } else if (m == "static") {
                flags |= 0b00010000;
            }
        }
        inline bool operator==(const Struct& other) const {
            return other.name == this->name;
        }
        inline bool operator!=(const Struct& other) const {
            return other.name != this->name;
        }
        std::vector<Variable> getDefinedMembers() const {
            std::vector<Variable> mems;
            for (size_t i = 0; i < members.size(); i++) {
                if (!memberInherited[i])
                    mems.push_back(members[i]);
            }
            return mems;
        }
    };

    struct Layout {
        std::string name;
        Token name_token;
        std::vector<Variable> members;
    
        Layout(std::string name) : Layout(name, Token(tok_identifier, name, 0, "")) {}
        Layout(std::string name, Token t) {
            this->name = name;
            this->name_token = t;
        }
        void addMember(Variable member) {
            members.push_back(member);
        }
        bool hasMember(std::string member) {
            for (Variable& v : members) {
                if (v.name == member) {
                    return true;
                }
            }
            return false;
        }
        Variable getMember(std::string name) {
            for (Variable& v : members) {
                if (v.name == name) {
                    return v;
                }
            }
            return Variable("", "");
        }
    };
} // namespace sclc
