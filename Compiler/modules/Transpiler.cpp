#include <filesystem>
#include <stack>
#include <functional>
#include <csignal>
#include <cstdlib>
#include <unistd.h>

#include "../headers/Common.hpp"
#include "../headers/TranspilerDefs.hpp"

#ifndef VERSION
#define VERSION ""
#endif

#define debugDump(_var) std::cout << __func__ << ":" << std::to_string(__LINE__) << ": " << #_var << ": " << _var << std::endl
#define typePop do { if (typeStack.size()) { typeStack.pop(); } } while (0)
#define safeInc() do { if (++i >= body.size()) { std::cerr << body.back().file << ":" << body.back().line << ":" << body.back().column << ":" << Color::RED << " Unexpected end of file!" << std::endl; std::raise(SIGSEGV); } } while (0)
#define safeIncN(n) do { if ((i + n) >= body.size()) { std::cerr << body.back().file << ":" << body.back().line << ":" << body.back().column << ":" << Color::RED << " Unexpected end of file!" << std::endl; std::raise(SIGSEGV); } i += n; } while (0)
#define THIS_INCREMENT_IS_CHECKED ++i;

namespace sclc {
    Function* currentFunction;
    Struct currentStruct("");
    std::unordered_map<std::string, std::vector<Function*>> functionsByFile;

    std::map<std::string, std::vector<Method*>> vtables;

    std::string sclFunctionNameToFriendlyString(Function* f);
    std::string sclFunctionNameToFriendlyString(std::string name);

    std::string capitalize(std::string s) {
        if (s.size() == 0) return s;
        s[0] = std::toupper(s[0]);
        return s;
    }

    bool hasTypealias(TPResult& r, std::string t) {
        t = removeTypeModifiers(t);
        for (const auto& p : r.typealiases) {
            if (p.first == t) {
                return true;
            }
        }
        return false;
    }

    std::string getTypealias(TPResult& r, std::string t) {
        t = removeTypeModifiers(t);
        for (const auto& p : r.typealiases) {
            if (p.first == t) {
                return p.second;
            }
        }
        return "";
    }

    std::string notNilTypeOf(std::string t) {
        while (t.size() && t != "?" && t.back() == '?') t = t.substr(0, t.size() - 1);
        return t;
    }

    const std::array<std::string, 3> removableTypeModifiers = {"mut ", "const ", "readonly "};

    std::string removeTypeModifiers(std::string t) {
        for (const std::string& modifier : removableTypeModifiers) {
            while (t.compare(0, modifier.size(), modifier) == 0) {
                t.erase(0, modifier.size());
            }
        }
        if (t.size() && t != "?" && t.back() == '?') {
            t = t.substr(0, t.size() - 1);
        }
        return t;
    }

    std::string sclTypeToCType(TPResult& result, std::string t) {
        if (t == "?") {
            return "scl_any";
        }
        t = removeTypeModifiers(t);

        if (strstarts(t, "lambda(")) return "_scl_lambda";
        if (t == "any") return "scl_any";
        if (t == "none") return "void";
        if (t == "nothing") return "_scl_no_return void";
        if (t == "int") return "scl_int";
        if (t == "uint") return "scl_uint";
        if (t == "float") return "scl_float";
        if (t == "bool") return "scl_bool";
        if (t == "varargs") return "...";
        if (!(getStructByName(result, t) == Struct::Null)) {
            return "scl_" + getStructByName(result, t).name;
        }
        if (t.size() > 2 && t.front() == '[') {
            std::string type = sclTypeToCType(result, t.substr(1, t.size() - 2));
            return type + "*";
        }
        if (getInterfaceByName(result, t)) {
            return "scl_any";
        }
        if (hasTypealias(result, t)) {
            return getTypealias(result, t);
        }
        if (hasLayout(result, t)) {
            return "scl_" + getLayout(result, t).name;
        }

        return "scl_any";
    }

    bool isPrimitiveIntegerType(std::string type);

    std::string sclIntTypeToConvert(std::string type) {
        if (type == "int") return "";
        if (type == "uint") return "Function_int$toUInt";
        if (type == "int8") return "Function_int$toInt8";
        if (type == "int16") return "Function_int$toInt16";
        if (type == "int32") return "Function_int$toInt32";
        if (type == "int64") return "";
        if (type == "uint8") return "Function_int$toUInt8";
        if (type == "uint16") return "Function_int$toUInt16";
        if (type == "uint32") return "Function_int$toUInt32";
        if (type == "uint64") return "";
        return "never_happens";
    }

    std::string generateArgumentsForFunction(TPResult& result, Function *func) {
        std::string args = "";
        size_t maxValue = func->args.size();
        if (func->isMethod) {
            args = "(" + sclTypeToCType(result, func->member_type) + ") _scl_positive_offset(" + std::to_string(func->args.size() - 1) + ")->i";
            maxValue--;
        }
        if (func->isCVarArgs())
            maxValue--;

        for (size_t i = 0; i < maxValue; i++) {
            Variable& arg = func->args[i];
            if (func->isMethod || i)
                args += ", ";

            std::string typeRemoved = removeTypeModifiers(arg.type);

            if (typeRemoved == "float") {
                args += "_scl_positive_offset(" + std::to_string(i) + ")->f";
            } else if (typeRemoved == "str") {
                args += "_scl_positive_offset(" + std::to_string(i) + ")->s";
            } else if (typeRemoved == "any") {
                args += "_scl_positive_offset(" + std::to_string(i) + ")->v";
            } else {
                args += "(" + sclTypeToCType(result, typeRemoved) + ") _scl_positive_offset(" + std::to_string(i) + ")->i";
            }
        }
        return args;
    }

    std::string argsToRTSignature(Function* f);

    size_t indexOf(std::vector<std::string>& v, std::string s) {
        for (size_t i = 0; i < v.size(); i++) {
            if (v[i] == s) return i;
        }
        return -1;
    }

    std::string generateSymbolForFunction(Function* f) {
        if (f->has_asm) {
            std::string label = f->getModifier(f->has_asm + 1);

            return std::string("\"") + label + "\"";
        }

        if (f->has_cdecl) {
            std::string cLabel = f->getModifier(f->has_cdecl + 1);

            return std::string("_scl_macro_to_string(__USER_LABEL_PREFIX__) \"") + cLabel + "\"";
        }

        std::string symbol = f->finalName();

        if (f->has_lambda) {
            std::string functionSymbol = f->getModifier(f->has_lambda + 1).substr(1);
            functionSymbol = functionSymbol.substr(0, functionSymbol.size() - 1);
            std::string lambda = f->finalName().substr(8);
            std::string count = lambda.substr(0, lambda.find("$"));
            symbol = "lambda[" + count + "@" + functionSymbol + "]" + argsToRTSignature(f);
        } else if (f->isMethod) {
            Method* m = ((Method*) f);
            if (f->isExternC || f->has_expect || f->has_export) {
                symbol = m->member_type + "$" + f->finalName();
            } else {
                std::string name = f->finalName();
                name = sclFunctionNameToFriendlyString(name);

                symbol = m->member_type + ":" + name;
                symbol += argsToRTSignature(f);
            }
        } else {
            std::string finalName = symbol;
            if (f->isExternC || f->has_expect || f->has_export) {
                symbol = finalName;
            } else {
                symbol = sclFunctionNameToFriendlyString(f->name);
                symbol += argsToRTSignature(f);
            }
        }

        if (f->isExternC || f->has_expect || f->has_export) {
            return "_scl_macro_to_string(__USER_LABEL_PREFIX__) \"" + symbol + "\"";
        }
        return "_scl_macro_to_string(__USER_LABEL_PREFIX__) \"" + symbol + "\"";
    }

    std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
        std::vector<std::string> body;
        size_t start = 0;
        size_t end = 0;
        while ((end = str.find(delimiter, start)) != std::string::npos)
        {
            body.push_back(str.substr(start, end - start));
            start = end + delimiter.size();
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

        append("#include <scale_runtime.h>\n");
        for (std::string& header : Main.frameworkNativeHeaders) {
            append("#include <%s>\n", header.c_str());
        }
        append("\n");
    }

    void ConvertC::writeFunctionHeaders(FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        append("/* FUNCTION HEADERS */\n");

        for (Function* function : result.functions) {
            std::string return_type = sclTypeToCType(result, function->return_type);
            auto args = function->args;
            std::string arguments;
            arguments.reserve(32);
            
            if (function->isMethod) {
                currentStruct = getStructByName(result, function->member_type);
                arguments += sclTypeToCType(result, function->member_type) + " Var_self";
                for (long i = 0; i < (long) args.size() - 1; i++) {
                    std::string type = sclTypeToCType(result, args[i].type);
                    arguments += ", " + type;
                    if (type == "varargs" || type == "...") continue;
                    if (args[i].name.size())
                        arguments += " Var_" + args[i].name;
                }
            } else {
                currentStruct = Struct::Null;
                if (args.size() > 0) {
                    for (long i = 0; i < (long) args.size(); i++) {
                        std::string type = sclTypeToCType(result, args[i].type);
                        if (i) {
                            arguments += ", ";
                        }
                        arguments += type;
                        if (type == "varargs" || type == "...") continue;
                        if (args[i].name.size())
                            arguments += " Var_" + args[i].name;
                    }
                }
            }

            if (arguments.empty()) {
                arguments = "void";
            }

            std::string symbol = generateSymbolForFunction(function);

            if (!function->isMethod) {
                if (function->name == "throw" || function->name == "builtinUnreachable") {
                    append("_scl_no_return ");
                }
                append("%s Function_%s(%s)\n", return_type.c_str(), function->finalName().c_str(), arguments.c_str());
                append("    __asm(%s);\n", symbol.c_str());
            } else {
                append("%s Method_%s$%s(%s)\n", return_type.c_str(), function->member_type.c_str(), function->finalName().c_str(), arguments.c_str());
                append("    __asm(%s);\n", symbol.c_str());
            }
        }

        append("\n");
    }

    std::string scaleArgs(std::vector<Variable> args) {
        std::string result = "";
        for (size_t i = 0; i < args.size(); i++) {
            if (i) {
                result += ", ";
            }
            result += args[i].name + ": " + args[i].type;
        }
        return result;
    }

    bool structImplements(TPResult& result, Struct s, std::string interface) {
        do {
            if (s.implements(interface)) {
                return true;
            }
            s = getStructByName(result, s.super);
        } while (s.name.size());
        return false;
    }

    void ConvertC::writeGlobals(FILE* fp, std::vector<Variable>& globals, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        if (result.globals.size() == 0) return;
        append("/* GLOBALS */\n");

        for (Variable& s : result.globals) {
            append("extern %s Var_%s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
            globals.push_back(s);
        }

        append("\n");
    }

    void ConvertC::writeContainers(FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        append("/* STRUCT TYPES */\n");
        for (Struct& c : result.structs) {
            if (c.name == "str" || c.name == "any" || c.name == "int" || c.name == "float" || isPrimitiveIntegerType(c.name)) continue;
            append("typedef struct Struct_%s* scl_%s;\n", c.name.c_str(), c.name.c_str());
        }
        append("\n");
        for (Layout& c : result.layouts) {
            append("typedef struct Layout_%s* scl_%s;\n", c.name.c_str(), c.name.c_str());
        }
        for (Layout& c : result.layouts) {
            append("struct Layout_%s {\n", c.name.c_str());
            for (Variable& s : c.members) {
                append("  %s %s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
            }
            append("};\n");
        }
        append("\n");
        if (result.containers.size() == 0) return;
        
        append("/* CONTAINERS */\n");
        for (Container& c : result.containers) {
            append("struct _Container_%s {\n", c.name.c_str());
            for (Variable& s : c.members) {
                append("  %s %s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
            }
            append("};\n");
            append("extern struct _Container_%s Container_%s;\n", c.name.c_str(), c.name.c_str());
        }
    }

    bool typeEquals(const std::string& a, const std::string& b);

    bool argsAreIdentical(std::vector<Variable>& methodArgs, std::vector<Variable>& functionArgs) {
        if ((methodArgs.size() - 1) != functionArgs.size()) return false;
        for (size_t i = 0; i < methodArgs.size(); i++) {
            if (methodArgs[i].name == "self" || functionArgs[i].name == "self") continue;
            if (!typeEquals(methodArgs[i].type, functionArgs[i].type)) return false;
        }
        return true;
    }

    std::string argVectorToString(std::vector<Variable>& args) {
        std::string arg = "";
        for (size_t i = 0; i < args.size(); i++) {
            if (i) 
                arg += ", ";
            const Variable& v = args[i];
            arg += removeTypeModifiers(v.type);
        }
        return arg;
    }

    template<typename T>
    std::vector<T> operator&(const std::vector<T>& a, std::function<bool(T)> b) {
        std::vector<T> result;
        for (auto&& t : a) {
            if (b(t)) {
                result.push_back(t);
            }
        }
        return result;
    }

    std::vector<Method*> makeVTable(TPResult& res, std::string name);
    std::string argsToRTSignatureIdent(Function* f);

    void ConvertC::writeStructs(FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        if (result.structs.size() == 0) return;
        append("/* STRUCT DEFINITIONS */\n");

        for (Struct& c : result.structs) {
            if (c.isStatic()) continue;
            currentStruct = c;
            for (const std::string& i : c.interfaces) {
                Interface* interface = getInterfaceByName(result, i);
                if (interface == nullptr) {
                    FPResult res;
                    Token t = c.name_token;
                    res.success = false;
                    res.message = "Struct '" + c.name + "' implements unknown interface '" + i + "'";
                    res.line = t.line;
                    res.in = t.file;
                    res.column = t.column;
                    res.value = t.value;
                    res.type = t.type;
                    errors.push_back(res);
                    continue;
                }
                for (Function* f : interface->toImplement) {
                    if (!hasMethod(result, f->name, c.name)) {
                        FPResult res;
                        Token t = c.name_token;
                        res.success = false;
                        res.message = "No implementation for method '" + f->name + "' on struct '" + c.name + "'";
                        res.line = t.line;
                        res.in = t.file;
                        res.column = t.column;
                        res.value = t.value;
                        res.type = t.type;
                        errors.push_back(res);
                        continue;
                    }
                    Method* m = getMethodByName(result, f->name, c.name);
                    if (!argsAreIdentical(m->args, f->args)) {
                        FPResult res;
                        Token t = m->name_token;
                        res.success = false;
                        res.message = "Arguments of method '" + c.name + ":" + m->name + "' do not match implemented method '" + f->name + "'";
                        res.line = t.line;
                        res.in = t.file;
                        res.column = t.column;
                        res.value = t.value;
                        res.type = t.type;
                        errors.push_back(res);
                        continue;
                    }
                    Struct argType = getStructByName(result, m->return_type);
                    bool implementedInterface = false;
                    if (argType != Struct::Null) {
                        Interface* interface = getInterfaceByName(result, f->return_type);
                        if (interface != nullptr) {
                            implementedInterface = structImplements(result, argType, interface->name);
                        }
                    }
                    if (!implementedInterface && f->return_type != "?" && !typeEquals(m->return_type, f->return_type)) {
                        FPResult res;
                        Token t = m->name_token;
                        res.success = false;
                        res.message = "Return type of method '" + c.name + ":" + m->name + "' does not match implemented method '" + f->name + "'. Return type should be: '" + f->return_type + "'";
                        res.line = t.line;
                        res.in = t.file;
                        res.column = t.column;
                        res.value = t.value;
                        res.type = t.type;
                        errors.push_back(res);
                        continue;
                    }
                    if (f->return_type == "?" && m->return_type == "none") {
                        FPResult res;
                        Token t = m->name_token;
                        res.success = false;
                        res.message = "Cannot return 'none' from method '" + c.name + ":" + m->name + "' when implemented method '" + f->name + "' returns '" + f->return_type + "'";
                        res.line = t.line;
                        res.in = t.file;
                        res.column = t.column;
                        res.value = t.value;
                        res.type = t.type;
                        errors.push_back(res);
                        continue;
                    }
                }
            }

            for (Function* f : result.functions) {
                if (!f->isMethod) continue;
                Method* m = (Method*) f;
                if (m->member_type != c.name) continue;

                Struct super = getStructByName(result, c.super);
                while (super != Struct::Null) {
                    Method* other = getMethodByNameOnThisType(result, m->name, super.name);
                    if (!other) goto afterSealedCheck;
                    if (other->has_sealed) {
                        FPResult res;
                        Token t = m->name_token;
                        res.success = false;
                        res.message = "Method '" + f->name + "' overrides 'sealed' method on '" + super.name + "'";
                        res.line = t.line;
                        res.in = t.file;
                        res.column = t.column;
                        res.value = t.value;
                        res.type = t.type;
                        errors.push_back(res);
                        FPResult res2;
                        Token t2 = other->name_token;
                        res2.success = false;
                        res2.isNote = true;
                        res2.message = "Declared here:";
                        res2.line = t2.line;
                        res2.in = t2.file;
                        res2.column = t2.column;
                        res2.value = t2.value;
                        res2.type = t2.type;
                        errors.push_back(res2);
                        break;
                    }
                afterSealedCheck:
                    super = getStructByName(result, super.super);
                }
            }

            makeVTable(result, c.name);

            append("struct Struct_%s {\n", c.name.c_str());
            append("  const _scl_lambda* const $fast;\n");
            append("  const TypeInfo* $statics;\n");
            append("  mutex_t $mutex;\n");
            for (Variable& s : c.members) {
                append("  %s %s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
            }
            append("};\n");
        }
        currentStruct = Struct::Null;
        append("\n");
    }

    int scopeDepth = 0;
    size_t i = 0;
    size_t condCount = 0;
    std::vector<char> whatWasIt;

    template<typename T>
    void addIfAbsent(std::vector<T>& vec, T val) {
        if (vec.size() == 0 || !contains<T>(vec, val))
            vec.push_back(val);
    }

#define wasRepeat()     (whatWasIt.size() > 0 && whatWasIt.back() == 1)
#define popRepeat()     (whatWasIt.pop_back())
#define pushRepeat()    (whatWasIt.push_back(1))
#define wasCatch()      (whatWasIt.size() > 0 && whatWasIt.back() == 2)
#define popCatch()      (whatWasIt.pop_back())
#define pushCatch()     (whatWasIt.push_back(2))
#define wasIterate()    (whatWasIt.size() > 0 && whatWasIt.back() == 4)
#define popIterate()    (whatWasIt.pop_back())
#define pushIterate()   (whatWasIt.push_back(4))
#define wasTry()        (whatWasIt.size() > 0 && whatWasIt.back() == 8)
#define popTry()        (whatWasIt.pop_back())
#define pushTry()       (whatWasIt.push_back(8))
#define wasSwitch()     (whatWasIt.size() > 0 && whatWasIt.back() == 16)
#define popSwitch()     (whatWasIt.pop_back())
#define pushSwitch()    (whatWasIt.push_back(16))
#define wasCase()       (whatWasIt.size() > 0 && whatWasIt.back() == 32)
#define popCase()       (whatWasIt.pop_back())
#define pushCase()      (whatWasIt.push_back(32))
#define wasOther()      (whatWasIt.size() > 0 && whatWasIt.back() == 0)
#define popOther()      (whatWasIt.pop_back())
#define pushOther()     (whatWasIt.push_back(0))

#define varScopePush()                          \
    do                                          \
    {                                           \
        var_indices.push_back(vars.size());     \
    } while (0)
#define varScopePop()                           \
    do                                          \
    {                                           \
        vars.erase(vars.begin() + var_indices.back(), vars.end()); \
        var_indices.pop_back();                 \
    } while (0)
#define varScopeTop() vars

    char repeat_depth = 0;
    int iterator_count = 0;
    bool noWarns;
    int isInUnsafe = 0;
    std::stack<std::string> typeStack;
#define typeStackTop (typeStack.size() ? typeStack.top() : "")
    std::string return_type = "";

    bool canBeCastTo(TPResult& r, const Struct& one, const Struct& other) {
        if (one == other || other.name == "SclObject") {
            return true;
        }
        if ((one.isStatic() && !other.isStatic()) || (!one.isStatic() && other.isStatic())) {
            return false;
        }
        Struct oneParent = getStructByName(r, one.super);
        bool extend = false;
        while (!extend) {
            if (oneParent == other) {
                extend = true;
            } else if (oneParent.name == "SclObject" || oneParent.name.empty()) {
                return false;
            }
            oneParent = getStructByName(r, oneParent.super);
        }
        return extend;
    }

    std::string lambdaReturnType(const std::string& lambda) {
        if (strstarts(lambda, "lambda("))
            return lambda.substr(lambda.find_last_of("):") + 1);
        return "";
    }

    size_t lambdaArgCount(const std::string& lambda) {
        if (strstarts(lambda, "lambda(")) {
            std::string args = lambda.substr(lambda.find_first_of("(") + 1, lambda.find_last_of("):") - lambda.find_first_of("(") - 1);
            return std::stoi(args);
        }
        return -1;
    }

    bool lambdasEqual(const std::string& a, const std::string& b) {
        std::string returnTypeA = lambdaReturnType(a);
        std::string returnTypeB = lambdaReturnType(b);

        size_t argAmountA = lambdaArgCount(a);
        size_t argAmountB = lambdaArgCount(b);

        return argAmountA == argAmountB && typeEquals(returnTypeA, returnTypeB);
    }

    bool typeEquals(const std::string& a, const std::string& b) {
        if (removeTypeModifiers(a) == removeTypeModifiers(b)) {
            return true;
        } else if (a.size() > 2 && a.front() == '[') {
            if (b.size() > 2 && b.front() == '[') {
                return typeEquals(a.substr(1, a.size() - 1), b.substr(1, b.size() - 1));
            }
        } else if (strstarts(a, "lambda(") && strstarts(b, "lambda(")) {
            return lambdasEqual(a, b);
        } else if (a == "?" || b == "?") {
            return true;
        }
        return false;
    }

    bool typeIsUnsigned(std::string s) {
        s = removeTypeModifiers(s);
        return isPrimitiveIntegerType(s) && s.front() == 'u';
    }

    bool typeIsSigned(std::string s) {
        s = removeTypeModifiers(s);
        return isPrimitiveIntegerType(s) && s.front() == 'i';
    }

    size_t intBitWidth(std::string s) {
        std::string type = removeTypeModifiers(s);
        if (type == "int" || type == "uint") return 64;
        if (type == "int8" || type == "uint8") return 8;
        if (type == "int16" || type == "uint16") return 16;
        if (type == "int32" || type == "uint32") return 32;
        if (type == "int64" || type == "uint64") return 64;
        return 0;
    }

    bool typesCompatible(TPResult& result, std::string stack, std::string arg, bool allowIntPromotion) {
        bool stackTypeIsNilable = typeCanBeNil(stack);
        bool argIsNilable = typeCanBeNil(arg);
        stack = removeTypeModifiers(stack);
        arg = removeTypeModifiers(arg);
        if (typeEquals(stack, arg) && stackTypeIsNilable == argIsNilable) {
            return true;
        }

        if (allowIntPromotion) {
            if (isPrimitiveIntegerType(arg) && isPrimitiveIntegerType(stack)) {
                return true;
            }
        } else {
            if (isPrimitiveIntegerType(arg) && isPrimitiveIntegerType(stack)) {
                if (typeIsUnsigned(arg) && typeIsSigned(stack)) {
                    return false;
                } else if (typeIsSigned(arg) && typeIsUnsigned(stack)) {
                    return false;
                } else if (intBitWidth(arg) != intBitWidth(stack)) {
                    return false;
                }
                return true;
            }
        }

        if (arg == "any" || arg == "[any]" || (argIsNilable && arg != "float" && (stack == "any" || stack == "[any]"))) {
            return true;
        }

        Interface* interface = getInterfaceByName(result, arg);
        if (stackTypeIsNilable && !argIsNilable) {
            return false;
        } else if (interface) {
            const Struct& givenType = getStructByName(result, stack);
            if (givenType == Struct::Null) {
                return false;
            } else if (!givenType.implements(interface->name)) {
                return false;
            }
        } else if (!typeEquals(stack, arg)) {
            const Struct& givenType = getStructByName(result, stack);
            const Struct& requiredType = getStructByName(result, arg);
            if (givenType == Struct::Null) {
                return false;
            } else if (requiredType == Struct::Null) {
                return false;
            } else if (!canBeCastTo(result, givenType, requiredType)) {
                return false;
            }
        }
        return true;
    }

    Method* attributeAccessor(TPResult& result, std::string struct_, std::string member) {
        Method* f = getMethodByName(result, "get" + capitalize(member), struct_);
        return f && f->has_getter ? f : nullptr;
    }

    Method* attributeMutator(TPResult& result, std::string struct_, std::string member) {
        Method* f = getMethodByName(result, "set" + capitalize(member), struct_);
        return f && f->has_setter ? f : nullptr;
    }

    bool checkStackType(TPResult& result, std::vector<Variable>& args, bool allowIntPromotion = false) {
        if (args.size() == 0) {
            return true;
        }
        if (typeStack.size() < args.size()) {
            return false;
        }

        if (args.back().type == "varargs") {
            return true;
        }

        auto end = &typeStack.top() + 1;
        auto begin = end - args.size();
        std::vector<std::string> stack(begin, end);

        for (ssize_t i = args.size() - 1; i >= 0; i--) {
            bool tmp = typesCompatible(result, stack[i], args[i].type, allowIntPromotion);
            if (tmp == false) {
                return false;
            }
        }
        return true;
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
            std::string stackType = tmp[i];
            
            if (i) {
                arg += ", ";
            }

            arg += stackType;
        }
        return arg;
    }

    void generateUnsafeCall(Method* self, FILE* fp, TPResult& result) {
        if (self->args.size() > 0)
            append("_stack.sp -= (%zu);\n", self->args.size());
        if (removeTypeModifiers(self->return_type) == "none" || removeTypeModifiers(self->return_type) == "nothing") {
            append("Method_%s$%s(%s);\n", self->member_type.c_str(), self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
        } else {
            if (removeTypeModifiers(self->return_type) == "float") {
                append("(_stack.sp++)->f = Method_%s$%s(%s);\n", self->member_type.c_str(), self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
            } else {
                append("(_stack.sp++)->i = (scl_int) Method_%s$%s(%s);\n", self->member_type.c_str(), self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
            }
        }
    }

    bool hasImplementation(TPResult& result, Function* func) {
        for (Function* f : result.functions) {
            if (*f == *func) {
                return true;
            }
        }
        return false;
    }

    std::string sclFunctionNameToFriendlyString(std::string);

    bool shouldCall(Function* self, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t i) {
        if (self->deprecated.size()) {
            Deprecation& depr = self->deprecated;
            std::string since = depr["since"];
            std::string replacement = depr["replacement"];
            bool forRemoval = depr["forRemoval"] == "true";

            Version sinceVersion = Version(since);
            Version thisVersion = Version(VERSION);
            if (sinceVersion > thisVersion) {
                goto notDeprecated;
            }
            
            std::string message = "Function '" + self->name + "' is deprecated";
            if (forRemoval) {
                message += " and marked for removal";
            }
            if (since.size()) {
                message += " in Version " + since + " and later";
            }
            message += "!";
            if (replacement.size()) {
                message += " Use '" + replacement + "' instead.";
            }
            transpilerError(message, i);
            warns.push_back(err);
        }
    notDeprecated:
        if (self->has_unsafe) {
            if (!isInUnsafe) {
                transpilerError("Calling unsafe function requires unsafe block or function", i);
                errors.push_back(err);
                return false;
            }
        }
        if (self->has_construct) {
            transpilerError("Calling construct function is not supported", i);
            errors.push_back(err);
            return false;
        }
        return true;
    }

    std::vector<bool> bools{false, true};

    bool opFunc(std::string name) {
        name = name.substr(0, name.find("$$ol"));
        return (name == "operator$add") ||
               (name == "operator$sub") ||
               (name == "operator$mul") ||
               (name == "operator$div") ||
               (name == "operator$mod") ||
               (name == "operator$logic_and") ||
               (name == "operator$logic_or") ||
               (name == "operator$logic_xor") ||
               (name == "operator$logic_not") ||
               (name == "operator$logic_lsh") ||
               (name == "operator$logic_rsh") ||
               (name == "operator$pow") ||
               (name == "operator$equal") ||
               (name == "operator$not_equal") ||
               (name == "operator$less") ||
               (name == "operator$more") ||
               (name == "operator$less_equal") ||
               (name == "operator$more_equal") ||
               (name == "operator$bool_and") ||
               (name == "operator$bool_or") ||
               (name == "operator$get") ||
               (name == "operator$set") ||
               (name == "operator$not");
    }

    std::string opToString(std::string op) {
        if (strstarts(op, "add")) return "+";
        if (strstarts(op, "sub")) return "-";
        if (strstarts(op, "mul")) return "*";
        if (strstarts(op, "div")) return "/";
        if (strstarts(op, "pow")) return "**";
        if (op == "mod") return "%";
        if (op == "land") return "&";
        if (op == "lor") return "|";
        if (op == "lxor") return "^";
        if (op == "lnot") return "~";
        if (op == "lsl") return "<<";
        if (op == "lsr") return ">>";
        if (op == "rol") return "<<<";
        if (op == "ror") return ">>>";
        return "???";
    }

    void functionCall(Function* self, FILE* fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t i, bool withIntPromotion = false, bool hasToCallStatic = false, bool checkOverloads = true);

    template<typename T>
    std::vector<T> joinVecs(std::vector<T> a, std::vector<T> b) {
        std::vector<T> ret;
        for (T& t : a) {
            ret.push_back(t);
        }
        for (T& t : b) {
            ret.push_back(t);
        }
        return ret;
    }

    Method* findMethodLocally(Method* self, TPResult& result) {
        for (Function* f : result.functions) {
            if (!f->isMethod) continue;
            Method* m = (Method*) f;
            std::string name = m->name.substr(0, m->name.find("$$ol"));
            if ((name == self->name) && m->member_type == self->member_type) {
                if (currentFunction) {
                    if (m->name_token.file == currentFunction->name_token.file) {
                        return m;
                    }
                }
                return m;
            }
        }
        return nullptr;
    }

    Function* findFunctionLocally(Function* self, TPResult& result) {
        for (Function* f : result.functions) {
            if (f->isMethod) continue;
            std::string name = f->name.substr(0, f->name.find("$$ol"));
            if (name == self->name) {
                if (currentFunction) {
                    if (f->name_token.file == currentFunction->name_token.file) {
                        return f;
                    }
                } else {
                    return f;
                }
            }
        }
        return nullptr;
    }

    std::unordered_map<Function*, std::string> functionPtrs;

    std::string getFunctionType(TPResult& result, Function* self) {
        std::string functionPtrCast = functionPtrs[self];
        if (functionPtrCast.size()) {
            return functionPtrCast;
        }
        if (self->return_type != "none" && self->return_type != "nothing") {
            functionPtrCast = sclTypeToCType(result, self->return_type) + "(*)(";
        } else {
            functionPtrCast = "void(*)(";
        }

        if (self->isMethod) {
            functionPtrCast += sclTypeToCType(result, self->member_type);
            if (self->args.size() > 1)
                functionPtrCast += ", ";
        }

        for (size_t i = 0; i < self->args.size() - ((size_t) self->isMethod); i++) {
            if (i) {
                functionPtrCast += ", ";
            }
            functionPtrCast += sclTypeToCType(result, self->args[i].type);
        }

        functionPtrCast += ")";

        functionPtrs[self] = functionPtrCast;

        return functionPtrCast;
    }

    std::string argsToRTSignature(Function*);

    void methodCall(Method* self, FILE* fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t i, bool ignoreArgs = false, bool doActualPop = true, bool withIntPromotion = false, bool onSuperType = false, bool checkOverloads = true) {
        if (!shouldCall(self, warns, errors, body, i)) {
            return;
        }
        if (currentFunction) {
            if (self->has_private) {
                Token selfNameToken = self->name_token;
                Token fromNameToken = currentFunction->name_token;
                if (selfNameToken.file != fromNameToken.file) {
                    Method* method = findMethodLocally(self, result);
                    if (!method) {
                        transpilerError("Calling private method from another file is not allowed", i);
                        errors.push_back(err);
                        return;
                    }
                    if (method != self) {
                        methodCall(method, fp, result, warns, errors, body, i, ignoreArgs, doActualPop, withIntPromotion, onSuperType);
                        return;
                    }
                }
            }
        }

        Method* tmp = checkOverloads ? findMethodLocally(self, result) : nullptr;
        if (tmp) {
            self = tmp;
        }
        std::vector<Function*>& overloads = self->overloads;
        bool argsEqual = ignoreArgs || checkStackType(result, self->args, withIntPromotion);
        if (checkOverloads && overloads.size() && !argsEqual && !ignoreArgs) {
            for (bool b : bools) {
                for (Function* overload : overloads) {
                    if (!overload->isMethod) continue;
                    
                    bool argsEqual = checkStackType(result, overload->args, b);
                    if (argsEqual) {
                        methodCall((Method*) overload, fp, result, warns, errors, body, i, ignoreArgs, doActualPop, b);
                        return;
                    }
                }
            }
        }

        argsEqual = checkStackType(result, self->args);

        if (!argsEqual && !ignoreArgs) {
            if (opFunc(self->name)) {
                Function* f = getFunctionByName(result, self->name);
                if (f) {
                    functionCall(f, fp, result, warns, errors, body, i, false, true);
                    return;
                }
            }

            {
                transpilerError("Arguments for method '" + sclFunctionNameToFriendlyString(self) + "' do not equal inferred stack", i);
                errors.push_back(err);
            }

            std::string overloadedMatches = "";
            if (overloads.size()) {
                for (auto&& overload : overloads) {
                    if (overload == self) continue;
                    overloadedMatches += ", or [ " + Color::BLUE + argVectorToString(overload->args) + Color::RESET + " ]";
                }
            }

            transpilerError("Expected: [ " + Color::BLUE + argVectorToString(self->args) + Color::RESET + " ]" + overloadedMatches + ", but got: [ " + Color::RED + stackSliceToString(self->args.size()) + Color::RESET + " ]", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }

        if (self->isExternC && !hasImplementation(result, self)) {
            std::string functionDeclaration = "";

            functionDeclaration += ((Method*) self)->member_type + ":" + self->name + "(";
            for (size_t i = 0; i < self->args.size() - 1; i++) {
                if (i != 0) {
                    functionDeclaration += ", ";
                }
                functionDeclaration += self->args[i].name + ": " + self->args[i].type;
            }
            functionDeclaration += "): " + self->return_type;
            append("*(_stack.tp++) = \"<extern %s>\";\n", sclFunctionNameToFriendlyString(self).c_str());
        }
        
        if (doActualPop) {
            for (size_t m = 0; m < self->args.size(); m++) {
                typePop;
            }
        }
        if (getInterfaceByName(result, self->member_type)) {
            append("// invokeinterface %s:%s\n", self->member_type.c_str(), self->finalName().c_str());
        } else {
            if (onSuperType) {
                append("// invokespecial %s:%s\n", self->member_type.c_str(), self->finalName().c_str());
            } else {
                append("// invokedynamic %s:%s\n", self->member_type.c_str(), self->finalName().c_str());
            }
        }
        bool found = false;
        if (getInterfaceByName(result, self->member_type) == nullptr) {
            auto vtable = vtables[self->member_type];
            size_t index = 0;
            size_t argc;
            if ((argc = self->args.size())) {
                append("_scl_popn(%zu);\n", argc);
            }
            std::string args = generateArgumentsForFunction(result, self);
            for (auto&& method : vtable) {
                if (method->operator==(self)) {
                    if (Main.options.debugBuild) {
                        append("CAST0((_stack.sp - 1)->v, %s, 0x%x);\n", self->member_type.c_str(), id((char*) self->member_type.c_str()));
                    }
                    if (self->return_type != "none" && self->return_type != "nothing") {
                        append("*(%s*) _stack.sp++ = ", sclTypeToCType(result, self->return_type).c_str());
                    } else {
                        append("");
                    }

                    std::string functionPtrCast = getFunctionType(result, self);

                    append2("((%s) _scl_positive_offset(%zu)->o->", functionPtrCast.c_str(), argc - 1);
                    if (onSuperType) {
                        append2("$statics->super_vtable[%zu].ptr)(%s);\n", index, args.c_str());
                    } else {
                        append2("$fast[%zu])(%s);\n", index, args.c_str());
                    }
                    found = true;
                    break;
                }
                index++;
            }
        } else {
            std::string rtSig = argsToRTSignature(self);
            append("_scl_call_method_or_throw((_stack.sp - 1)->v, 0x%xU, 0x%xU, 0, \"%s\", \"%s\");\n", id((char*) self->name.c_str()), id((char*) rtSig.c_str()), self->name.c_str(), rtSig.c_str());
            found = true;
        }
        if (removeTypeModifiers(self->return_type) != "none" && removeTypeModifiers(self->return_type) != "nothing") {
            typeStack.push(self->return_type);
        }
        if (self->isExternC && !hasImplementation(result, self)) {
            append("--_stack.tp;\n");
        }
        if (!found) {
            transpilerError("Method '" + sclFunctionNameToFriendlyString(self) + "' not found on type '" + self->member_type + "'", i);
            errors.push_back(err);
        }
    }

    void generateUnsafeCallF(Function* self, FILE* fp, TPResult& result) {
        if (self->args.size() > 0)
            append("_stack.sp -= (%zu);\n", self->args.size());
        if (removeTypeModifiers(self->return_type) == "none" || removeTypeModifiers(self->return_type) == "nothing") {
            append("Function_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
        } else {
            if (removeTypeModifiers(self->return_type) == "float") {
                append("(_stack.sp++)->f = Function_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
            } else {
                append("(_stack.sp++)->i = (scl_int) Function_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
            }
        }
    }

    bool argsContainsIntType(std::vector<Variable>& args) {
        for (auto&& arg : args) {
            if (isPrimitiveIntegerType(arg.type)) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    size_t indexInVec(std::vector<T>& vec, T elem) {
        for (size_t i = 0; i < vec.size(); i++) {
            if (vec[i] == elem) {
                return i;
            }
        }
        return -1;
    }

    bool operatorInt(const std::string& op) {
        return op == "add_ii" ||
               op == "sub_ii" ||
               op == "mul_ii" ||
               op == "div_ii" ||
               op == "pow_ii" ||
               op == "mod" ||
               op == "land" ||
               op == "lor" ||
               op == "lxor" ||
               op == "lnot" ||
               op == "lsr" ||
               op == "lsl" ||
               op == "ror" ||
               op == "rol";
    }

    bool operatorFloat(const std::string& op) {
        return op == "add_if" ||
               op == "add_fi" ||
               op == "add_ff" ||
               op == "sub_if" ||
               op == "sub_fi" ||
               op == "sub_ff" ||
               op == "mul_if" ||
               op == "mul_fi" ||
               op == "mul_ff" ||
               op == "div_if" ||
               op == "div_fi" ||
               op == "div_ff" ||
               op == "pow_if" ||
               op == "pow_fi" ||
               op == "pow_ff";
    }

    bool operatorBool(const std::string& op) {
        return op == "eq_ii" ||
               op == "eq_if" ||
               op == "eq_fi" ||
               op == "eq_ff" ||
               op == "ne_ii" ||
               op == "ne_if" ||
               op == "ne_fi" ||
               op == "ne_ff" ||
               op == "gt_ii" ||
               op == "gt_if" ||
               op == "gt_fi" ||
               op == "gt_ff" ||
               op == "ge_ii" ||
               op == "ge_if" ||
               op == "ge_fi" ||
               op == "ge_ff" ||
               op == "lt_ii" ||
               op == "lt_if" ||
               op == "lt_fi" ||
               op == "lt_ff" ||
               op == "le_ii" ||
               op == "le_if" ||
               op == "le_fi" ||
               op == "le_ff" ||
               op == "and" ||
               op == "or" ||
               op == "not";
    }

    std::map<std::string, std::string> getTemplates(TPResult& result, Function* func) {
        if (!func->isMethod) {
            return std::map<std::string, std::string>();
        }
        Struct s = getStructByName(result, func->member_type);
        return s.templates;
    }

    void functionCall(Function* self, FILE* fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t i, bool withIntPromotion, bool hasToCallStatic, bool checkOverloads) {
        if (!shouldCall(self, warns, errors, body, i)) {
            return;
        }
        if (currentFunction) {
            if (self->has_private) {
                if (self->name_token.file != currentFunction->name_token.file) {
                    Function* function = findFunctionLocally(self, result);
                    if (!function) {
                        transpilerError("Calling private function from another file is not allowed", i);
                        errors.push_back(err);
                        return;
                    }
                    functionCall(function, fp, result, warns, errors, body, i, withIntPromotion, hasToCallStatic);
                    return;
                }
            }
        }

        Function* function = checkOverloads ? findFunctionLocally(self, result) : nullptr;
        if (function) {
            self = function;
        }

        std::vector<Function*>& overloads = self->overloads;
        bool argsEqual = checkStackType(result, self->args, withIntPromotion);

        if (checkOverloads && overloads.size() && !argsEqual) {
            for (bool b : bools) {
                for (Function* overload : overloads) {
                    if (overload->isMethod) continue;

                    bool argsEqual = checkStackType(result, overload->args, b);
                    if (argsEqual) {
                        functionCall(overload, fp, result, warns, errors, body, i, b, hasToCallStatic, false);
                        return;
                    }
                }
            }
        }

        if (self->has_operator) {
            size_t sym = self->has_operator;

            if (!hasToCallStatic && !checkStackType(result, self->args, withIntPromotion) && hasMethod(result, self->name, typeStackTop)) {
                Method* method = getMethodByName(result, self->name, typeStackTop);
                methodCall(method, fp, result, warns, errors, body, i);
                return;
            }

            std::string op = self->modifiers[sym];

            if (typeStack.size() < (1 + (op != "lnot" && op != "not"))) {
                transpilerError("Cannot deduce type for operation '" + opToString(op) +  "'", i);
                errors.push_back(err);
                return;
            }

            if (op != "lnot" && op != "not") {
                typeStack.pop();
                typeStack.pop();
                append("_stack.sp -= 2;\n");
            } else {
                typeStack.pop();
                append("_stack.sp -= 1;\n");
            }
            std::string args = generateArgumentsForFunction(result, self);

            if (Main.options.debugBuild) {
                append("_scl_assert(_scl_stack_size() >= %d, \"_scl_%s() failed: Not enough data on the stack!\");\n", (1 + (op != "lnot" && op != "not")), op.c_str());
            }
            append("*(%s*) (_stack.sp++) = _scl_%s(%s);\n", sclTypeToCType(result, self->return_type).c_str(), op.c_str(), args.c_str());

            typeStack.push(self->return_type);

            return;
        }

        if (!hasToCallStatic && opFunc(self->name) && hasMethod(result, self->name, typeStackTop)) {
        makeMethodCallInstead:
            Method* method = getMethodByName(result, self->name, typeStackTop);
            methodCall(method, fp, result, warns, errors, body, i);
            return;
        }

        argsEqual = checkStackType(result, self->args);
        if (!argsEqual && argsContainsIntType(self->args)) {
            argsEqual = checkStackType(result, self->args, true);
        }
        if (!argsEqual) {
            if (hasMethod(result, self->name, typeStackTop)) {
                goto makeMethodCallInstead;
            }

            {
                transpilerError("Arguments for function '" + sclFunctionNameToFriendlyString(self) + "' do not equal inferred stack", i);
                errors.push_back(err);
            }

            std::string overloadedMatches = "";
            if (overloads.size()) {
                for (auto&& overload : overloads) {
                    if (overload == self) continue;
                    overloadedMatches += ", or [ " + Color::BLUE + argVectorToString(overload->args) + Color::RESET + " ]";
                }
            }

            transpilerError("Expected: [ " + Color::BLUE + argVectorToString(self->args) + Color::RESET + " ]" + overloadedMatches + ", but got: [ " + Color::RED + stackSliceToString(self->args.size()) + Color::RESET + " ]", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }

        if (self->isExternC && !hasImplementation(result, self)) {
            append("*(_stack.tp++) = \"<extern %s>\";\n", sclFunctionNameToFriendlyString(self).c_str());
        }
        
        if (self->args.size() > 0) {
            append("_stack.sp -= (%zu);\n", self->args.size());
        }
        for (size_t m = 0; m < self->args.size(); m++) {
            typePop;
        }
        append("// invokestatic %s\n", self->finalName().c_str());
        if (self->name == "throw" || self->name == "builtinUnreachable") {
            if (currentFunction->has_restrict) {
                if (currentFunction->isMethod) {
                    append("Function_Process$unlock((scl_SclObject) Var_self);\n");
                } else {
                    append("Function_Process$unlock(function_lock$%s);\n", currentFunction->finalName().c_str());
                }
            }
        }
        if (removeTypeModifiers(self->return_type) != "none" && removeTypeModifiers(self->return_type) != "nothing") {
            if (removeTypeModifiers(self->return_type) == "float") {
                append("(_stack.sp++)->f = Function_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
            } else {
                append("(_stack.sp++)->i = (scl_int) Function_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
            }
            typeStack.push(self->return_type);
        } else {
            append("Function_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
        }
        if (self->isExternC && !hasImplementation(result, self)) {
            append("--_stack.tp;\n");
        }
    }

#define handler(_tok) extern "C" void handle ## _tok (std::vector<Token>& body, Function* function, std::vector<FPResult>& errors, std::vector<FPResult>& warns, FILE* fp, TPResult& result)
#define handle(_tok) handle ## _tok (body, function, errors, warns, fp, result)
#define handlerRef(_tok) handle ## _tok
#define handleRef(ref) (ref)(body, function, errors, warns, fp, result)
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
    handler(Fi);
    handler(Unless);
    handler(Else);
    handler(Elif);
    handler(Elunless);
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

    bool handleOverriddenOperator(TPResult& result, FILE* fp, int scopeDepth, std::string op, std::string type);

    std::string makePath(TPResult& result, Variable v, bool topLevelDeref, std::vector<Token>& body, size_t& i, std::vector<FPResult>& errors, std::string prefix, std::string* currentType, bool doesWriteAfter = true);

    int lambdaCount = 0;

    handler(Lambda) {
        noUnused;
        Function* f;
        if (function->isMethod) {
            f = new Function("$lambda$" + std::to_string(lambdaCount++) + "$" + function->member_type + "$$" + function->finalName(), body[i]);
        } else {
            f = new Function("$lambda$" + std::to_string(lambdaCount++) + "$" + function->finalName(), body[i]);
        }
        f->addModifier("<lambda>");
        f->addModifier(generateSymbolForFunction(function).substr(44));
        f->return_type = "none";
        safeInc();
        if (body[i].type == tok_paren_open) {
            safeInc();
            while (i < body.size() && body[i].type != tok_paren_close) {
                if (body[i].type == tok_identifier || body[i].type == tok_column) {
                    std::string name = body[i].value;
                    std::string type = "any";
                    if (body[i].type == tok_column) {
                        name = "";
                    } else {
                        safeInc();
                    }
                    if (body[i].type == tok_column) {
                        safeInc();
                        FPResult r = parseType(body, &i, getTemplates(result, function));
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        if (type == "none" || type == "nothing") {
                            transpilerError("Type 'none' is only valid for function return types.", i);
                            errors.push_back(err);
                            continue;
                        }
                    } else {
                        transpilerError("A type is required", i);
                        errors.push_back(err);
                        safeInc();
                        continue;
                    }
                    f->addArgument(Variable(name, type));
                } else {
                    transpilerError("Expected identifier for argument name, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    safeInc();
                    continue;
                }
                safeInc();
                if (body[i].type == tok_comma || body[i].type == tok_paren_close) {
                    if (body[i].type == tok_comma) {
                        safeInc();
                    }
                    continue;
                }
                transpilerError("Expected ',' or ')', but got '" + body[i].value + "'", i);
                errors.push_back(err);
                continue;
            }
            safeInc();
        }
        if (body[i].type == tok_column) {
            safeInc();
            FPResult r = parseType(body, &i, getTemplates(result, function));
            if (!r.success) {
                errors.push_back(r);
                return;
            }
            std::string type = r.value;
            f->return_type = type;
            safeInc();
        }
        int lambdaDepth = 0;
        while (true) {
            if (body[i].type == tok_end && lambdaDepth == 0) {
                break;
            }
            if (body[i].type == tok_identifier && body[i].value == "lambda") {
                if (((ssize_t) i) - 1 < 0 || body[i - 1].type != tok_column) {
                    lambdaDepth++;
                }
            }
            if (body[i].type == tok_end) {
                lambdaDepth--;
            }
            f->addToken(body[i]);
            safeInc();
        }

        std::string arguments = "";
        if (f->isMethod) {
            arguments = sclTypeToCType(result, f->member_type) + " Var_self";
            for (size_t i = 0; i < f->args.size() - 1; i++) {
                std::string type = sclTypeToCType(result, f->args[i].type);
                arguments += ", " + type;
                if (type == "varargs" || type == "...") continue;
                if (f->args[i].name.size())
                    arguments += " Var_" + f->args[i].name;
            }
        } else {
            if (f->args.size() > 0) {
                for (size_t i = 0; i < f->args.size(); i++) {
                    std::string type = sclTypeToCType(result, f->args[i].type);
                    if (i) {
                        arguments += ", ";
                    }
                    arguments += type;
                    if (type == "varargs" || type == "...") continue;
                    if (f->args[i].name.size())
                        arguments += " Var_" + f->args[i].name;
                }
            }
        }

        std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;

        append("%s Function_$lambda%d$%s(%s) __asm(%s);\n", sclTypeToCType(result, f->return_type).c_str(), lambdaCount - 1, function->finalName().c_str(), arguments.c_str(), generateSymbolForFunction(f).c_str());
        append("(_stack.sp++)->v = Function_$lambda%d$%s;\n", lambdaCount - 1, function->finalName().c_str());
        typeStack.push(lambdaType);
        result.functions.push_back(f);
        functionsByFile[function->name_token.file].push_back(f);
    }

    handler(ReturnOnNil) {
        noUnused;
        if (typeCanBeNil(typeStackTop)) {
            std::string type = typeStackTop;
            typePop;
            typeStack.push(type.substr(0, type.size() - 1));
        }
        if (function->return_type == "nothing") {
            transpilerError("Returning from a function with return type 'nothing' is not allowed.", i);
            errors.push_back(err);
            return;
        }
        if (!typeCanBeNil(function->return_type) && function->return_type != "none") {
            transpilerError("Return-if-nil operator '?' behaves like assert-not-nil operator '!!' in not-nil returning function.", i);
            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
            else errors.push_back(err);
            append("_scl_assert((_stack.sp - 1)->i, \"Not nil assertion failed!\");\n");
        } else {
            append("if ((_stack.sp - 1)->i == 0) {\n");
            scopeDepth++;
            if (function->has_restrict) {
                if (function->isMethod) {
                    append("Function_Process$unlock((scl_SclObject) Var_self);\n");
                } else {
                    append("Function_Process$unlock(function_lock$%s);\n", function->finalName().c_str());
                }
            }
            append("_stack.tp--\n");
            append("_stack.sp = __current_base_ptr;\n");
            if (function->return_type == "none")
                append("return;\n");
            else {
                append("return 0;\n");
            }
            scopeDepth--;
            append("}\n");
        }
    }

    bool structExtends(TPResult& result, Struct& s, std::string name) {
        if (s.name == name) return true;

        Struct super = getStructByName(result, s.super);
        Struct oldSuper = s;
        while (super.name.size()) {
            if (super.name == name)
                return true;
            oldSuper = super;
            super = getStructByName(result, super.super);
        }
        return false;
    }

    handler(Catch) {
        noUnused;
        std::string ex = "Exception";
        safeInc();
        if (body[i].value == "typeof") {
            safeInc();
            varScopePop();
            scopeDepth--;
            if (wasTry()) {
                popTry();
                append("  _scl_exception_drop();\n");
                append("} else if (_scl_is_instance_of(*(_stack.ex), 0x%xU)) {\n", id((char*) "Error"));
                append("  Function_throw(*(_stack.ex));\n");
            } else if (wasCatch()) {
                popCatch();
            }

            varScopePush();
            pushCatch();
            Struct s = getStructByName(result, body[i].value);
            if (s == Struct::Null) {
                transpilerError("Trying to catch unknown Exception of type '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }

            if (structExtends(result, s, "Error")) {
                transpilerError("Cannot catch Exceptions that extend 'Error'", i);
                errors.push_back(err);
                return;
            }
            append("} else if (_scl_is_instance_of(*(_stack.ex), 0x%xU)) {\n", id((char*) body[i].value.c_str()));
        } else {
            i--;
            {
                transpilerError("Generic Exception caught here:", i);
                errors.push_back(err);
            }
            transpilerError("Add 'typeof Exception' here to fix this:\\insertText; typeof Exception;" + std::to_string(err.line) + ":" + std::to_string(err.column + body[i].value.size()), i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        std::string exName = "exception";
        if (body.size() > i + 1 && body[i + 1].value == "as") {
            safeInc();
            safeInc();
            exName = body[i].value;
        }
        scopeDepth++;
        varScopeTop().push_back(Variable(exName, ex));
        append("scl_%s Var_%s = (scl_%s) *(_stack.ex);\n", ex.c_str(), exName.c_str(), ex.c_str());
    }

    handler(Pragma) {
        noUnused;
        safeInc();
        if (body[i].value == "print") {
            safeInc();
            std::cout << body[i].file
                      << ":"
                      << body[i].line
                      << ":"
                      << body[i].column
                      << ": "
                      << body[i].value
                      << std::endl;
        } else if (body[i].value == "stackalloc") {
            safeInc();
            std::string length = body[i].value;
            long long len = std::stoll(length);
            if (len <= 0) {
                transpilerError("Can only stackalloc a positive amount of stack frames, but got '" + length + "'", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            scopeDepth++;
            append("scl_any* _stackalloc_ptr = (scl_any*) _stack.sp;\n");
            append("_scl_stack_resize_fit(%s);\n", length.c_str());
            append("_scl_add_stackallocation(_stackalloc_ptr, %s);\n", length.c_str());
            append("(_stack.sp++)->v = _stackalloc_ptr;\n");
            scopeDepth--;
            append("}\n");
            typeStack.push("[any]");
        } else if (body[i].value == "unset") {
            safeInc();

            std::string var = body[i].value;
            if (!hasVar(var)) {
                transpilerError("Trying to unset unknown variable '" + var + "'", i);
                errors.push_back(err);
                return;
            }
            Variable v = getVar(var);
            if (v.isConst) {
                transpilerError("Trying to unset constant variable '" + var + "'", i);
                errors.push_back(err);
                return;
            }
            if (v.type.size() < 2 || v.type.front() != '[' || v.type.back() != ']') {
                transpilerError("Trying to unset non-reference variable '" + var + "'", i);
                errors.push_back(err);
                return;
            }

            if (removeTypeModifiers(v.type) == "float") {
                append("*Var_%s = 0.0;\n", var.c_str());
            } else {
                append("*Var_%s = NULL;\n", var.c_str());
            }
        }
    }

    void createVariadicCall(Function* f, FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<Token> body, size_t& i) {
        safeInc();
        if (body[i].value != "!") {
            transpilerError("Expected '!' for variadic call, but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        safeInc();
        if (body[i].type != tok_number) {
            transpilerError("Amount of variadic arguments needs to be specified for variadic function call", i);
            errors.push_back(err);
            return;
        }

        append("{\n");
        scopeDepth++;
        size_t amountOfVarargs = std::stoi(body[i].value);
        
        if (f->isExternC && !hasImplementation(result, f)) {
            append("*(_stack.tp++) = \"<extern %s>\";\n", sclFunctionNameToFriendlyString(f).c_str());
        }

        append("_stack.sp -= (%zu);\n", amountOfVarargs);

        for (size_t i = 0; i < amountOfVarargs; i++) {
            append("scl_any vararg%zu = _scl_positive_offset(%zu)->v;\n", i, i);
        }

        std::string args = generateArgumentsForFunction(result, f);

        for (size_t i = 0; i < amountOfVarargs; i++) {
            args += ", vararg" + std::to_string(i);
        }
        // args += ", NULL";

        if (f->varArgsParam().name.size()) {
            append("(_stack.sp++)->i = %zu;\n", amountOfVarargs);
            typeStack.push("int");
        }

        if (f->args.size() > 1)
            append("_stack.sp -= (%zu);\n", f->args.size() - 1);

        for (size_t i = 0; i < (amountOfVarargs + f->args.size()); i++) {
            typePop;
        }

        if (f->return_type == "none" || f->return_type == "nothing") {
            append("Function_%s(%s);\n", f->name.c_str(), args.c_str());
        } else {
            if (f->return_type == "float") {
                append("(_stack.sp++)->f = Function_%s(%s);\n", f->name.c_str(), args.c_str());
            } else {
                append("(_stack.sp++)->v = (scl_any) Function_%s(%s);\n", f->name.c_str(), args.c_str());
            }
        }
        scopeDepth--;
        append("}\n");
    }

    std::pair<std::string, std::string> findNth(std::map<std::string, std::string> val, size_t n) {
        size_t i = 0;
        for (auto&& member : val) {
            if (i == n) {
                return member;
            }
            i++;
        }
        return std::pair<std::string, std::string>("", "");
    }

    std::string primitiveToReference(std::string type) {
        type = removeTypeModifiers(type);
        type = notNilTypeOf(type);
        if (!isPrimitiveType(type)) {
            return type;
        }
        if (isPrimitiveIntegerType(type) || type == "bool") {
            return "Int";
        }
        if (type == "float") {
            return "Float";
        }
        return "Any";
    }

    std::vector<std::string> vecWithout(std::vector<std::string> vec, std::string elem) {
        std::vector<std::string> newVec;
        for (auto&& member : vec) {
            if (member != elem) {
                newVec.push_back(member);
            }
        }
        return newVec;
    };

#define LOAD_PATH(path, type)                                       \
    if (removeTypeModifiers(type) == "float")                       \
    {                                                               \
        append("(_stack.sp++)->f = %s;\n", path.c_str());           \
    }                                                               \
    else                                                            \
    {                                                               \
        append("(_stack.sp++)->i = (scl_int) %s;\n", path.c_str()); \
    }                                                               \
    typeStack.push(type);

    handler(Identifier) {
        noUnused;
    #ifdef __APPLE__
    #pragma region Identifier functions
    #endif
        if (body[i].value == "drop") {
            append("--_stack.sp;\n");
            typePop;
        } else if (body[i].value == "dup") {
            append("(_stack.sp++)->v = (_stack.sp - 1)->v;\n");
            typeStack.push(typeStackTop);
        } else if (body[i].value == "swap") {
            append("_scl_swap();\n");
            std::string b = typeStackTop;
            typePop;
            std::string a = typeStackTop;
            typePop;
            typeStack.push(b);
            typeStack.push(a);
        } else if (body[i].value == "over") {
            append("_scl_over();\n");
            std::string c = typeStackTop;
            typePop;
            std::string b = typeStackTop;
            typePop;
            std::string a = typeStackTop;
            typePop;
            typeStack.push(c);
            typeStack.push(b);
            typeStack.push(a);
        } else if (body[i].value == "sdup2") {
            append("_scl_sdup2();\n");
            std::string b = typeStackTop;
            typePop;
            std::string a = typeStackTop;
            typePop;
            typeStack.push(a);
            typeStack.push(b);
            typeStack.push(a);
        } else if (body[i].value == "swap2") {
            append("_scl_swap2();\n");
            std::string c = typeStackTop;
            typePop;
            std::string b = typeStackTop;
            typePop;
            std::string a = typeStackTop;
            typePop;
            typeStack.push(b);
            typeStack.push(a);
            typeStack.push(c);
        } else if (body[i].value == "clearstack") {
            append("_stack.sp = _stack.bp;\n");
            for (long t = typeStack.size() - 1; t >= 0; t--) {
                typePop;
            }
        } else if (body[i].value == "!!") {
            if (typeCanBeNil(typeStackTop)) {
                std::string type = typeStackTop;
                typePop;
                typeStack.push(type.substr(0, type.size() - 1));
                append("_scl_assert((_stack.sp - 1)->i, \"Not nil assertion failed!\");\n");
            } else {
                transpilerError("Unnecessary assert-not-nil operator '!!' on not-nil type '" + typeStackTop + "'", i);
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
                append("_scl_assert((_stack.sp - 1)->i, \"Not nil assertion failed! If you see this, something has gone very wrong!\");\n");
            }
        } else if (body[i].value == "?") {
            handle(ReturnOnNil);
        } else if (body[i].value == "++") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "++", typeStackTop)) return;
            append("(_stack.sp - 1)->i++;\n");
            if (typeStack.size() == 0)
                typeStack.push("int");
        } else if (body[i].value == "--") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "--", typeStackTop)) return;
            append("--(_stack.sp - 1)->i;\n");
            if (typeStack.size() == 0)
                typeStack.push("int");
        } else if (body[i].value == "exit") {
            append("exit((--_stack.sp)->i);\n");
            typePop;
        } else if (body[i].value == "abort") {
            append("abort();\n");
        } else if (body[i].value == "typeof") {
            safeInc();
            auto templates = currentStruct.templates;
            if (hasVar(body[i].value)) {
                const Struct& s = getStructByName(result, getVar(body[i].value).type);
                if (s != Struct::Null && !s.isStatic()) {
                    append("(_stack.sp++)->s = _scl_create_string(Var_%s->$statics->type_name);\n", body[i].value.c_str());
                } else {
                    append("(_stack.sp++)->s = _scl_create_string(_scl_find_index_of_struct(*(scl_any*) &Var_%s) != -1 ? ((scl_SclObject) Var_%s)->$statics->type_name : \"%s\");\n", getVar(body[i].value).name.c_str(), getVar(body[i].value).name.c_str(), getVar(body[i].value).type.c_str());
                }
            } else if (hasFunction(result, body[i].value)) {
                Function* f = getFunctionByName(result, body[i].value);
                std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
                append("(_stack.sp++)->s = _scl_create_string(\"%s\");\n", lambdaType.c_str());
            } else if (function->isMethod && templates.find(body[i].value) != templates.end()) {
                append("(_stack.sp++)->s = _scl_create_string(Var_self->$template_argname_%s);\n", body[i].value.c_str());
            } else if (body[i].type == tok_paren_open) {
                append("{\n");
                scopeDepth++;
                size_t stack_start = typeStack.size();
                handle(ParenOpen);
                if (typeStack.size() <= stack_start) {
                    transpilerError("Expected expression evaluating to a value after 'typeof'", i);
                    errors.push_back(err);
                }
                if (getStructByName(result, typeStackTop) != Struct::Null) {
                    append("(_stack.sp - 1)->s = _scl_create_string((_stack.sp - 1)->o->$statics->type_name);\n");
                } else {
                    append("(_stack.sp - 1)->s = _scl_create_string(_scl_find_index_of_struct(*(scl_any*) (_stack.sp - 1)) != -1 ? (_stack.sp - 1)->o->$statics->type_name : \"%s\");\n", typeStackTop.c_str());
                }
                scopeDepth--;
                append("}\n");
                typePop;
            } else {
                FPResult res = parseType(body, &i);
                if (!res.success) {
                    errors.push_back(res);
                    return;
                }
                append("(_stack.sp++)->s = _scl_create_string(\"%s\");\n", res.value.c_str());
            }
            typeStack.push("str");
        } else if (body[i].value == "nameof") {
            safeInc();
            if (hasVar(body[i].value)) {
                append("(_stack.sp++)->s = _scl_create_string(\"%s\");\n", body[i].value.c_str());
                typeStack.push("str");
            } else {
                transpilerError("Unknown Variable: '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (body[i].value == "sizeof") {
            safeInc();
            auto templates = getTemplates(result, function);
            if (body[i].value == "int") {
                append("(_stack.sp++)->i = sizeof(scl_int);\n");
            } else if (body[i].value == "float") {
                append("(_stack.sp++)->i = sizeof(scl_float);\n");
            } else if (body[i].value == "str") {
                append("(_stack.sp++)->i = sizeof(scl_str);\n");
            } else if (body[i].value == "any") {
                append("(_stack.sp++)->i = sizeof(scl_any);\n");
            } else if (body[i].value == "none" || body[i].value == "nothing") {
                append("(_stack.sp++)->i = 0;\n");
            } else if (hasVar(body[i].value)) {
                append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, getVar(body[i].value).type).c_str());
            } else if (getStructByName(result, body[i].value) != Struct::Null) {
                append("(_stack.sp++)->i = sizeof(struct Struct_%s);\n", body[i].value.c_str());
            } else if (hasTypealias(result, body[i].value)) {
                append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, body[i].value).c_str());
            } else if (hasLayout(result, body[i].value)) {
                append("(_stack.sp++)->i = sizeof(struct Layout_%s);\n", body[i].value.c_str());
            } else if (templates.find(body[i].value) != templates.end()) {
                std::string replaceWith = templates[body[i].value];
                if (getStructByName(result, replaceWith) != Struct::Null)
                    append("(_stack.sp++)->i = sizeof(struct Struct_%s);\n", replaceWith.c_str());
                else if (hasTypealias(result, replaceWith))
                    append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, replaceWith).c_str());
                else if (hasLayout(result, replaceWith))
                    append("(_stack.sp++)->i = sizeof(struct Layout_%s);\n", replaceWith.c_str());
                else
                    append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, replaceWith).c_str());
            } else {
                FPResult type = parseType(body, &i, getTemplates(result, function));
                if (!type.success) {
                    transpilerError("Unknown Type: '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, type.value).c_str());
            }
            typeStack.push("int");
        } else if (body[i].value == "try") {
            append("_scl_exception_push();\n");
            append("if (setjmp(*(_stack.jmp - 1)) != 666) {\n");
            scopeDepth++;
            append("*(_stack.et - 1) = _stack.tp;\n");
            varScopePush();
            pushTry();
        } else if (body[i].value == "catch") {
            handle(Catch);
        } else if (body[i].value == "lambda") {
            handle(Lambda);
        } else if (body[i].value == "unsafe") {
            append("{\n");
            scopeDepth++;
            append("scl_int8* __prev = _stack.tp - 1;\n");
            append("*(_stack.tp - 1) = \"unsafe %s\";\n", sclFunctionNameToFriendlyString(function).c_str());
            isInUnsafe++;
            safeInc();
            while (body[i].type != tok_end) {
                handle(Token);
                safeInc();
            }
            isInUnsafe--;
            append("*(_stack.tp - 1) = __prev;\n");
            scopeDepth--;
            append("}\n");
        } else if (body[i].value == "pragma!") {
            handle(Pragma);
        } else if (body[i].value == "assert") {
            const Token& assertToken = body[i];
            safeInc();
            if (body[i].type != tok_paren_open) {
                transpilerError("Expected '(' after 'assert', but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            varScopePush();
            scopeDepth++;
            size_t stack_start = typeStack.size();
            handle(ParenOpen);
            if (stack_start >= typeStack.size()) {
                transpilerError("Expected expression returning a value after 'assert'", i);
                errors.push_back(err);
                return;
            }
            if (i + 1 < body.size() && body[i + 1].type == tok_else) {
                safeInc();
                safeInc();
                if (body[i].type != tok_paren_open) {
                    transpilerError("Expected '(' after 'else', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                append("if (!(_stack.sp - 1)->i) {\n");
                handle(ParenOpen);
                append("}\n");
            } else {
                append("_scl_assert((_stack.sp - 1)->i, \"Assertion at %s:%d:%d failed!\");\n", assertToken.file.c_str(), assertToken.line, assertToken.column);
            }
            varScopePop();
            scopeDepth--;
            append("}\n");
    #ifdef __APPLE__
    #pragma endregion
    #endif
        } else if (hasEnum(result, body[i].value)) {
            Enum e = getEnumByName(result, body[i].value);
            safeInc();
            if (body[i].type != tok_double_column) {
                transpilerError("Expected '::', but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            safeInc();
            if (e.indexOf(body[i].value) == Enum::npos) {
                transpilerError("Unknown member of enum '" + e.name + "': '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            append("(_stack.sp++)->i = %zuUL;\n", e.indexOf(body[i].value));
            typeStack.push("int");
        } else if (hasFunction(result, body[i].value)) {
            Function* f = getFunctionByName(result, body[i].value);
            if (f->isCVarArgs()) {
                createVariadicCall(f, fp, result, errors, body, i);
                return;
            }
            if (f->isMethod) {
                transpilerError("'" + f->name + "' is a method, not a function.", i);
                errors.push_back(err);
                return;
            }
            functionCall(f, fp, result, warns, errors, body, i);
        } else if (hasFunction(result, function->member_type + "$" + body[i].value)) {
            Function* f = getFunctionByName(result, function->member_type + "$" + body[i].value);
            if (f->isCVarArgs()) {
                createVariadicCall(f, fp, result, errors, body, i);
                return;
            }
            if (f->isMethod) {
                transpilerError("'" + f->name + "' is a method, not a function.", i);
                errors.push_back(err);
                return;
            }
            functionCall(f, fp, result, warns, errors, body, i);
        } else if (hasContainer(result, body[i].value)) {
            std::string containerName = body[i].value;
            safeInc();
            if (body[i].type != tok_dot) {
                transpilerError("Expected '.' to access container contents, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            safeInc();
            std::string memberName = body[i].value;
            Container container = getContainerByName(result, containerName);
            if (!container.hasMember(memberName)) {
                transpilerError("Unknown container member: '" + memberName + "'", i);
                errors.push_back(err);
                return;
            }
            Variable v = container.getMember(memberName);
            std::string lastType = "";
            std::string path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "Container_" + containerName + ".", &lastType, /* doesWriteAfter */ false);
            
            LOAD_PATH(path, lastType);
        } else if (getStructByName(result, body[i].value) != Struct::Null) {
            Struct s = getStructByName(result, body[i].value);
            std::map<std::string, std::string> templates;
            if (body[i + 1].type == tok_identifier && body[i + 1].value == "<") {
                auto nthTemplate = [&](Struct& s, size_t n) -> std::pair<std::string, std::string> {
                    size_t i = 0;
                    for (auto& template_ : s.templates) {
                        if (i == n) {
                            return template_;
                        }
                        safeInc();
                    }
                    return std::pair<std::string, std::string>();
                };
                safeIncN(2);
                while (body[i].value != ">") {
                    if (body[i].type == tok_paren_open || (body[i].type == tok_identifier && body[i].value == "typeof")) {
                        int begin = i;
                        handle(Token);
                        if (removeTypeModifiers(typeStackTop) != "str") {
                            transpilerError("Expected 'str', but got '" + typeStackTop + "'", begin);
                            errors.push_back(err);
                            return;
                        }
                        auto nth = nthTemplate(s, templates.size());
                        append("scl_str $template%s = (--_stack.sp)->s;\n", nth.first.c_str());
                        typePop;
                        templates[nth.first] = "$template" + nth.first;
                    } else {
                        FPResult type = parseType(body, &i, currentStruct.templates);
                        if (!type.success) {
                            errors.push_back(type);
                            return;
                        }
                        auto t = nthTemplate(s, templates.size());
                        type.value = removeTypeModifiers(type.value);
                        if (type.value.size() > 2 && type.value.front() == '[' && type.value.back() == ']') {
                            transpilerError("Expected scalar type, but got '" + type.value + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (t.second != "any") {
                            Struct f = Struct::Null;
                            if (isPrimitiveIntegerType(t.second)) {
                                if (!isPrimitiveIntegerType(type.value)) {
                                    transpilerError("Expected integer type, but got '" + type.value + "'", i);
                                    errors.push_back(err);
                                    return;
                                }
                            } else if ((f = getStructByName(result, t.second)) != Struct::Null) {
                                Struct g = getStructByName(result, type.value);
                                if (g == Struct::Null) {
                                    transpilerError("Expected struct type, but got '" + type.value + "'", i);
                                    errors.push_back(err);
                                    return;
                                }
                                if (!structExtends(result, g, f.name)) {
                                    transpilerError("Struct '" + g.name + "' is not convertible to '" + f.name + "'", i);
                                    errors.push_back(err);
                                    return;
                                }
                            }
                        }
                        templates[t.first] = type.value;
                    }
                    safeInc();
                    if (body[i].value == ",") {
                        safeInc();
                    }
                }
            }
            if (body[i + 1].type == tok_double_column) {
                safeInc();
                safeInc();
                Method* initMethod = getMethodByName(result, "init", s.name);
                bool hasInitMethod = initMethod != nullptr;
                if (body[i].value == "new" || body[i].value == "default") {
                    if (hasInitMethod && initMethod->has_private) {
                        transpilerError("Direct instantiation of struct '" + s.name + "' is not allowed", i);
                        errors.push_back(err);
                        return;
                    }
                }
                if (body[i].value == "new" && !s.isStatic()) {
                    append("{\n");
                    scopeDepth++;
                    append("(_stack.sp++)->v = ALLOC(%s);\n", s.name.c_str());
                    append("_scl_struct_allocation_failure((_stack.sp - 1)->i, \"%s\");\n", s.name.c_str());
                    append("%s tmp = (_stack.sp - 1)->v;\n", sclTypeToCType(result, s.name).c_str());
                    for (auto&& t : templates) {
                        if (t.second.front() == '$') {
                            append("tmp->$template_arg_%s = %s->hash;\n", t.first.c_str(), t.second.c_str());
                            append("tmp->$template_argname_%s = %s->data;\n", t.first.c_str(), t.second.c_str());
                        } else {
                            append("tmp->$template_arg_%s = 0x%xU;\n", t.first.c_str(), id((char*) t.second.c_str()));
                            append("tmp->$template_argname_%s = \"%s\";\n", t.first.c_str(), t.second.c_str());
                        }
                    }
                    
                    typeStack.push(s.name);
                    if (hasInitMethod) {
                        methodCall(initMethod, fp, result, warns, errors, body, i);
                        append("(_stack.sp++)->v = tmp;\n");
                        typeStack.push(s.name);
                    }
                    scopeDepth--;
                    append("}\n");
                } else if (body[i].value == "default" && !s.isStatic()) {
                    append("(_stack.sp++)->v = ALLOC(%s);\n", s.name.c_str());
                    append("_scl_struct_allocation_failure((_stack.sp - 1)->i, \"%s\");\n", s.name.c_str());
                    if (templates.size()) {
                        append("{\n");
                        scopeDepth++;
                        append("%s tmp = (_stack.sp - 1)->v;\n", sclTypeToCType(result, s.name).c_str());
                    }
                    for (auto&& t : templates) {
                        if (t.second.front() == '$') {
                            append("tmp->$template_arg_%s = %s->hash;\n", t.first.c_str(), t.second.c_str());
                            append("tmp->$template_argname_%s = %s->data;\n", t.first.c_str(), t.second.c_str());
                        } else {
                            append("tmp->$template_arg_%s = %d;\n", t.first.c_str(), id((char*) t.second.c_str()));
                            append("tmp->$template_argname_%s = \"%s\";\n", t.first.c_str(), t.second.c_str());
                        }
                    }
                    if (templates.size()) {
                        scopeDepth--;
                        append("}\n");
                    }
                    typeStack.push(s.name);
                } else {
                    if (hasFunction(result, s.name + "$" + body[i].value)) {
                        Function* f = getFunctionByName(result, s.name + "$" + body[i].value);
                        if (f->isCVarArgs()) {
                            createVariadicCall(f, fp, result, errors, body, i);
                            return;
                        }
                        if (f->isMethod) {
                            transpilerError("'" + f->name + "' is not static", i);
                            errors.push_back(err);
                            return;
                        }
                        if (f->isPrivate && function->belongsToType(s.name)) {
                            if (f->member_type != function->member_type) {
                                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                                errors.push_back(err);
                                return;
                            }
                        }
                        functionCall(f, fp, result, warns, errors, body, i);
                    } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                        Variable v = getVar(s.name + "$" + body[i].value);
                        
                        std::string lastType = "";
                        std::string path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "", &lastType, /* doesWriteAfter */ false);
                        
                        LOAD_PATH(path, lastType);
                    } else {
                        transpilerError("Unknown static member of struct '" + s.name + "'", i);
                        errors.push_back(err);
                    }
                }
            } else if (body[i + 1].type == tok_curly_open) {
                safeInc();
                append("{\n");
                scopeDepth++;
                size_t begin = i - 1;
                append("scl_%s tmp = ALLOC(%s);\n", s.name.c_str(), s.name.c_str());
                append("_scl_struct_allocation_failure(*(scl_int*) &tmp, \"%s\");\n", s.name.c_str());
                for (auto&& t : templates) {
                    if (t.second.front() == '$') {
                        append("tmp->$template_arg_%s = %s->hash;\n", t.first.c_str(), t.second.c_str());
                        append("tmp->$template_argname_%s = %s->data;\n", t.first.c_str(), t.second.c_str());
                    } else {
                        append("tmp->$template_arg_%s = %d;\n", t.first.c_str(), id((char*) t.second.c_str()));
                        append("tmp->$template_argname_%s = \"%s\";\n", t.first.c_str(), t.second.c_str());
                    }
                }
                append("_scl_frame_t* stack_start = _stack.sp;\n");
                safeInc();
                size_t count = 0;
                varScopePush();
                if (function->isMethod) {
                    Variable v("super", getVar("self").type);
                    varScopeTop().push_back(v);
                    append("scl_%s Var_super = Var_self;\n", removeTypeModifiers(getVar("self").type).c_str());
                }
                append("scl_%s Var_self = tmp;\n", s.name.c_str());
                varScopeTop().push_back(Variable("self", "mut " + s.name));
                std::vector<std::string> missedMembers;

                size_t membersToInitialize = 0;
                for (auto&& v : s.members) {
                    if (v.name.front() != '$') {
                        missedMembers.push_back(v.name);
                        membersToInitialize++;
                    }
                }

                while (body[i].type != tok_curly_close) {
                    append("{\n");
                    scopeDepth++;
                    handle(Token);
                    safeInc();
                    if (body[i].type != tok_store) {
                        transpilerError("Expected store, but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    safeInc();
                    if (body[i].type != tok_identifier) {
                        transpilerError("'" + body[i].value + "' is not an identifier", i);
                        errors.push_back(err);
                        return;
                    }

                    std::string lastType = typeStackTop;

                    missedMembers = vecWithout(missedMembers, body[i].value);
                    const Variable& v = s.getMember(body[i].value);

                    if (!typesCompatible(result, lastType, v.type, true)) {
                        transpilerError("Incompatible types: '" + v.type + "' and '" + lastType + "'", i);
                        errors.push_back(err);
                        return;
                    }

                    Method* mutator = attributeMutator(result, s.name, body[i].value);
                    
                    if (mutator) {
                        if (removeTypeModifiers(lastType) == "float") {
                            append("Method_%s$%s(tmp, (--_stack.sp)->f);\n", mutator->member_type.c_str(), mutator->finalName().c_str());
                        } else {
                            append("Method_%s$%s(tmp, (%s) (--_stack.sp)->i);\n", mutator->member_type.c_str(), mutator->finalName().c_str(), sclTypeToCType(result, lastType).c_str());
                        }
                    } else {
                        if (removeTypeModifiers(lastType) == "float") {
                            append("tmp->%s = (--_stack.sp)->f;\n", body[i].value.c_str());
                        } else {
                            append("tmp->%s = (%s) (--_stack.sp)->i;\n", body[i].value.c_str(), sclTypeToCType(result, lastType).c_str());
                        }
                    }
                    typePop;
                    scopeDepth--;
                    append("}\n");
                    append("_stack.sp = stack_start;\n");
                    safeInc();
                    count++;
                }
                varScopePop();
                append("(_stack.sp++)->v = tmp;\n");
                scopeDepth--;
                append("}\n");
                typeStack.push(s.name);
                if (count < membersToInitialize) {
                    std::string missed = "{ ";
                    for (size_t i = 0; i < missedMembers.size(); i++) {
                        if (i) {
                            missed += ", ";
                        }
                        missed += missedMembers[i];
                    }
                    missed += " }";
                    transpilerError("Not every member was initialized! Missed: " + missed, begin);
                    errors.push_back(err);
                    return;
                }
            }
        } else if (hasVar(body[i].value)) {
        normalVar:
            Variable v = getVar(body[i].value);
            std::string lastType = "";
            std::string path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "", &lastType, /* doesWriteAfter */ false);

            LOAD_PATH(path, lastType);
        } else if (body[i].value == "field" && (function->has_setter || function->has_getter)) {
            Struct s = getStructByName(result, function->member_type);
            std::string attribute = function->getModifier((function->has_setter ? function->has_setter : function->has_getter) + 1);
            const Variable& v = s.getMember(attribute);
            if (removeTypeModifiers(v.type) == "float") {
                append("(_stack.sp++)->f = Var_self->%s;\n", attribute.c_str());
            } else {
                append("(_stack.sp++)->i = (scl_int) Var_self->%s;\n", attribute.c_str());
            }
            typeStack.push(v.type);
        } else if (hasVar(function->member_type + "$" + body[i].value)) {
            Variable v = getVar(function->member_type + "$" + body[i].value);
            std::string lastType = "";
            std::string path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "", &lastType, /* doesWriteAfter */ false);
            
            LOAD_PATH(path, lastType);
        } else if (function->isMethod) {
            Method* m = ((Method*) function);
            Struct s = getStructByName(result, m->member_type);
            if (hasMethod(result, body[i].value, s.name)) {
                Method* f = getMethodByName(result, body[i].value, s.name);
                append("(_stack.sp++)->i = (scl_int) Var_self;\n");
                typeStack.push(f->member_type);
                methodCall(f, fp, result, warns, errors, body, i);
            } else if (hasFunction(result, s.name + "$" + body[i].value)) {
                std::string struct_ = s.name;
                Function* f = getFunctionByName(result, struct_ + "$" + body[i].value);
                if (f->isCVarArgs()) {
                    createVariadicCall(f, fp, result, errors, body, i);
                    return;
                }
                functionCall(f, fp, result, warns, errors, body, i);
            } else if (s.hasMember(body[i].value)) {

                Token here = body[i];
                body.insert(body.begin() + i, Token(tok_dot, ".", here.line, here.file));
                body.insert(body.begin() + i, Token(tok_identifier, "self", here.line, here.file));
                goto normalVar;
            } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                Variable v = getVar(s.name + "$" + body[i].value);
                std::string lastType = "";
                std::string path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "", &lastType, /* doesWriteAfter */ false);

                LOAD_PATH(path, lastType);
            } else {
                transpilerError("Unknown member of struct '" + m->member_type + "': '" + body[i].value + "'", i);
                errors.push_back(err);
            }
        } else {
            transpilerError("Unknown identifier: '" + body[i].value + "'", i);
            errors.push_back(err);
        }
    }

    handler(New) {
        noUnused;
        safeInc();
        std::string elemSize = "sizeof(scl_any)";
        std::string typeString = "any";
        if (body[i].type == tok_identifier && body[i].value == "<") {
            safeInc();
            auto type = parseType(body, &i);
            if (!type.success) {
                errors.push_back(type);
                return;
            }
            typeString = removeTypeModifiers(type.value);
            elemSize = "sizeof(" + sclTypeToCType(result, type.value) + ")";
            safeInc();
            safeInc();
        }
        int dimensions = 0;
        std::vector<int> startingOffsets;
        while (body[i].type == tok_bracket_open) {
            dimensions++;
            startingOffsets.push_back(i);
            safeInc();
            if (body[i].type == tok_bracket_close) {
                transpilerError("Empty array dimensions are not allowed", i);
                errors.push_back(err);
                i--;
                return;
            }
            while (body[i].type != tok_bracket_close) {
                handle(Token);
                safeInc();
            }
            safeInc();
        }
        i--;
        if (dimensions == 0) {
            transpilerError("Expected '[', but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        if (dimensions == 1) {
            append("(_stack.sp - 1)->v = _scl_new_array_by_size((_stack.sp - 1)->i, %s);\n", elemSize.c_str());
        } else {
            append("{\n");
            scopeDepth++;
            std::string dims = "";
            for (int i = 0; i < dimensions; i++) {
                append("scl_int _scl_dim%d = (--_stack.sp)->i;\n", i);
                if (i > 0) dims += ", ";
                dims += "_scl_dim" + std::to_string(i);
                if (!isPrimitiveIntegerType(typeStackTop)) {
                    transpilerError("Array size must be an integer, but got '" + removeTypeModifiers(typeStackTop) + "'", startingOffsets[i]);
                    errors.push_back(err);
                    return;
                }
                typePop;
            }

            append("scl_int tmp[] = {%s};\n", dims.c_str());
            append("(_stack.sp++)->v = _scl_multi_new_array_by_size(%d, tmp, %s);\n", dimensions, elemSize.c_str());
            scopeDepth--;
            append("}\n");
        }
        typeStack.push("[" + typeString + "]");
    }

    handler(Repeat) {
        noUnused;
        safeInc();
        if (body[i].type != tok_number) {
            transpilerError("Expected integer, but got '" + body[i].value + "'", i);
            errors.push_back(err);
            if (i + 1 < body.size() && body[i + 1].type != tok_do)
                goto lbl_repeat_do_tok_chk;
            safeInc();
            return;
        }
        if (parseNumber(body[i].value) <= 0) {
            transpilerError("Repeat count must be greater than 0", i);
            errors.push_back(err);
        }
        if (i + 1 < body.size() && body[i + 1].type != tok_do) {
        lbl_repeat_do_tok_chk:
            transpilerError("Expected 'do', but got '" + body[i + 1].value + "'", i+1);
            errors.push_back(err);
            safeInc();
            return;
        }
        append("for (scl_int %c = 0; %c < (scl_int) %s; %c++) {\n",
            'a' + repeat_depth,
            'a' + repeat_depth,
            body[i].value.c_str(),
            'a' + repeat_depth
        );
        repeat_depth++;
        scopeDepth++;
        varScopePush();
        pushRepeat();
        safeInc();
    }

    handler(For) {
        noUnused;
        safeInc();
        Token var = body[i];
        if (var.type != tok_identifier) {
            transpilerError("Expected identifier, but got: '" + var.value + "'", i);
            errors.push_back(err);
        }
        safeInc();
        if (body[i].type != tok_in) {
            transpilerError("Expected 'in' keyword in for loop header, but got: '" + body[i].value + "'", i);
            errors.push_back(err);
        }
        safeInc();
        std::string var_prefix = "";
        if (!hasVar(var.value)) {
            var_prefix = "scl_int ";
        }
        append("for (%sVar_%s = ({\n", var_prefix.c_str(), var.value.c_str());
        scopeDepth++;
        while (body[i].type != tok_to) {
            handle(Token);
            safeInc();
        }
        if (body[i].type != tok_to) {
            transpilerError("Expected 'to', but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        safeInc();
        append("(--_stack.sp)->i;\n");
        typePop;
        scopeDepth--;
        append("}); Var_%s != ({\n", var.value.c_str());
        scopeDepth++;
        while (body[i].type != tok_step && body[i].type != tok_do) {
            handle(Token);
            safeInc();
        }
        typePop;
        std::string iterator_direction = "++";
        if (body[i].type == tok_step) {
            safeInc();
            if (body[i].type == tok_do) {
                transpilerError("Expected step, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            std::string val = body[i].value;
            if (val == "+") {
                safeInc();
                iterator_direction = " += ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else if (body[i].value == "+") {
                    iterator_direction = "++";
                } else {
                    transpilerError("Expected number or '+', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "-") {
                safeInc();
                iterator_direction = " -= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else if (body[i].value == "-") {
                    iterator_direction = "--";
                } else {
                    transpilerError("Expected number or '-', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "++") {
                iterator_direction = "++";
            } else if (val == "--") {
                iterator_direction = "--";
            } else if (val == "*") {
                safeInc();
                iterator_direction = " *= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else {
                    transpilerError("Expected number, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "/") {
                safeInc();
                iterator_direction = " /= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else {
                    transpilerError("Expected number, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "<<") {
                safeInc();
                iterator_direction = " <<= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else {
                    transpilerError("Expected number, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == ">>") {
                safeInc();
                iterator_direction = " >>= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else {
                    transpilerError("Expected number, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "nop") {
                iterator_direction = "";
            }
            safeInc();
        }
        if (body[i].type != tok_do) {
            transpilerError("Expected 'do', but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        append("(--_stack.sp)->i;\n");
        scopeDepth--;
        if (iterator_direction == "") {
            append("});) {\n");
        } else
            append("}); Var_%s%s) {\n", var.value.c_str(), iterator_direction.c_str());
        varScopePush();
        if (!hasVar(var.value)) {
            varScopeTop().push_back(Variable(var.value, "int"));
        }
        
        iterator_count++;
        scopeDepth++;

        if (hasFunction(result, var.value)) {
            FPResult result;
            result.message = "Variable '" + var.value + "' shadowed by function '" + var.value + "'";
            result.success = false;
            result.line = var.line;
            result.in = var.file;
            result.column = var.column;
            result.value = var.value;
            result.type =  var.type;
            warns.push_back(result);
        }
        if (hasContainer(result, var.value)) {
            FPResult result;
            result.message = "Variable '" + var.value + "' shadowed by container '" + var.value + "'";
            result.success = false;
            result.line = var.line;
            result.in = var.file;
            result.column = var.column;
            result.value = var.value;
            result.type =  var.type;
            warns.push_back(result);
        }

        pushOther();
    }

    handler(Foreach) {
        noUnused;
        safeInc();
        if (body[i].type != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        std::string iterator_name = "__it" + std::to_string(iterator_count++);
        Token iter_var_tok = body[i];
        safeInc();
        if (body[i].type != tok_in) {
            transpilerError("Expected 'in', but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        safeInc();
        if (body[i].type == tok_do) {
            transpilerError("Expected iterable, but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        while (body[i].type != tok_do) {
            handle(Token);
            safeInc();
        }
        pushIterate();

        std::string type = removeTypeModifiers(typeStackTop);
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            append("%s %s = (%s) (--_stack.sp)->i;\n", sclTypeToCType(result, typeStackTop).c_str(), iterator_name.c_str(), sclTypeToCType(result, typeStackTop).c_str());
            typePop;
            append("for (scl_int i = 0; i < _scl_array_size(%s); i++) {\n", iterator_name.c_str());
            scopeDepth++;

            varScopePush();
            varScopeTop().push_back(Variable(iter_var_tok.value, type.substr(1, type.size() - 2)));
            append("%s Var_%s = %s[i];\n", sclTypeToCType(result, type.substr(1, type.size() - 2)).c_str(), iter_var_tok.value.c_str(), iterator_name.c_str());
            pushOther();
            return;
        }
        
        Struct s = getStructByName(result, typeStackTop);
        if (s == Struct::Null) {
            transpilerError("Can only iterate over structs and arrays, but got '" + typeStackTop + "'", i);
            errors.push_back(err);
            return;
        }
        if (!structImplements(result, s, "Iterable")) {
            transpilerError("Struct '" + typeStackTop + "' is not iterable", i);
            errors.push_back(err);
            return;
        }
        Method* iterateMethod = getMethodByName(result, "iterate", typeStackTop);
        if (iterateMethod == nullptr) {
            transpilerError("Could not find method 'iterate' on type '" + typeStackTop + "'", i);
            errors.push_back(err);
            return;
        }
        methodCall(iterateMethod, fp, result, warns, errors, body, i);
        append("%s %s = (%s) (--_stack.sp)->i;\n", sclTypeToCType(result, typeStackTop).c_str(), iterator_name.c_str(), sclTypeToCType(result, typeStackTop).c_str());
        type = typeStackTop;
        typePop;
        
        Method* nextMethod = getMethodByName(result, "next", type);
        Method* hasNextMethod = getMethodByName(result, "hasNext", type);
        if (hasNextMethod == nullptr) {
            transpilerError("Could not find method 'hasNext' on type '" + type + "'", i);
            errors.push_back(err);
            return;
        }
        if (nextMethod == nullptr) {
            transpilerError("Could not find method 'next' on type '" + type + "'", i);
            errors.push_back(err);
            return;
        }
        std::string var_prefix = "";
        if (!hasVar(iter_var_tok.value)) {
            var_prefix = sclTypeToCType(result, nextMethod->return_type) + " ";
        }
        varScopePush();
        varScopeTop().push_back(Variable(iter_var_tok.value, nextMethod->return_type));
        std::string cType = sclTypeToCType(result, getVar(iter_var_tok.value).type);
        append("while (virtual_call(%s, \"hasNext()i;\")) {\n", iterator_name.c_str());
        scopeDepth++;
        append("%sVar_%s = (%s) virtual_call(%s, \"next%s\");\n", var_prefix.c_str(), iter_var_tok.value.c_str(), cType.c_str(), iterator_name.c_str(), argsToRTSignature(nextMethod).c_str());
    }

    handler(AddrRef) {
        noUnused;
        safeInc();
        Token toGet = body[i];

        Variable v("", "");
        std::string variablePrefix = "";

        std::string lastType;
        std::string path;

        if (hasFunction(result, toGet.value)) {
            Function* f = getFunctionByName(result, toGet.value);
            if (f->isCVarArgs()) {
                transpilerError("Cannot take reference of varargs function '" + f->name + "'", i);
                errors.push_back(err);
                return;
            }
            append("(_stack.sp++)->i = (scl_int) &Function_%s;\n", f->finalName().c_str());
            std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
            typeStack.push(lambdaType);
            return;
        }
        if (!hasVar(body[i].value)) {
            if (body[i + 1].type == tok_double_column) {
                safeInc();
                Struct s = getStructByName(result, body[i - 1].value);
                safeInc();
                if (s != Struct::Null) {
                    if (hasFunction(result, s.name + "$" + body[i].value)) {
                        Function* f = getFunctionByName(result, s.name + "$" + body[i].value);
                        if (f->isCVarArgs()) {
                            transpilerError("Cannot take reference of varargs function '" + f->name + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (f->isMethod) {
                            transpilerError("'" + f->name + "' is not static", i);
                            errors.push_back(err);
                            return;
                        }
                        append("(_stack.sp++)->i = (scl_int) &Function_%s;\n", f->finalName().c_str());
                        std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
                        typeStack.push(lambdaType);
                        return;
                    } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                        std::string loadFrom = s.name + "$" + body[i].value;
                        v = getVar(loadFrom);
                    } else {
                        transpilerError("Struct '" + s.name + "' has no static member named '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                }
            } else if (hasContainer(result, body[i].value)) {
                Container c = getContainerByName(result, body[i].value);
                safeInc();
                safeInc();
                std::string memberName = body[i].value;
                if (!c.hasMember(memberName)) {
                    transpilerError("Unknown container member: '" + memberName + "'", i);
                    errors.push_back(err);
                    return;
                }
                variablePrefix = "Container_" + c.name + ".";
                v = c.getMember(memberName);
            } else if (function->isMethod) {
                Method* m = ((Method*) function);
                Struct s = getStructByName(result, m->member_type);
                if (s.hasMember(body[i].value)) {
                    v = s.getMember(body[i].value);
                } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                    v = getVar(s.name + "$" + body[i].value);
                }
            } else {
                transpilerError("Use of undefined variable '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (hasVar(body[i].value) && body[i + 1].type == tok_double_column) {
            Struct s = getStructByName(result, getVar(body[i].value).type);
            safeInc();
            safeInc();
            if (s != Struct::Null) {
                if (hasMethod(result, body[i].value, s.name)) {
                    Method* f = getMethodByName(result, body[i].value, s.name);
                    if (!f->isMethod) {
                        transpilerError("'" + f->name + "' is static", i);
                        errors.push_back(err);
                        return;
                    }
                    append("(_stack.sp++)->i = (scl_int) &Method_%s$%s;\n", s.name.c_str(), f->finalName().c_str());
                    std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
                    typeStack.push(lambdaType);
                    return;
                } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                    std::string loadFrom = s.name + "$" + body[i].value;
                    v = getVar(loadFrom);
                } else {
                    transpilerError("Struct '" + s.name + "' has no static member named '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
            }
        } else {
            v = getVar(body[i].value);
        }
        path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, variablePrefix, &lastType, /* doesWriteAfter */ false);
        append("(_stack.sp++)->i = (scl_int) &(%s);\n", path.c_str());
        typeStack.push("[" + lastType + "]");
    }

    std::string typePointedTo(std::string type) {
        if (type.size() >= 2 && type.front() == '[' && type.back() == ']') {
            return type.substr(1, type.size() - 2);
        }
        return "any";
    }

    std::string makePath(TPResult& result, Variable v, bool topLevelDeref, std::vector<Token>& body, size_t& i, std::vector<FPResult>& errors, std::string prefix, std::string* currentType, bool doesWriteAfter) {
        std::string path = prefix;
        if (path.size()) {
            path += v.name;
        } else {
            path += "Var_" + v.name;
        }
        *currentType = v.type;
        if (topLevelDeref) {
            path = "(*" + path + ")";
            *currentType = typePointedTo(*currentType);
        }
        if (i + 1 >= body.size() || body[i + 1].type != tok_dot) {
            if (doesWriteAfter && !v.isWritableFrom(currentFunction)) {
                transpilerError("Variable '" + v.name + "' is not mutable", i);
                errors.push_back(err);
                return std::string("(void) 0");
            }
            if (doesWriteAfter) {
                auto funcs = result.functions & std::function<bool(Function*)>([&](Function* f) {
                    return f && (f->name == v.type + "$operator$store" || strstarts(f->name, v.type + "$operator$store$"));
                });

                bool funcFound = false;
                for (Function* f : funcs) {
                    if (
                        f->isMethod ||
                        f->args.size() != 1 ||
                        f->return_type != v.type ||
                        !typesCompatible(result, typeStackTop, f->args[0].type, true)
                    ) {
                        continue;
                    }
                    path += " = Function_" + f->finalName() + "(*(" + sclTypeToCType(result, f->args[0].type) + "*) --_stack.sp)";
                    funcFound = true;
                }
                if (!funcFound) {
                    path += " = *(" + sclTypeToCType(result, v.type) + "*) --_stack.sp";
                }
            }
            return path;
        }
        safeInc();
        while (body[i].type == tok_dot) {
            safeInc();
            bool deref = body[i].type == tok_addr_of;
            if (deref) {
                safeInc();
            }
            Struct s = getStructByName(result, *currentType);
            Layout l = getLayout(result, *currentType);
            if (s == Struct::Null && l.name.size() == 0) {
                transpilerError("Struct '" + *currentType + "' does not exist", i);
                errors.push_back(err);
                return std::string("(void) 0");
            }
            if (!s.hasMember(body[i].value) && !l.hasMember(body[i].value)) {
                transpilerError("Struct '" + *currentType + "' has no member named '" + body[i].value + "'", i);
                errors.push_back(err);
                return std::string("(void) 0");
            } else if (!s.hasMember(body[i].value)) {
                v = l.getMember(body[i].value);
            } else {
                v = s.getMember(body[i].value);
            }
            *currentType = v.type;
            if (deref) {
                *currentType = typePointedTo(*currentType);
            }
            if (doesWriteAfter && !v.isWritableFrom(currentFunction) && doesWriteAfter) {
                transpilerError("Variable '" + v.name + "' is not mutable", i);
                errors.push_back(err);
                return std::string("(void) 0");
            }
            if (!v.isAccessible(currentFunction)) {
                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                errors.push_back(err);
                return std::string("(void) 0");
            }
            if (i + 1 >= body.size() || body[i + 1].type != tok_dot) {
                if (doesWriteAfter && !v.isWritableFrom(currentFunction) && doesWriteAfter) {
                    transpilerError("Member '" + v.name + "' of struct '" + s.name + "' is not mutable", i);
                    errors.push_back(err);
                    return std::string("(void) 0");
                }
                Method* f = nullptr;
                if (deref || !doesWriteAfter) {
                    f = attributeAccessor(result, s.name, v.name);
                } else {
                    f = attributeMutator(result, s.name, v.name);
                }
                if (f) {
                    if (deref || !doesWriteAfter) {
                        path = "Method_" + f->member_type + "$" + f->finalName() + "(" + path + ")";
                    } else {
                        auto funcs = result.functions & std::function<bool(Function*)>([&](Function* f) {
                            return f && (f->name == v.type + "$operator$store" || strstarts(f->name, v.type + "$operator$store$"));
                        });

                        path = "Method_" + f->member_type + "$" + f->finalName() + "(" + path + ", ";

                        bool funcFound = false;
                        for (Function* f : funcs) {
                            if (
                                f->isMethod ||
                                f->args.size() != 1 ||
                                f->return_type != v.type ||
                                !typesCompatible(result, typeStackTop, f->args[0].type, true)
                            ) {
                                continue;
                            }
                            path += "Function_" + f->finalName() + "(*(" + sclTypeToCType(result, f->args[0].type) + "*) --_stack.sp)";
                            funcFound = true;
                        }
                        if (!funcFound) {
                            path += "*(" + sclTypeToCType(result, v.type) + "*) --_stack.sp)";
                        }
                    }
                } else {
                    path = path + "->" + body[i].value;
                    if (!deref && doesWriteAfter) {
                        auto funcs = result.functions & std::function<bool(Function*)>([&](Function* f) {
                            return f && (f->name == v.type + "$operator$store" || strstarts(f->name, v.type + "$operator$store$"));
                        });

                        bool funcFound = false;
                        for (Function* f : funcs) {
                            if (
                                f->isMethod ||
                                f->args.size() != 1 ||
                                f->return_type != v.type ||
                                !typesCompatible(result, typeStackTop, f->args[0].type, true)
                            ) {
                                continue;
                            }
                            path += " = Function_" + f->finalName() + "(*(" + sclTypeToCType(result, f->args[0].type) + "*) --_stack.sp)";
                            funcFound = true;
                        }
                        if (!funcFound) {
                            path += " = *(" + sclTypeToCType(result, v.type) + "*) --_stack.sp";
                        }
                    }
                }
                if (deref) {
                    path = "(*" + path + ")";
                    if (doesWriteAfter) {
                        path += " = *(" + sclTypeToCType(result, v.type) + "*) --_stack.sp";
                    }
                }
                return path;
            } else {
                Method* f = attributeAccessor(result, s.name, v.name);
                if (f) {
                    path = "Method_" + f->member_type + "$" + f->finalName() + "(" + path + ")";
                } else {
                    path = path + "->" + body[i].value;
                }
                if (deref) {
                    path = "(*" + path + ")";
                }
            }
            safeInc();
        }
        i--;
        return path;
    }

    handler(Store) {
        noUnused;
        safeInc();        

        if (body[i].type == tok_paren_open) {
            append("{\n");
            scopeDepth++;
            append("scl_Array tmp = (scl_Array) (--_stack.sp)->i;\n");
            typePop;
            safeInc();
            int destructureIndex = 0;
            while (body[i].type != tok_paren_close) {
                if (body[i].type == tok_comma) {
                    safeInc();
                    continue;
                }
                if (body[i].type == tok_paren_close)
                    break;
                
                if (body[i].type == tok_addr_of) {
                    safeInc();
                    if (hasContainer(result, body[i].value)) {
                        std::string containerName = body[i].value;
                        safeInc();
                        if (body[i].type != tok_dot) {
                            transpilerError("Expected '.' to access container contents, but got '" + body[i].value + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        safeInc();
                        std::string memberName = body[i].value;
                        Container container = getContainerByName(result, containerName);
                        if (!container.hasMember(memberName)) {
                            transpilerError("Unknown container member: '" + memberName + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!container.getMember(memberName).isWritableFrom(function)) {
                            transpilerError("Container member variable '" + body[i].value + "' is not mutable", i);
                            errors.push_back(err);
                            safeInc();
                            safeInc();
                            return;
                        }
                        append("*((scl_any*) Container_%s.%s) = Method_Array$get(tmp, %d);\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                    } else {
                        if (body[i].type != tok_identifier) {
                            transpilerError("'" + body[i].value + "' is not an identifier", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!hasVar(body[i].value)) {
                            if (function->isMethod) {
                                Method* m = ((Method*) function);
                                Struct s = getStructByName(result, m->member_type);
                                if (s.hasMember(body[i].value)) {
                                    Variable mem = getVar(s.name + "$" + body[i].value);
                                    if (!mem.isWritableFrom(function)) {
                                        transpilerError("Variable '" + body[i].value + "' is not mutable", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("*(scl_any*) Var_self->%s = Method_Array$get(tmp, %d);\n", body[i].value.c_str(), destructureIndex);
                                    destructureIndex++;
                                    continue;
                                } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                                    Variable mem = getVar(s.name + "$" + body[i].value);
                                    if (!mem.isWritableFrom(function)) {
                                        transpilerError("Variable '" + body[i].value + "' is not mutable", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("*(scl_any*) Var_%s$%s = Method_Array$get(tmp, %d);\n", s.name.c_str(), body[i].value.c_str(), destructureIndex);
                                    destructureIndex++;
                                    continue;
                                }
                            }
                            transpilerError("Use of undefined variable '" + body[i].value + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        const Variable& v = getVar(body[i].value);
                        std::string loadFrom = v.name;
                        if (getStructByName(result, v.type) != Struct::Null) {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].value + "' is not mutable", i);
                                errors.push_back(err);
                                safeInc();
                                safeInc();
                                return;
                            }
                            safeInc();
                            if (body[i].type != tok_dot) {
                                append("*(scl_any*) Var_%s = Method_Array$get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                                destructureIndex++;
                                continue;
                            }
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i - 1].value + "' is not mutable", i - 1);
                                errors.push_back(err);
                                safeInc();
                                return;
                            }
                            safeInc();
                            Struct s = getStructByName(result, v.type);
                            if (!s.hasMember(body[i].value)) {
                                std::string help = "";
                                if (getMethodByName(result, body[i].value, s.name)) {
                                    help = ". Maybe you meant to use ':' instead of '.' here";
                                }
                                transpilerError("Struct '" + s.name + "' has no member named '" + body[i].value + "'" + help, i);
                                errors.push_back(err);
                                return;
                            }
                            Variable mem = s.getMember(body[i].value);
                            if (!mem.isWritableFrom(function)) {
                                transpilerError("Member variable '" + body[i].value + "' is not mutable", i);
                                errors.push_back(err);
                                return;
                            }
                            if (!mem.isAccessible(currentFunction)) {
                                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*(scl_any*) Var_%s->%s = Method_Array$get(tmp, %d);\n", loadFrom.c_str(), body[i].value.c_str(), destructureIndex);
                        } else {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].value + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*(scl_any*) Var_%s = Method_Array$get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                        }
                    }
                } else {
                    if (hasContainer(result, body[i].value)) {
                        std::string containerName = body[i].value;
                        safeInc();
                        if (body[i].type != tok_dot) {
                            transpilerError("Expected '.' to access container contents, but got '" + body[i].value + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        safeInc();
                        std::string memberName = body[i].value;
                        Container container = getContainerByName(result, containerName);
                        if (!container.hasMember(memberName)) {
                            transpilerError("Unknown container member: '" + memberName + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!container.getMember(memberName).isWritableFrom(function)) {
                            transpilerError("Container member variable '" + body[i].value + "' is const", i);
                            errors.push_back(err);
                            return;
                        }
                        append("{\n");
                        scopeDepth++;
                        append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                        if (!typeCanBeNil(container.getMember(memberName).type)) {
                            append("_scl_check_not_nil_store((scl_int) tmp%d, \"%s.%s\");\n", destructureIndex, containerName.c_str(), memberName.c_str());
                        }
                        append("(*(scl_any*) &Container_%s.%s) = tmp%d;\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                        scopeDepth--;
                        append("}\n");
                    } else {
                        if (body[i].type != tok_identifier) {
                            transpilerError("'" + body[i].value + "' is not an identifier", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!hasVar(body[i].value)) {
                            if (function->isMethod) {
                                Method* m = ((Method*) function);
                                Struct s = getStructByName(result, m->member_type);
                                if (s.hasMember(body[i].value)) {
                                    Variable mem = s.getMember(body[i].value);
                                    if (!mem.isWritableFrom(function)) {
                                        transpilerError("Member variable '" + body[i].value + "' is const", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("{\n");
                                    scopeDepth++;
                                    append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                                    if (!typeCanBeNil(mem.type)) {
                                        append("_scl_check_not_nil_store((scl_int) tmp%d, \"self.%s\");\n", destructureIndex, mem.name.c_str());
                                    }
                                    append("*(scl_any*) &Var_self->%s = tmp%d;\n", body[i].value.c_str(), destructureIndex);
                                    scopeDepth--;
                                    append("}\n");
                                    destructureIndex++;
                                    continue;
                                } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                                    if (!getVar(s.name + "$" + body[i].value).isWritableFrom(function)) {
                                        transpilerError("Member variable '" + body[i].value + "' is const", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("{\n");
                                    scopeDepth++;
                                    append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                                    if (!typeCanBeNil(getVar(s.name + "$" + body[i].value).type)) {
                                        append("_scl_check_not_nil_store((scl_int) tmp%d, \"%s::%s\");\n", destructureIndex, s.name.c_str(), body[i].value.c_str());
                                    }
                                    append("*(scl_any*) &Var_%s$%s = tmp%d;\n", s.name.c_str(), body[i].value.c_str(), destructureIndex);
                                    scopeDepth--;
                                    append("}\n");
                                    continue;
                                }
                            }
                            transpilerError("Use of undefined variable '" + body[i].value + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        const Variable& v = getVar(body[i].value);
                        std::string loadFrom = v.name;
                        if (getStructByName(result, v.type) != Struct::Null) {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].value + "' is const", i);
                                errors.push_back(err);
                                safeInc();
                                safeInc();
                                return;
                            }
                            safeInc();
                            if (body[i].type != tok_dot) {
                                append("{\n");
                                scopeDepth++;
                                append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                                if (!typeCanBeNil(v.type)) {
                                    append("_scl_check_not_nil_store((scl_int) tmp%d, \"%s\");\n", destructureIndex, v.name.c_str());
                                }
                                append("(*(scl_any*) &Var_%s) = tmp%d;\n", loadFrom.c_str(), destructureIndex);
                                scopeDepth--;
                                append("}\n");
                                destructureIndex++;
                                continue;
                            }
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i - 1].value + "' is not mutable", i - 1);
                                errors.push_back(err);
                                safeInc();
                                return;
                            }
                            safeInc();
                            Struct s = getStructByName(result, v.type);
                            if (!s.hasMember(body[i].value)) {
                                transpilerError("Struct '" + s.name + "' has no member named '" + body[i].value + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            Variable mem = s.getMember(body[i].value);
                            if (!mem.isWritableFrom(function)) {
                                transpilerError("Member variable '" + body[i].value + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            if (!mem.isAccessible(currentFunction)) {
                                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            append("{\n");
                            scopeDepth++;
                            append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                            if (!typeCanBeNil(mem.type)) {
                                append("_scl_check_not_nil_store((scl_int) tmp%d, \"%s.%s\");\n", destructureIndex, s.name.c_str(), mem.name.c_str());
                            }
                            if (!typeCanBeNil(mem.type)) {
                                append("_scl_check_not_nil_store((scl_int) tmp%d, \"self.%s\");\n", destructureIndex, mem.name.c_str());
                            }
                            Method* f = attributeMutator(result, s.name, mem.name);
                            if (f) {
                                append("Method_%s$%s(Var_%s, *(%s*) &tmp%d);\n", f->member_type.c_str(), f->name.c_str(), loadFrom.c_str(), sclTypeToCType(result, f->args[0].type).c_str(), destructureIndex);
                                return;
                            }
                            append("(*(scl_any*) &Var_%s->%s) = tmp%d;\n", loadFrom.c_str(), body[i].value.c_str(), destructureIndex);
                            scopeDepth--;
                            append("}\n");
                        } else {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].value + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            append("{\n");
                            scopeDepth++;
                            append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                            if (!typeCanBeNil(v.type)) {
                                append("_scl_check_not_nil_store((scl_int) tmp%d, \"%s\");\n", destructureIndex, v.name.c_str());
                            }
                            append("(*(scl_any*) &Var_%s) = tmp%d;\n", loadFrom.c_str(), destructureIndex);
                            scopeDepth--;
                            append("}\n");
                        }
                    }
                }
                safeInc();
                destructureIndex++;
            }
            if (destructureIndex == 0) {
                transpilerError("Empty Array destructure", i);
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
            }
            scopeDepth--;
            append("}\n");
        } else if (body[i].type == tok_declare) {
            safeInc();
            if (body[i].type != tok_identifier) {
                transpilerError("'" + body[i].value + "' is not an identifier", i+1);
                errors.push_back(err);
                return;
            }
            if (hasFunction(result, body[i].value)) {
                {
                    transpilerError("Variable '" + body[i].value + "' shadowed by function '" + body[i].value + "'", i);
                    if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                    else errors.push_back(err);
                }
            }
            if (hasContainer(result, body[i].value)) {
                {
                    transpilerError("Variable '" + body[i].value + "' shadowed by container '" + body[i].value + "'", i);
                    if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                    else errors.push_back(err);
                }
            }
            if (getStructByName(result, body[i].value) != Struct::Null) {
                {
                    transpilerError("Variable '" + body[i].value + "' shadowed by struct '" + body[i].value + "'", i);
                    if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                    else errors.push_back(err);
                }
            }
            std::string name = body[i].value;
            std::string type = "any";
            safeInc();
            if (body[i].type == tok_column) {
                safeInc();
                FPResult r = parseType(body, &i, getTemplates(result, function));
                if (!r.success) {
                    errors.push_back(r);
                    return;
                }
                type = r.value;
                if (type == "lambda" && strstarts(typeStackTop, "lambda(")) {
                    type = typeStackTop;
                }
            } else {
                type = typeStackTop;
                i--;
            }
            varScopeTop().push_back(Variable(name, type));
            
            auto funcs = result.functions & std::function<bool(Function*)>([&](Function* f) {
                return f && (f->name == type + "$operator$store" || strstarts(f->name, type + "$operator$store$"));
            });

            for (Function* f : funcs) {
                if (
                    f->isMethod ||
                    f->args.size() != 1 ||
                    f->return_type != type ||
                    !typesCompatible(result, typeStackTop, f->args[0].type, true)
                ) {
                    continue;
                }
                append("%s Var_%s = Function_%s(*(%s*) --_stack.sp);\n", sclTypeToCType(result, type).c_str(), name.c_str(), f->finalName().c_str(), sclTypeToCType(result, f->args[0].type).c_str());
                typePop;
                return;
            }
            if (!varScopeTop().back().canBeNil) {
                append("_scl_check_not_nil_store((_stack.sp - 1)->i, \"%s\");\n", varScopeTop().back().name.c_str());
            }
            if (!typesCompatible(result, typeStackTop, type, true)) {
                transpilerError("Incompatible types: '" + type + "' and '" + typeStackTop + "'", i);
                warns.push_back(err);
            }
            append("%s Var_%s = *(%s*) --_stack.sp;\n", sclTypeToCType(result, type).c_str(), name.c_str(), sclTypeToCType(result, type).c_str());
            typePop;
        } else {
            if (body[i].type != tok_identifier && body[i].type != tok_addr_of) {
                transpilerError("'" + body[i].value + "' is not an identifier", i);
                errors.push_back(err);
            }
            if (function->has_setter || function->has_getter) {
                if (body[i].value == "field") {
                    Struct s = getStructByName(result, function->member_type);
                    std::string attribute = function->getModifier((function->has_setter ? function->has_setter : function->has_getter) + 1);
                    const Variable& v = s.getMember(attribute);
                    if (removeTypeModifiers(v.type) == "float") {
                        append("Var_self->%s = (--_stack.sp)->f;\n", attribute.c_str());
                    } else {
                        append("Var_self->%s = (%s) (--_stack.sp)->i;\n", attribute.c_str(), sclTypeToCType(result, v.type).c_str());
                    }
                    typeStack.pop();
                    return;
                }
            }
            Variable v("", "");
            std::string containerBegin = "";
            std::string lastType = "";

            std::string variablePrefix = "";

            bool topLevelDeref = false;

            if (body[i].type == tok_addr_of) {
                safeInc();
                topLevelDeref = true;
                if (body[i].type == tok_paren_open) {
                    if (isInUnsafe) {
                        safeInc();
                        append("{\n");
                        scopeDepth++;
                        append("scl_int _scl_value_to_store = (--_stack.sp)->i;\n");
                        typePop;
                        append("{\n");
                        scopeDepth++;
                        while (body[i].type != tok_paren_close) {
                            handle(Token);
                            safeInc();
                        }
                        scopeDepth--;
                        append("}\n");
                        typePop;
                        append("*(scl_int*) (--_stack.sp)->i = _scl_value_to_store;\n");
                        scopeDepth--;
                        append("}\n");
                    } else {
                        transpilerError("Storing at calculated pointer offset is only allowed in 'unsafe' blocks or functions", i - 1);
                        errors.push_back(err);
                    }
                    return;
                }
            }

            int start = i;

            if (hasVar(body[i].value)) {
            normalVar:
                v = getVar(body[i].value);
            } else if (hasContainer(result, body[i].value)) {
                Container c = getContainerByName(result, body[i].value);
                safeInc(); // i -> .
                safeInc(); // i -> member
                std::string memberName = body[i].value;
                if (!c.hasMember(memberName)) {
                    transpilerError("Unknown container member: '" + memberName + "'", i);
                    errors.push_back(err);
                    return;
                }
                variablePrefix = "Container_" + c.name + ".";
                v = c.getMember(memberName);
            } else if (hasVar(function->member_type + "$" + body[i].value)) {
                v = getVar(function->member_type + "$" + body[i].value);
            } else if (body[i + 1].type == tok_double_column) {
                Struct s = getStructByName(result, body[i].value);
                safeInc(); // i -> ::
                safeInc(); // i -> member
                if (s != Struct::Null) {
                    if (!hasVar(s.name + "$" + body[i].value)) {
                        transpilerError("Struct '" + s.name + "' has no static member named '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    v = getVar(s.name + "$" + body[i].value);
                } else {
                    transpilerError("Struct '" + body[i].value + "' does not exist", i);
                    errors.push_back(err);
                    return;
                }
            } else if (function->isMethod) {
                Method* m = ((Method*) function);
                Struct s = getStructByName(result, m->member_type);
                if (s.hasMember(body[i].value)) {
                    // v = s.getMember(body[i].value);
                    Token here = body[i];
                    body.insert(body.begin() + i, Token(tok_dot, ".", here.line, here.file));
                    body.insert(body.begin() + i, Token(tok_identifier, "self", here.line, here.file));
                    goto normalVar;
                } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                    v = getVar(s.name + "$" + body[i].value);
                } else {
                    transpilerError("Struct '" + s.name + "' has no member named '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
            } else {
                transpilerError("Unknown variable: '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }

            std::string currentType;
            std::string path = makePath(result, v, topLevelDeref, body, i, errors, variablePrefix, &currentType);
            // allow for => x[...], but not => x[(...)]
            if (i + 1 < body.size() && body[i + 1].type == tok_bracket_open && (i + 2 >= body.size() || body[i + 2].type != tok_paren_open)) {
                i = start;
                append("{\n");
                scopeDepth++;
                append("scl_any value = (--_stack.sp)->v;\n");
                std::string valueType = removeTypeModifiers(typeStackTop);
                typePop;

                append("{\n");
                scopeDepth++;
                while (body[i].type != tok_bracket_open) {
                    handle(Token);
                    safeInc();
                }
                scopeDepth--;
                append("}\n");
                int openBracket = i;
                safeInc();
                std::string arrayType = removeTypeModifiers(typeStackTop);
                append("%s array = (--_stack.sp)->v;\n", sclTypeToCType(result, arrayType).c_str());

                typePop;
            reevaluateArrayIndexing:

                append("{\n");
                scopeDepth++;
                while (body[i].type != tok_bracket_close) {
                    handle(Token);
                    safeInc();
                }
                scopeDepth--;
                append("}\n");
                std::string indexingType = "";
                bool multiDimensional = false;
                if (i + 1 < body.size() && body[i + 1].type == tok_bracket_open) {
                    multiDimensional = true;
                    openBracket = i + 1;
                    safeIncN(2);
                }
                if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                    indexingType = "int";
                } else if (hasMethod(result, (multiDimensional ? "[]" : "=>[]"), arrayType)) {
                    Method* m = getMethodByName(result, (multiDimensional ? "[]" : "=>[]"), arrayType);
                    indexingType = m->args[0].type;
                }

                scopeDepth++;
                append("{\n");
                append("%s index = (%s) (--_stack.sp)->i;\n", sclTypeToCType(result, indexingType).c_str(), sclTypeToCType(result, indexingType).c_str());
                std::string indexType = removeTypeModifiers(typeStackTop);
                if (!typeEquals(indexType, removeTypeModifiers(indexingType))) {
                    transpilerError("'" + arrayType + "' cannot be indexed with '" + indexType + "'", i);
                    errors.push_back(err);
                    return;
                }
                // indexType and indexingType are the same type
                typePop;

                if (multiDimensional) {
                    if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                        append("_scl_array_check_bounds_or_throw(array, index);\n");
                        append("array = array[index];\n");
                        if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                            arrayType = arrayType.substr(1, arrayType.size() - 2);
                        } else {
                            arrayType = "any";
                        }
                    } else {
                        if (hasMethod(result, "[]", arrayType)) {
                            Method* m = getMethodByName(result, "[]", arrayType);
                            if (m->args.size() != 2) {
                                transpilerError("Method '[]' of type '" + arrayType + "' must have exactly 1 argument! Signature should be: '[](index: " + m->args[0].type + ")'", i);
                                errors.push_back(err);
                                return;
                            }
                            if (removeTypeModifiers(m->return_type) == "none" || removeTypeModifiers(m->return_type) == "nothing") {
                                transpilerError("Method '[]' of type '" + arrayType + "' must return a value", i);
                                errors.push_back(err);
                                return;
                            }
                            append("(_stack.sp++)->v = index;\n");
                            typeStack.push(indexType);
                            append("(_stack.sp++)->v = array;\n");
                            typeStack.push(arrayType);
                            methodCall(m, fp, result, warns, errors, body, i);
                        } else {
                            transpilerError("Type '" + arrayType + "' does not have a method '[]'", openBracket);
                            errors.push_back(err);
                        }
                    }
                    scopeDepth--;
                    append("}\n");
                    goto reevaluateArrayIndexing;
                }
                
                if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                    if (arrayType != "[any]" && !typeEquals(valueType, arrayType.substr(1, arrayType.size() - 2))) {
                        transpilerError("Array of type '" + arrayType + "' cannot contain '" + valueType + "'", start - 1);
                        errors.push_back(err);
                        return;
                    }
                    append("_scl_array_check_bounds_or_throw(array, index);\n");
                    append("array[index] = value;\n");
                } else {
                    if (hasMethod(result, "=>[]", arrayType)) {
                        Method* m = getMethodByName(result, "=>[]", arrayType);
                        if (m->args.size() != 3) {
                            transpilerError("Method '=>[]' of type '" + arrayType + "' does not take 2 arguments! Signature should be: '=>[](index: " + indexType + ", value: " + valueType + ")'", openBracket);
                            errors.push_back(err);
                            return;
                        }
                        if (removeTypeModifiers(m->return_type) != "none") {
                            transpilerError("Method '=>[]' of type '" + arrayType + "' should not return anything", openBracket);
                            errors.push_back(err);
                            return;
                        }
                        append("(_stack.sp++)->v = index;\n");
                        typeStack.push(indexType);
                        append("(_stack.sp++)->v = value;\n");
                        typeStack.push(valueType);
                        append("(_stack.sp++)->v = array;\n");
                        typeStack.push(arrayType);
                        methodCall(m, fp, result, warns, errors, body, i);
                    } else {
                        transpilerError("Type '" + arrayType + "' does not have a method '=>[]'", openBracket);
                        errors.push_back(err);
                    }
                }
                scopeDepth--;
                append("}\n");
                scopeDepth--;
                append("}\n");
                return;
            }

            append("{\n");
            scopeDepth++;

            if (!typesCompatible(result, typeStackTop, currentType, true)) {
                transpilerError("Incompatible types: '" + currentType + "' and '" + typeStackTop + "'", i);
                warns.push_back(err);
            }
            if (!typeCanBeNil(currentType)) {
                append("_scl_check_not_nil_store(*(scl_int*) (_stack.sp - 1), \"%s\");\n", v.name.c_str());
            }
            append("%s;\n", path.c_str());
            typePop;
            scopeDepth--;
            append("}\n");
        }
    }

    handler(Declare) {
        noUnused;
        safeInc();
        if (body[i].type != tok_identifier) {
            transpilerError("'" + body[i].value + "' is not an identifier", i+1);
            errors.push_back(err);
            return;
        }
        if (hasFunction(result, body[i].value)) {
            transpilerError("Variable '" + body[i].value + "' shadowed by function '" + body[i].value + "'", i+1);
            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
            else errors.push_back(err);
        }
        if (hasContainer(result, body[i].value)) {
            transpilerError("Variable '" + body[i].value + "' shadowed by container '" + body[i].value + "'", i+1);
            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
            else errors.push_back(err);
        }
        if (getStructByName(result, body[i].value) != Struct::Null) {
            transpilerError("Variable '" + body[i].value + "' shadowed by struct '" + body[i].value + "'", i+1);
            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
            else errors.push_back(err);
        }
        std::string name = body[i].value;
        size_t start = i;
        std::string type = "any";
        safeInc();
        if (body[i].type == tok_column) {
            safeInc();
            FPResult r = parseType(body, &i, getTemplates(result, function));
            if (!r.success) {
                errors.push_back(r);
                return;
            }
            type = r.value;
        } else {
            transpilerError("A type is required", i);
            errors.push_back(err);
            return;
        }
        varScopeTop().push_back(Variable(name, type));
        if (!varScopeTop().back().canBeNil) {
            transpilerError("Uninitialized variable '" + name + "' with non-nil type '" + type + "'", start);
            errors.push_back(err);
        }
        type = sclTypeToCType(result, type);
        if (type == "scl_float") {
            append("%s Var_%s = 0.0;\n", type.c_str(), name.c_str());
        } else {
            if (hasTypealias(result, type) || hasLayout(result, type)) {
                append("%s Var_%s;\n", type.c_str(), name.c_str());
            } else {
                append("%s Var_%s = 0;\n", type.c_str(), name.c_str());
            }
        }
    }

    handler(CurlyOpen) {
        noUnused;
        std::string struct_ = "Array";
        if (getStructByName(result, struct_) == Struct::Null) {
            transpilerError("Struct definition for 'Array' not found", i);
            errors.push_back(err);
            return;
        }
        append("{\n");
        scopeDepth++;
        Method* f = getMethodByName(result, "init", struct_);
        Struct arr = getStructByName(result, "Array");
        append("scl_Array tmp = ALLOC(Array);\n");
        if (f->return_type.size() > 0 && f->return_type != "none" && f->return_type != "nothing") {
            if (f->return_type == "float") {
                append("(_stack.sp++)->f = Method_Array$init(tmp, 1);\n");
            } else {
                append("(_stack.sp++)->i = (scl_int) Method_Array$init(tmp, 1);\n");
            }
            typeStack.push(f->return_type);
        } else {
            append("Method_Array$init(tmp, 1);\n");
        }
        safeInc();
        while (body[i].type != tok_curly_close) {
            if (body[i].type == tok_comma) {
                safeInc();
                continue;
            }
            bool didPush = false;
            while (body[i].type != tok_comma && body[i].type != tok_curly_close) {
                handle(Token);
                safeInc();
                didPush = true;
            }
            if (didPush) {
                append("Method_Array$push(tmp, (--_stack.sp)->v);\n");
                typePop;
            }
        }
        append("(_stack.sp++)->v = tmp;\n");
        typeStack.push("Array");
        scopeDepth--;
        append("}\n");
    }

    handler(BracketOpen) {
        noUnused;
        std::string type = removeTypeModifiers(typeStackTop);
        if (i + 1 < body.size()) {
            if (body[i + 1].type == tok_paren_open) {
                safeIncN(2);
                append("{\n");
                scopeDepth++;
                bool existingArrayUsed = false;
                if (body[i].type == tok_number && body[i + 1].type == tok_store) {
                    long long array_size = std::stoll(body[i].value);
                    if (array_size < 1) {
                        transpilerError("Array size must be positive", i);
                        errors.push_back(err);
                        return;
                    }
                    append("scl_int array_size = %s;\n", body[i].value.c_str());
                    safeIncN(2);
                } else {
                    while (body[i].type != tok_store) {
                        handle(Token);
                        safeInc();
                    }
                    std::string top = removeTypeModifiers(typeStackTop);
                    if (top.size() > 2 && top.front() == '[' && top.back() == ']') {
                        existingArrayUsed = true;
                    }
                    if (!isPrimitiveIntegerType(typeStackTop) && !existingArrayUsed) {
                        transpilerError("Array size must be an integer", i);
                        errors.push_back(err);
                        return;
                    }
                    if (existingArrayUsed) {
                        append("scl_int array_size = _scl_array_size((_stack.sp - 1)->v);\n");
                    } else {
                        append("scl_int array_size = (--_stack.sp)->i;\n");
                        typePop;
                    }
                    safeInc();
                }
                std::string arrayType = "[int]";
                std::string elementType = "int";
                if (existingArrayUsed) {
                    arrayType = removeTypeModifiers(typeStackTop);
                    typePop;
                    if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                        elementType = arrayType.substr(1, arrayType.size() - 2);
                    } else {
                        elementType = "any";
                    }
                    append("%s* array = (--_stack.sp)->v;\n", sclTypeToCType(result, elementType).c_str());
                } else {
                    append("scl_int* array = _scl_new_array(array_size);\n");
                }
                append("for (scl_int i = 0; i < array_size; i++) {\n");
                varScopePush();
                append("const scl_int Var_i = i;\n");
                varScopeTop().push_back(Variable("i", "const int"));
                size_t typeStackSize = typeStack.size();
                if (existingArrayUsed) {
                    append("const %s Var_val = array[i];\n", sclTypeToCType(result, elementType).c_str());
                    varScopeTop().push_back(Variable("val", "const " + elementType));
                }
                while (body[i].type != tok_paren_close) {
                    handle(Token);
                    safeInc();
                }
                for (size_t i = typeStackSize; i < typeStack.size(); i++) {
                    typePop;
                }
                varScopePop();
                typePop;
                append("array[i] = (--_stack.sp)->i;\n");
                append("}\n");
                append("(_stack.sp++)->v = array;\n");
                typeStack.push(arrayType);
                scopeDepth--;
                append("}\n");
                safeInc();
                if (body[i].type != tok_bracket_close) {
                    transpilerError("Expected ']', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                return;
            }
        }
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            if ((i + 2) < body.size()) {
                if (body[i + 1].type == tok_number && body[i + 2].type == tok_bracket_close) {
                    safeIncN(2);
                    long long index = std::stoll(body[i - 1].value);
                    if (index < 0) {
                        transpilerError("Array subscript cannot be negative", i - 1);
                        errors.push_back(err);
                        return;
                    }
                    append("_scl_array_check_bounds_or_throw((_stack.sp - 1)->v, %s);\n", body[i - 1].value.c_str());
                    append("(_stack.sp - 1)->v = ((%s) (_stack.sp - 1)->v)[%s];\n", sclTypeToCType(result, type).c_str(), body[i - 1].value.c_str());
                    typePop;
                    typeStack.push(type.substr(1, type.size() - 2));
                    return;
                }
            }
            safeInc();
            if (body[i].type == tok_bracket_close) {
                transpilerError("Array subscript required", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            scopeDepth++;
            append("%s tmp = (scl_any*) (--_stack.sp)->v;\n", sclTypeToCType(result, type).c_str());
            typePop;
            varScopePush();
            while (body[i].type != tok_bracket_close) {
                handle(Token);
                safeInc();
            }
            varScopePop();
            if (!isPrimitiveIntegerType(typeStackTop)) {
                transpilerError("'" + type + "' cannot be indexed with '" + typeStackTop + "'", i);
                errors.push_back(err);
                return;
            }
            typePop;
            append("scl_int index = (--_stack.sp)->i;\n");
            append("_scl_array_check_bounds_or_throw(tmp, index);\n");
            append("(_stack.sp++)->v = tmp[index];\n");
            typeStack.push(type.substr(1, type.size() - 2));
            scopeDepth--;
            append("}\n");
            return;
        }
        if (hasMethod(result, "[]", typeStackTop)) {
            Method* m = getMethodByName(result, "[]", typeStackTop);
            std::string type = typeStackTop;
            if (m->args.size() != 2) {
                transpilerError("Method '[]' of type '" + type + "' must have exactly 1 argument! Signature should be: '[](index: " + m->args[0].type + ")'", i);
                errors.push_back(err);
                return;
            }
            if (removeTypeModifiers(m->return_type) == "none" || removeTypeModifiers(m->return_type) == "nothing") {
                transpilerError("Method '[]' of type '" + type + "' must return a value", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            scopeDepth++;
            append("scl_any instance = (--_stack.sp)->v;\n");
            std::string indexType = typeStackTop;
            typePop;

            varScopePush();
            safeInc();
            if (body[i].type == tok_bracket_close) {
                transpilerError("Empty array subscript", i);
                errors.push_back(err);
                return;
            }
            while (body[i].type != tok_bracket_close) {
                handle(Token);
                safeInc();
            }
            varScopePop();
            if (!typeEquals(removeTypeModifiers(typeStackTop), removeTypeModifiers(m->args[0].type))) {
                transpilerError("'" + m->member_type + "' cannot be indexed with '" + removeTypeModifiers(typeStackTop) + "'", i);
                errors.push_back(err);
                return;
            }
            append("(_stack.sp++)->v = instance;\n");
            typeStack.push(type);
            methodCall(m, fp, result, warns, errors, body, i);
            scopeDepth--;
            append("}\n");
            return;
        }
        transpilerError("'" + type + "' cannot be indexed", i);
        errors.push_back(err);
    }

    handler(ParenOpen) {
        noUnused;
        int commas = 0;
        auto stackSizeHere = typeStack.size();
        safeInc();
        append("{\n");
        scopeDepth++;
        append("_scl_frame_t* begin_stack_size = _stack.sp;\n");
        while (body[i].type != tok_paren_close) {
            if (body[i].type == tok_comma) {
                commas++;
                safeInc();
            }
            handle(Token);
            safeInc();
        }

        if (commas == 0) {
            // Last-returns expression
            if (typeStack.size() > stackSizeHere) {
                std::string returns = typeStackTop;
                append("_scl_frame_t* return_value = (--_stack.sp);\n");
                append("_stack.sp = begin_stack_size;\n");
                while (typeStack.size() > stackSizeHere) {
                    typeStack.pop();
                }
                append("(_stack.sp++)->i = return_value->i;\n");
                typeStack.push(returns);
            }
        } else if (commas == 1) {
            Struct pair = getStructByName(result, "Pair");
            if (pair == Struct::Null) {
                transpilerError("Struct definition for 'Pair' not found", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            scopeDepth++;
            append("scl_Pair tmp = ALLOC(Pair);\n");
            append("_stack.sp -= (2);\n");
            Method* f = getMethodByName(result, "init", "Pair");
            typePop;
            typePop;
            if (f->return_type.size() > 0 && f->return_type != "none" && f->return_type != "nothing") {
                if (f->return_type == "float") {
                    append("(_stack.sp++)->f = Method_Pair$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v);\n");
                } else {
                    append("(_stack.sp++)->i = (scl_int) Method_Pair$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v);\n");
                }
                typeStack.push(f->return_type);
            } else {
                append("Method_Pair$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v);\n");
            }
            append("(_stack.sp++)->v = tmp;\n");
            typeStack.push("Pair");
            scopeDepth--;
            append("}\n");
        } else if (commas == 2) {
            Struct triple = getStructByName(result, "Triple");
            if (getStructByName(result, "Triple") == Struct::Null) {
                transpilerError("Struct definition for 'Triple' not found", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            scopeDepth++;
            append("scl_Triple tmp = ALLOC(Triple);\n");
            append("_stack.sp -= (3);\n");
            Method* f = getMethodByName(result, "init", "Triple");
            typePop;
            typePop;
            typePop;
            if (f->return_type.size() > 0 && f->return_type != "none" && f->return_type != "nothing") {
                if (f->return_type == "float") {
                    append("(_stack.sp++)->f = Method_Triple$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v, _scl_positive_offset(2)->v);\n");
                } else {
                    append("(_stack.sp++)->i = (scl_int) Method_Triple$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v, _scl_positive_offset(2)->v);\n");
                }
                typeStack.push(f->return_type);
            } else {
                append("Method_Triple$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v, _scl_positive_offset(2)->v);\n");
            }
            append("(_stack.sp++)->v = tmp;\n");
            typeStack.push("Triple");
            scopeDepth--;
            append("}\n");
        } else {
            transpilerError("Unsupported tuple-like literal", i);
            errors.push_back(err);
        }
        scopeDepth--;
        append("}\n");
    }

    handler(StringLiteral) {
        noUnused;
        if (i + 2 < body.size()) {
            if (body[i + 1].type == tok_column) {
                if (body[i + 2].type == tok_identifier) {
                    if (body[i + 2].value == "view") {
                        append("(_stack.sp++)->v = \"%s\";\n", body[i].value.c_str());
                        typeStack.push("[int8]");
                        safeInc();
                        safeInc();
                        return;
                    }
                }
            } else if (body[i + 1].type == tok_identifier) {
            stringLiteral_IdentifierAfter:
                if (body[i + 1].value == "puts") {
                    append("puts(\"%s\");\n", body[i].value.c_str());
                    safeInc();
                    return;
                } else if (body[i + 1].value == "eputs") {
                    append("fprintf(stderr, \"%s\\n\");\n", body[i].value.c_str());
                    safeInc();
                    return;
                }
            }
        } else if (i + 1 < body.size()) {
            if (body[i + 1].type == tok_identifier) {
                goto stringLiteral_IdentifierAfter;
            }
        }
        append("(_stack.sp++)->s = _scl_create_string(\"%s\");\n", body[i].value.c_str());
        typeStack.push("str");
    }

    handler(CharStringLiteral) {
        noUnused;
        append("(_stack.sp++)->v = \"%s\";\n", body[i].value.c_str());
        typeStack.push("[int8]");
    }

    handler(CDecl) {
        noUnused;
        safeInc();
        if (body[i].type != tok_string_literal) {
            transpilerError("Expected string literal, but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        std::string s = replaceAll(body[i].value, R"(\\\\)", "\\");
        s = replaceAll(s, R"(\\\")", "\"");
        append("%s\n", s.c_str());
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
    }

    handler(ExternC) {
        noUnused;
        if (!isInUnsafe) {
            transpilerError("'c!' is only allowed in unsafe blocks", i);
            errors.push_back(err);
            return;
        }
        append("{// Start C\n");
        scopeDepth++;
        append("scl_int8* __prev = *(_stack.tp - 1);\n");
        append("*(_stack.tp - 1) = \"<%s:native code>\";\n", function->name.c_str());
        std::string file = body[i].file;
        if (strstarts(file, scaleFolder)) {
            file = file.substr(scaleFolder.size() + std::string("/Frameworks/").size());
        } else {
            file = std::filesystem::path(file).relative_path();
        }
        std::string ext = body[i].value;
        for (const Variable& v : vars) {
            append("%s* %s = &Var_%s;\n", sclTypeToCType(result, v.type).c_str(), v.name.c_str(), v.name.c_str());
        }
        append("{\n");
        scopeDepth++;
        std::vector<std::string> lines = split(ext, "\n");
        for (std::string& line : lines) {
            if (ltrim(line).size() == 0)
                continue;
            append("%s\n", ltrim(line).c_str());
        }
        scopeDepth--;
        append("}\n");
        append("*(_stack.tp - 1) = __prev;\n");
        scopeDepth--;
        append("}// End C\n");
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
    }

    handler(IntegerLiteral) {
        noUnused;
        std::string stringValue = body[i].value;

        // only parse to verify bounds and make very large integer literals defined behavior
        try {
            if (strstarts(stringValue, "0x")) {
                std::stoul(stringValue, nullptr, 16);
            } else if (strstarts(stringValue, "0b")) {
                std::stoul(stringValue, nullptr, 2);
            } else if (strstarts(stringValue, "0c")) {
                std::stoul(stringValue, nullptr, 8);
            } else {
                std::stol(stringValue);
            }
        } catch (const std::invalid_argument& e) {
            transpilerError("Invalid integer literal: '" + stringValue + "'", i);
            errors.push_back(err);
            return;
        } catch (const std::out_of_range& e) {
            transpilerError("Integer literal out of range: '" + stringValue + "' " + e.what(), i);
            errors.push_back(err);
            return;
        }
        append("(_stack.sp++)->i = %s;\n", stringValue.c_str());
        typeStack.push("int");
    }

    handler(FloatLiteral) {
        noUnused;

        // only parse to verify bounds and make very large float literals defined behavior
        try {
            std::stod(body[i].value);
        } catch (const std::invalid_argument& e) {
            transpilerError("Invalid float literal: '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        } catch (const std::out_of_range& e) {
            transpilerError("Integer literal out of range: '" + body[i].value + "' " + e.what(), i);
            errors.push_back(err);
            return;
        }
        append("(_stack.sp++)->f = %s;\n", body[i].value.c_str());
        typeStack.push("float");
    }

    handler(FalsyType) {
        noUnused;
        append("(_stack.sp++)->i = (scl_int) 0;\n");
        if (body[i].type == tok_nil) {
            typeStack.push("any");
        } else {
            typeStack.push("bool");
        }
    }

    handler(TruthyType) {
        noUnused;
        append("(_stack.sp++)->i = (scl_int) 1;\n");
        typeStack.push("bool");
    }

    handler(Is) {
        noUnused;
        safeInc();
        FPResult res = parseType(body, &i);
        if (!res.success) {
            errors.push_back(res);
            return;
        }
        std::string type = res.value;
        if (isPrimitiveType(type)) {
            append("(_stack.sp - 1)->i = 1;\n");
            typePop;
            typeStack.push("bool");
            return;
        }
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            append("(_stack.sp - 1)->i = _scl_is_array((_stack.sp - 1)->v);\n");
            typePop;
            typeStack.push("bool");
            return;
        }
        if (getStructByName(result, type) == Struct::Null) {
            transpilerError("Usage of undeclared struct '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        append("(_stack.sp - 1)->i = (_stack.sp - 1)->v && _scl_is_instance_of((_stack.sp - 1)->v, 0x%xU);\n", id((char*) type.c_str()));
        typePop;
        typeStack.push("bool");
    }

    handler(If) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        append("{\n");
        scopeDepth++;
        append("if (({\n");
        scopeDepth++;
        varScopePush();
        safeInc();
        while (body[i].type != tok_then) {
            handle(Token);
            safeInc();
        }
        safeInc();
        varScopePop();
        append("(--_stack.sp)->i;\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        typePop;
        varScopePush();
        auto tokenEndsIfBlock = [body](TokenType type) {
            return type == tok_elif ||
                   type == tok_elunless ||
                   type == tok_fi ||
                   type == tok_else;
        };
        bool exhaustive = false;
        auto beginningStackSize = typeStack.size();
        std::vector<std::string> returnedTypes;
    nextIfPart:
        while (!tokenEndsIfBlock(body[i].type)) {
            handle(Token);
            safeInc();
        }
        if (typeStackTop.size() > beginningStackSize) {
            returnedTypes.push_back(typeStackTop);
        }
        while (typeStack.size() > beginningStackSize) {
            typePop;
        }
        if (body[i].type != tok_fi) {
            if (body[i].type == tok_else) {
                exhaustive = true;
            }
            handle(Token);
            safeInc();
            goto nextIfPart;
        }
        while (typeStack.size() > beginningStackSize) {
            typePop;
        }
        if (exhaustive && returnedTypes.size() > 0) {
            std::string returnType = returnedTypes[0];
            for (std::string& type : returnedTypes) {
                if (type != returnType) {
                    transpilerError("Not every branch of this if-statement returns the same type! Types are: [ " + returnType + ", " + type + " ]", i);
                    errors.push_back(err);
                    return;
                }
            }
            typeStack.push(returnType);
        }
        handle(Fi);
    }

    handler(Unless) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        append("{\n");
        scopeDepth++;
        append("if (!({\n");
        scopeDepth++;
        varScopePush();
        safeInc();
        while (body[i].type != tok_then) {
            handle(Token);
            safeInc();
        }
        safeInc();
        varScopePop();
        append("(--_stack.sp)->i;\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        typePop;
        varScopePush();
        auto tokenEndsIfBlock = [body](TokenType type) {
            return type == tok_elif ||
                   type == tok_elunless ||
                   type == tok_fi ||
                   type == tok_else;
        };
        std::string invalidType = "---------";
        std::string ifBlockReturnType = invalidType;
        bool exhaustive = false;
        auto beginningStackSize = typeStack.size();
        std::string problematicType = "";
    nextIfPart:
        while (!tokenEndsIfBlock(body[i].type)) {
            handle(Token);
            safeInc();
        }
        if (ifBlockReturnType == invalidType && typeStackTop.size()) {
            ifBlockReturnType = typeStackTop;
        } else if (typeStackTop.size() && ifBlockReturnType != typeStackTop) {
            problematicType = "[ " + ifBlockReturnType + ", " + typeStackTop + " ]";
            ifBlockReturnType = "---";
        }
        if (body[i].type != tok_fi) {
            handle(Token);
        }
        if (body[i].type == tok_else) {
            exhaustive = true;
        }
        while (typeStack.size() > beginningStackSize) {
            typePop;
        }
        if (body[i].type != tok_fi) {
            safeInc();
            goto nextIfPart;
        }
        if (ifBlockReturnType == "---") {
            transpilerError("Not every branch of this if-statement returns the same type! Types are: " + problematicType, i);
            errors.push_back(err);
            return;
        }
        bool sizeDecreased = false;
        while (typeStack.size() > beginningStackSize) {
            typePop;
            sizeDecreased = true;
        }
        if (exhaustive && sizeDecreased) {
            typeStack.push(ifBlockReturnType);
        }
        handle(Fi);
    }

    handler(Else) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        varScopePop();
        scopeDepth--;
        append("} else {\n");
        scopeDepth++;
        varScopePush();
    }

    handler(Elif) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        varScopePop();
        scopeDepth--;
        
        append("} else if (({\n");
        scopeDepth++;
        safeInc();
        while (body[i].type != tok_then) {
            handle(Token);
            safeInc();
        }
        append("(--_stack.sp)->i;\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        varScopePush();
    }

    handler(Elunless) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        varScopePop();
        scopeDepth--;
        
        append("} else if (!({\n");
        scopeDepth++;
        safeInc();
        while (body[i].type != tok_then) {
            handle(Token);
            safeInc();
        }
        append("(--_stack.sp)->i;\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        varScopePush();
    }

    handler(Fi) {
        noUnused;
        varScopePop();
        scopeDepth--;
        append("}\n");
        scopeDepth--;
        append("}\n");
        condCount--;
    }

    handler(While) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        append("while (({\n");
        scopeDepth++;
        safeInc();
        while (body[i].type != tok_do) {
            handle(Token);
            safeInc();
        }
        append("(--_stack.sp)->i;\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        pushOther();
        varScopePush();
    }

    handler(Do) {
        noUnused;
        typePop;
        append("if (!(--_stack.sp)->v) break;\n");
    }

    handler(DoneLike) {
        noUnused;
        varScopePop();
        scopeDepth--;
        if (wasCatch()) {
            append("} else {\n");
            if (function->has_restrict) {
                if (function->isMethod) {
                    append("  Function_Process$unlock((scl_SclObject) Var_self);\n");
                } else {
                    append("  Function_Process$unlock(function_lock$%s);\n", function->finalName().c_str());
                }
            }
            append("  Function_throw(*(_stack.ex));\n");
        }
        append("}\n");
        if (repeat_depth > 0 && wasRepeat()) {
            repeat_depth--;
        }
        popOther();
    }

    handler(Return) {
        noUnused;

        if (function->return_type == "nothing") {
            transpilerError("Cannot return from a function with return type 'nothing'", i);
            errors.push_back(err);
            return;
        }

        append("{\n");
        scopeDepth++;

        if (function->return_type != "none" && !function->namedReturnValue.name.size()) {
            append("_scl_frame_t* returnFrame = (--_stack.sp);\n");
        }
        if (function->has_restrict) {
            if (function->isMethod) {
                append("Function_Process$unlock((scl_SclObject) Var_self);\n");
            } else {
                append("Function_Process$unlock(function_lock$%s);\n", function->finalName().c_str());
            }
        }
        if (function->namedReturnValue.name.size()) {
            if (function->namedReturnValue.typeFromTemplate.size()) {
                append("_scl_checked_cast(*(scl_any*) &Var_%s, Var_self->$template_arg_%s, Var_self->$template_argname_%s);\n", function->namedReturnValue.name.c_str(), function->namedReturnValue.typeFromTemplate.c_str(), function->namedReturnValue.typeFromTemplate.c_str());
            }
        } else {
            if (function->templateArg.size()) {
                append("_scl_checked_cast(returnFrame->v, Var_self->$template_arg_%s, Var_self->$template_argname_%s);\n", function->templateArg.c_str(), function->templateArg.c_str());
            }
        }
        append("--_stack.tp;\n");
        append("_stack.sp = __current_base_ptr;\n");

        if (function->return_type != "none" && function->return_type != "nothing") {
            if (!typeCanBeNil(function->return_type)) {
                if (function->namedReturnValue.name.size()) {
                    if (typeCanBeNil(function->namedReturnValue.type)) {
                        transpilerError("Returning maybe-nil type '" + function->namedReturnValue.type + "' from function with not-nil return type '" + function->return_type + "'", i);
                        errors.push_back(err);
                        return;
                    }
                } else {
                    if (typeCanBeNil(typeStackTop)) {
                        transpilerError("Returning maybe-nil type '" + typeStackTop + "' from function with not-nil return type '" + function->return_type + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    if (!typesCompatible(result, typeStackTop, function->return_type, true)) {
                        transpilerError("Returning type '" + typeStackTop + "' from function with return type '" + function->return_type + "'", i);
                        errors.push_back(err);
                        return;
                    }
                }
                if (function->namedReturnValue.name.size())
                    append("_scl_not_nil_return(*(scl_int*) &Var_%s, \"%s\");\n", function->namedReturnValue.name.c_str(), function->return_type.c_str());
                else
                    append("_scl_not_nil_return(returnFrame->i, \"%s\");\n", function->return_type.c_str());
            }
        }

        auto type = removeTypeModifiers(function->return_type);

        if (type == "none")
            append("return;\n");
        else {
            if (function->namedReturnValue.name.size()) {
                std::string type = sclTypeToCType(result, function->namedReturnValue.type);
                append("return (%s) Var_%s;\n", type.c_str(), function->namedReturnValue.name.c_str());
            } else {
                if (type == "str") {
                    append("return returnFrame->s;\n");
                } else if (type == "int") {
                    append("return returnFrame->i;\n");
                } else if (type == "float") {
                    append("return returnFrame->f;\n");
                } else if (type == "any") {
                    append("return returnFrame->v;\n");
                } else {
                    append("return (%s) returnFrame->i;\n", sclTypeToCType(result, function->return_type).c_str());
                }
            }
        }
        scopeDepth--;
        append("}\n");
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
    }

    handler(Goto) {
        noUnused;
        safeInc();
        if (body[i].type != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
            errors.push_back(err);
        }
        append("goto %s;\n", body[i].value.c_str());
    }

    handler(Label) {
        noUnused;
        safeInc();
        if (body[i].type != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
            errors.push_back(err);
        }
        append("%s:\n", body[i].value.c_str());
    }

    handler(Case) {
        noUnused;
        safeInc();
        if (!wasSwitch()) {
            transpilerError("Unexpected 'case' statement", i);
            errors.push_back(err);
            return;
        }
        if (body[i].type == tok_string_literal) {
            transpilerError("String literal in case statement is not supported", i);
            errors.push_back(err);
        } else {
            append("case %s: {\n", body[i].value.c_str());
            scopeDepth++;
            varScopePush();
        }
    }

    handler(Esac) {
        noUnused;
        if (!wasSwitch()) {
            transpilerError("Unexpected 'esac'", i);
            errors.push_back(err);
            return;
        }
        append("break;\n");
        varScopePop();
        scopeDepth--;
        append("}\n");
    }

    handler(Default) {
        noUnused;
        if (!wasSwitch()) {
            transpilerError("Unexpected 'default' statement", i);
            errors.push_back(err);
            return;
        }
        append("default: {\n");
        scopeDepth++;
        varScopePush();
    }

    handler(Switch) {
        noUnused;
        typePop;
        append("switch ((--_stack.sp)->i) {\n");
        scopeDepth++;
        varScopePush();
        pushSwitch();
    }

    handler(AddrOf) {
        noUnused;
        if (handleOverriddenOperator(result, fp, scopeDepth, "@", typeStackTop)) return;
        append("_scl_nil_ptr_dereference((_stack.sp - 1)->i, NULL);\n");
        append("(_stack.sp - 1)->v = (*(scl_any*) (_stack.sp - 1)->v);\n");
        std::string ptr = removeTypeModifiers(typeStackTop);
        typePop;
        if (ptr.size()) {
            if (ptr.front() == '[' && ptr.back() == ']') {
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
        s = removeTypeModifiers(s);
        return s == "int" ||
               s == "int64" ||
               s == "int32" ||
               s == "int16" ||
               s == "int8" ||
               s == "uint" ||
               s == "uint64" ||
               s == "uint32" ||
               s == "uint16" ||
               s == "uint8";
    }

    handler(As) {
        noUnused;
        safeInc();
        FPResult type = parseType(body, &i, getTemplates(result, function));
        if (!type.success) {
            errors.push_back(type);
            return;
        }
        if (type.value == "float" && typeStackTop != "float") {
            append("(_stack.sp - 1)->f = (float) (_stack.sp - 1)->i;\n");
            typePop;
            typeStack.push("float");
            return;
        }
        if ((type.value == "int" && typeStackTop != "float") || type.value == "float" || type.value == "any") {
            typePop;
            typeStack.push(type.value);
            return;
        }
        if (type.value == "int") {
            append("(_stack.sp - 1)->i = (scl_int) (_stack.sp - 1)->f;\n");
            typePop;
            typeStack.push("int");
            return;
        }
        if (type.value == "lambda" || strstarts(type.value, "lambda(")) {
            typePop;
            append("_scl_not_nil_cast((_stack.sp - 1)->i, \"%s\");\n", type.value.c_str());
            typeStack.push(type.value);
            return;
        }
        if (isPrimitiveIntegerType(type.value)) {
            if (typeStackTop == "float") {
                append("(_stack.sp - 1)->i = (scl_int) (_stack.sp - 1)->f;\n");
            }
            typePop;
            typeStack.push(type.value);
            append("(_stack.sp - 1)->i &= SCL_%s_MAX;\n", type.value.c_str());
            return;
        }
        auto typeIsPtr = [type]() -> bool {
            std::string t = type.value;
            if (t.size() == 0) return false;
            t = removeTypeModifiers(t);
            t = notNilTypeOf(t);
            return (t.front() == '[' && t.back() == ']');
        };
        if (hasTypealias(result, type.value) || typeIsPtr()) {
            typePop;
            typeStack.push(type.value);
            return;
        }
        if (hasLayout(result, type.value)) {
            append("_scl_check_layout_size((_stack.sp - 1)->v, sizeof(struct Layout_%s), \"%s\");\n", type.value.c_str(), type.value.c_str());
            typePop;
            typeStack.push(type.value);
            return;
        }
        if (getStructByName(result, type.value) == Struct::Null) {
            transpilerError("Use of undeclared Struct '" + type.value + "'", i);
            errors.push_back(err);
            return;
        }
        if (i + 1 < body.size() && body[i + 1].value == "?") {
            if (!typeCanBeNil(typeStackTop)) {
                transpilerError("Unneccessary cast to maybe-nil type '" + type.value + "?' from not-nil type '" + typeStackTop + "'", i - 1);
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
            }
            typePop;
            typeStack.push(type.value + "?");
            safeInc();
        } else {
            if (typeCanBeNil(typeStackTop)) {
                append("_scl_not_nil_cast((_stack.sp - 1)->i, \"%s\");\n", type.value.c_str());
            }
            std::string typeStr = removeTypeModifiers(type.value);
            if (getStructByName(result, typeStr) != Struct::Null) {
                append("_scl_checked_cast((_stack.sp - 1)->v, %d, \"%s\");\n", id((char*) typeStr.c_str()), typeStr.c_str());
            }
            typePop;
            typeStack.push(type.value);
        }
    }

    handler(Dot) {
        noUnused;
        std::string type = typeStackTop;
        typePop;

        if (hasLayout(result, type)) {
            Layout l = getLayout(result, type);
            safeInc();
            std::string member = body[i].value;
            if (!l.hasMember(member)) {
                transpilerError("No layout for '" + member + "' in '" + type + "'", i);
                errors.push_back(err);
                return;
            }
            const Variable& v = l.getMember(member);

            if (removeTypeModifiers(v.type) == "float") {
                append("(_stack.sp - 1)->f = ((%s) (_stack.sp - 1)->i)->%s;\n", sclTypeToCType(result, l.name).c_str(), member.c_str());
            } else {
                append("(_stack.sp - 1)->i = (scl_int) ((%s) (_stack.sp - 1)->i)->%s;\n", sclTypeToCType(result, l.name).c_str(), member.c_str());
            }
            typeStack.push(l.getMember(member).type);
            return;
        }
        std::string tmp = removeTypeModifiers(type);
        if (tmp.size() > 2 && tmp.front() == '[' && tmp.back() == ']') {
            safeInc();
            if (body[i].type != tok_identifier) {
                transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            std::string member = body[i].value;
            if (member == "count") {
                append("(_stack.sp - 1)->i = _scl_array_size((_stack.sp - 1)->v);\n");
                typeStack.push("int");
            } else {
                transpilerError("Unknown array member '" + member + "'", i);
                errors.push_back(err);
            }
            return;
        }
        Struct s = getStructByName(result, type);
        if (s == Struct::Null) {
            transpilerError("Cannot infer type of stack top: expected valid Struct, but got '" + type + "'", i);
            errors.push_back(err);
            return;
        }
        Token dot = body[i];
        bool deref = body[i + 1].type == tok_addr_of;
        safeIncN(deref + 1);
        if (!s.hasMember(body[i].value)) {
            if (s.name == "Map" || s.super == "Map") {
                Method* f = getMethodByName(result, "get", s.name);
                if (!f) {
                    transpilerError("Could not find method 'get' on struct '" + s.name + "'", i - 1);
                    errors.push_back(err);
                    return;
                }
                
                append("{\n");
                scopeDepth++;
                append("scl_any tmp = (--_stack.sp)->v;\n");
                std::string t = type;
                append("(_stack.sp++)->s = _scl_create_string(\"%s\");\n", body[i].value.c_str());
                typeStack.push("str");
                append("(_stack.sp++)->v = tmp;\n");
                typeStack.push(t);
                scopeDepth--;
                append("}\n");
                methodCall(f, fp, result, warns, errors, body, i);
                return;
            }
            std::string help = "";
            Method* m;
            if ((m = getMethodByName(result, body[i].value, s.name)) != nullptr) {
                std::string lambdaType = "lambda(" + std::to_string(m->args.size()) + "):" + m->return_type;
                append("(_stack.sp++)->v = Method_%s$%s;\n", s.name.c_str(), m->name.c_str());
                typeStack.push(lambdaType);
                return;
            }
            transpilerError("Struct '" + s.name + "' has no member named '" + body[i].value + "'" + help, i);
            errors.push_back(err);
            return;
        }
        Variable mem = s.getMember(body[i].value);
        if (!mem.isAccessible(currentFunction)) {
            if (s.name != function->member_type) {
                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                errors.push_back(err);
                return;
            }
        }
        if (typeCanBeNil(type) && dot.value != "?.") {
            {
                transpilerError("Member access on maybe-nil type '" + type + "'", i);
                errors.push_back(err);
            }
            transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this '.'\\insertText;!!;" + std::to_string(err.line) + ":" + std::to_string(err.column), i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }

        Method* m = attributeAccessor(result, s.name, mem.name);
        if (m) {
            if (dot.value == "?.") {
                if (deref) {
                    append("*(%s*) (_stack.sp - 1) = (_stack.sp - 1)->i ? *(Method_%s$%s(*(%s*) (_stack.sp - 1))) : 0%s;\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str(), removeTypeModifiers(m->return_type) == "float" ? ".0" : "");
                    std::string type = m->return_type;
                    if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                        type = type.substr(1, type.size() - 2);
                    } else {
                        type = "any";
                    }
                    typeStack.push(type);
                } else {
                    append("*(%s*) (_stack.sp - 1) = (_stack.sp - 1)->i ? Method_%s$%s(*(%s*) (_stack.sp - 1)) : 0%s;\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str(), removeTypeModifiers(m->return_type) == "float" ? ".0" : "");
                    typeStack.push(m->return_type);
                }
            } else {
                if (deref) {
                    append("*(%s*) (_stack.sp - 1) = *(Method_%s$%s(*(%s*) (_stack.sp - 1)));\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str());
                    std::string type = m->return_type;
                    if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                        type = type.substr(1, type.size() - 2);
                    } else {
                        type = "any";
                    }
                    typeStack.push(type);
                } else {
                    append("*(%s*) (_stack.sp - 1) = Method_%s$%s(*(%s*) (_stack.sp - 1));\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str());
                    typeStack.push(m->return_type);
                }
            }
            return;
        }

        if (dot.value == "?.") {
            if (removeTypeModifiers(mem.type) == "float") {
                append("(_stack.sp - 1)->f = (_stack.sp - 1)->i ? ((%s) (_stack.sp - 1)->v)->%s : 0.0f;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            } else {
                append("(_stack.sp - 1)->v = (_stack.sp - 1)->i ? (scl_any) ((%s) (_stack.sp - 1)->v)->%s : NULL;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            }
        } else {
            if (removeTypeModifiers(mem.type) == "float") {
                append("(_stack.sp - 1)->f = ((%s) (_stack.sp - 1)->v)->%s;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            } else {
                append("(_stack.sp - 1)->v = (scl_any) ((%s) (_stack.sp - 1)->v)->%s;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            }
        }
        typeStack.push(mem.type);
        if (deref) {
            std::string path = dot.value + "@" + body[i].value;
            append("_scl_nil_ptr_dereference((_stack.sp - 1)->i, \"%s\");\n", path.c_str());
            append("(_stack.sp - 1)->v = *(scl_any*) (_stack.sp - 1)->v;\n");
            std::string type = typeStackTop;
            typePop;
            if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                type = type.substr(1, type.size() - 2);
            } else {
                type = "any";
            }
            typeStack.push(type);
        }
    }

    handler(ColumnOnInterface) {
        noUnused;
        std::string type = typeStackTop;
        Interface* interface = getInterfaceByName(result, type);
        safeInc();
        Method* m = getMethodByNameOnThisType(result, body[i].value, interface->name);
        if (!m) {
            transpilerError("Interface '" + interface->name + "' has no member named '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        if (typeCanBeNil(type)) {
            {
                transpilerError("Calling method on maybe-nil type '" + type + "'", i);
                errors.push_back(err);
            }
            transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this ':'\\insertText;!!;" + std::to_string(err.line) + ":" + std::to_string(err.column), i - 1);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        methodCall(m, fp, result, warns, errors, body, i);
    }

    handler(Column) {
        noUnused;
        if (strstarts(typeStackTop, "lambda(") || typeStackTop == "lambda") {
            safeInc();
            if (typeStackTop == "lambda") {
                transpilerError("Generic lambda has to be cast to a typed lambda before calling", i);
                errors.push_back(err);
                return;
            }
            std::string returnType = typeStackTop.substr(typeStackTop.find_last_of("):") + 1);
            std::string op = body[i].value;

            std::string args = typeStackTop.substr(typeStackTop.find_first_of("(") + 1, typeStackTop.find_last_of("):") - typeStackTop.find_first_of("(") - 1);
            size_t argAmount = std::stoi(args);
            
            std::string argTypes = "";
            std::string argGet = "";
            for (size_t argc = argAmount; argc; argc--) {
                argTypes += "scl_any";
                argGet += "_scl_positive_offset(" + std::to_string(argAmount - argc) + ")->v";
                if (argc > 1) {
                    argTypes += ", ";
                    argGet += ", ";
                }
                typePop;
            }

            typePop;
            if (op == "accept") {
                append("{\n");
                scopeDepth++;
                if (removeTypeModifiers(returnType) == "none" || removeTypeModifiers(returnType) == "nothing") {
                    append("void(*lambda)(%s) = (--_stack.sp)->v;\n", argTypes.c_str());
                    append("_stack.sp -= (%zu);\n", argAmount);
                    append("lambda(%s);\n", argGet.c_str());
                } else if (removeTypeModifiers(returnType) == "float") {
                    append("scl_float(*lambda)(%s) = (--_stack.sp)->v;\n", argTypes.c_str());
                    append("_stack.sp -= (%zu);\n", argAmount);
                    append("(_stack.sp++)->f = lambda(%s);\n", argGet.c_str());
                    typeStack.push(returnType);
                } else {
                    append("scl_any(*lambda)(%s) = (--_stack.sp)->v;\n", argTypes.c_str());
                    append("_stack.sp -= (%zu);\n", argAmount);
                    append("(_stack.sp++)->v = lambda(%s);\n", argGet.c_str());
                    typeStack.push(returnType);
                }
                scopeDepth--;
                append("}\n");
            } else {
                transpilerError("Unknown method '" + body[i].value + "' on type 'lambda'", i);
                errors.push_back(err);
            }
            return;
        }
        std::string type = typeStackTop;
        std::string tmp = removeTypeModifiers(type);
        if (tmp.size() > 2 && tmp.front() == '[' && tmp.back() == ']') {
            safeInc();
            if (body[i].type != tok_identifier) {
                transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            std::string op = body[i].value;
            if (op == "resize") {
                append("{\n");
                scopeDepth++;
                append("scl_any* array = (--_stack.sp)->v;\n");
                typePop;
                append("scl_int size = (--_stack.sp)->i;\n");
                typePop;
                append("array = _scl_array_resize(array, size);\n");
                append("(_stack.sp++)->v = array;\n");
                typeStack.push(type);
                scopeDepth--;
                append("}\n");
            } else if (op == "sort") {
                append("(_stack.sp - 1)->v = _scl_array_sort((_stack.sp - 1)->v);\n");
                typeStack.push(type);
            } else if (op == "reverse") {
                append("(_stack.sp - 1)->v = _scl_array_reverse((_stack.sp - 1)->v);\n");
                typeStack.push(type);
            } else if (op == "toString") {
                append("(_stack.sp - 1)->s = _scl_array_to_string((_stack.sp - 1)->v);\n");
                typeStack.push("str");
            } else {
                transpilerError("Unknown method '" + body[i].value + "' on type '" + type + "'", i);
                errors.push_back(err);
            }
            return;
        }
        Struct s = getStructByName(result, type);
        if (s == Struct::Null) {
            if (getInterfaceByName(result, type) != nullptr) {
                handle(ColumnOnInterface);
                return;
            }
            if (isPrimitiveIntegerType(type)) {
                s = getStructByName(result, "int");
            } else {
                transpilerError("Cannot infer type of stack top: expected valid Struct, but got '" + type + "'", i);
                errors.push_back(err);
                return;
            }
        }
        bool onSuper = function->isMethod && body[i - 1].type == tok_identifier && body[i - 1].value == "super";
        safeInc();
        if (!s.isStatic() && !hasMethod(result, body[i].value, removeTypeModifiers(type))) {
            std::string help = "";
            if (s.hasMember(body[i].value)) {
                help = ". Maybe you meant to use '.' instead of ':' here";

                const Variable& v = s.getMember(body[i].value);
                std::string type = v.type;
                if (!strstarts(removeTypeModifiers(type), "lambda(")) {
                    goto notLambdaType;
                }

                std::string returnType = lambdaReturnType(type);
                size_t argAmount = lambdaArgCount(type);


                append("{\n");
                scopeDepth++;

                append("%s tmp = (--_stack.sp)->v;\n", sclTypeToCType(result, s.name).c_str());
                typePop;

                if (typeStack.size() < argAmount) {
                    transpilerError("Arguments for lambda '" + s.name + ":" + v.name + "' do not equal inferred stack", i);
                    errors.push_back(err);
                    return;
                }

                std::string argTypes = "";
                std::string argGet = "";
                for (size_t argc = argAmount; argc; argc--) {
                    argTypes += "scl_any";
                    argGet += "_scl_positive_offset(" + std::to_string(argAmount - argc) + ")->v";
                    if (argc > 1) {
                        argTypes += ", ";
                        argGet += ", ";
                    }
                    typePop;
                }
                if (removeTypeModifiers(returnType) == "none" || removeTypeModifiers(returnType) == "nothing") {
                    append("void(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.name.c_str());
                    append("_stack.sp -= (%zu);\n", argAmount);
                    append("lambda(%s);\n", argGet.c_str());
                } else if (removeTypeModifiers(returnType) == "float") {
                    append("scl_float(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.name.c_str());
                    append("_stack.sp -= (%zu);\n", argAmount);
                    append("(_stack.sp++)->f = lambda(%s);\n", argGet.c_str());
                    typeStack.push(returnType);
                } else {
                    append("scl_any(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.name.c_str());
                    append("_stack.sp -= (%zu);\n", argAmount);
                    append("(_stack.sp++)->v = lambda(%s);\n", argGet.c_str());
                    typeStack.push(returnType);
                }
                scopeDepth--;
                append("}\n");
                return;
            }
        notLambdaType:
            transpilerError("Unknown method '" + body[i].value + "' on type '" + removeTypeModifiers(type) + "'" + help, i);
            errors.push_back(err);
            return;
        } else if (s.isStatic() && s.name != "any" && s.name != "int" && !hasFunction(result, removeTypeModifiers(type) + "$" + body[i].value)) {
            transpilerError("Unknown static function '" + body[i].value + "' on type '" + removeTypeModifiers(type) + "'", i);
            errors.push_back(err);
            return;
        }
        if (s.name == "any" || s.name == "int") {
            Function* anyMethod = getFunctionByName(result, s.name + "$" + body[i].value);
            if (!anyMethod) {
                if (s.name == "any") {
                    anyMethod = getFunctionByName(result, "int$" + body[i].value);
                } else {
                    anyMethod = getFunctionByName(result, "any$" + body[i].value);
                }
            }
            if (anyMethod) {
                Method* objMethod = getMethodByName(result, body[i].value, "SclObject");
                if (objMethod) {
                    append("if (_scl_is_instance_of((_stack.sp - 1)->v, 0x%xU)) {\n", id((char*) "SclObject"));
                    scopeDepth++;
                    methodCall(objMethod, fp, result, warns, errors, body, i, true, false);
                    if (objMethod->return_type != "none" && objMethod->return_type != "nothing") {
                        typeStack.pop();
                    }
                    scopeDepth--;
                    append("} else {\n");
                    scopeDepth++;
                }
                functionCall(anyMethod, fp, result, warns, errors, body, i);
                if (objMethod) {
                    scopeDepth--;
                    append("}\n");
                }
            } else {
                transpilerError("Unknown static function '" + body[i].value + "' on type '" + type + "'", i);
                errors.push_back(err);
            }
            return;
        }
        if (s.isStatic()) {
            Function* f = getFunctionByName(result, removeTypeModifiers(type) + "$" + body[i].value);
            functionCall(f, fp, result, warns, errors, body, i);
        } else {
            if (typeCanBeNil(type)) {
                {
                    transpilerError("Calling method on maybe-nil type '" + type + "'", i);
                    errors.push_back(err);
                }
                transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this ':'\\insertText;!!;" + std::to_string(err.line) + ":" + std::to_string(err.column), i - 1);
                err.isNote = true;
                errors.push_back(err);
                return;
            }
            if (removeTypeModifiers(type) == "str" && body[i].value == "toString") {
                return;
            }
            if (removeTypeModifiers(type) == "str" && body[i].value == "view") {
                append("(_stack.sp - 1)->v = (_stack.sp - 1)->s->data;\n");
                typePop;
                typeStack.push("[int8]");
                return;
            }
            Method* f = getMethodByName(result, body[i].value, s.name);
            if (f->isPrivate && (!function->isMethod || ((Method*) function)->member_type != s.name)) {
                if (f->member_type != function->member_type) {
                    transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                    errors.push_back(err);
                    return;
                }
            }
            methodCall(f, fp, result, warns, errors, body, i, false, true, false, onSuper);
        }
    }

    handler(INVALID) {
        noUnused;
        if (i < body.size() && ((ssize_t) i) >= 0) {
            transpilerError("Unknown token: '" + body[i].value + "'", i);
            errors.push_back(err);
        } else {
            transpilerError("Unknown token: '" + body.back().value + "'", i);
            errors.push_back(err);
        }
    }

    void (*handleRefs[tok_MAX])(std::vector<Token>&, Function*, std::vector<FPResult>&, std::vector<FPResult>&, FILE*, TPResult&);

    void populateTokenJumpTable() {
        for (int i = 0; i < tok_MAX; i++) {
            handleRefs[i] = handlerRef(INVALID);
        }

        handleRefs[tok_question_mark] = handlerRef(Identifier);
        handleRefs[tok_identifier] = handlerRef(Identifier);
        handleRefs[tok_dot] = handlerRef(Dot);
        handleRefs[tok_column] = handlerRef(Column);
        handleRefs[tok_as] = handlerRef(As);
        handleRefs[tok_string_literal] = handlerRef(StringLiteral);
        handleRefs[tok_char_string_literal] = handlerRef(CharStringLiteral);
        handleRefs[tok_cdecl] = handlerRef(CDecl);
        handleRefs[tok_extern_c] = handlerRef(ExternC);
        handleRefs[tok_number] = handlerRef(IntegerLiteral);
        handleRefs[tok_char_literal] = handlerRef(IntegerLiteral);
        handleRefs[tok_number_float] = handlerRef(FloatLiteral);
        handleRefs[tok_nil] = handlerRef(FalsyType);
        handleRefs[tok_false] = handlerRef(FalsyType);
        handleRefs[tok_true] = handlerRef(TruthyType);
        handleRefs[tok_new] = handlerRef(New);
        handleRefs[tok_is] = handlerRef(Is);
        handleRefs[tok_if] = handlerRef(If);
        handleRefs[tok_unless] = handlerRef(Unless);
        handleRefs[tok_else] = handlerRef(Else);
        handleRefs[tok_elif] = handlerRef(Elif);
        handleRefs[tok_elunless] = handlerRef(Elunless);
        handleRefs[tok_while] = handlerRef(While);
        handleRefs[tok_do] = handlerRef(Do);
        handleRefs[tok_repeat] = handlerRef(Repeat);
        handleRefs[tok_for] = handlerRef(For);
        handleRefs[tok_foreach] = handlerRef(Foreach);
        handleRefs[tok_fi] = handlerRef(Fi);
        handleRefs[tok_done] = handlerRef(DoneLike);
        handleRefs[tok_end] = handlerRef(DoneLike);
        handleRefs[tok_return] = handlerRef(Return);
        handleRefs[tok_addr_ref] = handlerRef(AddrRef);
        handleRefs[tok_store] = handlerRef(Store);
        handleRefs[tok_declare] = handlerRef(Declare);
        handleRefs[tok_continue] = handlerRef(Continue);
        handleRefs[tok_break] = handlerRef(Break);
        handleRefs[tok_goto] = handlerRef(Goto);
        handleRefs[tok_label] = handlerRef(Label);
        handleRefs[tok_case] = handlerRef(Case);
        handleRefs[tok_esac] = handlerRef(Esac);
        handleRefs[tok_default] = handlerRef(Default);
        handleRefs[tok_switch] = handlerRef(Switch);
        handleRefs[tok_curly_open] = handlerRef(CurlyOpen);
        handleRefs[tok_bracket_open] = handlerRef(BracketOpen);
        handleRefs[tok_paren_open] = handlerRef(ParenOpen);
        handleRefs[tok_addr_of] = handlerRef(AddrOf);
    }

    handler(Token) {
        noUnused;

        auto tokenStart = std::chrono::high_resolution_clock::now();

        // std::cout << body[i].tostring() << std::endl;

        // debugDump(hasVar(body[i].value));

        handleRef(handleRefs[body[i].type]);

        // std::cout << "Stack: " << stackSliceToString(typeStack.size()) << std::endl;

        auto tokenEnd = std::chrono::high_resolution_clock::now();
        Main.tokenHandleTime += std::chrono::duration_cast<std::chrono::microseconds>(tokenEnd - tokenStart).count();
    }

    std::string sclFunctionNameToFriendlyString(std::string name) {
        name = name.substr(0, name.find("$$ol"));
        if (name == "operator$add") name = "+";
        else if (name == "operator$sub") name = "-";
        else if (name == "operator$mul") name = "*";
        else if (name == "operator$div") name = "/";
        else if (name == "operator$mod") name = "%";
        else if (name == "operator$logic_and") name = "&";
        else if (name == "operator$logic_or") name = "|";
        else if (name == "operator$logic_xor") name = "^";
        else if (name == "operator$logic_not") name = "~";
        else if (name == "operator$logic_lsh") name = "<<";
        else if (name == "operator$logic_rol") name = "<<<";
        else if (name == "operator$logic_rsh") name = ">>";
        else if (name == "operator$logic_ror") name = ">>>";
        else if (name == "operator$pow") name = "**";
        else if (name == "operator$dot") name = ".";
        else if (name == "operator$less") name = "<";
        else if (name == "operator$less_equal") name = "<=";
        else if (name == "operator$more") name = ">";
        else if (name == "operator$more_equal") name = ">=";
        else if (name == "operator$equal") name = "==";
        else if (name == "operator$not") name = "!";
        else if (name == "operator$assert_not_nil") name = "!!";
        else if (name == "operator$not_equal") name = "!=";
        else if (name == "operator$bool_and") name = "&&";
        else if (name == "operator$bool_or") name = "||";
        else if (name == "operator$inc") name = "++";
        else if (name == "operator$dec") name = "--";
        else if (name == "operator$at") name = "@";
        else if (name == "operator$store") name = "=>";
        else if (strcontains(name, "operator$store")) name = replaceAll(name, "operator\\$store", "=>");
        else if (name == "operator$set") name = "=>[]";
        else if (name == "operator$get") name = "[]";
        else if (name == "operator$wildcard") name = "?";
        name = replaceAll(name, "\\$\\d+", "");
        name = replaceAll(name, "\\$", "::");

        if (strstarts(name, "::lambda")) {
            name = "<lambda" + name.substr(8, name.find_first_of("(") - 8) + ">";
        }

        return name;
    }

    std::string sclFunctionNameToFriendlyString(Function* f) {
        std::string name = f->finalName();
        name = sclFunctionNameToFriendlyString(name);
        if (f->isMethod) {
            name = f->member_type + ":" + name;
        }
        name += "(";
        for (size_t i = 0; i < f->args.size(); i++) {
            const Variable& v = f->args[i];
            if (f->isMethod && v.name == "self") continue;
            if (i) name += ", ";
            name += v.name + ": " + v.type;
        }
        name += "): " + f->return_type;
        return name;
    }

    std::string sclGenCastForMethod(TPResult& result, Method* m) {
        std::string return_type = sclTypeToCType(result, m->return_type);
        std::string arguments = "";
        if (m->args.size() > 0) {
            for (ssize_t i = (ssize_t) m->args.size() - 1; i >= 0; i--) {
                const Variable& var = m->args[i];
                arguments += sclTypeToCType(result, var.type);
                if (i) {
                    arguments += ", ";
                }
            }
        }
        return return_type + "(*)(" + arguments + ")";
    }

    std::string rtTypeToSclType(std::string rtType) {
        if (rtType.size() == 0) return "";
        if (rtType == "a") return "any";
        if (rtType == "i") return "int";
        if (rtType == "f") return "float";
        if (rtType == "s") return "str";
        if (rtType == "V") return "none";
        if (rtType == "cs") return "[int8]";
        if (rtType == "p") return "[any]";
        if (rtType == "F") return "lambda";
        if (rtType.front() == '[') {
            return "[" + rtTypeToSclType(rtType.substr(1, rtType.size() - 1)) + "]";
        }
        if (rtType == "u") return "uint";
        if (rtType == "i8") return "int8";
        if (rtType == "u8") return "uint8";
        if (rtType == "i16") return "int16";
        if (rtType == "u16") return "uint16";
        if (rtType == "i32") return "int32";
        if (rtType == "u32") return "uint32";
        if (rtType == "i64") return "int64";
        if (rtType == "u64") return "uint64";
        if (rtType.front() == 'L') {
            return rtType.substr(1, rtType.size() - 1);
        }
        return "<" + rtType + ">";
    }

    std::string typeToRTSig(std::string type) {
        type = removeTypeModifiers(type);
        if (type == "any") return "a;";
        if (type == "int" || type == "bool") return "i;";
        if (type == "float") return "f;";
        if (type == "str") return "s;";
        if (type == "none") return "V;";
        if (type == "[int8]") return "cs;";
        if (type == "[any]") return "p;";
        if (type == "lambda" || strstarts(type, "lambda(")) return "F;";
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            return "[" + typeToRTSig(type.substr(1, type.size() - 2));
        }
        if (type == "uint") return "u;";
        if (type == "int8") return "i8;";
        if (type == "int16") return "i16;";
        if (type == "int32") return "i32;";
        if (type == "int64") return "i64;";
        if (type == "uint8") return "u8;";
        if (type == "uint16") return "u16;";
        if (type == "uint32") return "u32;";
        if (type == "uint64") return "u64;";
        if (currentStruct.templates.find(type) != currentStruct.templates.end()) {
            return typeToRTSig(currentStruct.templates[type]);
        }
        return "L" + type + ";";
    }

    std::string argsToRTSignature(Function* f) {
        std::string args = "(";
        for (const Variable& v : f->args) {
            std::string type = removeTypeModifiers(v.type);
            if (v.name == "self" && f->isMethod) {
                continue;
            }
            args += typeToRTSig(type);
        }
        args += ")";
        args += typeToRTSig(f->return_type);
        return args;
    }

    std::string typeToRTSigIdent(std::string type) {
        type = removeTypeModifiers(type);
        if (type == "any") return "a$";
        if (type == "int" || type == "bool") return "i$";
        if (type == "float") return "f$";
        if (type == "str") return "s$";
        if (type == "none") return "V$";
        if (type == "[int8]") return "cs$";
        if (type == "[any]") return "p$";
        if (type == "lambda" || strstarts(type, "lambda(")) return "F$";
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            return "A" + typeToRTSigIdent(type.substr(1, type.size() - 2));
        }
        if (type == "uint") return "u$";
        if (type == "int8") return "i8$";
        if (type == "int16") return "i16$";
        if (type == "int32") return "i32$";
        if (type == "int64") return "i64$";
        if (type == "uint8") return "u8$";
        if (type == "uint16") return "u16$";
        if (type == "uint32") return "u32$";
        if (type == "uint64") return "u64$";
        if (currentStruct.templates.find(type) != currentStruct.templates.end()) {
            return typeToRTSigIdent(currentStruct.templates[type]);
        }
        return "L" + type + "$";
    }

    std::string argsToRTSignatureIdent(Function* f) {
        std::string args = "$$";
        for (const Variable& v : f->args) {
            std::string type = removeTypeModifiers(v.type);
            if (v.name == "self" && f->isMethod) {
                continue;
            }
            args += typeToRTSigIdent(type);
        }
        args += "$$";
        args += typeToRTSigIdent(f->return_type);
        return args;
    }

    bool argVecEquals(std::vector<Variable>& a, std::vector<Variable>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (size_t i = 0; i < a.size(); i++) {
            if (a[i].name == "self" || b[i].name == "self") {
                continue;
            }
            if (a[i].type != b[i].type) {
                return false;
            }
        }
        return true;
    }

    std::vector<Method*> makeVTable(TPResult& res, std::string name) {
        if (vtables.find(name) != vtables.end()) {
            return vtables[name];
        }
        std::vector<Method*> vtable;
        const Struct& s = getStructByName(res, name);
        if (s.super.size()) {
            auto parent = getStructByName(res, s.super);
            if (parent == Struct::Null) {
                fprintf(stderr, "Error: Struct '%s' extends '%s', but '%s' does not exist.\n", s.name.c_str(), s.super.c_str(), s.super.c_str());
                exit(1);
            }
            const std::vector<Method*>& parentVTable = makeVTable(res, parent.name);
            for (auto&& m : parentVTable) {
                vtable.push_back(m);
            }
        }
        for (auto&& m : methodsOnType(res, name)) {
            long indexInVtable = -1;
            for (size_t i = 0; i < vtable.size(); i++) {
                if (vtable[i]->name == m->name && argVecEquals(vtable[i]->args, m->args)) {
                    indexInVtable = i;
                    break;
                }
            }
            if (indexInVtable >= 0) {
                vtable[indexInVtable] = m;
            } else {
                vtable.push_back(m);
            }
        }
        return vtables[name] = vtable;
    }

    extern "C" void ConvertC::writeTables(FILE* fp, TPResult& result, std::string filename) {
        (void) filename;
        populateTokenJumpTable();

        scopeDepth = 0;
        append("extern tls _scl_stack_t _stack;\n\n");

        for (Function* function : result.functions) {
            if (function->isMethod) {
                if (getInterfaceByName(result, function->member_type)) {
                    continue;
                }
                append("static void _scl_vt_c_%s$%s();\n", function->member_type.c_str(), function->finalName().c_str());
            }
        }

        for (Struct& s : result.structs) {
            if (s.isStatic()) continue;
            append("extern const TypeInfo _scl_statics_%s;\n", s.name.c_str());
            append("extern const struct _scl_methodinfo _scl_vtable_%s[];\n", s.name.c_str());
            append("extern const _scl_lambda _scl_fast_vtable_%s[];\n", s.name.c_str());
        }
        for (Struct& s : result.structs) {
            if (s.isStatic()) continue;
            append("extern const TypeInfo _scl_statics_%s;\n", s.name.c_str());
        }

        for (auto&& vtable : vtables) {
            append("extern const struct _scl_methodinfo _scl_vtable_%s[];\n", vtable.first.c_str());
        }

        append("extern const ID_t id(const char*);\n\n");

        fclose(fp);
    }

    std::unordered_map<std::string, std::vector<Function*>> functionsByFileMap(std::vector<Function*> functions) {
        std::unordered_map<std::string, std::vector<Function*>> result;
        for (std::string& file : Main.options.files) {
            result[file] = std::vector<Function*>();
        }
        for (Function* f : functions) {
            result[f->name_token.file].push_back(f);
        }
        return result;
    }

    extern std::vector<std::string> cflags;

    extern "C" void ConvertC::writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<Variable>& globals, TPResult& result, std::string filename) {
        (void) warns;

        auto filePreamble = [&]() {
            append("#include \"./%s.h\"\n\n", filename.substr(0, filename.size() - 2).c_str());
            currentStruct = Struct::Null;

            append("#if defined(__clang__)\n");
            append("#pragma clang diagnostic push\n");
            append("#pragma clang diagnostic ignored \"-Wint-to-void-pointer-cast\"\n");
            append("#pragma clang diagnostic ignored \"-Wint-to-pointer-cast\"\n");
            append("#pragma clang diagnostic ignored \"-Wpointer-to-int-cast\"\n");
            append("#pragma clang diagnostic ignored \"-Wvoid-pointer-to-int-cast\"\n");
            append("#pragma clang diagnostic ignored \"-Wincompatible-pointer-types\"\n");
            append("#pragma clang diagnostic ignored \"-Wint-conversion\"\n");
            append("#endif\n\n");
        };

        auto filePostamble = [&]() {
            append("#if defined(__clang__)\n");
            append("#pragma clang diagnostic pop\n");
            append("#endif\n");
        };

        fp = fopen(("./" + filename).c_str(), "a");
        if (!fp) {
            fprintf(stderr, "Failed to open file %s\n", filename.c_str());
            exit(1);
        }
        filePreamble();

        for (size_t f = 0; f < result.functions.size(); f++) {
            Function* function = currentFunction = result.functions[f];
            if (function->isExternC) continue;
            if (contains<std::string>(function->modifiers, "<binary-inherited>")) {
                continue;
            }
            if (getInterfaceByName(result, function->member_type)) {
                continue;
            }
            if (function->isMethod) {
                currentStruct = getStructByName(result, ((Method*) function)->member_type);
            } else {
                currentStruct = Struct::Null;
            }

            for (long t = typeStack.size() - 1; t >= 0; t--) {
                typePop;
            }
            vars.clear();
            varScopePush();
            for (Variable& g : globals) {
                varScopeTop().push_back(g);
            }

            scopeDepth = 0;
            noWarns = false;
            lambdaCount = 0;

            for (std::string& modifier : function->modifiers) {
                if (modifier == "nowarn") {
                    noWarns = true;
                }
            }

            std::string functionDeclaration = "";

            if (function->isMethod) {
                functionDeclaration += ((Method*) function)->member_type + ":" + function->name + "(";
                for (size_t i = 0; i < function->args.size() - 1; i++) {
                    if (i != 0) {
                        functionDeclaration += ", ";
                    }
                    functionDeclaration += function->args[i].name + ": " + function->args[i].type;
                }
                functionDeclaration += "): " + function->return_type;
            } else {
                functionDeclaration += function->name + "(";
                for (size_t i = 0; i < function->args.size(); i++) {
                    if (i != 0) {
                        functionDeclaration += ", ";
                    }
                    functionDeclaration += function->args[i].name + ": " + function->args[i].type;
                }
                functionDeclaration += "): " + function->return_type;
            }

            return_type = "void";

            if (function->return_type.size() > 0) {
                std::string t = function->return_type;
                return_type = sclTypeToCType(result, t);
            } else {
                FPResult err;
                err.success = false;
                err.message = "Function '" + function->name + "' does not specify a return type, defaults to none.";
                err.line = function->name_token.line;
                err.in = function->name_token.file;
                err.column = function->name_token.column;
                err.value = function->name_token.value;
                err.type = function->name_token.type;
                if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                else errors.push_back(err);
            }

            return_type = "void";

            std::string arguments = "";
            if (function->isMethod) {
                arguments = sclTypeToCType(result, function->member_type) + " Var_self";
                for (size_t i = 0; i < function->args.size() - 1; i++) {
                    std::string type = sclTypeToCType(result, function->args[i].type);
                    arguments += ", " + type;
                    if (type == "varargs" || type == "...") continue;
                    if (function->args[i].name.size())
                        arguments += " Var_" + function->args[i].name;
                }
            } else {
                if (function->args.size() > 0) {
                    for (size_t i = 0; i < function->args.size(); i++) {
                        std::string type = sclTypeToCType(result, function->args[i].type);
                        if (i) {
                            arguments += ", ";
                        }
                        arguments += type;
                        if (type == "varargs" || type == "...") continue;
                        if (function->args[i].name.size())
                            arguments += " Var_" + function->args[i].name;
                    }
                }
            }

            if (arguments.empty()) {
                arguments = "void";
            }

            std::string file = function->name_token.file;
            if (strstarts(file, scaleFolder)) {
                file = file.substr(scaleFolder.size() + std::string("/Frameworks/").size());
            } else {
                file = std::filesystem::path(file).relative_path();
            }

            if (function->return_type.size() > 0) {
                std::string t = function->return_type;
                return_type = sclTypeToCType(result, t);
            }

            if (function->isMethod) {
                if (!(((Method*) function))->force_add && getStructByName(result, function->member_type).isSealed()) {
                    FPResult result;
                    result.message = "Struct '" + function->member_type + "' is sealed!";
                    result.value = function->name_token.value;
                    result.line = function->name_token.line;
                    result.in = function->name_token.file;
                    result.column = function->name_token.column;
                    result.type = function->name_token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                append("%s Method_%s$%s(%s) {\n", return_type.c_str(), function->member_type.c_str(), function->finalName().c_str(), arguments.c_str());
                if (function->name == "init") {
                    Method* thisMethod = (Method*) function;
                    Struct s = getStructByName(result, thisMethod->member_type);
                    if (s == Struct::Null) {
                        FPResult result;
                        result.message = "Struct '" + thisMethod->member_type + "' does not exist!";
                        result.value = function->name_token.value;
                        result.line = function->name_token.line;
                        result.in = function->name_token.file;
                        result.column = function->name_token.column;
                        result.type = function->name_token.type;
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    if (s.super.size()) {
                        Method* parentInit = getMethodByName(result, "init", s.super);
                        // implicit call to parent init, if it takes no arguments
                        if (parentInit && parentInit->args.size() == 1) {
                            scopeDepth++;
                            append("(_stack.sp++)->v = Var_self;\n");
                            generateUnsafeCall(parentInit, fp, result);
                            scopeDepth--;
                        }
                    }
                }
            } else {
                if (function->has_restrict) {
                    append("static volatile scl_SclObject function_lock$%s = NULL;\n", function->finalName().c_str());
                }
                if (function->name == "throw" || function->name == "builtinUnreachable") {
                    append("_scl_no_return ");
                }
                if (function->has_lambda) {
                    append("_scl_noinline %s Function_%s(%s) __asm(%s);\n", return_type.c_str(), function->finalName().c_str(), arguments.c_str(), generateSymbolForFunction(function).c_str());
                    append("_scl_noinline ");
                }
                append("%s Function_%s(%s) {\n", return_type.c_str(), function->finalName().c_str(), arguments.c_str());
                if (function->has_restrict) {
                    append("if (_scl_expect(function_lock$%s == NULL, 0)) function_lock$%s = ALLOC(SclObject);\n", function->finalName().c_str(), function->finalName().c_str());
                }
            }
            
            scopeDepth++;
            std::vector<Token> body = function->getBody();

            varScopePush();
            if (function->namedReturnValue.name.size()) {
                varScopeTop().push_back(function->namedReturnValue);
            }
            if (function->args.size() > 0) {
                for (size_t i = 0; i < function->args.size(); i++) {
                    const Variable& var = function->args[i];
                    if (var.type == "varargs") continue;
                    varScopeTop().push_back(var);
                }
            }

            // append("fprintf(stderr, \"trace ptr: %%p\\n\", _stack.tp);\n");
            append("*(_stack.tp++) = \"%s\";\n", sclFunctionNameToFriendlyString(function).c_str());
            if (function->namedReturnValue.name.size()) {
                std::string nrvName = function->namedReturnValue.name;
                if (hasFunction(result, nrvName)) {
                    {
                        FPResult err;
                        err.message = "Named return value '" + nrvName + "' shadowed by function '" + nrvName + "'";
                        err.value = function->name_token.value;
                        err.line = function->name_token.line;
                        err.in = function->name_token.file;
                        err.column = function->name_token.column;
                        err.type = function->name_token.type;
                        err.success = false;
                        if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                        else errors.push_back(err);
                    }
                }
                if (hasContainer(result, nrvName)) {
                    {
                        FPResult err;
                        err.message = "Named return value '" + nrvName + "' shadowed by container '" + nrvName + "'";
                        err.value = function->name_token.value;
                        err.line = function->name_token.line;
                        err.in = function->name_token.file;
                        err.column = function->name_token.column;
                        err.type = function->name_token.type;
                        err.success = false;
                        if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                        else errors.push_back(err);
                    }
                }
                if (getStructByName(result, nrvName) != Struct::Null) {
                    {
                        FPResult err;
                        err.message = "Named return value '" + nrvName + "' shadowed by struct '" + nrvName + "'";
                        err.value = function->name_token.value;
                        err.line = function->name_token.line;
                        err.in = function->name_token.file;
                        err.column = function->name_token.column;
                        err.type = function->name_token.type;
                        err.success = false;
                        if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                        else errors.push_back(err);
                    }
                }
                append("%s Var_%s;\n", sclTypeToCType(result, function->namedReturnValue.type).c_str(), function->namedReturnValue.name.c_str());
            }
            if (function->isMethod) {
                Struct s = getStructByName(result, function->member_type);
                std::string superType = s.super;
                if (superType.size() > 0) {
                    append("%s Var_super = (%s) Var_self;\n", sclTypeToCType(result, superType).c_str(), sclTypeToCType(result, superType).c_str());
                    varScopeTop().push_back(Variable("super", "const " + superType));
                }
            }
            append("const _scl_frame_t* __current_base_ptr = _stack.sp;\n");

            for (ssize_t a = (ssize_t) function->args.size() - 1; a >= 0; a--) {
                Variable& arg = function->args[a];
                if (arg.name.size() == 0) continue;
                if (!typeCanBeNil(arg.type)) {
                    if (function->isMethod && arg.name == "self") continue;
                    if (arg.type == "varargs") continue;
                    append("_scl_check_not_nil_argument(*(scl_int*) &Var_%s, \"%s\");\n", arg.name.c_str(), arg.name.c_str());
                }
                if (arg.typeFromTemplate.size()) {
                    append("_scl_checked_cast(*(scl_any*) &Var_%s, Var_self->$template_arg_%s, Var_self->$template_argname_%s);\n", arg.name.c_str(), arg.typeFromTemplate.c_str(), arg.typeFromTemplate.c_str());
                }
            }
            
            if (function->has_unsafe) {
                isInUnsafe++;
            }
            if (function->has_restrict) {
                if (function->isMethod) {
                    append("Function_Process$lock((scl_SclObject) Var_self);\n");
                } else {
                    append("Function_Process$lock(function_lock$%s);\n", function->finalName().c_str());
                }
            }

            if (function->isCVarArgs()) {
                if (function->varArgsParam().name.size()) {
                    append("va_list _cvarargs;\n");
                    append("va_start(_cvarargs, Var_%s$size);\n", function->varArgsParam().name.c_str());
                    append("scl_int _cvarargs_count = Var_%s$size;\n", function->varArgsParam().name.c_str());
                    append("scl_Array Var_%s = _scl_cvarargs_to_array(_cvarargs, _cvarargs_count);\n", function->varArgsParam().name.c_str());
                    append("va_end(_cvarargs);\n");
                    Variable v(function->varArgsParam().name, "const ReadOnlyArray");
                    varScopeTop().push_back(v);
                }
            }

            for (i = 0; i < body.size(); i++) {
                handle(Token);
            }
            
            if (function->has_unsafe) {
                isInUnsafe--;
            }
            if (function->has_restrict) {
                if (function->isMethod) {
                    append("Function_Process$unlock((scl_SclObject) Var_self);\n");
                } else {
                    append("Function_Process$unlock(function_lock$%s);\n", function->finalName().c_str());
                }
            }
            
            scopeDepth = 1;

            while (vars.size() > 1) {
                varScopePop();
            }

            append("--_stack.tp;\n");
            append("_stack.sp = __current_base_ptr;\n");

            scopeDepth = 0;
            append("}\n\n");
        }
    
        filePostamble();
    }
} // namespace sclc
