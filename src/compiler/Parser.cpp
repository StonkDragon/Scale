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
#define LONG_MAX 0x7FFFFFFFFFFFFFFFll
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

Function Parser::getFunctionByName(std::string name) {
    for (Function func : result.functions) {
        if (func.name == name) {
            return func;
        }
    }
    return Function("______NULL______");
}

ParseResult Parser::parse(std::string filename) {
    std::fstream fp((filename + std::string(".c")).c_str(), std::ios::out);

    fp << "#ifdef __cplusplus" << std::endl;
    fp << "extern \"C\" {"     << std::endl;
    fp << "#endif"             << std::endl;
    fp << "#include <scale.h>" << std::endl;

    fp << std::endl;
    fp << "/* FUNCTION PROTOTYPES */" << std::endl;

    for (Prototype proto : result.prototypes) {
        fp << "void fun_" << proto.name << "();" << std::endl;
    }

    fp << std::endl;
    fp << "/* FUNCTION HEADERS */" << std::endl;

    for (Function function : result.functions) {
        fp << "void fun_" << function.getName() << "(";
        for (int i = function.getArgs().size() - 1; i >= 0; i--) {
            std::string var = function.getArgs()[i];
            var = var.substr(1);
            vars.push_back(var);
            if (i != function.getArgs().size() - 1) {
                fp << ", ";
            }
            fp << "scl_word $" << var;
        }
        fp << ");" << std::endl;
    }
    
    fp << std::endl;
    fp << "/* EXTERNS */" << std::endl;

    for (Extern extern_ : result.externs) {
        fp << "void native_" << extern_.name << "();" << std::endl;
    }

    fp << std::endl;
    fp << "/* FUNCTIONS */" << std::endl;

    for (Function function : result.functions)
    {
        vars.clear();

        int scopeDepth = 1;
        bool funcPrivateStack = true;
        bool noWarns = false;

        for (Modifier modifier : function.getModifiers()) {
            if (modifier == mod_nps) {
                funcPrivateStack = false;
            } else if (modifier == mod_nowarn) {
                noWarns = true;
            }
        }

        
        fp << "void fun_" << function.getName() << "(";

        std::string functionDeclaration = "";

        functionDeclaration += function.getName() + "(";
        for (int i = 0; i < function.getArgs().size(); i++) {
            if (i != 0) {
                functionDeclaration += ", ";
            }
            functionDeclaration += function.getArgs()[i].substr(1);
        }
        functionDeclaration += ")";
        
        for (int i = function.getArgs().size() - 1; i >= 0; i--) {
            std::string var = function.getArgs()[i];
            var = var.substr(1);
            vars.push_back(var);
            if (i != function.getArgs().size() - 1) {
                fp << ", ";
            }
            fp << "scl_word $" << var;
        }
        fp << ") {" << std::endl;

        for (int j = 0; j < scopeDepth; j++) {
            fp << "    ";
        }
        if (funcPrivateStack) {
            fp << "function_start(\"" << functionDeclaration << "\");" << std::endl;
        } else {
            fp << "function_nps_start(\"" << functionDeclaration << "\");" << std::endl;
        }

        std::vector<Token> body = function.getBody();

        for (int i = 0; i < body.size(); i++)
        {
            if (body[i].getType() == tok_ignore) continue;
            if (isOperator(body[i])) {
                bool operatorsHandled = handleOperator(fp, body[i], scopeDepth);
                if (!operatorsHandled) {
                    ParseResult result;
                    result.success = false;
                    return result;
                }
            } else if (body[i].getType() == tok_identifier && hasVar(body[i].getValue())) {
                std::string loadFrom = body[i].getValue();
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "push($" << loadFrom << ");" << std::endl;
            } else if (body[i].getType() == tok_identifier && hasFunction(body[i].getValue())) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "require_elements(" << getFunctionByName(body[i].getValue()).getArgs().size() << ", ";
                fp << "\"" << body[i].getValue() << "\");" << std::endl;
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "fun_" << body[i].getValue() << "(";
                Function func = getFunctionByName(body[i].getValue());
                for (int j = func.getArgs().size() - 1; j >= 0; j--) {
                    if (j != func.getArgs().size() - 1) {
                        fp << ", ";
                    }
                    fp << "pop()";
                }
                fp << ");" << std::endl;
            } else if (body[i].getType() == tok_identifier && hasExtern(body[i].getValue())) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "function_native_start(\"" << body[i].getValue() << "\");" << std::endl;
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "native_" << body[i].getValue() << "();" << std::endl;
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "function_native_end();" << std::endl;
            } else if (body[i].getType() == tok_string_literal) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "push_string(\"" << body[i].getValue() << "\");" << std::endl;
            } else if (body[i].getType() == tok_number) {
                bool numberHandled = handleNumber(fp, body[i], scopeDepth);
                if (!numberHandled) {
                    ParseResult result;
                    result.success = false;
                    return result;
                }
            } else if (body[i].getType() == tok_nil) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "push((scl_word) 0);" << std::endl;
            } else if (body[i].getType() == tok_if) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                scopeDepth++;
                fp << "if (pop_long()) {" << std::endl;
            } else if (body[i].getType() == tok_else) {
                scopeDepth--;
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                scopeDepth++;
                fp << "} else {" << std::endl;
            } else if (body[i].getType() == tok_while) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                scopeDepth++;
                fp << "while (1) {" << std::endl;
            } else if (body[i].getType() == tok_do) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "if (!pop_long()) break;" << std::endl;
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
            } else if (body[i].getType() == tok_done
                    || body[i].getType() == tok_fi
                    || body[i].getType() == tok_end) {
                scopeDepth--;
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "}" << std::endl;
            } else if (body[i].getType() == tok_return) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                if (funcPrivateStack) {
                    fp << "scl_word return_value = pop();" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "    ";
                    }
                    fp << "function_end();" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "    ";
                    }
                    fp << "push(return_value);" << std::endl;
                } else {
                    fp << "function_nps_end();" << std::endl;
                }
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "return;" << std::endl;
            } else if (body[i].getType() == tok_addr_ref) {
                std::string toGet = body[i + 1].getValue();
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                if (hasExtern(toGet)) {
                    fp << "push((scl_word) &native_" << toGet << ");";
                } else if (hasFunction(toGet)) {
                    fp << "push((scl_word) &fun_" << toGet << ");";
                } else {
                    fp << "push($" << toGet << ");" << std::endl;
                }
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
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "$" << storeIn << " = pop();" << std::endl;
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
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "scl_word $" << loadFrom << ";" << std::endl;
                i++;
            } else if (body[i].getType() == tok_continue) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "continue;" << std::endl;
            } else if (body[i].getType() == tok_break) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "break;" << std::endl;
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
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "*((scl_word*) $" << body[i + 1].getValue() << ") = pop();" << std::endl;
                i++;
            } else if (body[i].getType() == tok_deref) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "push(*(scl_word*) pop());" << std::endl;
            } else if (body[i].getType() == tok_open_paren) {
                if (body[i + 1].getType() != tok_identifier) {
                    std::cerr << "Error: '" << body[i + 1].getValue() << "' is not an identifier!" << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                if (body[i + 2].getType() != tok_close_paren) {
                    std::cerr << "Error: Typecast is not properly finished!" << std::endl;
                    ParseResult result;
                    result.success = false;
                    return result;
                }
                std::string type = body[i + 1].getValue();
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "push_string(\"" << type << "\");" << std::endl;
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "    ";
                }
                fp << "reinterpret_cast();" << std::endl;
                i += 2;
            } else {
                std::cerr << "Error: Unknown token: '" << body[i].getValue() << "'" << std::endl;
                ParseResult result;
                result.success = false;
                return result;
            }
        }
        if (funcPrivateStack) fp << "    function_end();" << std::endl;
        else                  fp << "    function_nps_end();" << std::endl;
        fp << "}" << std::endl << std::endl;
    }

    fp << "#ifdef __cplusplus" << std::endl;
    fp << "}" << std::endl;
    fp << "#endif" << std::endl;
    fp << "/* END OF GENERATED CODE */" << std::endl;
    
    fp.close();

    ParseResult parseResult;
    parseResult.success = true;
    return parseResult;
}

#endif // _PARSER_CPP_
