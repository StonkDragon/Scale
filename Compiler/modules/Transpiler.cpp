#include <filesystem>
#include <stack>
#include <functional>
#include <csignal>

#include "../headers/Common.hpp"
#include "../headers/TranspilerDefs.hpp"

#ifndef VERSION
#define VERSION ""
#endif

#define debugDump(_var) std::cout << __func__ << ":" << std::to_string(__LINE__) << ": " << #_var << ": " << _var << std::endl
#define typePop do { if (typeStack.size()) { typeStack.pop(); } } while (0)
#define safeInc() do { if (++i >= body.size()) { std::cerr << body.back().getFile() << ":" << body.back().getLine() << ":" << body.back().getColumn() << ":" << Color::RED << " Unexpected end of file!" << std::endl; std::raise(SIGSEGV); } } while (0)
#define safeIncN(n) do { if ((i + n) >= body.size()) { std::cerr << body.back().getFile() << ":" << body.back().getLine() << ":" << body.back().getColumn() << ":" << Color::RED << " Unexpected end of file!" << std::endl; std::raise(SIGSEGV); } i += n; } while (0)
#define THIS_INCREMENT_IS_CHECKED ++i;

namespace sclc {
    Function* currentFunction;
    Struct currentStruct("");

    std::map<std::string, std::vector<Method*>> vtables;

    std::string sclFunctionNameToFriendlyString(Function* f);
    std::string sclFunctionNameToFriendlyString(std::string name);

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
        if (t.size() && t != "?" && t.back() == '?') {
            t = t.substr(0, t.size() - 1);
        }
        return t;
    }

    std::string sclTypeToCType(TPResult& result, std::string t) {
        if (t == "?") {
            return "/* ? */ scl_any";
        }
        t = removeTypeModifiers(t);

        if (strstarts(t, "lambda(")) return "/* " + t + " */ _scl_lambda";
        if (t == "any") return "/* any */ scl_any";
        if (t == "none") return "/* none */ void";
        if (t == "nothing") return "/* nothing */ _scl_no_return void";
        if (t == "int") return "/* int */ scl_int";
        if (t == "uint") return "/* uint */ scl_uint";
        if (t == "float") return "/* float */ scl_float";
        if (t == "bool") return "/* bool */ scl_bool";
        if (t == "varargs") return "/* varargs */ ...";
        if (!(getStructByName(result, t) == Struct::Null)) {
            return "/* " + t + " */ scl_" + getStructByName(result, t).getName();
        }
        if (t.size() > 2 && t.front() == '[') {
            std::string type = sclTypeToCType(result, t.substr(1, t.size() - 2));
            return type + "*";
        }
        if (getInterfaceByName(result, t)) {
            return "/* " + t + " */ scl_any";
        }
        if (hasTypealias(result, t)) {
            return "/* " + t + " */ " + getTypealias(result, t);
        }
        if (hasLayout(result, t)) {
            return "/*" + t + "*/ scl_" + getLayout(result, t).getName();
        }

        return "/* ERROR: unable to deduce type, treating as 'any' */ scl_any";
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
        size_t maxValue = func->getArgs().size();
        if (func->isMethod) {
            args = "(" + sclTypeToCType(result, func->member_type) + ") _scl_positive_offset(" + std::to_string(func->getArgs().size() - 1) + ")->i";
            maxValue--;
        }
        if (func->isCVarArgs())
            maxValue--;

        for (size_t i = 0; i < maxValue; i++) {
            Variable& arg = func->getArgs()[i];
            if (func->isMethod || i)
                args += ", ";

            if (removeTypeModifiers(arg.getType()) == "float") {
                if (i) {
                    args += "_scl_positive_offset(" + std::to_string(i) + ")->f";
                } else {
                    args += "_scl_positive_offset(0)->f";
                }
            } else if (removeTypeModifiers(arg.getType()) == "int" || removeTypeModifiers(arg.getType()) == "bool") {
                if (i) {
                    args += "_scl_positive_offset(" + std::to_string(i) + ")->i";
                } else {
                    args += "_scl_positive_offset(0)->i";
                }
            } else if (removeTypeModifiers(arg.getType()) == "str") {
                if (i) {
                    args += "_scl_positive_offset(" + std::to_string(i) + ")->s";
                } else {
                    args += "_scl_positive_offset(0)->s";
                }
            } else if (removeTypeModifiers(arg.getType()) == "any") {
                if (i) {
                    args += "_scl_positive_offset(" + std::to_string(i) + ")->v";
                } else {
                    args += "_scl_positive_offset(0)->v";
                }
            } else if (isPrimitiveIntegerType(arg.getType())) {
                if (i) {
                    args += sclIntTypeToConvert(arg.getType()) + "((scl_any) _scl_positive_offset(" + std::to_string(i) + ")->i)";
                } else {
                    args += sclIntTypeToConvert(arg.getType()) + "((scl_any) _scl_positive_offset(0)->i)";
                }
            } else {
                if (i) {
                    args += "(" + sclTypeToCType(result, arg.getType()) + ") _scl_positive_offset(" + std::to_string(i) + ")->i";
                } else {
                    args += "(" + sclTypeToCType(result, arg.getType()) + ") _scl_positive_offset(0)->i";
                }
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
                symbol = m->getMemberType() + "$" + f->finalName();
            } else {
                std::string name = f->finalName();
                name = sclFunctionNameToFriendlyString(name);

                symbol = m->getMemberType() + ":" + name;
                symbol += argsToRTSignature(f);
            }
        } else {
            std::string finalName = symbol;
            if (f->isExternC || f->has_expect || f->has_export) {
                symbol = finalName;
            } else {
                symbol = sclFunctionNameToFriendlyString(f->getName());
                symbol += argsToRTSignature(f);
            }
        }

        if (f->isExternC || f->has_expect || f->has_export) {
            return "_scl_macro_to_string(__USER_LABEL_PREFIX__) \"" + symbol + "\"";
        }
        return "\"" + symbol + "\"";
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

        append("const char _scl_version[] = \"%s\";\n\n", std::string(VERSION).c_str());
    }

    void ConvertC::writeFunctionHeaders(FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        append("/* FUNCTION HEADERS */\n");

        for (Function* function : result.functions) {
            std::string return_type = sclTypeToCType(result, function->return_type);
            auto args = function->getArgs();
            std::string arguments;
            arguments.reserve(32);
            
            if (function->isMethod) {
                currentStruct = getStructByName(result, function->member_type);
                arguments += sclTypeToCType(result, function->member_type) + " Var_self";
                for (long i = 0; i < (long) args.size() - 1; i++) {
                    std::string type = sclTypeToCType(result, args[i].getType());
                    arguments += ", " + type;
                    if (type == "varargs" || type == "/* varargs */ ...") continue;
                    if (args[i].getName().size())
                        arguments += " Var_" + args[i].getName();
                }
            } else {
                currentStruct = Struct::Null;
                if (args.size() > 0) {
                    for (long i = 0; i < (long) args.size(); i++) {
                        std::string type = sclTypeToCType(result, args[i].getType());
                        if (i) {
                            arguments += ", ";
                        }
                        arguments += type;
                        if (type == "varargs" || type == "/* varargs */ ...") continue;
                        if (args[i].getName().size())
                            arguments += " Var_" + args[i].getName();
                    }
                }
            }

            if (arguments.empty()) {
                arguments = "void";
            }

            std::string symbol = generateSymbolForFunction(function);

            if (!function->isMethod) {
                if (function->getName() == "throw" || function->getName() == "builtinUnreachable") {
                    append("_scl_no_return ");
                }
                append("%s Function_%s(%s)\n", return_type.c_str(), function->finalName().c_str(), arguments.c_str());
                append("    __asm(%s);\n", symbol.c_str());
            } else {
                append("%s Method_%s$%s(%s)\n", return_type.c_str(), function->getMemberType().c_str(), function->finalName().c_str(), arguments.c_str());
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
            result += args[i].getName() + ": " + args[i].getType();
        }
        return result;
    }

    bool structImplements(TPResult& result, Struct s, std::string interface) {
        do {
            if (s.implements(interface)) {
                return true;
            }
            s = getStructByName(result, s.extends());
        } while (s.getName().size());
        return false;
    }

    void ConvertC::writeGlobals(FILE* fp, std::vector<Variable>& globals, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        if (result.globals.size() == 0) return;
        append("/* GLOBALS */\n");

        for (Variable& s : result.globals) {
            append("%s Var_%s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
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
            if (c.getName() == "str" || c.getName() == "any" || c.getName() == "int" || c.getName() == "float" || isPrimitiveIntegerType(c.getName())) continue;
            append("typedef struct Struct_%s* scl_%s;\n", c.getName().c_str(), c.getName().c_str());
        }
        append("\n");
        for (Layout& c : result.layouts) {
            append("typedef struct Layout_%s* scl_%s;\n", c.getName().c_str(), c.getName().c_str());
        }
        for (Layout& c : result.layouts) {
            append("struct Layout_%s {\n", c.getName().c_str());
            for (Variable& s : c.getMembers()) {
                append("  %s %s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
            }
            append("};\n");
        }
        append("\n");
        if (result.containers.size() == 0) return;
        
        append("/* CONTAINERS */\n");
        for (Container& c : result.containers) {
            append("struct {\n");
            for (Variable& s : c.getMembers()) {
                append("  %s %s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
            }
            append("} Container_%s = {0};\n", c.getName().c_str());
        }
    }

    bool typeEquals(std::string a, std::string b);

    bool argsAreIdentical(std::vector<Variable> methodArgs, std::vector<Variable> functionArgs) {
        if ((methodArgs.size() - 1) != functionArgs.size()) return false;
        for (size_t i = 0; i < methodArgs.size(); i++) {
            if (methodArgs[i].getName() == "self" || functionArgs[i].getName() == "self") continue;
            if (!typeEquals(methodArgs[i].getType(), functionArgs[i].getType())) return false;
        }
        return true;
    }

    std::string argVectorToString(std::vector<Variable> args) {
        std::string arg = "";
        for (size_t i = 0; i < args.size(); i++) {
            if (i) 
                arg += ", ";
            const Variable& v = args[i];
            arg += removeTypeModifiers(v.getType());
        }
        return arg;
    }

    void ConvertC::writeStructs(FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        if (result.structs.size() == 0) return;
        append("/* STRUCT DEFINITIONS */\n");

        for (Struct& c : result.structs) {
            if (c.isStatic()) continue;
            currentStruct = c;
            for (std::string& i : c.getInterfaces()) {
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
                    Struct argType = getStructByName(result, m->getReturnType());
                    bool implementedInterface = false;
                if (argType != Struct::Null) {
                        Interface* interface = getInterfaceByName(result, f->getReturnType());
                        if (interface != nullptr) {
                            implementedInterface = structImplements(result, argType, interface->getName());
                        }
                    }
                    if (!implementedInterface && f->getReturnType() != "?" && !typeEquals(m->getReturnType(), f->getReturnType())) {
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
                    if (f->getReturnType() == "?" && m->getReturnType() == "none") {
                        FPResult res;
                        Token t = m->getNameToken();
                        res.success = false;
                        res.message = "Cannot return 'none' from method '" + c.getName() + ":" + m->getName() + "' when implemented method '" + f->getName() + "' returns '" + f->getReturnType() + "'";
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

            for (Function* f : result.functions) {
                if (!f->isMethod) continue;
                Method* m = (Method*) f;
                if (m->member_type != c.getName()) continue;

                Struct super = getStructByName(result, c.extends());
                while (super != Struct::Null) {
                    Method* other = getMethodByNameOnThisType(result, m->getName(), super.getName());
                    if (!other) goto afterSealedCheck;
                    if (other->has_sealed) {
                        FPResult res;
                        Token t = m->getNameToken();
                        res.success = false;
                        res.message = "Method '" + f->getName() + "' overrides 'sealed' method on '" + super.getName() + "'";
                        res.line = t.getLine();
                        res.in = t.getFile();
                        res.column = t.getColumn();
                        res.value = t.getValue();
                        res.type = t.getType();
                        errors.push_back(res);
                        FPResult res2;
                        Token t2 = other->getNameToken();
                        res2.success = false;
                        res2.isNote = true;
                        res2.message = "Declared here:";
                        res2.line = t2.getLine();
                        res2.in = t2.getFile();
                        res2.column = t2.getColumn();
                        res2.value = t2.getValue();
                        res2.type = t2.getType();
                        errors.push_back(res2);
                        break;
                    }
                afterSealedCheck:
                    super = getStructByName(result, super.extends());
                }
            }

            append("struct Struct_%s {\n", c.getName().c_str());
            append("  StaticMembers* $statics;\n");
            append("  mutex_t $mutex;\n");
            for (Variable& s : c.getMembers()) {
                append("  %s %s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
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

    bool canBeCastTo(TPResult& r, Struct& one, Struct& other) {
        if (one == other || other.getName() == "SclObject") {
            return true;
        }
        if ((one.isStatic() && !other.isStatic()) || (!one.isStatic() && other.isStatic())) {
            return false;
        }
        Struct oneParent = getStructByName(r, one.extends());
        bool extend = false;
        while (!extend) {
            if (oneParent == other) {
                extend = true;
            } else if (oneParent.getName() == "SclObject" || oneParent.getName().empty()) {
                return false;
            }
            oneParent = getStructByName(r, oneParent.extends());
        }
        return extend;
    }

    std::string lambdaReturnType(std::string lambda) {
        if (strstarts(lambda, "lambda("))
            return lambda.substr(lambda.find_last_of("):") + 1);
        return "";
    }

    size_t lambdaArgCount(std::string lambda) {
        if (strstarts(lambda, "lambda(")) {
            std::string args = lambda.substr(lambda.find_first_of("(") + 1, lambda.find_last_of("):") - lambda.find_first_of("(") - 1);
            return std::stoi(args);
        }
        return -1;
    }

    bool lambdasEqual(std::string a, std::string b) {
        std::string returnTypeA = lambdaReturnType(a);
        std::string returnTypeB = lambdaReturnType(b);

        size_t argAmountA = lambdaArgCount(a);
        size_t argAmountB = lambdaArgCount(b);

        return argAmountA == argAmountB && typeEquals(returnTypeA, returnTypeB);
    }

    bool typeEquals(std::string a, std::string b) {
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
            Struct givenType = getStructByName(result, stack);
            if (givenType == Struct::Null) {
                return false;
            } else if (!givenType.implements(interface->getName())) {
                return false;
            }
        } else if (!typeEquals(stack, arg)) {
            Struct givenType = getStructByName(result, stack);
            Struct requiredType = getStructByName(result, arg);
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

    bool checkStackType(TPResult& result, std::vector<Variable>& args, bool allowIntPromotion = false) {
        if (args.size() == 0) {
            return true;
        }
        if (typeStack.size() < args.size()) {
            return false;
        }

        if (args.back().getType() == "varargs") {
            return true;
        }

        auto end = &typeStack.top() + 1;
        auto begin = end - args.size();
        std::vector<std::string> stack(begin, end);

        for (ssize_t i = args.size() - 1; i >= 0; i--) {
            bool tmp = typesCompatible(result, stack[i], args[i].getType(), allowIntPromotion);
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
        if (self->getArgs().size() > 0)
            append("_stack.sp -= (%zu);\n", self->getArgs().size());
        if (removeTypeModifiers(self->getReturnType()) == "none" || removeTypeModifiers(self->getReturnType()) == "nothing") {
            append("Method_%s$%s(%s);\n", self->getMemberType().c_str(), self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
        } else {
            if (removeTypeModifiers(self->getReturnType()) == "float") {
                append("(_stack.sp++)->f = Method_%s$%s(%s);\n", self->getMemberType().c_str(), self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
            } else {
                append("(_stack.sp++)->i = (scl_int) Method_%s$%s(%s);\n", self->getMemberType().c_str(), self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
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
            
            std::string message = "Function '" + self->getName() + "' is deprecated";
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

    Method* findMethodLocally(Method* self, TPResult result) {
        for (Function* f : result.functions) {
            if (!f->isMethod) continue;
            Method* m = (Method*) f;
            if ((m->getName() == self->getName() || strstarts(m->getName(), self->getName() + "$$ol")) && m->getMemberType() == self->getMemberType()) {
                if (currentFunction) {
                    if (m->getNameToken().getFile() == currentFunction->getNameToken().getFile()) {
                        return m;
                    }
                }
                return m;
            }
        }
        return nullptr;
    }

    Function* findFunctionLocally(Function* self, TPResult result) {
        for (Function* f : result.functions) {
            if (f->isMethod) continue;
            if (f->getName() == self->getName() || strstarts(f->getName(), self->getName() + "$$ol")) {
                if (currentFunction) {
                    if (f->getNameToken().getFile() == currentFunction->getNameToken().getFile()) {
                        return f;
                    }
                } else {
                    return f;
                }
            }
        }
        return nullptr;
    }

    std::string argsToRTSignature(Function*);

    void methodCall(Method* self, FILE* fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t i, bool ignoreArgs = false, bool doActualPop = true, bool withIntPromotion = false, bool onSuperType = false, bool checkOverloads = true) {
        if (!shouldCall(self, warns, errors, body, i)) {
            return;
        }
        if (currentFunction) {
            if (self->has_private) {
                Token selfNameToken = self->getNameToken();
                Token fromNameToken = currentFunction->getNameToken();
                if (selfNameToken.getFile() != fromNameToken.getFile()) {
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
        bool argsEqual = ignoreArgs || checkStackType(result, self->getArgs(), withIntPromotion);
        if (checkOverloads && overloads.size() && !argsEqual && !ignoreArgs) {
            for (bool b : bools) {
                for (Function* overload : overloads) {
                    if (!overload->isMethod) continue;
                    
                    bool argsEqual = checkStackType(result, overload->getArgs(), b);
                    if (argsEqual) {
                        methodCall((Method*) overload, fp, result, warns, errors, body, i, ignoreArgs, doActualPop, b);
                        return;
                    }
                }
            }
        }

        argsEqual = checkStackType(result, self->getArgs());
        if (!argsEqual && !ignoreArgs) {
            if (opFunc(self->getName())) {
                Function* f = getFunctionByName(result, self->getName());
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
                    overloadedMatches += ", or [ " + Color::BLUE + argVectorToString(overload->getArgs()) + Color::RESET + " ]";
                }
            }

            transpilerError("Expected: [ " + Color::BLUE + argVectorToString(self->getArgs()) + Color::RESET + " ]" + overloadedMatches + ", but got: [ " + Color::RED + stackSliceToString(self->getArgs().size()) + Color::RESET + " ]", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }

        if (self->isExternC && !hasImplementation(result, self)) {
            std::string functionDeclaration = "";

            functionDeclaration += ((Method*) self)->getMemberType() + ":" + self->getName() + "(";
            for (size_t i = 0; i < self->getArgs().size() - 1; i++) {
                if (i != 0) {
                    functionDeclaration += ", ";
                }
                functionDeclaration += self->getArgs()[i].getName() + ": " + self->getArgs()[i].getType();
            }
            functionDeclaration += "): " + self->getReturnType();
            append("*(_stack.tp++) = \"<extern %s>\";\n", sclFunctionNameToFriendlyString(self).c_str());
        }
        
        if (doActualPop) {
            for (size_t m = 0; m < self->getArgs().size(); m++) {
                typePop;
            }
        }
        if (getInterfaceByName(result, self->getMemberType())) {
            append("// invokeinterface %s:%s\n", self->getMemberType().c_str(), self->finalName().c_str());
        } else {
            if (onSuperType) {
                append("// invokespecial %s:%s\n", self->getMemberType().c_str(), self->finalName().c_str());
            } else {
                append("// invokedynamic %s:%s\n", self->getMemberType().c_str(), self->finalName().c_str());
            }
        }
        bool found = false;
        if (getInterfaceByName(result, self->getMemberType()) == nullptr) {
            auto vtable = vtables[self->getMemberType()];
            size_t index = 0;
            for (auto&& method : vtable) {
                if (*method == *self) {
                    if (Main.options.debugBuild) {
                        append("CAST0((_stack.sp - 1)->v, %s, 0x%x);\n", self->getMemberType().c_str(), id((char*) self->getMemberType().c_str()));
                    }
                    if (onSuperType) {
                        append("(_stack.sp - 1)->o->$statics->super_vtable[%zu].ptr();\n", index);
                    } else {
                        append("(_stack.sp - 1)->o->$statics->vtable[%zu].ptr();\n", index);
                    }
                    found = true;
                    break;
                }
                index++;
            }
        } else {
            std::string rtSig = argsToRTSignature(self);
            append("_scl_call_method_or_throw((_stack.sp - 1)->v, 0x%xU, 0x%xU, 0, \"%s\", \"%s\");\n", id((char*) self->getName().c_str()), id((char*) rtSig.c_str()), self->getName().c_str(), rtSig.c_str());
            found = true;
        }
        if (removeTypeModifiers(self->getReturnType()) != "none" && removeTypeModifiers(self->getReturnType()) != "nothing") {
            typeStack.push(self->getReturnType());
        }
        if (self->isExternC && !hasImplementation(result, self)) {
            append("--_stack.tp;\n");
        }
        if (!found) {
            transpilerError("Method '" + sclFunctionNameToFriendlyString(self) + "' not found on type '" + self->getMemberType() + "'", i);
            errors.push_back(err);
        }
    }

    void generateUnsafeCall(Function* self, FILE* fp, TPResult& result) {
        if (self->getArgs().size() > 0)
            append("_stack.sp -= (%zu);\n", self->getArgs().size());
        if (removeTypeModifiers(self->getReturnType()) == "none" || removeTypeModifiers(self->getReturnType()) == "nothing") {
            append("Function_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
        } else {
            if (removeTypeModifiers(self->getReturnType()) == "float") {
                append("(_stack.sp++)->f = Function_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
            } else {
                append("(_stack.sp++)->i = (scl_int) Function_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
            }
        }
    }

    bool argsContainsIntType(std::vector<Variable> args) {
        for (auto&& arg : args) {
            if (isPrimitiveIntegerType(arg.getType())) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    size_t indexInVec(std::vector<T> vec, T elem) {
        for (size_t i = 0; i < vec.size(); i++) {
            if (vec[i] == elem) {
                return i;
            }
        }
        return -1;
    }

    bool operatorInt(std::string op) {
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

    bool operatorFloat(std::string op) {
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


    bool operatorBool(std::string op) {
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
            return {};
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
                Token selfNameToken = self->getNameToken();
                Token fromNameToken = currentFunction->getNameToken();
                if (selfNameToken.getFile() != fromNameToken.getFile()) {
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
        bool argsEqual = checkStackType(result, self->getArgs(), withIntPromotion);

        if (checkOverloads && overloads.size() && !argsEqual) {
            for (bool b : bools) {
                for (Function* overload : overloads) {
                    if (overload->isMethod) continue;

                    bool argsEqual = checkStackType(result, overload->getArgs(), b);
                    if (argsEqual) {
                        functionCall(overload, fp, result, warns, errors, body, i, b, hasToCallStatic, false);
                        return;
                    }
                }
            }
        }

        if (self->has_operator) {
            size_t sym = self->has_operator;

            if (!hasToCallStatic && !checkStackType(result, self->getArgs(), withIntPromotion) && hasMethod(result, self->getName(), typeStackTop)) {
                Method* method = getMethodByName(result, self->getName(), typeStackTop);
                methodCall(method, fp, result, warns, errors, body, i);
                return;
            }

            std::string op = self->getModifiers()[sym];

            if (typeStack.size() < (1 + (op != "lnot" && op != "not"))) {
                transpilerError("Cannot deduce type for operation '" + opToString(op) +  "'", i);
                errors.push_back(err);
                return;
            }

            if (op != "lnot" && op != "not") {
                typeStack.pop();
                typeStack.pop();
            } else {
                typeStack.pop();
            }

            append("_scl_%s%s();\n", op.c_str(), (Main.options.debugBuild ? "_chk" : ""));

            if (operatorInt(op)) {
                typeStack.push("int");
            } else if (operatorFloat(op)) {
                typeStack.push("float");
            } else if (operatorBool(op)) {
                typeStack.push("bool");
            } else {
                transpilerError("Unknown operator: " + op, i);
                errors.push_back(err);
            }

            return;
        }

        if (!hasToCallStatic && opFunc(self->getName()) && hasMethod(result, self->getName(), typeStackTop)) {
        makeMethodCallInstead:
            Method* method = getMethodByName(result, self->getName(), typeStackTop);
            methodCall(method, fp, result, warns, errors, body, i);
            return;
        }

        argsEqual = checkStackType(result, self->getArgs());
        if (!argsEqual && argsContainsIntType(self->getArgs())) {
            argsEqual = checkStackType(result, self->getArgs(), true);
        }
        if (!argsEqual) {
            if (hasMethod(result, self->getName(), typeStackTop)) {
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
                    overloadedMatches += ", or [ " + Color::BLUE + argVectorToString(overload->getArgs()) + Color::RESET + " ]";
                }
            }

            transpilerError("Expected: [ " + Color::BLUE + argVectorToString(self->getArgs()) + Color::RESET + " ]" + overloadedMatches + ", but got: [ " + Color::RED + stackSliceToString(self->getArgs().size()) + Color::RESET + " ]", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }

        if (self->isExternC && !hasImplementation(result, self)) {
            append("*(_stack.tp++) = \"<extern %s>\";\n", sclFunctionNameToFriendlyString(self).c_str());
        }
        
        if (self->getArgs().size() > 0) {
            append("_stack.sp -= (%zu);\n", self->getArgs().size());
        }
        for (size_t m = 0; m < self->getArgs().size(); m++) {
            typePop;
        }
        append("// invokestatic %s\n", self->finalName().c_str());
        if (self->getName() == "throw" || self->getName() == "builtinUnreachable") {
            if (currentFunction->has_restrict) {
                if (currentFunction->isMethod) {
                    append("Function_Process$unlock((scl_SclObject) Var_self);\n");
                } else {
                    append("Function_Process$unlock(function_lock$%s);\n", currentFunction->finalName().c_str());
                }
            }
        }
        if (removeTypeModifiers(self->getReturnType()) != "none" && removeTypeModifiers(self->getReturnType()) != "nothing") {
            if (removeTypeModifiers(self->getReturnType()) == "float") {
                append("(_stack.sp++)->f = Function_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
            } else {
                append("(_stack.sp++)->i = (scl_int) Function_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
            }
            typeStack.push(self->getReturnType());
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

    int lambdaCount = 0;

    handler(Lambda) {
        noUnused;
        Function* f;
        if (function->isMethod) {
            f = new Function("$lambda$" + std::to_string(lambdaCount++) + "$" + function->getMemberType() + "$$" + function->finalName(), body[i]);
        } else {
            f = new Function("$lambda$" + std::to_string(lambdaCount++) + "$" + function->finalName(), body[i]);
        }
        f->addModifier("<lambda>");
        f->addModifier(generateSymbolForFunction(function));
        f->setReturnType("none");
        safeInc();
        if (body[i].getType() == tok_paren_open) {
            safeInc();
            while (i < body.size() && body[i].getType() != tok_paren_close) {
                if (body[i].getType() == tok_identifier || body[i].getType() == tok_column) {
                    std::string name = body[i].getValue();
                    std::string type = "any";
                    if (body[i].getType() == tok_column) {
                        name = "";
                    } else {
                        safeInc();
                    }
                    if (body[i].getType() == tok_column) {
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
                    transpilerError("Expected identifier for argument name, but got '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                    safeInc();
                    continue;
                }
                safeInc();
                if (body[i].getType() == tok_comma || body[i].getType() == tok_paren_close) {
                    if (body[i].getType() == tok_comma) {
                        safeInc();
                    }
                    continue;
                }
                transpilerError("Expected ',' or ')', but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                continue;
            }
            safeInc();
        }
        if (body[i].getType() == tok_column) {
            safeInc();
            FPResult r = parseType(body, &i, getTemplates(result, function));
            if (!r.success) {
                errors.push_back(r);
                return;
            }
            std::string type = r.value;
            f->setReturnType(type);
            safeInc();
        }
        int lambdaDepth = 0;
        while (true) {
            if (body[i].getType() == tok_end && lambdaDepth == 0) {
                break;
            }
            if (body[i].getType() == tok_identifier && body[i].getValue() == "lambda") {
                if (((ssize_t) i) - 1 < 0 || body[i - 1].getType() != tok_column) {
                    lambdaDepth++;
                }
            }
            if (body[i].getType() == tok_end) {
                lambdaDepth--;
            }
            f->addToken(body[i]);
            safeInc();
        }

        std::string arguments = "";
        if (f->isMethod) {
            arguments = sclTypeToCType(result, f->member_type) + " Var_self";
            for (size_t i = 0; i < f->getArgs().size() - 1; i++) {
                std::string type = sclTypeToCType(result, f->getArgs()[i].getType());
                arguments += ", " + type;
                if (type == "varargs" || type == "/* varargs */ ...") continue;
                if (f->getArgs()[i].getName().size())
                    arguments += " Var_" + f->getArgs()[i].getName();
            }
        } else {
            if (f->getArgs().size() > 0) {
                for (size_t i = 0; i < f->getArgs().size(); i++) {
                    std::string type = sclTypeToCType(result, f->getArgs()[i].getType());
                    if (i) {
                        arguments += ", ";
                    }
                    arguments += type;
                    if (type == "varargs" || type == "/* varargs */ ...") continue;
                    if (f->getArgs()[i].getName().size())
                        arguments += " Var_" + f->getArgs()[i].getName();
                }
            }
        }

        std::string lambdaType = "lambda(" + std::to_string(f->getArgs().size()) + "):" + f->getReturnType();

        append("%s Function_$lambda%d$%s(%s) __asm(%s);\n", sclTypeToCType(result, f->getReturnType()).c_str(), lambdaCount - 1, function->finalName().c_str(), arguments.c_str(), generateSymbolForFunction(f).c_str());
        append("(_stack.sp++)->v = Function_$lambda%d$%s;\n", lambdaCount - 1, function->finalName().c_str());
        typeStack.push(lambdaType);
        result.functions.push_back(f);
    }

    handler(ReturnOnNil) {
        noUnused;
        if (typeCanBeNil(typeStackTop)) {
            std::string type = typeStackTop;
            typePop;
            typeStack.push(type.substr(0, type.size() - 1));
        }
        if (function->getReturnType() == "nothing") {
            transpilerError("Returning from a function with return type 'nothing' is not allowed.", i);
            errors.push_back(err);
            return;
        }
        if (!typeCanBeNil(function->getReturnType()) && function->getReturnType() != "none") {
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
            if (function->getReturnType() == "none")
                append("return;\n");
            else {
                append("return 0;\n");
            }
            scopeDepth--;
            append("}\n");
        }
    }

    bool structExtends(TPResult& result, Struct& s, std::string name) {
        if (s.getName() == name) return true;

        Struct super = getStructByName(result, s.extends());
        Struct oldSuper = s;
        while (super.getName().size()) {
            if (super.getName() == name)
                return true;
            oldSuper = super;
            super = getStructByName(result, super.extends());
        }
        return false;
    }

    handler(Catch) {
        noUnused;
        std::string ex = "Exception";
        safeInc();
        if (body[i].getValue() == "typeof") {
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
            Struct s = getStructByName(result, body[i].getValue());
            if (s == Struct::Null) {
                transpilerError("Trying to catch unknown Exception of type '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }

            if (structExtends(result, s, "Error")) {
                transpilerError("Cannot catch Exceptions that extend 'Error'", i);
                errors.push_back(err);
                return;
            }
            append("} else if (_scl_is_instance_of(*(_stack.ex), 0x%xU)) {\n", id((char*) body[i].getValue().c_str()));
        } else {
            i--;
            {
                transpilerError("Generic Exception caught here:", i);
                errors.push_back(err);
            }
            transpilerError("Add 'typeof Exception' here to fix this:\\insertText; typeof Exception;" + std::to_string(err.line) + ":" + std::to_string(err.column + body[i].getValue().size()), i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        std::string exName = "exception";
        if (body.size() > i + 1 && body[i + 1].getValue() == "as") {
            safeInc();
            safeInc();
            exName = body[i].getValue();
        }
        scopeDepth++;
        varScopeTop().push_back(Variable(exName, ex));
        append("scl_%s Var_%s = (scl_%s) *(_stack.ex);\n", ex.c_str(), exName.c_str(), ex.c_str());
    }

    handler(Pragma) {
        noUnused;
        if (body[i].getValue() == "macro!") {
            safeInc();
            FPResult type = parseType(body, &i, getTemplates(result, function));
            if (!type.success) {
                errors.push_back(type);
                return;
            }
            std::string ctype = sclTypeToCType(result, type.value);
            safeInc();
            std::string macro = body[i].getValue();
            if (type.value == "float") {
                append("(_stack.sp++)->f = %s;\n", macro.c_str());
            } else {
                append("(_stack.sp++)->v = (scl_any) %s;\n", macro.c_str());
            }
            typeStack.push(type.value);
            return;
        }
        safeInc();
        if (body[i].getValue() == "print") {
            safeInc();
            std::cout << body[i].getFile()
                      << ":"
                      << body[i].getLine()
                      << ":"
                      << body[i].getColumn()
                      << ": "
                      << body[i].getValue()
                      << std::endl;
        } else if (body[i].getValue() == "stackalloc") {
            safeInc();
            std::string length = body[i].getValue();
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
        }
    }

    void createVariadicCall(Function* f, FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<Token> body, size_t& i) {
        safeInc();
        if (body[i].getValue() != "!") {
            transpilerError("Expected '!' for variadic call, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        safeInc();
        if (body[i].getType() != tok_number) {
            transpilerError("Amount of variadic arguments needs to be specified for variadic function call", i);
            errors.push_back(err);
            return;
        }

        append("{\n");
        scopeDepth++;
        size_t amountOfVarargs = std::stoi(body[i].getValue());
        
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

        if (f->varArgsParam().getName().size()) {
            append("(_stack.sp++)->i = %zu;\n", amountOfVarargs);
            typeStack.push("int");
        }

        if (f->getArgs().size() > 1)
            append("_stack.sp -= (%zu);\n", f->getArgs().size() - 1);

        for (size_t i = 0; i < (amountOfVarargs + f->getArgs().size()); i++) {
            typePop;
        }

        if (f->getReturnType() == "none" || f->getReturnType() == "nothing") {
            append("Function_%s(%s);\n", f->getName().c_str(), args.c_str());
        } else {
            if (f->getReturnType() == "float") {
                append("(_stack.sp++)->f = Function_%s(%s);\n", f->getName().c_str(), args.c_str());
            } else {
                append("(_stack.sp++)->v = (scl_any) Function_%s(%s);\n", f->getName().c_str(), args.c_str());
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

    handler(Identifier) {
        noUnused;
    #ifdef __APPLE__
    #pragma region Identifier functions
    #endif
        if (body[i].getValue() == "drop") {
            append("--_stack.sp;\n");
            typePop;
        } else if (body[i].getValue() == "dup") {
            append("(_stack.sp++)->v = (_stack.sp - 1)->v;\n");
            typeStack.push(typeStackTop);
        } else if (body[i].getValue() == "swap") {
            append("_scl_swap();\n");
            std::string b = typeStackTop;
            typePop;
            std::string a = typeStackTop;
            typePop;
            typeStack.push(b);
            typeStack.push(a);
        } else if (body[i].getValue() == "over") {
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
        } else if (body[i].getValue() == "sdup2") {
            append("_scl_sdup2();\n");
            std::string b = typeStackTop;
            typePop;
            std::string a = typeStackTop;
            typePop;
            typeStack.push(a);
            typeStack.push(b);
            typeStack.push(a);
        } else if (body[i].getValue() == "swap2") {
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
        } else if (body[i].getValue() == "clearstack") {
            append("_stack.sp = _stack.bp;\n");
            for (long t = typeStack.size() - 1; t >= 0; t--) {
                typePop;
            }
        } else if (body[i].getValue() == "!!") {
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
        } else if (body[i].getValue() == "?") {
            handle(ReturnOnNil);
        } else if (body[i].getValue() == "++") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "++", typeStackTop)) return;
            append("(_stack.sp - 1)->i++;\n");
            if (typeStack.size() == 0)
                typeStack.push("int");
        } else if (body[i].getValue() == "--") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "--", typeStackTop)) return;
            append("--(_stack.sp - 1)->i;\n");
            if (typeStack.size() == 0)
                typeStack.push("int");
        } else if (body[i].getValue() == "exit") {
            append("exit((--_stack.sp)->i);\n");
            typePop;
        } else if (body[i].getValue() == "abort") {
            append("abort();\n");
        } else if (body[i].getValue() == "typeof") {
            safeInc();
            auto templates = currentStruct.templates;
            if (hasVar(body[i].getValue())) {
                append("(_stack.sp++)->s = _scl_create_string(_scl_find_index_of_struct(*(scl_any*) &Var_%s) != -1 ? ((scl_SclObject) Var_%s)->$statics->type_name : \"%s\");\n", getVar(body[i].getValue()).getName().c_str(), getVar(body[i].getValue()).getName().c_str(), getVar(body[i].getValue()).getType().c_str());
            } else if (getStructByName(result, getVar(body[i].getValue()).getType()) != Struct::Null) {
                append("(_stack.sp++)->s = _scl_create_string(Var_%s->$statics->type_name);\n", body[i].getValue().c_str());
            } else if (hasFunction(result, body[i].getValue())) {
                Function* f = getFunctionByName(result, body[i].getValue());
                std::string lambdaType = "lambda(" + std::to_string(f->getArgs().size()) + "):" + f->getReturnType();
                append("(_stack.sp++)->s = _scl_create_string(\"%s\");\n", lambdaType.c_str());
            } else if (function->isMethod && templates.find(body[i].getValue()) != templates.end()) {
                append("(_stack.sp++)->s = _scl_create_string(Var_self->$template_argname_%s);\n", body[i].getValue().c_str());
            } else {
                FPResult res = parseType(body, &i);
                if (!res.success) {
                    errors.push_back(res);
                    return;
                }
                append("(_stack.sp++)->s = _scl_create_string(\"%s\");\n", res.value.c_str());
            }
            typeStack.push("str");
        } else if (body[i].getValue() == "nameof") {
            safeInc();
            if (hasVar(body[i].getValue())) {
                append("(_stack.sp++)->s = _scl_create_string(\"%s\");\n", body[i].getValue().c_str());
                typeStack.push("str");
            } else {
                transpilerError("Unknown Variable: '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (body[i].getValue() == "sizeof") {
            safeInc();
            auto templates = getTemplates(result, function);
            if (body[i].getValue() == "int") {
                append("(_stack.sp++)->i = sizeof(scl_int);\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "float") {
                append("(_stack.sp++)->i = sizeof(scl_float);\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "str") {
                append("(_stack.sp++)->i = sizeof(scl_str);\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "any") {
                append("(_stack.sp++)->i = sizeof(scl_any);\n");
                typeStack.push("int");
                return;
            } else if (body[i].getValue() == "none" || body[i].getValue() == "nothing") {
                append("(_stack.sp++)->i = 0;\n");
                typeStack.push("int");
                return;
            }
            if (hasVar(body[i].getValue())) {
                append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, getVar(body[i].getValue()).getType()).c_str());
                typeStack.push("int");
            } else if (getStructByName(result, body[i].getValue()) != Struct::Null) {
                append("(_stack.sp++)->i = sizeof(struct Struct_%s);\n", body[i].getValue().c_str());
                typeStack.push("int");
            } else if (hasTypealias(result, body[i].getValue())) {
                append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, body[i].getValue()).c_str());
                typeStack.push("int");
            } else if (hasLayout(result, body[i].getValue())) {
                append("(_stack.sp++)->i = sizeof(struct Layout_%s);\n", body[i].getValue().c_str());
                typeStack.push("int");
            } else if (templates.find(body[i].getValue()) != templates.end()) {
                std::string replaceWith = templates[body[i].getValue()];
                if (getStructByName(result, replaceWith) != Struct::Null)
                    append("(_stack.sp++)->i = sizeof(struct Struct_%s);\n", replaceWith.c_str());
                else if (hasTypealias(result, replaceWith))
                    append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, replaceWith).c_str());
                else if (hasLayout(result, replaceWith))
                    append("(_stack.sp++)->i = sizeof(struct Layout_%s);\n", replaceWith.c_str());
                else
                    append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, replaceWith).c_str());
                typeStack.push("int");
            } else {
                FPResult type = parseType(body, &i, getTemplates(result, function));
                if (!type.success) {
                    transpilerError("Unknown Type: '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                    return;
                }
                append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, type.value).c_str());
                typeStack.push("int");
            }
        } else if (body[i].getValue() == "try") {
            append("_scl_exception_push();\n");
            append("if (setjmp(*(_stack.jmp - 1)) != 666) {\n");
            scopeDepth++;
            append("*(_stack.et - 1) = _stack.tp;\n");
            varScopePush();
            pushTry();
        } else if (body[i].getValue() == "catch") {
            handle(Catch);
        } else if (body[i].getValue() == "lambda") {
            handle(Lambda);
        } else if (body[i].getValue() == "unsafe") {
            isInUnsafe++;
            append("{\n");
            scopeDepth++;
            append("scl_int8* __prev = _stack.tp - 1;\n");
            append("*(_stack.tp - 1) = \"unsafe %s\";\n", sclFunctionNameToFriendlyString(function).c_str());
            safeInc();
            while (body[i].getType() != tok_end) {
                handle(Token);
                safeInc();
            }
            isInUnsafe--;
            append("*(_stack.tp - 1) = __prev;\n");
            scopeDepth--;
            append("}\n");

        } else if (body[i].getValue() == "pragma!" || body[i].getValue() == "macro!") {
            handle(Pragma);
    #ifdef __APPLE__
    #pragma endregion
    #endif
        } else if (hasEnum(result, body[i].getValue())) {
            Enum e = getEnumByName(result, body[i].getValue());
            safeInc();
            if (body[i].getType() != tok_double_column) {
                transpilerError("Expected '::', but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            safeInc();
            if (e.indexOf(body[i].getValue()) == Enum::npos) {
                transpilerError("Unknown member of enum '" + e.getName() + "': '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            append("(_stack.sp++)->i = %zu;\n", e.indexOf(body[i].getValue()));
            typeStack.push("int");
        } else if (hasFunction(result, body[i].getValue())) {
            Function* f = getFunctionByName(result, body[i].getValue());
            if (f->isCVarArgs()) {
                createVariadicCall(f, fp, result, errors, body, i);
                return;
            }
            if (f->isMethod) {
                transpilerError("'" + f->getName() + "' is a method, not a function.", i);
                errors.push_back(err);
                return;
            }
            functionCall(f, fp, result, warns, errors, body, i);
        } else if (hasFunction(result, function->member_type + "$" + body[i].getValue())) {
            Function* f = getFunctionByName(result, function->member_type + "$" + body[i].getValue());
            if (f->isCVarArgs()) {
                createVariadicCall(f, fp, result, errors, body, i);
                return;
            }
            if (f->isMethod) {
                transpilerError("'" + f->getName() + "' is a method, not a function.", i);
                errors.push_back(err);
                return;
            }
            functionCall(f, fp, result, warns, errors, body, i);
        } else if (hasContainer(result, body[i].getValue())) {
            std::string containerName = body[i].getValue();
            safeInc();
            if (body[i].getType() != tok_dot) {
                transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            safeInc();
            std::string memberName = body[i].getValue();
            Container container = getContainerByName(result, containerName);
            if (!container.hasMember(memberName)) {
                transpilerError("Unknown container member: '" + memberName + "'", i);
                errors.push_back(err);
                return;
            }
            if (removeTypeModifiers(container.getMemberType(memberName)) == "float") {
                append("(_stack.sp++)->f = Container_%s.%s;\n", containerName.c_str(), memberName.c_str());
            } else {
                append("(_stack.sp++)->i = (scl_int) Container_%s.%s;\n", containerName.c_str(), memberName.c_str());
            }
            typeStack.push(container.getMemberType(memberName));
        } else if (getStructByName(result, body[i].getValue()) != Struct::Null) {
            Struct s = getStructByName(result, body[i].getValue());
            std::map<std::string, std::string> templates;
            if (body[i + 1].getType() == tok_identifier && body[i + 1].getValue() == "<") {
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
                while (body[i].getValue() != ">") {
                    if (body[i].getType() == tok_paren_open || (body[i].getType() == tok_identifier && body[i].getValue() == "typeof")) {
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
                                if (!structExtends(result, g, f.getName())) {
                                    transpilerError("Struct '" + g.getName() + "' is not convertible to '" + f.getName() + "'", i);
                                    errors.push_back(err);
                                    return;
                                }
                            }
                        }
                        templates[t.first] = type.value;
                    }
                    safeInc();
                    if (body[i].getValue() == ",") {
                        safeInc();
                    }
                }
            }
            if (body[i + 1].getType() == tok_double_column) {
                safeInc();
                safeInc();
                Method* initMethod = getMethodByName(result, "init", s.getName());
                bool hasInitMethod = initMethod != nullptr;
                if (body[i].getValue() == "new" || body[i].getValue() == "default") {
                    if (hasInitMethod && initMethod->has_private) {
                        transpilerError("Direct instantiation of struct '" + s.getName() + "' is not allowed", i);
                        errors.push_back(err);
                        return;
                    }
                }
                if (body[i].getValue() == "new" && !s.isStatic()) {
                    append("{\n");
                    scopeDepth++;
                    append("(_stack.sp++)->v = ALLOC(%s);\n", s.getName().c_str());
                    append("_scl_struct_allocation_failure((_stack.sp - 1)->i, \"%s\");\n", s.getName().c_str());
                    append("%s tmp = (_stack.sp - 1)->v;\n", sclTypeToCType(result, s.getName()).c_str());
                    for (auto&& t : templates) {
                        if (t.second.front() == '$') {
                            append("tmp->$template_arg_%s = %s->hash;\n", t.first.c_str(), t.second.c_str());
                            append("tmp->$template_argname_%s = %s->data;\n", t.first.c_str(), t.second.c_str());
                        } else {
                            append("tmp->$template_arg_%s = 0x%xU;\n", t.first.c_str(), id((char*) t.second.c_str()));
                            append("tmp->$template_argname_%s = \"%s\";\n", t.first.c_str(), t.second.c_str());
                        }
                    }
                    
                    typeStack.push(s.getName());
                    if (hasInitMethod) {
                        methodCall(initMethod, fp, result, warns, errors, body, i);
                        append("(_stack.sp++)->v = tmp;\n");
                        typeStack.push(s.getName());
                    }
                    scopeDepth--;
                    append("}\n");
                } else if (body[i].getValue() == "default" && !s.isStatic()) {
                    append("(_stack.sp++)->v = ALLOC(%s);\n", s.getName().c_str());
                    append("_scl_struct_allocation_failure((_stack.sp - 1)->i, \"%s\");\n", s.getName().c_str());
                    if (templates.size()) {
                        append("{\n");
                        scopeDepth++;
                        append("%s tmp = (_stack.sp - 1)->v;\n", sclTypeToCType(result, s.getName()).c_str());
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
                    typeStack.push(s.getName());
                } else {
                    if (hasFunction(result, s.getName() + "$" + body[i].getValue())) {
                        Function* f = getFunctionByName(result, s.getName() + "$" + body[i].getValue());
                        if (f->isCVarArgs()) {
                            createVariadicCall(f, fp, result, errors, body, i);
                            return;
                        }
                        if (f->isMethod) {
                            transpilerError("'" + f->getName() + "' is not static", i);
                            errors.push_back(err);
                            return;
                        }
                        if (f->isPrivate && function->belongsToType(s.getName())) {
                            if (f->member_type != function->member_type) {
                                transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                        }
                        functionCall(f, fp, result, warns, errors, body, i);
                    } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                        std::string loadFrom = s.getName() + "$" + body[i].getValue();
                        const Variable& v = getVar(loadFrom);
                        if (!v.isAccessible(currentFunction)) {
                            transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (removeTypeModifiers(v.getType()) == "float") {
                            append("(_stack.sp++)->f = Var_%s;\n", loadFrom.c_str());
                        } else {
                            append("(_stack.sp++)->i = (scl_int) Var_%s;\n", loadFrom.c_str());
                        }
                        typeStack.push(v.getType());
                    } else {
                        transpilerError("Unknown static member of struct '" + s.getName() + "'", i);
                        errors.push_back(err);
                    }
                }
            } else if (body[i + 1].getType() == tok_curly_open) {
                safeInc();
                append("{\n");
                scopeDepth++;
                size_t begin = i - 1;
                append("scl_%s tmp = ALLOC(%s);\n", s.getName().c_str(), s.getName().c_str());
                append("_scl_struct_allocation_failure(*(scl_int*) &tmp, \"%s\");\n", s.getName().c_str());
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
                    Variable v("super", getVar("self").getType());
                    varScopeTop().push_back(v);
                    append("scl_%s Var_super = Var_self;\n", removeTypeModifiers(getVar("self").getType()).c_str());
                }
                append("scl_%s Var_self = tmp;\n", s.getName().c_str());
                varScopeTop().push_back(Variable("self", "mut " + s.getName()));
                std::vector<std::string> missedMembers;

                size_t membersToInitialize = 0;
                for (auto&& v : s.getMembers()) {
                    if (v.getName().front() != '$') {
                        missedMembers.push_back(v.getName());
                        membersToInitialize++;
                    }
                }

                while (body[i].getType() != tok_curly_close) {
                    if (body[i].getType() == tok_paren_open) {
                        handle(ParenOpen);
                    } else {
                        handle(Token);
                    }
                    safeInc();
                    if (body[i].getType() != tok_store) {
                        transpilerError("Expected store, but got '" + body[i].getValue() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    safeInc();
                    if (body[i].getType() != tok_identifier) {
                        transpilerError("'" + body[i].getValue() + "' is not an identifier", i);
                        errors.push_back(err);
                        return;
                    }

                    std::string lastType = typeStackTop;

                    missedMembers = vecWithout(missedMembers, body[i].getValue());
                    
                    if (hasMethod(result, "attribute_mutator$" + body[i].getValue(), s.getName())) {
                        if (removeTypeModifiers(lastType) == "float")
                            append("Method_%s$attribute_mutator$%s(tmp, (--_stack.sp)->f);\n", s.getName().c_str(), body[i].getValue().c_str());
                        else
                            append("Method_%s$attribute_mutator$%s(tmp, (%s) (--_stack.sp)->i);\n", s.getName().c_str(), body[i].getValue().c_str(), sclTypeToCType(result, lastType).c_str());
                    } else {
                        if (removeTypeModifiers(lastType) == "float")
                            append("tmp->%s = (--_stack.sp)->f;\n", body[i].getValue().c_str());
                        else
                            append("tmp->%s = (%s) (--_stack.sp)->i;\n", body[i].getValue().c_str(), sclTypeToCType(result, lastType).c_str());
                    }
                    typePop;
                    append("_stack.sp = stack_start;\n");
                    safeInc();
                    count++;
                }
                varScopePop();
                append("(_stack.sp++)->v = tmp;\n");
                scopeDepth--;
                append("}\n");
                typeStack.push(s.getName());
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
        } else if (hasVar(body[i].getValue())) {
            std::string loadFrom = body[i].getValue();
            const Variable& v = getVar(body[i].getValue());
            if (removeTypeModifiers(v.getType()) == "float") {
                append("(_stack.sp++)->f = Var_%s;\n", loadFrom.c_str());
            } else {
                append("(_stack.sp++)->i = (scl_int) Var_%s;\n", loadFrom.c_str());
            }
            typeStack.push(v.getType());
        } else if (body[i].getValue() == "field" && (function->has_setter || function->has_getter)) {
            Struct s = getStructByName(result, function->member_type);
            std::string attribute = function->getModifier((function->has_setter ? function->has_setter : function->has_getter) + 1);
            const Variable& v = s.getMember(attribute);
            if (removeTypeModifiers(v.getType()) == "float") {
                append("(_stack.sp++)->f = Var_self->%s;\n", attribute.c_str());
            } else {
                append("(_stack.sp++)->i = (scl_int) Var_self->%s;\n", attribute.c_str());
            }
            typeStack.push(v.getType());
        } else if (hasVar(function->member_type + "$" + body[i].getValue())) {
            std::string loadFrom = function->member_type + "$" + body[i].getValue();
            const Variable& v = getVar(loadFrom);
            if (removeTypeModifiers(v.getType()) == "float") {
                append("(_stack.sp++)->f = Var_%s;\n", loadFrom.c_str());
            } else {
                append("(_stack.sp++)->i = (scl_int) Var_%s;\n", loadFrom.c_str());
            }
            typeStack.push(v.getType());
        } else if (function->isMethod) {
            Method* m = ((Method*) function);
            Struct s = getStructByName(result, m->getMemberType());
            if (hasMethod(result, body[i].getValue(), s.getName())) {
                Method* f = getMethodByName(result, body[i].getValue(), s.getName());
                append("(_stack.sp++)->i = (scl_int) Var_self;\n");
                typeStack.push(f->getMemberType());
                methodCall(f, fp, result, warns, errors, body, i);
            } else if (hasFunction(result, s.getName() + "$" + body[i].getValue())) {
                std::string struct_ = s.getName();
                Function* f = getFunctionByName(result, struct_ + "$" + body[i].getValue());
                if (f->isCVarArgs()) {
                    createVariadicCall(f, fp, result, errors, body, i);
                    return;
                }
                if (f->isMethod) {
                    transpilerError("'" + f->getName() + "' is not static", i);
                    errors.push_back(err);
                    return;
                }
                functionCall(f, fp, result, warns, errors, body, i);
            } else if (s.hasMember(body[i].getValue())) {
                Variable mem = s.getMember(body[i].getValue());

                if (mem.getType() == "float") {
                    append("(_stack.sp++)->f = Var_self->%s;\n", body[i].getValue().c_str());
                } else {
                    append("(_stack.sp++)->i = (scl_int) Var_self->%s;\n", body[i].getValue().c_str());
                }
                typeStack.push(mem.getType());
            } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                std::string loadFrom = s.getName() + "$" + body[i].getValue();
                const Variable& v = getVar(loadFrom);
                if (v.getType() == "float") {
                    append("(_stack.sp++)->f = Var_%s;\n", loadFrom.c_str());
                } else {
                    append("(_stack.sp++)->i = (scl_int) Var_%s;\n", loadFrom.c_str());
                }
                typeStack.push(v.getType());
            } else {
                transpilerError("Unknown member of struct '" + m->getMemberType() + "': '" + body[i].getValue() + "'", i);
                errors.push_back(err);
            }
        } else {
            transpilerError("Unknown identifier: '" + body[i].getValue() + "'", i);
            errors.push_back(err);
        }
    }

    handler(New) {
        noUnused;
        safeInc();
        std::string elemSize = "sizeof(scl_any)";
        std::string typeString = "any";
        if (body[i].getType() == tok_identifier && body[i].getValue() == "<") {
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
        while (body[i].getType() == tok_bracket_open) {
            dimensions++;
            startingOffsets.push_back(i);
            safeInc();
            if (body[i].getType() == tok_bracket_close) {
                transpilerError("Empty array dimensions are not allowed", i);
                errors.push_back(err);
                i--;
                return;
            }
            while (body[i].getType() != tok_bracket_close) {
                handle(Token);
                safeInc();
            }
            safeInc();
        }
        i--;
        if (dimensions == 0) {
            transpilerError("Expected '[', but got '" + body[i].getValue() + "'", i);
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
        if (body[i].getType() != tok_number) {
            transpilerError("Expected integer, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            if (i + 1 < body.size() && body[i + 1].getType() != tok_do)
                goto lbl_repeat_do_tok_chk;
            safeInc();
            return;
        }
        if (i + 1 < body.size() && body[i + 1].getType() != tok_do) {
        lbl_repeat_do_tok_chk:
            transpilerError("Expected 'do', but got '" + body[i + 1].getValue() + "'", i+1);
            errors.push_back(err);
            safeInc();
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
        varScopePush();
        pushRepeat();
        safeInc();
    }

    handler(For) {
        noUnused;
        safeInc();
        Token var = body[i];
        if (var.getType() != tok_identifier) {
            transpilerError("Expected identifier, but got: '" + var.getValue() + "'", i);
            errors.push_back(err);
        }
        safeInc();
        if (body[i].getType() != tok_in) {
            transpilerError("Expected 'in' keyword in for loop header, but got: '" + body[i].getValue() + "'", i);
            errors.push_back(err);
        }
        safeInc();
        std::string var_prefix = "";
        if (!hasVar(var.getValue())) {
            var_prefix = "scl_int ";
        }
        append("for (%sVar_%s = ({\n", var_prefix.c_str(), var.getValue().c_str());
        scopeDepth++;
        while (body[i].getType() != tok_to) {
            handle(Token);
            safeInc();
        }
        if (body[i].getType() != tok_to) {
            transpilerError("Expected 'to', but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        safeInc();
        append("(--_stack.sp)->i;\n");
        typePop;
        scopeDepth--;
        append("}); Var_%s != ({\n", var.getValue().c_str());
        scopeDepth++;
        while (body[i].getType() != tok_step && body[i].getType() != tok_do) {
            handle(Token);
            safeInc();
        }
        typePop;
        std::string iterator_direction = "++";
        if (body[i].getType() == tok_step) {
            safeInc();
            if (body[i].getType() == tok_do) {
                transpilerError("Expected step, but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            std::string val = body[i].getValue();
            if (val == "+") {
                safeInc();
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
                safeInc();
                iterator_direction = " -= ";
                if (body[i].getType() == tok_number) {
                    iterator_direction += body[i].getValue();
                } else if (body[i].getValue() == "-") {
                    iterator_direction = "--";
                } else {
                    transpilerError("Expected number or '-', but got '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "++") {
                iterator_direction = "++";
            } else if (val == "--") {
                iterator_direction = "--";
            } else if (val == "*") {
                safeInc();
                iterator_direction = " *= ";
                if (body[i].getType() == tok_number) {
                    iterator_direction += body[i].getValue();
                } else {
                    transpilerError("Expected number, but got '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "/") {
                safeInc();
                iterator_direction = " /= ";
                if (body[i].getType() == tok_number) {
                    iterator_direction += body[i].getValue();
                } else {
                    transpilerError("Expected number, but got '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "<<") {
                safeInc();
                iterator_direction = " <<= ";
                if (body[i].getType() == tok_number) {
                    iterator_direction += body[i].getValue();
                } else {
                    transpilerError("Expected number, but got '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                }
            } else if (val == ">>") {
                safeInc();
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
            safeInc();
        }
        if (body[i].getType() != tok_do) {
            transpilerError("Expected 'do', but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        append("(--_stack.sp)->i;\n");
        scopeDepth--;
        if (iterator_direction == "") {
            append("});) {\n");
        } else
            append("}); Var_%s%s) {\n", var.getValue().c_str(), iterator_direction.c_str());
        varScopePush();
        if (!hasVar(var.getValue())) {
            varScopeTop().push_back(Variable(var.getValue(), "int"));
        }
        
        iterator_count++;
        scopeDepth++;

        if (hasFunction(result, var.getValue())) {
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
        if (hasContainer(result, var.getValue())) {
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

        pushOther();
    }

    handler(Foreach) {
        noUnused;
        safeInc();
        if (body[i].getType() != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        std::string iterator_name = "__it" + std::to_string(iterator_count++);
        Token iter_var_tok = body[i];
        safeInc();
        if (body[i].getType() != tok_in) {
            transpilerError("Expected 'in', but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        safeInc();
        if (body[i].getType() == tok_do) {
            transpilerError("Expected iterable, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        while (body[i].getType() != tok_do) {
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
            varScopeTop().push_back(Variable(iter_var_tok.getValue(), type.substr(1, type.size() - 2)));
            append("%s Var_%s = %s[i];\n", sclTypeToCType(result, type.substr(1, type.size() - 2)).c_str(), iter_var_tok.getValue().c_str(), iterator_name.c_str());
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
        if (!hasVar(iter_var_tok.getValue())) {
            var_prefix = sclTypeToCType(result, nextMethod->getReturnType()) + " ";
        }
        varScopePush();
        varScopeTop().push_back(Variable(iter_var_tok.getValue(), nextMethod->getReturnType()));
        std::string cType = sclTypeToCType(result, getVar(iter_var_tok.getValue()).getType());
        append("while (virtual_call(%s, \"hasNext()i;\")) {\n", iterator_name.c_str());
        scopeDepth++;
        append("%sVar_%s = (%s) virtual_call(%s, \"next%s\");\n", var_prefix.c_str(), iter_var_tok.getValue().c_str(), cType.c_str(), iterator_name.c_str(), argsToRTSignature(nextMethod).c_str());
    }

    std::string makePath(TPResult& result, Variable v, bool topLevelDeref, std::vector<Token>& body, size_t& i, std::vector<FPResult>& errors, std::string prefix, std::string* currentType, bool doesWriteAfter = true);

    handler(AddrRef) {
        noUnused;
        safeInc();
        Token toGet = body[i];

        Variable v("", "");
        std::string containerBegin = "";

        std::string lastType;
        std::string path;

        if (hasFunction(result, toGet.getValue())) {
            Function* f = getFunctionByName(result, toGet.getValue());
            if (f->isCVarArgs()) {
                transpilerError("Cannot take reference of varargs function '" + f->getName() + "'", i);
                errors.push_back(err);
                return;
            }
            append("(_stack.sp++)->i = (scl_int) &Function_%s;\n", f->finalName().c_str());
            std::string lambdaType = "lambda(" + std::to_string(f->getArgs().size()) + "):" + f->getReturnType();
            typeStack.push(lambdaType);
            return;
        }
        if (!hasVar(body[i].getValue())) {
            if (body[i + 1].getType() == tok_double_column) {
                safeInc();
                Struct s = getStructByName(result, body[i - 1].getValue());
                safeInc();
                if (s != Struct::Null) {
                    if (hasFunction(result, s.getName() + "$" + body[i].getValue())) {
                        Function* f = getFunctionByName(result, s.getName() + "$" + body[i].getValue());
                        if (f->isCVarArgs()) {
                            transpilerError("Cannot take reference of varargs function '" + f->getName() + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (f->isMethod) {
                            transpilerError("'" + f->getName() + "' is not static", i);
                            errors.push_back(err);
                            return;
                        }
                        append("(_stack.sp++)->i = (scl_int) &Function_%s;\n", f->finalName().c_str());
                        std::string lambdaType = "lambda(" + std::to_string(f->getArgs().size()) + "):" + f->getReturnType();
                        typeStack.push(lambdaType);
                        return;
                    } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                        std::string loadFrom = s.getName() + "$" + body[i].getValue();
                        v = getVar(loadFrom);
                    } else {
                        transpilerError("Struct '" + s.getName() + "' has no static member named '" + body[i].getValue() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                }
            } else if (hasContainer(result, body[i].getValue())) {
                Container c = getContainerByName(result, body[i].getValue());
                safeInc();
                safeInc();
                std::string memberName = body[i].getValue();
                if (!c.hasMember(memberName)) {
                    transpilerError("Unknown container member: '" + memberName + "'", i);
                    errors.push_back(err);
                    return;
                }
                containerBegin = "(Container_" + c.getName() + "." + body[i].getValue() + ")";
                v = c.getMember(memberName);
            } else if (function->isMethod) {
                Method* m = ((Method*) function);
                Struct s = getStructByName(result, m->getMemberType());
                if (s.hasMember(body[i].getValue())) {
                    v = s.getMember(body[i].getValue());
                } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                    v = getVar(s.getName() + "$" + body[i].getValue());
                }
            } else {
                transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (hasVar(body[i].getValue()) && body[i + 1].getType() == tok_double_column) {
            Struct s = getStructByName(result, getVar(body[i].getValue()).getType());
            safeInc();
            safeInc();
            if (s != Struct::Null) {
                if (hasMethod(result, body[i].getValue(), s.getName())) {
                    Method* f = getMethodByName(result, body[i].getValue(), s.getName());
                    if (!f->isMethod) {
                        transpilerError("'" + f->getName() + "' is static", i);
                        errors.push_back(err);
                        return;
                    }
                    append("(_stack.sp++)->i = (scl_int) &Method_%s$%s;\n", s.getName().c_str(), f->finalName().c_str());
                    std::string lambdaType = "lambda(" + std::to_string(f->getArgs().size()) + "):" + f->getReturnType();
                    typeStack.push(lambdaType);
                    return;
                } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                    std::string loadFrom = s.getName() + "$" + body[i].getValue();
                    v = getVar(loadFrom);
                } else {
                    transpilerError("Struct '" + s.getName() + "' has no static member named '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                    return;
                }
            }
        } else {
            v = getVar(body[i].getValue());
        }
        path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "", &lastType, /* doesWriteAfter */ false);
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
            path += v.getName();
        } else {
            path += "Var_" + v.getName();
        }
        *currentType = v.getType();
        if (topLevelDeref) {
            path = "(*" + path + ")";
            *currentType = typePointedTo(*currentType);
        }
        if (i + 1 >= body.size() || body[i + 1].getType() != tok_dot) {
            if (!v.isWritableFrom(currentFunction)) {
                transpilerError("Variable '" + v.getName() + "' is not mutable", i);
                errors.push_back(err);
                return std::string("(void) 0");
            }
            return path;
        }
        safeInc();
        while (i < body.size() && body[i].getType() == tok_dot) {
            safeInc();
            bool deref = body[i].getType() == tok_addr_of;
            if (deref) {
                safeInc();
            }
            Struct s = getStructByName(result, *currentType);
            Layout l = getLayout(result, *currentType);
            if (s == Struct::Null && l.getName().size() == 0) {
                transpilerError("Struct '" + *currentType + "' does not exist", i);
                errors.push_back(err);
                return std::string("(void) 0");
            }
            if (!s.hasMember(body[i].getValue()) && !l.hasMember(body[i].getValue())) {
                transpilerError("Struct '" + *currentType + "' has no member named '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return std::string("(void) 0");
            } else if (!s.hasMember(body[i].getValue())) {
                v = l.getMember(body[i].getValue());
            } else {
                v = s.getMember(body[i].getValue());
            }
            *currentType = v.getType();
            if (deref) {
                *currentType = typePointedTo(*currentType);
            }
            if (!v.isWritableFrom(currentFunction) && doesWriteAfter) {
                transpilerError("Variable '" + v.getName() + "' is not mutable", i);
                errors.push_back(err);
                return std::string("(void) 0");
            }
            if (!v.isAccessible(currentFunction)) {
                transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                errors.push_back(err);
                return std::string("(void) 0");
            }
            if (i + 1 >= body.size() || body[i + 1].getType() != tok_dot) {
                if (!v.isWritableFrom(currentFunction) && doesWriteAfter) {
                    transpilerError("Member '" + v.getName() + "' of struct '" + s.getName() + "' is not mutable", i);
                    errors.push_back(err);
                    return std::string("(void) 0");
                }
                path = path + "->" + body[i].getValue();
                if (deref) {
                    path = "(*" + path + ")";
                }
                return path;
            } else {
                path = path + "->" + body[i].getValue();
                if (deref) {
                    path = "(*" + path + ")";
                }
            }
            THIS_INCREMENT_IS_CHECKED;
        }
        i--;
        return path;
    }

    handler(Store) {
        noUnused;
        safeInc();        

        if (body[i].getType() == tok_paren_open) {
            append("{\n");
            scopeDepth++;
            append("scl_Array tmp = (scl_Array) (--_stack.sp)->i;\n");
            typePop;
            safeInc();
            int destructureIndex = 0;
            while (body[i].getType() != tok_paren_close) {
                if (body[i].getType() == tok_comma) {
                    safeInc();
                    continue;
                }
                if (body[i].getType() == tok_paren_close)
                    break;
                
                if (body[i].getType() == tok_addr_of) {
                    safeInc();
                    if (hasContainer(result, body[i].getValue())) {
                        std::string containerName = body[i].getValue();
                        safeInc();
                        if (body[i].getType() != tok_dot) {
                            transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        safeInc();
                        std::string memberName = body[i].getValue();
                        Container container = getContainerByName(result, containerName);
                        if (!container.hasMember(memberName)) {
                            transpilerError("Unknown container member: '" + memberName + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!container.getMember(memberName).isWritableFrom(function)) {
                            transpilerError("Container member variable '" + body[i].getValue() + "' is not mutable", i);
                            errors.push_back(err);
                            safeInc();
                            safeInc();
                            return;
                        }
                        append("*((scl_any*) Container_%s.%s) = Method_Array$get(tmp, %d);\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                    } else {
                        if (body[i].getType() != tok_identifier) {
                            transpilerError("'" + body[i].getValue() + "' is not an identifier", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!hasVar(body[i].getValue())) {
                            if (function->isMethod) {
                                Method* m = ((Method*) function);
                                Struct s = getStructByName(result, m->getMemberType());
                                if (s.hasMember(body[i].getValue())) {
                                    Variable mem = getVar(s.getName() + "$" + body[i].getValue());
                                    if (!mem.isWritableFrom(function)) {
                                        transpilerError("Variable '" + body[i].getValue() + "' is not mutable", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("*(scl_any*) Var_self->%s = Method_Array$get(tmp, %d);\n", body[i].getValue().c_str(), destructureIndex);
                                    destructureIndex++;
                                    continue;
                                } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                                    Variable mem = getVar(s.getName() + "$" + body[i].getValue());
                                    if (!mem.isWritableFrom(function)) {
                                        transpilerError("Variable '" + body[i].getValue() + "' is not mutable", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("*(scl_any*) Var_%s$%s = Method_Array$get(tmp, %d);\n", s.getName().c_str(), body[i].getValue().c_str(), destructureIndex);
                                    destructureIndex++;
                                    continue;
                                }
                            }
                            transpilerError("Use of undefined variable '" + body[i].getValue() + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        const Variable& v = getVar(body[i].getValue());
                        std::string loadFrom = v.getName();
                        if (getStructByName(result, v.getType()) != Struct::Null) {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is not mutable", i);
                                errors.push_back(err);
                                safeInc();
                                safeInc();
                                return;
                            }
                            safeInc();
                            if (body[i].getType() != tok_dot) {
                                append("*(scl_any*) Var_%s = Method_Array$get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                                destructureIndex++;
                                continue;
                            }
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i - 1].getValue() + "' is not mutable", i - 1);
                                errors.push_back(err);
                                safeInc();
                                return;
                            }
                            safeInc();
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
                            if (!mem.isWritableFrom(function)) {
                                transpilerError("Member variable '" + body[i].getValue() + "' is not mutable", i);
                                errors.push_back(err);
                                return;
                            }
                            if (!mem.isAccessible(currentFunction)) {
                                transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*(scl_any*) Var_%s->%s = Method_Array$get(tmp, %d);\n", loadFrom.c_str(), body[i].getValue().c_str(), destructureIndex);
                        } else {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*(scl_any*) Var_%s = Method_Array$get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                        }
                    }
                } else {
                    if (hasContainer(result, body[i].getValue())) {
                        std::string containerName = body[i].getValue();
                        safeInc();
                        if (body[i].getType() != tok_dot) {
                            transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        safeInc();
                        std::string memberName = body[i].getValue();
                        Container container = getContainerByName(result, containerName);
                        if (!container.hasMember(memberName)) {
                            transpilerError("Unknown container member: '" + memberName + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!container.getMember(memberName).isWritableFrom(function)) {
                            transpilerError("Container member variable '" + body[i].getValue() + "' is const", i);
                            errors.push_back(err);
                            return;
                        }
                        append("{\n");
                        scopeDepth++;
                        append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                        if (!typeCanBeNil(container.getMember(memberName).getType())) {
                            append("_scl_check_not_nil_store((scl_int) tmp%d, \"%s.%s\");\n", destructureIndex, containerName.c_str(), memberName.c_str());
                        }
                        append("(*(scl_any*) &Container_%s.%s) = tmp%d;\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                        scopeDepth--;
                        append("}\n");
                    } else {
                        if (body[i].getType() != tok_identifier) {
                            transpilerError("'" + body[i].getValue() + "' is not an identifier", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!hasVar(body[i].getValue())) {
                            if (function->isMethod) {
                                Method* m = ((Method*) function);
                                Struct s = getStructByName(result, m->getMemberType());
                                if (s.hasMember(body[i].getValue())) {
                                    Variable mem = s.getMember(body[i].getValue());
                                    if (!mem.isWritableFrom(function)) {
                                        transpilerError("Member variable '" + body[i].getValue() + "' is const", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("{\n");
                                    scopeDepth++;
                                    append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                                    if (!typeCanBeNil(mem.getType())) {
                                        append("_scl_check_not_nil_store((scl_int) tmp%d, \"self.%s\");\n", destructureIndex, mem.getName().c_str());
                                    }
                                    append("*(scl_any*) &Var_self->%s = tmp%d;\n", body[i].getValue().c_str(), destructureIndex);
                                    scopeDepth--;
                                    append("}\n");
                                    destructureIndex++;
                                    continue;
                                } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                                    if (!getVar(s.getName() + "$" + body[i].getValue()).isWritableFrom(function)) {
                                        transpilerError("Member variable '" + body[i].getValue() + "' is const", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("{\n");
                                    scopeDepth++;
                                    append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                                    if (!typeCanBeNil(getVar(s.getName() + "$" + body[i].getValue()).getType())) {
                                        append("_scl_check_not_nil_store((scl_int) tmp%d, \"%s::%s\");\n", destructureIndex, s.getName().c_str(), body[i].getValue().c_str());
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
                        const Variable& v = getVar(body[i].getValue());
                        std::string loadFrom = v.getName();
                        if (getStructByName(result, v.getType()) != Struct::Null) {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is const", i);
                                errors.push_back(err);
                                safeInc();
                                safeInc();
                                return;
                            }
                            safeInc();
                            if (body[i].getType() != tok_dot) {
                                append("{\n");
                                scopeDepth++;
                                append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                                if (!typeCanBeNil(v.getType())) {
                                    append("_scl_check_not_nil_store((scl_int) tmp%d, \"%s\");\n", destructureIndex, v.getName().c_str());
                                }
                                append("(*(scl_any*) &Var_%s) = tmp%d;\n", loadFrom.c_str(), destructureIndex);
                                scopeDepth--;
                                append("}\n");
                                destructureIndex++;
                                continue;
                            }
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i - 1].getValue() + "' is not mutable", i - 1);
                                errors.push_back(err);
                                safeInc();
                                return;
                            }
                            safeInc();
                            Struct s = getStructByName(result, v.getType());
                            if (!s.hasMember(body[i].getValue())) {
                                transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            Variable mem = s.getMember(body[i].getValue());
                            if (!mem.isWritableFrom(function)) {
                                transpilerError("Member variable '" + body[i].getValue() + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            if (!mem.isAccessible(currentFunction)) {
                                transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            append("{\n");
                            scopeDepth++;
                            append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                            if (!typeCanBeNil(mem.getType())) {
                                append("_scl_check_not_nil_store((scl_int) tmp%d, \"%s.%s\");\n", destructureIndex, s.getName().c_str(), mem.getName().c_str());
                            }
                            if (!typeCanBeNil(mem.getType())) {
                                append("_scl_check_not_nil_store((scl_int) tmp%d, \"self.%s\");\n", destructureIndex, mem.getName().c_str());
                            }
                            append("(*(scl_any*) &Var_%s->%s) = tmp%d;\n", loadFrom.c_str(), body[i].getValue().c_str(), destructureIndex);
                            scopeDepth--;
                            append("}\n");
                        } else {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].getValue() + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            append("{\n");
                            scopeDepth++;
                            append("scl_any tmp%d = Method_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                            if (!typeCanBeNil(v.getType())) {
                                append("_scl_check_not_nil_store((scl_int) tmp%d, \"%s\");\n", destructureIndex, v.getName().c_str());
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
        } else if (body[i].getType() == tok_declare) {
            safeInc();
            if (body[i].getType() != tok_identifier) {
                transpilerError("'" + body[i].getValue() + "' is not an identifier", i+1);
                errors.push_back(err);
                return;
            }
            if (hasFunction(result, body[i].getValue())) {
                {
                    transpilerError("Variable '" + body[i].getValue() + "' shadowed by function '" + body[i].getValue() + "'", i);
                    if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                    else errors.push_back(err);
                }
            }
            if (hasContainer(result, body[i].getValue())) {
                {
                    transpilerError("Variable '" + body[i].getValue() + "' shadowed by container '" + body[i].getValue() + "'", i);
                    if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                    else errors.push_back(err);
                }
            }
            if (getStructByName(result, body[i].getValue()) != Struct::Null) {
                {
                    transpilerError("Variable '" + body[i].getValue() + "' shadowed by struct '" + body[i].getValue() + "'", i);
                    if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                    else errors.push_back(err);
                }
            }
            std::string name = body[i].getValue();
            std::string type = "any";
            safeInc();
            if (body[i].getType() == tok_column) {
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
            if (!varScopeTop().back().canBeNil) {
                append("_scl_check_not_nil_store((_stack.sp - 1)->i, \"%s\");\n", varScopeTop().back().getName().c_str());
            }
            append("%s Var_%s = (%s) (--_stack.sp)->%s;\n", sclTypeToCType(result, type).c_str(), name.c_str(), sclTypeToCType(result, type).c_str(), removeTypeModifiers(type) == "float" ? "f" : "v");
            typePop;
        } else {
            if (body[i].getType() != tok_identifier && body[i].getType() != tok_addr_of) {
                transpilerError("'" + body[i].getValue() + "' is not an identifier", i);
                errors.push_back(err);
            }
            if (function->has_setter || function->has_getter) {
                if (body[i].getValue() == "field") {
                    Struct s = getStructByName(result, function->member_type);
                    std::string attribute = function->getModifier((function->has_setter ? function->has_setter : function->has_getter) + 1);
                    const Variable& v = s.getMember(attribute);
                    if (removeTypeModifiers(v.getType()) == "float") {
                        append("Var_self->%s = (--_stack.sp)->f;\n", attribute.c_str());
                    } else {
                        append("Var_self->%s = (%s) (--_stack.sp)->i;\n", attribute.c_str(), sclTypeToCType(result, v.getType()).c_str());
                    }
                    typeStack.pop();
                    return;
                }
            }
            Variable v("", "");
            std::string containerBegin = "";
            std::string getStaticVar = "";
            std::string lastType = "";

            std::string variablePrefix = "";

            bool topLevelDeref = false;

            if (body[i].getType() == tok_addr_of) {
                safeInc();
                topLevelDeref = true;
                if (body[i].getType() == tok_paren_open) {
                    if (isInUnsafe) {
                        safeInc();
                        append("{\n");
                        scopeDepth++;
                        append("scl_int _scl_value_to_store = (--_stack.sp)->i;\n");
                        typePop;
                        append("{\n");
                        scopeDepth++;
                        while (body[i].getType() != tok_paren_close) {
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

            if (hasVar(body[i].getValue())) {
            normalVar:
                v = getVar(body[i].getValue());
            } else if (hasContainer(result, body[i].getValue())) {
                Container c = getContainerByName(result, body[i].getValue());
                safeInc(); // i -> .
                safeInc(); // i -> member
                std::string memberName = body[i].getValue();
                if (!c.hasMember(memberName)) {
                    transpilerError("Unknown container member: '" + memberName + "'", i);
                    errors.push_back(err);
                    return;
                }
                variablePrefix = "Container_" + c.getName() + ".";
                v = c.getMember(memberName);
            } else if (hasVar(function->member_type + "$" + body[i].getValue())) {
                v = getVar(function->member_type + "$" + body[i].getValue());
            } else if (body[i + 1].getType() == tok_double_column) {
                Struct s = getStructByName(result, body[i].getValue());
                safeInc(); // i -> ::
                safeInc(); // i -> member
                if (s != Struct::Null) {
                    if (!hasVar(s.getName() + "$" + body[i].getValue())) {
                        transpilerError("Struct '" + s.getName() + "' has no static member named '" + body[i].getValue() + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    v = getVar(s.getName() + "$" + body[i].getValue());
                    getStaticVar = s.getName();
                } else {
                    transpilerError("Struct '" + body[i].getValue() + "' does not exist", i);
                    errors.push_back(err);
                    return;
                }
            } else if (function->isMethod) {
                Method* m = ((Method*) function);
                Struct s = getStructByName(result, m->getMemberType());
                if (s.hasMember(body[i].getValue())) {
                    v = s.getMember(body[i].getValue());
                    Token here = body[i];
                    body.insert(body.begin() + i, Token(tok_dot, ".", here.getLine(), here.getFile()));
                    body.insert(body.begin() + i, Token(tok_identifier, "self", here.getLine(), here.getFile()));
                    goto normalVar;
                } else if (hasGlobal(result, s.getName() + "$" + body[i].getValue())) {
                    v = getVar(s.getName() + "$" + body[i].getValue());
                    getStaticVar = s.getName();
                } else {
                    transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                    return;
                }
            } else {
                transpilerError("Unknown variable: '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }

            std::string currentType;
            std::string path = makePath(result, v, topLevelDeref, body, i, errors, variablePrefix, &currentType);
            // allow for => x[...], but not => x[(...)]
            if (i + 1 < body.size() && body[i + 1].getType() == tok_bracket_open && (i + 2 >= body.size() || body[i + 2].getType() != tok_paren_open)) {
                i = start;
                append("{\n");
                scopeDepth++;
                append("scl_any value = (--_stack.sp)->v;\n");
                std::string valueType = removeTypeModifiers(typeStackTop);
                typePop;

                append("{\n");
                scopeDepth++;
                while (body[i].getType() != tok_bracket_open) {
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
                while (body[i].getType() != tok_bracket_close) {
                    handle(Token);
                    safeInc();
                }
                scopeDepth--;
                append("}\n");
                std::string indexingType = "";
                bool multiDimensional = false;
                if (i + 1 < body.size() && body[i + 1].getType() == tok_bracket_open) {
                    multiDimensional = true;
                    openBracket = i + 1;
                    safeIncN(2);
                }
                if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                    indexingType = "int";
                } else if (hasMethod(result, (multiDimensional ? "[]" : "=>[]"), arrayType)) {
                    Method* m = getMethodByName(result, (multiDimensional ? "[]" : "=>[]"), arrayType);
                    indexingType = m->getArgs()[0].getType();
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
                            if (m->getArgs().size() != 2) {
                                transpilerError("Method '[]' of type '" + arrayType + "' must have exactly 1 argument! Signature should be: '[](index: " + m->getArgs()[0].getType() + ")'", i);
                                errors.push_back(err);
                                return;
                            }
                            if (removeTypeModifiers(m->getReturnType()) == "none" || removeTypeModifiers(m->getReturnType()) == "nothing") {
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
                        if (m->getArgs().size() != 3) {
                            transpilerError("Method '=>[]' of type '" + arrayType + "' does not take 2 arguments! Signature should be: '=>[](index: " + indexType + ", value: " + valueType + ")'", openBracket);
                            errors.push_back(err);
                            return;
                        }
                        if (removeTypeModifiers(m->getReturnType()) != "none") {
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
                append("_scl_check_not_nil_store(*(scl_int*) (_stack.sp - 1), \"%s\");\n", v.getName().c_str());
            }
            append("%s = *(%s*) --_stack.sp;\n", path.c_str(), sclTypeToCType(result, currentType).c_str());
            typePop;
            scopeDepth--;
            append("}\n");
        }
    }

    handler(Declare) {
        noUnused;
        safeInc();
        if (body[i].getType() != tok_identifier) {
            transpilerError("'" + body[i].getValue() + "' is not an identifier", i+1);
            errors.push_back(err);
            return;
        }
        if (hasFunction(result, body[i].getValue())) {
            transpilerError("Variable '" + body[i].getValue() + "' shadowed by function '" + body[i].getValue() + "'", i+1);
            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
            else errors.push_back(err);
        }
        if (hasContainer(result, body[i].getValue())) {
            transpilerError("Variable '" + body[i].getValue() + "' shadowed by container '" + body[i].getValue() + "'", i+1);
            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
            else errors.push_back(err);
        }
        if (getStructByName(result, body[i].getValue()) != Struct::Null) {
            transpilerError("Variable '" + body[i].getValue() + "' shadowed by struct '" + body[i].getValue() + "'", i+1);
            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
            else errors.push_back(err);
        }
        std::string name = body[i].getValue();
        size_t start = i;
        std::string type = "any";
        safeInc();
        if (body[i].getType() == tok_column) {
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
        if (f->getReturnType().size() > 0 && f->getReturnType() != "none" && f->getReturnType() != "nothing") {
            if (f->getReturnType() == "float") {
                append("(_stack.sp++)->f = Method_Array$init(tmp, 1);\n");
            } else {
                append("(_stack.sp++)->i = (scl_int) Method_Array$init(tmp, 1);\n");
            }
            typeStack.push(f->getReturnType());
        } else {
            append("Method_Array$init(tmp, 1);\n");
        }
        safeInc();
        while (body[i].getType() != tok_curly_close) {
            if (body[i].getType() == tok_comma) {
                safeInc();
                continue;
            }
            bool didPush = false;
            while (body[i].getType() != tok_comma && body[i].getType() != tok_curly_close) {
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
            if (body[i + 1].getType() == tok_paren_open) {
                safeIncN(2);
                append("{\n");
                scopeDepth++;
                bool existingArrayUsed = false;
                if (body[i].getType() == tok_number && body[i + 1].getType() == tok_store) {
                    long long array_size = std::stoll(body[i].getValue());
                    if (array_size < 1) {
                        transpilerError("Array size must be positive", i);
                        errors.push_back(err);
                        return;
                    }
                    append("scl_int array_size = %s;\n", body[i].getValue().c_str());
                    safeIncN(2);
                } else {
                    while (body[i].getType() != tok_store) {
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
                while (body[i].getType() != tok_paren_close) {
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
                if (body[i].getType() != tok_bracket_close) {
                    transpilerError("Expected ']', but got '" + body[i].getValue() + "'", i);
                    errors.push_back(err);
                    return;
                }
                return;
            }
        }
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            if ((i + 2) < body.size()) {
                if (body[i + 1].getType() == tok_number && body[i + 2].getType() == tok_bracket_close) {
                    safeIncN(2);
                    long long index = std::stoll(body[i - 1].getValue());
                    if (index < 0) {
                        transpilerError("Array subscript cannot be negative", i - 1);
                        errors.push_back(err);
                        return;
                    }
                    append("_scl_array_check_bounds_or_throw((_stack.sp - 1)->v, %s);\n", body[i - 1].getValue().c_str());
                    append("(_stack.sp - 1)->v = ((%s) (_stack.sp - 1)->v)[%s];\n", sclTypeToCType(result, type).c_str(), body[i - 1].getValue().c_str());
                    typePop;
                    typeStack.push(type.substr(1, type.size() - 2));
                    return;
                }
            }
            safeInc();
            if (body[i].getType() == tok_bracket_close) {
                transpilerError("Array subscript required", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            scopeDepth++;
            append("%s tmp = (scl_any*) (--_stack.sp)->v;\n", sclTypeToCType(result, type).c_str());
            typePop;
            varScopePush();
            while (body[i].getType() != tok_bracket_close) {
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
            if (m->getArgs().size() != 2) {
                transpilerError("Method '[]' of type '" + type + "' must have exactly 1 argument! Signature should be: '[](index: " + m->getArgs()[0].getType() + ")'", i);
                errors.push_back(err);
                return;
            }
            if (removeTypeModifiers(m->getReturnType()) == "none" || removeTypeModifiers(m->getReturnType()) == "nothing") {
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
            if (body[i].getType() == tok_bracket_close) {
                transpilerError("Empty array subscript", i);
                errors.push_back(err);
                return;
            }
            while (body[i].getType() != tok_bracket_close) {
                handle(Token);
                safeInc();
            }
            varScopePop();
            if (!typeEquals(removeTypeModifiers(typeStackTop), removeTypeModifiers(m->getArgs()[0].getType()))) {
                transpilerError("'" + m->getMemberType() + "' cannot be indexed with '" + removeTypeModifiers(typeStackTop) + "'", i);
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
        while (body[i].getType() != tok_paren_close) {
            if (body[i].getType() == tok_comma) {
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
            if (f->getReturnType().size() > 0 && f->getReturnType() != "none" && f->getReturnType() != "nothing") {
                if (f->getReturnType() == "float") {
                    append("(_stack.sp++)->f = Method_Pair$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v);\n");
                } else {
                    append("(_stack.sp++)->i = (scl_int) Method_Pair$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v);\n");
                }
                typeStack.push(f->getReturnType());
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
            if (f->getReturnType().size() > 0 && f->getReturnType() != "none" && f->getReturnType() != "nothing") {
                if (f->getReturnType() == "float") {
                    append("(_stack.sp++)->f = Method_Triple$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v, _scl_positive_offset(2)->v);\n");
                } else {
                    append("(_stack.sp++)->i = (scl_int) Method_Triple$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v, _scl_positive_offset(2)->v);\n");
                }
                typeStack.push(f->getReturnType());
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
            if (body[i + 1].getType() == tok_column) {
                if (body[i + 2].getType() == tok_identifier) {
                    if (body[i + 2].getValue() == "view") {
                        append("(_stack.sp++)->v = \"%s\";\n", body[i].getValue().c_str());
                        typeStack.push("[int8]");
                        safeInc();
                        safeInc();
                        return;
                    }
                }
            } else if (body[i + 1].getType() == tok_identifier) {
            stringLiteral_IdentifierAfter:
                if (body[i + 1].getValue() == "puts") {
                    append("puts(\"%s\");\n", body[i].getValue().c_str());
                    safeInc();
                    return;
                } else if (body[i + 1].getValue() == "eputs") {
                    append("fprintf(stderr, \"%s\\n\");\n", body[i].getValue().c_str());
                    safeInc();
                    return;
                }
            }
        } else if (i + 1 < body.size()) {
            if (body[i + 1].getType() == tok_identifier) {
                goto stringLiteral_IdentifierAfter;
            }
        }
        append("(_stack.sp++)->s = _scl_create_string(\"%s\");\n", body[i].getValue().c_str());
        typeStack.push("str");
    }

    handler(CharStringLiteral) {
        noUnused;
        append("(_stack.sp++)->v = \"%s\";\n", body[i].getValue().c_str());
        typeStack.push("[int8]");
    }

    handler(CDecl) {
        noUnused;
        safeInc();
        if (body[i].getType() != tok_string_literal) {
            transpilerError("Expected string literal, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        }
        std::string s = replaceAll(body[i].getValue(), R"(\\\\)", "\\");
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
        append("*(_stack.tp - 1) = \"<%s:native code>\";\n", function->getName().c_str());
        std::string file = body[i].getFile();
        if (strstarts(file, scaleFolder)) {
            file = file.substr(scaleFolder.size() + std::string("/Frameworks/").size());
        } else {
            file = std::filesystem::path(file).relative_path();
        }
        std::string ext = body[i].getValue();
        for (const Variable& v : vars) {
            append("%s* %s = &Var_%s;\n", sclTypeToCType(result, v.getType()).c_str(), v.getName().c_str(), v.getName().c_str());
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
        std::string stringValue = body[i].getValue();

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
            std::stod(body[i].getValue());
        } catch (const std::invalid_argument& e) {
            transpilerError("Invalid float literal: '" + body[i].getValue() + "'", i);
            errors.push_back(err);
            return;
        } catch (const std::out_of_range& e) {
            transpilerError("Integer literal out of range: '" + body[i].getValue() + "' " + e.what(), i);
            errors.push_back(err);
            return;
        }
        append("(_stack.sp++)->f = %s;\n", body[i].getValue().c_str());
        typeStack.push("float");
    }

    handler(FalsyType) {
        noUnused;
        append("(_stack.sp++)->i = (scl_int) 0;\n");
        if (body[i].getType() == tok_nil) {
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
            transpilerError("Usage of undeclared struct '" + body[i].getValue() + "'", i);
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
        while (body[i].getType() != tok_then) {
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
        while (!tokenEndsIfBlock(body[i].getType())) {
            handle(Token);
            safeInc();
        }
        if (typeStackTop.size() > beginningStackSize) {
            returnedTypes.push_back(typeStackTop);
        }
        while (typeStack.size() > beginningStackSize) {
            typePop;
        }
        if (body[i].getType() != tok_fi) {
            if (body[i].getType() == tok_else) {
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
        while (body[i].getType() != tok_then) {
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
        while (!tokenEndsIfBlock(body[i].getType())) {
            handle(Token);
            safeInc();
        }
        if (ifBlockReturnType == invalidType && typeStackTop.size()) {
            ifBlockReturnType = typeStackTop;
        } else if (typeStackTop.size() && ifBlockReturnType != typeStackTop) {
            problematicType = "[ " + ifBlockReturnType + ", " + typeStackTop + " ]";
            ifBlockReturnType = "---";
        }
        if (body[i].getType() != tok_fi) {
            handle(Token);
        }
        if (body[i].getType() == tok_else) {
            exhaustive = true;
        }
        while (typeStack.size() > beginningStackSize) {
            typePop;
        }
        if (body[i].getType() != tok_fi) {
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
        while (body[i].getType() != tok_then) {
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
        while (body[i].getType() != tok_then) {
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
        while (body[i].getType() != tok_do) {
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

        if (function->getReturnType() == "nothing") {
            transpilerError("Cannot return from a function with return type 'nothing'", i);
            errors.push_back(err);
            return;
        }

        append("{\n");
        scopeDepth++;

        if (function->getReturnType() != "none" && !function->hasNamedReturnValue) {
            append("_scl_frame_t* returnFrame = (--_stack.sp);\n");
        }
        if (function->has_restrict) {
            if (function->isMethod) {
                append("Function_Process$unlock((scl_SclObject) Var_self);\n");
            } else {
                append("Function_Process$unlock(function_lock$%s);\n", function->finalName().c_str());
            }
        }
        if (function->hasNamedReturnValue) {
            if (function->getNamedReturnValue().typeFromTemplate.size()) {
                append("_scl_checked_cast(*(scl_any*) &Var_%s, Var_self->$template_arg_%s, Var_self->$template_argname_%s);\n", function->getNamedReturnValue().getName().c_str(), function->getNamedReturnValue().typeFromTemplate.c_str(), function->getNamedReturnValue().typeFromTemplate.c_str());
            }
        } else {
            if (function->templateArg.size()) {
                append("_scl_checked_cast(returnFrame->v, Var_self->$template_arg_%s, Var_self->$template_argname_%s);\n", function->templateArg.c_str(), function->templateArg.c_str());
            }
        }
        append("--_stack.tp;\n");
        append("_stack.sp = __current_base_ptr;\n");

        if (function->getReturnType() != "none" && function->getReturnType() != "nothing") {
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
                    append("_scl_not_nil_return(*(scl_int*) &Var_%s, \"%s\");\n", function->getNamedReturnValue().getName().c_str(), function->getReturnType().c_str());
                else
                    append("_scl_not_nil_return(returnFrame->i, \"%s\");\n", function->getReturnType().c_str());
            }
        }

        auto type = removeTypeModifiers(function->getReturnType());

        if (type == "none")
            append("return;\n");
        else {
            if (function->hasNamedReturnValue) {
                std::string type = sclTypeToCType(result, function->getNamedReturnValue().getType());
                append("return (%s) Var_%s;\n", type.c_str(), function->getNamedReturnValue().getName().c_str());
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
                    append("return (%s) returnFrame->i;\n", sclTypeToCType(result, function->getReturnType()).c_str());
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
        if (body[i].getType() != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
        }
        append("goto %s;\n", body[i].getValue().c_str());
    }

    handler(Label) {
        noUnused;
        safeInc();
        if (body[i].getType() != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
            errors.push_back(err);
        }
        append("%s:\n", body[i].getValue().c_str());
    }

    handler(Case) {
        noUnused;
        safeInc();
        if (!wasSwitch()) {
            transpilerError("Unexpected 'case' statement", i);
            errors.push_back(err);
            return;
        }
        if (body[i].getType() == tok_string_literal) {
            transpilerError("String literal in case statement is not supported", i);
            errors.push_back(err);
        } else {
            append("case %s: {\n", body[i].getValue().c_str());
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
        if (type.value == "int" || type.value == "float" || type.value == "any") {
            typePop;
            typeStack.push(type.value);
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
        if (i + 1 < body.size() && body[i + 1].getValue() == "?") {
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
            std::string member = body[i].getValue();
            if (!l.hasMember(member)) {
                transpilerError("No layout for '" + member + "' in '" + type + "'", i);
                errors.push_back(err);
                return;
            }
            const Variable& v = l.getMember(member);

            if (removeTypeModifiers(v.getType()) == "float") {
                append("(_stack.sp - 1)->f = ((%s) (_stack.sp - 1)->i)->%s;\n", sclTypeToCType(result, l.getName()).c_str(), member.c_str());
            } else {
                append("(_stack.sp - 1)->i = (scl_int) ((%s) (_stack.sp - 1)->i)->%s;\n", sclTypeToCType(result, l.getName()).c_str(), member.c_str());
            }
            typeStack.push(l.getMember(member).getType());
            return;
        }
        std::string tmp = removeTypeModifiers(type);
        if (tmp.size() > 2 && tmp.front() == '[' && tmp.back() == ']') {
            safeInc();
            if (body[i].getType() != tok_identifier) {
                transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            std::string member = body[i].getValue();
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
        bool deref = body[i + 1].getType() == tok_addr_of;
        safeIncN(deref + 1);
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
                append("scl_any tmp = (--_stack.sp)->v;\n");
                std::string t = type;
                append("(_stack.sp++)->s = _scl_create_string(\"%s\");\n", body[i].getValue().c_str());
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
            if ((m = getMethodByName(result, body[i].getValue(), s.getName())) != nullptr) {
                std::string lambdaType = "lambda(" + std::to_string(m->getArgs().size()) + "):" + m->getReturnType();
                append("(_stack.sp++)->v = Method_%s$%s;\n", s.getName().c_str(), m->getName().c_str());
                typeStack.push(lambdaType);
                return;
            }
            transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'" + help, i);
            errors.push_back(err);
            return;
        }
        Variable mem = s.getMember(body[i].getValue());
        if (!mem.isAccessible(currentFunction)) {
            if (s.getName() != function->member_type) {
                transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                errors.push_back(err);
                return;
            }
        }
        if (typeCanBeNil(type) && dot.getValue() != "?.") {
            {
                transpilerError("Member access on maybe-nil type '" + type + "'", i);
                errors.push_back(err);
            }
            transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this '.'\\insertText;!!;" + std::to_string(err.line) + ":" + std::to_string(err.column), i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }

        if (dot.getValue() == "?.") {
            if (removeTypeModifiers(mem.getType()) == "float") {
                append("(_stack.sp - 1)->f = (_stack.sp - 1)->i ? ((%s) (_stack.sp - 1)->v)->%s : 0.0f;\n", sclTypeToCType(result, s.getName()).c_str(), body[i].getValue().c_str());
            } else {
                append("(_stack.sp - 1)->v = (_stack.sp - 1)->i ? (scl_any) ((%s) (_stack.sp - 1)->v)->%s : NULL;\n", sclTypeToCType(result, s.getName()).c_str(), body[i].getValue().c_str());
            }
        } else {
            if (removeTypeModifiers(mem.getType()) == "float") {
                append("(_stack.sp - 1)->f = ((%s) (_stack.sp - 1)->v)->%s;\n", sclTypeToCType(result, s.getName()).c_str(), body[i].getValue().c_str());
            } else {
                append("(_stack.sp - 1)->v = (scl_any) ((%s) (_stack.sp - 1)->v)->%s;\n", sclTypeToCType(result, s.getName()).c_str(), body[i].getValue().c_str());
            }
        }
        typeStack.push(mem.getType());
        if (deref) {
            std::string path = dot.getValue() + "@" + body[i].getValue();
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
        Method* m = getMethodByNameOnThisType(result, body[i].getValue(), interface->getName());
        if (!m) {
            transpilerError("Interface '" + interface->getName() + "' has no member named '" + body[i].getValue() + "'", i);
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
            std::string op = body[i].getValue();

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
                transpilerError("Unknown method '" + body[i].getValue() + "' on type 'lambda'", i);
                errors.push_back(err);
            }
            return;
        }
        std::string type = typeStackTop;
        std::string tmp = removeTypeModifiers(type);
        if (tmp.size() > 2 && tmp.front() == '[' && tmp.back() == ']') {
            safeInc();
            if (body[i].getType() != tok_identifier) {
                transpilerError("Expected identifier, but got '" + body[i].getValue() + "'", i);
                errors.push_back(err);
                return;
            }
            std::string op = body[i].getValue();
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
                transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + type + "'", i);
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
        bool onSuper = function->isMethod && body[i - 1].getType() == tok_identifier && body[i - 1].getValue() == "super";
        safeInc();
        if (!s.isStatic() && !hasMethod(result, body[i].getValue(), removeTypeModifiers(type))) {
            std::string help = "";
            if (s.hasMember(body[i].getValue())) {
                help = ". Maybe you meant to use '.' instead of ':' here";

                const Variable& v = s.getMember(body[i].getValue());
                std::string type = v.getType();
                if (!strstarts(removeTypeModifiers(type), "lambda(")) {
                    goto notLambdaType;
                }

                std::string returnType = lambdaReturnType(type);
                size_t argAmount = lambdaArgCount(type);


                append("{\n");
                scopeDepth++;

                append("%s tmp = (--_stack.sp)->v;\n", sclTypeToCType(result, s.getName()).c_str());
                typePop;

                if (typeStack.size() < argAmount) {
                    transpilerError("Arguments for lambda '" + s.getName() + ":" + v.getName() + "' do not equal inferred stack", i);
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
                    append("void(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.getName().c_str());
                    append("_stack.sp -= (%zu);\n", argAmount);
                    append("lambda(%s);\n", argGet.c_str());
                } else if (removeTypeModifiers(returnType) == "float") {
                    append("scl_float(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.getName().c_str());
                    append("_stack.sp -= (%zu);\n", argAmount);
                    append("(_stack.sp++)->f = lambda(%s);\n", argGet.c_str());
                    typeStack.push(returnType);
                } else {
                    append("scl_any(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.getName().c_str());
                    append("_stack.sp -= (%zu);\n", argAmount);
                    append("(_stack.sp++)->v = lambda(%s);\n", argGet.c_str());
                    typeStack.push(returnType);
                }
                scopeDepth--;
                append("}\n");
                return;
            }
        notLambdaType:
            transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + removeTypeModifiers(type) + "'" + help, i);
            errors.push_back(err);
            return;
        } else if (s.isStatic() && s.getName() != "any" && s.getName() != "int" && !hasFunction(result, removeTypeModifiers(type) + "$" + body[i].getValue())) {
            transpilerError("Unknown static function '" + body[i].getValue() + "' on type '" + removeTypeModifiers(type) + "'", i);
            errors.push_back(err);
            return;
        }
        if (s.getName() == "any" || s.getName() == "int") {
            Function* anyMethod = getFunctionByName(result, s.getName() + "$" + body[i].getValue());
            if (!anyMethod) {
                if (s.getName() == "any") {
                    anyMethod = getFunctionByName(result, "int$" + body[i].getValue());
                } else {
                    anyMethod = getFunctionByName(result, "any$" + body[i].getValue());
                }
            }
            if (anyMethod) {
                Method* objMethod = getMethodByName(result, body[i].getValue(), "SclObject");
                if (objMethod) {
                    append("if (_scl_is_instance_of((_stack.sp - 1)->v, 0x%xU)) {\n", id((char*) "SclObject"));
                    scopeDepth++;
                    methodCall(objMethod, fp, result, warns, errors, body, i, true, false);
                    if (objMethod->getReturnType() != "none" && objMethod->getReturnType() != "nothing") {
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
                transpilerError("Unknown static function '" + body[i].getValue() + "' on type '" + type + "'", i);
                errors.push_back(err);
            }
            return;
        }
        if (s.isStatic()) {
            Function* f = getFunctionByName(result, removeTypeModifiers(type) + "$" + body[i].getValue());
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
            if (removeTypeModifiers(type) == "str" && body[i].getValue() == "toString") {
                return;
            }
            if (removeTypeModifiers(type) == "str" && body[i].getValue() == "view") {
                append("(_stack.sp - 1)->v = (_stack.sp - 1)->s->data;\n");
                typePop;
                typeStack.push("[int8]");
                return;
            }
            Method* f = getMethodByName(result, body[i].getValue(), s.getName());
            if (f->isPrivate && (!function->isMethod || ((Method*) function)->getMemberType() != s.getName())) {
                if (f->getMemberType() != function->member_type) {
                    transpilerError("'" + body[i].getValue() + "' has private access in Struct '" + s.getName() + "'", i);
                    errors.push_back(err);
                    return;
                }
            }
            methodCall(f, fp, result, warns, errors, body, i, false, true, false, onSuper);
        }
    }

    handler(INVALID) {
        noUnused;
        transpilerError("Unknown token: '" + body[i].getValue() + "'", i);
        errors.push_back(err);
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

        handleRef(handleRefs[body[i].getType()]);

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
            name = f->getMemberType() + ":" + name;
        }
        name += "(";
        for (size_t i = 0; i < f->getArgs().size(); i++) {
            const Variable& v = f->getArgs()[i];
            if (f->isMethod && v.getName() == "self") continue;
            if (i) name += ", ";
            name += v.getName() + ": " + v.getType();
        }
        name += "): " + f->getReturnType();
        return name;
    }

    std::string sclGenCastForMethod(TPResult& result, Method* m) {
        std::string return_type = sclTypeToCType(result, m->getReturnType());
        std::string arguments = "";
        if (m->getArgs().size() > 0) {
            for (ssize_t i = (ssize_t) m->getArgs().size() - 1; i >= 0; i--) {
                const Variable& var = m->getArgs()[i];
                arguments += sclTypeToCType(result, var.getType());
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
        for (const Variable& v : f->getArgs()) {
            std::string type = removeTypeModifiers(v.getType());
            if (v.getName() == "self" && f->isMethod) {
                continue;
            }
            args += typeToRTSig(type);
        }
        args += ")";
        args += typeToRTSig(f->getReturnType());
        return args;
    }

    bool argVecEquals(std::vector<Variable>& a, std::vector<Variable>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (size_t i = 0; i < a.size(); i++) {
            if (a[i].getName() == "self" || b[i].getName() == "self") {
                continue;
            }
            if (a[i].getType() != b[i].getType()) {
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
        if (s.extends().size()) {
            auto parent = getStructByName(res, s.extends());
            if (parent == Struct::Null) {
                fprintf(stderr, "Error: Struct '%s' extends '%s', but '%s' does not exist.\n", s.getName().c_str(), s.extends().c_str(), s.extends().c_str());
                exit(1);
            }
            const std::vector<Method*>& parentVTable = makeVTable(res, parent.getName());
            for (auto&& m : parentVTable) {
                vtable.push_back(m);
            }
        }
        for (auto&& m : methodsOnType(res, name)) {
            long indexInVtable = -1;
            for (size_t i = 0; i < vtable.size(); i++) {
                if (vtable[i]->getName() == m->getName() && argVecEquals(vtable[i]->getArgs(), m->getArgs())) {
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
        return vtable;
    }

    extern "C" void ConvertC::writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<Variable>& globals, TPResult& result, std::string filename) {
        (void) warns;

        populateTokenJumpTable();

        scopeDepth = 0;
        append("extern tls _scl_stack_t _stack;\n\n");

        fclose(fp);
        fp = fopen((filename.substr(0, filename.size() - 2) + ".typeinfo.h").c_str(), "a");
        append("#include \"%s.h\"\n\n", filename.substr(0, filename.size() - 2).c_str());
        
        for (Function* function : result.functions) {
            if (function->isMethod) {
                if (getInterfaceByName(result, function->member_type)) {
                    continue;
                }
                append("static void _scl_vt_c_%s$%s();\n", function->getMemberType().c_str(), function->finalName().c_str());
            }
        }

        for (Struct& s : result.structs) {
            if (s.isStatic()) continue;
            if (vtables.find(s.getName()) == vtables.end()) {
                vtables[s.getName()] = makeVTable(result, s.getName());
            }
            append("extern const StaticMembers _scl_statics_%s;\n", s.getName().c_str());
            append("extern const struct _scl_methodinfo _scl_vtable_%s[];\n", s.getName().c_str());
            append("const ID_t _scl_supers_%s[] = {\n", s.getName().c_str());
            scopeDepth++;
            Struct current = s;
            while (current.extends().size()) {
                append("0x%xU,\n", id((char*) current.extends().c_str()));
                current = getStructByName(result, current.extends());
            }
            append("0x0U\n");
            scopeDepth--;
            append("};\n");
        }
        for (Struct& s : result.structs) {
            if (s.isStatic()) continue;
            append("const StaticMembers _scl_statics_%s = {\n", s.getName().c_str());
            scopeDepth++;
            append(".type = 0x%xU,\n", id((char*) s.getName().c_str()));
            append(".type_name = \"%s\",\n", s.getName().c_str());
            append(".vtable = _scl_vtable_%s,\n", s.getName().c_str());
            if (s.extends().size()) {
                append(".super_vtable = _scl_vtable_%s,\n", s.extends().c_str());
            } else {
                append(".super_vtable = 0,\n");
            }
            append(".supers = _scl_supers_%s,\n", s.getName().c_str());
            scopeDepth--;
            append("};\n");
        }

        for (auto&& vtable : vtables) {
            append("const struct _scl_methodinfo _scl_vtable_%s[] = {\n", vtable.first.c_str());
            scopeDepth++;
            for (auto&& m : vtable.second) {
                std::string signature = argsToRTSignature(m);
                append("(struct _scl_methodinfo) {\n");
                scopeDepth++;
                append(".ptr = _scl_vt_c_%s$%s,\n", m->getMemberType().c_str(), m->finalName().c_str());
                append(".pure_name = 0x%xU,\n", id((char*) sclFunctionNameToFriendlyString(m->finalName()).c_str()));
                append(".signature = 0x%xU,\n", id((char*) signature.c_str()));
                scopeDepth--;
                append("},\n");
            }
            append("{0}\n");
            scopeDepth--;
            append("};\n\n");
        }

        append("extern const ID_t id(const char*);\n\n");

        fclose(fp);
        fp = fopen(filename.c_str(), "a");
        
        append("#include \"%s.typeinfo.h\"\n\n", filename.substr(0, filename.size() - 2).c_str());

        for (auto&& f : result.functions) {
            if (f->isMethod) {
                if (getInterfaceByName(result, f->member_type)) {
                    continue;
                }
                currentStruct = getStructByName(result, f->member_type);
                auto m = (Method*) f;
                append("static void _scl_vt_c_%s$%s() {\n", m->getMemberType().c_str(), f->finalName().c_str());
                scopeDepth++;
                generateUnsafeCall(m, fp, result);
                scopeDepth--;
                append("}\n");
            }
        }
        currentStruct = Struct::Null;

        append("#if defined(__clang__)\n");
        append("#pragma clang diagnostic push\n");
        append("#pragma clang diagnostic ignored \"-Wint-to-void-pointer-cast\"\n");
        append("#pragma clang diagnostic ignored \"-Wint-to-pointer-cast\"\n");
        append("#pragma clang diagnostic ignored \"-Wpointer-to-int-cast\"\n");
        append("#pragma clang diagnostic ignored \"-Wvoid-pointer-to-int-cast\"\n");
        append("#pragma clang diagnostic ignored \"-Wincompatible-pointer-types\"\n");
        append("#pragma clang diagnostic ignored \"-Wint-conversion\"\n");
        append("#endif\n");

        for (size_t f = 0; f < result.functions.size(); f++) {
            Function* function = currentFunction = result.functions[f];
            if (function->isExternC) continue;
            if (contains<std::string>(function->getModifiers(), "<binary-inherited>")) {
                continue;
            }
            if (getInterfaceByName(result, function->member_type)) {
                continue;
            }
            if (function->isMethod) {
                currentStruct = getStructByName(result, ((Method*) function)->getMemberType());
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

            for (std::string& modifier : function->getModifiers()) {
                if (modifier == "nowarn") {
                    noWarns = true;
                }
            }

            std::string functionDeclaration = "";

            if (function->isMethod) {
                functionDeclaration += ((Method*) function)->getMemberType() + ":" + function->getName() + "(";
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
            if (function->isMethod) {
                arguments = sclTypeToCType(result, function->member_type) + " Var_self";
                for (size_t i = 0; i < function->getArgs().size() - 1; i++) {
                    std::string type = sclTypeToCType(result, function->getArgs()[i].getType());
                    arguments += ", " + type;
                    if (type == "varargs" || type == "/* varargs */ ...") continue;
                    if (function->getArgs()[i].getName().size())
                        arguments += " Var_" + function->getArgs()[i].getName();
                }
            } else {
                if (function->getArgs().size() > 0) {
                    for (size_t i = 0; i < function->getArgs().size(); i++) {
                        std::string type = sclTypeToCType(result, function->getArgs()[i].getType());
                        if (i) {
                            arguments += ", ";
                        }
                        arguments += type;
                        if (type == "varargs" || type == "/* varargs */ ...") continue;
                        if (function->getArgs()[i].getName().size())
                            arguments += " Var_" + function->getArgs()[i].getName();
                    }
                }
            }

            if (arguments.empty()) {
                arguments = "void";
            }

            std::string file = function->getNameToken().getFile();
            if (strstarts(file, scaleFolder)) {
                file = file.substr(scaleFolder.size() + std::string("/Frameworks/").size());
            } else {
                file = std::filesystem::path(file).relative_path();
            }

            if (function->getReturnType().size() > 0) {
                std::string t = function->getReturnType();
                return_type = sclTypeToCType(result, t);
            }

            if (function->isMethod) {
                if (!(((Method*) function))->addAnyway() && getStructByName(result, function->getMemberType()).isSealed()) {
                    FPResult result;
                    result.message = "Struct '" + function->getMemberType() + "' is sealed!";
                    result.value = function->getNameToken().getValue();
                    result.line = function->getNameToken().getLine();
                    result.in = function->getNameToken().getFile();
                    result.column = function->getNameToken().getColumn();
                    result.type = function->getNameToken().getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                append("%s Method_%s$%s(%s) {\n", return_type.c_str(), function->getMemberType().c_str(), function->finalName().c_str(), arguments.c_str());
                if (function->getName() == "init") {
                    Method* thisMethod = (Method*) function;
                    Struct s = getStructByName(result, thisMethod->getMemberType());
                    if (s == Struct::Null) {
                        FPResult result;
                        result.message = "Struct '" + thisMethod->getMemberType() + "' does not exist!";
                        result.value = function->getNameToken().getValue();
                        result.line = function->getNameToken().getLine();
                        result.in = function->getNameToken().getFile();
                        result.column = function->getNameToken().getColumn();
                        result.type = function->getNameToken().getType();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    if (s.extends().size()) {
                        Method* parentInit = getMethodByName(result, "init", s.extends());
                        // implicit call to parent init, if it takes no arguments
                        if (parentInit && parentInit->getArgs().size() == 1) {
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
                if (function->getName() == "throw" || function->getName() == "builtinUnreachable") {
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

            {
                std::vector<Token> body = function->getBody();
                if (body.size() == 0)
                    goto emptyFunction;

                varScopePush();
                if (function->hasNamedReturnValue) {
                    varScopeTop().push_back(function->getNamedReturnValue());
                }
                if (function->getArgs().size() > 0) {
                    for (size_t i = 0; i < function->getArgs().size(); i++) {
                        const Variable& var = function->getArgs()[i];
                        if (var.getType() == "varargs") continue;
                        varScopeTop().push_back(var);
                    }
                }

                append("*(_stack.tp++) = \"%s\";\n", sclFunctionNameToFriendlyString(function).c_str());
                if (function->hasNamedReturnValue) {
                    std::string nrvName = function->getNamedReturnValue().getName();
                    if (hasFunction(result, nrvName)) {
                        {
                            FPResult err;
                            err.message = "Named return value '" + nrvName + "' shadowed by function '" + nrvName + "'";
                            err.value = function->getNameToken().getValue();
                            err.line = function->getNameToken().getLine();
                            err.in = function->getNameToken().getFile();
                            err.column = function->getNameToken().getColumn();
                            err.type = function->getNameToken().getType();
                            err.success = false;
                            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                            else errors.push_back(err);
                        }
                    }
                    if (hasContainer(result, nrvName)) {
                        {
                            FPResult err;
                            err.message = "Named return value '" + nrvName + "' shadowed by container '" + nrvName + "'";
                            err.value = function->getNameToken().getValue();
                            err.line = function->getNameToken().getLine();
                            err.in = function->getNameToken().getFile();
                            err.column = function->getNameToken().getColumn();
                            err.type = function->getNameToken().getType();
                            err.success = false;
                            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                            else errors.push_back(err);
                        }
                    }
                    if (getStructByName(result, nrvName) != Struct::Null) {
                        {
                            FPResult err;
                            err.message = "Named return value '" + nrvName + "' shadowed by struct '" + nrvName + "'";
                            err.value = function->getNameToken().getValue();
                            err.line = function->getNameToken().getLine();
                            err.in = function->getNameToken().getFile();
                            err.column = function->getNameToken().getColumn();
                            err.type = function->getNameToken().getType();
                            err.success = false;
                            if (!Main.options.Werror) { if (!noWarns) warns.push_back(err); }
                            else errors.push_back(err);
                        }
                    }
                    append("%s Var_%s;\n", sclTypeToCType(result, function->getNamedReturnValue().getType()).c_str(), function->getNamedReturnValue().getName().c_str());
                }
                if (function->isMethod) {
                    Struct s = getStructByName(result, function->getMemberType());
                    std::string superType = s.extends();
                    if (superType.size() > 0) {
                        append("%s Var_super = (%s) Var_self;\n", sclTypeToCType(result, superType).c_str(), sclTypeToCType(result, superType).c_str());
                        varScopeTop().push_back(Variable("super", "const " + superType));
                    }
                }
                append("const _scl_frame_t* __current_base_ptr = _stack.sp;\n");

                for (ssize_t a = (ssize_t) function->getArgs().size() - 1; a >= 0; a--) {
                    Variable& arg = function->getArgs()[a];
                    if (arg.getName().size() == 0) continue;
                    if (!typeCanBeNil(arg.getType())) {
                        if (function->isMethod && arg.getName() == "self") continue;
                        if (arg.getType() == "varargs") continue;
                        append("_scl_check_not_nil_argument(*(scl_int*) &Var_%s, \"%s\");\n", arg.getName().c_str(), arg.getName().c_str());
                    }
                    if (arg.typeFromTemplate.size()) {
                        append("_scl_checked_cast(*(scl_any*) &Var_%s, Var_self->$template_arg_%s, Var_self->$template_argname_%s);\n", arg.getName().c_str(), arg.typeFromTemplate.c_str(), arg.typeFromTemplate.c_str());
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
                    if (function->varArgsParam().getName().size()) {
                        append("va_list _cvarargs;\n");
                        append("va_start(_cvarargs, Var_%s$size);\n", function->varArgsParam().getName().c_str());
                        append("scl_int _cvarargs_count = Var_%s$size;\n", function->varArgsParam().getName().c_str());
                        append("scl_Array Var_%s = _scl_cvarargs_to_array(_cvarargs, _cvarargs_count);\n", function->varArgsParam().getName().c_str());
                        append("va_end(_cvarargs);\n");
                        Variable v(function->varArgsParam().getName(), "const ReadOnlyArray");
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
            }

        emptyFunction:
            scopeDepth = 0;
            append("}\n\n");
        }

        append("#if defined(__clang__)\n");
        append("#pragma clang diagnostic pop\n");
        append("#endif\n");

        if (Main.options.noMain)
            return;
        
        append("int main(int argc, char** argv) {\n");
        scopeDepth++;
        append("return _scl_run(argc, argv, Function_main);\n");
        scopeDepth--;
        append("}\n");
    }
} // namespace sclc
