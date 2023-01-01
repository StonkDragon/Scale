#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

namespace sclc
{
    class Struct {
        std::string name;
        Token name_token;
        int flags;
        std::vector<Variable> members;
        std::vector<std::string> interfaces;
    public:
        Struct(std::string name) : Struct(name, Token(tok_identifier, name, 0, "")) {
            
        }
        Struct(std::string name, Token t) {
            this->name = name;
            this->name_token = t;
            this->flags = 0;
            toggleWarnings();
            toggleValueType();
            toggleReferenceType();
        }
        void addMember(Variable member) {
            members.push_back(member);
        }
        bool hasMember(std::string member) {
            for (Variable m : members) {
                if (m.getName() == member) {
                    return true;
                }
            }
            return false;
        }
        int indexOfMember(std::string member) {
            for (size_t i = 0; i < members.size(); i++) {
                if (members[i].getName() == member) {
                    return ((int) i) * 8;
                }
            }
            return -1;
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
        std::vector<Variable> getMembers() { return members; }
        void setName(const std::string& name) { this->name = name; }
    };
} // namespace sclc
