#ifndef TOKENHANDLERS_CPP_
#define TOKENHANDLERS_CPP_

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <regex>

#include "../headers/Common.hpp"
namespace sclc
{
    FPResult handleOperator(FILE* fp, Token token, int scopeDepth) {
        for (int j = 0; j < scopeDepth; j++) {
            fprintf(fp, "  ");
        }
        switch (token.getType()) {
            case tok_add: fprintf(fp, "{ scl_int b = stack.data[--stack.ptr].i; scl_int a = stack.data[--stack.ptr].i; stack.data[stack.ptr++].i = a + b; }\n"); break;
            case tok_sub: fprintf(fp, "{ scl_int b = stack.data[--stack.ptr].i; scl_int a = stack.data[--stack.ptr].i; stack.data[stack.ptr++].i = a - b; }\n"); break;
            case tok_mul: fprintf(fp, "{ scl_int b = stack.data[--stack.ptr].i; scl_int a = stack.data[--stack.ptr].i; stack.data[stack.ptr++].i = a * b; }\n"); break;
            case tok_div: fprintf(fp, "{ scl_int b = stack.data[--stack.ptr].i; scl_int a = stack.data[--stack.ptr].i; stack.data[stack.ptr++].i = a / b; }\n"); break;
            case tok_mod: fprintf(fp, "{ scl_int b = stack.data[--stack.ptr].i; scl_int a = stack.data[--stack.ptr].i; stack.data[stack.ptr++].i = a %% b; }\n"); break;
            case tok_land: fprintf(fp, "{ scl_int b = stack.data[--stack.ptr].i; scl_int a = stack.data[--stack.ptr].i; stack.data[stack.ptr++].i = a & b; }\n"); break;
            case tok_lor: fprintf(fp, "{ scl_int b = stack.data[--stack.ptr].i; scl_int a = stack.data[--stack.ptr].i; stack.data[stack.ptr++].i = a | b; }\n"); break;
            case tok_lxor: fprintf(fp, "{ scl_int b = stack.data[--stack.ptr].i; scl_int a = stack.data[--stack.ptr].i; stack.data[stack.ptr++].i = a ^ b; }\n"); break;
            case tok_lnot: fprintf(fp, "stack.data[stack.ptr - 1].i = ~((scl_int) stack.data[stack.ptr - 1].i);\n"); break;
            case tok_lsh: fprintf(fp, "{ scl_int b = stack.data[--stack.ptr].i; scl_int a = stack.data[--stack.ptr].i; stack.data[stack.ptr++].i = a << b; }\n"); break;
            case tok_rsh: fprintf(fp, "{ scl_int b = stack.data[--stack.ptr].i; scl_int a = stack.data[--stack.ptr].i; stack.data[stack.ptr++].i = a >> b; }\n"); break;
            case tok_pow: fprintf(fp, "{ scl_int exp = stack.data[--stack.ptr].i; scl_int base = stack.data[--stack.ptr].i; scl_int intResult = (scl_int) base; scl_int i = 0; while (i < exp) { intResult *= (scl_int) base; i++; } stack.data[stack.ptr++].i = intResult; }\n"); break;
            case tok_dadd: fprintf(fp, "{ scl_float n2 = stack.data[--stack.ptr].f; scl_float n1 = stack.data[--stack.ptr].f; stack.data[stack.ptr++].f = n1 + n2; }\n"); break;
            case tok_dsub: fprintf(fp, "{ scl_float n2 = stack.data[--stack.ptr].f; scl_float n1 = stack.data[--stack.ptr].f; stack.data[stack.ptr++].f = n1 - n2; }\n"); break;
            case tok_dmul: fprintf(fp, "{ scl_float n2 = stack.data[--stack.ptr].f; scl_float n1 = stack.data[--stack.ptr].f; stack.data[stack.ptr++].f = n1 * n2; }\n"); break;
            case tok_ddiv: fprintf(fp, "{ scl_float n2 = stack.data[--stack.ptr].f; scl_float n1 = stack.data[--stack.ptr].f; stack.data[stack.ptr++].f = n1 / n2; }\n"); break;
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
            for (int j = 0; j < scopeDepth; j++) {
                fprintf(fp, "  ");
            }
            fprintf(fp, "stack.data[stack.ptr++].i = %lld;\n", num);
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

    FPResult handleFor(Token keywDeclare, Token loopVar, Token keywIn, Token from, Token keywTo, Token to, Token keywDo, std::vector<std::string>* vars, FILE* fp, int* scopeDepth) {
        if (keywDeclare.getType() != tok_declare) {
            FPResult result;
            result.message = "Expected variable declaration after 'for' keyword, but got: '" + keywDeclare.getValue() + "'";
            result.success = false;
            result.line = keywDeclare.getLine();
            result.in = keywDeclare.getFile();
            result.value = keywDeclare.getValue();
            result.column = keywDeclare.getColumn();
            result.type = keywDeclare.getType();
            return result;
        }
        if (loopVar.getType() != tok_identifier) {
            FPResult result;
            result.message = "Expected identifier after 'decl', but got: '" + loopVar.getValue() + "'";
            result.success = false;
            result.line = loopVar.getLine();
            result.in = loopVar.getFile();
            result.value = loopVar.getValue();
            result.column = loopVar.getColumn();
            result.type = loopVar.getType();
            return result;
        }
        if (keywIn.getType() != tok_in) {
            FPResult result;
            result.message = "Expected 'in' keyword in for loop header, but got: '" + keywIn.getValue() + "'";
            result.success = false;
            result.line = keywIn.getLine();
            result.in = keywIn.getFile();
            result.value = keywIn.getValue();
            result.column = keywIn.getColumn();
            result.type = keywIn.getType();
            return result;
        }
        if (from.getType() != tok_number) {
            FPResult result;
            result.message = "Expected integer after 'in', but got: '" + from.getValue() + "'";
            result.success = false;
            result.line = from.getLine();
            result.in = from.getFile();
            result.value = from.getValue();
            result.column = from.getColumn();
            result.type = from.getType();
            return result;
        }
        if (keywTo.getType() != tok_to) {
            FPResult result;
            result.message = "Expected 'to' keyword in for loop header, but got: '" + keywTo.getValue() + "'";
            result.success = false;
            result.line = keywTo.getLine();
            result.in = keywTo.getFile();
            result.value = keywTo.getValue();
            result.column = keywTo.getColumn();
            result.type = keywTo.getType();
            return result;
        }
        if (to.getType() != tok_number) {
            FPResult result;
            result.message = "Expected integer after 'to', but got: '" + to.getValue() + "'";
            result.success = false;
            result.line = to.getLine();
            result.in = to.getFile();
            result.value = to.getValue();
            result.column = to.getColumn();
            result.type = to.getType();
            return result;
        }
        if (keywDo.getType() != tok_do) {
            FPResult result;
            result.message = "Expected 'do' keyword to finish for loop header, but got: '" + keywDo.getValue() + "'";
            result.success = false;
            result.line = keywDo.getLine();
            result.in = keywDo.getFile();
            result.value = keywDo.getValue();
            result.column = keywDo.getColumn();
            result.type = keywDo.getType();
            return result;
        }

        long long lower = parseNumber(from.getValue());
        long long higher = parseNumber(to.getValue());

        for (int j = 0; j < *scopeDepth; j++) {
            fprintf(fp, "  ");
        }
        if (from.getType() == tok_identifier && !hasVar(from)) {
            FPResult result;
            result.message = "Use of variables in for loops is not supported.";
            result.success = false;
            result.line = from.getLine();
            result.in = from.getFile();
            result.value = from.getValue();
            result.column = from.getColumn();
            result.type = from.getType();
            return result;
        }
        if (to.getType() == tok_identifier && !hasVar(to)) {
            FPResult result;
            result.message = "Use of variables in for loops is not supported.";
            result.success = false;
            result.line = to.getLine();
            result.in = to.getFile();
            result.value = to.getValue();
            result.column = to.getColumn();
            result.type = to.getType();
            return result;
        }
        if (from.getType() == tok_identifier && hasContainer(Main.parser->getResult(), from)) {
            FPResult result;
            result.message = "Use of containers in for loops is not supported.";
            result.success = false;
            result.line = from.getLine();
            result.in = from.getFile();
            result.value = from.getValue();
            result.column = from.getColumn();
            result.type = from.getType();
            return result;
        }
        if (to.getType() == tok_identifier && hasContainer(Main.parser->getResult(), to)) {
            FPResult result;
            result.message = "Use of containers in for loops is not supported.";
            result.success = false;
            result.line = to.getLine();
            result.in = to.getFile();
            result.value = to.getValue();
            result.column = to.getColumn();
            result.type = to.getType();
            return result;
        }

        if (!hasVar(loopVar)) {
            fprintf(fp, "scl_value _%s;", loopVar.getValue().c_str());
        }

        std::vector<FPResult> warns;

        if (hasFunction(Main.parser->getResult(), loopVar)) {
            FPResult result;
            result.message = "Variable '" + loopVar.getValue() + "' shadowed by function '" + loopVar.getValue() + "'";
            result.success = false;
            result.line = loopVar.getLine();
            result.in = loopVar.getFile();
            result.value = loopVar.getValue();
            result.type =  loopVar.getType();
            result.column = loopVar.getColumn();
            warns.push_back(result);
        }
        if (hasContainer(Main.parser->getResult(), loopVar)) {
            FPResult result;
            result.message = "Variable '" + loopVar.getValue() + "' shadowed by container '" + loopVar.getValue() + "'";
            result.success = false;
            result.line = loopVar.getLine();
            result.in = loopVar.getFile();
            result.value = loopVar.getValue();
            result.type =  loopVar.getType();
            result.column = loopVar.getColumn();
            warns.push_back(result);
        }
        if (hasVar(loopVar)) {
            FPResult result;
            result.message = "Variable '" + loopVar.getValue() + "' is already declared and shadows it.";
            result.success = false;
            result.line = loopVar.getLine();
            result.in = loopVar.getFile();
            result.value = loopVar.getValue();
            result.type =  loopVar.getType();
            result.column = loopVar.getColumn();
            warns.push_back(result);
        }

        fprintf(fp, "for (_%s = (void*) ", loopVar.getValue().c_str());
        fprintf(fp, "%s", from.getValue().c_str());

        if (lower < higher) {
            fprintf(fp, "; _%s < (void*) ", loopVar.getValue().c_str());
            fprintf(fp, "%s", to.getValue().c_str());
            fprintf(fp, "; _%s++) {\n", loopVar.getValue().c_str());
        } else if (lower > higher) {
            fprintf(fp, "; _%s > (void*) ", loopVar.getValue().c_str());
            fprintf(fp, "%s", to.getValue().c_str());
            fprintf(fp, "; _%s--) {\n", loopVar.getValue().c_str());
        } else {
            FPResult result;
            result.message = "Invalid looping condition.";
            result.success = false;
            result.line = keywTo.getLine();
            result.in = keywTo.getFile();
            result.value = keywTo.getValue();
            result.column = keywTo.getColumn();
            result.type = keywTo.getType();
            return result;
        }
        (*scopeDepth)++;

        vars->push_back(loopVar.getValue());
        FPResult result;
        result.success = true;
        result.message = "";
        result.warns = warns;
        return result;
    }

    FPResult handleDouble(FILE* fp, Token token, int scopeDepth) {
        try {
            double num = parseDouble(token.getValue());
            for (int j = 0; j < scopeDepth; j++) {
                fprintf(fp, "  ");
            }
            fprintf(fp, "stack.data[stack.ptr++].d = %f;\n", num);
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

#endif // TOKENHANDLERS_CPP_