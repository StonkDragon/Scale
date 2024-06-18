
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include "../headers/Common.hpp"

namespace sclc {
    Variable::Variable(std::string name, std::string type) : Variable(name, type, "") {}
    Variable::Variable(std::string name, std::string type, std::string memberType) {
        this->name = name;
        this->type = type;
        this->isConst = typeIsConst(type);
        this->isReadonly = typeIsReadonly(type);
        this->canBeNil = typeCanBeNil(type, true);
        this->internalMutableFrom = memberType;
        this->isInternalMut = memberType.size() != 0;
        this->isPrivate = false;
        this->typeFromTemplate = "";
        this->isVirtual = false;
        this->isExtern = false;
        this->hasInitializer = false;
    }
    Variable::~Variable() {}

    bool memberOfStruct(const Variable* self, Function* f);

    bool Variable::isAccessible(Function* f) const {
        if (this->isPrivate) {
            return memberOfStruct(this, f);
        }
        return true;
    }

    bool Variable::isWritableFrom(Function* f) const {
        if (this->isReadonly || this->isPrivate) {
            if (this->isReadonly && strstarts(this->name, f->member_type + "$")) {
                return true;
            }
            return memberOfStruct(this, f);
        }
        if (isConst) {
            return isInitFunction(f);
        }
        return true;
    }

    bool Variable::operator==(const Variable& other) const {
        if (this->type == "?" || other.type == "?") {
            return this->name == other.name;
        }
        return this->name == other.name && removeTypeModifiers(this->type) == removeTypeModifiers(other.type);
    }
    bool Variable::operator!=(const Variable& other) const {
        return !((*this) == other);
    }
    Variable& Variable::also(std::function<void(Variable&)> f) {
        f(*this);
        return *this;
    }

    Variable& Variable::emptyVar() {
        static Variable empty("", "");
        return empty;
    }
} // namespace sclc
