#ifndef TOKENHANDLERS_CPP_
#define TOKENHANDLERS_CPP_

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <regex>

#include "Common.hpp"

bool handleOperator(std::fstream &fp, Token token, int scopeDepth) {
    for (int j = 0; j < scopeDepth; j++) {
        fp << "    ";
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
        default: 
            std::cerr << "Unknown operator: " << token.type << std::endl;
            return false;
    }
    return true;
}

bool handleNumber(std::fstream &fp, Token token, int scopeDepth) {
    long long num = parseNumber(token.getValue());
    for (int j = 0; j < scopeDepth; j++) {
        fp << "    ";
    }
    fp << "push_long(" << num << ");" << std::endl;
    return true;
}

bool handleFor(Token keywDeclare, Token loopVar, Token keywIn, Token from, Token keywTo, Token to, Token keywDo, std::vector<std::string>* vars, std::fstream &fp, int* scopeDepth) {
    if (keywDeclare.getType() != tok_declare) {
        std::cerr << "Expected variable declaration after 'for' keyword, but got: '" << keywDeclare.getValue() << std::endl;
        return false;
    }
    if (loopVar.getType() != tok_identifier) {
        std::cerr << "Expected identifier after 'decl', but got: '" << loopVar.getValue() << std::endl;
        return false;
    }
    if (keywIn.getType() != tok_in) {
        std::cerr << "Expected 'in' keyword in for loop header, but got: '" << keywIn.getValue() << std::endl;
        return false;
    }
    if (from.getType() != tok_number && from.getType() != tok_identifier) {
        std::cerr << "Expected number or variable after 'in', but got: '" << from.getValue() << std::endl;
        return false;
    }
    if (keywTo.getType() != tok_to) {
        std::cerr << "Expected 'to' keyword in for loop header, but got: '" << keywTo.getValue() << std::endl;
        return false;
    }
    if (to.getType() != tok_number && to.getType() != tok_identifier) {
        std::cerr << "Expected number or variable after 'to', but got: '" << to.getValue() << std::endl;
        return false;
    }
    if (keywDo.getType() != tok_do) {
        std::cerr << "Expected 'do' keyword to finish for loop header, but got: '" << keywDo.getValue() << std::endl;
        return false;
    }
    long long lower = 0;
    if (from.getType() == tok_number) {
        lower = parseNumber(from.getValue());
    }
    long long higher = 0;
    if (to.getType() == tok_number) {
        higher = parseNumber(to.getValue());
    }

    for (int j = 0; j < *scopeDepth; j++) {
        fp << "    ";
    }
    fp << "void* $" << loopVar.getValue() << ";" << std::endl;

    if (lower < higher) {
        for (int j = 0; j < *scopeDepth; j++) {
            fp << "    ";
        }
        fp << "for ($" << loopVar.getValue() << " = (void*) ";
        fp << (from.getType() == tok_identifier ? "$" : "") << from.getValue();
        fp << "; $" << loopVar.getValue() << " <= (void*) ";
        fp << (to.getType() == tok_identifier ? "$" : "") << to.getValue();
        fp << "; $" << loopVar.getValue() << "++) {" << std::endl;
    } else {
        std::cerr << "Lower bound of for loop is greater than upper bound" << std::endl;
        return false;
    }
    *scopeDepth = *scopeDepth + 1;

    vars->push_back(loopVar.getValue());
    fp << std::endl;
    return true;
}

#endif // TOKENHANDLERS_CPP_