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

    bool handlePrimitiveOperator(FILE* fp, int scopeDepth, std::string op, std::string typeA) {
        if (typeA.size() == 0)
            return false;
        if (typeStack.size())
            typeStack.pop();
        std::string typeB = typeStackTop;
        if (typeStack.size())
            typeStack.pop();
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            if (!isFloatingType(typeA)) {
                if (isFloatingType(typeB)) {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->f = _scl_positive_offset(0)->f %s ((scl_float) _scl_positive_offset(1)->i);\n", op.c_str());
                    typeStack.push("float");
                } else {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->i = _scl_positive_offset(0)->i %s _scl_positive_offset(1)->i;\n", op.c_str());
                    typeStack.push("int");
                }
            } else {
                if (isFloatingType(typeB)) {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->f = _scl_positive_offset(0)->f %s _scl_positive_offset(1)->f;\n", op.c_str());
                    typeStack.push("float");
                } else {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->f = ((scl_float) _scl_positive_offset(0)->i) %s _scl_positive_offset(1)->f;\n", op.c_str());
                    typeStack.push("float");
                }
            }
            return true;
        }
        if (op == "**") {
            if (!isFloatingType(typeA)) {
                if (isFloatingType(typeB)) {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->f = pow(_scl_positive_offset(0)->f, ((scl_float) _scl_positive_offset(1)->i));\n");
                    typeStack.push("float");
                } else {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->i = (scl_int) pow(_scl_positive_offset(0)->i, _scl_positive_offset(1)->i);\n");
                    typeStack.push("int");
                }
            } else {
                if (isFloatingType(typeB)) {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->f = pow(_scl_positive_offset(0)->f, _scl_positive_offset(1)->f);\n");
                    typeStack.push("float");
                } else {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->f = pow(((scl_float) _scl_positive_offset(0)->i), _scl_positive_offset(1)->f);\n");
                    typeStack.push("float");
                }
            }
            return true;
        }
        if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
            if (!isFloatingType(typeA)) {
                if (isFloatingType(typeB)) {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->i = _scl_positive_offset(0)->f %s ((scl_float) _scl_positive_offset(1)->i);\n", op.c_str());
                } else {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->i = _scl_positive_offset(0)->i %s _scl_positive_offset(1)->i;\n", op.c_str());
                }
            } else {
                if (isFloatingType(typeB)) {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->i = _scl_positive_offset(0)->f %s _scl_positive_offset(1)->f;\n", op.c_str());
                } else {
                    append("_stack.sp -= (2);\n");
                    append("(_stack.sp++)->i = ((scl_float) _scl_positive_offset(0)->i) %s _scl_positive_offset(1)->f;\n", op.c_str());
                }
            }
            typeStack.push("bool");
            return true;
        }
        return false;
    }

#define debugDump(_var) std::cout << #_var << ": " << _var << std::endl

    bool checkStackType(TPResult& result, std::vector<Variable>& args, bool allowIntPromotion = false);
    std::string argVectorToString(std::vector<Variable>& args);
    std::string stackSliceToString(size_t amount);

    bool handleOverriddenOperator(TPResult& result, FILE* fp, int scopeDepth, std::string op, std::string type) {
        type = removeTypeModifiers(type);
        if (type.size() == 0)
            return false;
        if (isPrimitiveType(type) && type != "bool") {
            return handlePrimitiveOperator(fp, scopeDepth, op, type);
        }
        if (hasMethod(result, op, type)) {
            Method* f = getMethodByName(result, op, type);
            
            if (f->args.size() > 0) {
                append("_stack.sp -= (%zu);\n", f->args.size());
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
                if (f->return_type == "float") {
                    append("(_stack.sp++)->f = Method_%s$%s(%s);\n", f->member_type.c_str(), f->finalName().c_str(), generateArgumentsForFunction(result, f).c_str());
                } else {
                    append("(_stack.sp++)->i = (scl_int) Method_%s$%s(%s);\n", f->member_type.c_str(), f->finalName().c_str(), generateArgumentsForFunction(result, f).c_str());
                }
                typeStack.push(f->return_type);
            } else {
                append("Method_%s$%s(%s);\n", f->member_type.c_str(), f->finalName().c_str(), generateArgumentsForFunction(result, f).c_str());
            }
            return true;
        }
        return false;
    }
}
