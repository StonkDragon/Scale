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
    struct Interface {
        std::string name;
        std::vector<Function*> toImplement;
        std::vector<Method*> defaultImplementations;
        Token* name_token;
        Interface(std::string name) {
            this->name = name;
        }
        bool hasToImplement(std::string func) {
            for (Function* f : toImplement) {
                if (f->name == func) {
                    return true;
                }
            }
            return false;
        }
        bool hasDefaultImplementation(std::string func) {
            for (Method* f : defaultImplementations) {
                if (f->name == func) {
                    return true;
                }
            }
            return false;
        }
        Function* getToImplement(std::string func) {
            for (Function* f : toImplement) {
                if (f->name == func) {
                    return f;
                }
            }
            return nullptr;
        }
        Method* getDefaultImplementation(std::string func) {
            for (Method* f : defaultImplementations) {
                if (f->name == func) {
                    return f;
                }
            }
            return nullptr;
        }
        void addToImplement(Function* func) {
            toImplement.push_back(func);
        }
        void addDefaultImplementation(Method* func) {
            defaultImplementations.push_back(func);
        }

        inline bool operator==(const Interface& other) const {
            return this->name == other.name;
        }
        inline bool operator!=(const Interface& other) const {
            return !((*this) == other);
        }
    };
} // namespace sclc
