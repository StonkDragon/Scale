#ifndef _PARSER_CPP_
#define _PARSER_CPP_

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "Common.hpp"

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

#include "TokenHandlers.cpp"

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
    return token.type == tok_add || token.type == tok_sub || token.type == tok_mul || token.type == tok_div || token.type == tok_mod || token.type == tok_land || token.type == tok_lor || token.type == tok_lxor || token.type == tok_lnot || token.type == tok_lsh || token.type == tok_rsh || token.type == tok_pow;
}

void addSourceComment(std::fstream &fp, std::streampos lstart, std::string cmd) {
    std::streampos pos = fp.tellp();
    for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
        fp << " ";
    }
    fp << "/* " << cmd << " */";
}

ParseResult Parser::parse(std::string filename) {
    std::fstream fp((filename + std::string(".c")).c_str(), std::ios::out);
    std::fstream header((filename + std::string(".h")).c_str(), std::ios::out);
    header << "#ifndef __SCALE_COMP_H" << std::endl;
    header << "#define __SCALE_COMP_H" << std::endl;

    fp     << "#ifdef __cplusplus" << std::endl;
    fp     << "extern \"C\" {" << std::endl;
    fp     << "#endif" << std::endl;

    header << "#ifdef __cplusplus" << std::endl;
    header << "extern \"C\" {" << std::endl;
    header << "#endif" << std::endl;

    fp     << "#include <" << "scale.h" << ">" << std::endl;
    header << "#include <" << "scale.h" << ">" << std::endl;

    for (Prototype proto : result.prototypes) {
        fp     << "void scale_func_" << proto.name << "();" << std::endl;
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
        fp     << "void scale_func_" << function.getName() << "();" << std::endl;
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
        
        addSourceComment(fp, lstart, function.getName());

        if (!isInline) fp << "stacktrace_push(\"" << function.getName() << " -> head\", 1);" << std::endl;

        for (int i = function.getArgs().size() - 1; i >= 0; i--) {
            std::string var = function.getArgs()[i];
            var = var.substr(1);
            vars.push_back(var);
            fp << "void* " << function.getArgs()[i] << " = scale_pop();" << std::endl;
        }

        if (!isInline) fp << "stacktrace_pop(0);" << std::endl;
        if (!isInline) fp << "stacktrace_push(\"" << function.getName() << " -> body\", 0);" << std::endl;
        std::vector<Token> body = function.getBody();

        int scopeDepth = 1;
        for (int i = 0; i < body.size(); i++)
        {
            if (body[i].getType() == tok_ignore) continue;
            if (isOperator(body[i])) {
                bool operatorsHandled = handleOperator(fp, body[i]);
                if (!operatorsHandled) {
                    ParseResult result;
                    result.success = false;
                    return result;
                }
            } else if (body[i].getType() == tok_identifier && hasFunction(body[i].getValue())) {
                lstart = fp.tellp();
                fp << "scale_func_" << body[i].getValue() << "();";
                addSourceComment(fp, lstart, body[i].getValue());
            } else if (body[i].getType() == tok_identifier && hasExtern(body[i].getValue())) {
                fp << "stacktrace_push(\"" << body[i].getValue() << " -> native\", 0);" << std::endl;
                lstart = fp.tellp();
                fp << "scale_extern_" << body[i].getValue() << "();";
                addSourceComment(fp, lstart, body[i].getValue());
                fp << "stacktrace_pop(0);" << std::endl;
            } else if (body[i].getType() == tok_identifier && hasVar(body[i].getValue())) {
                std::string loadFrom = body[i].getValue();
                lstart = fp.tellp();
                fp << "scale_push($" << loadFrom << ");";
                addSourceComment(fp, lstart, body[i].getValue());
            } else if (body[i].getType() == tok_string_literal) {
                lstart = fp.tellp();
                fp << "scale_push_string(\"" << body[i].getValue() << "\");";
                addSourceComment(fp, lstart, body[i].getValue());
            } else if (body[i].getType() == tok_number) {
                bool numberHandled = handleNumber(fp, body[i]);
                if (!numberHandled) {
                    ParseResult result;
                    result.success = false;
                    return result;
                }
            } else if (body[i].getType() == tok_nil) {
                lstart = fp.tellp();
                fp << "scale_push((void*) 0);";
                addSourceComment(fp, lstart, body[i].getValue());
            } else if (body[i].getType() == tok_if) {
                scopeDepth++;
                lstart = fp.tellp();
                fp << "if (scale_pop_long()) {";
                addSourceComment(fp, lstart, body[i].getValue());
                fp << std::endl;
            } else if (body[i].getType() == tok_else) {
                scopeDepth--;
                scopeDepth++;
                lstart = fp.tellp();
                fp << "} else {";
                addSourceComment(fp, lstart, body[i].getValue());
                fp << std::endl;
            } else if (body[i].getType() == tok_while) {
                scopeDepth++;
                lstart = fp.tellp();
                fp << "while (1) {";
                addSourceComment(fp, lstart, body[i].getValue());
                fp << std::endl;
            } else if (body[i].getType() == tok_do) {
                lstart = fp.tellp();
                fp << "if (!scale_pop_long()) break;";
                addSourceComment(fp, lstart, body[i].getValue());
                fp << std::endl;
            } else if (body[i].getType() == tok_for) {
                bool forHandled = handleFor(
                    body[i+1],
                    body[i+2],
                    body[i+3],
                    body[i+4],
                    body[i+5],
                    body[i+6],
                    body[i+7],
                    &vars,
                    fp,
                    &scopeDepth
                );
                if (!forHandled) {
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                i += 7;
            } else if (body[i].getType() == tok_done || body[i].getType() == tok_fi || body[i].getType() == tok_end) {
                scopeDepth--;
                lstart = fp.tellp();
                fp << "}";
                addSourceComment(fp, lstart, body[i].getValue());
                fp << std::endl;
            } else if (body[i].getType() == tok_return) {
                lstart = fp.tellp();
                fp << "return;";
                addSourceComment(fp, lstart, body[i].getValue());
                fp << std::endl;
            } else if (body[i].getType() == tok_number_float) {
                double d = parseDouble(body[i].getValue());
                lstart = fp.tellp();
                fp << "scale_push_double(" << d << ");";
                addSourceComment(fp, lstart, body[i].getValue());
            } else if (body[i].getType() == tok_addr_ref) {
                std::string toGet = body[i + 1].getValue();
                lstart = fp.tellp();
                if (hasExtern(toGet)) {
                    fp << "scale_push((void*) &scale_extern_" << toGet << ");";
                } else if (hasFunction(toGet)) {
                    fp << "scale_push((void*) &scale_func_" << toGet << ");";
                } else {
                    fp << "scale_push($" << toGet << ");";
                }
                addSourceComment(fp, lstart, body[i].getValue());
                i++;
            } else if (body[i].getType() == tok_store) {
                if (body[i + 1].getType() != tok_identifier) {
                    std::cerr << "Error: '" << body[i + 1].getValue() << "' is not an identifier!" << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
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
                addSourceComment(fp, lstart, body[i].getValue());
                i++;
            } else if (body[i].getType() == tok_declare) {
                if (body[i + 1].getType() != tok_identifier) {
                    std::cerr << "Error: '" << body[i + 1].getValue() << "' is not an identifier!" << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                vars.push_back(body[i + 1].getValue());
                std::string loadFrom = body[i + 1].getValue();
                lstart = fp.tellp();
                fp << "void* $" << loadFrom << ";";
                addSourceComment(fp, lstart, body[i].getValue());
                i++;
            } else if (body[i].getType() == tok_continue) {
                lstart = fp.tellp();
                fp << "continue;";
                addSourceComment(fp, lstart, body[i].getValue());
            } else if (body[i].getType() == tok_break) {
                lstart = fp.tellp();
                fp << "break;";
                addSourceComment(fp, lstart, body[i].getValue());
            } else if (body[i].getType() == tok_ref) {
                if (body[i + 1].getType() != tok_identifier) {
                    std::cerr << "Error: '" << body[i + 1].getValue() << "' is not an identifier!" << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                if (!hasVar(body[i + 1].getValue())) {
                    std::cerr << "Error: Use of undefined variable '" << body[i + 1].getValue() << "'" << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                lstart = fp.tellp();
                fp << "*((void**) $" << body[i + 1].getValue() << ") = scale_pop();";
                addSourceComment(fp, lstart, body[i].getValue());
                i++;
            } else if (body[i].getType() == tok_deref) {
                lstart = fp.tellp();
                fp << "scale_push(*(void**) scale_pop());";
                addSourceComment(fp, lstart, body[i].getValue());
            } else {
                std::cerr << "Error: Unknown token: '" << body[i].getValue() << "'" << std::endl;
                ParseResult result;
                result.success = false;
                return result;
            }
        }
        lstart = fp.tellp();
        if (!isInline) fp << "stacktrace_pop(1);";
        addSourceComment(fp, lstart, "end");
        fp << std::endl << "}" << std::endl;
    }

    fp     << "#ifdef __cplusplus" << std::endl;
    fp     << "}" << std::endl;
    fp     << "#endif" << std::endl;
    fp     << std::endl;

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
