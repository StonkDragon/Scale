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

    bool handlePrimitiveOperator(FILE* fp, int scopeDepth, std::string op, std::string typeA) {
        if (typeA.size() == 0)
            return false;
        if (typeStack.size())
            typeStack.pop();
        std::string typeB = typeStackTop;
        if (typeStack.size())
            typeStack.pop();
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            if (strstarts(typeA, "int") || strstarts(typeA, "any")) {
                if (strstarts(typeB, "float")) {
                    append("stack.ptr -= 2; stack.data[stack.ptr++].f = stack.data[stack.ptr].f %s ((scl_float) stack.data[stack.ptr + 1].i);\n", op.c_str());
                    typeStack.push("float!");
                } else {
                    append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i %s stack.data[stack.ptr + 1].i;\n", op.c_str());
                    typeStack.push("int!");
                }
            } else {
                if (strstarts(typeB, "float")) {
                    append("stack.ptr -= 2; stack.data[stack.ptr++].f = stack.data[stack.ptr].f %s stack.data[stack.ptr + 1].f;\n", op.c_str());
                    typeStack.push("float!");
                } else {
                    append("stack.ptr -= 2; stack.data[stack.ptr++].f = ((scl_float) stack.data[stack.ptr].i) %s stack.data[stack.ptr + 1].f;\n", op.c_str());
                    typeStack.push("float!");
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
        if (strstarts(type, "int") || strstarts(type, "float") || strstarts(type, "any")) {
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
            if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
            if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                if (f->getReturnType() == "float") {
                    append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                } else {
                    append("stack.data[stack.ptr++].v = (scl_any) Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
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
                append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i + stack.data[stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
                break;
            }

            case tok_sub: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "-", typeStackTop)) break;
                append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i - stack.data[stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
                break;
            }

            case tok_mul: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "*", typeStackTop)) break;
                append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i * stack.data[stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
                break;
            }

            case tok_div: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "/", typeStackTop)) break;
                append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i / stack.data[stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
                break;
            }

            case tok_mod: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "%", typeStackTop)) break;
                append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i %% stack.data[stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
                break;
            }

            case tok_land: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "&", typeStackTop)) break;
                append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i & stack.data[stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
                break;
            }

            case tok_lor: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "|", typeStackTop)) break;
                append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i | stack.data[stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
                break;
            }

            case tok_lxor: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "^", typeStackTop)) break;
                append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i ^ stack.data[stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
                break;
            }

            case tok_lnot: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "~", typeStackTop)) break;
                append("stack.data[stack.ptr - 1].i = ~stack.data[stack.ptr - 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
                break;
            }

            case tok_lsh: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "<<", typeStackTop)) break;
                append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i << stack.data[stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
                break;
            }

            case tok_rsh: {
                if (handleOverriddenOperator(result, fp, scopeDepth, ">>", typeStackTop)) break;
                append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i >> stack.data[stack.ptr + 1].i;\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
                break;
            }

            case tok_pow: {
                if (handleOverriddenOperator(result, fp, scopeDepth, "**", typeStackTop)) break;
                append("{ scl_int exp = stack.data[--stack.ptr].i; scl_int base = stack.data[--stack.ptr].i; scl_int intResult = (scl_int) base; scl_int i = 1; while (i < exp) { intResult *= (scl_int) base; i++; } stack.data[stack.ptr++].i = intResult; }\n");
                if (typeStack.size())
                    typeStack.pop();
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push("int!");
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
        append("stack.data[stack.ptr++].i = %s;\n", token.getValue().c_str());
        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
        typeStack.push("int!");
        FPResult result;
        result.success = true;
        result.message = "";
        return result;
    }

    FPResult handleDouble(FILE* fp, Token token, int scopeDepth) {
        append("stack.data[stack.ptr++].f = %s;\n", token.getValue().c_str());
        typeStack.push("float!");
        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%f\\n\", stack.data[stack.ptr - 1].f);\n");
        FPResult result;
        result.success = true;
        result.message = "";
        return result;
    }
}
