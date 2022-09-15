#ifndef _PARSER_CPP_
#define _PARSER_CPP_

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <stdio.h>

#include "../Common.hpp"

#define append(...) fprintf(fp, __VA_ARGS__)

namespace sclc
{
    FPResult handleOperator(FILE* fp, Token token, int scopeDepth);
    FPResult handleNumber(FILE* fp, Token token, int scopeDepth);
    FPResult handleFor(Token keywDeclare, Token loopVar, Token keywIn, Token from, Token keywTo, Token to, Token keywDo, std::vector<std::string>* vars, FILE* fp, int* scopeDepth);
    FPResult handleDouble(FILE* fp, Token token, int scopeDepth);

    extern std::string scaleFolder;

    Function FunctionParser::getFunctionByName(std::string name) {
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

    FPResult FunctionParser::parse(std::string filename) {
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::vector<std::string> globals;

        remove((filename + std::string(".c")).c_str());
        FILE* fp = fopen((filename + std::string(".c")).c_str(), "a");

        if (getFunctionByName("main") == Function("%NULFUNC%")) {
            FPResult result;
            result.success = false;
            result.message = "No entry point found";
            result.where = 0;
            result.in = filename;
            result.column = 0;
            errors.push_back(result);
        }

        append("#ifdef __cplusplus\n");
        append("extern \"C\" {\n");
        append("#endif\n");

        append("\n");
        append("/* HEADERS */\n");
        for (std::string header : Main.frameworkNativeHeaders) {
            append("#include <%s>\n", header.c_str());
        }

        append("\n");
        append("const char* const __scl_internal__frameworks[] = {\n");
        for (std::string framework : Main.frameworks) {
            append("  \"%s\",\n", framework.c_str());
        }
        append("};\n");
        append("const size_t __scl_internal__frameworks_size = %zu;\n", Main.frameworks.size());

        append("\n");
        append("/* FUNCTION HEADERS */\n");

        for (Function function : result.functions) {
            append("void fn_%s(", function.getName().c_str());
            for (ssize_t i = (ssize_t) function.getArgs().size() - 1; i >= 0; i--) {
                std::string var = function.getArgs()[i];
                if (i != (ssize_t) function.getArgs().size() - 1) {
                    append(", ");
                }
                append("scl_word _%s", var.c_str());
            }
            if (function.getArgs().size() == 0) {
                append("void");
            }
            append(");\n");
        }
        
        append("\n");
        append("/* EXTERNS */\n");

        for (Extern extern_ : result.externs) {
            append("void native_%s(void);\n", extern_.name.c_str());
        }

        append("\n");
        append("const unsigned long long __scl_internal__function_names[] = {\n");
        for (Extern extern_ : result.externs) {
            append("  0x%016llxLLU /* %s */,\n", hash1((char*) extern_.name.c_str()), (char*) extern_.name.c_str());
        }
        for (Function function : result.functions) {
            append("  0x%016llxLLU /* %s */,\n", hash1((char*) function.getName().c_str()), (char*) function.getName().c_str());
        }
        append("};\n");

        append("const void* __scl_internal__function_ptrs[] = {\n");
        for (Extern extern_ : result.externs) {
            append("  native_%s,\n", extern_.name.c_str());
        }
        for (Function function : result.functions) {
            append("  fn_%s,\n", function.getName().c_str());
        }
        append("};\n");
        
        append("const char __scl_internal__function_args[] = {\n");
        for (Extern extern_ : result.externs) {
            append("  0,\n");
        }
        for (Function function : result.functions) {
            append("  %zu,\n", function.getArgs().size());
        }
        append("};\n");
        append("const size_t __scl_internal__function_ptrs_size = %zu;\n", result.functions.size() + result.externs.size());
        append("const size_t __scl_internal__function_names_size = %zu;\n", result.functions.size() + result.externs.size());
        append("const size_t __scl_internal__function_args_size = %zu;\n", result.functions.size() + result.externs.size());

        append("\n");
        append("/* GLOBALS */\n");

        for (std::string s : result.globals) {
            append("scl_word _%s;\n", s.c_str());
            vars.push_back(s);
            globals.push_back(s);
        }

        append("\n");
        append("/* FUNCTIONS */\n");

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

            append("void fn_%s(", function.getName().c_str());

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
                    append(", ");
                }
                append("scl_word _%s", var.c_str());
            }
            if (function.getArgs().size() == 0) {
                append("void");
            }
            append(") {\n");

            for (int j = 0; j < scopeDepth; j++) {
                append("  ");
            }
            if (funcPrivateStack) {
                append("ctrl_fn_start(\"%s\");\n", functionDeclaration.c_str());
            } else {
                append("ctrl_fn_nps_start(\"%s\");\n", functionDeclaration.c_str());
            }

            if (sap) {
                for (int j = 0; j < scopeDepth; j++) {
                    append("  ");
                }
                append("sap_open();\n");
            }

            std::vector<Token> body = function.getBody();
            std::vector<Token> sap_tokens;
            size_t i;
            ssize_t sap_depth = 0;
            for (i = 0; i < body.size(); i++)
            {
                if (body[i].getType() == tok_ignore) continue;

                for (int j = 0; j < scopeDepth; j++) {
                    append("  ");
                }
                std::string file = body[i].getFile();
                if (strncmp(file.c_str(), (scaleFolder + "/Frameworks/").c_str(), (scaleFolder + "/Frameworks/").size()) == 0) {
                    append("ctrl_where(\"(Scale Framework) %s\", %d, %d);\n", body[i].getFile().c_str() + scaleFolder.size() + 12, body[i].getLine(), body[i].getColumn());
                } else {
                    append("ctrl_where(\"%s\", %d, %d);\n", body[i].getFile().c_str(), body[i].getLine(), body[i].getColumn());
                }

                if (isOperator(body[i])) {
                    FPResult operatorsHandled = handleOperator(fp, body[i], scopeDepth);
                    if (!operatorsHandled.success) {
                        errors.push_back(operatorsHandled);
                    }
                } else if (body[i].getType() == tok_identifier && hasVar(body[i])) {
                    std::string loadFrom = body[i].getValue();
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("ctrl_push(_%s);\n", loadFrom.c_str());
                } else if (body[i].getType() == tok_identifier && hasFunction(body[i])) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("ctrl_required(%zu, ", getFunctionByName(body[i].getValue()).getArgs().size());
                    append("\"%s\");\n", body[i].getValue().c_str());
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("fn_%s(", body[i].getValue().c_str());
                    Function func = getFunctionByName(body[i].getValue());
                    for (ssize_t j = (ssize_t) func.getArgs().size() - 1; j >= 0; j--) {
                        if (j != (ssize_t) func.getArgs().size() - 1) {
                            append(", ");
                        }
                        append("ctrl_pop()");
                    }
                    append(");\n");
                } else if (body[i].getType() == tok_identifier && hasExtern(body[i])) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("ctrl_fn_native_start(\"%s\");\n", body[i].getValue().c_str());
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("native_%s();\n", body[i].getValue().c_str());
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("ctrl_fn_native_end();\n");
                } else if (body[i].getType() == tok_string_literal) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("ctrl_push_string(\"%s\");\n", body[i].getValue().c_str());
                } else if (body[i].getType() == tok_number) {
                    FPResult numberHandled = handleNumber(fp, body[i], scopeDepth);
                    if (!numberHandled.success) {
                        errors.push_back(numberHandled);
                    }
                } else if (body[i].getType() == tok_number_float) {
                    FPResult numberHandled = handleDouble(fp, body[i], scopeDepth);
                    if (!numberHandled.success) {
                        errors.push_back(numberHandled);
                    }
                } else if (body[i].getType() == tok_nil || body[i].getType() == tok_false) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("ctrl_push((scl_word) 0);\n");
                } else if (body[i].getType() == tok_true) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("ctrl_push((scl_word) 1);\n");
                } else if (body[i].getType() == tok_if) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    scopeDepth++;
                    append("if (ctrl_pop_long()) {\n");
                } else if (body[i].getType() == tok_else) {
                    scopeDepth--;
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    scopeDepth++;
                    append("} else {\n");
                } else if (body[i].getType() == tok_while) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    scopeDepth++;
                    append("while (1) {\n");
                } else if (body[i].getType() == tok_do) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("if (!ctrl_pop_long()) break;\n");
                } else if (body[i].getType() == tok_for) {
                    FPResult forHandled = handleFor(
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
                        append("  ");
                    }
                    append("}\n");
                } else if (body[i].getType() == tok_return) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("{\n");
                    scopeDepth++;
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("scl_word ret;\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("ssize_t stk_sz = ctrl_stack_size();\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("if (stk_sz > 0)  ret = ctrl_pop();\n");
                    for (ssize_t j = 0; j < sap_depth; j++) {
                        for (int k = 0; k < scopeDepth; k++) {
                            append("  ");
                        }
                        append("sap_close();\n");
                    }
                    if (sap) {
                        for (int j = 0; j < scopeDepth; j++) {
                            append("  ");
                        }
                        append("sap_close();\n");
                    }
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    if (funcPrivateStack) {
                        append("ctrl_fn_end();\n");
                    } else {
                        append("ctrl_fn_nps_end();\n");
                    }
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("if (stk_sz > 0) ctrl_push(ret);\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("return;\n");
                    scopeDepth--;
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("}\n");
                } else if (body[i].getType() == tok_addr_ref) {
                    Token toGet = body[i + 1];
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    if (hasExtern(toGet)) {
                        append("ctrl_push((scl_word) &native_%s);\n", toGet.getValue().c_str());
                    } else if (hasFunction(toGet)) {
                        append("ctrl_push((scl_word) &fn_%s);\n", toGet.getValue().c_str());
                    } else if (hasVar(toGet)) {
                        append("ctrl_push(_%s);\n", toGet.getValue().c_str());
                    } else {
                        FPResult err;
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
                        FPResult result;
                        result.message = "'" + body[i + 1].getValue() + "' is not an identifier!";
                        result.success = false;
                        result.where = body[i + 1].getLine();
                        result.in = body[i + 1].getFile();
                        result.token = body[i + 1].getValue();
                        result.column = body[i + 1].getColumn();
                        errors.push_back(result);
                    }
                    if (!hasVar(body[i + 1])) {
                        FPResult result;
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
                        append("  ");
                    }
                    append("_%s = ctrl_pop();\n", storeIn.c_str());
                    i++;
                } else if (body[i].getType() == tok_declare) {
                    if (body[i + 1].getType() != tok_identifier) {
                        FPResult result;
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
                        append("  ");
                    }
                    append("scl_word _%s;\n", loadFrom.c_str());
                    i++;
                } else if (body[i].getType() == tok_continue) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("continue;\n");
                } else if (body[i].getType() == tok_break) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("break;\n");
                } else if (body[i].getType() == tok_ref) {
                    if (body[i + 1].getType() != tok_identifier) {
                        FPResult result;
                        result.message = "'" + body[i + 1].getValue() + "' is not an identifier!";
                        result.success = false;
                        result.where = body[i + 1].getLine();
                        result.in = body[i + 1].getFile();
                        result.token = body[i + 1].getValue();
                        result.column = body[i + 1].getColumn();
                        errors.push_back(result);
                    }
                    if (!hasVar(body[i + 1])) {
                        FPResult result;
                        result.message = "Use of undefined variable '" + body[i + 1].getValue() + "'";
                        result.success = false;
                        result.where = body[i + 1].getLine();
                        result.in = body[i + 1].getFile();
                        result.token = body[i + 1].getValue();
                        result.column = body[i + 1].getColumn();
                        errors.push_back(result);
                    }
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("*((scl_word*) _%s) = ctrl_pop();\n", body[i + 1].getValue().c_str());
                    i++;
                } else if (body[i].getType() == tok_deref) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("{\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("scl_word addr = ctrl_pop();\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("scl_security_check_null(addr);\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("ctrl_push(*(scl_word*) addr);\n");
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("}\n");
                } else if (body[i].getType() == tok_sapopen) {
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("{\n");
                    scopeDepth++;
                    sap_depth++;
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("sap_open();\n");
                    sap_tokens.push_back(body[i]);
                } else if (body[i].getType() == tok_sapclose) {
                    if (sap_depth == 0) {
                        FPResult result;
                        result.message = "Trying to close unexistent SAP";
                        result.success = false;
                        result.where = body[i].getLine();
                        result.in = body[i].getFile();
                        result.token = body[i].getValue();
                        result.column = body[i].getColumn();
                        errors.push_back(result);
                    }
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("sap_close();\n");
                    sap_depth--;
                    scopeDepth--;
                    for (int j = 0; j < scopeDepth; j++) {
                        append("  ");
                    }
                    append("}\n");
                    if (sap_tokens.size() > 0) sap_tokens.pop_back();
                } else {
                    FPResult result;
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
                    append("  ctrl_fn_end();\n");
                } else if (body[body.size() - 1].getType() != tok_return) {
                    append("  ctrl_fn_end();\n");
                }
            } else {
                if (body.size() <= 0) {
                    append("  ctrl_fn_nps_end();\n");
                } else if (body[body.size() - 1].getType() != tok_return) {
                    append("  ctrl_fn_nps_end();\n");
                }
            }
            if (sap) {
                for (int j = 0; j < scopeDepth; j++) {
                    append("  ");
                }
                append("sap_close();\n");
            }
            append("}\n\n");

            if (sap_depth > 0) {
                FPResult result;
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

        append("%s\n", mainEntry.c_str());

        append("#ifdef __cplusplus\n");
        append("}\n");
        append("#endif\n");
        append("/* END OF GENERATED CODE */\n");

        fclose(fp);

        FPResult parseResult;
        parseResult.success = true;
        parseResult.message = "";
        parseResult.errors = errors;
        parseResult.warns = warns;
        return parseResult;
    }
}
#endif // _PARSER_CPP_
