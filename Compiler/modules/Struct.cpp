#include <gc/gc_allocator.h>

#include "../headers/Common.hpp"

namespace sclc {
    Struct::Struct(std::string name) : Struct(name, Token(tok_identifier, name)) {}
    Struct::Struct(std::string name, Token t) {
        this->name = name;
        this->name_token = t;
        this->flags = 0;
        this->required_typed_arguments = 0;
    }
    Struct::~Struct() {};
    void Struct::addMember(Variable member, bool inherited) {
        if (inherited) {
            members.insert(members.begin(), member);
            memberInherited.insert(memberInherited.begin(), true);
            return;
        }
        members.push_back(member);
        memberInherited.push_back(false);
    }
    void Struct::clearMembers() {
        members.clear();
        memberInherited.clear();
    }
    bool Struct::hasMember(const std::string& member) const {
        for (const Variable& v : members) {
            if (v.name == member) {
                return true;
            }
        }
        return false;
    }
    bool Struct::hasMember(const std::string&& member) const {
        for (const Variable& v : members) {
            if (v.name == member) {
                return true;
            }
        }
        return false;
    }
    const Variable& Struct::getMember(std::string&& name) const {
        for (const Variable& v : members) {
            if (v.name == name) {
                return v;
            }
        }
        return Variable::emptyVar();
    }
    const Variable& Struct::getMember(const std::string& name) const {
        for (const Variable& v : members) {
            if (v.name == name) {
                return v;
            }
        }
        return Variable::emptyVar();
    }
    bool Struct::implements(const std::string& name) const {
        return interfaces.find(name) != interfaces.end();
    }
    void Struct::implement(const std::string& interface) {
        interfaces.insert(interface);
    }
    void Struct::addTemplateArgument(std::string name, Token type) {
        templates[name] = type;
    }
    bool Struct::isSealed() const {
        return (flags & 0b00000100) != 0;
    }
    bool Struct::isStatic() const {
        return (flags & 0b00010000) != 0;
    }
    bool Struct::isFinal() const {
        return (flags & 0b00100000) != 0;
    }
    bool Struct::isExtern() const {
        return (flags & 0b00000010) != 0;
    }
    void Struct::toggleFinal() {
        flags ^= 0b00100000;
    }
    void Struct::addModifier(std::string m) {
        if (m == "final") {
            flags |= 0b00100000;
        } else if (m == "sealed") {
            flags |= 0b00000100;
        } else if (m == "static") {
            flags |= 0b00010000;
        } else if (m == "expect") {
            flags |= 0b00000010;
        }
    }
    bool Struct::operator==(const Struct& other) const {
        return other.name == this->name;
    }
    bool Struct::operator!=(const Struct& other) const {
        return other.name != this->name;
    }
    std::vector<Variable> Struct::getDefinedMembers() const {
        std::vector<Variable> mems;
        for (size_t i = 0; i < members.size(); i++) {
            if (!memberInherited[i])
                mems.push_back(members[i]);
        }
        return mems;
    }

    Layout::Layout(std::string name) : Layout(name, Token(tok_identifier, name)) {}
    Layout::Layout(std::string name, Token t) {
        this->name = name;
        this->name_token = t;
    }
    void Layout::addMember(Variable member) {
        members.push_back(member);
    }
    bool Layout::hasMember(std::string member) {
        for (Variable& v : members) {
            if (v.name == member) {
                return true;
            }
        }
        return false;
    }
    Variable Layout::getMember(std::string name) {
        for (Variable& v : members) {
            if (v.name == name) {
                return v;
            }
        }
        return Variable("", "");
    }

    bool Layout::operator==(const Layout& other) const {
        return name == other.name;
    }
    bool Layout::operator!=(const Layout& other) const {
        return name != other.name;
    }
} // namespace sclc
