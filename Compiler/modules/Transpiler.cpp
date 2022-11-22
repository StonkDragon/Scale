#ifndef _TRANSPILER_CPP
#define _TRANSPILER_CPP

#include <filesystem>

#include "../headers/Common.hpp"
#include "../headers/TranspilerDefs.hpp"

#define ITER_INC do { i++; if (i >= body.size()) { FPResult err; err.success = false; err.message = "Unexpected end of function!"; err.column = body[i - 1].getColumn(); err.line = body[i - 1].getLine(); err.in = body[i - 1].getFile(); err.value = body[i - 1].getValue(); err.type =  body[i - 1].getType(); errors.push_back(err); return; } } while (0)

namespace sclc {
    FILE* nomangled;

    void ConvertC::writeHeader(FILE* fp) {
        int scopeDepth = 0;
        append("#ifdef __cplusplus\n");
        append("extern \"C\" {\n");
        append("#endif\n");

        append("\n");
        append("/* HEADERS */\n");
        append("#include <scale_internal.h>\n");
        for (std::string header : Main.frameworkNativeHeaders) {
            append("#include <%s>\n", header.c_str());
        }

        append("\n");
        append("const char* const scl_internal_frameworks[] = {\n");
        for (std::string framework : Main.frameworks) {
            append("  \"%s\",\n", framework.c_str());
        }
        append("};\n");
        append("const size_t scl_internal_frameworks_size = %zu;\n", Main.frameworks.size());
        append("\n");
    }

    std::string sclReturnTypeToCReturnType(TPResult result, std::string t) {
        std::string return_type = "scl_value";
        if (t == "any") return_type = "scl_value";
        else if (t == "none") return_type = "void";
        else if (t == "int") return_type = "scl_int";
        else if (t == "float") return_type = "scl_float";
        else if (t == "str") return_type = "scl_str";
        else if (!(getStructByName(result, t) == Struct(""))) {
            return_type = "struct Struct_" + getStructByName(result, t).getName() + "*";
        }
        return return_type;
    }

    void ConvertC::writeFunctionHeaders(FILE* fp, TPResult result) {
        int scopeDepth = 0;
        append("/* FUNCTION HEADERS */\n");

        if (Main.options.transpileOnly) {
            remove("scale_support.h");
            nomangled = fopen("scale_support.h", "a");
            fprintf(nomangled, "#ifndef SCALE_SUPPORT_H\n");
            fprintf(nomangled, "#define SCALE_SUPPORT_H\n\n");
            fprintf(nomangled, "#include <scale_internal.h>\n\n");

            fprintf(nomangled, "#define scl_export(func_name) \\\n");
            fprintf(nomangled, "    void func_name (void); \\\n");
            fprintf(nomangled, "    void Function_ ## func_name (void); \\\n");
            fprintf(nomangled, "    void Function_ ## func_name () { func_name (); } \\\n");
            fprintf(nomangled, "    void func_name (void)\n\n");
            fprintf(nomangled, "#define ssize_t signed long\n");
            fprintf(nomangled, "typedef void* scl_value;\n");
            fprintf(nomangled, "typedef long long scl_int;\n");
            fprintf(nomangled, "typedef char* scl_str;\n");
            fprintf(nomangled, "typedef double scl_float;\n\n");
            fprintf(nomangled, "extern scl_stack_t stack;\n\n");
        }

        for (Function* function : result.functions) {
            std::string return_type = "void";

            if (function->getReturnType().size() > 0) {
                std::string t = function->getReturnType();
                return_type = sclReturnTypeToCReturnType(result, t);
            }

            if (!function->isMethod) {
                append("%s Function_%s(void);\n", return_type.c_str(), function->getName().c_str());
            } else {
                append("%s Method_%s_%s(void);\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str());
            }
            if (!Main.options.transpileOnly) continue;
            for (Modifier m : function->getModifiers()) {
                if (m == mod_nomangle) {
                    std::string args = "(";
                    for (size_t i = 0; i < function->getArgs().size(); i++) {
                        if (i != 0) {
                            args += ", ";
                        }
                        args += sclReturnTypeToCReturnType(result, function->getArgs()[i].getType()) + " ";
                        args += function->getArgs()[i].getName();
                    }
                    args += ")";
                    
                    fprintf(nomangled, "%s %s%s;\n", return_type.c_str(), function->getName().c_str(), args.c_str());
                }
            }
        }

        append("\n");
    }

    void ConvertC::writeExternHeaders(FILE* fp, TPResult result) {
        int scopeDepth = 0;
        append("/* EXTERN FUNCTIONS */\n");

        for (Function* function : result.extern_functions) {
            bool hasFunction = false;
            for (Function* f : result.functions) {
                if (f->getName() == function->getName()) {
                    hasFunction = true;
                }
            }
            if (!hasFunction) {
                std::string return_type = "void";

                if (function->getReturnType().size() > 0) {
                    std::string t = function->getReturnType();
                    return_type = sclReturnTypeToCReturnType(result, t);
                }

                if (!function->isMethod) {
                    append("%s Function_%s(void);\n", return_type.c_str(), function->getName().c_str());
                } else {
                    append("%s Method_%s_%s(void);\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str());
                }
            }
        }

        append("\n");
    }

    void ConvertC::writeInternalFunctions(FILE* fp, TPResult result) {
        int scopeDepth = 0;
        append("const unsigned long long scl_internal_function_names_with_args[] = {\n");
        for (Function* function : result.extern_functions) {
            std::string functionDeclaration = "";

            if (function->isMethod) {
                functionDeclaration += static_cast<Method*>(function)->getMemberType() + ":" + function->getName() + "(";
                for (size_t i = 0; i < function->getArgs().size() - 1; i++) {
                    if (i != 0) {
                        functionDeclaration += ", ";
                    }
                    functionDeclaration += function->getArgs()[i].getName() + ": " + function->getArgs()[i].getType();
                }
                functionDeclaration += "): " + function->getReturnType();
            } else {
                functionDeclaration += function->getName() + "(";
                for (size_t i = 0; i < function->getArgs().size(); i++) {
                    if (i != 0) {
                        functionDeclaration += ", ";
                    }
                    functionDeclaration += function->getArgs()[i].getName() + ": " + function->getArgs()[i].getType();
                }
                functionDeclaration += "): " + function->getReturnType();
            }
            
            append("  0x%016llxLLU /* %s */,\n", hash1((char*) functionDeclaration.c_str()), (char*) functionDeclaration.c_str());
        }
        for (Function* function : result.functions) {
            std::string functionDeclaration = "";

            if (function->isMethod) {
                functionDeclaration += static_cast<Method*>(function)->getMemberType() + ":" + function->getName() + "(";
                for (size_t i = 0; i < function->getArgs().size() - 1; i++) {
                    if (i != 0) {
                        functionDeclaration += ", ";
                    }
                    functionDeclaration += function->getArgs()[i].getName() + ": " + function->getArgs()[i].getType();
                }
                functionDeclaration += "): " + function->getReturnType();
            } else {
                functionDeclaration += function->getName() + "(";
                for (size_t i = 0; i < function->getArgs().size(); i++) {
                    if (i != 0) {
                        functionDeclaration += ", ";
                    }
                    functionDeclaration += function->getArgs()[i].getName() + ": " + function->getArgs()[i].getType();
                }
                functionDeclaration += "): " + function->getReturnType();
            }
            append("  0x%016llxLLU /* %s */,\n", hash1((char*) functionDeclaration.c_str()), (char*) functionDeclaration.c_str());
        }
        append("};\n");
        append("const unsigned long long scl_internal_function_names[] = {\n");
        for (Function* function : result.extern_functions) {
            std::string functionDeclaration = function->getName();
            if (function->isMethod) {
                functionDeclaration = static_cast<Method*>(function)->getMemberType() + ":" + functionDeclaration;
            }
            append("  0x%016llxLLU /* %s */,\n", hash1((char*) functionDeclaration.c_str()), (char*) functionDeclaration.c_str());
        }
        for (Function* function : result.functions) {
            std::string functionDeclaration = function->getName();
            if (function->isMethod) {
                functionDeclaration = static_cast<Method*>(function)->getMemberType() + ":" + functionDeclaration;
            }
            append("  0x%016llxLLU /* %s */,\n", hash1((char*) functionDeclaration.c_str()), (char*) functionDeclaration.c_str());
        }
        append("};\n");

        append("const scl_method scl_internal_function_ptrs[] = {\n");
        for (Function* function : result.extern_functions) {
            std::string return_type = "void";

            if (function->getReturnType().size() > 0) {
                std::string t = function->getReturnType();
                return_type = sclReturnTypeToCReturnType(result, t);
            }
            if (!function->isMethod) {
                append("  (scl_method) Function_%s,\n", function->getName().c_str());
            } else {
                append("  (scl_method) Method_%s_%s,\n", ((Method*)(function))->getMemberType().c_str(), function->getName().c_str());
            }
        }
        for (Function* function : result.functions) {
            std::string return_type = "void";

            if (function->getReturnType().size() > 0) {
                std::string t = function->getReturnType();
                return_type = sclReturnTypeToCReturnType(result, t);
            }
            if (!function->isMethod) {
                append("  (scl_method) Function_%s,\n", function->getName().c_str());
            } else {
                append("  (scl_method) Method_%s_%s,\n", ((Method*)(function))->getMemberType().c_str(), function->getName().c_str());
            }
        }
        append("};\n");
        append("const scl_str scl_internal_function_returns[] = {\n");
        for (Function* function : result.extern_functions) {
            append("  \"%s\",\n", function->getReturnType().c_str());
        }
        for (Function* function : result.functions) {
            append("  \"%s\",\n", function->getReturnType().c_str());
        }
        append("};\n");
        append("const size_t scl_internal_functions_size = %zu;\n", result.functions.size() + result.extern_functions.size());

        append("\n");
    }

    void ConvertC::writeGlobals(FILE* fp, std::vector<Variable>& globals, TPResult result) {
        int scopeDepth = 0;
        append("/* GLOBALS */\n");

        for (Variable s : result.globals) {
            append("%s Var_%s;\n", sclReturnTypeToCReturnType(result, s.getType()).c_str(), s.getName().c_str());
            if (Main.options.transpileOnly) fprintf(nomangled, "extern %s Var_%s;\n", sclReturnTypeToCReturnType(result, s.getType()).c_str(), s.getName().c_str());
            vars.push_back(s);
            globals.push_back(s);
        }

        append("\n");
    }

    void ConvertC::writeContainers(FILE* fp, TPResult result) {
        int scopeDepth = 0;
        append("/* CONTAINERS */\n");
        for (Container c : result.containers) {
            append("struct {\n");
            for (Variable s : c.getMembers()) {
                append("  %s %s;\n", sclReturnTypeToCReturnType(result, s.getType()).c_str(), s.getName().c_str());
            }
            append("} Container_%s = {0};\n", c.getName().c_str());
            if (Main.options.transpileOnly) {
                fprintf(nomangled, "struct Container_%s {\n", c.getName().c_str());
                for (Variable s : c.getMembers()) {
                    fprintf(nomangled, "  %s %s;\n", sclReturnTypeToCReturnType(result, s.getType()).c_str(), s.getName().c_str());
                }
                fprintf(nomangled, "};\n");
                fprintf(nomangled, "extern struct Container_%s Container_%s;\n", c.getName().c_str(), c.getName().c_str());
            }
        }

        append("\n");
        append("const scl_value* scl_internal_globals_ptrs[] = {\n");
        for (Variable s : result.globals) {
            append("  (const scl_value*) &Var_%s,\n", s.getName().c_str());
        }
        size_t container_member_count = 0;
        for (Container c : result.containers) {
            for (Variable s : c.getMembers()) {
                append("  (const scl_value*) &Container_%s.%s,\n", c.getName().c_str(), s.getName().c_str());
                container_member_count++;
            }
        }
        append("};\n");
        append("const size_t scl_internal_globals_ptrs_size = %zu;\n", result.globals.size() + container_member_count);
        
        append("\n");
    }

    void ConvertC::writeStructs(FILE* fp, TPResult result) {
        int scopeDepth = 0;
        append("/* STRUCTS */\n");
        for (Struct c : result.structs) {
            append("struct Struct_%s {\n", c.getName().c_str());
            append("  scl_int $__type__;\n");
            append("  scl_str $__type_name__;\n");
            for (Variable s : c.getMembers()) {
                append("  %s %s;\n", sclReturnTypeToCReturnType(result, s.getType()).c_str(), s.getName().c_str());
            }
            append("};\n");
            if (Main.options.transpileOnly) {
                fprintf(nomangled, "struct %s {\n", c.getName().c_str());
                fprintf(nomangled, "  scl_int __type_identifier__;\n");
                fprintf(nomangled, "  scl_str __type_string__;\n");
                for (Variable s : c.getMembers()) {
                    fprintf(nomangled, "  %s %s;\n", sclReturnTypeToCReturnType(result, s.getType()).c_str(), s.getName().c_str());
                }
                fprintf(nomangled, "};\n");
            }
        }
        append("\n");

        if (Main.options.transpileOnly) {
            fprintf(nomangled, "#endif\n");
            fclose(nomangled);
        }
    }

    void ConvertC::writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<Variable>& globals, TPResult result) {
        (void) warns;
        int scopeDepth = 0;
        append("/* EXTERN VARS FROM INTERNAL */\n");
        append("extern scl_str current_file;\n");
        append("extern size_t current_line;\n");
        append("extern size_t current_column;\n");
        append("extern scl_stack_t stack;\n");
        append("extern scl_stack_t callstk;\n\n");

        append("/* FUNCTIONS */\n");

        int funcsSize = result.functions.size();
        for (int f = 0; f < funcsSize; f++)
        {
            Function* function = result.functions[f];
            vars.clear();
            for (Variable g : globals) {
                vars.push_back(g);
            }

            scopeDepth = 0;
            bool noWarns = false;
            bool no_mangle = false;
            bool nps = false;
            ssize_t sap_depth = 0;

            for (Modifier modifier : function->getModifiers()) {
                if (modifier == mod_nowarn) {
                    noWarns = true;
                } else if (modifier == mod_nomangle) {
                    no_mangle = true;
                } else if (modifier == mod_nps) {
                    nps = true;
                }
            }

            (void) noWarns;
            (void) nps;

            std::string functionDeclaration = "";

            if (function->isMethod) {
                functionDeclaration += static_cast<Method*>(function)->getMemberType() + ":" + function->getName() + "(";
                for (size_t i = 0; i < function->getArgs().size() - 1; i++) {
                    if (i != 0) {
                        functionDeclaration += ", ";
                    }
                    functionDeclaration += function->getArgs()[i].getName() + ": " + function->getArgs()[i].getType();
                }
                functionDeclaration += "): " + function->getReturnType();
            } else {
                functionDeclaration += function->getName() + "(";
                for (size_t i = 0; i < function->getArgs().size(); i++) {
                    if (i != 0) {
                        functionDeclaration += ", ";
                    }
                    functionDeclaration += function->getArgs()[i].getName() + ": " + function->getArgs()[i].getType();
                }
                functionDeclaration += "): " + function->getReturnType();
            }

            std::string return_type = "void";

            if (function->getReturnType().size() > 0) {
                std::string t = function->getReturnType();
                return_type = sclReturnTypeToCReturnType(result, t);
            } else {
                FPResult err;
                err.success = false;
                err.message = "Function '" + function->getName() + "' does not specify a return type, defaults to none.";
                err.column = function->getBody()[0].getColumn();
                err.line = function->getBody()[0].getLine();
                err.in = function->getBody()[0].getFile();
                err.value = function->getBody()[0].getValue();
                err.type = function->getBody()[0].getType();
                if (!Main.options.Werror) warns.push_back(err);
                else errors.push_back(err);
            }

            if (no_mangle) {
                std::string args = "(";
                for (size_t i = 0; i < function->getArgs().size(); i++) {
                    if (i != 0) {
                        args += ", ";
                    }
                    args += sclReturnTypeToCReturnType(result, function->getArgs()[i].getType()) + " Var_";
                    args += function->getArgs()[i].getName();
                }
                args += ")";

                append("%s %s%s {\n", return_type.c_str(), function->getName().c_str(), args.c_str());
                scopeDepth++;
                for (size_t i = 0; i < function->getArgs().size(); i++) {
                    if (function->getArgs()[i].getType() == "float") {
                        append("stack.data[stack.ptr++].f = Var_%s;\n", function->getArgs()[i].getName().c_str());
                    } else {
                        append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", function->getArgs()[i].getName().c_str());
                    }
                }
                if (return_type != "void") {
                    if (!function->isMethod) {
                        append("return Function_%s();\n", function->getName().c_str());
                    } else {
                        append("return Method_%s_%s();\n", ((Method*)(function))->getMemberType().c_str(), function->getName().c_str());
                    }
                } else {
                    if (!function->isMethod) {
                        append("Function_%s();\n", function->getName().c_str());
                    } else {
                        append("Method_%s_%s();\n", ((Method*)(function))->getMemberType().c_str(), function->getName().c_str());
                    }
                }
                scopeDepth--;
                append("}\n");
            }

            return_type = "void";

            if (function->getReturnType().size() > 0) {
                std::string t = function->getReturnType();
                return_type = sclReturnTypeToCReturnType(result, t);
            }
            if (!function->isMethod) {
                append("%s Function_%s(void) {\n", return_type.c_str(), function->getName().c_str());
            } else {
                append("%s Method_%s_%s(void) {\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str());
            }
            
            scopeDepth++;

            append("callstk.data[callstk.ptr++].v = \"%s\";\n", functionDeclaration.c_str());

            for (ssize_t i = (ssize_t) function->getArgs().size() - 1; i >= 0; i--) {
                Variable var = function->getArgs()[i];
                vars.push_back(var);
                std::string ret_type = sclReturnTypeToCReturnType(result, var.getType());
                if (function->getArgs()[i].getType() == "float") {
                    append("%s Var_%s = stack.data[--stack.ptr].f;\n", ret_type.c_str(), var.getName().c_str());
                } else {
                    append("%s Var_%s = (%s) stack.data[--stack.ptr].v;\n", ret_type.c_str(), var.getName().c_str(), ret_type.c_str());
                }
            }

            if (!Main.options.transpileOnly || Main.options.debugBuild) {
                std::string file = function->getFile();
                if (strncmp(function->getFile().c_str(), (scaleFolder + "/Frameworks/").c_str(), (scaleFolder + "/Frameworks/").size()) == 0) {
                    append("current_file = \"%s\";\n", function->getFile().c_str() + scaleFolder.size() + 12);
                } else {
                    append("current_file = \"%s\";\n", function->getFile().c_str());
                }
            }

            std::vector<Token> body = function->getBody();
            std::vector<Token> sap_tokens;
            std::vector<bool> was_rep;
            size_t i;
            char repeat_depth = 0;
            int current_line = -1;
            for (i = 0; i < body.size(); i++) {
                if (body[i].getType() == tok_ignore) continue;

                std::string file = body[i].getFile();
                if (!Main.options.transpileOnly || Main.options.debugBuild) {
                    if (
                        body[i].getType() != tok_declare && body[i].getType() != tok_while &&
                        body[i].getType() != tok_end && body[i].getType() != tok_fi &&
                        body[i].getType() != tok_done
                    ) {
                        if (body[i].getLine() != current_line) {
                            append("current_line = %d;\n", body[i].getLine());
                            current_line = body[i].getLine();
                        }
                        if (!(Main.options.optimizer == "O3" || Main.options.optimizer == "Ofast"))
	                        append("current_column = %d;\n", body[i].getColumn());
                    }
                }

                if (isOperator(body[i])) {
                    FPResult operatorsHandled = handleOperator(fp, body[i], scopeDepth);
                    if (!operatorsHandled.success) {
                        errors.push_back(operatorsHandled);
                    }
                } else {
                    switch (body[i].getType()) {
                        case tok_identifier: {
                            if (body[i].getValue() == "self" && !function->isMethod) {
                                transpilerError("Can't use 'self' outside a member function.", i);
                                errors.push_back(err);
                                continue;
                            }
                            if (body[i].getValue() == "drop") {
                                append("stack.ptr--;\n");
                            } else if (body[i].getValue() == "dup") {
                                append("stack.data[stack.ptr++].v = stack.data[stack.ptr - 1].v;\n");
                            } else if (body[i].getValue() == "swap") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_value b = stack.data[--stack.ptr].v;\n");
                                append("scl_value a = stack.data[--stack.ptr].v;\n");
                                append("stack.data[stack.ptr++].v = b;\n");
                                append("stack.data[stack.ptr++].v = a;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "over") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_value c = stack.data[--stack.ptr].v;\n");
                                append("scl_value b = stack.data[--stack.ptr].v;\n");
                                append("scl_value a = stack.data[--stack.ptr].v;\n");
                                append("stack.data[stack.ptr++].v = c;\n");
                                append("stack.data[stack.ptr++].v = b;\n");
                                append("stack.data[stack.ptr++].v = a;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "sdup2") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_value b = stack.data[--stack.ptr].v;\n");
                                append("scl_value a = stack.data[--stack.ptr].v;\n");
                                append("stack.data[stack.ptr++].v = a;\n");
                                append("stack.data[stack.ptr++].v = b;\n");
                                append("stack.data[stack.ptr++].v = a;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "swap2") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_value c = stack.data[--stack.ptr].v;\n");
                                append("scl_value b = stack.data[--stack.ptr].v;\n");
                                append("scl_value a = stack.data[--stack.ptr].v;\n");
                                append("stack.data[stack.ptr++].v = b;\n");
                                append("stack.data[stack.ptr++].v = a;\n");
                                append("stack.data[stack.ptr++].v = c;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "clearstack") {
                                append("stack.ptr = 0;\n");
                            } else if (body[i].getValue() == "&&") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i && stack.data[stack.ptr + 1].i;\n");
                            } else if (body[i].getValue() == "!") {
                                append("stack.data[stack.ptr - 1].i = !stack.data[stack.ptr - 1].i;\n");
                            } else if (body[i].getValue() == "||") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i || stack.data[stack.ptr + 1].i;\n");
                            } else if (body[i].getValue() == "<") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i < stack.data[stack.ptr + 1].i;\n");
                            } else if (body[i].getValue() == ">") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i > stack.data[stack.ptr + 1].i;\n");
                            } else if (body[i].getValue() == "==") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i == stack.data[stack.ptr + 1].i;\n");
                            } else if (body[i].getValue() == "<=") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i <= stack.data[stack.ptr + 1].i;\n");
                            } else if (body[i].getValue() == ">=") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i >= stack.data[stack.ptr + 1].i;\n");
                            } else if (body[i].getValue() == "!=") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i != stack.data[stack.ptr + 1].i;\n");
                            } else if (body[i].getValue() == "++") {
                                append("stack.data[stack.ptr - 1].i++;\n");
                            } else if (body[i].getValue() == "--") {
                                append("stack.data[stack.ptr - 1].i--;\n");
                            } else if (body[i].getValue() == "exit") {
                                append("exit(stack.data[--stack.ptr].i);\n");
                            } else if (body[i].getValue() == "abort") {
                                append("abort();\n");
                            } else if (hasFunction(result, body[i])) {
                                Function* f = getFunctionByName(result, body[i].getValue());
                                if (f->isMethod) {
                                    transpilerError("'" + f->getName() + "' is a method, not a function.", i);
                                    errors.push_back(err);
                                    continue;
                                }
                                if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                                    if (f->getReturnType() == "float") {
                                        append("stack.data[stack.ptr++].f = Function_%s();\n", f->getName().c_str());
                                    } else {
                                        append("stack.data[stack.ptr++].v = (scl_value) Function_%s();\n", f->getName().c_str());
                                    }
                                } else {
                                    append("Function_%s();\n", f->getName().c_str());
                                }
                                if (!Main.options.transpileOnly || Main.options.debugBuild) {
                                    std::string file = function->getFile();
                                    if (file != f->getFile()) {
                                        if (strncmp(function->getFile().c_str(), (scaleFolder + "/Frameworks/").c_str(), (scaleFolder + "/Frameworks/").size()) == 0) {
                                            append("current_file = \"%s\";\n", function->getFile().c_str() + scaleFolder.size() + 12);
                                        } else {
                                            append("current_file = \"%s\";\n", function->getFile().c_str());
                                        }
                                    }
                                }
                            } else if (hasContainer(result, body[i])) {
                                std::string containerName = body[i].getValue();
                                ITER_INC;
                                if (body[i].getType() != tok_dot) {
                                    transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                                    errors.push_back(err);
                                    continue;
                                }
                                ITER_INC;
                                std::string memberName = body[i].getValue();
                                Container container = getContainerByName(result, containerName);
                                if (!container.hasMember(memberName)) {
                                    transpilerError("Unknown container member: '" + memberName + "'", i);
                                    errors.push_back(err);
                                    continue;
                                }
                                if (container.getMemberType(memberName) == "float")
                                    append("stack.data[stack.ptr++].f = Container_%s.%s;\n", containerName.c_str(), memberName.c_str());
                                else
                                    append("stack.data[stack.ptr++].v = (scl_value) Container_%s.%s;\n", containerName.c_str(), memberName.c_str());
                            } else if (getStructByName(result, body[i].getValue()) != Struct("")) {
                                if (body[i + 1].getType() == tok_column) {
                                    ITER_INC;
                                    ITER_INC;
                                    if (body[i].getType() == tok_new) {
                                        std::string struct_ = body[i - 2].getValue();
                                        append("stack.data[stack.ptr++].v = scl_alloc_struct(sizeof(struct Struct_%s), \"%s\");\n", struct_.c_str(), struct_.c_str());
                                        if (hasMethod(result, Token(tok_identifier, "init", 0, "", 0), struct_)) {
                                            append("{\n");
                                            scopeDepth++;
                                            append("scl_value tmp = stack.data[stack.ptr - 1].v;\n");
                                            Method* f = getMethodByName(result, "init", struct_);
                                            if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                                                if (f->getReturnType() == "float") {
                                                    append("stack.data[stack.ptr++].f = Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                                } else {
                                                    append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                                }
                                            } else {
                                                append("Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                            }
                                            append("stack.data[stack.ptr++].v = tmp;\n");
                                            scopeDepth--;
                                            append("}\n");
                                        }
                                    } else {
                                        if (!hasMethod(result, body[i], body[i - 2].getValue())) {
                                            transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + body[i - 2].getValue() + "'", i);
                                            errors.push_back(err);
                                            continue;
                                        }
                                        Method* f = getMethodByName(result, body[i].getValue(), body[i - 2].getValue());
                                        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                                            if (f->getReturnType() == "float") {
                                                append("stack.data[stack.ptr++].f = Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                            } else {
                                                append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                            }
                                        } else {
                                            append("Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                        }
                                    }
                                } else {
                                    ITER_INC;
                                    ITER_INC;
                                    Struct s = getStructByName(result, body[i - 2].getValue());
                                    if (!s.hasMember(body[i].getValue())) {
                                        transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8];
                                    if (mem.getType() == "float")
                                        append("stack.data[stack.ptr - 1].f = ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", s.getName().c_str(), body[i].getValue().c_str());
                                    else
                                        append("stack.data[stack.ptr - 1].v = (scl_value) ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", s.getName().c_str(), body[i].getValue().c_str());
                                }
                            } else if (hasVar(body[i])) {
                                std::string loadFrom = body[i].getValue();
                                Variable v = getVar(body[i]);
                                if (getStructByName(result, v.getType()) != Struct("")) {
                                    if (body[i + 1].getType() == tok_column) {
                                        ITER_INC;
                                        ITER_INC;
                                        if (!hasMethod(result, body[i], v.getType())) {
                                            transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + v.getType() + "'", i);
                                            errors.push_back(err);
                                            continue;
                                        }
                                        Method* f = getMethodByName(result, body[i].getValue(), v.getType());
                                        append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());
                                        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                                            if (f->getReturnType() == "float") {
                                                append("stack.data[stack.ptr++].f = Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                            } else {
                                                append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                            }
                                        } else {
                                            append("Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                        }
                                    } else {
                                        if (body[i + 1].getType() != tok_dot) {
                                            append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());
                                            continue;
                                        }
                                        ITER_INC;
                                        ITER_INC;
                                        Struct s = getStructByName(result, v.getType());
                                        if (!s.hasMember(body[i].getValue())) {
                                            transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                                            errors.push_back(err);
                                            continue;
                                        }
                                        Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8];
                                        if (mem.getType() == "float")
                                            append("stack.data[stack.ptr++].f = Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());
                                        else
                                            append("stack.data[stack.ptr++].v = (scl_value) Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());
                                    }
                                } else {
                                    if (v.getType() == "float")
                                        append("stack.data[stack.ptr++].f = Var_%s;\n", loadFrom.c_str());
                                    else
                                        append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());
                                }
                            } else {
                                transpilerError("Unknown identifier: '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                            }
                            break;
                        }

                        case tok_string_literal: {
                            append("stack.data[stack.ptr++].v = \"%s\";\n", body[i].getValue().c_str());
                            break;
                        }

                        case tok_cdecl: {
                            ITER_INC;
                            if (body[i].getType() != tok_string_literal) {
                                transpilerError("Expected string literal, but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            std::string s = replaceAll(body[i].getValue(), R"(\\\\)", "\\");
                            s = replaceAll(s, R"(\\\")", "\"");
                            append("%s\n", s.c_str());
                            break;
                        }

                        case tok_number:
                        case tok_char_literal: {
                            FPResult numberHandled = handleNumber(fp, body[i], scopeDepth);
                            if (!numberHandled.success) {
                                errors.push_back(numberHandled);
                            }
                            break;
                        }

                        case tok_number_float: {
                            FPResult numberHandled = handleDouble(fp, body[i], scopeDepth);
                            if (!numberHandled.success) {
                                errors.push_back(numberHandled);
                            }
                            break;
                        }

                        case tok_nil:
                        case tok_false: {
                            append("stack.data[stack.ptr++].v = (scl_value) 0;\n");
                            break;
                        }

                        case tok_true: {
                            append("stack.data[stack.ptr++].v = (scl_value) 1;\n");
                            break;
                        }

                        case tok_new: {
                            ITER_INC;
                            if (body[i].getType() != tok_identifier) {
                                transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            std::string struct_ = body[i].getValue();
                            if (getStructByName(result, struct_) == Struct("")) {
                                transpilerError("Usage of undeclared struct '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            append("stack.data[stack.ptr++].v = scl_alloc_struct(sizeof(struct Struct_%s), \"%s\");\n", struct_.c_str(), struct_.c_str());
                            if (hasMethod(result, Token(tok_identifier, "init", 0, "", 0), struct_)) {
                                append("{\n");
                                scopeDepth++;
                                append("scl_value tmp = stack.data[stack.ptr - 1].v;\n");
                                Method* f = getMethodByName(result, "init", struct_);
                                if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                                    if (f->getReturnType() == "float") {
                                        append("stack.data[stack.ptr++].f = Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                    } else {
                                        append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                    }
                                } else {
                                    append("Method_%s_%s();\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                }
                                append("stack.data[stack.ptr++].v = tmp;\n");
                                scopeDepth--;
                                append("}\n");
                            }
                            break;
                        }

                        case tok_is: {
                            ITER_INC;
                            if (body[i].getType() != tok_identifier) {
                                transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            std::string struct_ = body[i].getValue();
                            if (getStructByName(result, struct_) == Struct("")) {
                                transpilerError("Usage of undeclared struct '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            append("stack.data[stack.ptr - 1].v = stack.data[stack.ptr - 1].v && ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->$__type__ == 0x%016llx;\n", struct_.c_str(), hash1((char*) struct_.c_str()));
                            break;
                        }

                        case tok_if: {
                            append("if (stack.data[--stack.ptr].v) {\n");
                            scopeDepth++;
                            break;
                        }

                        case tok_else: {
                            scopeDepth--;
                            append("} else {\n");
                            scopeDepth++;
                            break;
                        }

                        case tok_while: {
                            append("while (1) {\n");
                            scopeDepth++;
                            was_rep.push_back(false);
                            break;
                        }

                        case tok_do: {
                            append("if (!stack.data[--stack.ptr].v) break;\n");
                            break;
                        }

                        case tok_repeat: {
                            if (body[i + 1].getType() != tok_number) {
                                transpilerError("Expected integer, but got '" + body[i + 1].getValue() + "'", i+1);
                                errors.push_back(err);
                                if (body[i + 2].getType() != tok_do)
                                    goto lbl_repeat_do_tok_chk;
                                i += 2;
                                continue;
                            }
                            if (body[i + 2].getType() != tok_do) {
                            lbl_repeat_do_tok_chk:
                                transpilerError("Expected 'do', but got '" + body[i + 2].getValue() + "'", i+2);
                                errors.push_back(err);
                                i += 2;
                                continue;
                            }
                            append("for (scl_value %c = 0; %c < (scl_value) %s; %c++) {\n",
                                'a' + repeat_depth,
                                'a' + repeat_depth,
                                body[i + 1].getValue().c_str(),
                                'a' + repeat_depth
                            );
                            repeat_depth++;
                            scopeDepth++;
                            was_rep.push_back(true);
                            i += 2;
                            break;
                        }

                        case tok_for: {
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
                            if (forHandled.warns.size() > 0) {
                                warns.insert(warns.end(), forHandled.warns.begin(), forHandled.warns.end());
                            }
                            was_rep.push_back(false);
                            i += 7;
                            break;
                        }

                        case tok_done:
                        case tok_fi:
                        case tok_end: {
                            scopeDepth--;
                            append("}\n");
                            if (repeat_depth > 0 && was_rep[was_rep.size() - 1]) {
                                repeat_depth--;
                            }
                            if (was_rep.size() > 0) was_rep.pop_back();
                            break;
                        }

                        case tok_return: {
                            if (i == (body.size() - 1) && (function->getReturnType().size() == 0 || function->getReturnType() == "none")) break;
                            append("callstk.ptr--;\n");
                            if (return_type == "void")
                                append("return;\n");
                            else {
                                if (return_type == "scl_str") {
                                    append("return stack.data[--stack.ptr].s;\n");
                                } else if (return_type == "scl_int") {
                                    append("return stack.data[--stack.ptr].i;\n");
                                } else if (return_type == "scl_float") {
                                    append("return stack.data[--stack.ptr].f;\n");
                                } else if (return_type == "scl_value") {
                                    append("return stack.data[--stack.ptr].v;\n");
                                } else if (strncmp(return_type.c_str(), "struct", 6) == 0) {
                                    append("return (struct Struct_%s*) stack.data[--stack.ptr].v;\n", function->getReturnType().c_str());
                                }
                            }
                            break;
                        }

                        case tok_addr_ref: {
                            ITER_INC;
                            Token toGet = body[i];
                            if (hasFunction(result, toGet)) {
                                Function* f = getFunctionByName(result, toGet.getValue());
                                append("stack.data[stack.ptr++].v = (scl_value) &Function_%s;\n", f->getName().c_str());
                                ITER_INC;
                            } else if (hasVar(toGet)) {
                                Variable v = getVar(body[i]);
                                std::string loadFrom = v.getName();
                                if (getStructByName(result, v.getType()) != Struct("")) {
                                    if (body[i + 1].getType() == tok_column) {
                                        if (!hasMethod(result, body[i], v.getType())) {
                                            transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + v.getType() + "'", i);
                                            errors.push_back(err);
                                            continue;
                                        }
                                        Method* f = getMethodByName(result, body[i].getValue(), v.getType());
                                        append("stack.data[stack.ptr++].v = (scl_value) &Method_%s_%s;\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());    
                                    } else {
                                        if (body[i + 1].getType() != tok_dot) {
                                            append("stack.data[stack.ptr++].v = (scl_value) &Var_%s;\n", loadFrom.c_str());
                                            continue;
                                        }
                                        ITER_INC;
                                        ITER_INC;
                                        Struct s = getStructByName(result, v.getType());
                                        if (!s.hasMember(body[i].getValue())) {
                                            transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                                            errors.push_back(err);
                                            continue;
                                        }
                                        append("stack.data[stack.ptr++].v = (scl_value) &Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());
                                    }
                                } else {
                                    append("stack.data[stack.ptr++].v = (scl_value) &Var_%s;\n", loadFrom.c_str());
                                }
                            } else if (hasContainer(result, toGet)) {
                                ITER_INC;
                                std::string containerName = body[i].getValue();
                                ITER_INC;
                                if (body[i].getType() != tok_dot) {
                                    transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                                    errors.push_back(err);
                                    continue;
                                }
                                ITER_INC;
                                std::string memberName = body[i].getValue();
                                Container container = getContainerByName(result, containerName);
                                if (!container.hasMember(memberName)) {
                                    transpilerError("Unknown container member: '" + memberName + "'", i);
                                    errors.push_back(err);
                                    continue;
                                }
                                append("stack.data[stack.ptr++].v = (scl_value) &(Container_%s.%s);\n", containerName.c_str(), memberName.c_str());
                            } else {
                                transpilerError("Unknown variable: '" + toGet.getValue() + "'", i+1);
                                errors.push_back(err);
                            }
                            break;
                        }

                        case tok_store: {
                            ITER_INC;
                            if (body[i].getType() == tok_addr_of) {
                                ITER_INC;
                                if (hasContainer(result, body[i])) {
                                    std::string containerName = body[i].getValue();
                                    ITER_INC;
                                    if (body[i].getType() != tok_dot) {
                                        transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    ITER_INC;
                                    std::string memberName = body[i].getValue();
                                    Container container = getContainerByName(result, containerName);
                                    if (!container.hasMember(memberName)) {
                                        transpilerError("Unknown container member: '" + memberName + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    append("*((scl_value*) Container_%s.%s) = stack.data[--stack.ptr].v;\n", containerName.c_str(), memberName.c_str());
                                } else {
                                    if (body[i].getType() != tok_identifier) {
                                        transpilerError("'" + body[i].getValue() + "' is not an identifier!", i);
                                        errors.push_back(err);
                                    }
                                    if (!hasVar(body[i])) {
                                        transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                    }
                                    Variable v = getVar(body[i]);
                                    std::string loadFrom = v.getName();
                                    if (getStructByName(result, v.getType()) != Struct("")) {
                                        if (body[i + 1].getType() != tok_dot) {
                                            append("*(scl_value*) Var_%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str());
                                            continue;
                                        }
                                        ITER_INC;
                                        ITER_INC;
                                        Struct s = getStructByName(result, v.getType());
                                        if (!s.hasMember(body[i].getValue())) {
                                            transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                                            errors.push_back(err);
                                            continue;
                                        }
                                        append("*(scl_value*) Var_%s->%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str(), body[i].getValue().c_str());
                                    } else {
                                        append("*(scl_value*) Var_%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str());
                                    }
                                }
                            } else {
                                if (hasContainer(result, body[i])) {
                                    std::string containerName = body[i].getValue();
                                    ITER_INC;
                                    if (body[i].getType() != tok_dot) {
                                        transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    ITER_INC;
                                    std::string memberName = body[i].getValue();
                                    Container container = getContainerByName(result, containerName);
                                    if (!container.hasMember(memberName)) {
                                        transpilerError("Unknown container member: '" + memberName + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    append("Container_%s.%s = (%s) stack.data[--stack.ptr].v;\n", containerName.c_str(), memberName.c_str(), sclReturnTypeToCReturnType(result, container.getMemberType(memberName)).c_str());
                                } else {
                                    if (body[i].getType() != tok_identifier) {
                                        transpilerError("'" + body[i].getValue() + "' is not an identifier!", i);
                                        errors.push_back(err);
                                    }
                                    if (!hasVar(body[i])) {
                                        transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                    }
                                    Variable v = getVar(body[i]);
                                    std::string loadFrom = v.getName();
                                    if (getStructByName(result, v.getType()) != Struct("")) {
                                        if (body[i + 1].getType() != tok_dot) {
                                            append("Var_%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str());
                                            continue;
                                        }
                                        ITER_INC;
                                        ITER_INC;
                                        Struct s = getStructByName(result, v.getType());
                                        if (!s.hasMember(body[i].getValue())) {
                                            transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                                            errors.push_back(err);
                                            continue;
                                        }
                                        Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8];
                                    if (mem.getType() == "float")
                                        append("Var_%s->%s = stack.data[--stack.ptr].f;\n", loadFrom.c_str(), body[i].getValue().c_str());
                                    else
                                        append("Var_%s->%s = (%s) stack.data[--stack.ptr].v;\n", loadFrom.c_str(), body[i].getValue().c_str(), sclReturnTypeToCReturnType(result, mem.getType()).c_str());
                                    } else {
                                        Variable v = getVar(body[i]);
                                        if (v.getType() == "float")
                                            append("Var_%s = stack.data[--stack.ptr].f;\n", loadFrom.c_str());
                                        else
                                            append("Var_%s = (%s) stack.data[--stack.ptr].v;\n", loadFrom.c_str(), sclReturnTypeToCReturnType(result, v.getType()).c_str());
                                    }
                                }
                            }
                            break;
                        }

                        case tok_column: break;

                        case tok_declare: {
                            if (body[i + 1].getType() != tok_identifier) {
                                transpilerError("'" + body[i + 1].getValue() + "' is not an identifier!", i+1);
                                errors.push_back(err);
                                continue;
                            }
                            if (hasFunction(result, body[i + 1])) {
                                transpilerError("Variable '" + body[i + 1].getValue() + "' shadowed by function '" + body[i + 1].getValue() + "'", i+1);
                                if (!Main.options.Werror) warns.push_back(err);
                                else errors.push_back(err);
                            }
                            if (hasContainer(result, body[i + 1])) {
                                transpilerError("Variable '" + body[i + 1].getValue() + "' shadowed by container '" + body[i + 1].getValue() + "'", i+1);
                                if (!Main.options.Werror) warns.push_back(err);
                                else errors.push_back(err);
                            }
                            if (hasVar(body[i + 1])) {
                                transpilerError("Variable '" + body[i + 1].getValue() + "' is already declared and shadows it.", i+1);
                                if (!Main.options.Werror) warns.push_back(err);
                                else errors.push_back(err);
                            }
                            std::string name = body[i + 1].getValue();
                            std::string type = "any";
                            ITER_INC;
                            if (body[i+1].getType() == tok_column) {
                                ITER_INC;
                                ITER_INC;
                                if (body[i].getType() != tok_identifier) {
                                    FPResult result;
                                    result.message = "Expected identifier, but got '" + body[i].getValue() + "'";
                                    result.column = body[i].getColumn();
                                    result.value = body[i].getValue();
                                    result.line = body[i].getLine();
                                    result.in = body[i].getFile();
                                    result.type = body[i].getType();
                                    result.success = false;
                                    errors.push_back(result);
                                }
                                if (body[i].getValue() == "none") {
                                    transpilerError("Type 'none' is only valid for function return types.", i);
                                    errors.push_back(err);
                                }
                                type = body[i].getValue();
                            } else {
                                transpilerError("A type is required!", i);
                                errors.push_back(err);
                                continue;
                            }
                            Variable v = Variable(name, type);
                            vars.push_back(v);
                            append("%s Var_%s;\n", sclReturnTypeToCReturnType(result, v.getType()).c_str(), v.getName().c_str());
                            break;
                        }

                        case tok_continue: {
                            append("continue;\n");
                            break;
                        }

                        case tok_break: {
                            append("break;\n");
                            break;
                        }

                        case tok_goto: {
                            ITER_INC;
                            if (body[i].getType() != tok_identifier) {
                                transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                            }
                            append("goto %s;\n", body[i].getValue().c_str());
                            break;
                        }

                        case tok_label: {
                            ITER_INC;
                            if (body[i].getType() != tok_identifier) {
                                transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                            }
                            append("%s:\n", body[i].getValue().c_str());
                            break;
                        }

                        case tok_addr_of: {
                            append("stack.data[stack.ptr - 1].v = (*(scl_value*) stack.data[stack.ptr - 1].v);\n");
                            break;
                        }

                        case tok_curly_close: {
                            append("stack.ptr -= 2; stack.data[stack.ptr++].v =(*(scl_value*) (stack.data[stack.ptr].v + (stack.data[stack.ptr + 1].i * 8)));\n");
                            break;
                        }

                        case tok_curly_open: break;

                        default: {
                            transpilerError("Unknown identifier: '" + body[i].getValue() + "'", i);
                            errors.push_back(err);
                        }
                    }
                }
            }
            scopeDepth = 1;
            append("callstk.ptr--;\n");
            scopeDepth = 0;
            append("}\n\n");

            if (sap_depth > 0) {
                transpilerError("SAP never closed, Opened here:", sap_tokens.size() - 1);
                errors.push_back(err);
            }
        }
    }
} // namespace sclc

#endif
