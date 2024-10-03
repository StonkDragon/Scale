
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include <Interface.hpp>

namespace sclc {
    Interface::Interface(std::string name) {
        this->name = name;
    }
    bool Interface::hasToImplement(std::string func) {
        for (Function* f : toImplement) {
            if (f->name == func) {
                return true;
            }
        }
        return false;
    }
    bool Interface::hasDefaultImplementation(std::string func) {
        for (Method* f : defaultImplementations) {
            if (f->name == func) {
                return true;
            }
        }
        return false;
    }
    Function* Interface::getToImplement(std::string func) {
        for (Function* f : toImplement) {
            if (f->name == func) {
                return f;
            }
        }
        return nullptr;
    }
    Method* Interface::getDefaultImplementation(std::string func) {
        for (Method* f : defaultImplementations) {
            if (f->name == func) {
                return f;
            }
        }
        return nullptr;
    }
    void Interface::addToImplement(Function* func) {
        toImplement.push_back(func);
    }
    void Interface::addDefaultImplementation(Method* func) {
        defaultImplementations.push_back(func);
    }

    bool Interface::operator==(const Interface& other) const {
        return this->name == other.name;
    }
    bool Interface::operator!=(const Interface& other) const {
        return !((*this) == other);
    }
} // namespace sclc
