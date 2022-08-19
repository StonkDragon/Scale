#ifndef _PARSER_CPP_
#define _PARSER_CPP_

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "../Common.hpp"

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

namespace sclc
{
    static std::vector<std::string> vars;

    bool hasVar(std::string name) {
        for (std::string var : vars) {
            if (var == name) {
                return true;
            }
        }
        return false;
    }
}

#include "TokenHandlers.cpp"

namespace sclc
{
    Function Parser::getFunctionByName(std::string name) {
        for (Function func : result.functions) {
            if (func.name == name) {
                return func;
            }
        }
        return Function("%NULFUNC%");
    }

    ParseResult Parser::parse(std::string filename) {
        std::vector<ParseResult> errors;
        std::vector<ParseResult> warns;
        std::vector<std::string> globals;

        std::fstream fp((filename + std::string(".c")).c_str(), std::ios::out);

        if (getFunctionByName("main") == Function("%NULFUNC%")) {
            ParseResult result;
            result.success = false;
            result.message = "No entry point found";
            result.where = 0;
            result.in = filename;
            result.column = 0;
            errors.push_back(result);
        }

        fp << "#ifdef __cplusplus" << std::endl;
        fp << "extern \"C\" {"	 << std::endl;
        fp << "#endif"	         << std::endl;
        fp << "#include <scale.h>" << std::endl;

        fp << std::endl;
        fp << "/* FUNCTION HEADERS */" << std::endl;

        for (Function function : result.functions) {
            fp << "void fn_" << function.getName() << "(";
            for (ssize_t i = (ssize_t) function.getArgs().size() - 1; i >= 0; i--) {
                std::string var = function.getArgs()[i];
                if (i != (ssize_t) function.getArgs().size() - 1) {
                    fp << ", ";
                }
                fp << "scl_word _" << var;
            }
            if (function.getArgs().size() == 0) {
                fp << "void";
            }
            fp << ");" << std::endl;
        }
        
        fp << std::endl;
        fp << "/* EXTERNS */" << std::endl;

        for (Extern extern_ : result.externs) {
            fp << "void native_" << extern_.name << "(void);" << std::endl;
        }

        fp << std::endl;
        fp << "/* GLOBALS */" << std::endl;

        for (std::string s : result.globals) {
            fp << "scl_word _" << s << ";" << std::endl;
            vars.push_back(s);
            globals.push_back(s);
        }

        fp << std::endl;
        fp << "/* FUNCTIONS */" << std::endl;

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

            fp << "void fn_" << function.getName() << "(";

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
                    fp << ", ";
                }
                fp << "scl_word _" << var;
            }
            if (function.getArgs().size() == 0) {
                fp << "void";
            }
            fp << ") {" << std::endl;

            for (int j = 0; j < scopeDepth; j++) {
                fp << "	";
            }
            if (funcPrivateStack) {
                fp << "ctrl_fn_start(\"" << functionDeclaration << "\");" << std::endl;
            } else {
                fp << "ctrl_fn_nps_start(\"" << functionDeclaration << "\");" << std::endl;
            }

            if (sap) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "	";
                }
                fp << "sap_open();" << std::endl;
            }

            std::vector<Token> body = function.getBody();
            size_t i;
            size_t sap_depth = 0;
            for (i = 0; i < body.size(); i++)
            {
                if (body[i].getType() == tok_ignore) continue;

                for (int j = 0; j < scopeDepth; j++) {
                    fp << "	";
                }
                fp << "ctrl_where(\"" + body[i].getFile() + "\", " + std::to_string(body[i].getLine()) + ", " + std::to_string(body[i].getColumn()) + ");" << std::endl;

                if (isOperator(body[i])) {
                    ParseResult operatorsHandled = handleOperator(fp, body[i], scopeDepth);
                    if (!operatorsHandled.success) {
                        errors.push_back(operatorsHandled);
                    }
                } else if (body[i].getType() == tok_identifier && hasVar(body[i].getValue())) {
                    std::string loadFrom = body[i].getValue();
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "ctrl_push(_" << loadFrom << ");" << std::endl;
                } else if (body[i].getType() == tok_identifier && hasFunction(body[i].getValue())) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "ctrl_required(" << getFunctionByName(body[i].getValue()).getArgs().size() << ", ";
                    fp << "\"" << body[i].getValue() << "\");" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "fn_" << body[i].getValue() << "(";
                    Function func = getFunctionByName(body[i].getValue());
                    for (ssize_t j = (ssize_t) func.getArgs().size() - 1; j >= 0; j--) {
                        if (j != (ssize_t) func.getArgs().size() - 1) {
                            fp << ", ";
                        }
                        fp << "ctrl_pop()";
                    }
                    fp << ");" << std::endl;
                } else if (body[i].getType() == tok_identifier && hasExtern(body[i].getValue())) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "ctrl_fn_native_start(\"" << body[i].getValue() << "\");" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "native_" << body[i].getValue() << "();" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "ctrl_fn_native_end();" << std::endl;
                } else if (body[i].getType() == tok_string_literal) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "ctrl_push_string(\"" << body[i].getValue() << "\");" << std::endl;
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
                        fp << "	";
                    }
                    fp << "ctrl_push((scl_word) 0);" << std::endl;
                } else if (body[i].getType() == tok_true) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "ctrl_push((scl_word) 1);" << std::endl;
                } else if (body[i].getType() == tok_if) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    scopeDepth++;
                    fp << "if (ctrl_pop_long()) {" << std::endl;
                } else if (body[i].getType() == tok_else) {
                    scopeDepth--;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    scopeDepth++;
                    fp << "} else {" << std::endl;
                } else if (body[i].getType() == tok_while) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    scopeDepth++;
                    fp << "while (1) {" << std::endl;
                } else if (body[i].getType() == tok_do) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "if (!ctrl_pop_long()) break;" << std::endl;
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
                        fp << "	";
                    }
                    fp << "}" << std::endl;
                } else if (body[i].getType() == tok_return) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "{" << std::endl;
                    scopeDepth++;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "scl_word ret;" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "ssize_t stk_sz = ctrl_stack_size();" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "if (stk_sz > 0)  ret = ctrl_pop();" << std::endl;
                    for (size_t j = 0; j < sap_depth; j++) {
                        for (int k = 0; k < scopeDepth; k++) {
                            fp << "	";
                        }
                        fp << "sap_close();" << std::endl;
                    }
                    if (sap) {
                        for (int j = 0; j < scopeDepth; j++) {
                            fp << "	";
                        }
                        fp << "sap_close();" << std::endl;
                    }
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    if (funcPrivateStack) {
                        fp << "ctrl_fn_end();" << std::endl;
                    } else {
                        fp << "ctrl_fn_nps_end();" << std::endl;
                    }
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "if (stk_sz > 0) ctrl_push(ret);" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "return;" << std::endl;
                    scopeDepth--;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "}" << std::endl;
                } else if (body[i].getType() == tok_addr_ref) {
                    std::string toGet = body[i + 1].getValue();
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    if (hasExtern(toGet)) {
                        fp << "ctrl_push((scl_word) &native_" << toGet << ");" << std::endl;
                    } else if (hasFunction(toGet)) {
                        fp << "ctrl_push((scl_word) &fn_" << toGet << ");" << std::endl;
                    } else if (hasVar(toGet)) {
                        fp << "ctrl_push(_" << toGet << ");" << std::endl;
                    } else {
                        ParseResult err;
                        err.success = false;
                        err.message = "Unknown variable: '" + toGet + "'";
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
                    if (!hasVar(body[i + 1].getValue())) {
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
                        fp << "	";
                    }
                    fp << "_" << storeIn << " = ctrl_pop();" << std::endl;
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
                        fp << "	";
                    }
                    fp << "scl_word _" << loadFrom << ";" << std::endl;
                    i++;
                } else if (body[i].getType() == tok_continue) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "continue;" << std::endl;
                } else if (body[i].getType() == tok_break) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "break;" << std::endl;
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
                    if (!hasVar(body[i + 1].getValue())) {
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
                        fp << "	";
                    }
                    fp << "*((scl_word*) _" << body[i + 1].getValue() << ") = ctrl_pop();" << std::endl;
                    i++;
                } else if (body[i].getType() == tok_deref) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "{" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "scl_word addr = ctrl_pop();" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "scl_security_check_null(addr);" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "ctrl_push(*(scl_word*) addr);" << std::endl;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "}" << std::endl;
                } else if (body[i].getType() == tok_sapopen) {
                    scopeDepth++;
                    sap_depth++;
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "sap_open();" << std::endl;
                } else if (body[i].getType() == tok_sapclose) {
                    for (int j = 0; j < scopeDepth; j++) {
                        fp << "	";
                    }
                    fp << "sap_close();" << std::endl;
                    sap_depth--;
                    scopeDepth--;
                } else {
                    ParseResult result;
                    result.message = "Unknown identifier: '" + body[i].getValue() + "'";
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
                    fp << "	ctrl_fn_end();" << std::endl;
                } else if (body[body.size() - 1].getType() != tok_return) {
                    fp << "	ctrl_fn_end();" << std::endl;
                }
            } else {
                if (body.size() <= 0) {
                    fp << "	ctrl_fn_nps_end();" << std::endl;
                } else if (body[body.size() - 1].getType() != tok_return) {
                    fp << "	ctrl_fn_nps_end();" << std::endl;
                }
            }
            if (sap) {
                for (int j = 0; j < scopeDepth; j++) {
                    fp << "	";
                }
                fp << "sap_close();" << std::endl;
            }
            fp << "}" << std::endl << std::endl;
        }

        std::string mainEntry = 
        "int main(int argc, char const *argv[])\n"
        "{\n"
	    "	signal(SIGINT, process_signal);\n"
	    "	signal(SIGILL, process_signal);\n"
	    "	signal(SIGABRT, process_signal);\n"
	    "	signal(SIGFPE, process_signal);\n"
	    "	signal(SIGSEGV, process_signal);\n"
	    "	signal(SIGBUS, process_signal);\n"
        "#ifdef SIGTERM\n"
	    "	signal(SIGTERM, process_signal);\n"
        "#endif\n"
        "\n"
	    "	for (int i = argc - 1; i > 0; i--) {\n"
		"		ctrl_push_string(argv[i]);\n"
	    "	}\n"
        "\n"
	    "	fn_main();\n"
	    "	return 0;\n"
        "}\n";

		fp << mainEntry << std::endl;

        fp << "#ifdef __cplusplus" << std::endl;
        fp << "}" << std::endl;
        fp << "#endif" << std::endl;
        fp << "/* END OF GENERATED CODE */" << std::endl;
        
        fp.close();

        ParseResult parseResult;
        parseResult.success = true;
        parseResult.message = "";
        parseResult.errors = errors;
        parseResult.warns = warns;
        return parseResult;
    }
}
#endif // _PARSER_CPP_
