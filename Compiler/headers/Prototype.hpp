#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

namespace sclc
{
    class Prototype
    {
        std::string name;
        std::string return_type;
        int argCount;
    public:
        Prototype(std::string name, int argCount) {
            this->name = name;
            this->argCount = argCount;
        }
        ~Prototype() {}
        std::string getName() { return name; }
        int getArgCount() { return argCount; }
        std::string getReturnType() { return return_type; }
        void setReturnType(std::string type) { return_type = type; }
        void setArgCount(int argCount) { this->argCount = argCount; }
        void setName(std::string name) { this->name = name; }
    };
} // namespace sclc
