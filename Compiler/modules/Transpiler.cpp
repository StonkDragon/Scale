#ifndef _TRANSPILER_CPP
#define _TRANSPILER_CPP

#include <filesystem>

#include "../headers/Common.hpp"
#include "../headers/TranspilerDefs.hpp"

#define ITER_INC do { i++; if (i >= body.size()) { FPResult err; err.success = false; err.message = "Unexpected end of function!"; err.column = body[i - 1].getColumn(); err.line = body[i - 1].getLine(); err.in = body[i - 1].getFile(); err.value = body[i - 1].getValue(); err.type =  body[i - 1].getType(); errors.push_back(err); return; } } while (0)

namespace sclc {
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

    void ConvertC::writeFunctionHeaders(FILE* fp, TPResult result) {
        int scopeDepth = 0;
        append("/* FUNCTION HEADERS */\n");

        FILE* nomangled;
        if (Main.options.transpileOnly) {
            remove("scale_support.h");
            nomangled = fopen("scale_support.h", "a");
            fprintf(nomangled, "#ifndef SCALE_SUPPORT_H\n");
            fprintf(nomangled, "#define SCALE_SUPPORT_H\n\n");

            fprintf(nomangled, "#define scl_export(func_name) \\\n");
            fprintf(nomangled, "    void func_name (void); \\\n");
            fprintf(nomangled, "    void fn_ ## func_name (void); \\\n");
            fprintf(nomangled, "    void fn_ ## func_name () { func_name (); } \\\n");
            fprintf(nomangled, "    void func_name (void)\n\n");
            fprintf(nomangled, "#define ssize_t signed long\n");
            fprintf(nomangled, "#define scl_value void*\n");
            fprintf(nomangled, "#define scl_int long long\n");
            fprintf(nomangled, "#define scl_str char*\n");
            fprintf(nomangled, "#define scl_float double\n\n");
        }

        for (Function function : result.functions) {
            append("void fn_%s(void);\n", function.getName().c_str());
            if (!Main.options.transpileOnly) continue;
            for (Modifier m : function.getModifiers()) {
                if (m == mod_nomangle) {
                    std::string args = "(";
                    for (size_t i = 0; i < function.getArgs().size(); i++) {
                        if (i != 0) {
                            args += ", ";
                        }
                        args += "scl_value ";
                        args += function.getArgs()[i];
                    }
                    args += ")";
                    
                    fprintf(nomangled, "void %s%s;\n", function.getName().c_str(), args.c_str());
                }
            }
        }

        if (Main.options.transpileOnly) {
            fprintf(nomangled, "#endif\n");
            fclose(nomangled);
        }
        
        append("\n");
    }

    void ConvertC::writeExternHeaders(FILE* fp, TPResult result) {
        int scopeDepth = 0;
        append("/* EXTERN FUNCTIONS */\n");

        for (Function function : result.extern_functions) {
            bool hasFunction = false;
            for (Function f : result.functions) {
                if (f.getName() == function.getName()) {
                    hasFunction = true;
                }
            }
            if (!hasFunction) {
                std::string functionDeclaration = "";

                functionDeclaration += function.getName() + "(";
                for (size_t i = 0; i < function.getArgs().size(); i++) {
                    if (i != 0) {
                        functionDeclaration += ", ";
                    }
                    functionDeclaration += function.getArgs()[i];
                }
                functionDeclaration += ")";
                append("void fn_%s(void);\n", function.getName().c_str());
            }
        }

        append("\n");
    }

    void ConvertC::writeInternalFunctions(FILE* fp, TPResult result) {
        int scopeDepth = 0;
        append("const unsigned long long scl_internal_function_names_with_args[] = {\n");
        for (Function function : result.extern_functions) {
            std::string functionDeclaration = "";

            functionDeclaration += function.getName() + "(";
            for (size_t i = 0; i < function.getArgs().size(); i++) {
                if (i != 0) {
                    functionDeclaration += ", ";
                }
                functionDeclaration += function.getArgs()[i];
            }
            functionDeclaration += ")";
            append("  0x%016llxLLU /* %s */,\n", hash1((char*) functionDeclaration.c_str()), (char*) functionDeclaration.c_str());
        }
        for (Function function : result.functions) {
            std::string functionDeclaration = "";

            functionDeclaration += function.getName() + "(";
            for (size_t i = 0; i < function.getArgs().size(); i++) {
                if (i != 0) {
                    functionDeclaration += ", ";
                }
                functionDeclaration += function.getArgs()[i];
            }
            functionDeclaration += ")";
            append("  0x%016llxLLU /* %s */,\n", hash1((char*) functionDeclaration.c_str()), (char*) functionDeclaration.c_str());
        }
        append("};\n");
        append("const unsigned long long scl_internal_function_names[] = {\n");
        for (Function function : result.extern_functions) {
            std::string functionDeclaration = function.getName();
            append("  0x%016llxLLU /* %s */,\n", hash1((char*) functionDeclaration.c_str()), (char*) functionDeclaration.c_str());
        }
        for (Function function : result.functions) {
            std::string functionDeclaration = function.getName();
            append("  0x%016llxLLU /* %s */,\n", hash1((char*) functionDeclaration.c_str()), (char*) functionDeclaration.c_str());
        }
        append("};\n");

        append("const scl_method scl_internal_function_ptrs[] = {\n");
        for (Function function : result.extern_functions) {
            append("  fn_%s,\n", function.getName().c_str());
        }
        for (Function function : result.functions) {
            append("  fn_%s,\n", function.getName().c_str());
        }
        append("};\n");
        append("const size_t scl_internal_function_ptrs_size = %zu;\n", result.functions.size() + result.extern_functions.size());
        append("const size_t scl_internal_function_names_size = %zu;\n", result.functions.size() + result.extern_functions.size());

        append("\n");
    }

    void ConvertC::writeGlobals(FILE* fp, std::vector<std::string>& globals, TPResult result) {
        int scopeDepth = 0;
        append("/* GLOBALS */\n");

        for (std::string s : result.globals) {
            append("scl_value _%s;\n", s.c_str());
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
            for (std::string s : c.getMembers()) {
                append("  scl_value %s;\n", s.c_str());
            }
            append("} cont_%s = {0};\n", c.getName().c_str());
        }

        append("\n");
        append("const scl_value* scl_internal_globals_ptrs[] = {\n");
        for (std::string s : result.globals) {
            append("  (const scl_value*) &_%s,\n", s.c_str());
        }
        size_t container_member_count = 0;
        for (Container c : result.containers) {
            for (std::string s : c.getMembers()) {
                append("  (const scl_value*) &cont_%s.%s,\n", c.getName().c_str(), s.c_str());
                container_member_count++;
            }
        }
        append("};\n");
        append("const size_t scl_internal_globals_ptrs_size = %zu;\n", result.globals.size() + container_member_count);
        
        append("\n");
    }

    void ConvertC::writeStructs(FILE* fp, TPResult result) {
        int scopeDepth = 0;
        append("/* COMPLEXES */\n");
        for (Struct c : result.structs) {
            append("struct scl_struct_%s {\n", c.getName().c_str());
            append("  scl_int $__type__;\n");
            append("  scl_str $__type_name__;\n");
            for (std::string s : c.getMembers()) {
                append("  scl_value %s;\n", s.c_str());
            }
            append("};\n");
        }
        append("\n");
    }

    void ConvertC::writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<std::string>& globals, TPResult result) {
        (void) warns;
        int scopeDepth = 0;
        append("/* EXTERN VARS FROM INTERNAL */\n");
        append("extern scl_str current_file;\n");
        append("extern size_t current_line;\n");
        append("extern size_t current_column;\n");
        append("extern scl_stack_t stack;\n");
        append("extern scl_stack_t callstk;\n");
        append("extern size_t sap_index;\n");
        append("extern size_t sap_enabled[];\n");
        append("extern scl_value alloced_structs[];\n");
        append("extern size_t alloced_structs_count;\n");
        append("extern scl_value allocated[];\n");
        append("extern size_t allocated_count;\n");
        append("extern scl_value** locals[];\n");
        append("extern size_t locals_count[];\n");

        append("/* FUNCTIONS */\n");

        int funcsSize = result.functions.size();
        for (int f = 0; f < funcsSize; f++)
        {
            Function function = result.functions[f];
            vars.clear();
            for (std::string g : globals) {
                vars.push_back(g);
            }

            scopeDepth = 0;
            bool noWarns = false;
            bool no_mangle = false;
            bool nps = false;
            ssize_t sap_depth = 0;

            for (Modifier modifier : function.getModifiers()) {
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

            functionDeclaration += function.getName() + "(";
            for (size_t i = 0; i < function.getArgs().size(); i++) {
                if (i != 0) {
                    functionDeclaration += ", ";
                }
                functionDeclaration += function.getArgs()[i];
            }
            functionDeclaration += ")";

            if (no_mangle) {
                std::string args = "(";
                for (size_t i = 0; i < function.getArgs().size(); i++) {
                    if (i != 0) {
                        args += ", ";
                    }
                    args += "scl_value _";
                    args += function.getArgs()[i];
                }
                args += ")";

                append("void %s%s {\n", function.getName().c_str(), args.c_str());
                scopeDepth++;
                for (size_t i = 0; i < function.getArgs().size(); i++) {
                    append("stack.data[stack.ptr++].v = _%s;\n", function.getArgs()[i].c_str());
                }
                append("fn_%s();\n", function.getName().c_str());
                scopeDepth--;
                append("}\n");
            }

            append("void fn_%s(void) {\n", function.getName().c_str());
            
            scopeDepth++;

            append("callstk.data[callstk.ptr++].v = \"%s\";\n", functionDeclaration.c_str());

            for (ssize_t i = (ssize_t) function.getArgs().size() - 1; i >= 0; i--) {
                std::string var = function.getArgs()[i];
                vars.push_back(var);
                append("scl_value _%s = stack.data[--stack.ptr].v;\n", var.c_str());
            }

            if (!Main.options.transpileOnly || Main.options.debugBuild) {
                std::string file = function.getFile();
                if (strncmp(function.getFile().c_str(), (scaleFolder + "/Frameworks/").c_str(), (scaleFolder + "/Frameworks/").size()) == 0) {
                    append("current_file = \"%s\";\n", function.getFile().c_str() + scaleFolder.size() + 12);
                } else {
                    append("current_file = \"%s\";\n", function.getFile().c_str());
                }
            }


            std::vector<Token> body = function.getBody();
            std::vector<Token> sap_tokens;
            std::vector<bool> was_rep;
            size_t i;
            char repeat_depth = 0;
            for (i = 0; i < body.size(); i++)
            {
                if (body[i].getType() == tok_ignore) continue;

                std::string file = body[i].getFile();
                if (!Main.options.transpileOnly || Main.options.debugBuild) {
                    if (
                        body[i].getType() != tok_declare && body[i].getType() != tok_while &&
                        body[i].getType() != tok_end && body[i].getType() != tok_fi &&
                        body[i].getType() != tok_done
                    ) {
                        append("current_line = %d;\n", body[i].getLine());
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
                                append("{\n");
                                scopeDepth++;
                                append("scl_int b = stack.data[--stack.ptr].i;\n");
                                append("scl_int a = stack.data[--stack.ptr].i;\n");
                                append("stack.data[stack.ptr++].i = a && b;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "!") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_int a = stack.data[--stack.ptr].i;\n");
                                append("stack.data[stack.ptr++].i = !a;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "||") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_int b = stack.data[--stack.ptr].i;\n");
                                append("scl_int a = stack.data[--stack.ptr].i;\n");
                                append("stack.data[stack.ptr++].i = a || b;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "<") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_int b = stack.data[--stack.ptr].i;\n");
                                append("scl_int a = stack.data[--stack.ptr].i;\n");
                                append("stack.data[stack.ptr++].i = a < b;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == ">") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_int b = stack.data[--stack.ptr].i;\n");
                                append("scl_int a = stack.data[--stack.ptr].i;\n");
                                append("stack.data[stack.ptr++].i = a > b;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "==") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_int b = stack.data[--stack.ptr].i;\n");
                                append("scl_int a = stack.data[--stack.ptr].i;\n");
                                append("stack.data[stack.ptr++].i = a == b;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "<=") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_int b = stack.data[--stack.ptr].i;\n");
                                append("scl_int a = stack.data[--stack.ptr].i;\n");
                                append("stack.data[stack.ptr++].i = a <= b;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == ">=") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_int b = stack.data[--stack.ptr].i;\n");
                                append("scl_int a = stack.data[--stack.ptr].i;\n");
                                append("stack.data[stack.ptr++].i = a >= b;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "!=") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_int b = stack.data[--stack.ptr].i;\n");
                                append("scl_int a = stack.data[--stack.ptr].i;\n");
                                append("stack.data[stack.ptr++].i = a != b;\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "++") {
                                append("stack.data[stack.ptr - 1].i++;\n");
                            } else if (body[i].getValue() == "--") {
                                append("stack.data[stack.ptr - 1].i--;\n");
                            } else if (body[i].getValue() == "exit") {
                                append("exit(stack.data[--stack.ptr].i);\n");
                            } else if (body[i].getValue() == "abort") {
                                append("abort();\n");
                            } else if (hasFunction(result, body[i])) {
                                Function f = getFunctionByName(result, body[i].getValue());
                                std::string functionDeclaration = "";

                                functionDeclaration += f.getName() + "(";
                                for (size_t i = 0; i < f.getArgs().size(); i++) {
                                    if (i != 0) {
                                        functionDeclaration += ", ";
                                    }
                                    functionDeclaration += f.getArgs()[i];
                                }
                                functionDeclaration += ")";
                                if (!(Main.options.optimizer == "O3" || Main.options.optimizer == "Ofast")) append("if (stack.ptr < %zu) { scl_str err = (scl_str) scl_alloc(MAX_STRING_SIZE); sprintf(err, \"Error: Function %s requires %zu arguments, but only %%zu are provided.\", stack.ptr); scl_security_throw(EX_STACK_UNDERFLOW, err); }\n", f.getArgs().size(), functionDeclaration.c_str(), f.getArgs().size());
                                append("fn_%s();\n", body[i].getValue().c_str());
                                if (!Main.options.transpileOnly || Main.options.debugBuild) {
                                    std::string file = function.getFile();
                                    if (strncmp(function.getFile().c_str(), (scaleFolder + "/Frameworks/").c_str(), (scaleFolder + "/Frameworks/").size()) == 0) {
                                        append("current_file = \"%s\";\n", function.getFile().c_str() + scaleFolder.size() + 12);
                                    } else {
                                        append("current_file = \"%s\";\n", function.getFile().c_str());
                                    }
                                }
                            } else if (hasContainer(result, body[i])) {
                                std::string containerName = body[i].getValue();
                                ITER_INC;
                                if (body[i].getType() != tok_container_acc) {
                                    transpilerError("Expected '->' to access container contents, but got '" + body[i].getValue() + "'", i);
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
                                append("stack.data[stack.ptr++].v = cont_%s.%s;\n", containerName.c_str(), memberName.c_str());
                            } else if (hasVar(body[i])) {
                                std::string loadFrom = body[i].getValue();
                                append("stack.data[stack.ptr++].v = _%s;\n", loadFrom.c_str());
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

                        case tok_double_column: {
                            ITER_INC;
                            if (body[i].getType() != tok_identifier) {
                                transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            std::string structName = body[i].getValue();
                            if (getStructByName(result, structName) == Struct("")) {
                                transpilerError("Usage of undeclared struct '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            ITER_INC;
                            if (body[i].getType() != tok_dot) {
                                transpilerError("Expected '.', but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            ITER_INC;
                            if (body[i].getType() != tok_identifier) {
                                transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            std::string memberName = body[i].getValue();
                            if (!getStructByName(result, structName).hasMember(memberName)) {
                                transpilerError("Unknown member of struct '" + structName + "': '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }

                            append("stack.data[stack.ptr - 1].v = ((struct scl_struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", structName.c_str(), memberName.c_str());
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
                            append("{\n");
                            scopeDepth++;
                            append("struct scl_struct_%s* new_tmp = scl_alloc_struct(sizeof(struct scl_struct_%s));\n", struct_.c_str(), struct_.c_str());
                            append("new_tmp->$__type__ = 0x%016llx;\n", hash1((char*) struct_.c_str()));
                            append("new_tmp->$__type_name__ = \"%s\";\n", struct_.c_str());
                            append("stack.data[stack.ptr++].v = new_tmp;\n");
                            scopeDepth--;
                            append("}\n");
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
                            append("{\n");
                            append("  scl_value addr = stack.data[--stack.ptr].v;\n");
                            append("  stack.data[stack.ptr++].v = addr && ((struct scl_struct_%s*) addr)->$__type__ == 0x%016llx;\n", struct_.c_str(), hash1((char*) struct_.c_str()));
                            append("}\n");
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
                            if (i == (body.size() - 1)) break;
                            append("callstk.ptr--;\n");
                            append("return;\n");
                            break;
                        }

                        case tok_addr_ref: {
                            Token toGet = body[i + 1];
                            if (hasFunction(result, toGet)) {
                                append("stack.data[stack.ptr++].v = (scl_value) &fn_%s;\n", toGet.getValue().c_str());
                                ITER_INC;
                            } else if (hasVar(toGet) && body[i + 2].getType() != tok_double_column) {
                                append("stack.data[stack.ptr++].v = &_%s;\n", toGet.getValue().c_str());
                                ITER_INC;
                            } else if (hasContainer(result, toGet)) {
                                ITER_INC;
                                std::string containerName = body[i].getValue();
                                ITER_INC;
                                if (body[i].getType() != tok_container_acc) {
                                    transpilerError("Expected '->' to access container contents, but got '" + body[i].getValue() + "'", i);
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
                                append("stack.data[stack.ptr++].v = &(cont_%s.%s);\n", containerName.c_str(), memberName.c_str());
                            } else if (body[i + 2].getType() == tok_double_column) {
                                ITER_INC;
                                std::string varName = body[i].getValue();
                                i += 2;
                                if (body[i].getType() != tok_identifier) {
                                    transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                    errors.push_back(err);
                                    continue;
                                }
                                std::string structName = body[i].getValue();
                                if (getStructByName(result, structName) == Struct("")) {
                                    transpilerError("Usage of undeclared struct '" + body[i].getValue() + "'", i);
                                    errors.push_back(err);
                                    continue;
                                }
                                ITER_INC;
                                if (body[i].getType() != tok_dot) {
                                    transpilerError("Expected '.', but got '" + body[i].getValue() + "'", i);
                                    errors.push_back(err);
                                    continue;
                                }
                                ITER_INC;
                                if (body[i].getType() != tok_identifier) {
                                    transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                    errors.push_back(err);
                                    continue;
                                }
                                std::string memberName = body[i].getValue();
                                if (!getStructByName(result, structName).hasMember(memberName)) {
                                    transpilerError("Unknown member of struct '" + structName + "': '" + body[i].getValue() + "'", i);
                                    errors.push_back(err);
                                    continue;
                                }
                                append("stack.data[stack.ptr++].v = &(((struct scl_struct_%s*) _%s)->%s);\n", structName.c_str(), varName.c_str(), memberName.c_str());
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
                                if (body[i + 1].getType() == tok_container_acc) {
                                    std::string containerName = body[i].getValue();
                                    ITER_INC;
                                    if (body[i].getType() != tok_container_acc) {
                                        transpilerError("Expected '->' to access container contents, but got '" + body[i].getValue() + "'", i);
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
                                    append("*((scl_value*) cont_%s.%s) = stack.data[--stack.ptr].v;\n", containerName.c_str(), memberName.c_str());
                                } else if (body[i + 1].getType() == tok_double_column) {
                                    std::string varName = body[i].getValue();
                                    i += 2;
                                    if (body[i].getType() != tok_identifier) {
                                        transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    std::string structName = body[i].getValue();
                                    if (getStructByName(result, structName) == Struct("")) {
                                        transpilerError("Usage of undeclared struct '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    ITER_INC;
                                    if (body[i].getType() != tok_dot) {
                                        transpilerError("Expected '.', but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    ITER_INC;
                                    if (body[i].getType() != tok_identifier) {
                                        transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    std::string memberName = body[i].getValue();
                                    if (!getStructByName(result, structName).hasMember(memberName)) {
                                        transpilerError("Unknown member of struct '" + structName + "': '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    append("*((scl_value*) ((struct scl_struct_%s*) _%s)->%s) = stack.data[--stack.ptr].v;\n", structName.c_str(), varName.c_str(), memberName.c_str());
                                } else {
                                    if (body[i].getType() != tok_identifier) {
                                        transpilerError("'" + body[i].getValue() + "' is not an identifier!", i);
                                        errors.push_back(err);
                                    }
                                    if (!hasVar(body[i])) {
                                        transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                    }
                                    append("*((scl_value*) _%s) = stack.data[--stack.ptr].v;\n", body[i].getValue().c_str());
                                }
                            } else {
                                if (body[i + 1].getType() == tok_container_acc) {
                                    std::string containerName = body[i].getValue();
                                    ITER_INC;
                                    if (body[i].getType() != tok_container_acc) {
                                        transpilerError("Expected '->' to access container contents, but got '" + body[i].getValue() + "'", i);
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
                                    append("cont_%s.%s = stack.data[--stack.ptr].v;\n", containerName.c_str(), memberName.c_str());
                                } else if (body[i + 1].getType() == tok_double_column) {
                                    std::string varName = body[i].getValue();
                                    i += 2;
                                    if (body[i].getType() != tok_identifier) {
                                        transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    std::string structName = body[i].getValue();
                                    if (getStructByName(result, structName) == Struct("")) {
                                        transpilerError("Usage of undeclared struct '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    ITER_INC;
                                    if (body[i].getType() != tok_dot) {
                                        transpilerError("Expected '.', but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    ITER_INC;
                                    if (body[i].getType() != tok_identifier) {
                                        transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    std::string memberName = body[i].getValue();
                                    if (!getStructByName(result, structName).hasMember(memberName)) {
                                        transpilerError("Unknown member of struct '" + structName + "': '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                        continue;
                                    }
                                    append("((struct scl_struct_%s*) _%s)->%s = stack.data[--stack.ptr].v;\n", structName.c_str(), varName.c_str(), memberName.c_str());
                                } else {
                                    if (body[i].getType() != tok_identifier) {
                                        transpilerError("'" + body[i].getValue() + "' is not an identifier!", i);
                                        errors.push_back(err);
                                    }
                                    if (!hasVar(body[i])) {
                                        transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                    }
                                    std::string storeIn = body[i].getValue();
                                    append("_%s = stack.data[--stack.ptr].v;\n", storeIn.c_str());
                                }
                            }
                            break;
                        }

                        case tok_declare: {
                            if (body[i + 1].getType() != tok_identifier) {
                                transpilerError("'" + body[i + 1].getValue() + "' is not an identifier!", i+1);
                                errors.push_back(err);
                                continue;
                            }
                            if (hasFunction(result, body[i + 1])) {
                                transpilerError("Variable '" + body[i + 1].getValue() + "' shadowed by function '" + body[i + 1].getValue() + "'", i+1);
                                warns.push_back(err);
                            }
                            if (hasContainer(result, body[i + 1])) {
                                transpilerError("Variable '" + body[i + 1].getValue() + "' shadowed by container '" + body[i + 1].getValue() + "'", i+1);
                                warns.push_back(err);
                            }
                            if (hasVar(body[i + 1])) {
                                transpilerError("Variable '" + body[i + 1].getValue() + "' is already declared and shadows it.", i+1);
                                warns.push_back(err);
                            }
                            vars.push_back(body[i + 1].getValue());
                            std::string loadFrom = body[i + 1].getValue();
                            append("scl_value _%s;\n", loadFrom.c_str());
                            ITER_INC;
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

                        case tok_curly_open: {
                            ITER_INC;
                            if (body[i].getType() != tok_number) {
                                transpilerError("Expected integer, but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                            }
                            Token index = body[i];
                            ITER_INC;
                            if (body[i].getType() != tok_curly_close) {
                                transpilerError("Expected '}', but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                            }
                            append("stack.data[stack.ptr - 1].v =(*(scl_value*) (stack.data[stack.ptr - 1].v + (%s * 8)));\n", index.getValue().c_str());
                            break;
                        }

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
