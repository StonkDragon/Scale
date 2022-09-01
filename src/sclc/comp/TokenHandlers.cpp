#ifndef TOKENHANDLERS_CPP_
#define TOKENHANDLERS_CPP_

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <regex>

#include "../Common.hpp"
namespace sclc
{
    ParseResult handleOperator(std::fstream &fp, Token token, int scopeDepth) {
        for (int j = 0; j < scopeDepth; j++) {
            fp << "  ";
        }
        switch (token.type) {
            case tok_add: fp << "op_add();" << std::endl; break;
            case tok_sub: fp << "op_sub();" << std::endl; break;
            case tok_mul: fp << "op_mul();" << std::endl; break;
            case tok_div: fp << "op_div();" << std::endl; break;
            case tok_mod: fp << "op_mod();" << std::endl; break;
            case tok_land: fp << "op_land();" << std::endl; break;
            case tok_lor: fp << "op_lor();" << std::endl; break;
            case tok_lxor: fp << "op_lxor();" << std::endl; break;
            case tok_lnot: fp << "op_lnot();" << std::endl; break;
            case tok_lsh: fp << "op_lsh();" << std::endl; break;
            case tok_rsh: fp << "op_rsh();" << std::endl; break;
            case tok_pow: fp << "op_pow();" << std::endl; break;
            case tok_dadd: fp << "op_dadd();" << std::endl; break;
            case tok_dsub: fp << "op_dsub();" << std::endl; break;
            case tok_dmul: fp << "op_dmul();" << std::endl; break;
            case tok_ddiv: fp << "op_ddiv();" << std::endl; break;
            default:
            {
                ParseResult result;
                result.success = false;
                result.message = "Unknown operator type: " + std::to_string(token.type);
                result.where = token.getLine();
                result.in = token.getFile();
                result.column = token.getColumn();
                return result;
            }
        }
        ParseResult result;
        result.success = true;
        result.message = "";
        return result;
    }

    ParseResult handleNumber(std::fstream &fp, Token token, int scopeDepth) {
        try {
            long long num = parseNumber(token.getValue());
            for (int j = 0; j < scopeDepth; j++) {
                fp << "  ";
            }
            fp << "ctrl_push_long(" << num << ");" << std::endl;
        } catch (std::exception &e) {
            ParseResult result;
            result.success = false;
            result.message = "Error parsing number: " + token.getValue() + ": " + e.what();
            result.where = token.getLine();
            result.in = token.getFile();
            result.token = token.getValue();
            result.column = token.getColumn();
            return result;
        }
        ParseResult result;
        result.success = true;
        result.message = "";
        return result;
    }

    ParseResult handleFor(Token keywDeclare, Token loopVar, Token keywIn, Token from, Token keywTo, Token to, Token keywDo, std::vector<std::string>* vars, std::fstream &fp, int* scopeDepth) {
        if (keywDeclare.getType() != tok_declare) {
            ParseResult result;
            result.message = "Expected variable declaration after 'for' keyword, but got: '" + keywDeclare.getValue() + "'";
            result.success = false;
            result.where = keywDeclare.getLine();
            result.in = keywDeclare.getFile();
            result.token = keywDeclare.getValue();
            result.column = keywDeclare.getColumn();
            return result;
        }
        if (loopVar.getType() != tok_identifier) {
            ParseResult result;
            result.message = "Expected identifier after 'decl', but got: '" + loopVar.getValue() + "'";
            result.success = false;
            result.where = loopVar.getLine();
            result.in = loopVar.getFile();
            result.token = loopVar.getValue();
            result.column = loopVar.getColumn();
            return result;
        }
        if (keywIn.getType() != tok_in) {
            ParseResult result;
            result.message = "Expected 'in' keyword in for loop header, but got: '" + keywIn.getValue() + "'";
            result.success = false;
            result.where = keywIn.getLine();
            result.in = keywIn.getFile();
            result.token = keywIn.getValue();
            result.column = keywIn.getColumn();
            return result;
        }
        if (from.getType() != tok_number && from.getType() != tok_identifier) {
            ParseResult result;
            result.message = "Expected number or variable after 'in', but got: '" + from.getValue() + "'";
            result.success = false;
            result.where = from.getLine();
            result.in = from.getFile();
            result.token = from.getValue();
            result.column = from.getColumn();
            return result;
        }
        if (keywTo.getType() != tok_to) {
            ParseResult result;
            result.message = "Expected 'to' keyword in for loop header, but got: '" + keywTo.getValue() + "'";
            result.success = false;
            result.where = keywTo.getLine();
            result.in = keywTo.getFile();
            result.token = keywTo.getValue();
            result.column = keywTo.getColumn();
            return result;
        }
        if (to.getType() != tok_number && to.getType() != tok_identifier) {
            ParseResult result;
            result.message = "Expected number or variable after 'to', but got: '" + to.getValue() + "'";
            result.success = false;
            result.where = to.getLine();
            result.in = to.getFile();
            result.token = to.getValue();
            result.column = to.getColumn();
            return result;
        }
        if (keywDo.getType() != tok_do) {
            ParseResult result;
            result.message = "Expected 'do' keyword to finish for loop header, but got: '" + keywDo.getValue() + "'";
            result.success = false;
            result.where = keywDo.getLine();
            result.in = keywDo.getFile();
            result.token = keywDo.getValue();
            result.column = keywDo.getColumn();
            return result;
        }
        bool doNumberCheck = false;
        long long lower = 0;
        if (from.getType() == tok_number) {
            lower = parseNumber(from.getValue());
            doNumberCheck = true;
        } else if (from.getType() != tok_identifier) {
            ParseResult result;
            result.message = "Expected number or variable after 'in', but got: '" + from.getValue() + "'";
            result.success = false;
            result.where = from.getLine();
            result.in = from.getFile();
            result.token = from.getValue();
            result.column = from.getColumn();
            return result;
        }
        long long higher = 0;
        if (to.getType() == tok_number) {
            higher = parseNumber(to.getValue());
            doNumberCheck = true;
        } else if (to.getType() != tok_identifier) {
            ParseResult result;
            result.message = "Expected number or variable after 'to', but got: '" + to.getValue() + "'";
            result.success = false;
            result.where = to.getLine();
            result.in = to.getFile();
            result.token = to.getValue();
            result.column = to.getColumn();
            return result;
        }

        bool varExists = false;
        for (size_t i = 0; i < vars->size(); i++) {
            if (vars->at(i) == loopVar.getValue()) {
                varExists = true;
                break;
            }
        }
        if (!varExists) {
            for (int j = 0; j < *scopeDepth; j++) {
                fp << "  ";
            }
            fp << "scl_word _" << loopVar.getValue() << ";" << std::endl;
        }

        if (!doNumberCheck || lower <= higher) {
            for (int j = 0; j < *scopeDepth; j++) {
                fp << "  ";
            }
            if (from.getType() == tok_identifier && !hasVar(from)) {
                ParseResult result;
                result.message = "Use of undeclared variable: '" + from.getValue() + "'";
                result.success = false;
                result.where = from.getLine();
                result.in = from.getFile();
                result.token = from.getValue();
                result.column = from.getColumn();
                return result;
            }
            if (to.getType() == tok_identifier && !hasVar(to)) {
                ParseResult result;
                result.message = "Use of undeclared variable: '" + to.getValue() + "'";
                result.success = false;
                result.where = to.getLine();
                result.in = to.getFile();
                result.token = to.getValue();
                result.column = to.getColumn();
                return result;
            }

            fp << "for (_" << loopVar.getValue() << " = (void*) ";
            fp << (from.getType() == tok_identifier ? "_" : "") << from.getValue();
            fp << "; _" << loopVar.getValue() << " <= (void*) ";
            fp << (to.getType() == tok_identifier ? "_" : "") << to.getValue();
            fp << "; _" << loopVar.getValue() << "++) {" << std::endl;
        } else {
            ParseResult result;
            result.message = "Lower bound of for loop is greater than upper bound";
            result.success = false;
            result.where = from.getLine();
            result.in = from.getFile();
            result.token = from.getValue();
            result.column = from.getColumn();
            return result;
        }
        *scopeDepth = *scopeDepth + 1;

        vars->push_back(loopVar.getValue());
        ParseResult result;
        result.success = true;
        result.message = "";
        return result;
    }

    ParseResult handleDouble(std::fstream &fp, Token token, int scopeDepth) {
        try {
            double num = parseDouble(token.getValue());
            for (int j = 0; j < scopeDepth; j++) {
                fp << "  ";
            }
            fp << "ctrl_push_double(" << num << ");" << std::endl;
        } catch (std::exception &e) {
            ParseResult result;
            result.success = false;
            result.message = "Error parsing number: " + token.getValue() + ": " + e.what();
            result.where = token.getLine();
            result.in = token.getFile();
            result.token = token.getValue();
            result.column = token.getColumn();
            return result;
        }
        ParseResult result;
        result.success = true;
        result.message = "";
        return result;
    }
}

#endif // TOKENHANDLERS_CPP_