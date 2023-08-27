#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <stack>
#include <regex>

#include "../headers/Common.hpp"

namespace sclc {
    extern std::stack<std::string> typeStack;
#define typeStackTop (typeStack.size() ? typeStack.top() : "")

    std::string sclTypeToCType(TPResult& result, std::string t);
    std::string generateArgumentsForFunction(TPResult& result, Function *func);
    bool isPrimitiveIntegerType(std::string s);
    std::string removeTypeModifiers(std::string t);

    bool isFloatingType(std::string s) {
        return s == "float";
    }

#define debugDump(_var) std::cout << #_var << ": " << _var << std::endl

    bool checkStackType(TPResult& result, std::vector<Variable>& args, bool allowIntPromotion = false);
    std::string argVectorToString(std::vector<Variable>& args);
    std::string stackSliceToString(size_t amount);

    bool handleOverriddenOperator(TPResult& result, FILE* fp, int scopeDepth, std::string op, std::string type) {
        type = removeTypeModifiers(type);
        if (type.size() == 0)
            return false;

        if (hasMethod(result, op, type)) {
            Method* f = getMethodByName(result, op, type);
            
            if (f->args.size() > 0) {
                append("_scl_popn(%zu);\n", f->args.size());
            }
            bool argsCorrect = checkStackType(result, f->args);
            if (!argsCorrect) {
                return false;
            }
            for (size_t m = 0; m < f->args.size(); m++) {
                if (typeStack.size())
                    typeStack.pop();
            }
            if (f->return_type.size() > 0 && f->return_type != "none" && f->return_type != "nothing") {
                append("*(%s*) (localstack) = mt_%s$%s(%s);\n", sclTypeToCType(result, f->return_type).c_str(), f->member_type.c_str(), f->finalName().c_str(), generateArgumentsForFunction(result, f).c_str());
                append("localstack++;\n");
                typeStack.push(f->return_type);
            } else {
                append("mt_%s$%s(%s);\n", f->member_type.c_str(), f->finalName().c_str(), generateArgumentsForFunction(result, f).c_str());
            }
            return true;
        }
        return false;
    }
}
