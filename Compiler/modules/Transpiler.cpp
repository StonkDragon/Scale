#include <filesystem>
#include <stack>

#include "../headers/Common.hpp"
#include "../headers/TranspilerDefs.hpp"

#define debugDump(_var) std::cout << #_var << ": " << _var << std::endl
#define ITER_INC                                             \
    do {                                                     \
        i++;                                                 \
        if (i >= body.size()) {                              \
            FPResult err;                                    \
            err.success = false;                             \
            err.message = "Unexpected end of function! "     \
                + std::string("Error happened in function ") \
                + std::string(__func__)                      \
                + " in line "                                \
                + std::to_string(__LINE__);                  \
            err.line = body[i - 1].getLine();                \
            err.in = body[i - 1].getFile();                  \
            err.column = body[i - 1].getColumn();            \
            err.value = body[i - 1].getValue();              \
            err.type = body[i - 1].getType();                \
            errors.push_back(err);                           \
            return;                                          \
        }                                                    \
    } while (0)

namespace sclc {
    FILE* support_header;
    FILE* symbolTable;

    std::string sclFunctionNameToFriendlyString(std::string name);

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

    std::string sclTypeToCType(TPResult result, std::string t) {
        if (t.size() && t != "?" && t.at(t.size() - 1) == '?') t = t.substr(0, t.size() - 1);
        std::string return_type = "scl_any";
        if (t == "any") return_type = "scl_any";
        else if (t == "none") return_type = "void";
        else if (t == "int") return_type = "scl_int";
        else if (t == "float") return_type = "scl_float";
        else if (t == "str") return_type = "scl_str";
        else if (t == "bool") return_type = "scl_int";
        else if (!(getStructByName(result, t) == Struct(""))) {
            return_type = "scl_" + getStructByName(result, t).getName();
        } else if (t.size() > 2 && t.size() && t.at(0) == '[') {
            std::string type = sclTypeToCType(result, t.substr(1, t.length() - 2));
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
                    args += "(" + sclTypeToCType(result, arg.getType()) + ") stack.data[stack.ptr + " + std::to_string(i) + "].v";
                } else {
                    args += "(" + sclTypeToCType(result, arg.getType()) + ") stack.data[stack.ptr].v";
                }
            }
            if (i) args += ", ";
        }
        return args;
    }

    std::string typeToASCII(std::string v) {
        bool canBeNil = false;
        if (v.size() && v != "?" && v.at(v.size() - 1) == '?') {
            v = v.substr(0, v.size() - 1);
            canBeNil = true;
        }
        if (v.size() && v.at(0) == '[') {
            return "_sclPtrTo_" + typeToASCII(v.substr(1, v.length() - 2)) + (canBeNil ? "_sclMaybeNil" : "");
        }
        if (v == "?") {
            return std::string("_sclWildcard") + (canBeNil ? "_sclMaybeNil" : "");
        }
        return v;
    }

    std::string generateSymbolForFunction(Function* f) {
        std::string symbol = "function_" + f->getName();
        if (f->getName().find('$') != std::string::npos) {
            symbol = "static_" + f->getName().substr(0, f->getName().find('$')) + "_function_" + f->getName().substr(f->getName().find('$') + 1);
        }
        if (f->isMethod) {
            symbol = "method_" + static_cast<Method*>(f)->getMemberType() + "_" + symbol;
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
        symbol += "_sclArgs_" + arguments + "_sclType_" + typeToASCII(f->getReturnType());
        if (f->isExternC) {
            if (f->isMethod) {
                symbol = "_Method_" + static_cast<Method*>(f)->getMemberType() + "_" + f->getName();
            } else {
                symbol = "_Function_" + f->getName();
            }
        }

        if (Main.options.transpileOnly && symbolTable) {
            if (f->isMethod) {
                fprintf(symbolTable, "Symbol for Method %s:%s: '%s'\n", static_cast<Method*>(f)->getMemberType().c_str(), f->getName().c_str(), symbol.c_str());
            } else {
                fprintf(symbolTable, "Symbol for Function %s: '%s'\n", replaceFirstAfter(f->getName(), "$", "::", 1).c_str(), symbol.c_str());
            }
        }

        return symbol;
    }

    std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
        std::vector<std::string> body;
        size_t start = 0;
        size_t end = 0;
        while ((end = str.find(delimiter, start)) != std::string::npos)
        {
            body.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
        }
        body.push_back(str.substr(start));
        return body;
    }

    std::string ltrim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start);
    }

    void ConvertC::writeHeader(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;

        append("#ifndef SCALE_COMMON_HEADER\n");
        append("#define SCALE_COMMON_HEADER\n\n");
        
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

    void ConvertC::writeFunctionHeaders(FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        append("/* FUNCTION HEADERS */\n");

        if (Main.options.transpileOnly) {
            remove("scale-symbol-table.txt");
            if (Main.options.debugBuild)
                symbolTable = fopen("scale-symbol-table.txt", "a");
        }

        for (Function* function : result.functions) {
            std::string return_type = "void";

            if (function->getReturnType().size() > 0) {
                std::string t = function->getReturnType();
                return_type = sclTypeToCType(result, t);
            }
            
            std::string arguments = "";
            if (function->getArgs().size() > 0) {
                for (ssize_t i = (ssize_t) function->getArgs().size() - 1; i >= 0; i--) {
                    arguments += sclTypeToCType(result, function->getArgs()[i].getType()) + " Var_" + function->getArgs()[i].getName();
                    if (i) {
                        arguments += ", ";
                    }
                }
            }

            std::string symbol = generateSymbolForFunction(function);

            if (!function->isMethod) {
                append("void scl_reflect_call_function_%s();\n", function->getName().c_str());
                append("%s Function_%s(%s) __asm(\"%s\");\n", return_type.c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
            } else {
                append("void scl_reflect_call_method_%s_function_%s();\n", ((Method*)(function))->getMemberType().c_str(), function->getName().c_str());
                append("%s Method_%s_%s(%s) __asm(\"%s\");\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
            }
            if (function->isExternC) {
                if (!function->isMethod) {
                    fprintf(support_header, "expect %s %s(%s) __asm(\"%s\");\n", return_type.c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                } else {
                    fprintf(support_header, "expect %s Method_%s_%s(%s) __asm(\"%s\");\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                }
                continue;
            }
            for (std::string m : function->getModifiers()) {
                if (contains<std::string>(function->getModifiers(), "export")) {
                    if (!function->isMethod) {
                        fprintf(support_header, "export %s %s(%s) __asm(\"%s\");\n", return_type.c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                    } else {
                        fprintf(support_header, "export %s Method_%s_%s(%s) __asm(\"%s\");\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                    }
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
                    return_type = sclTypeToCType(result, t);
                }

                std::string arguments = "";
                if (function->getArgs().size() > 0) {
                    for (ssize_t i = (ssize_t) function->getArgs().size() - 1; i >= 0; i--) {
                        arguments += sclTypeToCType(result, function->getArgs()[i].getType()) + " Var_" + function->getArgs()[i].getName();
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
                if (function->isExternC) {
                    if (!function->isMethod) {
                        fprintf(support_header, "expect %s %s(%s) __asm(\"%s\");\n", return_type.c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                    } else {
                        fprintf(support_header, "expect %s Method_%s_%s(%s) __asm(\"%s\");\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                    }
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
            append("%s Var_%s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
            fprintf(support_header, "extern %s Var_%s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
            // vars[varDepth].push_back(s);
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
                append("  %s %s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
            }
            append("} Container_%s = {0};\n", c.getName().c_str());
            fprintf(support_header, "struct Container_%s {\n", c.getName().c_str());
            for (Variable s : c.getMembers()) {
                fprintf(support_header, "  %s %s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
            }
            fprintf(support_header, "};\n");
            fprintf(support_header, "extern struct Container_%s Container_%s;\n", c.getName().c_str(), c.getName().c_str());
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

    std::string argVectorToString(std::vector<Variable> args) {
        std::string arg = "";
        for (size_t i = 0; i < args.size(); i++) {
            if (i) 
                arg += ", ";
            Variable v = args[i];
            arg += v.getType();
        }
        return arg;
    }

    void ConvertC::writeStructs(FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        if (result.structs.size() == 0) return;
        append("/* STRUCT TYPES */\n");
        for (Struct c : result.structs) {
            append("typedef struct Struct_%s* scl_%s;\n", c.getName().c_str(), c.getName().c_str());
            fprintf(support_header, "typedef struct %s* scl_%s;\n", c.getName().c_str(), c.getName().c_str());
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
                    res.line = t.getLine();
                    res.in = t.getFile();
                    res.column = t.getColumn();
                    res.value = t.getValue();
                    res.type = t.getType();
                    errors.push_back(res);
                    continue;
                }
                for (Function* f : interface->getImplements()) {
                    if (!hasMethod(result, f->getName(), c.getName())) {
                        FPResult res;
                        Token t = c.nameToken();
                        res.success = false;
                        res.message = "No implementation for method '" + f->getName() + "' on struct '" + c.getName() + "'";
                        res.line = t.getLine();
                        res.in = t.getFile();
                        res.column = t.getColumn();
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
                        res.line = t.getLine();
                        res.in = t.getFile();
                        res.column = t.getColumn();
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
                        res.line = t.getLine();
                        res.in = t.getFile();
                        res.column = t.getColumn();
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
            append("  scl_int $__super__;\n");
            append("  scl_int $__size__;\n");
            for (Variable s : c.getMembers()) {
                append("  %s %s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
            }
            append("};\n");
            fprintf(support_header, "struct %s {\n", c.getName().c_str());
            fprintf(support_header, "  scl_int __type_identifier__;\n");
            fprintf(support_header, "  scl_str __type_string__;\n");
            fprintf(support_header, "  scl_int __super__;\n");
            fprintf(support_header, "  scl_int __size__;\n");
            for (Variable s : c.getMembers()) {
                fprintf(support_header, "  %s %s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
            }
            fprintf(support_header, "};\n");
        }
        append("\n");
    }

    int scopeDepth = 0;
    size_t i = 0;
    size_t condCount = 0;
    std::vector<bool> was_rep;
    char repeat_depth = 0;
    int iterator_count = 0;
    bool noWarns;
    std::stack<std::string> typeStack;
#define typeStackTop (typeStack.size() ? typeStack.top() : "")
    std::string return_type = "";

    bool canBeCastTo(TPResult r, Struct& one, Struct& other) {
        if (one == other || other.getName() == "SclObject") {
            return true;
        }
        Struct super = getStructByName(r, one.extends());
        bool extend = false;
        while (!extend) {
            if (super == other || super.getName() == "SclObject") {
                extend = true;
            }
            super = getStructByName(r, super.extends());
        }
        return extend;
    }

    bool checkStackType(TPResult result, std::vector<Variable> args) {
        if (typeStack.size() == 0 && typeStack.size() == args.size()) {
            return true;
        }
        if (typeStack.size() < args.size()) {
            return false;
        }
        auto end = &typeStack.top() + 1;
        auto begin = end - args.size();
        std::vector<std::string> tmp(begin, end);

        bool typesMatch = true;
        for (ssize_t i = args.size() - 1; i >= 0; i--) {
            std::string typeA = tmp.at(i);
            if (typeA == "_String") typeA = "str";
            std::string typeB = args.at(i).getType();
            if (typeB == "_String") typeB = "str";

            if (args.at(i).getType() == "any" || args.at(i).getType() == "[any]") {
                continue;
            }

            if (typeA != typeB) {
                Struct a = getStructByName(result, typeA);
                Struct b = getStructByName(result, typeB);
                if (a.getName() != "" && b.getName() != "") {
                    if (!canBeCastTo(result, a, b)) {
                        typesMatch = false;
                    }
                } else {
                    typesMatch = false;
                }
            }
        }
        return typesMatch;
    }

    std::string stackSliceToString(size_t amount) {
        if (typeStack.size() < amount) amount = typeStack.size();

        if (amount == 0)
            return "";
        auto end = &typeStack.top() + 1;
        auto begin = end - amount;
        std::vector<std::string> tmp(begin, end);

        std::string arg = "";
        for (size_t i = 0; i < amount; i++) {
            std::string typeA = tmp.at(i);
            if (typeA == "_String") typeA = "str";
            
            if (i) {
                arg += ", ";
            }

            arg += typeA;
        }
        return arg;
    }

#define handler(_tok) extern "C" void handle ## _tok (std::vector<Token>& body, Function* function, std::vector<FPResult>& errors, std::vector<FPResult>& warns, FILE* fp, TPResult result)
#define handle(_tok) handle ## _tok (body, function, errors, warns, fp, result)
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

    bool handleOverriddenOperator(TPResult result, FILE* fp, int scopeDepth, std::string op, std::string type);

    handler(Identifier) {
        noUnused;
        if (body[i].getValue() == "self" && !function->isMethod) {
            transpilerError("'self' can only be used in methods, not in functions.", i);
            errors.push_back(err);
            return;
        }
    #ifdef __APPLE__
    #pragma region Identifier functions
    #endif
        if (body[i].getValue() == "drop") {
            append("stack.ptr--;\n");
            if (typeStack.size())
                typeStack.pop();
        } else if (body[i].getValue() == "dup") {
            append("stack.data[stack.ptr++].v = stack.data[stack.ptr - 1].v;\n");
            typeStack.push(typeStackTop);
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
            std::string b = typeStackTop;
            if (typeStack.size())
                typeStack.pop();
            std::string a = typeStackTop;
            if (typeStack.size())
                typeStack.pop();
            typeStack.push(b);
            typeStack.push(a);
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
            std::string c = typeStackTop;
            if (typeStack.size())
                typeStack.pop();
            std::string b = typeStackTop;
            if (typeStack.size())
                typeStack.pop();
            std::string a = typeStackTop;
            if (typeStack.size())
                typeStack.pop();
            typeStack.push(c);
            typeStack.push(b);
            typeStack.push(a);
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
            std::string b = typeStackTop;
            if (typeStack.size())
                typeStack.pop();
            std::string a = typeStackTop;
            if (typeStack.size())
                typeStack.pop();
            typeStack.push(a);
            typeStack.push(b);
            typeStack.push(a);
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
            std::string c = typeStackTop;
            if (typeStack.size())
                typeStack.pop();
            std::string b = typeStackTop;
            if (typeStack.size())
                typeStack.pop();
            std::string a = typeStackTop;
            if (typeStack.size())
                typeStack.pop();
            typeStack.push(b);
            typeStack.push(a);
            typeStack.push(c);
        } else if (body[i].getValue() == "clearstack") {
            append("stack.ptr = 0;\n");
            for (size_t t = 0; t < typeStack.size(); t++) {
                if (typeStack.size())
                    typeStack.pop();
            }
        } else if (body[i].getValue() == "&&") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "&&", typeStackTop)) return;
            append("stack.ptr -= 2;\n");
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i && stack.data[stack.ptr + 1].i;\n");
            debugPrintPush();
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "!") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "!", typeStackTop)) return;
            append("stack.data[stack.ptr - 1].i = !stack.data[stack.ptr - 1].i;\n");
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "!!") {
            if (typeCanBeNil(typeStackTop)) {
                std::string type = typeStackTop;
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push(type.substr(0, type.size() - 1));
                append("scl_assert(stack.data[stack.ptr - 1].i, \"Not nil assertion failed!\");\n");
            } else {
                transpilerError("Unnecessary assert-not-nil operator '!!' on not-nil type '" + typeStackTop + "'", i);
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
                append("scl_assert(stack.data[stack.ptr - 1].i, \"Not nil assertion failed! If you see this, something has gone very wrong!\");\n");
            }
        } else if (body[i].getValue() == "?") {
            if (typeCanBeNil(typeStackTop)) {
                std::string type = typeStackTop;
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push(type.substr(0, type.size() - 1));
            }
            if (!typeCanBeNil(function->getReturnType()) && return_type != "void") {
                transpilerError("Return-if-nil operator '?' behaves like assert-not-nil operator '!!' in not-nil returning function.", i);
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
                append("scl_assert(stack.data[stack.ptr - 1].i, \"Not nil assertion failed!\");\n");
            } else {
                append("if (stack.data[stack.ptr - 1].i == 0) {\n");
                scopeDepth++;
                append("callstk.ptr--;\n");
                if (return_type == "void")
                    append("return;\n");
                else {
                    append("return 0;\n");
                }
                scopeDepth--;
                append("}\n");
            }
        } else if (body[i].getValue() == "||") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "||", typeStackTop)) return;
            append("stack.ptr -= 2;\n");
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i || stack.data[stack.ptr + 1].i;\n");
            debugPrintPush();
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "<") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "<", typeStackTop)) return;
            append("stack.ptr -= 2;\n");
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i < stack.data[stack.ptr + 1].i;\n");
            debugPrintPush();
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == ">") {
            if (handleOverriddenOperator(result, fp, scopeDepth, ">", typeStackTop)) return;
            append("stack.ptr -= 2;\n");
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i > stack.data[stack.ptr + 1].i;\n");
            debugPrintPush();
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "==") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "==", typeStackTop)) return;
            append("stack.ptr -= 2;\n");
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i == stack.data[stack.ptr + 1].i;\n");
            debugPrintPush();
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "<=") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "<=", typeStackTop)) return;
            append("stack.ptr -= 2;\n");
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i <= stack.data[stack.ptr + 1].i;\n");
            debugPrintPush();
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == ">=") {
            if (handleOverriddenOperator(result, fp, scopeDepth, ">=", typeStackTop)) return;
            append("stack.ptr -= 2;\n");
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i >= stack.data[stack.ptr + 1].i;\n");
            debugPrintPush();
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "!=") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "!=", typeStackTop)) return;
            append("stack.ptr -= 2;\n");
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i != stack.data[stack.ptr + 1].i;\n");
            debugPrintPush();
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "++") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "++", typeStackTop)) return;
            append("stack.data[stack.ptr - 1].i++;\n");
            if (typeStack.size() == 0)
                typeStack.push("int");
        } else if (body[i].getValue() == "--") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "--", typeStackTop)) return;
            append("stack.data[stack.ptr - 1].i--;\n");
            if (typeStack.size() == 0)
                typeStack.push("int");
        } else if (body[i].getValue() == "exit") {
            append("exit(stack.data[--stack.ptr].i);\n");
            if (typeStack.size())
                typeStack.pop();
        } else if (body[i].getValue() == "puts") {
            append("puts(stack.data[--stack.ptr].s);\n");
            if (typeStack.size())
                typeStack.pop();
        } else if (body[i].getValue() == "eputs") {
            append("fprintf(stderr, \"%%s\\n\", stack.data[--stack.ptr].s);\n");
            if (typeStack.size())
                typeStack.pop();
        } else if (body[i].getValue() == "abort") {
            append("abort();\n");
        } else if (body[i].getValue() == "typeof") {
            ITER_INC;
            if (hasVar(body[i])) {
                append("stack.data[stack.ptr++].s = \"%s\";\n", getVar(body[i]).getType().c_str());
                typeStack.push("str");
            } else {
                transpilerError("Unknown Variable: '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (body[i].getValue() == "nameof") {
            ITER_INC;
            if (hasVar(body[i])) {
                append("stack.data[stack.ptr++].s = \"%s\";\n", body[i].getValue().c_str());
                typeStack.push("str");
            } else {
                transpilerError("Unknown Variable: '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (body[i].getValue() == "sizeof") {
            ITER_INC;
            if (body[i].getValue() == "int") {
                append("stack.data[stack.ptr++].i = 8;\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "int") {
                append("stack.data[stack.ptr++].i = 8;\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "float") {
                append("stack.data[stack.ptr++].i = 8;\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "str") {
                append("stack.data[stack.ptr++].i = 8;\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "any") {
                append("stack.data[stack.ptr++].i = 8;\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "none") {
                append("stack.data[stack.ptr++].i = 0;\n");
                typeStack.push("int");
                return;
            }
            if (hasVar(body[i])) {
                append("stack.data[stack.ptr++].i = sizeof(%s);\n", sclTypeToCType(result, getVar(body[i]).getType()).c_str());
                typeStack.push("int");
            } else if (getStructByName(result, body[i].getValue()) != Struct("")) {
                append("stack.data[stack.ptr++].i = sizeof(%s);\n", sclTypeToCType(result, body[i].getValue()).c_str());
                typeStack.push("int");
            } else if (hasTypealias(result, body[i].getValue())) {
                append("stack.data[stack.ptr++].i = sizeof(%s);\n", sclTypeToCType(result, body[i].getValue()).c_str());
                typeStack.push("int");
            } else {
                transpilerError("Unknown Variable: '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
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
            bool argsCorrect = checkStackType(result, f->getArgs());
            if (!argsCorrect) {
                {
                    transpilerError("Arguments for function '" + sclFunctionNameToFriendlyString(f->getName()) + "' do not equal inferred stack!", i);
                    errors.push_back(err);
                }
                transpilerError("Expected: [" + argVectorToString(f->getArgs()) + "], but got: [" + stackSliceToString(f->getArgs().size()) + "]", i);
                err.isNote = true;
                errors.push_back(err);
                return;
            }
            for (ssize_t m = f->getArgs().size() - 1; m >= 0; m--) {
                if (typeStack.size())
                    typeStack.pop();
            }
            if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                if (f->getReturnType() == "float") {
                    append("stack.data[stack.ptr++].f = Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                    debugPrintPush();
                } else {
                    append("stack.data[stack.ptr++].v = (scl_any) Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                    debugPrintPush();
                }
                typeStack.push(f->getReturnType());
            } else {
                append("Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
            }
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
            typeStack.push(container.getMemberType(memberName) + (!container.getMember(memberName).canBeNil ? "" : ""));
        } else if (getStructByName(result, body[i].getValue()) != Struct("")) {
            if (body[i + 1].getType() == tok_double_column) {
                std::string struct_ = body[i].getValue();
                ITER_INC;
                ITER_INC;
                Struct s = getStructByName(result, struct_);
                if ((body[i].getValue() == "new" || body[i].getValue() == "default") && sclIsProhibitedInit(s.getName())) {
                    transpilerError("Explicit instanciation of struct '_String' is not allowed!", i);
                    errors.push_back(err);
                    return;
                }
                if (body[i].getValue() == "new") {
                    if (!s.heapAllocAllowed()) {
                        transpilerError("Struct '" + struct_ + "' may not be instanciated using '" + struct_ + "::new'", i);
                        errors.push_back(err);
                        return;
                    }
                    append("stack.data[stack.ptr++].v = scl_alloc_struct(sizeof(struct Struct_%s), \"%s\", %llu);\n", struct_.c_str(), struct_.c_str(), hash1((char*) s.extends().c_str()));
                    append("scl_assert(stack.data[stack.ptr - 1].i, \"Failed to allocate memory for struct '%s'\");\n", struct_.c_str());
                    typeStack.push(struct_);
                    debugPrintPush();
                    if (hasMethod(result, Token(tok_identifier, "init", 0, ""), struct_)) {
                        append("{\n");
                        scopeDepth++;
                        Method* f = getMethodByName(result, "init", struct_);
                        append("struct Struct_%s* tmp = (struct Struct_%s*) stack.data[stack.ptr - 1].v;\n", struct_.c_str(), struct_.c_str());
                        debugPrintPush();
                        if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
                        bool argsCorrect = checkStackType(result, f->getArgs());
                        if (!argsCorrect) {
                            {
                                transpilerError("Arguments for method '" + f->getMemberType() + ":" + sclFunctionNameToFriendlyString(f->getName()) + "' do not equal inferred stack!", i);
                                errors.push_back(err);
                            }
                            transpilerError("Expected: [" + argVectorToString(f->getArgs()) + "], but got: [" + stackSliceToString(f->getArgs().size()) + "]", i);
                            err.isNote = true;
                            errors.push_back(err);
                            return;
                        }
                        for (ssize_t m = f->getArgs().size() - 1; m >= 0; m--) {
                            if (typeStack.size())
                                typeStack.pop();
                        }
                        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                            if (f->getReturnType() == "float") {
                                append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            } else {
                                append("stack.data[stack.ptr++].v = (scl_any) Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            }
                            typeStack.push(f->getReturnType());
                        } else {
                            append("Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                        }
                        append("stack.data[stack.ptr++].v = tmp;\n");
                        typeStack.push(struct_);
                        debugPrintPush();
                        scopeDepth--;
                        append("}\n");
                    }
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
                    typeStack.push(struct_);
                    debugPrintPush();
                    if (hasMethod(result, Token(tok_identifier, "init", 0, ""), struct_)) {
                        Method* f = getMethodByName(result, "init", struct_);
                        if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
                        bool argsCorrect = checkStackType(result, f->getArgs());
                        if (!argsCorrect) {
                            {
                                transpilerError("Arguments for method '" + f->getMemberType() + ":" + sclFunctionNameToFriendlyString(f->getName()) + "' do not equal inferred stack!", i);
                                errors.push_back(err);
                            }
                            transpilerError("Expected: [" + argVectorToString(f->getArgs()) + "], but got: [" + stackSliceToString(f->getArgs().size()) + "]", i);
                            err.isNote = true;
                            errors.push_back(err);
                            return;
                        }
                        for (ssize_t m = f->getArgs().size() - 1; m >= 0; m--) {
                            if (typeStack.size())
                                typeStack.pop();
                        }
                        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                            if (f->getReturnType() == "float") {
                                append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            } else {
                                append("stack.data[stack.ptr++].v = (scl_any) Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            }
                            typeStack.push(f->getReturnType());
                        } else {
                            append("Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                        }
                        append("stack.data[stack.ptr++].v = (scl_any*) &tmp;\n");
                        typeStack.push(struct_);
                        debugPrintPush();
                    }
                    scopeDepth--;
                    append("}\n");
                } else {
                    if (hasFunction(result, Token(tok_identifier, struct_ + "$" + body[i].getValue(), 0, ""))) {
                        Function* f = getFunctionByName(result, struct_ + "$" + body[i].getValue());
                        if (f->isMethod) {
                            transpilerError("'" + f->getName() + "' is not static!", i);
                            errors.push_back(err);
                            return;
                        }
                        if (f->isPrivate && ((!function->isMethod && !strstarts(function->getName(), struct_ + "$")) || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                            transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
                        bool argsCorrect = checkStackType(result, f->getArgs());
                        if (!argsCorrect) {
                            {
                                transpilerError("Arguments for function '" + sclFunctionNameToFriendlyString(f->getName()) + "' do not equal inferred stack!", i);
                                errors.push_back(err);
                            }
                            transpilerError("Expected: [" + argVectorToString(f->getArgs()) + "], but got: [" + stackSliceToString(f->getArgs().size()) + "]", i);
                            err.isNote = true;
                            errors.push_back(err);
                            return;
                        }
                        for (ssize_t m = f->getArgs().size() - 1; m >= 0; m--) {
                            if (typeStack.size())
                                typeStack.pop();
                        }
                        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                            if (f->getReturnType() == "float") {
                                append("stack.data[stack.ptr++].f = Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            } else {
                                append("stack.data[stack.ptr++].v = (scl_any) Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                                debugPrintPush();
                            }
                            typeStack.push(f->getReturnType());
                        } else {
                            append("Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                        }
                    } else if (hasGlobal(result, struct_ + "$" + body[i].getValue())) {
                        std::string loadFrom = struct_ + "$" + body[i].getValue();
                        Variable v = getVar(Token(tok_identifier, loadFrom, 0, ""));
                        if ((body[i].getValue().at(0) == '_' || v.isPrivate) && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != struct_))) {
                            transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + struct_ + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (v.getType() == "float") {
                            append("stack.data[stack.ptr++].f = Var_%s;\n", loadFrom.c_str());
                            debugPrintPush();
                        } else {
                            append("stack.data[stack.ptr++].v = (scl_any) Var_%s;\n", loadFrom.c_str());
                            debugPrintPush();
                        }
                        typeStack.push(v.getType());
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
            typeStack.push(v.getType());
        } else if (function->isMethod) {
            Method* m = static_cast<Method*>(function);
            Struct s = getStructByName(result, m->getMemberType());
            if (hasMethod(result, body[i], s.getName())) {
                Method* f = getMethodByName(result, body[i].getValue(), s.getName());
                append("stack.data[stack.ptr++].v = (scl_any) Var_self;\n");
                typeStack.push(f->getMemberType());
                if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
                bool argsCorrect = checkStackType(result, f->getArgs());
                if (!argsCorrect) {
                    {
                        transpilerError("Arguments for method '" + f->getMemberType() + ":" + sclFunctionNameToFriendlyString(f->getName()) + "' do not equal inferred stack!", i);
                        errors.push_back(err);
                    }
                    transpilerError("Expected: [" + argVectorToString(f->getArgs()) + "], but got: [" + stackSliceToString(f->getArgs().size()) + "]", i);
                    err.isNote = true;
                    errors.push_back(err);
                    return;
                }
                for (ssize_t m = f->getArgs().size() - 1; m >= 0; m--) {
                    if (typeStack.size())
                        typeStack.pop();
                }
                debugPrintPush();
                if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                    if (f->getReturnType() == "float") {
                        append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                        debugPrintPush();
                    } else {
                        append("stack.data[stack.ptr++].v = (scl_any) Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                        debugPrintPush();
                    }
                    typeStack.push(f->getReturnType());
                } else {
                    append("Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                }
            } else if (hasFunction(result, Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""))) {
                std::string struct_ = s.getName();
                ITER_INC;
                if (hasFunction(result, Token(tok_identifier, struct_ + "$" + body[i].getValue(), 0, ""))) {
                    Function* f = getFunctionByName(result, struct_ + "$" + body[i].getValue());
                    if (f->isMethod) {
                        transpilerError("'" + f->getName() + "' is not static!", i);
                        errors.push_back(err);
                        return;
                    }
                    if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
                    bool argsCorrect = checkStackType(result, f->getArgs());
                    if (!argsCorrect) {
                        {
                            transpilerError("Arguments for function '" + sclFunctionNameToFriendlyString(f->getName()) + "' do not equal inferred stack!", i);
                            errors.push_back(err);
                        }
                        transpilerError("Expected: [" + argVectorToString(f->getArgs()) + "], but got: [" + stackSliceToString(f->getArgs().size()) + "]", i);
                        err.isNote = true;
                        errors.push_back(err);
                        return;
                    }
                    for (ssize_t m = f->getArgs().size() - 1; m >= 0; m--) {
                        if (typeStack.size())
                            typeStack.pop();
                    }
                    if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                        if (f->getReturnType() == "float") {
                            append("stack.data[stack.ptr++].f = Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                            debugPrintPush();
                        } else {
                            append("stack.data[stack.ptr++].v = (scl_any) Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                            debugPrintPush();
                        }
                        typeStack.push(f->getReturnType());
                    } else {
                        append("Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());
                    }
                }
            } else if (s.hasMember(body[i].getValue())) {
                Variable mem = s.getMember(body[i].getValue());
                if (mem.getType() == "float") {
                    append("stack.data[stack.ptr++].f = Var_self->%s;\n", body[i].getValue().c_str());
                    debugPrintPush();
                } else {
                    append("stack.data[stack.ptr++].v = (scl_any) Var_self->%s;\n", body[i].getValue().c_str());
                    debugPrintPush();
                }
                typeStack.push(mem.getType());
            } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                std::string loadFrom = s.getName() + "$" + body[i].getValue();
                Variable v = getVar(Token(tok_identifier, loadFrom, 0, ""));
                if (v.getType() == "float") {
                    append("stack.data[stack.ptr++].f = Var_%s;\n", loadFrom.c_str());
                    debugPrintPush();
                } else {
                    append("stack.data[stack.ptr++].v = (scl_any) Var_%s;\n", loadFrom.c_str());
                    debugPrintPush();
                }
                typeStack.push(v.getType());
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
        ITER_INC;
        if (body[i].getType() != tok_number) {
            transpilerError("Expected integer, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            if (i + 1 < body.size() && body[i + 1].getType() != tok_do)
                goto lbl_repeat_do_tok_chk;
            ITER_INC;
            return;
        }
        if (i + 1 < body.size() && body[i + 1].getType() != tok_do) {
        lbl_repeat_do_tok_chk:
            transpilerError("Expected 'do', but got '" + body[i + 1].getValue() + "'", i+1);
            errors.push_back(err);
            ITER_INC;
            return;
        }
        append("for (scl_any %c = 0; %c < (scl_any) %s; %c++) {\n",
            'a' + repeat_depth,
            'a' + repeat_depth,
            body[i].getValue().c_str(),
            'a' + repeat_depth
        );
        repeat_depth++;
        scopeDepth++;
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
        was_rep.push_back(true);
        ITER_INC;
    }

    handler(For) {
        noUnused;
        ITER_INC;
        Token var = body[i];
        if (var.getType() != tok_identifier) {
            transpilerError("Expected identifier, but got: '" + var.getValue() + "'", i);
            errors.push_back(err);
        }
        ITER_INC;
        if (body[i].getType() != tok_in) {
            transpilerError("Expected 'in' keyword in for loop header, but got: '" + body[i].getValue() + "'", i);
            errors.push_back(err);
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
        if (typeStack.size())
            typeStack.pop();
        while (body[i].getType() != tok_step && body[i].getType() != tok_do) {
            handle(Token);
            ITER_INC;
        }
        append("__it%d.end = stack.data[--stack.ptr].i;\n", iterator_count);
        if (typeStack.size())
            typeStack.pop();
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
                } else if (body[i].getValue() == "-") {
                    iterator_direction = "--";
                } else {
                    transpilerError("Expected number or '-', but got '" + body[i].getValue() + "'", i);
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
            result.column = var.getColumn();
            result.value = var.getValue();
            result.type =  var.getType();
            warns.push_back(result);
        }
        if (hasContainer(Main.parser->getResult(), var)) {
            FPResult result;
            result.message = "Variable '" + var.getValue() + "' shadowed by container '" + var.getValue() + "'";
            result.success = false;
            result.line = var.getLine();
            result.in = var.getFile();
            result.column = var.getColumn();
            result.value = var.getValue();
            result.type =  var.getType();
            warns.push_back(result);
        }
        if (hasVar(var)) {
            FPResult result;
            result.message = "Variable '" + var.getValue() + "' is already declared and shadows it.";
            result.success = false;
            result.line = var.getLine();
            result.in = var.getFile();
            result.column = var.getColumn();
            result.value = var.getValue();
            result.type =  var.getType();
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
        
        if (!getStructByName(result, typeStackTop).implements("IIterable")) {
            transpilerError("Struct '" + typeStackTop + "' is not iterable!", i);
            errors.push_back(err);
            return;
        }
        append("struct Struct_%s* %s = (struct Struct_%s*) stack.data[--stack.ptr].v;\n", typeStackTop.c_str(), iterator_name.c_str(), typeStackTop.c_str());
        std::string type = typeStackTop;
        if (typeStack.size()) {typeStack.pop();
        }

        Method* beginMethod = getMethodByName(result, "begin", type);
        Method* nextMethod = getMethodByName(result, "next", type);
        Method* hasNextMethod = getMethodByName(result, "hasNext", type);
        std::string var_prefix = "";
        if (!hasVar(iter_var_tok)) {
            var_prefix = sclTypeToCType(result, nextMethod->getReturnType()) + " ";
        }
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
        vars[varDepth].push_back(Variable(iter_var_tok.getValue(), nextMethod->getReturnType()));
        std::string cType = sclTypeToCType(result, getVar(iter_var_tok).getType());
        append(
            "for (%sVar_%s = (%s) Method_%s_begin(%s); Method_%s_hasNext(%s); Var_%s = (%s) Method_%s_next(%s)) {\n",
            var_prefix.c_str(),
            iter_var_tok.getValue().c_str(),
            cType.c_str(),
            beginMethod->getMemberType().c_str(),
            iterator_name.c_str(),
            hasNextMethod->getMemberType().c_str(),
            iterator_name.c_str(),
            iter_var_tok.getValue().c_str(),
            cType.c_str(),
            nextMethod->getMemberType().c_str(),
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
            typeStack.push("any");
            debugPrintPush();
            ITER_INC;
        } else if (getStructByName(result, body[i].getValue()) != Struct("")) {
            ITER_INC;
            if (body[i].getType() != tok_double_column) {
                transpilerError("Expected '::', but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            std::string struct_ = body[i - 1].getValue();
            ITER_INC;
            if (hasFunction(result, Token(tok_identifier, struct_ + "$" + body[i].getValue(), 0, ""))) {
                Function* f = getFunctionByName(result, struct_ + "$" + body[i].getValue());
                if (f->isMethod) {
                    transpilerError("'" + f->getName() + "' is not static!", i);
                    errors.push_back(err);
                    return;
                }
                append("stack.data[stack.ptr++].v = (scl_any) &Function_%s;\n", f->getName().c_str());
                typeStack.push("any");
                debugPrintPush();
            } else if (hasGlobal(result, struct_ + "$" + body[i].getValue())) {
                std::string loadFrom = struct_ + "$" + body[i].getValue();
                Variable v = getVar(Token(tok_identifier, loadFrom, 0, ""));
                append("stack.data[stack.ptr++].v = (scl_any) &Var_%s;\n", loadFrom.c_str());
                typeStack.push("any");
                debugPrintPush();
            }
        } else if (hasVar(toGet)) {
            Variable v = getVar(body[i]);
            std::string loadFrom = v.getName();
            if (getStructByName(result, v.getType()) != Struct("")) {
                if (i + 1 < body.size() && body[i + 1].getType() == tok_column) {
                    if (!hasMethod(result, body[i], v.getType())) {
                        std::string help = "";
                        Struct s = getStructByName(result, v.getType());
                        if (s.hasMember(body[i].getValue())) {
                            help = ". Maybe you meant to use '.' instead of ':' here";
                        }
                        transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + v.getType() + "'" + help, i);
                        errors.push_back(err);
                        return;
                    }
                    Method* f = getMethodByName(result, body[i].getValue(), v.getType());
                    append("stack.data[stack.ptr++].v = (scl_any) &Method_%s_%s;\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());    
                    typeStack.push("any");
                    debugPrintPush();
                } else {
                    ITER_INC;
                    if (body[i].getType() != tok_dot) {
                        append("stack.data[stack.ptr++].v = (scl_any) &Var_%s;\n", loadFrom.c_str());
                        debugPrintPush();
                        typeStack.push("any");
                        i--;
                        return;
                    }
                    ITER_INC;
                    Struct s = getStructByName(result, v.getType());
                    if (!s.hasMember(body[i].getValue())) {
                        std::string help = "";
                        if (getMethodByName(result, body[i].getValue(), s.getName())) {
                            help = ". Maybe you meant to use ':' instead of '.' here";
                        }
                        transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'" + help, i);
                        errors.push_back(err);
                        return;
                    }
                    Variable mem = s.getMember(body[i].getValue());
                    if ((body[i].getValue().at(0) == '_' || mem.isPrivate) && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                        transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    append("stack.data[stack.ptr++].v = (scl_any) &Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());
                    typeStack.push("any");
                    debugPrintPush();
                }
            } else {
                append("stack.data[stack.ptr++].v = (scl_any) &Var_%s;\n", loadFrom.c_str());
                typeStack.push("any");
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
            typeStack.push("any");
            debugPrintPush();
        } else {
            transpilerError("Unknown variable: '" + toGet.getValue() + "'", i+1);
            errors.push_back(err);
            return;
        }
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
                if (!container.getMember(memberName).isWritableFrom(function, VarAccess::Dereference)) {
                    transpilerError("Container member variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                    errors.push_back(err);
                    return;
                }
                append("*((scl_any*) Container_%s.%s) = stack.data[--stack.ptr].v;\n", containerName.c_str(), memberName.c_str());
                if (typeStack.size())
                    typeStack.pop();
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
                            Variable mem = s.getMember(body[i].getValue());
                            if (!mem.isWritableFrom(function, VarAccess::Dereference)) {
                                transpilerError("Member variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*(scl_any*) Var_self->%s = stack.data[--stack.ptr].v;\n", body[i].getValue().c_str());
                            if (typeStack.size())
                                typeStack.pop();
                            return;
                        } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                            Variable mem = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""));
                            if (!mem.isWritableFrom(function, VarAccess::Dereference)) {
                                transpilerError("Static variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*(scl_any*) Var_%s$%s = stack.data[--stack.ptr].v;\n", s.getName().c_str(), body[i].getValue().c_str());
                            if (typeStack.size())
                                typeStack.pop();
                            return;
                        }
                    }
                    ITER_INC;
                    ITER_INC;
                    i--;
                    i--;
                    if (body[i + 1].getType() == tok_column && body[i + 2].getType() == tok_column) {
                        i += 2;
                        Struct s = getStructByName(result, body[i - 2].getValue());
                        ITER_INC;
                        if (s != Struct("")) {
                            if (!hasVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""))) {
                                transpilerError("Struct '" + s.getName() + "' has no static member named '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            Variable mem = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""));
                            if (!mem.isWritableFrom(function, VarAccess::Dereference)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                                errors.push_back(err);
                                return;
                            }
                            std::string loadFrom = s.getName() + "$" + body[i].getValue();
                            if (mem.getType() == "float")
                                append("*((scl_any*) Var_%s) = stack.data[--stack.ptr].f;\n", loadFrom.c_str());
                            else
                                append("*((scl_any*) Var_%s) = (%s) stack.data[--stack.ptr].v;\n", loadFrom.c_str(), sclTypeToCType(result, mem.getType()).c_str());
                            if (typeStack.size())
                                typeStack.pop();
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
                    if (!v.isWritableFrom(function, VarAccess::Dereference)) {
                        transpilerError("Variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                        errors.push_back(err);
                        ITER_INC;
                        ITER_INC;
                        return;
                    }
                    ITER_INC;
                    if (body[i].getType() != tok_dot) {
                        append("*(scl_any*) Var_%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str());
                        if (typeStack.size())
                            typeStack.pop();
                        i--;
                        return;
                    }
                    if (!v.isWritableFrom(function, VarAccess::Dereference)) {
                        transpilerError("Variable '" + body[i - 1].getValue() + "' is not deref-writable in the current scope", i - 1);
                        errors.push_back(err);
                        ITER_INC;
                        return;
                    }
                    ITER_INC;
                    Struct s = getStructByName(result, v.getType());
                    if (!s.hasMember(body[i].getValue())) {
                        std::string help = "";
                        if (getMethodByName(result, body[i].getValue(), s.getName())) {
                            help = ". Maybe you meant to use ':' instead of '.' here";
                        }
                        transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'" + help, i);
                        errors.push_back(err);
                        return;
                    }
                    Variable mem = s.getMember(body[i].getValue());
                    if (!mem.isWritableFrom(function, VarAccess::Dereference)) {
                        transpilerError("Member variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                        errors.push_back(err);
                        ITER_INC;
                        ITER_INC;
                        return;
                    }
                    if ((body[i].getValue().at(0) == '_' || mem.isPrivate) && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                        transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    append("*(scl_any*) Var_%s->%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str(), body[i].getValue().c_str());
                } else {
                    if (!v.isWritableFrom(function, VarAccess::Dereference)) {
                        transpilerError("Variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                        errors.push_back(err);
                        return;
                    }
                    append("*(scl_any*) Var_%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str());
                }
                if (typeStack.size())
                    typeStack.pop();
            }
        } else if (body[i].getType() == tok_paren_open) {
            append("{\n");
            scopeDepth++;
            append("struct Struct_Array* tmp = (struct Struct_Array*) stack.data[--stack.ptr].v;\n");
            if (typeStack.size())
                typeStack.pop();
            ITER_INC;
            int destructureIndex = 0;
            while (body[i].getType() != tok_paren_close) {
                if (body[i].getType() == tok_comma) {
                    ITER_INC;
                    continue;
                }
                if (body[i].getType() == tok_paren_close)
                    break;
                
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
                        if (!container.getMember(memberName).isWritableFrom(function, VarAccess::Dereference)) {
                            transpilerError("Container member variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                            errors.push_back(err);
                            ITER_INC;
                            ITER_INC;
                            return;
                        }
                        append("*((scl_any*) Container_%s.%s) = Method_Array_get(tmp, %d);\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                    } else {
                        if (body[i].getType() != tok_identifier) {
                            transpilerError("'" + body[i].getValue() + "' is not an identifier!", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!hasVar(body[i])) {
                            if (function->isMethod) {
                                Method* m = static_cast<Method*>(function);
                                Struct s = getStructByName(result, m->getMemberType());
                                if (s.hasMember(body[i].getValue())) {
                                    Variable mem = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""));
                                    if (!mem.isWritableFrom(function, VarAccess::Dereference)) {
                                        transpilerError("Variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("*(scl_any*) Var_self->%s = Method_Array_get(tmp, %d);\n", body[i].getValue().c_str(), destructureIndex);
                                    // ITER_INC;
                                    destructureIndex++;
                                    continue;
                                } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                                    Variable mem = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""));
                                    if (!mem.isWritableFrom(function, VarAccess::Dereference)) {
                                        transpilerError("Variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("*(scl_any*) Var_%s$%s = Method_Array_get(tmp, %d);\n", s.getName().c_str(), body[i].getValue().c_str(), destructureIndex);
                                    // ITER_INC;
                                    destructureIndex++;
                                    continue;
                                }
                            }
                            transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        Variable v = getVar(body[i]);
                        std::string loadFrom = v.getName();
                        if (getStructByName(result, v.getType()) != Struct("")) {
                            if (!v.isWritableFrom(function, VarAccess::Dereference)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                                errors.push_back(err);
                                ITER_INC;
                                ITER_INC;
                                return;
                            }
                            ITER_INC;
                            if (body[i].getType() != tok_dot) {
                                append("*(scl_any*) Var_%s = Method_Array_get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                                // ITER_INC;
                                destructureIndex++;
                                continue;
                            }
                            if (!v.isWritableFrom(function, VarAccess::Dereference)) {
                                transpilerError("Variable '" + body[i - 1].getValue() + "' is not deref-writable in the current scope", i - 1);
                                errors.push_back(err);
                                ITER_INC;
                                return;
                            }
                            ITER_INC;
                            Struct s = getStructByName(result, v.getType());
                            if (!s.hasMember(body[i].getValue())) {
                                std::string help = "";
                                if (getMethodByName(result, body[i].getValue(), s.getName())) {
                                    help = ". Maybe you meant to use ':' instead of '.' here";
                                }
                                transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'" + help, i);
                                errors.push_back(err);
                                return;
                            }
                            Variable mem = s.getMember(body[i].getValue());
                            if (!mem.isWritableFrom(function, VarAccess::Dereference)) {
                                transpilerError("Member variable '" + body[i].getValue() + "' is not deref-writable in the current scope", i);
                                errors.push_back(err);
                                return;
                            }
                            if ((body[i].getValue().at(0) == '_' || mem.isPrivate) && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                                transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*(scl_any*) Var_%s->%s = Method_Array_get(tmp, %d);\n", loadFrom.c_str(), body[i].getValue().c_str(), destructureIndex);
                        } else {
                            if (!v.isWritableFrom(function, VarAccess::Dereference)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                                errors.push_back(err);
                                return;
                            }
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
                        if (!container.getMember(memberName).isWritableFrom(function, VarAccess::Write)) {
                            transpilerError("Container member variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!typeCanBeNil(container.getMember(memberName).getType())) {
                            append("scl_assert((scl_int) Method_Array_get(tmp, %d), \"Nil cannot be stored in non-nil variable '%s.%s'\");\n", destructureIndex, containerName.c_str(), memberName.c_str());
                        }
                        append("(*(scl_any*) &Container_%s.%s) = Method_Array_get(tmp, %d);\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                    } else {
                        if (body[i].getType() != tok_identifier) {
                            transpilerError("'" + body[i].getValue() + "' is not an identifier!", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!hasVar(body[i])) {
                            if (function->isMethod) {
                                Method* m = static_cast<Method*>(function);
                                Struct s = getStructByName(result, m->getMemberType());
                                if (s.hasMember(body[i].getValue())) {
                                    Variable mem = s.getMember(body[i].getValue());
                                    if (!mem.isWritableFrom(function, VarAccess::Write)) {
                                        transpilerError("Member variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    if (!typeCanBeNil(mem.getType())) {
                                        append("scl_assert((scl_int) Method_Array_get(tmp, %d), \"Nil cannot be stored in non-nil variable 'self.%s'\");\n", destructureIndex, mem.getName().c_str());
                                    }
                                    append("*(scl_any*) &Var_self->%s = Method_Array_get(tmp, %d);\n", body[i].getValue().c_str(), destructureIndex);
                                    // ITER_INC;
                                    destructureIndex++;
                                    continue;
                                } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                                    if (!getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, "")).isWritableFrom(function, VarAccess::Write)) {
                                        transpilerError("Member variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    if (!typeCanBeNil(getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, "")).getType())) {
                                        append("scl_assert((scl_int) Method_Array_get(tmp, %d), \"Nil cannot be stored in non-nil variable '%s::%s'\");\n", destructureIndex, s.getName().c_str(), body[i].getValue().c_str());
                                    }
                                    append("*(scl_any*) &Var_%s$%s = Method_Array_get(tmp, %d);\n", s.getName().c_str(), body[i].getValue().c_str(), destructureIndex);
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
                            if (!v.isWritableFrom(function, VarAccess::Write)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                                errors.push_back(err);
                                ITER_INC;
                                ITER_INC;
                                return;
                            }
                            ITER_INC;
                            if (body[i].getType() != tok_dot) {
                                if (!typeCanBeNil(v.getType())) {
                                    append("scl_assert((scl_int) Method_Array_get(tmp, %d), \"Nil cannot be stored in non-nil variable '%s'\");\n", destructureIndex, v.getName().c_str());
                                }
                                append("(*(scl_any*) &Var_%s) = Method_Array_get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                                // ITER_INC;
                                destructureIndex++;
                                continue;
                            }
                            if (!v.isWritableFrom(function, VarAccess::Dereference)) {
                                transpilerError("Variable '" + body[i - 1].getValue() + "' is not deref-writable in the current scope", i - 1);
                                errors.push_back(err);
                                ITER_INC;
                                return;
                            }
                            ITER_INC;
                            Struct s = getStructByName(result, v.getType());
                            if (!s.hasMember(body[i].getValue())) {
                                std::string help = "";
                                if (getMethodByName(result, body[i].getValue(), s.getName())) {
                                    help = ". Maybe you meant to use ':' instead of '.' here";
                                }
                                transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'" + help, i);
                                errors.push_back(err);
                                return;
                            }
                            Variable mem = s.getMember(body[i].getValue());
                            if (!mem.isWritableFrom(function, VarAccess::Write)) {
                                transpilerError("Member variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                                errors.push_back(err);
                                return;
                            }
                            if ((body[i].getValue().at(0) == '_' || mem.isPrivate) && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                                transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            if (!typeCanBeNil(mem.getType())) {
                                append("scl_assert((scl_int) Method_Array_get(tmp, %d), \"Nil cannot be stored in non-nil variable '%s.%s'\");\n", destructureIndex, s.getName().c_str(), mem.getName().c_str());
                            }
                            append("(*(scl_any*) &Var_%s->%s) = Method_Array_get(tmp, %d);\n", loadFrom.c_str(), body[i].getValue().c_str(), destructureIndex);
                        } else {
                            if (!v.isWritableFrom(function, VarAccess::Write)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                                errors.push_back(err);
                                return;
                            }
                            if (!typeCanBeNil(v.getType())) {
                                append("scl_assert((scl_int) Method_Array_get(tmp, %d), \"Nil cannot be stored in non-nil variable '%s'\");\n", destructureIndex, v.getName().c_str());
                            }
                            append("(*(scl_any*) &Var_%s) = Method_Array_get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                        }
                    }
                }
                ITER_INC;
                destructureIndex++;
            }
            if (destructureIndex == 0) {
                transpilerError("Empty Array destructure", i);
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
            }
            scopeDepth--;
            append("}\n");
        } else if (body[i].getType() == tok_declare) {
            ITER_INC;
            if (body[i].getType() != tok_identifier) {
                transpilerError("'" + body[i].getValue() + "' is not an identifier!", i+1);
                errors.push_back(err);
                return;
            }
            if (hasFunction(result, body[i])) {
                transpilerError("Variable '" + body[i].getValue() + "' shadowed by function '" + body[i].getValue() + "'", i+1);
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
            }
            if (hasContainer(result, body[i])) {
                transpilerError("Variable '" + body[i].getValue() + "' shadowed by container '" + body[i].getValue() + "'", i+1);
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
            }
            if (hasVar(body[i])) {
                transpilerError("Variable '" + body[i].getValue() + "' is already declared and shadows it.", i+1);
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
            }
            std::string name = body[i].getValue();
            if namingConvention("Variables", body[i], flatcase, camelCase, false)
            else if namingConvention("Variables", body[i], UPPERCASE, camelCase, false)
            else if namingConvention("Variables", body[i], PascalCase, camelCase, false)
            else if namingConvention("Variables", body[i], snake_case, camelCase, false)
            else if namingConvention("Variables", body[i], SCREAMING_SNAKE_CASE, camelCase, false)
            else if namingConvention("Variables", body[i], IPascalCase, camelCase, false)
            std::string type = "any";
            ITER_INC;
            bool isConst = false;
            bool isMut = false;
            bool typeWasInferred = false;
            if (body[i].getType() == tok_column) {
                ITER_INC;
                while (body[i].getValue() == "const" || body[i].getValue() == "mut") {
                    if (body[i].getValue() == "const") {
                        isConst = true;
                    } else if (body[i].getValue() == "mut") {
                        isMut = true;
                    }
                    ITER_INC;
                }
                FPResult r = parseType(body, &i);
                if (!r.success) {
                    errors.push_back(r);
                    return;
                }
                type = r.value;
            } else {
                // transpilerError("A type is required!", i);
                // errors.push_back(err);
                // return;
                type = typeStackTop;
                typeWasInferred = true;
                i--;
            }
            Variable v = Variable(name, type, isConst, isMut);
            if (!typeWasInferred && body[i + 1].getValue() == "?") {
                v.canBeNil = true;
                ITER_INC;
            }
            vars[varDepth].push_back(v);
            if (!typeCanBeNil(v.getType())) {
                append("scl_assert(stack.data[stack.ptr - 1].i, \"Nil cannot be stored in non-nil variable '%s'\");\n", v.getName().c_str());
            }
            append("%s Var_%s = (%s) stack.data[--stack.ptr].%s;\n", sclTypeToCType(result, v.getType()).c_str(), v.getName().c_str(), sclTypeToCType(result, v.getType()).c_str(), v.getType() == "float" ? "f" : "v");
            if (typeStack.size())
                typeStack.pop();
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
                if (!container.getMember(memberName).isWritableFrom(function, VarAccess::Write)) {
                    transpilerError("Container member variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                    errors.push_back(err);
                    return;
                }
                if (!typeCanBeNil(container.getMember(memberName).getType())) {
                    append("scl_assert(stack.data[stack.ptr - 1].i, \"Nil cannot be stored in non-nil variable '%s.%s'\");\n", containerName.c_str(), memberName.c_str());
                }
                append("Container_%s.%s = (%s) stack.data[--stack.ptr].%s;\n", containerName.c_str(), memberName.c_str(), sclTypeToCType(result, container.getMemberType(memberName)).c_str(), container.getMemberType(memberName) == "float" ? "f" : "v");
                if (typeStack.size())
                    typeStack.pop();
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
                            Variable mem = s.getMember(body[i].getValue());
                            if (!mem.isWritableFrom(function, VarAccess::Write)) {
                                transpilerError("Member variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                                errors.push_back(err);
                                return;
                            }
                            if (!typeCanBeNil(mem.getType())) {
                                append("scl_assert(stack.data[stack.ptr - 1].i, \"Nil cannot be stored in non-nil variable 'self.%s'\");\n", mem.getName().c_str());
                            }
                            if (mem.getType() == "float")
                                append("Var_self->%s = stack.data[--stack.ptr].f;\n", body[i].getValue().c_str());
                            else
                                append("Var_self->%s = (%s) stack.data[--stack.ptr].v;\n", body[i].getValue().c_str(), sclTypeToCType(result, mem.getType()).c_str());
                            if (typeStack.size())
                                typeStack.pop();
                            return;
                        } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                            Variable mem = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""));
                            if (!mem.isWritableFrom(function, VarAccess::Write)) {
                                transpilerError("Static member variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                                errors.push_back(err);
                                return;
                            }
                            if (!typeCanBeNil(mem.getType())) {
                                append("scl_assert(stack.data[stack.ptr - 1].i, \"Nil cannot be stored in non-nil variable '%s::%s'\");\n", s.getName().c_str(), mem.getName().c_str());
                            }
                            if (mem.getType() == "float")
                                append("Var_%s$%s = stack.data[--stack.ptr].f;\n", s.getName().c_str(), body[i].getValue().c_str());
                            else
                                append("Var_%s$%s = (%s) stack.data[--stack.ptr].v;\n", s.getName().c_str(), body[i].getValue().c_str(), sclTypeToCType(result, mem.getType()).c_str());
                            if (typeStack.size())
                                typeStack.pop();
                            return;
                        }
                    }
                    ITER_INC;
                    ITER_INC;
                    i--;
                    i--;
                    if (body[i + 1].getType() == tok_column && body[i + 2].getType() == tok_column) {
                        i += 2;
                        Struct s = getStructByName(result, body[i - 2].getValue());
                        ITER_INC;
                        if (s != Struct("")) {
                            if (!hasVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""))) {
                                transpilerError("Struct '" + s.getName() + "' has no static member named '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            Variable mem = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""));
                            if (!mem.isWritableFrom(function, VarAccess::Write)) {
                                transpilerError("Static member variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                                errors.push_back(err);
                                return;
                            }
                            std::string loadFrom = s.getName() + "$" + body[i].getValue();
                            if (!typeCanBeNil(mem.getType())) {
                                append("scl_assert(stack.data[stack.ptr - 1].i, \"Nil cannot be stored in non-nil variable '%s::%s'\");\n", s.getName().c_str(), body[i].getValue().c_str());
                            }
                            if (mem.getType() == "float")
                                append("Var_%s = stack.data[--stack.ptr].f;\n", loadFrom.c_str());
                            else
                                append("Var_%s = (%s) stack.data[--stack.ptr].v;\n", loadFrom.c_str(), sclTypeToCType(result, mem.getType()).c_str());
                            if (typeStack.size())
                                typeStack.pop();
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
                    if (!v.isWritableFrom(function, VarAccess::Write)) {
                        transpilerError("Variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                        errors.push_back(err);
                        ITER_INC;
                        ITER_INC;
                        return;
                    }
                    ITER_INC;
                    if (body[i].getType() != tok_dot) {
                        if (!typeCanBeNil(v.getType())) {
                            append("scl_assert(stack.data[stack.ptr - 1].i, \"Nil cannot be stored in non-nil variable '%s'\");\n", loadFrom.c_str());
                        }
                        append("Var_%s = stack.data[--stack.ptr].v;\n", loadFrom.c_str());
                        if (typeStack.size())
                            typeStack.pop();
                        i--;
                        return;
                    }
                    if (!v.isWritableFrom(function, VarAccess::Dereference)) {
                        transpilerError("Variable '" + body[i - 1].getValue() + "' is not deref-writable in the current scope", i - 1);
                        errors.push_back(err);
                        ITER_INC;
                        return;
                    }
                    ITER_INC;
                    Struct s = getStructByName(result, v.getType());
                    if (!s.hasMember(body[i].getValue())) {
                        std::string help = "";
                        if (getMethodByName(result, body[i].getValue(), s.getName())) {
                            help = ". Maybe you meant to use ':' instead of '.' here";
                        }
                        transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'" + help, i);
                        errors.push_back(err);
                        return;
                    }
                    Variable mem = s.getMember(body[i].getValue());
                    if (!mem.isWritableFrom(function, VarAccess::Write)) {
                        transpilerError("Member variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                        errors.push_back(err);
                        return;
                    }
                    if ((body[i].getValue().at(0) == '_' || mem.isPrivate) && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                        transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    if (!typeCanBeNil(mem.getType())) {
                        append("scl_assert(stack.data[stack.ptr - 1].i, \"Nil cannot be stored in non-nil variable '%s.%s'\");\n", loadFrom.c_str(), mem.getName().c_str());
                    }
                    if (mem.getType() == "float")
                        append("Var_%s->%s = stack.data[--stack.ptr].f;\n", loadFrom.c_str(), body[i].getValue().c_str());
                    else
                        append("Var_%s->%s = (%s) stack.data[--stack.ptr].v;\n", loadFrom.c_str(), body[i].getValue().c_str(), sclTypeToCType(result, mem.getType()).c_str());
                    if (typeStack.size())
                        typeStack.pop();
                } else {
                    Variable v = getVar(body[i]);
                    if (!v.isWritableFrom(function, VarAccess::Write)) {
                        transpilerError("Variable '" + body[i].getValue() + "' is not writable in the current scope", i);
                        errors.push_back(err);
                        return;
                    }
                    if (!typeCanBeNil(v.getType())) {
                        append("scl_assert(stack.data[stack.ptr - 1].i, \"Nil cannot be stored in non-nil variable '%s'\");\n", v.getName().c_str());
                    }
                    if (v.getType() == "float")
                        append("Var_%s = stack.data[--stack.ptr].f;\n", loadFrom.c_str());
                    else
                        append("Var_%s = (%s) stack.data[--stack.ptr].v;\n", loadFrom.c_str(), sclTypeToCType(result, v.getType()).c_str());
                    if (typeStack.size())
                        typeStack.pop();
                }
            }
        }
    }

    handler(Declare) {
        noUnused;
        ITER_INC;
        if (body[i].getType() != tok_identifier) {
            transpilerError("'" + body[i].getValue() + "' is not an identifier!", i+1);
            errors.push_back(err);
            return;
        }
        if (hasFunction(result, body[i])) {
            transpilerError("Variable '" + body[i].getValue() + "' shadowed by function '" + body[i].getValue() + "'", i+1);
            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
            else errors.push_back(err);
        }
        if (hasContainer(result, body[i])) {
            transpilerError("Variable '" + body[i].getValue() + "' shadowed by container '" + body[i].getValue() + "'", i+1);
            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
            else errors.push_back(err);
        }
        if (hasVar(body[i])) {
            transpilerError("Variable '" + body[i].getValue() + "' is already declared and shadows it.", i+1);
            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
            else errors.push_back(err);
        }
        std::string name = body[i].getValue();
        if namingConvention("Variables", body[i], flatcase, camelCase, false)
        else if namingConvention("Variables", body[i], UPPERCASE, camelCase, false)
        else if namingConvention("Variables", body[i], PascalCase, camelCase, false)
        else if namingConvention("Variables", body[i], snake_case, camelCase, false)
        else if namingConvention("Variables", body[i], SCREAMING_SNAKE_CASE, camelCase, false)
        else if namingConvention("Variables", body[i], IPascalCase, camelCase, false)
        std::string type = "any";
        ITER_INC;
        bool isConst = false;
        bool isMut = false;
        if (body[i].getType() == tok_column) {
            ITER_INC;
            while (body[i].getValue() == "const" || body[i].getValue() == "mut") {
                if (body[i].getValue() == "const") {
                    isConst = true;
                } else if (body[i].getValue() == "mut") {
                    isMut = true;
                }
                i++;
            }
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
        Variable v = Variable(name, type, isConst, isMut);
        if (body[i + 1].getValue() == "?") {
            v.canBeNil = true;
            ITER_INC;
        }
        vars[varDepth].push_back(v);
        append("%s Var_%s;\n", sclTypeToCType(result, v.getType()).c_str(), v.getName().c_str());
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
        Struct arr = getStructByName(result, "Array");
        append("struct Struct_Array* tmp = scl_alloc_struct(sizeof(struct Struct_Array), \"Array\", %llu);\n", hash1((char*) arr.getName().c_str()));
        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
            if (f->getReturnType() == "float") {
                append("stack.data[stack.ptr++].f = Method_Array_init(tmp, 1);\n");
                debugPrintPush();
            } else {
                append("stack.data[stack.ptr++].v = (scl_any) Method_Array_init(tmp, 1);\n");
                debugPrintPush();
            }
            typeStack.push(f->getReturnType());
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
            if (didPush) {
                append("Method_Array_push(tmp, stack.data[--stack.ptr].v);\n");
                if (typeStack.size())
                    typeStack.pop();
            }
        }
        append("stack.data[stack.ptr++].v = tmp;\n");
        typeStack.push("Array");
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
        Struct map = getStructByName(result, "Map");
        append("struct Struct_Map* tmp = scl_alloc_struct(sizeof(struct Struct_Map), \"Map\", %llu);\n", hash1((char*) map.getName().c_str()));
        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
            if (f->getReturnType() == "float") {
                append("stack.data[stack.ptr++].f = Method_Map_init(tmp, 1);\n");
                debugPrintPush();
            } else {
                append("stack.data[stack.ptr++].v = (scl_any) Method_Map_init(tmp, 1);\n");
                debugPrintPush();
            }
            typeStack.push(f->getReturnType());
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
            if (didPush) {
                append("Method_Map_set(tmp, stack.data[--stack.ptr].v, \"%s\");\n", key.c_str());
                if (typeStack.size())
                    typeStack.pop();
            }
        }
        append("stack.data[stack.ptr++].v = tmp;\n");
        typeStack.push("Map");
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
            Struct entry = getStructByName(result, "MapEntry");
            append("struct Struct_MapEntry* tmp = scl_alloc_struct(sizeof(struct Struct_MapEntry), \"MapEntry\", %llu);\n", hash1((char*) entry.getName().c_str()));
            if (typeStack.size())
                typeStack.pop();
            if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                if (f->getReturnType() == "float") {
                    append("stack.data[stack.ptr++].f = Method_MapEntry_init(tmp, stack.data[--stack.ptr].v, \"%s\");\n", key.c_str());
                    debugPrintPush();
                } else {
                    append("stack.data[stack.ptr++].v = (scl_any) Method_MapEntry_init(tmp, stack.data[--stack.ptr].v, \"%s\");\n", key.c_str());
                    debugPrintPush();
                }
                typeStack.push(f->getReturnType());
            } else {
                append("Method_MapEntry_init(tmp, stack.data[--stack.ptr].v, \"%s\");\n", key.c_str());
            }
            append("stack.data[stack.ptr++].v = tmp;\n");
            typeStack.push("MapEntry");
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
                Struct pair = getStructByName(result, "Pair");
                append("struct Struct_Pair* tmp = scl_alloc_struct(sizeof(struct Struct_Pair), \"Pair\", %llu);\n", hash1((char*) pair.getName().c_str()));
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
                    typeStack.push(f->getReturnType());
                } else {
                    if (i) {
                        append("Method_Pair_init(tmp, stack.data[stack.ptr + 1].v, stack.data[stack.ptr].v);\n");
                    } else {
                        append("Method_Pair_init(tmp, stack.data[stack.ptr + 1].v,\n");
                    }
                }
                append("stack.data[stack.ptr++].v = tmp;\n");
                typeStack.push("Pair");
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
                Struct triple = getStructByName(result, "Triple");
                append("struct Struct_Triple* tmp = scl_alloc_struct(sizeof(struct Struct_Triple), \"Triple\", %llu);\n", hash1((char*) triple.getName().c_str()));
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
                    typeStack.push(f->getReturnType());
                } else {
                    if (i) {
                        append("Method_Triple_init(tmp, stack.data[stack.ptr + 2].v, stack.data[stack.ptr + 1].v, stack.data[stack.ptr].v);\n");
                    } else {
                        append("Method_Triple_init(tmp, stack.data[stack.ptr + 2].v, stack.data[stack.ptr + 1].v,\n");
                    }
                }
                append("stack.data[stack.ptr++].v = tmp;\n");
                typeStack.push("Triple");
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
        append("stack.data[stack.ptr++].s = \"%s\";\n", body[i].getValue().c_str());
        typeStack.push("str");
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
        for (size_t t = 0; t < typeStack.size(); t++) {
            if (typeStack.size())
                typeStack.pop();
        }
    }

    handler(ExternC) {
        noUnused;
        append("{// Start C\n");
        scopeDepth++;
        std::string ext = body[i].getValue();
        for (std::vector<Variable> lvl : vars) {
            for (Variable v : lvl) {
                append("%s* %s = &Var_%s;\n", sclTypeToCType(result, v.getType()).c_str(), v.getName().c_str(), v.getName().c_str());
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
        for (size_t t = 0; t < typeStack.size(); t++) {
            if (typeStack.size())
                typeStack.pop();
        }
    }

    handler(IntegerLiteral) {
        noUnused;
        FPResult numberHandled = handleNumber(fp, body[i], scopeDepth);
        if (!numberHandled.success) {
            errors.push_back(numberHandled);
            return;
        }
    }

    handler(FloatLiteral) {
        noUnused;
        FPResult numberHandled = handleDouble(fp, body[i], scopeDepth);
        if (!numberHandled.success) {
            errors.push_back(numberHandled);
            return;
        }
    }

    handler(FalsyType) {
        noUnused;
        append("stack.data[stack.ptr++].v = (scl_any) 0;\n");
        if (body[i].getType() == tok_nil) {
            typeStack.push("any");
        } else {
            typeStack.push("bool");
        }
        debugPrintPush();
    }

    handler(TruthyType) {
        noUnused;
        append("stack.data[stack.ptr++].v = (scl_any) 1;\n");
        typeStack.push("bool");
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
        if (struct_ == "str" || struct_ == "int" || struct_ == "float" || struct_ == "any") {
            append("stack.data[stack.ptr - 1].i = 1;\n");
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
            return;
        }
        if (getStructByName(result, struct_) == Struct("")) {
            transpilerError("Usage of undeclared struct '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        append("stack.data[stack.ptr - 1].i = stack.data[stack.ptr - 1].v && scl_struct_is_type(stack.data[stack.ptr - 1].v, 0x%016llx);\n", hash1((char*) struct_.c_str()));
        if (typeStack.size())
            typeStack.pop();
        typeStack.push("bool");
    }

    handler(If) {
        for (size_t t = 0; t < typeStack.size(); t++) {
            if (typeStack.size())
                typeStack.pop();
        }
        noUnused;
        append("{\n");
        scopeDepth++;
        ITER_INC;
        while (body[i].getType() != tok_then) {
            handle(Token);
            ITER_INC;
        }
        append("\n");
        append("scl_int _passedCondition%zu = 0;\n", condCount++);
        append("if (stack.data[--stack.ptr].i) {\n");
        scopeDepth++;
        append("_passedCondition%zu = 1;\n\n", condCount - 1);

        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
        varDepth++;
    }

    handler(Else) {
        for (size_t t = 0; t < typeStack.size(); t++) {
            if (typeStack.size())
                typeStack.pop();
        }
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

    handler(Elif) {
        for (size_t t = 0; t < typeStack.size(); t++) {
            if (typeStack.size())
                typeStack.pop();
        }
        noUnused;
        scopeDepth--;
        varDepth--;
        vars.pop_back();
        
        append("}\n");
        ITER_INC;
        while (body[i].getType() != tok_then) {
            handle(Token);
            ITER_INC;
        }
        append("\n");
        append("if (_passedCondition%zu) stack.ptr--;\n", condCount - 1);
        append("else if (stack.data[--stack.ptr].i) {\n");
        scopeDepth++;
        append("_passedCondition%zu = 1;\n\n", condCount - 1);
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
    }

    handler(Fi) {
        for (size_t t = 0; t < typeStack.size(); t++) {
            if (typeStack.size())
                typeStack.pop();
        }
        noUnused;
        scopeDepth--;
        varDepth--;
        append("}\n");
        scopeDepth--;
        append("}\n");
        condCount--;
        vars.pop_back();
    }

    handler(While) {
        for (size_t t = 0; t < typeStack.size(); t++) {
            if (typeStack.size())
                typeStack.pop();
        }
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
        if (typeStack.size())
            typeStack.pop();
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
        append("callstk.ptr--;\n");

        if (return_type != "void") {
            if (!typeCanBeNil(function->getReturnType())) {
                if (function->hasNamedReturnValue) {
                    if (typeCanBeNil(function->getNamedReturnValue().getType())) {
                        transpilerError("Returning maybe-nil type '" + function->getNamedReturnValue().getType() + "' from function with not-nil return type '" + function->getReturnType() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                } else {
                    if (typeCanBeNil(typeStackTop)) {
                        transpilerError("Returning maybe-nil type '" + typeStackTop + "' from function with not-nil return type '" + function->getReturnType() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                }
                if (function->hasNamedReturnValue)
                    append("scl_assert(*(scl_int*) &Var_%s, \"Tried returning nil from function returning not-nil type '%s'!\");\n", function->getNamedReturnValue().getName().c_str(), function->getReturnType().c_str());
                else
                    append("scl_assert(stack.data[stack.ptr - 1].i, \"Tried returning nil from function returning not-nil type '%s'!\");\n", function->getReturnType().c_str());
            }
        }

        if (return_type == "void")
            append("return;\n");
        else {
            if (function->hasNamedReturnValue) {
                std::string type = sclTypeToCType(result, function->getNamedReturnValue().getType());
                if (type == "scl__String") type = "scl_str";
                append("return (%s) Var_%s;\n", type.c_str(), function->getNamedReturnValue().getName().c_str());
            } else {
                if (return_type == "scl_str" || return_type == "scl__String") {
                    append("return stack.data[--stack.ptr].s;\n");
                } else if (return_type == "scl_int") {
                    append("return stack.data[--stack.ptr].i;\n");
                } else if (return_type == "scl_float") {
                    append("return stack.data[--stack.ptr].f;\n");
                } else if (return_type == "scl_any") {
                    append("return stack.data[--stack.ptr].v;\n");
                } else if (strncmp(return_type.c_str(), "scl_", 4) == 0 || hasTypealias(result, return_type)) {
                    std::string type = sclTypeToCType(result, function->getReturnType());
                    append("return (%s) stack.data[--stack.ptr].i;\n", type.c_str());
                }
            }
        }
        for (size_t t = 0; t < typeStack.size(); t++) {
            if (typeStack.size())
                typeStack.pop();
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
        if (typeStack.size())
            typeStack.pop();
        append("switch (stack.data[--stack.ptr].i) {\n");
        scopeDepth++;
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
        was_rep.push_back(false);
    }

    handler(AddrOf) {
        noUnused;
        if (handleOverriddenOperator(result, fp, scopeDepth, "@", typeStackTop)) return;
        append("stack.data[stack.ptr - 1].v = (*(scl_any*) stack.data[stack.ptr - 1].v);\n");
        std::string ptr = typeStackTop;
        if (typeStack.size())
            typeStack.pop();
        if (ptr.size()) {
            if (ptr.at(0) == '[' && ptr.at(ptr.size() - 1) == ']') {
                typeStack.push(ptr.substr(1, ptr.size() - 2));
                return;
            } else if (ptr == "str") {
                typeStack.push("int");
                return;
            }
        }
        typeStack.push("any");
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
        FPResult type = parseType(body, &i);
        if (!type.success) {
            errors.push_back(type);
            return;
        }
        if (type.value == "int" || type.value == "float" || type.value == "str" || type.value == "any") {
            if (typeStack.size())
                typeStack.pop();
            typeStack.push(type.value);
            return;
        }
        if (getStructByName(result, type.value) == Struct("")) {
            transpilerError("Use of undeclared Struct '" + type.value + "'", i);
            errors.push_back(err);
            return;
        }
        if (i + 1 < body.size() && body[i + 1].getValue() == "?") {
            if (!typeCanBeNil(typeStackTop)) {
                transpilerError("Unneccessary cast to maybe-nil type '" + type.value + "?' from not-nil type '" + typeStackTop + "'", i - 1);
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
            }
            if (typeStack.size())
                typeStack.pop();
            typeStack.push(type.value + "?");
            ITER_INC;
        } else {
            if (typeCanBeNil(typeStackTop)) {
                append("scl_assert(stack.data[stack.ptr - 1].i, \"Nil cannot be cast to non-nil type '%s!'\");\n", type.value.c_str());
            }
            typeStack.push(type.value);
        }
    }

    handler(Dot) {
        noUnused;
        Struct s = getStructByName(result, typeStackTop);
        if (s == Struct("")) {
            transpilerError("Cannot infer type of stack top: expected valid Struct, but got '" + typeStackTop + "'", i);
            errors.push_back(err);
            return;
        }
        ITER_INC;
        if (!s.hasMember(body[i].getValue())) {
            std::string help = "";
            if (getMethodByName(result, body[i].getValue(), s.getName())) {
                help = ". Maybe you meant to use ':' instead of '.' here";
            }
            transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'" + help, i);
            errors.push_back(err);
            return;
        }
        Variable mem = s.getMember(body[i].getValue());
        if ((body[i].getValue().at(0) == '_' || mem.isPrivate) && (!function->isMethod || static_cast<Method*>(function)->getMemberType() != s.getName())) {
            transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
            errors.push_back(err);
            return;
        }
        if (typeCanBeNil(typeStackTop)) {
            {
                transpilerError("Member access on maybe-nil type '" + typeStackTop + "'", i);
                errors.push_back(err);
            }
            transpilerError("If you know '" + typeStackTop + "' can't be nil at this location, add '!!' before this '.'", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        if (mem.getType() == "float")
            append("stack.data[stack.ptr - 1].f = ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", s.getName().c_str(), body[i].getValue().c_str());
        else
            append("stack.data[stack.ptr - 1].v = (scl_any) ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", s.getName().c_str(), body[i].getValue().c_str());
        if (typeStack.size())
            typeStack.pop();
        typeStack.push(mem.getType());
    }

    handler(Column) {
        noUnused;
        Struct s = getStructByName(result, typeStackTop);
        if (s == Struct("")) {
            transpilerError("Cannot infer type of stack top: expected valid Struct, but got '" + typeStackTop + "'", i);
            errors.push_back(err);
            return;
        }
        ITER_INC;
        if (!hasMethod(result, body[i], typeStackTop)) {
            std::string help = "";
            if (s.hasMember(body[i].getValue())) {
                help = ". Maybe you meant to use '.' instead of ':' here";
            }
            transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + typeStackTop + "'" + help, i);
            errors.push_back(err);
            return;
        }
        if (typeCanBeNil(typeStackTop)) {
            {
                transpilerError("Calling method on maybe-nil type '" + typeStackTop + "'", i);
                errors.push_back(err);
            }
            transpilerError("If you know '" + typeStackTop + "' can't be nil at this location, add '!!' before this ':'\\insertText;!!;" + std::to_string(err.line) + ":" + std::to_string(err.column), i - 1);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        Method* f = getMethodByName(result, body[i].getValue(), typeStackTop);
        if (f->isPrivate && (!function->isMethod || static_cast<Method*>(function)->getMemberType() != s.getName())) {
            transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
            errors.push_back(err);
            return;
        }
        if (f->getArgs().size() > 0) append("stack.ptr -= %zu;\n", f->getArgs().size());
        bool argsCorrect = checkStackType(result, f->getArgs());
        if (!argsCorrect) {
            {
                transpilerError("Arguments for method '" + f->getMemberType() + ":" + sclFunctionNameToFriendlyString(f->getName()) + "' do not equal inferred stack!", i);
                errors.push_back(err);
            }
            transpilerError("Expected: [" + argVectorToString(f->getArgs()) + "], but got: [" + stackSliceToString(f->getArgs().size()) + "]", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        for (ssize_t m = f->getArgs().size() - 1; m >= 0; m--) {
            if (typeStack.size())
                typeStack.pop();
        }
        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
            if (f->getReturnType() == "float") {
                append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                debugPrintPush();
            } else {
                append("stack.data[stack.ptr++].v = (scl_any) Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
                debugPrintPush();
            }
            typeStack.push(f->getReturnType());
        } else {
            append("Method_%s_%s(%s);\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());
        }
    }

    handler(Token) {
        noUnused;

        std::string file = body[i].getFile();

        append("current_file = \"%s\";\n", std::filesystem::path(body[i].getFile()).filename().c_str());
        append("current_line = %d;\n", body[i].getLine());
        append("current_col = %d;\n", body[i].getColumn());

        if (isOperator(body[i])) {
            FPResult operatorsHandled = handleOperator(result, fp, body[i], scopeDepth);
            if (!operatorsHandled.success) {
                errors.push_back(operatorsHandled);
            }
        } else {
            switch (body[i].getType()) {
                case tok_question_mark:
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

                case tok_elif: {
                    handle(Elif);
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

                case tok_fi: {
                    handle(Fi);
                    break;
                }

                case tok_done:
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

    std::string sclFunctionNameToFriendlyString(std::string name) {
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_add", "+");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_sub", "-");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_mul", "*");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_div", "/");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_mod", "%");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_logic_and", "&");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_logic_or", "|");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_logic_xor", "^");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_logic_not", "~");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_logic_lsh", "<<");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_logic_rsh", ">>");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_pow", "**");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_dot", ".");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_less", "<");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_less_equal", "<=");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_more", ">");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_more_equal", ">=");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_equal", "==");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_not", "!");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_assert_not_nil", "!!");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_not_equal", "!=");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_bool_and", "&&");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_bool_or", "||");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_inc", "++");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_dec", "--");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_at", "@");
        name = replaceAll(name, "operator_" + Main.options.operatorRandomData + "_wildcard", "?");
        name = replaceAll(name, "\\$", "::");
        return name;
    }

    std::string sclGenCastForMethod(TPResult result, Method* m) {
        std::string return_type = sclTypeToCType(result, m->getReturnType());
        std::string arguments = "";
        if (m->getArgs().size() > 0) {
            for (ssize_t i = (ssize_t) m->getArgs().size() - 1; i >= 0; i--) {
                Variable var = m->getArgs()[i];
                arguments += sclTypeToCType(result, var.getType());
                if (i) {
                    arguments += ", ";
                }
            }
        }
        return return_type + "(*)(" + arguments + ")";
    }

    extern "C" void ConvertC::writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<Variable>& globals, TPResult result, std::string filename) {
        (void) warns;
        scopeDepth = 0;
        append("/* EXTERN VARS FROM INTERNAL */\n");
        append("extern scl_stack_t stack;\n");
        append("extern scl_stack_t callstk;\n\n");

        append("/* STRUCTS */\n");
        append("struct scltype_iterable {\n");
        scopeDepth++;
        append("scl_int start;\n");
        append("scl_int end;\n");
        scopeDepth--;
        append("};\n");
        append("struct scl_methodinfo {\n");
        scopeDepth++;
        append("scl_int  $__type__;\n");
        append("scl_str  $__type_name__;\n");
        append("scl_int  $__super__;\n");
        append("scl_int  $__size__;\n");
        append("scl_int  name_hash;\n");
        append("scl_str  name;\n");
        append("scl_int  id;\n");
        append("scl_any  ptr;\n");
        append("scl_int  member_type;\n");
        append("scl_int  arg_count;\n");
        append("scl_str  return_type;\n");
        scopeDepth--;
        append("};\n");
        append("struct scl_typeinfo {\n");
        scopeDepth++;
        append("scl_int    $__type__;\n");
        append("scl_str    $__type_name__;\n");
        append("scl_int    $__super__;\n");
        append("scl_int    $__size__;\n");
        append("scl_int    type;\n");
        append("scl_str    name;\n");
        append("scl_int    size;\n");
        append("scl_int    super;\n");
        append("scl_int    methodscount;\n");
        append("struct scl_methodinfo** methods;\n");
        scopeDepth--;
        append("};\n\n");

        fprintf(fp, "#ifdef __cplusplus\n");
        fprintf(fp, "}\n");
        fprintf(fp, "#endif\n");
        fprintf(fp, "#endif\n");

        fclose(fp);
        fp = fopen((filename.substr(0, filename.size() - 2) + ".typeinfo.h").c_str(), "a");
        fprintf(fp, "#include \"%s.h\"\n\n", filename.substr(0, filename.size() - 2).c_str());
        fprintf(fp, "#ifdef __cplusplus\n");
        fprintf(fp, "extern \"c\" {\n");
        fprintf(fp, "#endif\n\n");

        append("/* FUNCTION REFLECT */\n");
        scopeDepth++;
        for (Function* f : result.functions) {
            if (f->isMethod) continue;
            append("struct scl_methodinfo methodinfo_function_%s = (struct scl_methodinfo) {\n", f->getName().c_str());
            scopeDepth++;
            append(".$__type__ = 0x%016llx,\n", hash1((char*) std::string("Method").c_str()));
            append(".$__type_name__ = \"Method\",\n");
            append(".$__super__ = 258689265892916,\n");
            append(".$__size__ = sizeof(struct scl_methodinfo),\n");
            append(".name_hash = 0x%016llx,\n", hash1((char*) sclFunctionNameToFriendlyString(f->getName()).c_str()));
            append(".name = \"%s\",\n", sclFunctionNameToFriendlyString(f->getName()).c_str());
            append(".ptr = (scl_any) scl_reflect_call_function_%s,\n", f->getName().c_str());
            append(".id = 0x%016llx,\n", hash1((char*) f->getName().c_str()));
            append(".member_type = 0,\n");
            append(".arg_count = %zu,\n", f->getArgs().size());
            append(".return_type = \"%s\",\n", f->getReturnType().c_str());
            scopeDepth--;
            append("};\n");
        }
        for (Function* f : result.extern_functions) {
            if (f->isMethod) continue;
            append("struct scl_methodinfo methodinfo_function_%s = (struct scl_methodinfo) {\n", f->getName().c_str());
            scopeDepth++;
            append(".$__type__ = 0x%016llx,\n", hash1((char*) std::string("Method").c_str()));
            append(".$__type_name__ = \"Method\",\n");
            append(".$__super__ = 258689265892916,\n");
            append(".$__size__ = sizeof(struct scl_methodinfo),\n");
            append(".name_hash = 0x%016llx,\n", hash1((char*) sclFunctionNameToFriendlyString(f->getName()).c_str()));
            append(".name = \"%s\",\n", sclFunctionNameToFriendlyString(f->getName()).c_str());
            append(".ptr = (scl_any) scl_reflect_call_function_%s,\n", f->getName().c_str());
            append(".id = 0x%016llx,\n", hash1((char*) f->getName().c_str()));
            append(".member_type = 0,\n");
            append(".arg_count = %zu,\n", f->getArgs().size());
            append(".return_type = \"%s\",\n", f->getReturnType().c_str());
            scopeDepth--;
            append("};\n");
        }
        scopeDepth--;
        
        size_t fCount = 0;
        append("struct scl_methodinfo* scl_internal_functions[] = {\n");
        for (Function* f : result.functions) {
            if (f->isMethod) continue;
            fCount++;
            append("  &methodinfo_function_%s,\n", f->getName().c_str());
        }
        for (Function* f : result.extern_functions) {
            if (f->isMethod) continue;
            fCount++;
            append("  &methodinfo_function_%s,\n", f->getName().c_str());
        }
        append("};\n");
        append("size_t scl_internal_functions_size = %zu;\n\n", fCount);

        append("/* METHOD REFLECT */\n");
        scopeDepth++;
        for (Function* f : result.functions) {
            if (!f->isMethod) continue;
            Method* m = (Method*) f;
            append("struct scl_methodinfo methodinfo_method_%s_function_%s = (struct scl_methodinfo) {\n", m->getMemberType().c_str(), f->getName().c_str());
            scopeDepth++;
            append(".$__type__ = 0x%016llx,\n", hash1((char*) std::string("Method").c_str()));
            append(".$__type_name__ = \"Method\",\n");
            append(".$__super__ = 258689265892916,\n");
            append(".$__size__ = sizeof(struct scl_methodinfo),\n");
            append(".name_hash = 0x%016llx,\n", hash1((char*) (m->getMemberType() + ":" + sclFunctionNameToFriendlyString(f->getName())).c_str()));
            append(".name = \"%s:%s\",\n", m->getMemberType().c_str(), sclFunctionNameToFriendlyString(f->getName()).c_str());
            append(".ptr = (scl_any) scl_reflect_call_method_%s_function_%s,\n", m->getMemberType().c_str(), f->getName().c_str());
            append(".id = 0x%016llx,\n", hash1((char*) f->getName().c_str()));
            append(".member_type = 0x%016llx,\n", hash1((char*) m->getMemberType().c_str()));
            append(".arg_count = %zu,\n", f->getArgs().size());
            append(".return_type = \"%s\",\n", f->getReturnType().c_str());
            scopeDepth--;
            append("};\n");
        }
        for (Function* f : result.extern_functions) {
            if (!f->isMethod) continue;
            Method* m = (Method*) f;
            append("struct scl_methodinfo methodinfo_method_%s_function_%s = (struct scl_methodinfo) {\n", m->getMemberType().c_str(), f->getName().c_str());
            scopeDepth++;
            append(".$__type__ = 0x%016llx,\n", hash1((char*) std::string("Method").c_str()));
            append(".$__type_name__ = \"Method\",\n");
            append(".$__super__ = 258689265892916,\n");
            append(".$__size__ = sizeof(struct scl_methodinfo),\n");
            append(".name_hash = 0x%016llx,\n", hash1((char*) (m->getMemberType() + ":" + sclFunctionNameToFriendlyString(f->getName())).c_str()));
            append(".name = \"%s:%s\",\n", m->getMemberType().c_str(), sclFunctionNameToFriendlyString(f->getName()).c_str());
            append(".ptr = (scl_any) scl_reflect_call_method_%s_function_%s,\n", m->getMemberType().c_str(), f->getName().c_str());
            append(".id = 0x%016llx,\n", hash1((char*) f->getName().c_str()));
            append(".member_type = 0x%016llx,\n", hash1((char*) m->getMemberType().c_str()));
            append(".arg_count = %zu,\n", f->getArgs().size());
            append(".return_type = \"%s\",\n", f->getReturnType().c_str());
            scopeDepth--;
            append("};\n");
        }
        scopeDepth--;
        append("struct scl_methodinfo* scl_internal_methods[] = {\n");
        fCount = 0;
        for (Function* f : result.functions) {
            if (!f->isMethod) continue;
            Method* m = (Method*) f;
            fCount++;
            append("  &methodinfo_method_%s_function_%s,\n", m->getMemberType().c_str(), f->getName().c_str());
        }
        for (Function* f : result.extern_functions) {
            if (!f->isMethod) continue;
            Method* m = (Method*) f;
            fCount++;
            append("  &methodinfo_method_%s_function_%s,\n", m->getMemberType().c_str(), f->getName().c_str());
        }
        append("};\n");
        append("size_t scl_internal_methods_size = %zu;\n\n", fCount);
        
        append("/* TYPES */\n");
        for (Struct s : result.structs) {
            std::vector<Method*> methods = methodsOnType(result, s.getName());
            append("/* %s */\n", s.getName().c_str());
            if (methods.size()) {
                append("struct scl_methodinfo* methodinfo_ptrs_%s[] = (struct scl_methodinfo*[]) {\n", s.getName().c_str());
                scopeDepth++;
                for (Method* m : methods) {
                    append("  &methodinfo_method_%s_function_%s,\n", m->getMemberType().c_str(), m->getName().c_str());
                }
                scopeDepth--;
                append("};\n");
            // } else {
            //     append("struct scl_methodinfo*[] methodinfo_ptrs_%s = {0};\n", s.getName().c_str());
            }
        }
        append("struct scl_typeinfo scl_internal_types[] = {\n");
        scopeDepth++;
        for (Struct s : result.structs) {
            append("/* %s */\n", s.getName().c_str());
            append("(struct scl_typeinfo) {\n");
            scopeDepth++;
            append(".$__type__ = 0x%016llx,\n", hash1((char*) std::string("Struct").c_str()));
            append(".$__type_name__ = \"Struct\",\n");
            append(".$__super__ = 258689265892916,\n");
            append(".$__size__ = sizeof(struct scl_typeinfo),\n");
            append(".type = 0x%016llx,\n", hash1((char*) s.getName().c_str()));
            append(".name = \"%s\",\n", s.getName().c_str());
            append(".size = sizeof(struct Struct_%s),\n", s.getName().c_str());
            append(".super = %llu,\n", hash1((char*) s.extends().c_str()));

            std::vector<Method*> methods = methodsOnType(result, s.getName());

            append(".methodscount = %zu,\n", methods.size());
            if (methods.size()) {
                append(".methods = (struct scl_methodinfo**) methodinfo_ptrs_%s,\n", s.getName().c_str());
            } else {
                append(".methods = NULL,\n");
            }
            scopeDepth--;
            append("},\n");
        }
        scopeDepth--;
        append("};\n");
        append("size_t scl_internal_types_count = %zu;\n\n", result.structs.size());
        append("int scl_do_method_check = 0;\n\n");

        append("extern unsigned long long hash1(char*);\n\n");

        fprintf(fp, "#ifdef __cplusplus\n");
        fprintf(fp, "}\n");
        fprintf(fp, "#endif\n");

        fclose(fp);
        fp = fopen(filename.c_str(), "a");
        fprintf(fp, "#include \"%s.h\"\n", filename.substr(0, filename.size() - 2).c_str());
        fprintf(fp, "#include \"%s.typeinfo.h\"\n\n", filename.substr(0, filename.size() - 2).c_str());
        fprintf(fp, "#ifdef __cplusplus\n");
        fprintf(fp, "extern \"c\" {\n");
        fprintf(fp, "#endif\n\n");

        append("scl_str current_file = \"<init>\";\n");
        append("scl_int current_line = 0;\n");
        append("scl_int current_col  = 0;\n");

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
            for (size_t t = 0; t < typeStack.size(); t++) {
                if (typeStack.size())
                    typeStack.pop();
            }

            scopeDepth = 0;
            noWarns = false;

            for (std::string modifier : function->getModifiers()) {
                if (modifier == "nowarn") {
                    noWarns = true;
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
                return_type = sclTypeToCType(result, t);
            } else {
                FPResult err;
                err.success = false;
                err.message = "Function '" + function->getName() + "' does not specify a return type, defaults to none.";
                err.line = function->getNameToken().getLine();
                err.in = function->getNameToken().getFile();
                err.column = function->getNameToken().getColumn();
                err.value = function->getNameToken().getValue();
                err.type = function->getNameToken().getType();
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
            }

            return_type = "void";

            std::string arguments = "";
            if (function->hasNamedReturnValue) {
                vars[varDepth].push_back(function->getNamedReturnValue());
            }
            if (function->getArgs().size() > 0) {
                for (ssize_t i = (ssize_t) function->getArgs().size() - 1; i >= 0; i--) {
                    Variable var = function->getArgs()[i];
                    vars[varDepth].push_back(var);
                    std::string type = sclTypeToCType(result, function->getArgs()[i].getType());
                    if (type == "_String") type = "str";
                    arguments += type + " Var_" + function->getArgs()[i].getName();
                    if (i) {
                        arguments += ", ";
                    }
                }
            }

            if (function->getReturnType().size() > 0) {
                std::string t = function->getReturnType();
                return_type = sclTypeToCType(result, t);
            }
            if (!function->isMethod) {
                append("void scl_reflect_call_function_%s() {\n", function->getName().c_str());
                scopeDepth++;
                if (function->getArgs().size() > 0)
                    append("stack.ptr -= %zu;\n", function->getArgs().size());
                if (return_type == "void") {
                    append("Function_%s(%s);\n", function->getName().c_str(), sclGenArgs(result, function).c_str());
                } else {
                    if (function->getReturnType() == "float")
                        append("stack.data[stack.ptr++].f = Function_%s(%s);\n", function->getName().c_str(), sclGenArgs(result, function).c_str());
                    else
                        append("stack.data[stack.ptr++].v = (scl_any) Function_%s(%s);\n", function->getName().c_str(), sclGenArgs(result, function).c_str());
                }
                scopeDepth--;
                append("}\n");
                append("%s Function_%s(%s) {\n", return_type.c_str(), function->getName().c_str(), arguments.c_str());
            } else {
                if (!((Method*)(function))->addAnyway() && getStructByName(result, ((Method*)(function))->getMemberType()).isSealed()) {
                    FPResult result;
                    result.message = "Struct '" + ((Method*)(function))->getMemberType() + "' is sealed!";
                    result.value = function->getNameToken().getValue();
                    result.line = function->getNameToken().getLine();
                    result.in = function->getNameToken().getFile();
                    result.column = function->getNameToken().getColumn();
                    result.type = function->getNameToken().getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                Method* m = (Method*) function;
                append("void scl_reflect_call_method_%s_function_%s() {\n", m->getMemberType().c_str(), function->getName().c_str());
                scopeDepth++;
                if (function->getArgs().size() > 0)
                    append("stack.ptr -= %zu;\n", function->getArgs().size());
                if (return_type == "void") {
                    append("Method_%s_%s(%s);\n", m->getMemberType().c_str(), function->getName().c_str(), sclGenArgs(result, function).c_str());
                } else {
                    if (function->getReturnType() == "float")
                        append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", m->getMemberType().c_str(), function->getName().c_str(), sclGenArgs(result, function).c_str());
                    else
                        append("stack.data[stack.ptr++].v = (scl_any) Method_%s_%s(%s);\n", m->getMemberType().c_str(), function->getName().c_str(), sclGenArgs(result, function).c_str());
                }
                scopeDepth--;
                append("}\n");
                append("%s Method_%s_%s(%s) {\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str());
            }
            
            scopeDepth++;

            if (function->isMethod) {
                Method* m = static_cast<Method*>(function);
                append("callstk.data[callstk.ptr++].v = \"%s -> <runtime-checks>\";\n", sclFunctionNameToFriendlyString(m->getMemberType() + ":" + m->getName()).c_str());
                append("if (!scl_do_method_check && scl_find_index_of_struct(Var_self) != -1 && Var_self->$__type__ != 0x%016llx) {\n", hash1((char*) m->getMemberType().c_str()));
                scopeDepth++;
                append("scl_any method = scl_get_method_on_type(Var_self->$__type__, 0x%016llx);\n", hash1((char*) sclFunctionNameToFriendlyString(m->getMemberType() + ":" + m->getName()).c_str()));
                append("if (method != NULL) {\n");
                if (sclTypeToCType(result, m->getReturnType()) != "void") {
                    append("  scl_do_method_check = 1;\n");
                    append("  %s ret = ((%s) method)(%s);\n", sclTypeToCType(result, m->getReturnType()).c_str(), sclGenCastForMethod(result, m).c_str(), sclGenArgs(result, m).c_str());
                    append("  scl_do_method_check = 0;\n");
                    append("  return ret;\n");
                } else {
                    append("  scl_do_method_check = 1;\n");
                    append("  ((%s) method)(%s);\n", sclGenCastForMethod(result, m).c_str(), sclGenArgs(result, m).c_str());
                    append("  scl_do_method_check = 0;\n");
                    append("  return;\n");
                }
                append("}\n");
                scopeDepth--;
                append("}\n");
                append("callstk.ptr--;\n");
                append("scl_do_method_check = 0;\n");
            }

            std::vector<Token> body = function->getBody();
            if (body.size() == 0)
                goto emptyFunction;

            append("callstk.data[callstk.ptr++].v = \"%s\";\n", sclFunctionNameToFriendlyString(functionDeclaration).c_str());
            if (function->hasNamedReturnValue) {
                append("%s Var_%s;\n", sclTypeToCType(result, function->getNamedReturnValue().getType()).c_str(), function->getNamedReturnValue().getName().c_str());
            }

            for (Variable arg : function->getArgs()) {
                if (!typeCanBeNil(arg.getType())) {
                    append("current_file = \"%s\";\n", std::filesystem::path(function->getNameToken().getFile()).filename().c_str());
                    append("current_line = %d;\n", function->getNameToken().getLine());
                    append("current_col = %d;\n", function->getNameToken().getColumn());
                    append("scl_assert(*(scl_int*) &Var_%s, \"Argument '%s' is nil!\");", arg.getName().c_str(), arg.getName().c_str());
                }
            }

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
