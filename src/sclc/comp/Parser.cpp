#ifndef _PARSER_CPP_
#define _PARSER_CPP_

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <stdio.h>

#include "../Common.hpp"

namespace sclc
{
    ParseResult handleOperator(FILE* fp, Token token, int scopeDepth);
    ParseResult handleNumber(FILE* fp, Token token, int scopeDepth);
    ParseResult handleFor(Token keywDeclare, Token loopVar, Token keywIn, Token from, Token keywTo, Token to, Token keywDo, std::vector<std::string>* vars, FILE* fp, int* scopeDepth);
    ParseResult handleDouble(FILE* fp, Token token, int scopeDepth);

    Function Parser::getFunctionByName(std::string name) {
        for (Function func : result.functions) {
            if (func.name == name) {
                return func;
            }
        }
        for (Prototype proto : result.prototypes) {
            if (proto.name == name) {
                Function func(name);
                func.args = std::vector<std::string>();
                for (int i = 0; i < proto.argCount; i++) {
                    func.args.push_back("$arg" + std::to_string(i));
                }
                return func;
            }
        }
        return Function("%NULFUNC%");
    }

    ParseResult Parser::parse(std::string filename) {
        std::vector<ParseResult> errors;
        std::vector<ParseResult> warns;
        std::vector<std::string> globals;

        FILE* fp = fopen((filename + std::string(".c")).c_str(), "a");

        if (getFunctionByName("main") == Function("%NULFUNC%")) {
            ParseResult result;
            result.success = false;
            result.message = "No entry point found";
            result.where = 0;
            result.in = filename;
            result.column = 0;
            errors.push_back(result);
        }

        fprintf(fp, "#ifdef __cplusplus\n");
        fprintf(fp, "extern \"C\" {\n");
        fprintf(fp, "#endif\n");

        fprintf(fp, "\n");
        fprintf(fp, "/* HEADERS */\n");
        for (std::string header : MAIN.frameworkNativeHeaders) {
            fprintf(fp, "#include <%s>\n", header.c_str());
        }

        fprintf(fp, "\n");
        fprintf(fp, "/* FUNCTION HEADERS */\n");

        for (Function function : result.functions) {
            fprintf(fp, "void fn_%s(", function.getName().c_str());
            for (ssize_t i = (ssize_t) function.getArgs().size() - 1; i >= 0; i--) {
                std::string var = function.getArgs()[i];
                if (i != (ssize_t) function.getArgs().size() - 1) {
                    fprintf(fp, ", ");
                }
                fprintf(fp, "scl_word _%s", var.c_str());
            }
            if (function.getArgs().size() == 0) {
                fprintf(fp, "void");
            }
            fprintf(fp, ");\n");
        }
        
        fprintf(fp, "\n");
        fprintf(fp, "/* EXTERNS */\n");

        for (Extern extern_ : result.externs) {
            fprintf(fp, "void native_%s(void);\n", extern_.name.c_str());
        }

        fprintf(fp, "\n");
        fprintf(fp, "/* GLOBALS */\n");

        for (std::string s : result.globals) {
            fprintf(fp, "scl_word _%s;\n", s.c_str());
            vars.push_back(s);
            globals.push_back(s);
        }

        fprintf(fp, "\n");
        fprintf(fp, "/* FUNCTIONS */\n");

        int funcsSize = result.functions.size();
        for (int f = 0; f < funcsSize; f++)
        {
            Function function = result.functions[f];
            vars.clear();
            for (std::string g : globals) {
                vars.push_back(g);
            }

            int scopeDepth = 1;
            bool funcPrivateStack = true;
            bool noWarns = false;
            bool sap = false;

            for (Modifier modifier : function.getModifiers()) {
                if (modifier == mod_nps) {
                    funcPrivateStack = false;
                } else if (modifier == mod_nowarn) {
                    noWarns = true;
                } else if (modifier == mod_sap) {
                    sap = true;
                }
            }

            (void) noWarns;

            fprintf(fp, "void fn_%s(", function.getName().c_str());

            std::string functionDeclaration = "";

            functionDeclaration += function.getName() + "(";
            for (size_t i = 0; i < function.getArgs().size(); i++) {
                if (i != 0) {
                    functionDeclaration += ", ";
                }
                functionDeclaration += function.getArgs()[i];
            }
            functionDeclaration += ")";
            
            for (ssize_t i = (ssize_t) function.getArgs().size() - 1; i >= 0; i--) {
                std::string var = function.getArgs()[i];
                vars.push_back(var);
                if (i != (ssize_t) function.getArgs().size() - 1) {
                    fprintf(fp, ", ");
                }
                fprintf(fp, "scl_word _%s", var.c_str());
            }
            if (function.getArgs().size() == 0) {
                fprintf(fp, "void");
            }
            fprintf(fp, ") {\n");

            for (int j = 0; j < scopeDepth; j++) {
                fprintf(fp, "  ");
            }
            if (funcPrivateStack) {
                fprintf(fp, "ctrl_fn_start(\"%s\");\n", functionDeclaration.c_str());
            } else {
                fprintf(fp, "ctrl_fn_nps_start(\"%s\");\n", functionDeclaration.c_str());
            }

            if (sap) {
                for (int j = 0; j < scopeDepth; j++) {
                    fprintf(fp, "  ");
                }
                fprintf(fp, "sap_open();\n");
            }

            std::vector<Token> body = function.getBody();
            std::vector<Token> sap_tokens;
            size_t i;
            ssize_t sap_depth = 0;
            for (i = 0; i < body.size(); i++)
            {
                if (body[i].getType() == tok_ignore) continue;

                for (int j = 0; j < scopeDepth; j++) {
                    fprintf(fp, "  ");
                }
                fprintf(fp, "ctrl_where(\"%s\", %d, %d);\n", body[i].getFile().c_str(), body[i].getLine(), body[i].getColumn());

                if (isOperator(body[i])) {
                    ParseResult operatorsHandled = handleOperator(fp, body[i], scopeDepth);
                    if (!operatorsHandled.success) {
                        errors.push_back(operatorsHandled);
                    }
                } else if (body[i].getType() == tok_identifier && hasVar(body[i])) {
                    std::string loadFrom = body[i].getValue();
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "ctrl_push(_%s);\n", loadFrom.c_str());
                } else if (body[i].getType() == tok_identifier && hasFunction(body[i])) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "ctrl_required(%zu, ", getFunctionByName(body[i].getValue()).getArgs().size());
                    fprintf(fp, "\"%s\");\n", body[i].getValue().c_str());
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "fn_%s(", body[i].getValue().c_str());
                    Function func = getFunctionByName(body[i].getValue());
                    for (ssize_t j = (ssize_t) func.getArgs().size() - 1; j >= 0; j--) {
                        if (j != (ssize_t) func.getArgs().size() - 1) {
                            fprintf(fp, ", ");
                        }
                        fprintf(fp, "ctrl_pop()");
                    }
                    fprintf(fp, ");\n");
                } else if (body[i].getType() == tok_identifier && hasExtern(body[i])) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "ctrl_fn_native_start(\"%s\");\n", body[i].getValue().c_str());
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "native_%s();\n", body[i].getValue().c_str());
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "ctrl_fn_native_end();\n");
                } else if (body[i].getType() == tok_string_literal) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "ctrl_push_string(\"%s\");\n", body[i].getValue().c_str());
                } else if (body[i].getType() == tok_number) {
                    ParseResult numberHandled = handleNumber(fp, body[i], scopeDepth);
                    if (!numberHandled.success) {
                        errors.push_back(numberHandled);
                    }
                } else if (body[i].getType() == tok_number_float) {
                    ParseResult numberHandled = handleDouble(fp, body[i], scopeDepth);
                    if (!numberHandled.success) {
                        errors.push_back(numberHandled);
                    }
                } else if (body[i].getType() == tok_nil || body[i].getType() == tok_false) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "ctrl_push((scl_word) 0);\n");
                } else if (body[i].getType() == tok_true) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "ctrl_push((scl_word) 1);\n");
                } else if (body[i].getType() == tok_if) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    scopeDepth++;
                    fprintf(fp, "if (ctrl_pop_long()) {\n");
                } else if (body[i].getType() == tok_else) {
                    scopeDepth--;
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    scopeDepth++;
                    fprintf(fp, "} else {\n");
                } else if (body[i].getType() == tok_while) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    scopeDepth++;
                    fprintf(fp, "while (1) {\n");
                } else if (body[i].getType() == tok_do) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "if (!ctrl_pop_long()) break;\n");
                } else if (body[i].getType() == tok_for) {
                    ParseResult forHandled = handleFor(
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
                    if (!forHandled.success) {
                        errors.push_back(forHandled);
                    }
                    i += 7;
                } else if (body[i].getType() == tok_done
                        || body[i].getType() == tok_fi
                        || body[i].getType() == tok_end) {
                    scopeDepth--;
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "}\n");
                } else if (body[i].getType() == tok_return) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "{\n");
                    scopeDepth++;
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "scl_word ret;\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "ssize_t stk_sz = ctrl_stack_size();\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "if (stk_sz > 0)  ret = ctrl_pop();\n");
                    for (ssize_t j = 0; j < sap_depth; j++) {
                        for (int k = 0; k < scopeDepth; k++) {
                            fprintf(fp, "  ");
                        }
                        fprintf(fp, "sap_close();\n");
                    }
                    if (sap) {
                        for (int j = 0; j < scopeDepth; j++) {
                            fprintf(fp, "  ");
                        }
                        fprintf(fp, "sap_close();\n");
                    }
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    if (funcPrivateStack) {
                        fprintf(fp, "ctrl_fn_end();\n");
                    } else {
                        fprintf(fp, "ctrl_fn_nps_end();\n");
                    }
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "if (stk_sz > 0) ctrl_push(ret);\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "return;\n");
                    scopeDepth--;
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "}\n");
                } else if (body[i].getType() == tok_addr_ref) {
                    Token toGet = body[i + 1];
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    if (hasExtern(toGet)) {
                        fprintf(fp, "ctrl_push((scl_word) &native_%s);\n", toGet.getValue().c_str());
                    } else if (hasFunction(toGet)) {
                        fprintf(fp, "ctrl_push((scl_word) &fn_%s);\n", toGet.getValue().c_str());
                    } else if (hasVar(toGet)) {
                        fprintf(fp, "ctrl_push(_%s);\n", toGet.getValue().c_str());
                    } else {
                        ParseResult err;
                        err.success = false;
                        err.message = "Unknown variable: '" + toGet.getValue() + "'";
                        err.column = body[i + 1].getColumn();
                        err.where = body[i + 1].getLine();
                        err.in = body[i + 1].getFile();
                        err.token = body[i + 1].getValue();
                        errors.push_back(err);
                    }
                    i++;
                } else if (body[i].getType() == tok_store) {
                    if (body[i + 1].getType() != tok_identifier) {
                        ParseResult result;
                        result.message = "'" + body[i + 1].getValue() + "' is not an identifier!";
                        result.success = false;
                        result.where = body[i + 1].getLine();
                        result.in = body[i + 1].getFile();
                        result.token = body[i + 1].getValue();
                        result.column = body[i + 1].getColumn();
                        errors.push_back(result);
                    }
                    if (!hasVar(body[i + 1])) {
                        ParseResult result;
                        result.message = "Use of undefined variable '" + body[i + 1].getValue() + "'";
                        result.success = false;
                        result.where = body[i + 1].getLine();
                        result.in = body[i + 1].getFile();
                        result.token = body[i + 1].getValue();
                        result.column = body[i + 1].getColumn();
                        errors.push_back(result);
                    }
                    std::string storeIn = body[i + 1].getValue();
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "_%s = ctrl_pop();\n", storeIn.c_str());
                    i++;
                } else if (body[i].getType() == tok_declare) {
                    if (body[i + 1].getType() != tok_identifier) {
                        ParseResult result;
                        result.message = "'" + body[i + 1].getValue() + "' is not an identifier!";
                        result.success = false;
                        result.where = body[i + 1].getLine();
                        result.in = body[i + 1].getFile();
                        result.token = body[i + 1].getValue();
                        result.column = body[i + 1].getColumn();
                        errors.push_back(result);
                    }
                    vars.push_back(body[i + 1].getValue());
                    std::string loadFrom = body[i + 1].getValue();
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "scl_word _%s;\n", loadFrom.c_str());
                    i++;
                } else if (body[i].getType() == tok_continue) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "continue;\n");
                } else if (body[i].getType() == tok_break) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "break;\n");
                } else if (body[i].getType() == tok_ref) {
                    if (body[i + 1].getType() != tok_identifier) {
                        ParseResult result;
                        result.message = "'" + body[i + 1].getValue() + "' is not an identifier!";
                        result.success = false;
                        result.where = body[i + 1].getLine();
                        result.in = body[i + 1].getFile();
                        result.token = body[i + 1].getValue();
                        result.column = body[i + 1].getColumn();
                        errors.push_back(result);
                    }
                    if (!hasVar(body[i + 1])) {
                        ParseResult result;
                        result.message = "Use of undefined variable '" + body[i + 1].getValue() + "'";
                        result.success = false;
                        result.where = body[i + 1].getLine();
                        result.in = body[i + 1].getFile();
                        result.token = body[i + 1].getValue();
                        result.column = body[i + 1].getColumn();
                        errors.push_back(result);
                    }
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "*((scl_word*) _%s) = ctrl_pop();\n", body[i + 1].getValue().c_str());
                    i++;
                } else if (body[i].getType() == tok_deref) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "{\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "scl_word addr = ctrl_pop();\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "scl_security_check_null(addr);\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "ctrl_push(*(scl_word*) addr);\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "}\n");
                } else if (body[i].getType() == tok_sapopen) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "{\n");
                    scopeDepth++;
                    sap_depth++;
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "sap_open();\n");
                    sap_tokens.push_back(body[i]);
                } else if (body[i].getType() == tok_sapclose) {
                    if (sap_depth == 0) {
                        ParseResult result;
                        result.message = "Trying to close unexistent SAP";
                        result.success = false;
                        result.where = body[i].getLine();
                        result.in = body[i].getFile();
                        result.token = body[i].getValue();
                        result.column = body[i].getColumn();
                        errors.push_back(result);
                    }
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "sap_close();\n");
                    sap_depth--;
                    scopeDepth--;
                    for (int j = 0; j < scopeDepth; j++) {
                        fprintf(fp, "  ");
                    }
                    fprintf(fp, "}\n");
                    if (sap_tokens.size() > 0) sap_tokens.pop_back();
                } else {
                    ParseResult result;
                    result.message = "Unknown identifier: '%s', body[i].getValue().c_str()";
                    result.success = false;
                    result.where = body[i].getLine();
                    result.in = body[i].getFile();
                    result.token = body[i].getValue();
                    result.column = body[i].getColumn();
                    errors.push_back(result);
                }
            }
            if (funcPrivateStack) {
                if (body.size() <= 0) {
                    fprintf(fp, "  ctrl_fn_end();\n");
                } else if (body[body.size() - 1].getType() != tok_return) {
                    fprintf(fp, "  ctrl_fn_end();\n");
                }
            } else {
                if (body.size() <= 0) {
                    fprintf(fp, "  ctrl_fn_nps_end();\n");
                } else if (body[body.size() - 1].getType() != tok_return) {
                    fprintf(fp, "  ctrl_fn_nps_end();\n");
                }
            }
            if (sap) {
                for (int j = 0; j < scopeDepth; j++) {
                    fprintf(fp, "  ");
                }
                fprintf(fp, "sap_close();\n");
            }
            fprintf(fp, "}\n\n");

            if (sap_depth > 0) {
                ParseResult result;
                result.message = "SAP never closed, Opened here:";
                result.success = false;
                result.where = sap_tokens[sap_tokens.size() - 1].getLine();
                result.in = sap_tokens[sap_tokens.size() - 1].getFile();
                result.token = sap_tokens[sap_tokens.size() - 1].getValue();
                result.column = sap_tokens[sap_tokens.size() - 1].getColumn();
                errors.push_back(result);
            }
        }

        std::string mainEntry = 
        "int main(int argc, char const *argv[]) {\n"
        "  signal(SIGINT, process_signal);\n"
        "  signal(SIGILL, process_signal);\n"
        "  signal(SIGABRT, process_signal);\n"
        "  signal(SIGFPE, process_signal);\n"
        "  signal(SIGSEGV, process_signal);\n"
        "  signal(SIGBUS, process_signal);\n"
        "#ifdef SIGTERM\n"
        "  signal(SIGTERM, process_signal);\n"
        "#endif\n"
        "\n"
        "  for (int i = argc - 1; i > 0; i--) {\n"
        "    ctrl_push_string(argv[i]);\n"
        "  }\n"
        "\n"
        "  fn_main();\n"
        "  return 0;\n"
        "}\n";

        fprintf(fp, "%s\n", mainEntry.c_str());

        fprintf(fp, "#ifdef __cplusplus\n");
        fprintf(fp, "}\n");
        fprintf(fp, "#endif\n");
        fprintf(fp, "/* END OF GENERATED CODE */\n");

        fclose(fp);

        ParseResult parseResult;
        parseResult.success = true;
        parseResult.message = "";
        parseResult.errors = errors;
        parseResult.warns = warns;
        return parseResult;
    }
}
#endif // _PARSER_CPP_
