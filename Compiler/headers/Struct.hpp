#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <map>

// #include "Results.hpp"

namespace sclc
{
    class Struct {
        std::string name;
        Token name_token;
        int flags;
        std::vector<Variable> members;
        std::vector<bool> memberInherited;
        std::vector<std::string> interfaces;
        std::string super;
    public:
        std::vector<std::string> toImplementFunctions;
        static Struct Null;

        Struct(std::string name) : Struct(name, Token(tok_identifier, name, 0, "")) {}
        Struct(std::string name, Token t) {
            this->name = name;
            this->name_token = t;
            this->flags = 0;
            toggleWarnings();
            toggleValueType();
            toggleReferenceType();
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
        bool hasMember(std::string member) {
            for (Variable v : members) {
                if (v.getName() == member) {
                    return true;
                }
            }
            return false;
        }
        bool extends(std::string s) {
            return this->super == s;
        }
        std::string extends() {
            return this->super;
        }
        void setExtends(std::string s) {
            this->super = s;
        }
        Variable getMember(std::string name) {
            for (Variable v : members) {
                if (v.getName() == name) {
                    return v;
                }
            }
            return Variable("", "");
        }
        bool implements(std::string name) {
            return std::find(interfaces.begin(), interfaces.end(), name) != interfaces.end();
        }
        void implement(std::string interface) {
            interfaces.push_back(interface);
        }
        std::vector<std::string> getInterfaces() {
            return interfaces;
        }
        Token nameToken() {
            return name_token;
        }
        void setNameToken(Token t) {
            this->name_token = t;
        }
        bool heapAllocAllowed() {
            return (flags & 0b00000001) != 0;
        }
        bool stackAllocAllowed() {
            return (flags & 0b00000010) != 0;
        }
        bool isSealed() {
            return (flags & 0b00000100) != 0;
        }
        bool warnsEnabled() {
            return (flags & 0b00001000) != 0;
        }
        bool isStatic() {
            return (flags & 0b00010000) != 0;
        }
        void toggleReferenceType() {
            flags ^= 0b00000001;
        }
        void toggleValueType() {
            flags ^= 0b00000010;
        }
        void toggleSealed() {
            flags ^= 0b00000100;
        }
        void toggleWarnings() {
            flags ^= 0b00001000;
        }
        void toggleStatic() {
            flags ^= 0b00010000;
        }
        inline bool operator==(const Struct& other) const {
            return other.name == this->name;
        }
        inline bool operator!=(const Struct& other) const {
            return other.name != this->name;
        }
        std::string getName() { return name; }
        std::vector<Variable> getMembers() {
            return members;
        }
        std::vector<Variable> getDefinedMembers() {
            std::vector<Variable> mems;
            for (size_t i = 0; i < members.size(); i++) {
                if (!memberInherited[i])
                    mems.push_back(members[i]);
            }
            return mems;
        }
        void setName(const std::string& name) { this->name = name; }
    };
} // namespace sclc
