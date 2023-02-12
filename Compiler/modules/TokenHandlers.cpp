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

    std::string sclTypeToCType(TPResult result, std::string t);
    std::string sclGenArgs(TPResult result, Function *func);
    bool isPrimitiveIntegerType(std::string s);

    bool handlePrimitiveOperator(FILE* fp, int scopeDepth, std::string op, std::string typeA) {
        if (typeA.size() == 0)
            return false;
        if (typeStack.size())
            typeStack.pop();
        std::string typeB = typeStackTop;
        if (typeStack.size())
            typeStack.pop();
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            if (isPrimitiveIntegerType(typeA) || typeA == "any") {
                if (typeB == "float") {
                    append("_scl_internal_stack.ptr -= 2; _scl_push()->f = _scl_internal_stack.data[_scl_internal_stack.ptr].f %s ((scl_float) _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i);\n", op.c_str());
                    typeStack.push("float");
                } else {
                    append("_scl_internal_stack.ptr -= 2; _scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i %s _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n", op.c_str());
                    typeStack.push("int");
                }
            } else {
                if (typeB == "float") {
                    append("_scl_internal_stack.ptr -= 2; _scl_push()->f = _scl_internal_stack.data[_scl_internal_stack.ptr].f %s _scl_internal_stack.data[_scl_internal_stack.ptr + 1].f;\n", op.c_str());
                    typeStack.push("float");
                } else {
                    append("_scl_internal_stack.ptr -= 2; _scl_push()->f = ((scl_float) _scl_internal_stack.data[_scl_internal_stack.ptr].i) %s _scl_internal_stack.data[_scl_internal_stack.ptr + 1].f;\n", op.c_str());
                    typeStack.push("float");
                }
            }
            return true;
        }
        return false;
    }

#define debugDump(_var) std::cout << #_var << ": " << _var << std::endl

    bool handleOverriddenOperator(TPResult result, FILE* fp, int scopeDepth, std::string op, std::string type) {
        if (type.size() == 0)
            return false;
        if (isPrimitiveType(type) && type != "bool") {
            return handlePrimitiveOperator(fp, scopeDepth, op, type);
        }
        if (hasMethod(result, op, type)) {
            Method* f = getMethodByName(result, op, type);
            bool equals = true;
            if (typeStack.size())
                typeStack.pop();
            for (ssize_t m = f->getArgs().size() - 2; m >= 0; m--) {
                std::string typeA = sclConvertToStructType(f->getArgs()[m].getType());
                std::string typeB = sclConvertToStructType(typeStackTop);
                if (typeA != typeB) {
                    equals = false;
                }
                if (typeStack.size())
                    typeStack.pop();
            }
            if (!equals) {
                return false;
            }
            if (f->getArgs().size() > 0) {
                append("_scl_internal_stack.ptr -= %zu;\n", f->getArgs().size());
            }
            if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                if (f->getReturnType() == "float") {
                    append("_scl_push()->f = Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                } else {
                    append("_scl_push()->v = (scl_any) Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                }
                typeStack.push(f->getReturnType());
            } else {
                append("Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
            }
            return true;
        }
        return false;
    }

    FPResult handleOperator(TPResult result, FILE* fp, Token token, int scopeDepth) {
        switch (token.getType()) {
            case tok_add: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "+", typeStackTop)) break;
                append("_scl_internal_stack.ptr -= 2; _scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i + _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            case tok_sub: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "-", typeStackTop)) break;
                append("_scl_internal_stack.ptr -= 2; _scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i - _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            case tok_mul: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "*", typeStackTop)) break;
                append("_scl_internal_stack.ptr -= 2; _scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i * _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            case tok_div: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "/", typeStackTop)) break;
                append("_scl_internal_stack.ptr -= 2; _scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i / _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            case tok_mod: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "%", typeStackTop)) break;
                append("_scl_internal_stack.ptr -= 2; _scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i %% _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            case tok_land: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "&", typeStackTop)) break;
                append("_scl_internal_stack.ptr -= 2; _scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i & _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            case tok_lor: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "|", typeStackTop)) break;
                append("_scl_internal_stack.ptr -= 2; _scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i | _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            case tok_lxor: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "^", typeStackTop)) break;
                append("_scl_internal_stack.ptr -= 2; _scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i ^ _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            case tok_lnot: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "~", typeStackTop)) break;
                append("_scl_top()->i = ~_scl_top()->i;\n");
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            case tok_lsh: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "<<", typeStackTop)) break;
                append("_scl_internal_stack.ptr -= 2; _scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i << _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            case tok_rsh: {
                if (handleOverriddenOperator(result, fp, scopeDepth, ">>", typeStackTop)) break;
                append("_scl_internal_stack.ptr -= 2; _scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i >> _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            case tok_pow: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "**", typeStackTop)) break;
                append("{ scl_int exp = _scl_pop()->i; scl_int base = _scl_pop()->i; scl_int intResult = (scl_int) base; scl_int i = 1; while (i < exp) { intResult *= (scl_int) base; i++; } _scl_push()->i = intResult; }\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int");
                break;
            }

            default: {
                FPResult result;
                result.success = false;
                result.message = "Unknown operator type: " + std::to_string(token.getType());
                result.line = token.getLine();
                result.in = token.getFile();
                result.column = token.getColumn();
                result.type = token.getType();
                return result;
            }
        }
        FPResult res;
        res.success = true;
        res.message = "";
        return res;
    }

    FPResult handleNumber(FILE* fp, Token token, int scopeDepth) {
        append("_scl_push()->i = %s;\n", token.getValue().c_str());
        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", _scl_top()->i);\n");
        typeStack.push("int");
        FPResult result;
        result.success = true;
        result.message = "";
        return result;
    }

    FPResult handleDouble(FILE* fp, Token token, int scopeDepth) {
        append("_scl_push()->f = %s;\n", token.getValue().c_str());
        typeStack.push("float");
        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%f\\n\", _scl_top()->f);\n");
        FPResult result;
        result.success = true;
        result.message = "";
        return result;
    }
}
