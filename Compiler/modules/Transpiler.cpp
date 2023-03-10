#include <filesystem>
#include <stack>
#include <functional>

#include "../headers/Common.hpp"
#include "../headers/TranspilerDefs.hpp"

#ifndef VERSION
#define VERSION ""
#endif

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

    std::string sclFunctionNameToFriendlyString(Function* f);

    bool hasTypealias(TPResult r, std::string t) {
        if (t.size() && t != "?" && t.at(t.size() - 1) == '?') t = t.substr(0, t.size() - 1);
        using spair = std::pair<std::string, std::string>;
        for (spair p : r.typealiases) {
            if (p.first == t) {
                return true;
            }
        }
        return false;
    }

    std::string getTypealias(TPResult r, std::string t) {
        if (t.size() && t != "?" && t.at(t.size() - 1) == '?') t = t.substr(0, t.size() - 1);
        using spair = std::pair<std::string, std::string>;
        for (spair p : r.typealiases) {
            if (p.first == t) {
                return p.second;
            }
        }
        return "";
    }

    std::string notNilTypeOf(std::string t) {
        if (t.size() && t != "?" && t.at(t.size() - 1) == '?') t = t.substr(0, t.size() - 1);
        return t;
    }

    std::string removeTypeModifiers(std::string t) {
        while (strstarts(t, "mut ") || strstarts(t, "const ") || strstarts(t, "readonly ")) {
            if (strstarts(t, "mut ")) {
                t = t.substr(4);
            } else if (strstarts(t, "const ")) {
                t = t.substr(6);
            } else if (strstarts(t, "readonly ")) {
                t = t.substr(9);
            }
        }
        if (t.size() && t != "?" && t.at(t.size() - 1) == '?') {
            t = t.substr(0, t.size() - 1);
        }
        return t;
    }

    std::string sclTypeToCType(TPResult result, std::string t) {
        std::string return_type = "scl_any";

        if (t.size() && t != "?" && t.at(t.size() - 1) == '?') {
            t = t.substr(0, t.size() - 1);
        }
        t = removeTypeModifiers(t);

        if (strstarts(t, "lambda(")) return_type = "_scl_lambda";
        if (t == "any") return_type = "scl_any";
        else if (t == "none") return_type = "void";
        else if (t == "int") return_type = "scl_int";
        else if (t == "float") return_type = "scl_float";
        else if (t == "bool") return_type = "scl_int";
        else if (!(getStructByName(result, t) == Struct(""))) {
            return_type = "scl_" + getStructByName(result, t).getName();
        } else if (t.size() > 2 && t.size() && t.at(0) == '[') {
            std::string type = sclTypeToCType(result, t.substr(1, t.length() - 2));
            return_type = type + "*";
        } else if (hasTypealias(result, t)) {
            return_type = getTypealias(result, t);
        }

        return return_type;
    }

    std::string sclGenArgs(TPResult result, Function *func) {
        std::string args = "";
        for (size_t i = 0; i < func->getArgs().size(); i++) {
            Variable arg = func->getArgs()[i];
            if (i) args += ", ";
            if (removeTypeModifiers(arg.getType()) == "float") {
                if (i) {
                    args += "_scl_internal_stack.data[_scl_internal_stack.ptr + " + std::to_string(i) + "].f";
                } else {
                    args += "_scl_internal_stack.data[_scl_internal_stack.ptr].f";
                }
            } else if (removeTypeModifiers(arg.getType()) == "int" || removeTypeModifiers(arg.getType()) == "bool") {
                if (i) {
                    args += "_scl_internal_stack.data[_scl_internal_stack.ptr + " + std::to_string(i) + "].i";
                } else {
                    args += "_scl_internal_stack.data[_scl_internal_stack.ptr].i";
                }
            } else if (removeTypeModifiers(arg.getType()) == "str") {
                if (i) {
                    args += "_scl_internal_stack.data[_scl_internal_stack.ptr + " + std::to_string(i) + "].s";
                } else {
                    args += "_scl_internal_stack.data[_scl_internal_stack.ptr].s";
                }
            } else {
                if (i) {
                    args += "(" + sclTypeToCType(result, arg.getType()) + ") _scl_internal_stack.data[_scl_internal_stack.ptr + " + std::to_string(i) + "].v";
                } else {
                    args += "(" + sclTypeToCType(result, arg.getType()) + ") _scl_internal_stack.data[_scl_internal_stack.ptr].v";
                }
            }
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
            return ".ptrType" + typeToASCII(v.substr(1, v.length() - 2)) + (canBeNil ? ".nilable" : "");
        }
        if (v == "?") {
            return std::string(".sclWildcard") + (canBeNil ? ".nilable" : "");
        }
        return v;
    }

    std::string generateSymbolForFunction(Function* f) {
        auto indexOf = [](std::vector<std::string> v, std::string s) -> size_t {
            for (size_t i = 0; i < v.size(); i++) {
                if (v.at(i) == s) return i;
            }
            return -1;
        };

        if (contains<std::string>(f->getModifiers(), "asm")) {
            std::string label = f->getModifiers().at(indexOf(f->getModifiers(), "asm") + 1);

            return std::string("\"") + label + "\"";
        }

        if (contains<std::string>(f->getModifiers(), "cdecl")) {
            std::string cLabel = f->getModifiers().at(indexOf(f->getModifiers(), "cdecl") + 1);

            return "_scl_macro_to_string(__USER_LABEL_PREFIX__) " + std::string("\"") + cLabel + "\"";
        }
        std::string symbol = f->getName();
        if (f->isMethod) {
            Method* m = static_cast<Method*>(f);
            symbol = "m." + m->getMemberType() + "." + f->getName();
            if (f->isExternC || contains<std::string>(f->getModifiers(), "expect")) {
                symbol = m->getMemberType() + "$" + f->getName();
            }
        } else {
            symbol = "f." + f->getName();
            if (f->isExternC || contains<std::string>(f->getModifiers(), "expect")) {
                symbol = f->getName();
            }
        }
        if (contains<std::string>(f->getModifiers(), "<lambda>")) {
            symbol = "$scl_lambda_" + std::string(f->getName().c_str() + 7);
        }

        if (Main.options.transpileOnly && symbolTable) {
            if (f->isMethod) {
                fprintf(symbolTable, "Symbol for Method %s:%s: '%s'\n", static_cast<Method*>(f)->getMemberType().c_str(), f->getName().c_str(), symbol.c_str());
            } else {
                fprintf(symbolTable, "Symbol for Function %s: '%s'\n", replaceFirstAfter(f->getName(), "$", "::", 1).c_str(), symbol.c_str());
            }
        }

        return "_scl_macro_to_string(__USER_LABEL_PREFIX__) " + std::string("\"") + symbol + "\"";
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

        append("const char _scl_version[] = \"%s\";\n\n", std::string(VERSION).c_str());
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
                for (size_t i = 0; i < function->getArgs().size(); i++) {
                    if (i) {
                        arguments += ", ";
                    }
                    arguments += sclTypeToCType(result, function->getArgs()[i].getType()) + " Var_" + function->getArgs()[i].getName();
                }
            }

            std::string symbol = generateSymbolForFunction(function);

            if (!function->isMethod) {
                append("%s Function_%s(%s) __asm(%s);\n", return_type.c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
            } else {
                append("%s Method_%s$%s(%s) __asm(%s);\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
            }
            if (function->isExternC) {
                if (!function->isMethod) {
                    fprintf(support_header, "expect %s %s(%s) __asm(%s);\n", return_type.c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                } else {
                    fprintf(support_header, "expect %s Method_%s$%s(%s) __asm(%s);\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                }
                continue;
            } else if (contains<std::string>(function->getModifiers(), "export")) {
                if (!function->isMethod) {
                    fprintf(support_header, "export %s %s(%s) __asm(%s);\n", return_type.c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                } else {
                    fprintf(support_header, "export %s Method_%s$%s(%s) __asm(%s);\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
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
                    for (size_t i = 0; i < function->getArgs().size(); i++) {
                        if (i) {
                            arguments += ", ";
                        }
                        arguments += sclTypeToCType(result, function->getArgs()[i].getType()) + " Var_" + function->getArgs()[i].getName();
                    }
                }
                std::string symbol = generateSymbolForFunction(function);

                if (!function->isMethod) {
                    append("%s Function_%s(%s) __asm(%s);\n", return_type.c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                } else {
                    append("%s Method_%s$%s(%s) __asm(%s);\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                }
                if (function->isExternC) {
                    if (!function->isMethod) {
                        fprintf(support_header, "expect %s %s(%s) __asm(%s);\n", return_type.c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
                    } else {
                        fprintf(support_header, "expect %s Method_%s$%s(%s) __asm(%s);\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str(), symbol.c_str());
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
            if (methodArgs[i].getName() == "self" || functionArgs[i].getName() == "self") continue;
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
            arg += removeTypeModifiers(v.getType());
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
            if (c.getName() == "str") continue;
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
                    if (!Main.options.minify) res.line = t.getLine();
                    res.in = t.getFile();
                    if (!Main.options.minify) res.column = t.getColumn();
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
                        if (!Main.options.minify) res.line = t.getLine();
                        res.in = t.getFile();
                        if (!Main.options.minify) res.column = t.getColumn();
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
                        if (!Main.options.minify) res.line = t.getLine();
                        res.in = t.getFile();
                        if (!Main.options.minify) res.column = t.getColumn();
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
                        if (!Main.options.minify) res.line = t.getLine();
                        res.in = t.getFile();
                        if (!Main.options.minify) res.column = t.getColumn();
                        res.value = t.getValue();
                        res.type = t.getType();
                        errors.push_back(res);
                        continue;
                    }
                }
            }

            append("struct Struct_%s {\n", c.getName().c_str());
            append("  scl_int $__type__;\n");
            append("  scl_int8* $__type_name__;\n");
            append("  scl_int $__super__;\n");
            append("  scl_int $__size__;\n");
            for (Variable s : c.getMembers()) {
                append("  %s %s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
            }
            append("};\n");
            fprintf(support_header, "struct %s {\n", c.getName().c_str());
            fprintf(support_header, "  scl_int __type_identifier__;\n");
            fprintf(support_header, "  scl_int8* __type_string__;\n");
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
    std::vector<bool> was_catch;
    char repeat_depth = 0;
    int iterator_count = 0;
    bool noWarns;
    std::string currentFile = "";
    int currentLine = -1;
    int currentCol = -1;
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
        if (typeStack.size() == 0 || args.size() == 0) {
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
            std::string typeA = removeTypeModifiers(tmp.at(i));
            if (typeA == "bool") typeA = "int";
            
            while (typeA.size() && typeA.at(typeA.size() - 1) == '?') {
                typeA = typeA.substr(0, typeA.size() - 1);
            }

            if (strstarts(typeA, "lambda(")) typeA = "lambda";

            std::string typeB = removeTypeModifiers(args.at(i).getType());
            if (typeB == "bool") typeB = "int";
            
            while (typeB.size() && typeB.at(typeB.size() - 1) == '?') {
                typeB = typeB.substr(0, typeB.size() - 1);
            }

            if (strstarts(typeB, "lambda(")) typeB = "lambda";

            if (typeB == "any" || typeB == "[any]" || (typeCanBeNil(typeB) && (typeA == "any" || typeA == "[any]"))) {
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
            
            if (i) {
                arg += ", ";
            }

            arg += typeA;
        }
        return arg;
    }

    void generateUnsafeCall(Method* self, FILE* fp, TPResult result) {
        if (self->getArgs().size() > 0)
            append("_scl_popn(%zu);\n", self->getArgs().size());
        if (removeTypeModifiers(self->getReturnType()) == "none") {
            append("Method_%s$%s(%s);\n", self->getMemberType().c_str(), self->getName().c_str(), sclGenArgs(result, self).c_str());
        } else {
            if (removeTypeModifiers(self->getReturnType()) == "float") {
                append("_scl_push()->f = Method_%s$%s(%s);\n", self->getMemberType().c_str(), self->getName().c_str(), sclGenArgs(result, self).c_str());
            } else {
                append("_scl_push()->i = (scl_int) Method_%s$%s(%s);\n", self->getMemberType().c_str(), self->getName().c_str(), sclGenArgs(result, self).c_str());
            }
        }
    }

    void generateCall(Method* self, FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<Token>& body) {
        if (self->getArgs().size() > 0) {
            append("_scl_popn(%zu);\n", self->getArgs().size());
        }
        bool argsCorrect = checkStackType(result, self->getArgs());
        if (!argsCorrect) {
            {
                transpilerError("Arguments for function '" + sclFunctionNameToFriendlyString(self) + "' do not equal inferred stack!", i);
                errors.push_back(err);
            }
            transpilerError("Expected: [ " + argVectorToString(self->getArgs()) + " ], but got: [ " + stackSliceToString(self->getArgs().size()) + " ]", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        for (size_t m = 0; m < self->getArgs().size(); m++) {
            if (typeStack.size())
                typeStack.pop();
        }
        if (self->getReturnType().size() > 0 && removeTypeModifiers(self->getReturnType()) != "none") {
            if (removeTypeModifiers(self->getReturnType()) == "float") {
                append("_scl_push()->f = Method_%s$%s(%s);\n", self->getMemberType().c_str(), self->getName().c_str(), sclGenArgs(result, self).c_str());
            } else {
                append("_scl_push()->i = (scl_int) Method_%s$%s(%s);\n", self->getMemberType().c_str(), self->getName().c_str(), sclGenArgs(result, self).c_str());
            }
            typeStack.push(self->getReturnType());
        } else {
            append("Method_%s$%s(%s);\n", self->getMemberType().c_str(), self->getName().c_str(), sclGenArgs(result, self).c_str());
        }
    }

    void generateUnsafeCall(Function* self, FILE* fp, TPResult result) {
        if (self->getArgs().size() > 0)
            append("_scl_popn(%zu);\n", self->getArgs().size());
        if (removeTypeModifiers(self->getReturnType()) == "none") {
            append("Function_%s(%s);\n", self->getName().c_str(), sclGenArgs(result, self).c_str());
        } else {
            if (removeTypeModifiers(self->getReturnType()) == "float") {
                append("_scl_push()->f = Function_%s(%s);\n", self->getName().c_str(), sclGenArgs(result, self).c_str());
            } else {
                append("_scl_push()->i = (scl_int) Function_%s(%s);\n", self->getName().c_str(), sclGenArgs(result, self).c_str());
            }
        }
    }

    void generateCall(Function* self, FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<Token>& body) {
        if (self->getArgs().size() > 0) {
            append("_scl_popn(%zu);\n", self->getArgs().size());
        }
        bool argsCorrect = checkStackType(result, self->getArgs());
        if (!argsCorrect) {
            {
                transpilerError("Arguments for function '" + sclFunctionNameToFriendlyString(self) + "' do not equal inferred stack!", i);
                errors.push_back(err);
            }
            transpilerError("Expected: [ " + argVectorToString(self->getArgs()) + " ], but got: [ " + stackSliceToString(self->getArgs().size()) + " ]", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        for (ssize_t m = self->getArgs().size() - 1; m >= 0; m--) {
            if (typeStack.size())
                typeStack.pop();
        }
        if (self->getReturnType().size() > 0 && removeTypeModifiers(self->getReturnType()) != "none") {
            if (removeTypeModifiers(self->getReturnType()) == "float") {
                append("_scl_push()->f = Function_%s(%s);\n", self->getName().c_str(), sclGenArgs(result, self).c_str());
            } else {
                append("_scl_push()->i = (scl_int) Function_%s(%s);\n", self->getName().c_str(), sclGenArgs(result, self).c_str());
            }
            typeStack.push(self->getReturnType());
        } else {
            append("Function_%s(%s);\n", self->getName().c_str(), sclGenArgs(result, self).c_str());
        }
    }

#define handler(_tok) extern "C" void handle ## _tok (std::vector<Token>& body, Function* function, std::vector<FPResult>& errors, std::vector<FPResult>& warns, FILE* fp, TPResult& result)
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
    handler(Lambda);

    bool handleOverriddenOperator(TPResult result, FILE* fp, int scopeDepth, std::string op, std::string type);

    int lambdaCount = 0;

    handler(Lambda) {
        noUnused;
        Function* f = new Function("$lambda" + std::to_string(lambdaCount++) + "$" + function->getName(), body[i]);
        f->addModifier("<lambda>");
        f->addModifier("no_cleanup");
        f->setReturnType("none");
        ITER_INC;
        if (body[i].getType() == tok_paren_open) {
            ITER_INC;
            while (i < body.size() && body[i].getType() != tok_paren_close) {
                if (body[i].getType() == tok_identifier) {
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
                        i++;
                        FPResult r = parseType(body, &i);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        isConst = typeIsConst(type);
                        isMut = typeIsMut(type);
                        if (type == "none") {
                            transpilerError("Type 'none' is only valid for function return types.", i);
                            errors.push_back(err);
                            continue;
                        }
                    } else {
                        transpilerError("A type is required!", i);
                        errors.push_back(err);
                        ITER_INC;
                        continue;
                    }
                    Variable v = Variable(name, type, isConst, isMut);
                    v.canBeNil = typeCanBeNil(type);
                    f->addArgument(v);
                } else {
                    transpilerError("Expected identifier for argument name, but got '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                    i++;
                    continue;
                }
                ITER_INC;
                if (body[i].getType() == tok_comma || body[i].getType() == tok_paren_close) {
                    if (body[i].getType() == tok_comma) {
                        ITER_INC;
                    }
                    continue;
                }
                transpilerError("Expected ',' or ')', but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                continue;
            }
            ITER_INC;
        }
        if (body[i].getType() == tok_column) {
            ITER_INC;
            FPResult r = parseType(body, &i);
            if (!r.success) {
                errors.push_back(r);
                return;
            }
            std::string type = r.value;
            f->setReturnType(type);
            ITER_INC;
        }
        while (body[i].getType() != tok_end) {
            f->addToken(body[i]);
            ITER_INC;
        }

        std::string arguments = "";
        if (f->getArgs().size() > 0) {
            for (size_t i = 0; i < f->getArgs().size(); i++) {
                std::string type = sclTypeToCType(result, f->getArgs()[i].getType());
                if (i) {
                    arguments += ", ";
                }
                arguments += type + " Var_" + f->getArgs()[i].getName();
            }
        }

        std::string lambdaType = "lambda(" + std::to_string(f->getArgs().size()) + "):" + f->getReturnType();

        append("%s Function_$lambda%d$%s() __asm(%s);\n", sclTypeToCType(result, f->getReturnType()).c_str(), lambdaCount - 1, function->getName().c_str(), generateSymbolForFunction(f).c_str());
        append("_scl_push()->v = Function_$lambda%d$%s;\n", lambdaCount - 1, function->getName().c_str());
        typeStack.push(lambdaType);
        result.functions.push_back(f);
    }

    handler(ReturnOnNil) {
        noUnused;
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
            append("_scl_assert(_scl_top()->i, \"Not nil assertion failed!\");\n");
        } else {
            append("if (_scl_top()->i == 0) {\n");
            scopeDepth++;
            append("_scl_internal_callstack.ptr--;\n");
            if (!contains<std::string>(function->getModifiers(), "no_cleanup")) append("_scl_cleanup_post_func(_scl_internal_callstack.ptr);\n");
            if (return_type == "void")
                append("return;\n");
            else {
                append("return 0;\n");
            }
            scopeDepth--;
            append("}\n");
        }
    }

    handler(Catch) {
        noUnused;
        std::string ex = "Exception";
        ITER_INC;
        if (body[i].getValue() == "typeof") {
            ITER_INC;
            std::vector<Variable> defaultScope;
            vars.pop_back();
            vars.push_back(defaultScope);
            was_rep.push_back(false);
            was_catch.push_back(true);
            scopeDepth--;
            // TODO: Check for children of 'Exception'
            if (getStructByName(result, body[i].getValue()) == Struct("")) {
                transpilerError("Trying to catch unknown Exception of type '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            append("} else if (_scl_struct_is_type(_scl_internal_exceptions.extable[_scl_internal_exceptions.ptr], %uU)) {\n", hash1((char*) body[i].getValue().c_str()));
        } else {
            i--;
            {
                transpilerError("Generic Exception caught here:", i);
                errors.push_back(err);
            }
            transpilerError("Add 'typeof Exception' here to fix this:\\insertText; typeof Exception;" + std::to_string(err.line) + ":" + std::to_string(err.column + body[i].getValue().length()), i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        std::string exName = "exception";
        if (body.size() > i + 1 && body[i + 1].getValue() == "as") {
            ITER_INC;
            ITER_INC;
            exName = body[i].getValue();
        }
        scopeDepth++;
        Variable v = Variable(exName, ex);
        vars[varDepth].push_back(v);
        append("scl_%s Var_%s = (scl_%s) _scl_internal_exceptions.extable[_scl_internal_exceptions.ptr];\n", ex.c_str(), exName.c_str(), ex.c_str());
    }

    handler(Pragma) {
        noUnused;
        ITER_INC;
        if (body[i].getValue() == "print") {
            ITER_INC;
            std::cout << body[i].getFile()
                      << ":"
                      << body[i].getLine()
                      << ":"
                      << body[i].getColumn()
                      << ": "
                      << body[i].getValue()
                      << std::endl;
        }
    }

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
            append("_scl_internal_stack.ptr--;\n");
            if (typeStack.size())
                typeStack.pop();
        } else if (body[i].getValue() == "dup") {
            append("_scl_push()->v = _scl_top()->v;\n");
            typeStack.push(typeStackTop);
        } else if (body[i].getValue() == "swap") {
            append("{\n");
            scopeDepth++;
            append("scl_any b = _scl_pop()->v;\n");
            append("scl_any a = _scl_pop()->v;\n");
            append("_scl_push()->v = b;\n");
            append("_scl_push()->v = a;\n");
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
            append("scl_any c = _scl_pop()->v;\n");
            append("scl_any b = _scl_pop()->v;\n");
            append("scl_any a = _scl_pop()->v;\n");
            append("_scl_push()->v = c;\n");
            append("_scl_push()->v = b;\n");
            append("_scl_push()->v = a;\n");
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
            append("scl_any b = _scl_pop()->v;\n");
            append("scl_any a = _scl_pop()->v;\n");
            append("_scl_push()->v = a;\n");
            append("_scl_push()->v = b;\n");
            append("_scl_push()->v = a;\n");
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
            append("scl_any c = _scl_pop()->v;\n");
            append("scl_any b = _scl_pop()->v;\n");
            append("scl_any a = _scl_pop()->v;\n");
            append("_scl_push()->v = b;\n");
            append("_scl_push()->v = a;\n");
            append("_scl_push()->v = c;\n");
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
            append("_scl_bulk_decr(0, _scl_internal_stack.ptr);\n");
            append("_scl_internal_stack.ptr = 0;\n");
            for (size_t t = 0; t < typeStack.size(); t++) {
                if (typeStack.size())
                    typeStack.pop();
            }
        } else if (body[i].getValue() == "&&") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "&&", typeStackTop)) return;
            append("_scl_popn(2);\n");
            append("_scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i && _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "!") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "!", typeStackTop)) return;
            append("_scl_top()->i = !_scl_top()->i;\n");
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "!!") {
            if (typeCanBeNil(typeStackTop)) {
                std::string type = typeStackTop;
                if (typeStack.size())
                    typeStack.pop();
                typeStack.push(type.substr(0, type.size() - 1));
                append("_scl_assert(_scl_top()->i, \"Not nil assertion failed!\");\n");
            } else {
                transpilerError("Unnecessary assert-not-nil operator '!!' on not-nil type '" + typeStackTop + "'", i);
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
                append("_scl_assert(_scl_top()->i, \"Not nil assertion failed! If you see this, something has gone very wrong!\");\n");
            }
        } else if (body[i].getValue() == "?") {
            handle(ReturnOnNil);
        } else if (body[i].getValue() == "||") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "||", typeStackTop)) return;
            append("_scl_popn(2);\n");
            append("_scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i || _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "<") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "<", typeStackTop)) return;
            append("_scl_popn(2);\n");
            append("_scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i < _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == ">") {
            if (handleOverriddenOperator(result, fp, scopeDepth, ">", typeStackTop)) return;
            append("_scl_popn(2);\n");
            append("_scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i > _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "==") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "==", typeStackTop)) return;
            append("_scl_popn(2);\n");
            append("_scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i == _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "<=") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "<=", typeStackTop)) return;
            append("_scl_popn(2);\n");
            append("_scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i <= _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == ">=") {
            if (handleOverriddenOperator(result, fp, scopeDepth, ">=", typeStackTop)) return;
            append("_scl_popn(2);\n");
            append("_scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i >= _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "!=") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "!=", typeStackTop)) return;
            append("_scl_popn(2);\n");
            append("_scl_push()->i = _scl_internal_stack.data[_scl_internal_stack.ptr].i != _scl_internal_stack.data[_scl_internal_stack.ptr + 1].i;\n");
            if (typeStack.size())
                typeStack.pop();
            if (typeStack.size())
                typeStack.pop();
            typeStack.push("bool");
        } else if (body[i].getValue() == "++") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "++", typeStackTop)) return;
            append("_scl_top()->i++;\n");
            if (typeStack.size() == 0)
                typeStack.push("int");
        } else if (body[i].getValue() == "--") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "--", typeStackTop)) return;
            append("_scl_top()->i--;\n");
            if (typeStack.size() == 0)
                typeStack.push("int");
        } else if (body[i].getValue() == "exit") {
            append("exit(_scl_pop()->i);\n");
            if (typeStack.size())
                typeStack.pop();
        } else if (body[i].getValue() == "puts") {
            append("puts((_scl_top()->s ? _scl_pop()->s->_data : \"(null)\"));\n");
            if (typeStack.size())
                typeStack.pop();
        } else if (body[i].getValue() == "eputs") {
            append("fprintf(stderr, \"%%s\\n\", (_scl_top()->s ? _scl_pop()->s->_data : \"(null)\"));\n");
            if (typeStack.size())
                typeStack.pop();
        } else if (body[i].getValue() == "abort") {
            append("abort();\n");
        } else if (body[i].getValue() == "typeof") {
            ITER_INC;
            if (hasVar(body[i]) && getStructByName(result, getVar(body[i]).getType()) == Struct("")) {
                append("_scl_push()->s = _scl_create_string(\"%s\");\n", getVar(body[i]).getType().c_str());
                typeStack.push("str");
            } else if (getStructByName(result, getVar(body[i]).getType()) != Struct("")) {
                append("_scl_push()->s = _scl_create_string(Var_%s->$__type_name__);\n", body[i].getValue().c_str());
                typeStack.push("str");
            } else {
                transpilerError("Unknown Variable: '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (body[i].getValue() == "nameof") {
            ITER_INC;
            if (hasVar(body[i])) {
                append("_scl_push()->s = _scl_create_string(\"%s\");\n", body[i].getValue().c_str());
                typeStack.push("str");
            } else {
                transpilerError("Unknown Variable: '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (body[i].getValue() == "sizeof") {
            ITER_INC;
            if (body[i].getValue() == "int") {
                append("_scl_push()->i = sizeof(scl_int);\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "float") {
                append("_scl_push()->i = sizeof(scl_float);\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "str") {
                append("_scl_push()->i = sizeof(scl_str);\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "any") {
                append("_scl_push()->i = sizeof(scl_any);\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "none") {
                append("_scl_push()->i = 0;\n");
                typeStack.push("int");
                return;
            }
            if (hasVar(body[i])) {
                append("_scl_push()->i = sizeof(%s);\n", sclTypeToCType(result, getVar(body[i]).getType()).c_str());
                typeStack.push("int");
            } else if (getStructByName(result, body[i].getValue()) != Struct("")) {
                append("_scl_push()->i = sizeof(%s);\n", sclTypeToCType(result, body[i].getValue()).c_str());
                typeStack.push("int");
            } else if (hasTypealias(result, body[i].getValue())) {
                append("_scl_push()->i = sizeof(%s);\n", sclTypeToCType(result, body[i].getValue()).c_str());
                typeStack.push("int");
            } else {
                transpilerError("Unknown Variable: '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (body[i].getValue() == "try") {
            append("_scl_exception_push();\n");
            append("if (setjmp(_scl_internal_exceptions.jmptable[_scl_internal_exceptions.ptr - 1]) != 666) {\n");
            scopeDepth++;
            append("_scl_internal_exceptions.callstk_ptr[_scl_internal_exceptions.ptr - 1] = _scl_internal_callstack.ptr;\n");
            varDepth++;
            std::vector<Variable> defaultScope;
            vars.push_back(defaultScope);
        } else if (body[i].getValue() == "catch") {
            handle(Catch);
        } else if (body[i].getValue() == "lambda") {
            handle(Lambda);
        } else if (body[i].getValue() == "pragma!") {
            handle(Pragma);
    #ifdef __APPLE__
    #pragma endregion
    #endif
        } else if (hasEnum(result, body[i].getValue())) {
            Enum e = getEnumByName(result, body[i].getValue());
            ITER_INC;
            if (body[i].getType() != tok_double_column) {
                transpilerError("Expected '::', but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            ITER_INC;
            if (e.indexOf(body[i].getValue()) == Enum::npos) {
                transpilerError("Unknown member of enum '" + e.getName() + "': '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            append("_scl_push()->i = %zu;\n", e.indexOf(body[i].getValue()));
            typeStack.push("int");
        } else if (hasFunction(result, body[i])) {
            Function* f = getFunctionByName(result, body[i].getValue());
            if (f->isMethod) {
                transpilerError("'" + f->getName() + "' is a method, not a function.", i);
                errors.push_back(err);
                return;
            }
            generateCall(f, fp, result, errors, body);
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
                append("_scl_push()->f = Container_%s.%s;\n", containerName.c_str(), memberName.c_str());
            } else {
                append("_scl_push()->v = (scl_any) Container_%s.%s;\n", containerName.c_str(), memberName.c_str());
            }
            typeStack.push(container.getMemberType(memberName));
        } else if (getStructByName(result, body[i].getValue()) != Struct("")) {
            if (body[i + 1].getType() == tok_double_column) {
                std::string struct_ = body[i].getValue();
                ITER_INC;
                ITER_INC;
                Struct s = getStructByName(result, struct_);
                if (body[i].getValue() == "new") {
                    append("_scl_push()->v = _scl_alloc_struct(sizeof(struct Struct_%s), \"%s\", %uU);\n", struct_.c_str(), struct_.c_str(), hash1((char*) s.extends().c_str()));
                    append("_scl_assert(_scl_top()->i, \"Failed to allocate memory for struct '%s'\");\n", struct_.c_str());
                    typeStack.push(struct_);
                    if (hasMethod(result, Token(tok_identifier, "init", 0, ""), struct_)) {
                        append("{\n");
                        scopeDepth++;
                        Method* f = getMethodByName(result, "init", struct_);
                        append("%s tmp = (%s) _scl_top()->v;\n", sclTypeToCType(result, struct_).c_str(), sclTypeToCType(result, struct_).c_str());
                        generateCall(f, fp, result, errors, body);
                        append("_scl_push()->v = tmp;\n");
                        typeStack.push(struct_);
                        scopeDepth--;
                        append("}\n");
                    }
                } else if (body[i].getValue() == "default") {
                    append("_scl_push()->v = _scl_alloc_struct(sizeof(struct Struct_%s), \"%s\", %uU);\n", struct_.c_str(), struct_.c_str(), hash1((char*) s.extends().c_str()));
                    append("_scl_assert(_scl_top()->i, \"Failed to allocate memory for struct '%s'\");\n", struct_.c_str());
                    typeStack.push(struct_);
                } else {
                    if (hasFunction(result, Token(tok_identifier, struct_ + "$" + body[i].getValue(), 0, ""))) {
                        Function* f = getFunctionByName(result, struct_ + "$" + body[i].getValue());
                        if (f->isMethod) {
                            transpilerError("'" + f->getName() + "' is not static!", i);
                            errors.push_back(err);
                            return;
                        }
                        if (f->isPrivate && function->belongsToType(struct_)) {
                            transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        generateCall(f, fp, result, errors, body);
                    } else if (hasGlobal(result, struct_ + "$" + body[i].getValue())) {
                        std::string loadFrom = struct_ + "$" + body[i].getValue();
                        Variable v = getVar(Token(tok_identifier, loadFrom, 0, ""));
                        if ((body[i].getValue().at(0) == '_' || v.isPrivate) && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != struct_))) {
                            transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + struct_ + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (v.getType() == "float") {
                            append("_scl_push()->f = Var_%s;\n", loadFrom.c_str());
                        } else {
                            append("_scl_push()->v = (scl_any) Var_%s;\n", loadFrom.c_str());
                        }
                        typeStack.push(v.getType());
                    } else {
                        transpilerError("Unknown static member of struct '" + s.getName() + "'", i);
                        errors.push_back(err);
                    }
                }
            }
        } else if (hasVar(body[i])) {
            std::string loadFrom = body[i].getValue();
            Variable v = getVar(body[i]);
            if (removeTypeModifiers(v.getType()) == "float") {
                append("_scl_push()->f = Var_%s;\n", loadFrom.c_str());
            } else {
                append("_scl_push()->v = (scl_any) Var_%s;\n", loadFrom.c_str());
            }
            typeStack.push(v.getType());
        } else if (function->isMethod) {
            Method* m = static_cast<Method*>(function);
            Struct s = getStructByName(result, m->getMemberType());
            if (hasMethod(result, body[i], s.getName())) {
                Method* f = getMethodByName(result, body[i].getValue(), s.getName());
                append("_scl_push()->v = (scl_any) Var_self;\n");
                typeStack.push(f->getMemberType());
                generateCall(f, fp, result, errors, body);
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
                    generateCall(f, fp, result, errors, body);
                }
            } else if (s.hasMember(body[i].getValue())) {
                Variable mem = s.getMember(body[i].getValue());
                if (mem.getType() == "float") {
                    append("_scl_push()->f = Var_self->%s;\n", body[i].getValue().c_str());
                } else {
                    append("_scl_push()->v = (scl_any) Var_self->%s;\n", body[i].getValue().c_str());
                }
                typeStack.push(mem.getType());
            } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                std::string loadFrom = s.getName() + "$" + body[i].getValue();
                Variable v = getVar(Token(tok_identifier, loadFrom, 0, ""));
                if (v.getType() == "float") {
                    append("_scl_push()->f = Var_%s;\n", loadFrom.c_str());
                } else {
                    append("_scl_push()->v = (scl_any) Var_%s;\n", loadFrom.c_str());
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
        was_catch.push_back(false);
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
        append("__it%d.start = _scl_pop()->i;\n", iterator_count);
        if (typeStack.size())
            typeStack.pop();
        while (body[i].getType() != tok_step && body[i].getType() != tok_do) {
            handle(Token);
            ITER_INC;
        }
        append("__it%d.end = _scl_pop()->i;\n", iterator_count);
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
            if (!Main.options.minify) result.line = var.getLine();
            result.in = var.getFile();
            if (!Main.options.minify) result.column = var.getColumn();
            result.value = var.getValue();
            result.type =  var.getType();
            warns.push_back(result);
        }
        if (hasContainer(Main.parser->getResult(), var)) {
            FPResult result;
            result.message = "Variable '" + var.getValue() + "' shadowed by container '" + var.getValue() + "'";
            result.success = false;
            if (!Main.options.minify) result.line = var.getLine();
            result.in = var.getFile();
            if (!Main.options.minify) result.column = var.getColumn();
            result.value = var.getValue();
            result.type =  var.getType();
            warns.push_back(result);
        }
        if (hasVar(var)) {
            FPResult result;
            result.message = "Variable '" + var.getValue() + "' is already declared and shadows it.";
            result.success = false;
            if (!Main.options.minify) result.line = var.getLine();
            result.in = var.getFile();
            if (!Main.options.minify) result.column = var.getColumn();
            result.value = var.getValue();
            result.type =  var.getType();
            warns.push_back(result);
        }

        if (!hasVar(var))
            vars[varDepth].push_back(Variable(var.getValue(), "int"));
        was_rep.push_back(false);
        was_catch.push_back(false);
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
        append("%s %s = (%s) _scl_pop()->v;\n", sclTypeToCType(result, typeStackTop).c_str(), iterator_name.c_str(), sclTypeToCType(result, typeStackTop).c_str());
        std::string type = typeStackTop;
        if (typeStack.size()) {
            typeStack.pop();
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
            "for (%sVar_%s = (%s) Method_%s$begin(%s); Method_%s$hasNext(%s); Var_%s = (%s) Method_%s$next(%s)) {\n",
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
            append("_scl_push()->v = (scl_any) &Function_%s;\n", f->getName().c_str());
            std::string lambdaType = "lambda(" + std::to_string(f->getArgs().size()) + "):" + f->getReturnType();
            typeStack.push(lambdaType);
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
                append("_scl_push()->v = (scl_any) &Function_%s;\n", f->getName().c_str());
                std::string lambdaType = "lambda(" + std::to_string(f->getArgs().size()) + "):" + f->getReturnType();
                typeStack.push(lambdaType);
            } else if (hasGlobal(result, struct_ + "$" + body[i].getValue())) {
                std::string loadFrom = struct_ + "$" + body[i].getValue();
                Variable v = getVar(Token(tok_identifier, loadFrom, 0, ""));
                append("_scl_push()->v = (scl_any) &Var_%s;\n", loadFrom.c_str());
                typeStack.push("[" + notNilTypeOf(v.getType()) + "]");
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
                    append("_scl_push()->v = (scl_any) &Method_%s$%s;\n", ((Method*)(f))->getMemberType().c_str(), f->getName().c_str());
                    std::string lambdaType = "lambda(" + std::to_string(f->getArgs().size()) + "):" + f->getReturnType();
                    typeStack.push(lambdaType);
                } else {
                    ITER_INC;
                    if (body[i].getType() != tok_dot) {
                        append("_scl_push()->v = (scl_any) &Var_%s;\n", loadFrom.c_str());
                        typeStack.push("[" + notNilTypeOf(v.getType()) + "]");
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
                    append("_scl_push()->v = (scl_any) &Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());
                    typeStack.push("[" + notNilTypeOf(mem.getType()) + "]");
                }
            } else {
                append("_scl_push()->v = (scl_any) &Var_%s;\n", loadFrom.c_str());
                typeStack.push("[" + notNilTypeOf(v.getType()) + "]");
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
            append("_scl_push()->v = (scl_any) &(Container_%s.%s);\n", containerName.c_str(), memberName.c_str());
            typeStack.push("[" + notNilTypeOf(container.getMemberType(memberName)) + "]");
        } else {
            transpilerError("Unknown variable: '" + toGet.getValue() + "'", i+1);
            errors.push_back(err);
            return;
        }
    }

    handler(Store) {
        noUnused;
        ITER_INC;
        if (body[i].getType() == tok_paren_open) {
            append("{\n");
            scopeDepth++;
            append("struct Struct_Array* tmp = (struct Struct_Array*) _scl_pop()->v;\n");
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
                            transpilerError("Container member variable '" + body[i].getValue() + "' is not mutable", i);
                            errors.push_back(err);
                            ITER_INC;
                            ITER_INC;
                            return;
                        }
                        append("*((scl_any*) Container_%s.%s) = Method_Array$get(%d, tmp);\n", containerName.c_str(), memberName.c_str(), destructureIndex);
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
                                        transpilerError("Variable '" + body[i].getValue() + "' is not mutable", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("*(scl_any*) Var_self->%s = Method_Array$get(%d, tmp);\n", body[i].getValue().c_str(), destructureIndex);
                                    destructureIndex++;
                                    continue;
                                } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                                    Variable mem = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""));
                                    if (!mem.isWritableFrom(function, VarAccess::Dereference)) {
                                        transpilerError("Variable '" + body[i].getValue() + "' is not mutable", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("*(scl_any*) Var_%s$%s = Method_Array$get(%d, tmp);\n", s.getName().c_str(), body[i].getValue().c_str(), destructureIndex);
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
                                transpilerError("Variable '" + body[i].getValue() + "' is not mutable", i);
                                errors.push_back(err);
                                ITER_INC;
                                ITER_INC;
                                return;
                            }
                            ITER_INC;
                            if (body[i].getType() != tok_dot) {
                                append("*(scl_any*) Var_%s = Method_Array$get(%d, tmp);\n", loadFrom.c_str(), destructureIndex);
                                destructureIndex++;
                                continue;
                            }
                            if (!v.isWritableFrom(function, VarAccess::Dereference)) {
                                transpilerError("Variable '" + body[i - 1].getValue() + "' is not mutable", i - 1);
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
                                transpilerError("Member variable '" + body[i].getValue() + "' is not mutable", i);
                                errors.push_back(err);
                                return;
                            }
                            if ((body[i].getValue().at(0) == '_' || mem.isPrivate) && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                                transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*(scl_any*) Var_%s->%s = Method_Array$get(%d, tmp);\n", loadFrom.c_str(), body[i].getValue().c_str(), destructureIndex);
                        } else {
                            if (!v.isWritableFrom(function, VarAccess::Dereference)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*(scl_any*) Var_%s = Method_Array$get(%d, tmp);\n", loadFrom.c_str(), destructureIndex);
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
                            transpilerError("Container member variable '" + body[i].getValue() + "' is const", i);
                            errors.push_back(err);
                            return;
                        }
                        append("{\n");
                        scopeDepth++;
                        append("scl_any tmp%d = Method_Array$get(%d, tmp);\n", destructureIndex, destructureIndex);
                        if (!typeCanBeNil(container.getMember(memberName).getType())) {
                            append("_scl_assert((scl_int) tmp%d, \"Nil cannot be stored in non-nil variable '%s.%s'\");\n", destructureIndex, containerName.c_str(), memberName.c_str());
                        }
                        append("(*(scl_any*) &Container_%s.%s) = tmp%d;\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                        scopeDepth--;
                        append("}\n");
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
                                        transpilerError("Member variable '" + body[i].getValue() + "' is const", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("{\n");
                                    scopeDepth++;
                                    append("scl_any tmp%d = Method_Array$get(%d, tmp);\n", destructureIndex, destructureIndex);
                                    if (!typeCanBeNil(mem.getType())) {
                                        append("_scl_assert((scl_int) tmp%d, \"Nil cannot be stored in non-nil variable 'self.%s'\");\n", destructureIndex, mem.getName().c_str());
                                    }
                                    append("*(scl_any*) &Var_self->%s = tmp%d;\n", body[i].getValue().c_str(), destructureIndex);
                                    scopeDepth--;
                                    append("}\n");
                                    destructureIndex++;
                                    continue;
                                } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                                    if (!getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, "")).isWritableFrom(function, VarAccess::Write)) {
                                        transpilerError("Member variable '" + body[i].getValue() + "' is const", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("{\n");
                                    scopeDepth++;
                                    append("scl_any tmp%d = Method_Array$get(%d, tmp);\n", destructureIndex, destructureIndex);
                                    if (!typeCanBeNil(getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, "")).getType())) {
                                        append("_scl_assert((scl_int) tmp%d, \"Nil cannot be stored in non-nil variable '%s::%s'\");\n", destructureIndex, s.getName().c_str(), body[i].getValue().c_str());
                                    }
                                    append("*(scl_any*) &Var_%s$%s = tmp%d;\n", s.getName().c_str(), body[i].getValue().c_str(), destructureIndex);
                                    scopeDepth--;
                                    append("}\n");
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
                            if (!v.isWritableFrom(function, VarAccess::Write)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is const", i);
                                errors.push_back(err);
                                ITER_INC;
                                ITER_INC;
                                return;
                            }
                            ITER_INC;
                            if (body[i].getType() != tok_dot) {
                                append("{\n");
                                scopeDepth++;
                                append("scl_any tmp%d = Method_Array$get(%d, tmp);\n", destructureIndex, destructureIndex);
                                if (!typeCanBeNil(v.getType())) {
                                    append("_scl_assert((scl_int) tmp%d, \"Nil cannot be stored in non-nil variable '%s'\");\n", destructureIndex, v.getName().c_str());
                                }
                                append("(*(scl_any*) &Var_%s) = tmp%d;\n", loadFrom.c_str(), destructureIndex);
                                scopeDepth--;
                                append("}\n");
                                destructureIndex++;
                                continue;
                            }
                            if (!v.isWritableFrom(function, VarAccess::Dereference)) {
                                transpilerError("Variable '" + body[i - 1].getValue() + "' is not mutable", i - 1);
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
                                transpilerError("Member variable '" + body[i].getValue() + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            if ((body[i].getValue().at(0) == '_' || mem.isPrivate) && (!function->isMethod || (function->isMethod && static_cast<Method*>(function)->getMemberType() != s.getName()))) {
                                transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            append("{\n");
                            scopeDepth++;
                            append("scl_any tmp%d = Method_Array$get(%d, tmp);\n", destructureIndex, destructureIndex);
                            if (!typeCanBeNil(mem.getType())) {
                                append("_scl_assert((scl_int) tmp%d, \"Nil cannot be stored in non-nil variable '%s.%s'\");\n", destructureIndex, s.getName().c_str(), mem.getName().c_str());
                            }
                            append("(*(scl_any*) &Var_%s->%s) = tmp%d;\n", loadFrom.c_str(), body[i].getValue().c_str(), destructureIndex);
                            scopeDepth--;
                            append("}\n");
                        } else {
                            if (!v.isWritableFrom(function, VarAccess::Write)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            append("{\n");
                            scopeDepth++;
                            append("scl_any tmp%d = Method_Array$get(%d, tmp);\n", destructureIndex, destructureIndex);
                            if (!typeCanBeNil(v.getType())) {
                                append("_scl_assert((scl_int) tmp%d, \"Nil cannot be stored in non-nil variable '%s'\");\n", destructureIndex, v.getName().c_str());
                            }
                            append("(*(scl_any*) &Var_%s) = tmp%d;\n", loadFrom.c_str(), destructureIndex);
                            scopeDepth--;
                            append("}\n");
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
                {
                    transpilerError("Variable '" + body[i].getValue() + "' shadowed by function '" + body[i].getValue() + "'", i);
                    if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                    else errors.push_back(err);
                }
            }
            if (hasContainer(result, body[i])) {
                {
                    transpilerError("Variable '" + body[i].getValue() + "' shadowed by container '" + body[i].getValue() + "'", i);
                    if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                    else errors.push_back(err);
                }
            }
            if (getStructByName(result, body[i].getValue()) != Struct("")) {
                {
                    transpilerError("Variable '" + body[i].getValue() + "' shadowed by struct '" + body[i].getValue() + "'", i);
                    if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                    else errors.push_back(err);
                }
            }
            if (hasVar(body[i])) {
                {
                    transpilerError("Variable '" + body[i].getValue() + "' is already declared and shadows it.", i);
                    if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                    else errors.push_back(err);
                }
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
                FPResult r = parseType(body, &i);
                if (!r.success) {
                    errors.push_back(r);
                    return;
                }
                type = r.value;
                isConst = typeIsConst(type);
                isMut = typeIsMut(type);
                if (type == "lambda" && strstarts(typeStackTop, "lambda(")) {
                    type = typeStackTop;
                }
            } else {
                type = typeStackTop;
                i--;
            }
            Variable v = Variable(name, type, isConst, isMut);
            if (typeCanBeNil(type)) {
                v.canBeNil = true;
            }
            vars[varDepth].push_back(v);
            if (!typeCanBeNil(v.getType())) {
                append("_scl_assert(_scl_top()->i, \"Nil cannot be stored in non-nil variable '%s'\");\n", v.getName().c_str());
            }
            append("%s Var_%s = (%s) _scl_pop()->%s;\n", sclTypeToCType(result, v.getType()).c_str(), v.getName().c_str(), sclTypeToCType(result, v.getType()).c_str(), v.getType() == "float" ? "f" : "v");
            if (typeStack.size())
                typeStack.pop();
        } else {
            if (body[i].getType() != tok_identifier && body[i].getType() != tok_addr_of) {
                transpilerError("'" + body[i].getValue() + "' is not an identifier!", i);
                errors.push_back(err);
            }
            Variable v("", "");
            bool topLevelDeref = false;
            std::string containerBegin = "";

            auto generatePathStructRoot = [result, function, fp](std::vector<Token>& body, size_t* i, std::vector<FPResult> &errors, std::string* lastType, bool parseFromExpr, Variable& v, std::string& containerBegin, bool topLevelDeref) -> std::string {
                #define REF_INC do { (*i)++; if ((*i) >= body.size()) { FPResult err; err.success = false; err.message = "Unexpected end of function! " + std::string("Error happened in function ") + std::string(__func__) + " in line " + std::to_string(__LINE__); err.line = body[(*i) - 1].getLine(); err.in = body[(*i) - 1].getFile(); err.column = body[(*i) - 1].getColumn(); err.value = body[(*i) - 1].getValue(); err.type = body[(*i) - 1].getType(); errors.push_back(err); return ""; } } while (0)
                std::string path = "(Var_" + body[(*i)].getValue() + ")";
                std::string sclPath = body[(*i)].getValue();
                Variable v2 = v;
                Struct currentRoot = getStructByName(result, v.getType());
                std::string nextType = removeTypeModifiers(v2.getType());
                if (parseFromExpr) {
                    std::string s = typeStackTop;
                    if (s.at(0) == '[' && s.at(s.size() - 1) == ']') {
                        s = s.substr(1, s.size() - 2);
                    } else {
                        s = "any";
                    }
                    path = "(*(" + sclTypeToCType(result, typeStackTop) + ") tmp)";
                    currentRoot = getStructByName(result, s);
                    nextType = removeTypeModifiers(s);
                    sclPath = "<expr>";
                } else {
                    if (containerBegin.size()) {
                        path = containerBegin;
                    }
                    if (topLevelDeref) {
                        path = "(*" + path + ")";
                        sclPath = "@" + sclPath;
                    }
                    if (topLevelDeref) {
                        nextType = notNilTypeOf(nextType);
                        if (nextType.at(0) == '[' && nextType.at(nextType.size() - 1) == ']') {
                            nextType = removeTypeModifiers(nextType.substr(1, nextType.size() - 2));
                        } else {
                            nextType = "any";
                        }
                    }
                    if (!v2.isWritableFrom(function, VarAccess::Write)) {
                        transpilerError("Variable '" + body[*i].getValue() + "' is const", *i);
                        errors.push_back(err);
                        (*lastType) = nextType;
                        return path;
                    }
                    (*i)++;
                    if ((*i) >= body.size()) {
                        (*lastType) = nextType;
                        return path;
                    }
                }
                size_t last = -1;
                while (body[*i].getType() == tok_dot) {
                    REF_INC;
                    bool deref = false;
                    sclPath += ".";
                    if (body[*i].getType() == tok_addr_of) {
                        deref = true;
                        nextType = notNilTypeOf(nextType);
                        sclPath += "@";
                        REF_INC;
                    }
                    if (!currentRoot.hasMember(body[*i].getValue())) {
                        transpilerError("Struct '" + currentRoot.getName() + "' has no member named '" + body[*i].getValue() + "'", *i);
                        errors.push_back(err);
                        (*lastType) = nextType;
                        return path;
                    } else {
                        if (v2.getName().size() && !v2.isWritableFrom(function, VarAccess::Dereference)) {
                            transpilerError("Variable '" + body[last].getValue() + "' is not mut", last);
                            errors.push_back(err);
                            (*lastType) = nextType;
                            return path;
                        }
                        last = *i;
                        v2 = currentRoot.getMember(body[*i].getValue());
                        nextType = v2.getType();
                        if (deref) {
                            nextType = removeTypeModifiers(nextType);
                            nextType = notNilTypeOf(nextType);
                            if (nextType.at(0) == '[' && nextType.at(nextType.size() - 1) == ']') {
                                nextType = nextType.substr(1, nextType.size() - 2);
                            } else {
                                nextType = "any";
                            }
                        }
                        if (!v2.isWritableFrom(function, VarAccess::Write)) {
                            transpilerError("Variable '" + body[*i].getValue() + "' is const", *i);
                            errors.push_back(err);
                            (*lastType) = nextType;
                            return path;
                        }
                        currentRoot = getStructByName(result, nextType);
                    }
                    sclPath += body[*i].getValue();
                    append("_scl_assert(*(scl_int*) &(%s), \"Tried dereferencing nil pointer '%s'!\");\n", path.c_str(), sclPath.c_str());
                    if (deref) {
                        append("_scl_assert(*(scl_int*) &(%s->%s), \"Tried dereferencing nil pointer '%s'!\");\n", path.c_str(), body[*i].getValue().c_str(), sclPath.c_str());
                        path = "(*(" + path + "->" + body[*i].getValue() + "))";
                    } else {
                        path = "(" + path + "->" + body[*i].getValue() + ")";
                    }
                    (*i)++;
                    if ((*i) >= body.size()) {
                        (*i)--;
                        (*lastType) = nextType;
                        return path;
                    }
                }
                if (!typeCanBeNil(v2.getType())) {
                    append("_scl_assert(_scl_top()->i, \"Nil cannot be stored in non-nil variable '%s'\");\n", v2.getName().c_str());
                }
                (*i)--;
                (*lastType) = nextType;
                return path;
            };

            std::string lastType;
            std::string path;

            if (body[i].getType() == tok_addr_of) {
                topLevelDeref = true;
                ITER_INC;
                if (body[i].getType() == tok_paren_open) {
                    ITER_INC;
                    append("{\n");
                    scopeDepth++;
                    append("scl_any _scl_value_to_store = _scl_pop()->v;\n");
                    if (typeStack.size()) typeStack.pop();
                    append("{\n");
                    scopeDepth++;
                    while (body[i].getType() != tok_paren_close) {
                        handle(Token);
                        ITER_INC;
                    }
                    scopeDepth--;
                    append("}\n");
                    append("scl_any* tmp = _scl_pop()->v;\n");
                    if (i + 1 < body.size() && body[i + 1].getType() == tok_dot) {
                        ITER_INC;
                        path = generatePathStructRoot(body, &i, errors, &lastType, true, v, containerBegin, topLevelDeref);
                    } else {
                        path = "(*tmp)";
                    }
                    if (typeStack.size()) typeStack.pop();
                    append("_scl_push()->v = _scl_value_to_store;\n");
                    if (removeTypeModifiers(lastType) == "float")
                        append("%s = _scl_pop()->f;\n", path.c_str());
                    else
                        append("%s = (%s) _scl_pop()->v;\n", path.c_str(), sclTypeToCType(result, lastType).c_str());
                    scopeDepth--;
                    append("}\n");
                    return;
                }
            }
            if (!hasVar(body[i])) {
                if (function->isMethod) {
                    Method* m = static_cast<Method*>(function);
                    Struct s = getStructByName(result, m->getMemberType());
                    if (s.hasMember(body[i].getValue())) {
                        v = s.getMember(body[i].getValue());
                    } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                        v = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""));
                    }
                } else if (body[i + 1].getType() == tok_double_column) {
                    ITER_INC;
                    Struct s = getStructByName(result, body[i - 1].getValue());
                    ITER_INC;
                    if (s != Struct("")) {
                        if (!hasVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""))) {
                            transpilerError("Struct '" + s.getName() + "' has no static member named '" + body[i].getValue() + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        v = getVar(Token(tok_identifier, s.getName() + "$" + body[i].getValue(), 0, ""));
                    }
                } else if (hasContainer(result, body[i])) {
                    Container c = getContainerByName(result, body[i].getValue());
                    ITER_INC;
                    ITER_INC;
                    std::string memberName = body[i].getValue();
                    if (!c.hasMember(memberName)) {
                        transpilerError("Unknown container member: '" + memberName + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    containerBegin = "(Container_" + c.getName() + "." + body[i].getValue() + ")";
                    v = c.getMember(memberName);
                } else {
                    transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                    return;
                }
            } else {
                v = getVar(body[i]);
            }

            path = generatePathStructRoot(body, &i, errors, &lastType, false, v, containerBegin, topLevelDeref);

            if (removeTypeModifiers(lastType) == "float")
                append("%s = _scl_pop()->f;\n", path.c_str());
            else
                append("%s = (%s) _scl_pop()->v;\n", path.c_str(), sclTypeToCType(result, lastType).c_str());
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
        if (getStructByName(result, body[i].getValue()) != Struct("")) {
            transpilerError("Variable '" + body[i].getValue() + "' shadowed by struct '" + body[i].getValue() + "'", i+1);
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
            FPResult r = parseType(body, &i);
            if (!r.success) {
                errors.push_back(r);
                return;
            }
            type = r.value;
            isConst = typeIsConst(type);
            isMut = typeIsMut(type);
        } else {
            transpilerError("A type is required!", i);
            errors.push_back(err);
            return;
        }
        Variable v = Variable(name, type, isConst, isMut);
        v.canBeNil = typeCanBeNil(type);
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
        append("{\n");
        scopeDepth++;
        Method* f = getMethodByName(result, "init", struct_);
        Struct arr = getStructByName(result, "Array");
        append("struct Struct_Array* tmp = _scl_alloc_struct(sizeof(struct Struct_Array), \"Array\", %uU);\n", hash1((char*) std::string("SclObject").c_str()));
        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
            if (f->getReturnType() == "float") {
                append("_scl_push()->f = Method_Array$init(1, tmp);\n");
            } else {
                append("_scl_push()->v = (scl_any) Method_Array$init(1, tmp);\n");
            }
            typeStack.push(f->getReturnType());
        } else {
            append("Method_Array$init(1, tmp);\n");
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
                append("Method_Array$push(_scl_pop()->v, tmp);\n");
                if (typeStack.size())
                    typeStack.pop();
            }
        }
        append("_scl_push()->v = tmp;\n");
        typeStack.push("Array");
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
        append("{\n");
        scopeDepth++;
        Method* f = getMethodByName(result, "init", struct_);
        Struct map = getStructByName(result, "Map");
        append("struct Struct_Map* tmp = _scl_alloc_struct(sizeof(struct Struct_Map), \"Map\", %uU);\n", hash1((char*) std::string("SclObject").c_str()));
        if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
            if (f->getReturnType() == "float") {
                append("_scl_push()->f = Method_Map$init(1, tmp);\n");
            } else {
                append("_scl_push()->v = (scl_any) Method_Map$init(1, tmp);\n");
            }
            typeStack.push(f->getReturnType());
        } else {
            append("Method_Map$init(1, tmp);\n");
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
                append("Method_Map$set(_scl_create_string(\"%s\"), _scl_pop()->v, tmp);\n", key.c_str());
                if (typeStack.size())
                    typeStack.pop();
            }
        }
        append("_scl_push()->v = tmp;\n");
        typeStack.push("Map");
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
            append("struct Struct_MapEntry* tmp = _scl_alloc_struct(sizeof(struct Struct_MapEntry), \"MapEntry\", %uU);\n", hash1((char*) std::string("SclObject").c_str()));
            if (typeStack.size())
                typeStack.pop();
            if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                if (f->getReturnType() == "float") {
                    append("_scl_push()->f = Method_MapEntry$init(\"%s\", _scl_pop()->v, tmp);\n", key.c_str());
                } else {
                    append("_scl_push()->v = (scl_any) Method_MapEntry$init(\"%s\", _scl_pop()->v, tmp);\n", key.c_str());
                }
                typeStack.push(f->getReturnType());
            } else {
                append("Method_MapEntry$init(_scl_create_string(\"%s\"), _scl_pop()->v, tmp);\n", key.c_str());
            }
            append("_scl_push()->v = tmp;\n");
            typeStack.push("MapEntry");
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
                append("struct Struct_Pair* tmp = _scl_alloc_struct(sizeof(struct Struct_Pair), \"Pair\", %uU);\n", hash1((char*) pair.getName().c_str()));
                append("_scl_popn(2);\n");
                if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                    if (f->getReturnType() == "float") {
                        append("_scl_push()->f = Method_Pair$init(_scl_internal_stack.data[_scl_internal_stack.ptr].v, _scl_internal_stack.data[_scl_internal_stack.ptr + 1].v, tmp);\n");
                    } else {
                        append("_scl_push()->v = (scl_any) Method_Pair$init(_scl_internal_stack.data[_scl_internal_stack.ptr].v, _scl_internal_stack.data[_scl_internal_stack.ptr + 1].v, tmp);\n");
                    }
                    typeStack.push(f->getReturnType());
                } else {
                    append("Method_Pair$init(_scl_internal_stack.data[_scl_internal_stack.ptr].v, _scl_internal_stack.data[_scl_internal_stack.ptr + 1].v, tmp);\n");
                }
                append("_scl_push()->v = tmp;\n");
                typeStack.push("Pair");
                scopeDepth--;
                append("}\n");
            } else if (commas == 2) {
                if (getStructByName(result, "Triple") == Struct("")) {
                    transpilerError("Struct definition for 'Triple' not found!", i);
                    errors.push_back(err);
                    return;
                }
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
                append("struct Struct_Triple* tmp = _scl_alloc_struct(sizeof(struct Struct_Triple), \"Triple\", %uU);\n", hash1((char*) std::string("SclObject").c_str()));
                append("_scl_popn(3);\n");
                if (f->getReturnType().size() > 0 && f->getReturnType() != "none") {
                    if (f->getReturnType() == "float") {
                        append("_scl_push()->f = Method_Triple$init(_scl_internal_stack.data[_scl_internal_stack.ptr + 1].v, _scl_internal_stack.data[_scl_internal_stack.ptr + 1].v, _scl_internal_stack.data[_scl_internal_stack.ptr + 2].v, tmp);\n");
                    } else {
                        append("_scl_push()->v = (scl_any) Method_Triple$init(_scl_internal_stack.data[_scl_internal_stack.ptr + 1].v, _scl_internal_stack.data[_scl_internal_stack.ptr + 1].v, _scl_internal_stack.data[_scl_internal_stack.ptr + 2].v, tmp);\n");
                    }
                    typeStack.push(f->getReturnType());
                } else {
                    append("Method_Triple$init(_scl_internal_stack.data[_scl_internal_stack.ptr + 1].v, _scl_internal_stack.data[_scl_internal_stack.ptr + 1].v, _scl_internal_stack.data[_scl_internal_stack.ptr + 2].v, tmp);\n");
                }
                append("_scl_push()->v = tmp;\n");
                typeStack.push("Triple");
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
        append("_scl_push()->s = _scl_create_string(\"%s\");\n", body[i].getValue().c_str());
        typeStack.push("str");
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
        append("_scl_internal_callstack.data[_scl_internal_callstack.ptr++].func = \"<%s:native code>\";\n", function->getName().c_str());
        if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].line = %d;\n", body[i].getLine());
        if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].col = %d;\n", body[i].getColumn());
        std::string file = body[i].getFile();
        if (strstarts(file, scaleFolder)) {
            file = file.substr(scaleFolder.size() + std::string("/Frameworks/").size());
        } else {
            file = std::filesystem::path(file).relative_path();
        }
        if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].file = \"%s\";\n", file.c_str());
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
        append("_scl_internal_callstack.ptr--;\n");
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
        append("_scl_push()->v = (scl_any) 0;\n");
        if (body[i].getType() == tok_nil) {
            typeStack.push("any");
        } else {
            typeStack.push("bool");
        }
    }

    handler(TruthyType) {
        noUnused;
        append("_scl_push()->v = (scl_any) 1;\n");
        typeStack.push("bool");
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
        if (struct_ == "int" || struct_ == "float" || struct_ == "any") {
            append("_scl_top()->i = 1;\n");
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
        append("_scl_top()->i = _scl_top()->v && _scl_struct_is_type(_scl_top()->v, 0x%xU);\n", hash1((char*) struct_.c_str()));
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
        append("if (_scl_pop()->i) {\n");
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
        append("if (_passedCondition%zu) _scl_internal_stack.ptr--;\n", condCount - 1);
        append("else if (_scl_pop()->i) {\n");
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
        was_catch.push_back(false);
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
    }

    handler(Do) {
        noUnused;
        if (typeStack.size())
            typeStack.pop();
        append("if (!_scl_pop()->v) break;\n");
    }

    handler(DoneLike) {
        noUnused;
        scopeDepth--;
        varDepth--;
        vars.pop_back();
        if (was_catch.size() > 0 && was_catch.back()) {
            append("} else {\n");
            append("  Function_throw(_scl_internal_exceptions.extable[_scl_internal_exceptions.ptr]);\n");
        }
        append("}\n");
        if (repeat_depth > 0 && was_rep.back()) {
            repeat_depth--;
        }
        if (was_catch.size() > 0) was_catch.pop_back();
        if (was_rep.size() > 0) was_rep.pop_back();
    }

    handler(Return) {
        noUnused;
        append("{\n");
        scopeDepth++;
        if (return_type != "void" && !function->hasNamedReturnValue)
            append("_scl_frame_t returnFrame = _scl_internal_stack.data[--_scl_internal_stack.ptr];\n");
        append("_scl_internal_callstack.ptr--;\n");
        if (!contains<std::string>(function->getModifiers(), "no_cleanup")) append("_scl_cleanup_post_func(_scl_internal_callstack.ptr);\n");

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
                    append("_scl_assert(*(scl_int*) &Var_%s, \"Tried returning nil from function returning not-nil type '%s'!\");\n", function->getNamedReturnValue().getName().c_str(), function->getReturnType().c_str());
                else
                    append("_scl_assert(returnFrame.i, \"Tried returning nil from function returning not-nil type '%s'!\");\n", function->getReturnType().c_str());
            }
        }

        if (return_type == "void")
            append("return;\n");
        else {
            if (function->hasNamedReturnValue) {
                std::string type = sclTypeToCType(result, function->getNamedReturnValue().getType());
                append("return (%s) Var_%s;\n", type.c_str(), function->getNamedReturnValue().getName().c_str());
            } else {
                if (return_type == "scl_str") {
                    append("return returnFrame.s;\n");
                } else if (return_type == "scl_int") {
                    append("return returnFrame.i;\n");
                } else if (return_type == "scl_float") {
                    append("return returnFrame.f;\n");
                } else if (return_type == "scl_any") {
                    append("return returnFrame.v;\n");
                } else if (strncmp(return_type.c_str(), "scl_", 4) == 0 || hasTypealias(result, function->getReturnType())) {
                    std::string type = sclTypeToCType(result, function->getReturnType());
                    append("return (%s) returnFrame.v;\n", type.c_str());
                }
            }
        }
        scopeDepth--;
        append("}\n");
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
        append("switch (_scl_pop()->i) {\n");
        scopeDepth++;
        varDepth++;
        std::vector<Variable> defaultScope;
        vars.push_back(defaultScope);
        was_rep.push_back(false);
        was_catch.push_back(false);
    }

    handler(AddrOf) {
        noUnused;
        if (handleOverriddenOperator(result, fp, scopeDepth, "@", typeStackTop)) return;
        append("_scl_top()->v = (*(scl_any*) _scl_top()->v);\n");
        std::string ptr = removeTypeModifiers(typeStackTop);
        if (typeStack.size())
            typeStack.pop();
        if (ptr.size()) {
            if (ptr.at(0) == '[' && ptr.at(ptr.size() - 1) == ']') {
                typeStack.push(ptr.substr(1, ptr.size() - 2));
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

    bool isPrimitiveIntegerType(std::string s) {
        return s == "int" ||
               s == "int32" ||
               s == "int16" ||
               s == "int8" ||
               s == "uint32" ||
               s == "uint16" ||
               s == "uint8";
    }

    handler(As) {
        noUnused;
        ITER_INC;
        FPResult type = parseType(body, &i);
        if (!type.success) {
            errors.push_back(type);
            return;
        }
        if (type.value == "int" || type.value == "float" || type.value == "any") {
            if (typeStack.size())
                typeStack.pop();
            typeStack.push(type.value);
            return;
        }
        if (isPrimitiveIntegerType(type.value)) {
            if (typeStack.size())
                typeStack.pop();
            typeStack.push(type.value);
            append("_scl_top()->i &= ((1UL << (sizeof(scl_%s) * 8)) - 1);\n", type.value.c_str());
            append("_scl_top()->i = (scl_%s) _scl_top()->i;\n", type.value.c_str());
            return;
        }
        auto typeIsPtr = [type]() -> bool {
            std::string t = type.value;
            if (t.size() == 0) return false;
            t = removeTypeModifiers(t);
            t = notNilTypeOf(t);
            return (t.at(0) == '[' && t.at(t.size() - 1) == ']');
        };
        if (hasTypealias(result, type.value) || typeIsPtr()) {
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
                append("_scl_assert(_scl_top()->i, \"Nil cannot be cast to non-nil type '%s!'\");\n", type.value.c_str());
            }
            if (typeStack.size())
                typeStack.pop();
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
            if (s.getName() == "Map" || s.extends("Map")) {
                Method* f = getMethodByName(result, "get", s.getName());
                if (!f) {
                    transpilerError("Could not find method 'get' on struct '" + s.getName() + "'", i - 1);
                    errors.push_back(err);
                    return;
                }
                
                append("{\n");
                scopeDepth++;
                append("scl_any tmp = _scl_pop()->v;\n");
                std::string t = typeStackTop;
                typeStack.pop();
                append("_scl_push()->s = _scl_create_string(\"%s\");\n", body[i].getValue().c_str());
                typeStack.push(t);
                typeStack.push("str");
                append("_scl_push()->v = tmp;\n");
                scopeDepth--;
                append("}\n");
                generateCall(f, fp, result, errors, body);
                return;
            }
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
            transpilerError("If you know '" + typeStackTop + "' can't be nil at this location, add '!!' before this '.'\\insertText;!!;" + std::to_string(err.line) + ":" + std::to_string(err.column), i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        if (mem.getType() == "float")
            append("_scl_top()->f = ((%s) _scl_top()->v)->%s;\n", sclTypeToCType(result, s.getName()).c_str(), body[i].getValue().c_str());
        else
            append("_scl_top()->v = (scl_any) ((%s) _scl_top()->v)->%s;\n", sclTypeToCType(result, s.getName()).c_str(), body[i].getValue().c_str());
        if (typeStack.size())
            typeStack.pop();
        typeStack.push(mem.getType());
    }

    handler(Column) {
        noUnused;
        if (strstarts(typeStackTop, "lambda(") || typeStackTop == "lambda") {
            ITER_INC;
            if (typeStackTop == "lambda") {
                std::string op = body[i].getValue();
                if (op == "accept") {
                    append("((void(*)(void)) _scl_pop()->v)();\n");
                } else if (op == "acceptWithIntResult") {
                    append("_scl_internal_stack.ptr--;\n");
                    append("_scl_push()->i = ((scl_int(*)(void)) _scl_internal_stack.data[_scl_internal_stack.ptr].v)();\n");
                } else if (op == "acceptWithFloatResult") {
                    append("_scl_internal_stack.ptr--;\n");
                    append("_scl_push()->f = ((scl_float(*)(void)) _scl_internal_stack.data[_scl_internal_stack.ptr].v)();\n");
                } else if (op == "acceptWithStringResult") {
                    append("_scl_internal_stack.ptr--;\n");
                    append("_scl_push()->s = ((scl_str(*)(void)) _scl_internal_stack.data[_scl_internal_stack.ptr].v)();\n");
                } else if (op == "acceptWithResult") {
                    append("_scl_internal_stack.ptr--;\n");
                    append("_scl_push()->v = ((scl_any(*)(void)) _scl_internal_stack.data[_scl_internal_stack.ptr].v)();\n");
                } else {
                    transpilerError("Unknown method '" + body[i].getValue() + "' on type 'untyped-lambda'", i);
                    errors.push_back(err);
                }
                typeStack.pop();
                return;
            }
            std::string returnType = typeStackTop.substr(typeStackTop.find_last_of("):") + 1);
            std::string op = body[i].getValue();

            std::string args = typeStackTop.substr(typeStackTop.find_first_of("(") + 1, typeStackTop.find_last_of("):") - typeStackTop.find_first_of("(") - 1);
            size_t argAmount = std::stoi(args);
            
            for (size_t argc = argAmount; argc; argc--) {
                if (typeStack.size())
                    typeStack.pop();
            }

            if (typeStack.size())
                typeStack.pop();
            if (op == "accept") {
                if (returnType == "none") {
                    append("((void(*)(void)) _scl_pop()->v)();\n");
                } else if (returnType == "float") {
                    append("_scl_internal_stack.ptr--;\n");
                    append("_scl_push()->f = ((scl_float(*)(void)) _scl_internal_stack.data[_scl_internal_stack.ptr].v)();\n");
                    typeStack.push(returnType);
                } else {
                    append("_scl_internal_stack.ptr--;\n");
                    append("_scl_push()->v = ((scl_any(*)(void)) _scl_internal_stack.data[_scl_internal_stack.ptr].v)();\n");
                    typeStack.push(returnType);
                }
            } else {
                transpilerError("Unknown method '" + body[i].getValue() + "' on type 'lambda'", i);
                errors.push_back(err);
            }
            return;
        }
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
        generateCall(f, fp, result, errors, body);
    }

    handler(Token) {
        noUnused;

        std::string file = body[i].getFile();

        if (strstarts(file, scaleFolder)) {
            file = file.substr(scaleFolder.size() + std::string("/Frameworks/").size());
        } else {
            file = std::filesystem::path(file).relative_path();
        }

        if (body[i].getType() != tok_case) {
            if (currentFile != file) {
                if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].file = \"%s\";\n", file.c_str());
                currentFile = file;
            }
            if (currentLine != body[i].getLine()) {
                if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].line = %d;\n", body[i].getLine());
                currentLine = body[i].getLine();
            }
            if (currentCol != body[i].getColumn()) {
                if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].col = %d;\n", body[i].getColumn());
                currentCol = body[i].getColumn();
            }
        }

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

        if (strstarts(name, "::lambda")) {
            name = "<lambda" + name.substr(8, name.find_first_of("(") - 8) + ">";
        }

        return name;
    }

    std::string sclFunctionNameToFriendlyString(Function* f) {
        std::string name = f->getName();
        name = sclFunctionNameToFriendlyString(name);
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
        append("extern __thread _scl_stack_t _scl_internal_stack _scl_align;\n");
        append("extern __thread _scl_callstack_t _scl_internal_callstack _scl_align;\n");
        append("extern __thread struct _exception_handling {\n");
	    append("  scl_Exception* extable;\n");
	    append("  jmp_buf*       jmptable;\n");
	    append("  scl_int        ptr;\n");
	    append("  scl_int        cap;\n");
        append("  scl_int*       callstk_ptr;\n");
        append("} _scl_internal_exceptions _scl_align;\n");
        append("\n");

        append("/* STRUCTS */\n");
        append("struct scltype_iterable {\n");
        scopeDepth++;
        append("scl_int start;\n");
        append("scl_int end;\n");
        scopeDepth--;
        append("};\n");
        append("struct _scl_methodinfo {\n");
        scopeDepth++;;
        append("scl_any  ptr;\n");
        append("scl_int  pure_name;\n");
        scopeDepth--;
        append("};\n");
        append("struct _scl_typeinfo {\n");
        scopeDepth++;
        append("scl_int    type;\n");
        append("scl_int    super;\n");
        append("scl_int    methodscount;\n");
        append("struct _scl_methodinfo** methods;\n");
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

        for (Function* function : result.functions) {
            if (function->isMethod) {
                append("static void _scl_caller_func_%s$%s();\n", ((Method*)(function))->getMemberType().c_str(), function->getName().c_str());
            }
        }

        append("/* METHOD REFLECT */\n");
        scopeDepth++;
        for (Function* f : result.functions) {
            if (!f->isMethod) continue;
            Method* m = (Method*) f;
            append("static struct _scl_methodinfo _scl_methodinfo_method_%s_function_%s = (struct _scl_methodinfo) {\n", m->getMemberType().c_str(), f->getName().c_str());
            scopeDepth++;
            append(".ptr = (scl_any) _scl_caller_func_%s$%s,\n", m->getMemberType().c_str(), f->getName().c_str());
            append(".pure_name = 0x%xU,\n", hash1((char*) f->getName().c_str()));
            scopeDepth--;
            append("};\n");
        }
        scopeDepth--;

        append("/* TYPES */\n");
        for (Struct s : result.structs) {
            std::vector<Method*> methods = methodsOnType(result, s.getName());
            append("/* %s */\n", s.getName().c_str());
            if (methods.size()) {
                append("static struct _scl_methodinfo* _scl_methodinfo_ptrs_%s[] = (struct _scl_methodinfo*[]) {\n", s.getName().c_str());
                scopeDepth++;
                for (Method* m : methods) {
                    append("  &_scl_methodinfo_method_%s_function_%s,\n", m->getMemberType().c_str(), m->getName().c_str());
                }
                scopeDepth--;
                append("};\n");
            }
        }
        append("struct _scl_typeinfo _scl_internal_types[] = {\n");
        scopeDepth++;
        for (Struct s : result.structs) {
            append("/* %s */\n", s.getName().c_str());
            append("(struct _scl_typeinfo) {\n");
            scopeDepth++;
            append(".type = 0x%xU,\n", hash1((char*) s.getName().c_str()));
            append(".super = %uU,\n", hash1((char*) s.extends().c_str()));

            std::vector<Method*> methods = methodsOnType(result, s.getName());

            append(".methodscount = %zu,\n", methods.size());
            if (methods.size()) {
                append(".methods = (struct _scl_methodinfo**) _scl_methodinfo_ptrs_%s,\n", s.getName().c_str());
            } else {
                append(".methods = NULL,\n");
            }
            scopeDepth--;
            append("},\n");
        }
        scopeDepth--;
        append("};\n");
        append("size_t _scl_internal_types_count = %zu;\n\n", result.structs.size());
        append("static int _scl_do_method_check = 1;\n\n");

        append("extern hash hash1(char*);\n\n");

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

        append("/* FUNCTIONS */\n");

        for (size_t f = 0; f < result.functions.size(); f++) {
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
            lambdaCount = 0;

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
                if (!Main.options.minify) err.line = function->getNameToken().getLine();
                err.in = function->getNameToken().getFile();
                if (!Main.options.minify) err.column = function->getNameToken().getColumn();
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
                for (size_t i = 0; i < function->getArgs().size(); i++) {
                    Variable var = function->getArgs()[i];
                    vars[varDepth].push_back(var);
                    std::string type = sclTypeToCType(result, function->getArgs()[i].getType());
                    if (i) {
                        arguments += ", ";
                    }
                    arguments += type + " Var_" + function->getArgs()[i].getName();
                }
            }

            if (function->getReturnType().size() > 0) {
                std::string t = function->getReturnType();
                return_type = sclTypeToCType(result, t);
            }
            if (function->isMethod) {
                if (!((Method*)(function))->addAnyway() && getStructByName(result, ((Method*)(function))->getMemberType()).isSealed()) {
                    FPResult result;
                    result.message = "Struct '" + ((Method*)(function))->getMemberType() + "' is sealed!";
                    result.value = function->getNameToken().getValue();
                    if (!Main.options.minify) result.line = function->getNameToken().getLine();
                    result.in = function->getNameToken().getFile();
                    if (!Main.options.minify) result.column = function->getNameToken().getColumn();
                    result.type = function->getNameToken().getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                Method* m = (Method*) function;
                append("static void _scl_caller_func_%s$%s() {\n", m->getMemberType().c_str(), function->getName().c_str());
                scopeDepth++;
                generateUnsafeCall(m, fp, result);
                scopeDepth--;
                append("}\n");
                append("%s Method_%s$%s(%s) {\n", return_type.c_str(), ((Method*)(function))->getMemberType().c_str(), function->getName().c_str(), arguments.c_str());
            } else {
                if (contains<std::string>(function->getModifiers(), "<lambda>")) {
                    append("%s Function_%s() {\n", return_type.c_str(), function->getName().c_str());
                } else {
                    append("%s Function_%s(%s) {\n", return_type.c_str(), function->getName().c_str(), arguments.c_str());
                }
            }
            
            scopeDepth++;

            if (function->isMethod) {
                Method* m = static_cast<Method*>(function);
                append("if (!_scl_do_method_check) goto _scl_jmp_after_%s_%s;\n", m->getMemberType().c_str(), m->getName().c_str());
                {
                    append("_scl_internal_callstack.data[_scl_internal_callstack.ptr++].func = \"<%s>\";\n", sclFunctionNameToFriendlyString(functionDeclaration).c_str());
                    std::string file = function->getNameToken().getFile();
                    if (strstarts(file, scaleFolder)) {
                        file = file.substr(scaleFolder.size() + std::string("/Frameworks/").size());
                    } else {
                        file = std::filesystem::path(file).relative_path();
                    }
                    if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].file = \"%s\";\n", file.c_str());
                    if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].line = %d;\n", function->getNameToken().getLine());
                    if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].col = %d;\n", function->getNameToken().getColumn());
                }
                append("if (Var_self->$__type__ != 0x%xU) {\n", hash1((char*) m->getMemberType().c_str()));
                scopeDepth++;
                append("scl_any method = _scl_get_method_on_type(Var_self->$__type__, 0x%xU);\n", hash1((char*) sclFunctionNameToFriendlyString(m).c_str()));
                append("if (method != NULL) {\n");
                for (size_t k = 0; k < function->getArgs().size(); k++) {
                    Variable arg = function->getArgs()[k];
                    append("  _scl_push()->i = *(scl_int*) &Var_%s;\n", arg.getName().c_str());
                }
                append("  _scl_do_method_check = 0;\n");
                append("  _scl_internal_callstack.ptr--;\n");
                append("  ((void(*)()) method)();\n");
                append("  _scl_do_method_check = 1;\n");
                if (sclTypeToCType(result, m->getReturnType()) != "void") {
                    if (function->getReturnType() == "float") {
                        append("  return _scl_pop()->f;\n");
                    } else {
                        append("  return (%s) _scl_pop()->v;\n", sclTypeToCType(result, m->getReturnType()).c_str());
                    }
                } else {
                    append("  return;\n");
                }
                append("}\n");
                scopeDepth--;
                append("}\n");
                append("_scl_internal_callstack.ptr--;\n");
                scopeDepth--;
                append("_scl_jmp_after_%s_%s:", m->getMemberType().c_str(), m->getName().c_str());
                scopeDepth++;
                append("_scl_do_method_check = 1;\n");
            }

            std::vector<Token> body = function->getBody();
            if (body.size() == 0)
                goto emptyFunction;

            append("_scl_internal_callstack.data[_scl_internal_callstack.ptr++].func = \"%s\";\n", sclFunctionNameToFriendlyString(functionDeclaration).c_str());
            if (function->hasNamedReturnValue) {
                append("%s Var_%s;\n", sclTypeToCType(result, function->getNamedReturnValue().getType()).c_str(), function->getNamedReturnValue().getName().c_str());
            }

            {
                std::string file = function->getNameToken().getFile();
                if (strstarts(file, scaleFolder)) {
                    file = file.substr(scaleFolder.size() + std::string("/Frameworks/").size());
                } else {
                    file = std::filesystem::path(file).relative_path();
                }
                if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].file = \"%s\";\n", file.c_str());
                if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].line = %d;\n", function->getNameToken().getLine());
                if (!Main.options.minify) append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].col = %d;\n", function->getNameToken().getColumn());
                append("_scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].begin_stack_size = _scl_internal_stack.ptr;\n");
            }
            if (contains<std::string>(function->getModifiers(), "<lambda>")) {
                append("_scl_assert(_scl_internal_stack.ptr >= %zu, \"Not enough arguments for lambda\");\n", function->getArgs().size());
            }

            for (ssize_t a = (ssize_t) function->getArgs().size() - 1; a >= 0; a--) {
                Variable arg = function->getArgs()[a];
                if (contains<std::string>(function->getModifiers(), "<lambda>")) {
                    if (arg.getType() == "float")
                        append("%s Var_%s = _scl_pop()->f;\n", sclTypeToCType(result, arg.getType()).c_str(), arg.getName().c_str());
                    else
                        append("%s Var_%s = (%s) _scl_pop()->v;\n", sclTypeToCType(result, arg.getType()).c_str(), arg.getName().c_str(), sclTypeToCType(result, arg.getType()).c_str());
                }
                if (!typeCanBeNil(arg.getType())) {
                    append("_scl_assert(*(scl_int*) &Var_%s, \"Argument '%s' is nil!\");\n", arg.getName().c_str(), arg.getName().c_str());
                }
            }

            for (i = 0; i < body.size(); i++) {
                handle(Token);
            }
            scopeDepth = 1;
            if (body.size() == 0 || body[body.size() - 1].getType() != tok_return) {
                append("_scl_internal_callstack.ptr--;\n");
                if (!contains<std::string>(function->getModifiers(), "no_cleanup")) {
                    append("_scl_cleanup_post_func(_scl_internal_callstack.ptr);\n");
                }
            }

        emptyFunction:
            scopeDepth = 0;
            append("}\n\n");
        }
    }
} // namespace sclc
