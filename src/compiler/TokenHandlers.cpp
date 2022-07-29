#ifndef TOKENHANDLERS_CPP_
#define TOKENHANDLERS_CPP_

#include <iostream>
#include <string>

#include "Common.hpp"

bool handleOperator(std::fstream &fp, Token token) {
    switch (token.type) {
        case tok_add: fp << "scale_op_add();" << std::endl; break;
        case tok_sub: fp << "scale_op_sub();" << std::endl; break;
        case tok_mul: fp << "scale_op_mul();" << std::endl; break;
        case tok_div: fp << "scale_op_div();" << std::endl; break;
        case tok_mod: fp << "scale_op_mod();" << std::endl; break;
        case tok_land: fp << "scale_op_land();" << std::endl; break;
        case tok_lor: fp << "scale_op_lor();" << std::endl; break;
        case tok_lxor: fp << "scale_op_lxor();" << std::endl; break;
        case tok_lnot: fp << "scale_op_lnot();" << std::endl; break;
        case tok_lsh: fp << "scale_op_lsh();" << std::endl; break;
        case tok_rsh: fp << "scale_op_rsh();" << std::endl; break;
        case tok_pow: fp << "scale_op_pow();" << std::endl; break;
        default: 
            std::cerr << "Unknown operator: " << token.type << std::endl;
            return false;
    }
    return true;
}

bool handleNumber(std::fstream &fp, Token token) {
    long long num = parseNumber(token.getValue());
    std::streampos lstart = fp.tellp();
    fp << "scale_push_long(" << num << ");";
    addSourceComment(fp, lstart, token.getValue());
    return true;
}

bool handleFor(Token keywDeclare, Token loopVar, Token keywIn, Token from, Token keywTo, Token to, Token keywDo, std::vector<std::string>* vars, std::fstream &fp, int* scopeDepth) {
    if (keywDeclare.getType() != tok_declare) {
        std::cerr << "Expected variable declaration after 'for' keyword, but got: '" << keywDeclare.getValue() << std::endl;
        return false;
    }
    *scopeDepth++;
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

    fp << "stackframe_t $" << loopVar.getValue() << ";" << std::endl;
    std::streampos lstart = fp.tellp();

    if (lower < higher) {
        fp << "for ($" << loopVar.getValue() << ".ptr = (void*) ";
        fp << (from.getType() == tok_identifier ? "$" : "") << from.getValue();
        fp << (from.getType() == tok_identifier ? ".ptr" : "") << "; $";
        fp << loopVar.getValue() << ".ptr <= (void*) ";
        fp << (to.getType() == tok_identifier ? "$" : "") << to.getValue();
        fp << (to.getType() == tok_identifier ? ".ptr" : "") << "; $";
        fp << loopVar.getValue() << ".ptr++) {";
    } else {
        std::cerr << "Lower bound of for loop is greater than upper bound" << std::endl;
        return false;
    }

    vars->push_back(loopVar.getValue());
    addSourceComment(fp, lstart, "for :" + loopVar.getValue() + " in " + from.getValue() + ".." + to.getValue() + " do");
    fp << std::endl;
}

#endif // TOKENHANDLERS_CPP_