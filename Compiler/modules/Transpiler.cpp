#include <filesystem>

#include "../headers/Common.hpp"
#include "../headers/TranspilerDefs.hpp"

#define ITER_INC do { i++; if (i >= body.size()) { FPResult err; err.success = false; err.message = "Unexpected end of function!"; err.column = body[i - 1].getColumn(); err.line = body[i - 1].getLine(); err.in = body[i - 1].getFile(); err.value = body[i - 1].getValue(); err.type =  body[i - 1].getType(); errors.push_back(err); return; } } while (0)

namespace sclc {
    FILE* nomangled;
    FILE* symbolTable;

    void ConvertC::writeHeader(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
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

    bool hasTypealias(TPResult r, std::string t) {
        using spair = std::pair<std::string, std::string>;
        for (spair p : r.typealiases) {
            if (p.first == t) {
                return true;
            }
        }
        return false;
    }

    std::string getTypealias(TPResult r, std::string t) {
        using spair = std::pair<std::string, std::string>;
        for (spair p : r.typealiases) {
            if (p.first == t) {
                return p.second;
            }
        }
        return "";
    }

    std::string sclReturnTypeToCReturnType(TPResult result, std::string t) {
        std::string return_type = "scl_any";
        if (t == "any") return_type = "scl_any";
        else if (t == "none") return_type = "void";
        else if (t == "int") return_type = "scl_int";
        else if (t == "float") return_type = "scl_float";
        else if (t == "str") return_type = "scl_str";
        else if (t == "bool") return_type = "scl_int";
        else if (!(getStructByName(result, t) == Struct(""))) {
            return_type = "scl_" + getStructByName(result, t).getName();
        } else if (t.at(0) == '[') {
            std::string type = sclReturnTypeToCReturnType(result, t.substr(1, t.length() - 2));
            return type + "*";
        } else if (hasTypealias(result, t)) {
            return_type = getTypealias(result, t);
        }
        return return_type;
    }

    std::string sclGenArgs(TPResult result, Function *func) {
        std::string args = "";
        for (ssize_t i = (ssize_t) func->getArgs().size() - 1; i >= 0; i--) {
            Variable arg = func->getArgs()[i];
            if (arg.getType() == "float") {
                if (i) {
                    args += "stack.data[stack.ptr + " + std::to_string(i) + "].f";
                } else {
                    args += "stack.data[stack.ptr].f";
                }
            } else if (arg.getType() == "int" || arg.getType() == "bool") {
                if (i) {
                    args += "stack.data[stack.ptr + " + std::to_string(i) + "].i";
                } else {
                    args += "stack.data[stack.ptr].i";
                }
            } else if (arg.getType() == "str") {
                if (i) {
                    args += "stack.data[stack.ptr + " + std::to_string(i) + "].s";
                } else {
                    args += "stack.data[stack.ptr].s";
                }
            } else {
                if (i) {
                    args += "(" + sclReturnTypeToCReturnType(result, arg.getType()) + ") stack.data[stack.ptr + " + std::to_string(i) + "].v";
                } else {
                    args += "(" + sclReturnTypeToCReturnType(result, arg.getType()) + ") stack.data[stack.ptr].v";
                }
            }
            if (i) args += ", ";
        }
        return args;
    }

    std::string typeToASCII(std::string v) {
        if (v.at(0) == '[') {
            return "_sIptrType_" + typeToASCII(v.substr(1, v.length() - 2));
        }
        if (v == "?") {
            return "_sIwildcard";
        }
        return v;
    }

    std::string generateSymbolForFunction(Function* f) {
        std::string symbol = "fnct_" + f->getName();
        if (f->isMethod) {
            symbol = "mthd_" + static_cast<Method*>(f)->getMemberType() + "_" + symbol;
        }
        std::string arguments = "";
        if (f->getArgs().size() > 0) {
            for (size_t i = 0; i < f->getArgs().size(); i++) {
                if (f->isMethod) {
                    if (f->getArgs()[i].getName() == "self") {
                        continue;
                    }
                }
                if (i) {
                    arguments += "_";
                }
                arguments += typeToASCII(f->getArgs()[i].getType());
            }
        }
        symbol += "_sIargs_" + arguments + "_sItype_" + typeToASCII(f->getReturnType());

        if (Main.options.transpileOnly && symbolTable) {
            if (f->isMethod) {
                fprintf(symbolTable, "Symbol for Method %s:%s: '%s'\n", static_cast<Method*>(f)->getMemberType().c_str(), f->getName().c_str(), symbol.c_str());
            } else {
                fprintf(symbolTable, "Symbol for Function %s: '%s'\n", f->getName().c_str(), symbol.c_str());
            }
        }

        return symbol;
    }

    void ConvertC::writeFunctionHeaders(FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        append("/* FUNCTION HEADERS */\n");

        if (Main.options.transpileOnly) {
            remove("scale_support.h");
            remove("scale-symbol-table.txt");
            symbolTable = fopen("scale-symbol-table.txt", "a");
            nomangled = fopen("scale_support.h", "a");
            fprintf(nomangled, "#include <scale_internal.h>\n\n");

            fprintf(nomangled, "#define scl_export(func_name) \\\n");
            fprintf(nomangled, "    void func_name (void); \\\n");
            fprintf(nomangled, "    void Function_ ## func_name (void) __asm(\"fnct_\" #func_name \"_sIargs__sItype_none\"); \\\n");
            fprintf(nomangled, "    void Function_ ## func_name () { func_name (); } \\\n");
            fprintf(nomangled, "    void func_name (void)\n\n");
            fprintf(nomangled, "#define ssize_t signed long\n");
            fprintf(nomangled, "typedef void* scl_any;\n");
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
            
            std::string arguments = "";
            if (function->getArgs().size() > 0) {
                for (ssize_t i = (ssize_t) function->getArgs().size() - 1; i >= 0; i--) {
                    arguments += sclReturnTypeToCReturnType(result, function->getArgs()[i].getType()) + " Var_" + function->getArgs()[i].getName();
                    if (i) {
                        arguments += ", ";
                    }
                }
            }

            std::string symbol = generateSymbolForFunction(function);

            if (!function->isMethod) {
                append("%s Function_%s(%s) __asm(\"%s\");\n", return_type.c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
            } else {
                append("%s Method_%s_%s(%s) __asm(\"%s\");\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
            }
            if (!Main.options.transpileOnly) continue;
            for (std::string m : function->getModifiers()) {
                if (m == "nomangle") {
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

    void ConvertC::writeExternHeaders(FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
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

                std::string arguments = "";
                if (function->getArgs().size() > 0) {
                    for (ssize_t i = (ssize_t) function->getArgs().size() - 1; i >= 0; i--) {
                        arguments += sclReturnTypeToCReturnType(result, function->getArgs()[i].getType()) + " Var_" + function->getArgs()[i].getName();
                        if (i) {
                            arguments += ", ";
                        }
                    }
                }
                std::string symbol = generateSymbolForFunction(function);

                if (!function->isMethod) {
                    append("%s Function_%s(%s) __asm(\"%s\");\n", return_type.c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                } else {
                    append("%s Method_%s_%s(%s) __asm(\"%s\");\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                }
            }
        }

        if (symbolTable) {
            fclose(symbolTable);
        }

        append("\n");
    }

    void ConvertC::writeGlobals(FILE* fp, std::vector<Variable>& globals, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
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

    void ConvertC::writeContainers(FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
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

    bool argsAreIdentical(std::vector<Variable> methodArgs, std::vector<Variable> functionArgs) {
        if ((methodArgs.size() - 1) != functionArgs.size()) return false;
        for (size_t i = 0; i < methodArgs.size(); i++) {
            if (methodArgs[i].getName() == "self") continue;
            if (methodArgs[i] != functionArgs[i]) return false;
        }
        return true;
    }

    void ConvertC::writeStructs(FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        if (result.structs.size() == 0) goto label_writeStructs_close_file;
        append("/* STRUCT TYPES */\n");
        for (Struct c : result.structs) {
            append("typedef struct Struct_%s* scl_%s;\n", c.getName().c_str(), c.getName().c_str());
        }
        append("\n");
        append("/* STRUCT DEFINITIONS */\n");
        for (Struct c : result.structs) {
            for (std::string i : c.getInterfaces()) {
                Interface* interface = getInterfaceByName(result, i);
                if (interface == nullptr) {
                    FPResult res;
                    Token t = c.nameToken();
                    res.success = false;
                    res.message = "Struct '" + c.getName() + "' implements unknown interface '" + i + "'";
                    res.column = t.getColumn();
                    res.line = t.getLine();
                    res.in = t.getFile();
                    res.value = t.getValue();
                    res.type = t.getType();
                    errors.push_back(res);
                    continue;
                }
                for (Function* f : interface->getImplements()) {
                    if (!hasMethod(result, Token(tok_identifier, f->getName(), 0, "", 0), c.getName())) {
                        FPResult res;
                        Token t = c.nameToken();
                        res.success = false;
                        res.message = "No implementation for method '" + f->getName() + "' on struct '" + c.getName() + "'";
                        res.column = t.getColumn();
                        res.line = t.getLine();
                        res.in = t.getFile();
                        res.value = t.getValue();
                        res.type = t.getType();
                        errors.push_back(res);
                        continue;
                    }
                    Method* m = getMethodByName(result, f->getName(), c.getName());
                    if (!argsAreIdentical(m->getArgs(), f->getArgs())) {
                        FPResult res;
                        Token t = m->getNameToken();
                        res.success = false;
                        res.message = "Arguments of method '" + c.getName() + ":" + m->getName() + "' do not match implemented method '" + f->getName() + "'";
                        res.column = t.getColumn();
                        res.line = t.getLine();
                        res.in = t.getFile();
                        res.value = t.getValue();
                        res.type = t.getType();
                        errors.push_back(res);
                        continue;
                    }
                    if (f->getReturnType() != "?" && m->getReturnType() != f->getReturnType()) {
                        FPResult res;
                        Token t = m->getNameToken();
                        res.success = false;
                        res.message = "Return type of method '" + c.getName() + ":" + m->getName() + "' does not match implemented method '" + f->getName() + "'. Return type should be: '" + f->getReturnType() + "'";
                        res.column = t.getColumn();
                        res.line = t.getLine();
                        res.in = t.getFile();
                        res.value = t.getValue();
                        res.type = t.getType();
                        errors.push_back(res);
                        continue;
                    }
                }
            }

            append("struct Struct_%s {\n", c.getName().c_str());
            append("  scl_int $__type__;\n");
            append("  scl_str $__type_name__;\n");
            append("  scl_any $__lock__;\n");
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

    int scopeDepth = 0;
    size_t i = 0;
    std::vector<bool> was_rep;
    char repeat_depth = 0;
    int iterator_count = 0;
    bool noWarns;
    bool noMangle;
    std::string lastPushedType = "";
    std::string return_type = "";

#define handler(_tok) extern "C" void handle ## _tok (std::vector<Token>& body, Function* function, std::vector<FPResult>& errors, std::vector<FPResult>& warns, FILE* fp, TPResult result)
#define handle(_tok) handle ## _tok (body, function, errors, warns, fp, result)
#define debugDump(_var) std::cout << #_var << ": " << _var << std::endl;
#define noUnused \
        (void) body; \
        (void) function; \
        (void) errors; \
        (void) warns; \
        (void) fp; \
        (void) result

    handler(Identifier);
    handler(New);
    handler(Repeat);
    handler(For);
    handler(Foreach);
    handler(AddrRef);
    handler(Store);
    handler(Declare);
    handler(CurlyOpen);
    handler(BracketOpen);
    handler(ParenOpen);
    handler(StringLiteral);
    handler(CDecl);
    handler(IntegerLiteral);
    handler(FloatLiteral);
    handler(FalsyType);
    handler(TruthyType);
    handler(Is);
    handler(If);
    handler(Else);
    handler(While);
    handler(Do);
    handler(DoneLike);
    handler(Return);
    handler(Goto);
    handler(Label);
    handler(Case);
    handler(Esac);
    handler(Default);
    handler(Switch);
    handler(AddrOf);
    handler(Break);
    handler(Continue);
    handler(As);
    handler(Dot);
    handler(Column);
    handler(Token);

    handler(Identifier) {
        noUnused;
        if (body[i].getValue() == "self" && !function->isMethod) {
            transpilerError("Can't use 'self' outside a member function.", i);
            errors.push_back(err);
            return;
        }
    #ifdef __APPLE__
    #pragma region Identifier functions
    #endif
        if (body[i].getValue() == "drop") {
            append("stack.ptr--;\n");
            lastPushedType = "";
        } else if (body[i].getValue() == "dup") {
            append("stack.data[stack.ptr++].v = stack.data[stack.ptr - 1].v;\n");
            debugPrintPush();
        } else if (body[i].getValue() == "swap") {
            append("{\n");
            scopeDepth++;
            append("scl_any b = stack.data[--stack.ptr].v;\n");
            append("scl_any a = stack.data[--stack.ptr].v;\n");
            append("stack.data[stack.ptr++].v = b;\n");
            debugPrintPush();
            append("stack.data[stack.ptr++].v = a;\n");
            debugPrintPush();
            scopeDepth--;
            append("}\n");
            lastPushedType = "any";
        } else if (body[i].getValue() == "over") {
            append("{\n");
            scopeDepth++;
            append("scl_any c = stack.data[--stack.ptr].v;\n");
            append("scl_any b = stack.data[--stack.ptr].v;\n");
            append("scl_any a = stack.data[--stack.ptr].v;\n");
            append("stack.data[stack.ptr++].v = c;\n");
            debugPrintPush();
            append("stack.data[stack.ptr++].v = b;\n");
            debugPrintPush();
            append("stack.data[stack.ptr++].v = a;\n");
            debugPrintPush();
            scopeDepth--;
            append("}\n");
            lastPushedType = "any";
        } else if (body[i].getValue() == "sdup2") {
            append("{\n");
            scopeDepth++;
            append("scl_any b = stack.data[--stack.ptr].v;\n");
            append("scl_any a = stack.data[--stack.ptr].v;\n");
            append("stack.data[stack.ptr++].v = a;\n");
            debugPrintPush();
            append("stack.data[stack.ptr++].v = b;\n");
            debugPrintPush();
            append("stack.data[stack.ptr++].v = a;\n");
            debugPrintPush();
            scopeDepth--;
            append("}\n");
            lastPushedType = "any";
        } else if (body[i].getValue() == "swap2") {
            append("{\n");
            scopeDepth++;
            append("scl_any c = stack.data[--stack.ptr].v;\n");
            append("scl_any b = stack.data[--stack.ptr].v;\n");
            append("scl_any a = stack.data[--stack.ptr].v;\n");
            append("stack.data[stack.ptr++].v = b;\n");
            debugPrintPush();
            append("stack.data[stack.ptr++].v = a;\n");
            debugPrintPush();
            append("stack.data[stack.ptr++].v = c;\n");
            debugPrintPush();
            scopeDepth--;
            append("}\n");
            lastPushedType = "any";
        } else if (body[i].getValue() == "clearstack") {
            append("stack.ptr = 0;\n");
            lastPushedType = "";
        } else if (body[i].getValue() == "&&") {
            append("stack.ptr -= 2;\n");
            if (i) {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i && stack.data[stack.ptr + 1].i;\n");
            } else {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i && stack-\n");
            }
            debugPrintPush();
            lastPushedType = "bool";
        } else if (body[i].getValue() == "!") {
            append("stack.data[stack.ptr - 1].i = !stack.data[stack.ptr - 1].i;\n");
            lastPushedType = "bool";
        } else if (body[i].getValue() == "||") {
            append("stack.ptr -= 2;\n");
            if (i) {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i || stack.data[stack.ptr + 1].i;\n");
            } else {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i || stack-\n");
            }
            debugPrintPush();
            lastPushedType = "bool";
        } else if (body[i].getValue() == "<") {
            append("stack.ptr -= 2;\n");
            if (i) {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i < stack.data[stack.ptr + 1].i;\n");
            } else {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i < stack-\n");
            }
            debugPrintPush();
            lastPushedType = "bool";
        } else if (body[i].getValue() == ">") {
            append("stack.ptr -= 2;\n");
            if (i) {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i > stack.data[stack.ptr + 1].i;\n");
            } else {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i > stack-\n");
            }
            debugPrintPush();
            lastPushedType = "bool";
        } else if (body[i].getValue() == "==") {
            append("stack.ptr -= 2;\n");
            if (i) {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i == stack.data[stack.ptr + 1].i;\n");
            } else {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i == stack-\n");
            }
            debugPrintPush();
            lastPushedType = "bool";
        } else if (body[i].getValue() == "<=") {
            append("stack.ptr -= 2;\n");
            if (i) {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i <= stack.data[stack.ptr + 1].i;\n");
            } else {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i <= stack-\n");
            }
            debugPrintPush();
            lastPushedType = "bool";
        } else if (body[i].getValue() == ">=") {
            append("stack.ptr -= 2;\n");
            if (i) {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i >= stack.data[stack.ptr + 1].i;\n");
            } else {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i >= stack-\n");
            }
            debugPrintPush();
            lastPushedType = "bool";
        } else if (body[i].getValue() == "!=") {
            append("stack.ptr -= 2;\n");
            if (i) {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i != stack.data[stack.ptr + 1].i;\n");
            } else {
                append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i != stack-\n");
            }
            debugPrintPush();
            lastPushedType = "bool";
        } else if (body[i].getValue() == "++") {
            append("stack.data[stack.ptr - 1].i++;\n");
        } else if (body[i].getValue() == "--") {
            append("stack.data[stack.ptr - 1].i--;\n");
        } else if (body[i].getValue() == "exit") {
            append("exit(stack.data[--stack.ptr].i);\n");
        } else if (body[i].getValue() == "puts") {
            append("puts(stack.data[--stack.ptr].s);\n");
        } else if (body[i].getValue() == "eputs") {
            append("fprintf(stderr, \"%%s\\n\", stack.data[--stack.ptr].s);\n");
        } else if (body[i].getValue() == "abort") {
            append("abort();\n");
    #ifdef __APPLE__
    #pragma endregion
    #endif
        } else if (hasFunction(result, body[i])) {
            Function* f = getFunctionByName(result, body[i].getValue());
            if (f->isMethod) {
                transpilerError("'" + f->getName() + "' is a method, not a function.", i);
                errors.push_back(err);
                return;
            }
            if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
            if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                if (f->getReturnType() == "float") {
                    append("stack.data[stack.ptr++].f = Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                    debugPrintPush();
                } else {
                    append("stack.data[stack.ptr++].v = (scl_any) Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                    debugPrintPush();
                }
            } else {
                append("Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
            }
            lastPushedType = f->getReturnType();
        } else if (hasContainer(result, body[i])) {
            std::string containerName = body[i].getValue();
            ITER_INC;
            if (body[i].getType() != tok_dot) {
                transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            ITER_INC;
            std::string memberName = body[i].getValue();
            Container container = getContainerByName(result, containerName);
            if (!container.hasMember(memberName)) {
                transpilerError("Unknown container member: '" + memberName + "'", i);
                errors.push_back(err);
                return;
            }
            if (container.getMemberType(memberName) == "float") {
                append("stack.data[stack.ptr++].f = Container_%s.%s;\n", containerName.c_str(), memberName.c_str());
                debugPrintPush();
            } else {
                append("stack.data[stack.ptr++].v = (scl_any) Container_%s.%s;\n", containerName.c_str(), memberName.c_str());
                debugPrintPush();
            }
            lastPushedType = container.getMemberType(memberName);
        } else if (getStructByName(result, body[i].getValue()) != Struct("")) {
            if (body[i + 1].getType() == tok_column && body[i + 2].getType() == tok_column) {
                std::string struct_ = body[i].getValue();
                ITER_INC;
                ITER_INC;
                ITER_INC;
                Struct s = getStructByName(result, struct_);
                if (body[i].getValue() == "new") {
                    if (!s.heapAllocAllowed()) {
                        transpilerError("Struct '" + struct_ + "' may not be instanciated using '" + struct_ + "::new'", i);
                        errors.push_back(err);
                        return;
                    }
                    append("stack.data[stack.ptr++].v = scl_alloc_struct(sizeof(struct Struct_%s), \"%s\");\n", struct_.c_str(), struct_.c_str());
                    debugPrintPush();
                    if (hasMethod(result, Token(tok_identifier, "init", 0, "", 0), struct_)) {
                        append("{\n");
                        scopeDepth++;
                        Method* f = getMethodByName(result, "init", struct_);
                        append("struct Struct_%s* tmp = (struct Struct_%s*) stack.data[--stack.ptr].v;\n", ((Method*)(f))->getMemberType().c_str(), ((Method*)(f))->getMemberType().c_str());
                        append("stack.data[stack.ptr++].v = tmp;\n");
                        debugPrintPush();
                        if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
                        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                            if (f->getReturnType() == "float") {
                                append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            } else {
                                append("stack.data[stack.ptr++].v = (scl_any) Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            }
                        } else {
                            append("Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                        }
                        append("stack.data[stack.ptr++].v = tmp;\n");
                        debugPrintPush();
                        scopeDepth--;
                        append("}\n");
                    }
                    lastPushedType = struct_;
                } else if (body[i].getValue() == "default") {
                    if (!s.stackAllocAllowed()) {
                        transpilerError("Struct '" + struct_ + "' may not be instanciated using '" + struct_ + "::default'", i);
                        errors.push_back(err);
                        return;
                    }
                    append("{\n");
                    scopeDepth++;
                    append("struct Struct_%s tmp = {0x%016llx, \"%s\"};\n", struct_.c_str(), hash1((char*) struct_.c_str()), struct_.c_str());
                    append("stack.data[stack.ptr++].v = (scl_any*) &tmp;\n");
                    debugPrintPush();
                    if (hasMethod(result, Token(tok_identifier, "init", 0, "", 0), struct_)) {
                        Method* f = getMethodByName(result, "init", struct_);
                        if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
                        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                            if (f->getReturnType() == "float") {
                                append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            } else {
                                append("stack.data[stack.ptr++].v = (scl_any) Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            }
                        } else {
                            append("Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                        }
                        append("stack.data[stack.ptr++].v = (scl_any*) &tmp;\n");
                        debugPrintPush();
                    }
                    lastPushedType = struct_;
                    scopeDepth--;
                    append("}\n");
                } else {
                    if (hasFunction(result, Token(tok_identifier, struct_ + "$" + body[i].getValue(), 0, "", 0))) {
                        Function* f = getFunctionByName(result, struct_ + "$" + body[i].getValue());
                        if (f->isMethod) {
                            transpilerError("'" + f->getName() + "' is not static!", i);
                            errors.push_back(err);
                            return;
                        }
                        if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
                        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                            if (f->getReturnType() == "float") {
                                append("stack.data[stack.ptr++].f = Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            } else {
                                append("stack.data[stack.ptr++].v = (scl_any) Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            }
                        } else {
                            append("Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                        }
                        lastPushedType = f->getReturnType();
                    } else if (hasGlobal(result, struct_ + "$" + body[i].getValue())) {
                        std::string loadFrom = struct_ + "$" + body[i].getValue();
                        Variable v = getVar(Token(tok_identifier, loadFrom, 0, "", 0));
                        if (v.getType() == "float") {
                            append("stack.data[stack.ptr++].f = Var_%s;\n", loadFrom.c_str());
                            debugPrintPush();
                        } else {
                            append("stack.data[stack.ptr++].v = (scl_any) Var_%s;\n", loadFrom.c_str());
                            debugPrintPush();
                        }
                        lastPushedType = v.getType();
                    }
                }
            }
        } else if (hasVar(body[i])) {
            std::string loadFrom = body[i].getValue();
            Variable v = getVar(body[i]);
            if (v.getType() == "float") {
                append("stack.data[stack.ptr++].f = Var_%s;\n", loadFrom.c_str());
                debugPrintPush();
            } else {
                append("stack.data[stack.ptr++].v = (scl_any) Var_%s;\n", loadFrom.c_str());
                debugPrintPush();
            }
            lastPushedType = v.getType();
        } else if (function->isMethod) {
            Method* m = static_cast<Method*>(function);
            Struct s = getStructByName(result, m->getMemberType());
            if (hasMethod(result, body[i], s.getName())) {
                Method* f = getMethodByName(result, body[i].getValue(), s.getName());
                append("stack.data[stack.ptr++].v = (scl_any) Var_self;\n");
                if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
                debugPrintPush();
                if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                    if (f->getReturnType() == "float") {
                        append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                        debugPrintPush();
                    } else {
                        append("stack.data[stack.ptr++].v = (scl_any) Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                        debugPrintPush();
                    }
                } else {
                    append("Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                }
                lastPushedType = f->getReturnType();
            } else if (hasFunction(result, Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, "", 0))) {
                std::string struct_ = s.getName();
                ITER_INC;
                if (hasFunction(result, Token(tok_identifier, struct_ + "$" + body[i].getValue(), 0, "", 0))) {
                    Function* f = getFunctionByName(result, struct_ + "$" + body[i].getValue());
                    if (f->isMethod) {
                        transpilerError("'" + f->getName() + "' is not static!", i);
                        errors.push_back(err);
                        return;
                    }
                    if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
                    if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                        if (f->getReturnType() == "float") {
                            append("stack.data[stack.ptr++].f = Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                            debugPrintPush();
                        } else {
                            append("stack.data[stack.ptr++].v = (scl_any) Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                            debugPrintPush();
                        }
                    } else {
                        append("Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                    }
                    lastPushedType = f->getReturnType();
                }
            } else if (s.hasMember(body[i].getValue())) {
                Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8];
                if (mem.getType() == "float") {
                    append("stack.data[stack.ptr++].f = Var_self->%s;\n", body[i].getValue().c_str());
                    debugPrintPush();
                } else {
                    append("stack.data[stack.ptr++].v = (scl_any) Var_self->%s;\n", body[i].getValue().c_str());
                    debugPrintPush();
                }
                lastPushedType = mem.getType();
            } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                std::string loadFrom = s.getName() + "$" + body[i].getValue();
                Variable v = getVar(Token(tok_identifier, loadFrom, 0, "", 0));
                if (v.getType() == "float") {
                    append("stack.data[stack.ptr++].f = Var_%s;\n", loadFrom.c_str());
                    debugPrintPush();
                } else {
                    append("stack.data[stack.ptr++].v = (scl_any) Var_%s;\n", loadFrom.c_str());
                    debugPrintPush();
                }
                lastPushedType = v.getType();
            } else {
                transpilerError("Unknown identifier: '" + body[i].getValue() + "'", i);
                errors.push_back(err);
            }
        } else {
            transpilerError("Unknown identifier: '" + body[i].getValue() + "'", i);
            errors.push_back(err);
        }
    }

    handler(New) {
        noUnused;
        {
            transpilerError("Using 'new <type>' is deprecated! Use '<type>::new' instead.", i);
            errors.push_back(err);
        }
        ITER_INC;
    }

    handler(Repeat) {
        noUnused;
        if (body[i + 1].getType() != tok_number) {
            transpilerError("Expected integer, but got '" + body[i + 1].getValue() + "'", i+1);
            errors.push_back(err);
            if (body[i + 2].getType() != tok_do)
                goto lbl_repeat_do_tok_chk;
            i += 2;
            return;
        }
        if (body[i + 2].getType() != tok_do) {
        lbl_repeat_do_tok_chk:
            transpilerError("Expected 'do', but got '" + body[i + 2].getValue() + "'", i+2);
            errors.push_back(err);
            i += 2;
            return;
        }
        append("for (scl_any %c = 0; %c < (scl_any) %s; %c++) {\n",
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
    }

    handler(For) {
        noUnused;
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
        append("struct scltype_iterable __it%d = (struct scltype_iterable) {0, 0};\n", iterator_count);
        while (body[i].getType() != tok_to) {
            handle(Token);
            ITER_INC;
        }
        if (body[i].getType() != tok_to) {
            transpilerError("Expected 'to', but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        ITER_INC;
        append("__it%d.start = stack.data[--stack.ptr].i;\n", iterator_count);
        while (body[i].getType() != tok_step && body[i].getType() != tok_do) {
            handle(Token);
            ITER_INC;
        }
        append("__it%d.end = stack.data[--stack.ptr].i;\n", iterator_count);
        std::string iterator_direction = "++";
        if (body[i].getType() == tok_step) {
            ITER_INC;
            if (body[i].getType() == tok_do) {
                transpilerError("Expected step, but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
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
            return;
        }
        
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
    }

    handler(Foreach) {
        noUnused;
        ITER_INC;
        if (body[i].getType() != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        std::string iterator_name = "__it" + std::to_string(iterator_count++);
        Token iter_var_tok = body[i];
        ITER_INC;
        if (body[i].getType() != tok_in) {
            transpilerError("Expected 'in', but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        ITER_INC;
        if (body[i].getType() == tok_do) {
            transpilerError("Expected iterable, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        while (body[i].getType() != tok_do) {
            handle(Token);
            ITER_INC;
        }
        if (body[i].getType() != tok_do) {
            transpilerError("Expected 'do', but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        
        if (!getStructByName(result, lastPushedType).implements("Iterable")) {
            transpilerError("Struct '" + lastPushedType + "' is not iterable!", i);
            errors.push_back(err);
            return;
        }
        append("struct Struct_%s* %s = (struct Struct_%s*) stack.data[--stack.ptr].v;\n", lastPushedType.c_str(), iterator_name.c_str(), lastPushedType.c_str());

        Method* nextMethod = getMethodByName(result, "next", lastPushedType);
        std::string var_prefix = "";
        if (!hasVar(iter_var_tok)) {
            var_prefix = sclReturnTypeToCReturnType(result, nextMethod->getReturnType()) + " ";
        }
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
        vars[varDepth].push_back(Variable(iter_var_tok.getValue(), nextMethod->getReturnType()));
        std::string cType = sclReturnTypeToCReturnType(result, getVar(iter_var_tok).getType());
        append(
            "for (%sVar_%s = (%s) Method_%s_begin(%s); Method_%s_has_next(%s); Var_%s = (%s) Method_%s_next(%s)) {\n",
            var_prefix.c_str(),
            iter_var_tok.getValue().c_str(),
            cType.c_str(),
            lastPushedType.c_str(),
            iterator_name.c_str(),
            lastPushedType.c_str(),
            iterator_name.c_str(),
            iter_var_tok.getValue().c_str(),
            cType.c_str(),
            lastPushedType.c_str(),
            iterator_name.c_str()
        );
        scopeDepth++;
    }

    handler(AddrRef) {
        noUnused;
        ITER_INC;
        Token toGet = body[i];
        if (hasFunction(result, toGet)) {
            Function* f = getFunctionByName(result, toGet.getValue());
            append("stack.data[stack.ptr++].v = (scl_any) &Function_%s;\n", f->getName().c_str());
            debugPrintPush();
            ITER_INC;
        } else if (getStructByName(result, body[i].getValue()) != Struct("")) {
            ITER_INC;
            if (body[i].getType() != tok_column) {
                transpilerError("Expected ':', but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            ITER_INC;
            if (body[i].getType() != tok_column) {
                transpilerError("Expected ':', but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            std::string struct_ = body[i - 2].getValue();
            ITER_INC;
            if (hasFunction(result, Token(tok_identifier, struct_ + "$" + body[i].getValue(), 0, "", 0))) {
                Function* f = getFunctionByName(result, struct_ + "$" + body[i].getValue());
                if (f->isMethod) {
                    transpilerError("'" + f->getName() + "' is not static!", i);
                    errors.push_back(err);
                    return;
                }
                append("stack.data[stack.ptr++].v = (scl_any) &Function_%s;\n", f->getName().c_str());
                debugPrintPush();
            } else if (hasGlobal(result, struct_ + "$" + body[i].getValue())) {
                std::string loadFrom = struct_ + "$" + body[i].getValue();
                Variable v = getVar(Token(tok_identifier, loadFrom, 0, "", 0));
                append("stack.data[stack.ptr++].v = (scl_any) &Var_%s;\n", loadFrom.c_str());
                debugPrintPush();
            }
        } else if (hasVar(toGet)) {
            Variable v = getVar(body[i]);
            std::string loadFrom = v.getName();
            if (getStructByName(result, v.getType()) != Struct("")) {
                if (body[i + 1].getType() == tok_column) {
                    if (!hasMethod(result, body[i], v.getType())) {
                        transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + v.getType() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    Method* f = getMethodByName(result, body[i].getValue(), v.getType());
                    append("stack.data[stack.ptr++].v = (scl_any) &Method_%s_%s;\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());    
                    debugPrintPush();
                } else {
                    if (body[i + 1].getType() != tok_dot) {
                        append("stack.data[stack.ptr++].v = (scl_any) &Var_%s;\n", loadFrom.c_str());
                        debugPrintPush();
                        return;
                    }
                    ITER_INC;
                    ITER_INC;
                    Struct s = getStructByName(result, v.getType());
                    if (!s.hasMember(body[i].getValue())) {
                        transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    if (body[i].getValue().at(0) == '_' && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                        transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    append("stack.data[stack.ptr++].v = (scl_any) &Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());
                    debugPrintPush();
                }
            } else {
                append("stack.data[stack.ptr++].v = (scl_any) &Var_%s;\n", loadFrom.c_str());
                debugPrintPush();
            }
        } else if (hasContainer(result, toGet)) {
            ITER_INC;
            std::string containerName = body[i].getValue();
            ITER_INC;
            if (body[i].getType() != tok_dot) {
                transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            ITER_INC;
            std::string memberName = body[i].getValue();
            Container container = getContainerByName(result, containerName);
            if (!container.hasMember(memberName)) {
                transpilerError("Unknown container member: '" + memberName + "'", i);
                errors.push_back(err);
                return;
            }
            append("stack.data[stack.ptr++].v = (scl_any) &(Container_%s.%s);\n", containerName.c_str(), memberName.c_str());
            debugPrintPush();
        } else {
            transpilerError("Unknown variable: '" + toGet.getValue() + "'", i+1);
            errors.push_back(err);
            return;
        }
        lastPushedType = "any";
    }

    handler(Store) {
        noUnused;
        ITER_INC;
        if (body[i].getType() == tok_addr_of) {
            ITER_INC;
            if (hasContainer(result, body[i])) {
                std::string containerName = body[i].getValue();
                ITER_INC;
                if (body[i].getType() != tok_dot) {
                    transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                    return;
                }
                ITER_INC;
                std::string memberName = body[i].getValue();
                Container container = getContainerByName(result, containerName);
                if (!container.hasMember(memberName)) {
                    transpilerError("Unknown container member: '" + memberName + "'", i);
                    errors.push_back(err);
                    return;
                }
                append("*((scl_any*) Container_%s.%s) = stack.data[--stack.ptr].v;\n", containerName.c_str(), memberName.c_str());
            } else {
                if (body[i].getType() != tok_identifier) {
                    transpilerError("'" + body[i].getValue() + "' is not an identifier!", i);
                    errors.push_back(err);
                }
                if (!hasVar(body[i])) {
                    if (function->isMethod) {
                        Method* m = static_cast<Method*>(function);
                        Struct s = getStructByName(result, m->getMemberType());
                        if (s.hasMember(body[i].getValue())) {
                            append("*(scl_any*) Var_self->%s = stack.data[--stack.ptr].v;\n", body[i].getValue().c_str());
                            return;
                        } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                            append("*(scl_any*) Var_%s$%s = stack.data[--stack.ptr].v;\n", s.getName().c_str(), body[i].getValue().c_str());
                            return;
                        }
                    }
                    if (body[i + 1].getType() == tok_column && body[i + 2].getType() == tok_column) {
                        i += 2;
                        Struct s = getStructByName(result, body[i - 2].getValue());
                        ITER_INC;
                        if (s != Struct("")) {
                            if (!hasVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, "", 0))) {
                                transpilerError("Struct '" + s.getName() + "' has no static member named '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            Variable mem = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, "", 0));
                            std::string loadFrom = s.getName() + "$" + body[i].getValue();
                            if (mem.getType() == "float")
                                append("Var_%s = stack.data[--stack.ptr].f;\n", loadFrom.c_str());
                            else
                                append("Var_%s = (%s) stack.data[--stack.ptr].v;\n", loadFrom.c_str(), sclReturnTypeToCReturnType(result, mem.getType()).c_str());
                            return;
                        }
                    }
                    transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                    return;
                }
                Variable v = getVar(body[i]);
                std::string loadFrom = v.getName();
                if (getStructByName(result, v.getType()) != Struct("")) {
                    if (body[i + 1].getType() != tok_dot) {
                        append("*(scl_any*) Var_%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str());
                        return;
                    }
                    ITER_INC;
                    ITER_INC;
                    Struct s = getStructByName(result, v.getType());
                    if (!s.hasMember(body[i].getValue())) {
                        transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    if (body[i].getValue().at(0) == '_' && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                        transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    append("*(scl_any*) Var_%s->%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str(), body[i].getValue().c_str());
                } else {
                    append("*(scl_any*) Var_%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str());
                }
            }
        } else if (body[i].getType() == tok_paren_open) {
            append("{\n");
            scopeDepth++;
            append("struct Struct_Array* tmp = (struct Struct_Array*) stack.data[--stack.ptr].v;\n");
            ITER_INC;
            int destructureIndex = 0;
            while (body[i].getType() != tok_paren_close) {
                if (body[i].getType() == tok_comma) {
                    ITER_INC;
                    continue;
                }
                if (body[i].getType() != tok_comma && body[i].getType() != tok_paren_close) {
                    if (body[i].getType() == tok_addr_of) {
                        ITER_INC;
                        if (hasContainer(result, body[i])) {
                            std::string containerName = body[i].getValue();
                            ITER_INC;
                            if (body[i].getType() != tok_dot) {
                                transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            ITER_INC;
                            std::string memberName = body[i].getValue();
                            Container container = getContainerByName(result, containerName);
                            if (!container.hasMember(memberName)) {
                                transpilerError("Unknown container member: '" + memberName + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*((scl_any*) Container_%s.%s) = Method_Array_get(tmp, %d);\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                        } else {
                            if (body[i].getType() != tok_identifier) {
                                transpilerError("'" + body[i].getValue() + "' is not an identifier!", i);
                                errors.push_back(err);
                            }
                            if (!hasVar(body[i])) {
                                if (function->isMethod) {
                                    Method* m = static_cast<Method*>(function);
                                    Struct s = getStructByName(result, m->getMemberType());
                                    if (s.hasMember(body[i].getValue())) {
                                        append("*(scl_any*) Var_self->%s = Method_Array_get(tmp, %d);\n", body[i].getValue().c_str(), destructureIndex);
                                        continue;
                                    } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                                        append("*(scl_any*) Var_%s$%s = Method_Array_get(tmp, %d);\n", s.getName().c_str(), body[i].getValue().c_str(), destructureIndex);
                                        return;
                                    }
                                }
                                transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                            }
                            Variable v = getVar(body[i]);
                            std::string loadFrom = v.getName();
                            if (getStructByName(result, v.getType()) != Struct("")) {
                                if (body[i + 1].getType() != tok_dot) {
                                    append("*(scl_any*) Var_%s = Method_Array_get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                                    continue;
                                }
                                ITER_INC;
                                ITER_INC;
                                Struct s = getStructByName(result, v.getType());
                                if (!s.hasMember(body[i].getValue())) {
                                    transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                                    errors.push_back(err);
                                    return;
                                }
                                if (body[i].getValue().at(0) == '_' && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                                    transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                                    errors.push_back(err);
                                    return;
                                }
                                append("*(scl_any*) Var_%s->%s = Method_Array_get(tmp, %d);\n", loadFrom.c_str(), body[i].getValue().c_str(), destructureIndex);
                            } else {
                                append("*(scl_any*) Var_%s = Method_Array_get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                            }
                        }
                    } else {
                        if (hasContainer(result, body[i])) {
                            std::string containerName = body[i].getValue();
                            ITER_INC;
                            if (body[i].getType() != tok_dot) {
                                transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            ITER_INC;
                            std::string memberName = body[i].getValue();
                            Container container = getContainerByName(result, containerName);
                            if (!container.hasMember(memberName)) {
                                transpilerError("Unknown container member: '" + memberName + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            append("(*(scl_any*) &Container_%s.%s) = Method_Array_get(tmp, %d);\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                        } else {
                            if (body[i].getType() != tok_identifier) {
                                transpilerError("'" + body[i].getValue() + "' is not an identifier!", i);
                                errors.push_back(err);
                            }
                            if (!hasVar(body[i])) {
                                if (function->isMethod) {
                                    Method* m = static_cast<Method*>(function);
                                    Struct s = getStructByName(result, m->getMemberType());
                                    if (s.hasMember(body[i].getValue())) {
                                        append("*(scl_any*) &Var_self->%s = Method_Array_get(tmp, %d);\n", body[i].getValue().c_str(), destructureIndex);
                                        continue;
                                    } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                                        append("*(scl_any*) &Var_%s$%s = Method_Array_get(tmp, %d);\n", s.getName().c_str(), body[i].getValue().c_str(), destructureIndex);
                                        return;
                                    }
                                }
                                transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                            }
                            Variable v = getVar(body[i]);
                            std::string loadFrom = v.getName();
                            if (getStructByName(result, v.getType()) != Struct("")) {
                                if (body[i + 1].getType() != tok_dot) {
                                    append("(*(scl_any*) &Var_%s) = Method_Array_get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                                    continue;
                                }
                                ITER_INC;
                                ITER_INC;
                                Struct s = getStructByName(result, v.getType());
                                if (!s.hasMember(body[i].getValue())) {
                                    transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                                    errors.push_back(err);
                                    return;
                                }
                                if (body[i].getValue().at(0) == '_' && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                                    transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                                    errors.push_back(err);
                                    return;
                                }
                                append("(*(scl_any*) &Var_%s->%s) = Method_Array_get(tmp, %d);\n", loadFrom.c_str(), body[i].getValue().c_str(), destructureIndex);
                            } else {
                                append("(*(scl_any*) &Var_%s) = Method_Array_get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                            }
                        }
                    }
                }
                ITER_INC;
                destructureIndex++;
            }
            if (destructureIndex == 0) {
                transpilerError("Empty Array destructure", i);
                warns.push_back(err);
            }
            scopeDepth--;
            append("}\n");
        } else if (body[i].getType() == tok_declare) {
            if (body[i + 1].getType() != tok_identifier) {
                transpilerError("'" + body[i + 1].getValue() + "' is not an identifier!", i+1);
                errors.push_back(err);
                return;
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
                FPResult r = parseType(body, &i);
                if (!r.success) {
                    errors.push_back(r);
                    return;
                }
                type = r.value;
            } else {
                transpilerError("A type is required!", i);
                errors.push_back(err);
                return;
            }
            Variable v = Variable(name, type);
            vars[varDepth].push_back(v);
            append("%s Var_%s = (%s) stack.data[--stack.ptr].%s;\n", sclReturnTypeToCReturnType(result, v.getType()).c_str(), v.getName().c_str(), sclReturnTypeToCReturnType(result, v.getType()).c_str(), v.getType() == "float" ? "f" : "v");
        } else {
            if (hasContainer(result, body[i])) {
                std::string containerName = body[i].getValue();
                ITER_INC;
                if (body[i].getType() != tok_dot) {
                    transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                    return;
                }
                ITER_INC;
                std::string memberName = body[i].getValue();
                Container container = getContainerByName(result, containerName);
                if (!container.hasMember(memberName)) {
                    transpilerError("Unknown container member: '" + memberName + "'", i);
                    errors.push_back(err);
                    return;
                }
                append("Container_%s.%s = (%s) stack.data[--stack.ptr].%s;\n", containerName.c_str(), memberName.c_str(), sclReturnTypeToCReturnType(result, container.getMemberType(memberName)).c_str(), container.getMemberType(memberName) == "float" ? "f" : "v");
            } else {
                if (body[i].getType() != tok_identifier) {
                    transpilerError("'" + body[i].getValue() + "' is not an identifier!", i);
                    errors.push_back(err);
                }
                if (!hasVar(body[i])) {
                    if (function->isMethod) {
                        Method* m = static_cast<Method*>(function);
                        Struct s = getStructByName(result, m->getMemberType());
                        if (s.hasMember(body[i].getValue())) {
                            Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8];
                            if (mem.getType() == "float")
                                append("Var_self->%s = stack.data[--stack.ptr].f;\n", body[i].getValue().c_str());
                            else
                                append("Var_self->%s = (%s) stack.data[--stack.ptr].v;\n", body[i].getValue().c_str(), sclReturnTypeToCReturnType(result, mem.getType()).c_str());
                            return;
                        } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                            Variable mem = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, "", 0));
                            if (mem.getType() == "float")
                                append("Var_%s$%s = stack.data[--stack.ptr].f;\n", s.getName().c_str(), body[i].getValue().c_str());
                            else
                                append("Var_%s$%s = (%s) stack.data[--stack.ptr].v;\n", s.getName().c_str(), body[i].getValue().c_str(), sclReturnTypeToCReturnType(result, mem.getType()).c_str());
                            return;
                        }
                    }
                    if (body[i + 1].getType() == tok_column && body[i + 2].getType() == tok_column) {
                        i += 2;
                        Struct s = getStructByName(result, body[i - 2].getValue());
                        ITER_INC;
                        if (s != Struct("")) {
                            if (!hasVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, "", 0))) {
                                transpilerError("Struct '" + s.getName() + "' has no static member named '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            Variable mem = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, "", 0));
                            std::string loadFrom = s.getName() + "$" + body[i].getValue();
                            if (mem.getType() == "float")
                                append("Var_%s = stack.data[--stack.ptr].f;\n", loadFrom.c_str());
                            else
                                append("Var_%s = (%s) stack.data[--stack.ptr].v;\n", loadFrom.c_str(), sclReturnTypeToCReturnType(result, mem.getType()).c_str());
                            return;
                        }
                    }
                    transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                }
                Variable v = getVar(body[i]);
                std::string loadFrom = v.getName();
                if (getStructByName(result, v.getType()) != Struct("")) {
                    if (body[i + 1].getType() != tok_dot) {
                        append("Var_%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str());
                        return;
                    }
                    ITER_INC;
                    ITER_INC;
                    Struct s = getStructByName(result, v.getType());
                    if (!s.hasMember(body[i].getValue())) {
                        transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    if (body[i].getValue().at(0) == '_' && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                        transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                        errors.push_back(err);
                        return;
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
        lastPushedType = "";
    }

    handler(Declare) {
        noUnused;
        if (body[i + 1].getType() != tok_identifier) {
            transpilerError("'" + body[i + 1].getValue() + "' is not an identifier!", i+1);
            errors.push_back(err);
            return;
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
            FPResult r = parseType(body, &i);
            if (!r.success) {
                errors.push_back(r);
                return;
            }
            type = r.value;
        } else {
            transpilerError("A type is required!", i);
            errors.push_back(err);
            return;
        }
        Variable v = Variable(name, type);
        vars[varDepth].push_back(v);
        append("%s Var_%s;\n", sclReturnTypeToCReturnType(result, v.getType()).c_str(), v.getName().c_str());
    }

    handler(CurlyOpen) {
        noUnused;
        std::string struct_ = "Array";
        if (getStructByName(result, struct_) == Struct("")) {
            transpilerError("Struct definition for 'Array' not found!", i);
            errors.push_back(err);
            return;
        }
        debugPrintPush();
        append("{\n");
        scopeDepth++;
        Method* f = getMethodByName(result, "init", struct_);
        append("struct Struct_Array* tmp = scl_alloc_struct(sizeof(struct Struct_Array), \"Array\");\n");
        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
            if (f->getReturnType() == "float") {
                append("stack.data[stack.ptr++].f = Method_Array_init(tmp, 1);\n");
                debugPrintPush();
            } else {
                append("stack.data[stack.ptr++].v = (scl_any) Method_Array_init(tmp, 1);\n");
                debugPrintPush();
            }
        } else {
            append("Method_Array_init(tmp, 1);\n");
        }
        ITER_INC;
        while (body[i].getType() != tok_curly_close) {
            if (body[i].getType() == tok_comma) {
                ITER_INC;
                continue;
            }
            bool didPush = false;
            while (body[i].getType() != tok_comma && body[i].getType() != tok_curly_close) {
                handle(Token);
                ITER_INC;
                didPush = true;
            }
            if (didPush)
                append("Method_Array_push(tmp, stack.data[--stack.ptr].v);\n");
        }
        append("stack.data[stack.ptr++].v = tmp;\n");
        lastPushedType = "Array";
        debugPrintPush();
        scopeDepth--;
        append("}\n");
    }

    handler(BracketOpen) {
        noUnused;
        std::string struct_ = "Map";
        if (getStructByName(result, struct_) == Struct("")) {
            transpilerError("Struct definition for 'Map' not found!", i);
            errors.push_back(err);
            return;
        }
        debugPrintPush();
        append("{\n");
        scopeDepth++;
        Method* f = getMethodByName(result, "init", struct_);
        append("struct Struct_Map* tmp = scl_alloc_struct(sizeof(struct Struct_Map), \"Map\");\n");
        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
            if (f->getReturnType() == "float") {
                append("stack.data[stack.ptr++].f = Method_Map_init(tmp, 1);\n");
                debugPrintPush();
            } else {
                append("stack.data[stack.ptr++].v = (scl_any) Method_Map_init(tmp, 1);\n");
                debugPrintPush();
            }
        } else {
            append("Method_Map_init(tmp, 1);\n");
        }
        ITER_INC;
        while (body[i].getType() != tok_bracket_close) {
            if (body[i].getType() == tok_comma) {
                ITER_INC;
                continue;
            }
            if (body[i].getType() != tok_string_literal) {
                transpilerError("Map keys must be strings!", i);
                errors.push_back(err);
                return;
            }
            std::string key = body[i].getValue();
            ITER_INC;
            if (body[i].getType() != tok_column) {
                transpilerError("Expected ':', but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            ITER_INC;
            bool didPush = false;
            while (body[i].getType() != tok_comma && body[i].getType() != tok_bracket_close) {
                handle(Token);
                ITER_INC;
                didPush = true;
            }
            if (didPush)
                append("Method_Map_set(tmp, stack.data[--stack.ptr].v, \"%s\");\n", key.c_str());
        }
        append("stack.data[stack.ptr++].v = tmp;\n");
        lastPushedType = "Map";
        debugPrintPush();
        scopeDepth--;
        append("}\n");
    }

    handler(ParenOpen) {
        noUnused;
        if (body[i + 2].getType() == tok_column) {
            if (getStructByName(result, "MapEntry") == Struct("")) {
                transpilerError("Struct definition for 'MapEntry' not found!", i);
                errors.push_back(err);
                return;
            }
            debugPrintPush();
            append("{\n");
            scopeDepth++;
            ITER_INC;
            if (body[i].getType() != tok_string_literal) {
                transpilerError("MapEntry keys must be strings!", i);
                errors.push_back(err);
                return;
            }
            std::string key = body[i].getValue();
            ITER_INC;
            if (body[i].getType() != tok_column) {
                transpilerError("Expected ':', but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            ITER_INC;
            while (body[i].getType() != tok_paren_close) {
                handle(Token);
                ITER_INC;
            }
            Method* f = getMethodByName(result, "init", "MapEntry");
            append("struct Struct_MapEntry* tmp = scl_alloc_struct(sizeof(struct Struct_MapEntry), \"MapEntry\");\n");
            if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                if (f->getReturnType() == "float") {
                    append("stack.data[stack.ptr++].f = Method_MapEntry_init(tmp, stack.data[--stack.ptr].v, \"%s\");\n", key.c_str());
                    debugPrintPush();
                } else {
                    append("stack.data[stack.ptr++].v = (scl_any) Method_MapEntry_init(tmp, stack.data[--stack.ptr].v, \"%s\");\n", key.c_str());
                    debugPrintPush();
                }
            } else {
                append("Method_MapEntry_init(tmp, stack.data[--stack.ptr].v, \"%s\");\n", key.c_str());
            }
            append("stack.data[stack.ptr++].v = tmp;\n");
            lastPushedType = "MapEntry";
            debugPrintPush();
            scopeDepth--;
            append("}\n");
        } else if (body[i + 2].getType() == tok_comma) {
            int j = 0;
            int commas = 0;
            for (; body[i + j].getType() != tok_paren_close; j++) {
                if (body[i + j].getType() == tok_comma) commas++;
            }
            if (commas == 1) {
                if (getStructByName(result, "Pair") == Struct("")) {
                    transpilerError("Struct definition for 'Pair' not found!", i);
                    errors.push_back(err);
                    return;
                }
                debugPrintPush();
                append("{\n");
                scopeDepth++;
                ITER_INC;
                while (body[i].getType() != tok_paren_close) {
                    if (body[i].getType() == tok_comma) {
                        ITER_INC;
                        continue;
                    }
                    while (body[i].getType() != tok_comma && body[i].getType() != tok_paren_close) {
                        handle(Token);
                        ITER_INC;
                    }
                }
                Method* f = getMethodByName(result, "init", "Pair");
                append("struct Struct_Pair* tmp = scl_alloc_struct(sizeof(struct Struct_Pair), \"Pair\");\n");
                append("stack.ptr -= 2;\n");
                if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                    if (f->getReturnType() == "float") {
                        if (i) {
                            append("stack.data[stack.ptr++].f = Method_Pair_init(tmp, stack.data[stack.ptr + 1].v, stack.data[stack.ptr].v);\n");
                        } else {
                            append("stack.data[stack.ptr++].f = Method_Pair_init(tmp, stack.data[stack.ptr + 1].v,\n");
                        }
                        debugPrintPush();
                    } else {
                        if (i) {
                            append("stack.data[stack.ptr++].v = (scl_any) Method_Pair_init(tmp, stack.data[stack.ptr + 1].v, stack.data[stack.ptr].v);\n");
                        } else {
                            append("stack.data[stack.ptr++].v = (scl_any) Method_Pair_init(tmp, stack.data[stack.ptr + 1].v,\n");
                        }
                        debugPrintPush();
                    }
                } else {
                    if (i) {
                        append("Method_Pair_init(tmp, stack.data[stack.ptr + 1].v, stack.data[stack.ptr].v);\n");
                    } else {
                        append("Method_Pair_init(tmp, stack.data[stack.ptr + 1].v,\n");
                    }
                }
                append("stack.data[stack.ptr++].v = tmp;\n");
                lastPushedType = "Pair";
                debugPrintPush();
                scopeDepth--;
                append("}\n");
            } else if (commas == 2) {
                if (getStructByName(result, "Triple") == Struct("")) {
                    transpilerError("Struct definition for 'Triple' not found!", i);
                    errors.push_back(err);
                    return;
                }
                debugPrintPush();
                append("{\n");
                scopeDepth++;
                ITER_INC;
                while (body[i].getType() != tok_paren_close) {
                    if (body[i].getType() == tok_comma) {
                        ITER_INC;
                        continue;
                    }
                    while (body[i].getType() != tok_comma && body[i].getType() != tok_paren_close) {
                        handle(Token);
                        ITER_INC;
                    }
                }
                Method* f = getMethodByName(result, "init", "Triple");
                append("struct Struct_Triple* tmp = scl_alloc_struct(sizeof(struct Struct_Triple), \"Triple\");\n");
                append("stack.ptr -= 3;\n");
                if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                    if (f->getReturnType() == "float") {
                        if (i) {
                            append("stack.data[stack.ptr++].f = Method_Triple_init(tmp, stack.data[stack.ptr + 2].v, stack.data[stack.ptr + 1].v, stack.data[stack.ptr].v);\n");
                        } else {
                            append("stack.data[stack.ptr++].f = Method_Triple_init(tmp, stack.data[stack.ptr + 2].v, stack.data[stack.ptr + 1].v,\n");
                        }
                        debugPrintPush();
                    } else {
                        if (i) {
                            append("stack.data[stack.ptr++].v = (scl_any) Method_Triple_init(tmp, stack.data[stack.ptr + 2].v, stack.data[stack.ptr + 1].v, stack.data[stack.ptr].v);\n");
                        } else {
                            append("stack.data[stack.ptr++].v = (scl_any) Method_Triple_init(tmp, stack.data[stack.ptr + 2].v, stack.data[stack.ptr + 1].v,\n");
                        }
                        debugPrintPush();
                    }
                } else {
                    if (i) {
                        append("Method_Triple_init(tmp, stack.data[stack.ptr + 2].v, stack.data[stack.ptr + 1].v, stack.data[stack.ptr].v);\n");
                    } else {
                        append("Method_Triple_init(tmp, stack.data[stack.ptr + 2].v, stack.data[stack.ptr + 1].v,\n");
                    }
                }
                append("stack.data[stack.ptr++].v = tmp;\n");
                lastPushedType = "Triple";
                debugPrintPush();
                scopeDepth--;
                append("}\n");
            } else {
                transpilerError("Unsupported tuple-like literal!", i);
                errors.push_back(err);
            }
        } else {
            transpilerError("Unsupported tuple-like literal!", i);
            errors.push_back(err);
        }
    }

    handler(StringLiteral) {
        noUnused;
        append("stack.data[stack.ptr++].v = \"%s\";\n", body[i].getValue().c_str());
        lastPushedType = "str";
        debugPrintPush();
    }

    handler(CDecl) {
        noUnused;
        ITER_INC;
        if (body[i].getType() != tok_string_literal) {
            transpilerError("Expected string literal, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        std::string s = replaceAll(body[i].getValue(), R"(\\\\)", "\\");
        s = replaceAll(s, R"(\\\")", "\"");
        append("%s\n", s.c_str());
    }

    std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = 0;
        while ((end = str.find(delimiter, start)) != std::string::npos)
        {
            tokens.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
        }
        tokens.push_back(str.substr(start));
        return tokens;
    }


    std::string ltrim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start);
    }

    handler(ExternC) {
        noUnused;
        append("{// Start C\n");
        scopeDepth++;
        std::string ext = body[i].getValue();
        for (std::vector<Variable> lvl : vars) {
            for (Variable v : lvl) {
                append("%s* %s = &Var_%s;\n", sclReturnTypeToCReturnType(result, v.getType()).c_str(), v.getName().c_str(), v.getName().c_str());
            }
        }
        append("{\n");
        scopeDepth++;
        std::vector<std::string> lines = split(ext, "\n");
        for (std::string line : lines) {
            if (ltrim(line).size() == 0)
                continue;
            append("%s\n", ltrim(line).c_str());
        }
        scopeDepth--;
        append("}\n");
        scopeDepth--;
        append("}// End C\n");
    }

    handler(IntegerLiteral) {
        noUnused;
        FPResult numberHandled = handleNumber(fp, body[i], scopeDepth);
        if (!numberHandled.success) {
            errors.push_back(numberHandled);
        }
        lastPushedType = "int";
    }

    handler(FloatLiteral) {
        noUnused;
        FPResult numberHandled = handleDouble(fp, body[i], scopeDepth);
        if (!numberHandled.success) {
            errors.push_back(numberHandled);
        }
        lastPushedType = "float";
    }

    handler(FalsyType) {
        noUnused;
        append("stack.data[stack.ptr++].v = (scl_any) 0;\n");
        lastPushedType = "bool";
        debugPrintPush();
    }

    handler(TruthyType) {
        noUnused;
        append("stack.data[stack.ptr++].v = (scl_any) 1;\n");
        lastPushedType = "bool";
        debugPrintPush();
    }

    handler(Is) {
        noUnused;
        ITER_INC;
        if (body[i].getType() != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        std::string struct_ = body[i].getValue();
        if (getStructByName(result, struct_) == Struct("")) {
            transpilerError("Usage of undeclared struct '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        append("stack.data[stack.ptr - 1].i = stack.data[stack.ptr - 1].v && ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->$__type__ == 0x%016llx;\n", struct_.c_str(), hash1((char*) struct_.c_str()));
        lastPushedType = "bool";
    }

    handler(If) {
        noUnused;
        append("if (stack.data[--stack.ptr].v) {\n");
        scopeDepth++;
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
    }

    handler(Else) {
        noUnused;
        scopeDepth--;
        varDepth--;
        vars.pop_back();
        append("} else {\n");
        scopeDepth++;
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
    }

    handler(While) {
        noUnused;
        append("while (1) {\n");
        scopeDepth++;
        was_rep.push_back(false);
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
    }

    handler(Do) {
        noUnused;
        append("if (!stack.data[--stack.ptr].v) break;\n");
    }

    handler(DoneLike) {
        noUnused;
        scopeDepth--;
        varDepth--;
        vars.pop_back();
        append("}\n");
        if (repeat_depth > 0 && was_rep[was_rep.size() - 1]) {
            repeat_depth--;
        }
        if (was_rep.size() > 0) was_rep.pop_back();
    
    }

    handler(Return) {
        noUnused;
        if (i == (body.size() - 1) && (function->getReturnType().size() == 0 || function->getReturnType() == "none")) return;
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
            } else if (return_type == "scl_any") {
                append("return stack.data[--stack.ptr].v;\n");
            } else if (strncmp(return_type.c_str(), "scl_", 4) == 0) {
                append("return (%s) stack.data[--stack.ptr].v;\n", sclReturnTypeToCReturnType(result, function->getReturnType()).c_str());
            }
        }
    }

    handler(Goto) {
        noUnused;
        ITER_INC;
        if (body[i].getType() != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
        }
        append("goto %s;\n", body[i].getValue().c_str());
    }

    handler(Label) {
        noUnused;
        ITER_INC;
        if (body[i].getType() != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
        }
        append("%s:\n", body[i].getValue().c_str());
    }

    handler(Case) {
        noUnused;
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
    }

    handler(Esac) {
        noUnused;
        append("break;\n");
        scopeDepth--;
        varDepth--;
        vars.pop_back();
        append("}\n");
    }

    handler(Default) {
        noUnused;
        append("default: {\n");
        scopeDepth++;
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
    }

    handler(Switch) {
        noUnused;
        append("switch (stack.data[--stack.ptr].i) {\n");
        scopeDepth++;
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
        was_rep.push_back(false);
    }

    handler(AddrOf) {
        noUnused;
        append("stack.data[stack.ptr - 1].v = (*(scl_any*) stack.data[stack.ptr - 1].v);\n");
        lastPushedType = "any";
    }

    handler(Break) {
        noUnused;
        append("break;\n");
    }

    handler(Continue) {
        noUnused;
        append("continue;\n");
    }

    handler(As) {
        noUnused;
        ITER_INC;
        if (body[i].getValue() == "int" || body[i].getValue() == "float" || body[i].getValue() == "str" || body[i].getValue() == "any") {
            lastPushedType = body[i].getValue();
            return;
        }
        if (getStructByName(result, body[i].getValue()) == Struct("")) {
            transpilerError("Use of undeclared Struct '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        lastPushedType = body[i].getValue();
    }

    handler(Dot) {
        noUnused;
        Struct s = getStructByName(result, lastPushedType);
        if (s == Struct("")) {
            transpilerError("Cannot infer type of stack top: expected valid Struct, but got '" + lastPushedType + "'", i);
            errors.push_back(err);
            return;
        }
        ITER_INC;
        if (!s.hasMember(body[i].getValue())) {
            transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        if (body[i].getValue().at(0) == '_' && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
            transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
            errors.push_back(err);
            return;
        }
        Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8];
        if (mem.getType() == "float")
            append("stack.data[stack.ptr - 1].f = ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", s.getName().c_str(), body[i].getValue().c_str());
        else
            append("stack.data[stack.ptr - 1].v = (scl_any) ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", s.getName().c_str(), body[i].getValue().c_str());
        lastPushedType = mem.getType();
        if (body[i + 1].getType() == tok_dot) {
            ITER_INC;
            handle(Dot);
        }
        if (body[i + 1].getType() == tok_column) {
            ITER_INC;
            handle(Column);
        }
    }

    handler(Column) {
        noUnused;
        Struct s = getStructByName(result, lastPushedType);
        if (s == Struct("")) {
            transpilerError("Cannot infer type of stack top: expected valid Struct, but got '" + lastPushedType + "'", i);
            errors.push_back(err);
            return;
        }
        ITER_INC;
        if (!hasMethod(result, body[i], lastPushedType)) {
            transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + lastPushedType + "'", i);
            errors.push_back(err);
            return;
        }
        Method* f = getMethodByName(result, body[i].getValue(), lastPushedType);
        if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
            if (f->getReturnType() == "float") {
                append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                debugPrintPush();
            } else {
                append("stack.data[stack.ptr++].v = (scl_any) Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                debugPrintPush();
            }
        } else {
            append("Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
        }
        lastPushedType = f->getReturnType();
        if (body[i + 1].getType() == tok_dot) {
            ITER_INC;
            handle(Dot);
        }
        if (body[i + 1].getType() == tok_column) {
            ITER_INC;
            handle(Column);
        }
    }

    handler(Token) {
        noUnused;
        if (body[i].getType() == tok_ignore) return;

        std::string file = body[i].getFile();

        if (isOperator(body[i])) {
            FPResult operatorsHandled = handleOperator(fp, body[i], scopeDepth);
            if (!operatorsHandled.success) {
                errors.push_back(operatorsHandled);
            }
        } else {
            switch (body[i].getType()) {
                case tok_identifier: {
                    handle(Identifier);
                    break;
                }

                case tok_dot: {
                    handle(Dot);
                    break;
                }

                case tok_column: {
                    handle(Column);
                    break;
                }

                case tok_as: {
                    handle(As);
                    break;
                }

                case tok_string_literal: {
                    handle(StringLiteral);
                    break;
                }

                case tok_cdecl: {
                    handle(CDecl);
                    break;
                }

                case tok_extern_c: {
                    handle(ExternC);
                    break;
                }

                case tok_number:
                case tok_char_literal: {
                    handle(IntegerLiteral);
                    break;
                }

                case tok_number_float: {
                    handle(FloatLiteral);
                    break;
                }

                case tok_nil:
                case tok_false: {
                    handle(FalsyType);
                    break;
                }

                case tok_true: {
                    handle(TruthyType);
                    break;
                }

                case tok_new: {
                    handle(New);
                    break;
                }

                case tok_is: {
                    handle(Is);
                    break;
                }

                case tok_if: {
                    handle(If);
                    break;
                }

                case tok_else: {
                    handle(Else);
                    break;
                }

                case tok_while: {
                    handle(While);
                    break;
                }

                case tok_do: {
                    handle(Do);
                    break;
                }

                case tok_repeat: {
                    handle(Repeat);
                    break;
                }

                case tok_for: {
                    handle(For);
                    break;
                }

                case tok_foreach: {
                    handle(Foreach);
                    break;
                }

                case tok_done:
                case tok_fi:
                case tok_end: {
                    handle(DoneLike);
                    break;
                }

                case tok_return: {
                    handle(Return);
                    break;
                }

                case tok_addr_ref: {
                    handle(AddrRef);
                    break;
                }

                case tok_store: {
                    handle(Store);
                    break;
                }

                case tok_declare: {
                    handle(Declare);
                    break;
                }

                case tok_continue: {
                    handle(Continue);
                    break;
                }

                case tok_break: {
                    handle(Break);
                    break;
                }

                case tok_goto: {
                    handle(Goto);
                    break;
                }

                case tok_label: {
                    handle(Label);
                    break;
                }

                case tok_case: {
                    handle(Case);
                    break;
                }

                case tok_esac: {
                    handle(Esac);
                    break;
                }

                case tok_default: {
                    handle(Default);
                    break;
                }

                case tok_switch: {
                    handle(Switch);
                    break;
                }

                case tok_curly_open: {
                    handle(CurlyOpen);
                    break;
                }

                case tok_bracket_open: {
                    handle(BracketOpen);
                    break;
                }

                case tok_paren_open: {
                    handle(ParenOpen);
                    break;
                }

                case tok_addr_of: {
                    handle(AddrOf);
                    break;
                }

                default: {
                    transpilerError("Unknown token: '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                    break;
                }
            }
        }
    }

    extern "C" void ConvertC::writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<Variable>& globals, TPResult result) {
        (void) warns;
        scopeDepth = 0;
        append("/* EXTERN VARS FROM INTERNAL */\n");
        append("extern scl_stack_t stack;\n");
        append("extern scl_stack_t  callstk;\n\n");

        append("/* LOOP STRUCTS */\n");
        append("struct scltype_iterable {\n");
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
            while (vars.size() < 2) {
                std::vector<Variable> def;
                vars.push_back(def);
            }
            varDepth = 1;
            vars[varDepth].clear();
            for (Variable g : globals) {
                vars[varDepth].push_back(g);
            }

            scopeDepth = 0;
            noWarns = false;
            noMangle = false;

            for (std::string modifier : function->getModifiers()) {
                if (modifier == "nowarn") {
                    noWarns = true;
                } else if (modifier == "nomangle") {
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

            return_type = "void";

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
                args = "";
                for (ssize_t i = (ssize_t) function->getArgs().size() - 1; i >= 0; i--) {
                    Variable arg = function->getArgs()[i];
                    args += "Var_" + arg.getName();
                    if (i) args += ", ";
                }
                if (return_type != "void") {
                    append("return Function_%s(%s);\n", function->getName().c_str(), args.c_str());
                } else {
                    append("Function_%s(%s);\n", function->getName().c_str(), args.c_str());
                }
                scopeDepth--;
                append("}\n");
            } else if (noMangle && function->isMethod) {
                FPResult err;
                err.success = false;
                err.message = "Methods don't support the @nomangle Modifier!";
                err.column = function->getNameToken().getColumn();
                err.line = function->getNameToken().getLine();
                err.in = function->getNameToken().getFile();
                err.value = function->getNameToken().getValue();
                err.type = function->getNameToken().getType();
                errors.push_back(err);
                continue;
            }

            return_type = "void";

            std::string arguments = "";
            if (function->getArgs().size() > 0) {
                for (ssize_t i = (ssize_t) function->getArgs().size() - 1; i >= 0; i--) {
                    Variable var = function->getArgs()[i];
                    vars[varDepth].push_back(var);
                    arguments += sclReturnTypeToCReturnType(result, function->getArgs()[i].getType()) + " Var_" + function->getArgs()[i].getName();
                    if (i) {
                        arguments += ", ";
                    }
                }
            }

            if (function->getReturnType().size() > 0) {
                std::string t = function->getReturnType();
                return_type = sclReturnTypeToCReturnType(result, t);
            }
            if (!function->isMethod) {
                append("%s Function_%s(%s) {\n", return_type.c_str(), function->getName().c_str(), arguments.c_str());
            } else {
                if (!((Method*)(function))->addAnyway() && getStructByName(result, ((Method*)(function))->getMemberType()).isSealed()) {
                    FPResult result;
                    result.message = "Struct '" + ((Method*)(function))->getMemberType() + "' is sealed!";
                    result.column = function->getNameToken().getColumn();
                    result.value = function->getNameToken().getValue();
                    result.line = function->getNameToken().getLine();
                    result.in = function->getNameToken().getFile();
                    result.type = function->getNameToken().getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                append("%s Method_%s_%s(%s) {\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str());
            }

            std::vector<Token> body = function->getBody();
            if (body.size() == 0)
                goto emptyFunction;
            
            scopeDepth++;

            append("callstk.data[callstk.ptr++].v = \"%s\";\n", functionDeclaration.c_str());

            for (i = 0; i < body.size(); i++) {
                handle(Token);
            }
            scopeDepth = 1;
            if (body.size() == 0 || body[body.size() - 1].getType() != tok_return)
                append("callstk.ptr--;\n");
        emptyFunction:
            scopeDepth = 0;
            append("}\n\n");
        }
    }
} // namespace sclc
