#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <regex>

#include "../headers/Common.hpp"

namespace sclc
{
    FPResult handleOperator(FILE* fp, Token token, int scopeDepth) {
        switch (token.getType()) {
            case tok_add: append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i + stack.data[stack.ptr + 1].i;\n"); break;
            case tok_sub: append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i - stack.data[stack.ptr + 1].i;\n"); break;
            case tok_mul: append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i * stack.data[stack.ptr + 1].i;\n"); break;
            case tok_div: append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i / stack.data[stack.ptr + 1].i;\n"); break;
            case tok_mod: append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i %% stack.data[stack.ptr + 1].i;\n"); break;
            case tok_land: append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i & stack.data[stack.ptr + 1].i;\n"); break;
            case tok_lor: append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i | stack.data[stack.ptr + 1].i;\n"); break;
            case tok_lxor: append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i ^ stack.data[stack.ptr + 1].i;\n"); break;
            case tok_lnot: append("stack.data[stack.ptr - 1].i = ~stack.data[stack.ptr - 1].i;\n"); break;
            case tok_lsh: append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i << stack.data[stack.ptr + 1].i;\n"); break;
            case tok_rsh: append("stack.ptr -= 2; stack.data[stack.ptr++].i = stack.data[stack.ptr].i >> stack.data[stack.ptr + 1].i;\n"); break;
            case tok_pow: append("{ scl_int exp = stack.data[--stack.ptr].i; scl_int base = stack.data[--stack.ptr].i; scl_int intResult = (scl_int) base; scl_int i = 1; while (i < exp) { intResult *= (scl_int) base; i++; } stack.data[stack.ptr++].i = intResult; }\n"); break;
            case tok_dadd: append("stack.ptr -= 2; stack.data[stack.ptr++].f = stack.data[stack.ptr].f + stack.data[stack.ptr + 1].f;\n"); break;
            case tok_dsub: append("stack.ptr -= 2; stack.data[stack.ptr++].f = stack.data[stack.ptr].f - stack.data[stack.ptr + 1].f;\n"); break;
            case tok_dmul: append("stack.ptr -= 2; stack.data[stack.ptr++].f = stack.data[stack.ptr].f * stack.data[stack.ptr + 1].f;\n"); break;
            case tok_ddiv: append("stack.ptr -= 2; stack.data[stack.ptr++].f = stack.data[stack.ptr].f / stack.data[stack.ptr + 1].f;\n"); break;
            default:
            {
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
        FPResult result;
        result.success = true;
        result.message = "";
        return result;
    }

    FPResult handleNumber(FILE* fp, Token token, int scopeDepth) {
        try {
            long long num = parseNumber(token.getValue());
            append("stack.data[stack.ptr++].i = %lld;\n", num);
            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
        } catch (std::exception &e) {
            FPResult result;
            result.success = false;
            result.message = "Error parsing number: " + token.getValue() + ": " + e.what();
            result.line = token.getLine();
            result.in = token.getFile();
            result.value = token.getValue();
            result.column = token.getColumn();
            result.type = token.getType();
            return result;
        }
        FPResult result;
        result.success = true;
        result.message = "";
        return result;
    }

    FPResult handleDouble(FILE* fp, Token token, int scopeDepth) {
        try {
            double num = parseDouble(token.getValue());
            append("stack.data[stack.ptr++].f = %f;\n", num);
            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%f\\n\", stack.data[stack.ptr - 1].f);\n");
        } catch (std::exception &e) {
            FPResult result;
            result.success = false;
            result.message = "Error parsing number: " + token.getValue() + ": " + e.what();
            result.line = token.getLine();
            result.in = token.getFile();
            result.value = token.getValue();
            result.column = token.getColumn();
            result.type = token.getType();
            return result;
        }
        FPResult result;
        result.success = true;
        result.message = "";
        return result;
    }
}
