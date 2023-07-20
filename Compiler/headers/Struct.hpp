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
        Token name_token;
        int flags;
        std::vector<Variable> members;
        std::vector<bool> memberInherited;
        std::vector<std::string> interfaces;
        std::string super;
        std::vector<std::string> toImplementFunctions;
        size_t required_typed_arguments;
        std::string fancyName;
        std::map<std::string, std::string> templates;
        static Struct Null;

        Struct(std::string name) : Struct(name, Token(tok_identifier, name, 0, "")) {}
        Struct(std::string name, Token t) {
            this->name = name;
            this->name_token = t;
            this->flags = 0;
            this->required_typed_arguments = 0;
            this->fancyName = name;
        }
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
        bool hasMember(std::string member) const {
            for (const Variable& v : members) {
                if (v.getName() == member) {
                    return true;
                }
            }
            return false;
        }
        bool extends(std::string s) const {
            return this->super == s;
        }
        std::string extends() const {
            return this->super;
        }
        void setExtends(std::string s) {
            this->super = s;
        }
        Variable getMember(std::string name) const {
            for (const Variable& v : members) {
                if (v.getName() == name) {
                    return v;
                }
            }
            return Variable("", "");
        }
        bool implements(std::string name) const {
            return std::find(interfaces.begin(), interfaces.end(), name) != interfaces.end();
        }
        void implement(std::string interface) {
            interfaces.push_back(interface);
        }
        std::vector<std::string> getInterfaces() const {
            return interfaces;
        }
        Token nameToken() const {
            return name_token;
        }
        void setNameToken(Token t) {
            this->name_token = t;
        }
        Token getNameToken() const {
            return name_token;
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
        void toggleSealed() {
            flags ^= 0b00000100;
        }
        void toggleStatic() {
            flags ^= 0b00010000;
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
        std::string getName() const {
            return name;
        }
        std::vector<Variable> getMembers() const {
            return members;
        }
        std::vector<Variable> getDefinedMembers() const {
            std::vector<Variable> mems;
            for (size_t i = 0; i < members.size(); i++) {
                if (!memberInherited[i])
                    mems.push_back(members[i]);
            }
            return mems;
        }
        void setName(const std::string& name) {
            this->name = name;
            this->fancyName = name;
        }
    };

    class Layout {
        std::string name;
        Token name_token;
        std::vector<Variable> members;
    public:
    
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
                if (v.getName() == member) {
                    return true;
                }
            }
            return false;
        }
        Variable getMember(std::string name) {
            for (Variable& v : members) {
                if (v.getName() == name) {
                    return v;
                }
            }
            return Variable("", "");
        }
        void setName(const std::string& name) { this->name = name; }
        Token getNameToken() {
            return name_token;
        }
        void setNameToken(Token t) {
            this->name_token = t;
        }
        std::string getName() {
            return name;
        }
        std::vector<Variable> getMembers() {
            return members;
        }
    };
} // namespace sclc
