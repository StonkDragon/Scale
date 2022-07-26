#ifndef _PARSER_CPP_
#define _PARSER_CPP_

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "Common.h"

#include "Lexer.cpp"
#include "Tokenizer.cpp"

#undef INT_MAX
#undef INT_MIN
#undef LONG_MAX
#undef LONG_MIN

#define INT_MAX 0x7FFFFFFF
#define INT_MIN 0x80000000
#define LONG_MAX 0x7FFFFFFFFFFFFFFF
#define LONG_MIN 0x8000000000000000ll

#define LINE_LENGTH 48

static std::vector<std::string> vars;

bool hasVar(std::string name) {
    for (std::string var : vars) {
        if (var == name) {
            return true;
        }
    }
    return false;
}

bool isOperator(Token token) {
    return token.type == tok_add || token.type == tok_sub || token.type == tok_mul || token.type == tok_div || token.type == tok_mod || token.type == tok_land || token.type == tok_lor || token.type == tok_lxor || token.type == tok_lnot || token.type == tok_lsh || token.type == tok_rsh;
}

ParseResult Parser::parse(std::string filename) {
    std::fstream fp((filename + std::string(".c")).c_str(), std::ios::out);
    std::fstream header((filename + std::string(".h")).c_str(), std::ios::out);
    header << "#ifndef __SCALE_COMP_H" << std::endl;
    header << "#define __SCALE_COMP_H" << std::endl;

    fp << "#ifdef __cplusplus" << std::endl;
    fp << "extern \"C\" {" << std::endl;
    fp << "#endif" << std::endl;

    header << "#ifdef __cplusplus" << std::endl;
    header << "extern \"C\" {" << std::endl;
    header << "#endif" << std::endl;

    fp << "#include \"" << getenv("HOME") << "/Scale/comp/scale.h" << "\"" << std::endl;
    header << "#include \"" << getenv("HOME") << "/Scale/comp/scale.h" << "\"" << std::endl;

    for (Prototype proto : result.prototypes) {
        fp << "void scale_func_" << proto.name << "();" << std::endl;
        header << "void scale_func_" << proto.name << "();" << std::endl;
    }

    for (Function function : result.functions) {
        for (Modifier modifier : function.modifiers) {
            switch (modifier) {
                case mod_static:
                    fp << "static ";
                    break;
                case mod_inline:
                    fp << "inline ";
                    break;
                case mod_extern:
                    fp << "extern ";
                    break;
            
                default:
                    break;
            }
        }
        fp << "void scale_func_" << function.getName() << "();" << std::endl;
        header << "void scale_func_" << function.getName() << "();" << std::endl;
    }

    for (Extern extern_ : result.externs) {
        fp << "void scale_extern_" << extern_.name << "();" << std::endl;
    }

    for (Function function : result.functions)
    {
        vars.clear();
        bool isStatic = false;
        bool isExtern = false;
        bool isInline = false;

        std::streampos lstart = fp.tellp();
        for (Modifier modifier : function.modifiers) {
            switch (modifier) {
                case mod_static:
                    isStatic = true;
                    fp << "static ";
                    break;
                case mod_inline:
                    isInline = true;
                    fp << "inline ";
                    break;
                case mod_extern:
                    isExtern = true;
                    fp << "extern ";
                    break;
            
                default:
                    break;
            }
        }
        fp << "void scale_func_" << function.getName() << "() {";
        std::streampos pos = fp.tellp();
        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
            fp << " ";
        }
        fp << "/* function " << function.getName() << "() */" << std::endl;
        if (!isInline) fp << "stacktrace_push(\"" << function.getName() << " -> head\");" << std::endl;

        for (int i = function.getArgs().size() - 1; i >= 0; i--) {
            std::string var = function.getArgs()[i];
            var = var.substr(1);
            vars.push_back(var);
            fp << "void* " << function.getArgs()[i] << " = scale_pop();" << std::endl;
        }

        if (!isInline) fp << "stacktrace_pop();" << std::endl;
        if (!isInline) fp << "stacktrace_push(\"" << function.getName() << " -> body\");" << std::endl;
        std::vector<Token> body = function.getBody();

        int scopeDepth = 1;
        for (int i = 0; i < body.size(); i++)
        {
            if (body[i].getType() == tok_ignore) continue;
            if (isOperator(body[i])) {
                switch (body[i].type) {
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
                    default: 
                        std::cerr << "Unknown operator: " << body[i].type << std::endl;
                        {
                            ParseResult result;
                            result.success = false;
                            return result;
                        }
                        break;
                }
            } else if (body[i].getType() == tok_identifier && hasFunction(body[i].getValue())) {
                lstart = fp.tellp();
                fp << "scale_func_" << body[i].getValue() << "();";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */" << std::endl;
            } else if (body[i].getType() == tok_identifier && hasExtern(body[i].getValue())) {
                fp << "stacktrace_push(\"" << body[i].getValue() << " -> native\");" << std::endl;
                lstart = fp.tellp();
                fp << "scale_extern_" << body[i].getValue() << "();";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */" << std::endl;
                fp << "stacktrace_pop();" << std::endl;
            } else if (body[i].getType() == tok_identifier && hasVar(body[i].getValue())) {
                std::string loadFrom = body[i].getValue();
                lstart = fp.tellp();
                fp << "scale_push($" << loadFrom << ");";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */" << std::endl;
            } else if (body[i].getType() == tok_string_literal) {
                lstart = fp.tellp();
                fp << "scale_push_string(\"" << body[i].getValue() << "\");";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */" << std::endl;
            } else if (body[i].getType() == tok_number) {
                
                long long num;
                try
                {
                    if (body[i].getValue().substr(0, 2) == "0x") {
                        num = std::stoll(body[i].getValue().substr(2), nullptr, 16);
                    } else if (body[i].getValue().substr(0, 2) == "0b") {
                        num = std::stoll(body[i].getValue().substr(2), nullptr, 2);
                    } else if (body[i].getValue().substr(0, 2) == "0o") {
                        num = std::stoll(body[i].getValue().substr(2), nullptr, 8);
                    } else {
                        num = std::stoll(body[i].getValue());
                    }
                    lstart = fp.tellp();
                    fp << "scale_push_long(" << num << ");";
                    pos = fp.tellp();
                    for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                        fp << " ";
                    }
                    fp << "/* " << body[i].getValue() << " */" << std::endl;
                }
                catch(const std::exception& e)
                {
                    std::cerr << "Number out of range: " << body[i].getValue() << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
            } else if (body[i].getType() == tok_nil) {
                lstart = fp.tellp();
                fp << "scale_push((void*) 0);";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */" << std::endl;
            } else if (body[i].getType() == tok_if) {
                scopeDepth++;
                lstart = fp.tellp();
                fp << "if (scale_pop_long()) {";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */";
                fp << std::endl;
            } else if (body[i].getType() == tok_else) {
                scopeDepth--;
                scopeDepth++;
                lstart = fp.tellp();
                fp << "} else {";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */";
                fp << std::endl;
            } else if (body[i].getType() == tok_while) {
                scopeDepth++;
                lstart = fp.tellp();
                fp << "while (1) {";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */";
                fp << std::endl;
            } else if (body[i].getType() == tok_do) {
                lstart = fp.tellp();
                fp << "if (!scale_pop_long()) break;";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */";
                fp << std::endl;
            } else if (body[i].getType() == tok_for) {
                if (body[i + 1].getType() != tok_declare) {
                    std::cerr << "Expected variable declaration after 'for' keyword, but got: '" << body[i + 1].getValue() << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                scopeDepth++;
                if (body[i + 2].getType() != tok_identifier) {
                    std::cerr << "Expected identifier after 'decl', but got: '" << body[i + 2].getValue() << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                if (body[i + 3].getType() != tok_in) {
                    std::cerr << "Expected 'in' keyword in for loop header, but got: '" << body[i + 3].getValue() << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                if (body[i + 4].getType() != tok_number && body[i + 4].getType() != tok_identifier) {
                    std::cerr << "Expected number or variable after 'in', but got: '" << body[i + 4].getValue() << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                if (body[i + 5].getType() != tok_to) {
                    std::cerr << "Expected 'to' keyword in for loop header, but got: '" << body[i + 5].getValue() << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                if (body[i + 6].getType() != tok_number && body[i + 6].getType() != tok_identifier) {
                    std::cerr << "Expected number or variable after 'to', but got: '" << body[i + 6].getValue() << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                if (body[i + 7].getType() != tok_do) {
                    std::cerr << "Expected 'do' keyword to finish for loop header, but got: '" << body[i + 7].getValue() << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                if (body[i + 4].getType() == tok_identifier) {
                    goto checkHigher;
                }
                long long lower;
                try
                {
                    if (body[i + 4].getValue().substr(0, 2) == "0x") {
                        lower = std::stoll(body[i + 4].getValue().substr(2), nullptr, 16);
                    } else if (body[i + 4].getValue().substr(0, 2) == "0b") {
                        lower = std::stoll(body[i + 4].getValue().substr(2), nullptr, 2);
                    } else if (body[i + 4].getValue().substr(0, 2) == "0o") {
                        lower = std::stoll(body[i + 4].getValue().substr(2), nullptr, 8);
                    } else {
                        lower = std::stoll(body[i + 4].getValue());
                    }
                }
                catch(const std::exception& e)
                {
                    std::cerr << "Number out of range: " << body[i + 4].getValue() << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
            checkHigher:
                if (body[i + 6].getType() == tok_identifier) {
                    goto checksDone;
                }
                long long higher;
                try
                {
                    if (body[i + 6].getValue().substr(0, 2) == "0x") {
                        higher = std::stoll(body[i + 6].getValue().substr(2), nullptr, 16);
                    } else if (body[i + 6].getValue().substr(0, 2) == "0b") {
                        higher = std::stoll(body[i + 6].getValue().substr(2), nullptr, 2);
                    } else if (body[i + 6].getValue().substr(0, 2) == "0o") {
                        higher = std::stoll(body[i + 6].getValue().substr(2), nullptr, 8);
                    } else {
                        higher = std::stoll(body[i + 6].getValue());
                    }
                }
                catch(const std::exception& e)
                {
                    std::cerr << "Number out of range: " << body[i + 6].getValue() << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
            checksDone:
                fp << "void* $" << body[i + 2].getValue() << ";" << std::endl;
                lstart = fp.tellp();
                if (body[i + 4].getType() == tok_identifier || body[i + 6].getType() == tok_identifier) {
                    goto finalCondition;
                }
                if (lower < higher) {
            finalCondition:
                    fp << "for ($" << body[i + 2].getValue() << " = (void*) " << body[i + 4].getValue() << "; $" << body[i + 2].getValue() << " <= (void*) " << body[i + 6].getValue() << "; $" << body[i + 2].getValue() << "++) {";
                } else {
                    std::cerr << "Lower bound of for loop is greater than upper bound" << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                vars.push_back(body[i + 2].getValue());
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* for :" << body[i + 2].getValue() << " in " << body[i + 4].getValue() << ".." << body[i + 6].getValue() << " do */";
                fp << std::endl;
                i += 7;
            } else if (body[i].getType() == tok_done || body[i].getType() == tok_fi || body[i].getType() == tok_end) {
                scopeDepth--;
                lstart = fp.tellp();
                fp << "}";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */";
                fp << std::endl;
            } else if (body[i].getType() == tok_return) {
                lstart = fp.tellp();
                fp << "return;";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */";
                fp << std::endl;
            } else if (body[i].getType() == tok_number_float) {
                double num;
                try
                {
                    num = std::stold(body[i].getValue());
                    lstart = fp.tellp();
                    fp << "scale_push_double(" << num << ");";
                    pos = fp.tellp();
                    for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                        fp << " ";
                    }
                    fp << "/* " << body[i].getValue() << " */" << std::endl;
                }
                catch(const std::exception& e)
                {
                    std::cerr << "Number out of range: " << body[i].getValue() << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
            } else if (body[i].getType() == tok_addr_ref) {
                std::string toGet = body[i + 1].getValue();
                lstart = fp.tellp();
                if (hasExtern(toGet)) {
                    fp << "scale_push((void*) &scale_extern_" << toGet << ");";
                } else if (hasFunction(toGet)) {
                    fp << "scale_push((void*) &scale_func_" << toGet << ");";
                } else {
                    fp << "scale_push((void*) &" << toGet << ");";
                }
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << body[i + 1].getValue() << " */" << std::endl;
                i++;
            } else if (body[i].getType() == tok_store) {
                if (body[i + 1].getType() != tok_identifier) {
                    std::cerr << "Error: '" << body[i + 1].getValue() << "' is not an identifier!" << std::endl;
                }
                if (!hasVar(body[i + 1].getValue())) {
                    std::cerr << "Error: Use of undefined variable '" << body[i + 1].getValue() << "'" << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                std::string storeIn = body[i + 1].getValue();
                lstart = fp.tellp();
                fp << "$" << storeIn << " = scale_pop();";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " " << body[i + 1].getValue() << " */" << std::endl;
                i++;
            } else if (body[i].getType() == tok_declare) {
                if (body[i + 1].getType() != tok_identifier) {
                    std::cerr << "Error: '" << body[i + 1].getValue() << "' is not an identifier!" << std::endl;
                }
                vars.push_back(body[i + 1].getValue());
                std::string loadFrom = body[i + 1].getValue();
                lstart = fp.tellp();
                fp << "void* $" << loadFrom << ";";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " " << body[i + 1].getValue() << " */" << std::endl;
                i++;
            } else if (body[i].getType() == tok_continue) {
                lstart = fp.tellp();
                fp << "continue;";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */" << std::endl;
            } else if (body[i].getType() == tok_break) {
                lstart = fp.tellp();
                fp << "break;";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */" << std::endl;
            } else {
                std::cerr << "Error: Unknown token: '" << body[i].getValue() << "'" << std::endl;
                ParseResult result;
                result.success = false;
                return result;
            }
        }
        lstart = fp.tellp();
        if (!isInline) fp << "stacktrace_pop();";
        pos = fp.tellp();
        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
            fp << " ";
        }
        fp << "/* end */" << std::endl;
        fp << "}";
        fp << std::endl;
    }

    fp << "#ifdef __cplusplus" << std::endl;
    fp << "}" << std::endl;
    fp << "#endif" << std::endl;
    fp << std::endl;

    header << "#endif" << std::endl;
    header << std::endl;
    header << "#ifdef __cplusplus" << std::endl;
    header << "}" << std::endl;
    header << "#endif" << std::endl;
    
    fp.close();
    header.close();

    ParseResult parseResult;
    parseResult.success = true;
    return parseResult;
}

#endif // _PARSER_CPP_