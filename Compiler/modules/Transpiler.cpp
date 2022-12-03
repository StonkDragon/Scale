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
                append("%s Method_%s_%s(struct Struct_%s* Var_self);\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), ((Method*)(function))->getMemberType().c_str());
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
        if (result.extern_functions.size() == 0) return;
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
                    append("%s Method_%s_%s(struct Struct_%s* Var_self);\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), ((Method*)(function))->getMemberType().c_str());
                }
            }
        }

        append("\n");
    }

    void ConvertC::writeGlobals(FILE* fp, std::vector<Variable>& globals, TPResult result) {
        int scopeDepth = 0;
        if (result.globals.size() == 0) return;
        append("/* GLOBALS */\n");

        for (Variable s : result.globals) {
            append("%s Var_%s;\n", sclReturnTypeToCReturnType(result, s.getType()).c_str(), s.getName().c_str());
            if (nomangled && Main.options.transpileOnly) fprintf(nomangled, "extern %s Var_%s;\n", sclReturnTypeToCReturnType(result, s.getType()).c_str(), s.getName().c_str());
            vars[varDepth].push_back(s);
            globals.push_back(s);
        }

        append("\n");
    }

    void ConvertC::writeContainers(FILE* fp, TPResult result) {
        int scopeDepth = 0;
        if (result.containers.size() == 0) return;
        append("/* CONTAINERS */\n");
        for (Container c : result.containers) {
            append("struct {\n");
            for (Variable s : c.getMembers()) {
                append("  %s %s;\n", sclReturnTypeToCReturnType(result, s.getType()).c_str(), s.getName().c_str());
            }
            append("} Container_%s = {0};\n", c.getName().c_str());
            if (nomangled && Main.options.transpileOnly) {
                fprintf(nomangled, "struct Container_%s {\n", c.getName().c_str());
                for (Variable s : c.getMembers()) {
                    fprintf(nomangled, "  %s %s;\n", sclReturnTypeToCReturnType(result, s.getType()).c_str(), s.getName().c_str());
                }
                fprintf(nomangled, "};\n");
                fprintf(nomangled, "extern struct Container_%s Container_%s;\n", c.getName().c_str(), c.getName().c_str());
            }
        }
    }

    void ConvertC::writeStructs(FILE* fp, TPResult result) {
        int scopeDepth = 0;
        if (result.structs.size() == 0) goto label_writeStructs_close_file;
        append("/* STRUCTS */\n");
        for (Struct c : result.structs) {
            append("struct Struct_%s {\n", c.getName().c_str());
            append("  scl_int $__type__;\n");
            append("  scl_str $__type_name__;\n");
            for (Variable s : c.getMembers()) {
                append("  %s %s;\n", sclReturnTypeToCReturnType(result, s.getType()).c_str(), s.getName().c_str());
            }
            append("};\n");
            if (nomangled && Main.options.transpileOnly) {
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
    label_writeStructs_close_file:

        if (nomangled && Main.options.transpileOnly) {
            fprintf(nomangled, "#endif\n");
            fclose(nomangled);
        }
    }

    void ConvertC::writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<Variable>& globals, TPResult result) {
        (void) warns;
        int scopeDepth = 0;
        append("/* EXTERN VARS FROM INTERNAL */\n");
        append("extern scl_stack_t stack;\n");
        append("extern scl_stack_t callstk;\n\n");

        append("/* LOOP STRUCTS */\n");
        append("struct scl_iterable {\n");
        scopeDepth++;
        append("scl_int start;\n");
        append("scl_int end;\n");
        scopeDepth--;
        append("};\n\n");

        append("/* FUNCTIONS */\n");

        int funcsSize = result.functions.size();
        for (int f = 0; f < funcsSize; f++)
        {
            Function* function = result.functions[f];
            vars.clear();
            std::vector<Variable> defaultScope;
            vars.push_back(defaultScope);
            for (Variable g : globals) {
                vars[0].push_back(g);
            }

            scopeDepth = 0;
            bool noWarns = false;
            bool noMangle = false;

            for (Modifier modifier : function->getModifiers()) {
                if (modifier == mod_nowarn) {
                    noWarns = true;
                } else if (modifier == mod_nomangle) {
                    noMangle = true;
                }
            }

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
                err.column = function->getNameToken().getColumn();
                err.line = function->getNameToken().getLine();
                err.in = function->getNameToken().getFile();
                err.value = function->getNameToken().getValue();
                err.type = function->getNameToken().getType();
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
            }

            if (noMangle && !function->isMethod) {
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
                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                    } else {
                        append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", function->getArgs()[i].getName().c_str());
                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                    }
                }
                if (return_type != "void") {
                    append("return Function_%s();\n", function->getName().c_str());
                } else {
                    append("Function_%s();\n", function->getName().c_str());
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
                append("%s Method_%s_%s(struct Struct_%s* Var_self) {\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), ((Method*)(function))->getMemberType().c_str());
            }
            
            scopeDepth++;

            append("callstk.data[callstk.ptr++].v = \"%s\";\n", functionDeclaration.c_str());

            if (function->isMethod) {
                append("if (stack.ptr < %zu)\n", function->getArgs().size() - 1);
            } else {
                append("if (stack.ptr < %zu)\n", function->getArgs().size());
            }
            scopeDepth++;
            append("scl_security_throw(EX_INVALID_ARGUMENT, \"Not enough data on the stack!\");\n");
            scopeDepth--;


            for (ssize_t i = (ssize_t) function->getArgs().size() - 1; i >= 0; i--) {
                Variable var = function->getArgs()[i];
                vars[varDepth].push_back(var);
                if (function->isMethod && var.getName() == "self") continue;
                std::string ret_type = sclReturnTypeToCReturnType(result, var.getType());
                if (function->getArgs()[i].getType() == "float") {
                    append("%s Var_%s = stack.data[--stack.ptr].f;\n", ret_type.c_str(), var.getName().c_str());
                } else {
                    append("%s Var_%s = (%s) stack.data[--stack.ptr].v;\n", ret_type.c_str(), var.getName().c_str(), ret_type.c_str());
                }
            }

            std::vector<Token> body = function->getBody();
            std::vector<bool> was_rep;
            size_t i;
            char repeat_depth = 0;
            int iterator_count = 0;
            for (i = 0; i < body.size(); i++) {
                if (body[i].getType() == tok_ignore) continue;

                std::string file = body[i].getFile();

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
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            } else if (body[i].getValue() == "swap") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_value b = stack.data[--stack.ptr].v;\n");
                                append("scl_value a = stack.data[--stack.ptr].v;\n");
                                append("stack.data[stack.ptr++].v = b;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                append("stack.data[stack.ptr++].v = a;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "over") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_value c = stack.data[--stack.ptr].v;\n");
                                append("scl_value b = stack.data[--stack.ptr].v;\n");
                                append("scl_value a = stack.data[--stack.ptr].v;\n");
                                append("stack.data[stack.ptr++].v = c;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                append("stack.data[stack.ptr++].v = b;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                append("stack.data[stack.ptr++].v = a;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "sdup2") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_value b = stack.data[--stack.ptr].v;\n");
                                append("scl_value a = stack.data[--stack.ptr].v;\n");
                                append("stack.data[stack.ptr++].v = a;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                append("stack.data[stack.ptr++].v = b;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                append("stack.data[stack.ptr++].v = a;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "swap2") {
                                append("{\n");
                                scopeDepth++;
                                append("scl_value c = stack.data[--stack.ptr].v;\n");
                                append("scl_value b = stack.data[--stack.ptr].v;\n");
                                append("scl_value a = stack.data[--stack.ptr].v;\n");
                                append("stack.data[stack.ptr++].v = b;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                append("stack.data[stack.ptr++].v = a;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                append("stack.data[stack.ptr++].v = c;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                scopeDepth--;
                                append("}\n");
                            } else if (body[i].getValue() == "clearstack") {
                                append("stack.ptr = 0;\n");
                            } else if (body[i].getValue() == "&&") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i && stack.data[stack.ptr + 1].i;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            } else if (body[i].getValue() == "!") {
                                append("stack.data[stack.ptr - 1].i = !stack.data[stack.ptr - 1].i;\n");
                            } else if (body[i].getValue() == "||") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i || stack.data[stack.ptr + 1].i;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            } else if (body[i].getValue() == "<") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i < stack.data[stack.ptr + 1].i;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            } else if (body[i].getValue() == ">") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i > stack.data[stack.ptr + 1].i;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            } else if (body[i].getValue() == "==") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i == stack.data[stack.ptr + 1].i;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            } else if (body[i].getValue() == "<=") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i <= stack.data[stack.ptr + 1].i;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            } else if (body[i].getValue() == ">=") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i >= stack.data[stack.ptr + 1].i;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            } else if (body[i].getValue() == "!=") {
                                append("stack.ptr -= 2;\n");
                                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i != stack.data[stack.ptr + 1].i;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
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
                                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                    } else {
                                        append("stack.data[stack.ptr++].v = (scl_value) Function_%s();\n", f->getName().c_str());
                                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                    }
                                } else {
                                    append("Function_%s();\n", f->getName().c_str());
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
                                if (container.getMemberType(memberName) == "float") {
                                    append("stack.data[stack.ptr++].f = Container_%s.%s;\n", containerName.c_str(), memberName.c_str());
                                    if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                } else {
                                    append("stack.data[stack.ptr++].v = (scl_value) Container_%s.%s;\n", containerName.c_str(), memberName.c_str());
                                    if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                }
                            } else if (getStructByName(result, body[i].getValue()) != Struct("")) {
                                if (body[i + 1].getType() == tok_column) {
                                    ITER_INC;
                                    ITER_INC;
                                    if (body[i].getType() == tok_new) {
                                        std::string struct_ = body[i - 2].getValue();
                                        append("stack.data[stack.ptr++].v = scl_alloc_struct(sizeof(struct Struct_%s), \"%s\");\n", struct_.c_str(), struct_.c_str());
                                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                        if (hasMethod(result, Token(tok_identifier, "init", 0, "", 0), struct_)) {
                                            append("{\n");
                                            scopeDepth++;
                                            Method* f = getMethodByName(result, "init", struct_);
                                            append("struct Struct_%s* tmp = (struct Struct_%s*) stack.data[--stack.ptr].v;\n", ((Method*)(f))->getMemberType().c_str(), ((Method*)(f))->getMemberType().c_str());
                                            if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                                                if (f->getReturnType() == "float") {
                                                    append("stack.data[stack.ptr++].f = Method_%s_%s(tmp);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                                    if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                                } else {
                                                    append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(tmp);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                                    if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                                }
                                            } else {
                                                append("Method_%s_%s(tmp);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                            }
                                            append("stack.data[stack.ptr++].v = tmp;\n");
                                            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
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
                                                append("stack.data[stack.ptr++].f = Method_%s_%s((struct Struct_%s*) stack.data[--stack.ptr].v);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), ((Method*)(f))->getMemberType().c_str());
                                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                            } else {
                                                append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s((struct Struct_%s*) stack.data[--stack.ptr].v);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), ((Method*)(f))->getMemberType().c_str());
                                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                            }
                                        } else {
                                            append("Method_%s_%s((struct Struct_%s*) stack.data[--stack.ptr].v);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), ((Method*)(f))->getMemberType().c_str());
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
                                        if (Main.options.debugBuild) {
                                            // appendfprintf(stderr, \"Pushing: %lld\\n\",= (scl_value) Var_%s;\n", loadFrom.c_str());
                                        }
                                        // append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());
                                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                                            if (f->getReturnType() == "float") {
                                                append("stack.data[stack.ptr++].f = Method_%s_%s(Var_%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), loadFrom.c_str());
                                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                            } else {
                                                append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(Var_%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), loadFrom.c_str());
                                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                            }
                                        } else {
                                            append("Method_%s_%s(Var_%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), loadFrom.c_str());
                                        }
                                    } else {
                                        if (body[i + 1].getType() != tok_dot) {
                                            append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());
                                            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
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
                                        if (mem.getType() == "float") {
                                            append("stack.data[stack.ptr++].f = Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());
                                            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                        } else {
                                            append("stack.data[stack.ptr++].v = (scl_value) Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());
                                            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                        }
                                    }
                                } else {
                                    if (v.getType() == "float") {
                                        append("stack.data[stack.ptr++].f = Var_%s;\n", loadFrom.c_str());
                                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                    } else {
                                        append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());
                                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                    }
                                }
                            } else {
                                transpilerError("Unknown identifier: '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                            }
                            break;
                        }

                        case tok_string_literal: {
                            append("stack.data[stack.ptr++].v = \"%s\";\n", body[i].getValue().c_str());
                            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: '%%s'\\n\", stack.data[stack.ptr - 1].s);\n");
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
                            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            break;
                        }

                        case tok_true: {
                            append("stack.data[stack.ptr++].v = (scl_value) 1;\n");
                            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            break;
                        }

                        case tok_new: {
                            {
                                transpilerError("Using 'new <type>' is deprecated! Use '<type>:new' instead.", i);
                                { if (!noWarns) warns.push_back(err); }
                            }
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
                            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            if (hasMethod(result, Token(tok_identifier, "init", 0, "", 0), struct_)) {
                                append("{\n");
                                scopeDepth++;
                                Method* f = getMethodByName(result, "init", struct_);
                                append("struct Struct_%s* tmp = (struct Struct_%s*) stack.data[--stack.ptr].v;\n", ((Method*)(f))->getMemberType().c_str(), ((Method*)(f))->getMemberType().c_str());
                                if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                                    if (f->getReturnType() == "float") {
                                        append("stack.data[stack.ptr++].f = Method_%s_%s(tmp);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                    } else {
                                        append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(tmp);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                    }
                                } else {
                                    append("Method_%s_%s(tmp);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                                }
                                append("stack.data[stack.ptr++].v = tmp;\n");
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
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
                            varDepth++;
                            std::vector<Variable> defaultScope;
                            vars.push_back(defaultScope);
                            break;
                        }

                        case tok_else: {
                            scopeDepth--;
                            varDepth--;
                            vars.pop_back();
                            append("} else {\n");
                            scopeDepth++;
                            varDepth++;
                            std::vector<Variable> defaultScope;
                            vars.push_back(defaultScope);
                            break;
                        }

                        case tok_while: {
                            append("while (1) {\n");
                            scopeDepth++;
                            was_rep.push_back(false);
                            varDepth++;
                            std::vector<Variable> defaultScope;
                            vars.push_back(defaultScope);
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
                            varDepth++;
                            std::vector<Variable> defaultScope;
                            vars.push_back(defaultScope);
                            was_rep.push_back(true);
                            i += 2;
                            break;
                        }

                        case tok_for: {
                            ITER_INC;
                            Token var = body[i];
                            if (var.getType() != tok_identifier) {
                                FPResult result;
                                result.message = "Expected identifier, but got: '" + var.getValue() + "'";
                                result.success = false;
                                result.line = var.getLine();
                                result.in = var.getFile();
                                result.value = var.getValue();
                                result.column = var.getColumn();
                                result.type = var.getType();
                                errors.push_back(result);
                            }
                            ITER_INC;
                            if (body[i].getType() != tok_in) {
                                FPResult result;
                                result.message = "Expected 'in' keyword in for loop header, but got: '" + body[i].getValue() + "'";
                                result.success = false;
                                result.line = body[i].getLine();
                                result.in = body[i].getFile();
                                result.value = body[i].getValue();
                                result.column = body[i].getColumn();
                                result.type = body[i].getType();
                                errors.push_back(result);
                            }
                            ITER_INC;
                            push_var();
                            ITER_INC;
                            if (body[i].getType() == tok_addr_of) {
                                append("stack.data[stack.ptr - 1].v = (*(scl_value*) stack.data[stack.ptr - 1].v);\n");
                                ITER_INC;
                            }
                            if (body[i].getType() != tok_to) {
                                transpilerError("Expected 'to', but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            ITER_INC;
                            push_var();
                            ITER_INC;
                            if (body[i].getType() == tok_addr_of) {
                                append("stack.data[stack.ptr - 1].v = (*(scl_value*) stack.data[stack.ptr - 1].v);\n");
                                ITER_INC;
                            }
                            std::string iterator_direction = "++";
                            if (body[i].getType() == tok_step) {
                                ITER_INC;
                                if (body[i].getType() == tok_do) {
                                    transpilerError("Expected step, but got '" + body[i].getValue() + "'", i);
                                    errors.push_back(err);
                                    continue;
                                }
                                std::string val = body[i].getValue();
                                if (val == "+") {
                                    ITER_INC;
                                    iterator_direction = " += ";
                                    if (body[i].getType() == tok_number) {
                                        iterator_direction += body[i].getValue();
                                    } else if (body[i].getValue() == "+") {
                                        iterator_direction = "++";
                                    } else {
                                        transpilerError("Expected number or '+', but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                    }
                                } else if (val == "-") {
                                    ITER_INC;
                                    iterator_direction = " -= ";
                                    if (body[i].getType() == tok_number) {
                                        iterator_direction += body[i].getValue();
                                    } else if (body[i].getValue() == "+") {
                                        iterator_direction = "--";
                                    } else {
                                        transpilerError("Expected number, but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                    }
                                } else if (val == "*") {
                                    ITER_INC;
                                    iterator_direction = " *= ";
                                    if (body[i].getType() == tok_number) {
                                        iterator_direction += body[i].getValue();
                                    } else {
                                        transpilerError("Expected number, but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                    }
                                } else if (val == "/") {
                                    ITER_INC;
                                    iterator_direction = " /= ";
                                    if (body[i].getType() == tok_number) {
                                        iterator_direction += body[i].getValue();
                                    } else {
                                        transpilerError("Expected number, but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                    }
                                } else if (val == "<<") {
                                    ITER_INC;
                                    iterator_direction = " <<= ";
                                    if (body[i].getType() == tok_number) {
                                        iterator_direction += body[i].getValue();
                                    } else {
                                        transpilerError("Expected number, but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                    }
                                } else if (val == ">>") {
                                    ITER_INC;
                                    iterator_direction = " >>= ";
                                    if (body[i].getType() == tok_number) {
                                        iterator_direction += body[i].getValue();
                                    } else {
                                        transpilerError("Expected number, but got '" + body[i].getValue() + "'", i);
                                        errors.push_back(err);
                                    }
                                } else if (val == "nop") {
                                    iterator_direction = "";
                                }
                                ITER_INC;
                            }
                            if (body[i].getType() != tok_do) {
                                transpilerError("Expected 'do', but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            append("struct scl_iterable __it%d = (struct scl_iterable) {0, 0};\n", iterator_count);
                            append("__it%d.end = stack.data[--stack.ptr].i;\n", iterator_count);
                            append("__it%d.start = stack.data[--stack.ptr].i;\n", iterator_count);
                            
                            std::string var_prefix = "";
                            if (!hasVar(var)) {
                                var_prefix = "scl_int ";
                            }
                            varDepth++;
                            std::vector<Variable> defaultScope;
                            vars.push_back(defaultScope);
                            
                            if (iterator_direction == "")
                                append("for (%sVar_%s = __it%d.start; Var_%s != __it%d.end;) {\n", var_prefix.c_str(), var.getValue().c_str(), iterator_count, var.getValue().c_str(), iterator_count);
                            else
                                append("for (%sVar_%s = __it%d.start; Var_%s != __it%d.end; Var_%s%s) {\n", var_prefix.c_str(), var.getValue().c_str(), iterator_count, var.getValue().c_str(), iterator_count, var.getValue().c_str(), iterator_direction.c_str());
                            iterator_count++;
                            scopeDepth++;

                            if (hasFunction(Main.parser->getResult(), var)) {
                                FPResult result;
                                result.message = "Variable '" + var.getValue() + "' shadowed by function '" + var.getValue() + "'";
                                result.success = false;
                                result.line = var.getLine();
                                result.in = var.getFile();
                                result.value = var.getValue();
                                result.type =  var.getType();
                                result.column = var.getColumn();
                                warns.push_back(result);
                            }
                            if (hasContainer(Main.parser->getResult(), var)) {
                                FPResult result;
                                result.message = "Variable '" + var.getValue() + "' shadowed by container '" + var.getValue() + "'";
                                result.success = false;
                                result.line = var.getLine();
                                result.in = var.getFile();
                                result.value = var.getValue();
                                result.type =  var.getType();
                                result.column = var.getColumn();
                                warns.push_back(result);
                            }
                            if (hasVar(var)) {
                                FPResult result;
                                result.message = "Variable '" + var.getValue() + "' is already declared and shadows it.";
                                result.success = false;
                                result.line = var.getLine();
                                result.in = var.getFile();
                                result.value = var.getValue();
                                result.type =  var.getType();
                                result.column = var.getColumn();
                                warns.push_back(result);
                            }

                            if (!hasVar(var))
                                vars[varDepth].push_back(Variable(var.getValue(), "int"));
                            was_rep.push_back(false);
                            break;
                        }

                        case tok_foreach: {
                            ITER_INC;
                            if (body[i].getType() != tok_identifier) {
                                transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            std::string iterator_name = "__it" + std::to_string(iterator_count++);
                            Token iter_var_tok = body[i];
                            ITER_INC;
                            if (body[i].getType() != tok_in) {
                                transpilerError("Expected 'in', but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                continue;
                            }
                            ITER_INC;
                            Token iterable_tok = body[i];
                            if (!hasVar(iterable_tok)) {
                                transpilerError("Variable '" + body[i].getValue() + "' not found!", i);
                                errors.push_back(err);
                                continue;
                            }

                            Variable iterable = getVar(iterable_tok);

                            if (getStructByName(result, iterable.getType() + "Iterator") == Struct("")) {
                                transpilerError("Struct '" + iterable.getType() + "' is not iterable!", i);
                                errors.push_back(err);
                                continue;
                            }

                            append("struct Struct_%sIterator %s = (struct Struct_%sIterator) {0};\n", iterable.getType().c_str(), iterator_name.c_str(), iterable.getType().c_str());
                            if (!hasMethod(result, Token(tok_identifier, "init", 0, "", 0), iterable.getType() + "Iterator")) {
                                transpilerError("Iterator for '" + iterable.getType() + "' has no 'init' method!", i);
                                errors.push_back(err);
                                continue;
                            }
                            if (!hasMethod(result, Token(tok_identifier, "has_next", 0, "", 0), iterable.getType() + "Iterator")) {
                                transpilerError("Iterator for '" + iterable.getType() + "' has no 'has_next' method!", i);
                                errors.push_back(err);
                                continue;
                            }
                            if (!hasMethod(result, Token(tok_identifier, "next", 0, "", 0), iterable.getType() + "Iterator")) {
                                transpilerError("Iterator for '" + iterable.getType() + "' has no 'next' method!", i);
                                errors.push_back(err);
                                continue;
                            }
                            if (!hasMethod(result, Token(tok_identifier, "begin", 0, "", 0), iterable.getType() + "Iterator")) {
                                transpilerError("Iterator for '" + iterable.getType() + "' has no 'begin' method!", i);
                                errors.push_back(err);
                                continue;
                            }
                            append("stack.data[stack.ptr++].v = Var_%s;\n", iterable_tok.getValue().c_str());
                            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                            append("Method_%sIterator_init(&%s);\n", iterable.getType().c_str(), iterator_name.c_str());
                            std::string var_prefix = "";
                            if (!hasVar(iter_var_tok)) {
                                var_prefix = "scl_value ";
                            }
                            varDepth++;
                            std::vector<Variable> defaultScope;
                            vars.push_back(defaultScope);
                            vars[varDepth].push_back(Variable(iter_var_tok.getValue(), "any"));
                            std::string iter_type = iterable.getType();
                            std::string type = sclReturnTypeToCReturnType(result, getVar(iter_var_tok).getType());
                            append("for (%sVar_%s = (%s) Method_%sIterator_begin(&%s); Method_%sIterator_has_next(&%s); Var_%s = (%s) Method_%sIterator_next(&%s)) {\n", var_prefix.c_str(), iter_var_tok.getValue().c_str(), type.c_str(), iter_type.c_str(), iterator_name.c_str(), iter_type.c_str(), iterator_name.c_str(), iter_var_tok.getValue().c_str(), type.c_str(), iter_type.c_str(), iterator_name.c_str());
                            scopeDepth++;
                            ITER_INC;
                            break;
                        }

                        case tok_done:
                        case tok_fi:
                        case tok_end: {
                            scopeDepth--;
                            varDepth--;
                            vars.pop_back();
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
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
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
                                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                    } else {
                                        if (body[i + 1].getType() != tok_dot) {
                                            append("stack.data[stack.ptr++].v = (scl_value) &Var_%s;\n", loadFrom.c_str());
                                            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
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
                                        if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
                                    }
                                } else {
                                    append("stack.data[stack.ptr++].v = (scl_value) &Var_%s;\n", loadFrom.c_str());
                                    if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
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
                                if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
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
                                    append("Container_%s.%s = (%s) stack.data[--stack.ptr].%s;\n", containerName.c_str(), memberName.c_str(), sclReturnTypeToCReturnType(result, container.getMemberType(memberName)).c_str(), container.getMemberType(memberName) == "float" ? "f" : "v");
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
                                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                                else errors.push_back(err);
                            }
                            if (hasContainer(result, body[i + 1])) {
                                transpilerError("Variable '" + body[i + 1].getValue() + "' shadowed by container '" + body[i + 1].getValue() + "'", i+1);
                                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                                else errors.push_back(err);
                            }
                            if (hasVar(body[i + 1])) {
                                transpilerError("Variable '" + body[i + 1].getValue() + "' is already declared and shadows it.", i+1);
                                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
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
                            vars[varDepth].push_back(v);
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

                        case tok_case: {
                            ITER_INC;
                            if (body[i].getType() == tok_string_literal) {
                                transpilerError("String literal in case statement detected!", i);
                                errors.push_back(err);
                            } else {
                                append("case %s: {\n", body[i].getValue().c_str());
                                scopeDepth++;
                                varDepth++;
                                std::vector<Variable> defaultScope;
                                vars.push_back(defaultScope);
                            }
                            break;
                        }

                        case tok_esac: {
                            append("break;\n");
                            scopeDepth--;
                            varDepth--;
                            vars.pop_back();
                            append("}\n");
                            break;
                        }

                        case tok_default: {
                            append("default: {\n");
                            scopeDepth++;
                            varDepth++;
                            std::vector<Variable> defaultScope;
                            vars.push_back(defaultScope);
                            break;
                        }

                        case tok_switch: {
                            append("switch (stack.data[--stack.ptr].i) {\n");
                            scopeDepth++;
                            varDepth++;
                            std::vector<Variable> defaultScope;
                            vars.push_back(defaultScope);
                            was_rep.push_back(false);
                            break;
                        }

                        case tok_addr_of: {
                            append("stack.data[stack.ptr - 1].v = (*(scl_value*) stack.data[stack.ptr - 1].v);\n");
                            break;
                        }

                        case tok_curly_close: {
                            append("stack.ptr -= 2; stack.data[stack.ptr++].v =(*(scl_value*) (stack.data[stack.ptr].v + (stack.data[stack.ptr + 1].i * 8)));\n");
                            if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
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
            if (body.size() == 0 || body[body.size() - 1].getType() != tok_return)
                append("callstk.ptr--;\n");
            scopeDepth = 0;
            append("}\n\n");
        }
    }
} // namespace sclc
