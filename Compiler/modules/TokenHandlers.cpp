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
            case tok_add: fprintf(fp, "op_add();\n"); break;
            case tok_sub: fprintf(fp, "op_sub();\n"); break;
            case tok_mul: fprintf(fp, "op_mul();\n"); break;
            case tok_div: fprintf(fp, "op_div();\n"); break;
            case tok_mod: fprintf(fp, "op_mod();\n"); break;
            case tok_land: fprintf(fp, "op_land();\n"); break;
            case tok_lor: fprintf(fp, "op_lor();\n"); break;
            case tok_lxor: fprintf(fp, "op_lxor();\n"); break;
            case tok_lnot: fprintf(fp, "op_lnot();\n"); break;
            case tok_lsh: fprintf(fp, "op_lsh();\n"); break;
            case tok_rsh: fprintf(fp, "op_rsh();\n"); break;
            case tok_pow: fprintf(fp, "op_pow();\n"); break;
            case tok_dadd: fprintf(fp, "op_dadd();\n"); break;
            case tok_dsub: fprintf(fp, "op_dsub();\n"); break;
            case tok_dmul: fprintf(fp, "op_dmul();\n"); break;
            case tok_ddiv: fprintf(fp, "op_ddiv();\n"); break;
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
            fprintf(fp, "ctrl_push_long(%lld);\n", num);
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
            fprintf(fp, "scl_gc_addlocal(&_%s);\n", loopVar.getValue().c_str());
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
            fprintf(fp, "ctrl_push_double(%f);\n", num);
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