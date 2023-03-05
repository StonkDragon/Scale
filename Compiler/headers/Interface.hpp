#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include "Function.hpp"

namespace sclc
{
    class Interface {
        std::string name;
        std::vector<Function*> toImplement;
        std::vector<Method*> defaultImplementations;
    public:
        Interface(std::string name) {
            this->name = name;
        }
        bool hasToImplement(std::string func) {
            for (Function* f : toImplement) {
                if (f->getName() == func) {
                    return true;
                }
            }
            return false;
        }
        bool hasDefaultImplementation(std::string func) {
            for (Method* f : defaultImplementations) {
                if (f->getName() == func) {
                    return true;
                }
            }
            return false;
        }
        Function* getToImplement(std::string func) {
            for (Function* f : toImplement) {
                if (f->getName() == func) {
                    return f;
                }
            }
            return nullptr;
        }
        Method* getDefaultImplementation(std::string func) {
            for (Method* f : defaultImplementations) {
                if (f->getName() == func) {
                    return f;
                }
            }
            return nullptr;
        }
        std::vector<Function*> getImplements() {
            return toImplement;
        }
        std::vector<Method*> getDefaultImplementations() {
            return defaultImplementations;
        }
        void addToImplement(Function* func) {
            toImplement.push_back(func);
        }
        void addDefaultImplementation(Method* func) {
            defaultImplementations.push_back(func);
        }
        std::string getName() {
            return name;
        }
        void setName(std::string name) {
            this->name = name;
        }

        inline bool operator==(const Interface& other) const {
            return this->name == other.name;
        }
        inline bool operator!=(const Interface& other) const {
            return !((*this) == other);
        }
    };
} // namespace sclc
