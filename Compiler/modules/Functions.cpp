
#include <csignal>

#include "../headers/Common.hpp"
#include "../headers/Types.hpp"
#include "../headers/TranspilerDefs.hpp"
#include "../headers/Functions.hpp"

#define OS_APPLE 1
#define OS_WIN 2
#define OS_LINUX 3
#define OS_OTHER 4
#if defined(__APPLE__)
#define OS OS_APPLE
#elif defined(_WIN32)
#define OS OS_WIN
#elif defined(__gnu_linux__)
#define OS OS_LINUX
#else
#define OS OS_OTHER
#endif

#ifndef VERSION
#define VERSION ""
#endif

namespace sclc {
    extern int scopeDepth;
    extern std::vector<std::string> typeStack;
    extern Function* currentFunction;
    extern Struct currentStruct;
    extern std::unordered_map<std::string, std::vector<Method*>> vtables;
    extern int isInUnsafe;

    std::unordered_map<Function*, std::string> functionPtrs;
    std::vector<bool> bools{false, true};

    std::string retemplate(std::string type);
    std::string generateArgumentsForFunction(TPResult& result, Function *func) {
        size_t maxValue = func->args.size();
        std::string args;
        bool isVarargs = func->isCVarArgs();
        if (func->isMethod) {
            const std::string& self_type = func->args.back().type;
            if (self_type.front() == '@') {
                args += "*(";
            }
            args += "scale_positive_offset(" + std::to_string(func->args.size() - 1) + ", " + sclTypeToCType(result, func->member_type) + ")";
            if (self_type.front() == '@') {
                args += ")";
            }
            maxValue--;
        }
        if (isVarargs)
            maxValue--;

        for (size_t i = 0; i < maxValue; i++) {
            const Variable& arg = func->args[i];
            if (removeTypeModifiers(arg.type) == "varargs") {
                continue;
            }

            if (func->isMethod || i)
                args += ", ";

            bool isValueStructParam = arg.type.front() == '@';
            if (isValueStructParam) {
                args += "*(";
            } else {
                std::string stack = typeStack[typeStack.size() - maxValue + i];
                if (isPrimitiveIntegerType(stack) && isPrimitiveIntegerType(arg.type) && !typesCompatible(result, stack, arg.type, false)) {
                    args += "scale_cast_positive_offset(" + std::to_string(i) + ", " + sclTypeToCType(result, stack) + ", " + sclTypeToCType(result, arg.type) + ")";
                    continue;
                }
            }
            args += "scale_positive_offset(" + std::to_string(i) + ", " + sclTypeToCType(result, arg.type.substr(isValueStructParam)) + ")";
            if (isValueStructParam) args += ")";
        }
        return args;
    }

    std::string functionArgsToStructBody(Function* f, TPResult& result) {
        std::string s = "{ ";
        for (auto&& arg : f->args) {
            s += sclTypeToCType(result, arg.type) + " Var_" + arg.name + "; ";
        }
        return s + "}";
    }

    void createVariadicCall(Function* f, std::ostream& fp, TPResult& result, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i) {
        bool parseCount = (i + 1) < body.size() && body[i + 1].value == "!";
        size_t amountOfVarargs = 0;

        append("{\n");
        scopeDepth++;
        if (parseCount) {
            safeIncN(2);
            if (body[i].type != tok_number) {
                transpilerError("Amount of variadic arguments needs to be specified for variadic function call", i);
                errors.push_back(err);
                return;
            }
            transpilerError("Specifying the amount of variadic arguments is deprecated! Use 'varargs' specifier instead.", i);
            errors.push_back(err);
            return;
        } else {
            bool hitVarargs = false;
            for (ssize_t i = typeStack.size() - 1; i >= 0; i--) {
                if (typeStack[i] == "<varargs>") {
                    hitVarargs = true;
                    break;
                }
                amountOfVarargs++;
            }
            if (!hitVarargs) {
                amountOfVarargs = 0;
            }
        }

        for (long i = amountOfVarargs - 1; i >= 0; i--) {
            std::string nextType = typeStackTop;
            typeStack.pop_back();
            std::string ctype = sclTypeToCType(result, nextType);
            append("%s vararg%ld = scale_pop(%s);\n", ctype.c_str(), i, ctype.c_str());
        }

        if (f->varArgsParam().name.size()) {
            append("scale_push(scale_int, %zu);\n", amountOfVarargs);
            typeStack.push_back("int");
        }

        std::string args = generateArgumentsForFunction(result, f);

        for (size_t i = 0; i < amountOfVarargs; i++) {
            args += ", ";
            if (f->varArgsParam().name.size()) {
                args += "&";
            }
            args += "vararg" + std::to_string(i);
        }

        append("scale_popn(%zu);\n", f->args.size() - 1);

        for (size_t i = 0; i < f->args.size(); i++) {
            typePop;
        }

        bool closeThePush = false;
        if (f->return_type.size() && f->return_type.front() == '@' && !f->has_async) {
            const Struct& s = getStructByName(result, f->return_type);
            append("scale_push_value(%s, %s, ", sclTypeToCType(result, f->return_type).c_str(), (s != Struct::Null ? "MEM_FLAG_INSTANCE" : "0"));
            closeThePush = true;
        } else {
            if (f->return_type != "none" && f->return_type != "nothing" && !f->has_async) {
                append("scale_push(%s, ", sclTypeToCType(result, f->return_type).c_str());
                closeThePush = true;
            } else {
                append("");
            }
        }

        if (f->has_async) {
            transpilerError("Variadic functions cannot be called asynchronously", i);
            errors.push_back(err);
            return;
        } else {
            append2("fn_%s(%s)", f->name.c_str(), args.c_str());
        }
        if (closeThePush) {
            append2(")");
        }
        append2(";\n");

        if (f->has_async) {
            typeStack.push_back("async<" + f->return_type + ">");
        } else if (removeTypeModifiers(f->return_type) != "none" && removeTypeModifiers(f->return_type) != "nothing") {
            if (f->return_type.front() == '@') {
                typeStack.push_back(f->return_type.substr(1));
            } else {
                typeStack.push_back(f->return_type);
            }
        }
        scopeDepth--;
        append("}\n");
    }

    std::string generateInternal(Function* f) {
        std::string symbol;

        std::string name = f->name_without_overload;
        if (f->member_type.size()) {
            if (!f->isMethod) {
                name = name.substr(f->member_type.size() + 1);
            }
            symbol += std::to_string(f->member_type.length()) + f->member_type;
        }
        if (f->has_lambda) {
            name = name.substr(8);
            std::string count = name.substr(0, name.find("$"));
            name = generateInternal(f->container) + "$" + count;
        } else {
            name = name.substr(0, name.find("$$ol"));
            symbol += std::to_string(name.length());
        }
        symbol += name;

        std::string typeToSymbol(std::string type);
        for (size_t i = 0; i < f->args.size() - ((size_t) f->isMethod); i++) {
            symbol += typeToSymbol(f->args[i].type);
        }
        symbol += typeToSymbol(f->return_type);
        return symbol;
    }

    std::string generateSymbolForFunction(Function* f) {
        if (f->has_asm) {
            return format("\"%s\"", f->getModifier(f->has_asm + 1).c_str());
        }

        if (f->has_cdecl) {
            return format("scale_macro_to_string(__USER_LABEL_PREFIX__) \"%s\"", f->getModifier(f->has_cdecl + 1).c_str());
        }

        if (!f->isMethod && !Main::options::noMain && f->name == "main") {
            return "scale_macro_to_string(__USER_LABEL_PREFIX__) \"main\"";
        }

        if (f->has_foreign) {
            if (f->isMethod) {
                return format("scale_macro_to_string(__USER_LABEL_PREFIX__) \"%s$%s\"", f->member_type.c_str(), f->name.c_str());
            } else {
                return format("scale_macro_to_string(__USER_LABEL_PREFIX__) \"%s\"", f->name.c_str());
            }
        }

        std::string symbol;
        if (f->isMethod) {
            symbol = "_M";
        } else if (f->has_lambda) {
            symbol = "_L";
        } else {
            symbol = "_F";
        }
        symbol += generateInternal(f);
        return format("scale_macro_to_string(__USER_LABEL_PREFIX__) \"%s\"", symbol.c_str());
    }
    
    Method* findMethodLocally(Method* self, TPResult& result) {
        for (Function* f : result.functions) {
            if (!f->isMethod) continue;
            Method* m = (Method*) f;
            std::string name = m->name.substr(0, m->name.find("$$ol"));
            if ((name == self->name) && m->member_type == self->member_type) {
                if (currentFunction) {
                    if (m->name_token.location.file == currentFunction->name_token.location.file) {
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
            if (f->name_without_overload == self->name) {
                if (currentFunction) {
                    if (f->name_token.location.file == currentFunction->name_token.location.file) {
                        return f;
                    }
                } else {
                    return f;
                }
            }
        }
        return nullptr;
    }

    bool opFunc(std::string name) {
        name = name.substr(0, name.find("$$ol"));
        for (auto&& p : funcNameIdents) {
            if (p.first == name || p.second == name) {
                return true;
            }
        }
        return false;
    }

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
            std::string selfType = self->args[self->args.size() - 1].type;
            functionPtrCast += sclTypeToCType(result, selfType);
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

    void createReifiedCall(Function *self, std::ostream &fp, TPResult &result, std::vector<FPResult> &warns, std::vector<FPResult> &errors, std::vector<Token> &body, size_t &i);

    void methodCall(Method* self, std::ostream& fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i, bool ignoreArgs, bool doActualPop, bool withIntPromotion, bool onSuperType, bool checkOverloads) {
        if (!shouldCall(self, warns, errors, body, i)) {
            return;
        }
        if (self->has_reified) {
            createReifiedCall(self, fp, result, warns, errors, body, i);
            return;
        }
        if (currentFunction) {
            if (self->has_private) {
                Token selfNameToken = self->name_token;
                Token fromNameToken = currentFunction->name_token;
                if (selfNameToken.location.file != fromNameToken.location.file) {
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

        if (i + 1 < body.size() && body[i + 1].type == tok_double_column) {
            size_t begin = i;
            safeInc();
            safeInc();
            if (body[i].type != tok_identifier || body[i].value != "<") {
                transpilerError("Expected '<' to specify argument types, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            safeInc();
            std::vector<std::string> argTypes;
            while (body[i].value != ">") {
                FPResult type = parseType(body, i);
                if (!type.success) {
                    errors.push_back(type);
                    return;
                }
                argTypes.push_back(removeTypeModifiers(type.value));
                safeInc();
                if (body[i].type != tok_comma && (body[i].value != ">" || body[i].type != tok_identifier)) {
                    transpilerError("Expected ',' or '>', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                if (body[i].type == tok_comma) {
                    safeInc();
                }
            }

            auto argsEqual = [&](std::vector<Variable> args) {
                if (args.size() != argTypes.size()) return false;
                    bool foundWithoutPromotion = false;
                    for (size_t i = 0; i < args.size(); i++) {
                        if (args[i].name == "self") continue;
                        if (argTypes[i] == args[i].type) continue;
                        if (typesCompatible(result, argTypes[i], args[i].type, false)) {
                            foundWithoutPromotion = true;
                            continue;
                        }
                        return false;
                    }
                    if (foundWithoutPromotion) return true;
                    for (size_t i = 0; i < args.size(); i++) {
                        if (args[i].name == "self") continue;
                        if (argTypes[i] == args[i].type) continue;
                        if (typesCompatible(result, argTypes[i], args[i].type, true)) {
                            continue;
                        }
                        return false;
                    }
                    return true;
            };

            bool found = false;
            for (auto&& overload : self->overloads) {
                if (argsEqual(overload->args) && overload->isMethod == self->isMethod) {
                    self = (Method*) overload;
                    found = true;
                    break;
                }
            }
            if (!found) {
                transpilerError("No overload of '" + self->name + "' with arguments [ " + argVectorToString(argTypes) + " ] found", begin);
                errors.push_back(err);
                return;
            }

            goto callMethod;
        }
        
        {
            std::vector<Function*>& overloads = self->overloads;
            bool argsEqual = ignoreArgs || checkStackType(result, self->args, withIntPromotion);
            if (checkOverloads && overloads.size() && !argsEqual && !ignoreArgs) {
                for (Function* overload : overloads) {
                    for (bool b : bools) {
                        if (!overload->isMethod) continue;
                        
                        bool argsEqual = checkStackType(result, overload->args, b);
                        if (argsEqual || overload->has_reified) {
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
        }

    callMethod:

        std::string type = typeStackTop;
        std::string args = generateArgumentsForFunction(result, self);
        if (doActualPop) {
            for (size_t m = 0; m < self->args.size(); m++) {
                typePop;
            }
        }
        bool found = false;
        size_t argc = self->args.size();
        append("scale_popn(%zu);\n", argc);
        append("// INVOKE %s\n", sclFunctionNameToFriendlyString(self).c_str());
        bool closeThePush = false;
        if (self->return_type.size() && self->return_type.front() == '@' && !self->has_async) {
            const Struct& s = getStructByName(result, self->return_type);
            append("scale_push_value(%s, %s, ", sclTypeToCType(result, self->return_type).c_str(), (s != Struct::Null ? "MEM_FLAG_INSTANCE" : "0"));
            closeThePush = true;
        } else {
            if (self->return_type != "none" && self->return_type != "nothing" && !self->has_async) {
                append("scale_push(%s, ", sclTypeToCType(result, self->return_type).c_str());
                closeThePush = true;
            } else {
                append("");
            }
        }

        bool asyncImmediatelyAwaited = self->has_async && (i + 1) < body.size() && body[i + 1].type == tok_await;
        std::string rtype = removeTypeModifiers(self->return_type);

        if (self->has_nonvirtual || self->has_final) {
            if (self->has_async) {
                if (asyncImmediatelyAwaited) {
                    append2("scale_sync(mt_%s$%s, %s, %s)", self->member_type.c_str(), self->name.c_str(), functionArgsToStructBody(self, result).c_str(), args.c_str());
                } else {
                    append2("scale_async(mt_%s$%s, %s, %s)", self->member_type.c_str(), self->name.c_str(), functionArgsToStructBody(self, result).c_str(), args.c_str());
                }
            } else {
                append2("mt_%s$%s(%s)", self->member_type.c_str(), self->name.c_str(), args.c_str());
            }
            found = true;
        } else if (getInterfaceByName(result, self->member_type) == nullptr) {
            auto vtable = vtables[self->member_type];
            size_t index = 0;
            for (auto&& method : vtable) {
                if (method->operator==(self)) {
                    std::string functionPtrCast = getFunctionType(result, self);
                    if (self->has_async) {
                        if (asyncImmediatelyAwaited) {
                            if (rtype != "none" && rtype != "nothing") {
                                append2("scale_sync(%s, (%s) scale_positive_offset(%zu, %s)->", sclTypeToCType(result, self->return_type).c_str(), functionPtrCast.c_str(), argc - 1, sclTypeToCType(result, self->member_type).c_str());
                            } else {
                                append2("scale_sync_v((%s) scale_positive_offset(%zu, %s)->", functionPtrCast.c_str(), argc - 1, sclTypeToCType(result, self->member_type).c_str());
                            }
                        } else {
                            append2("scale_async((%s) scale_positive_offset(%zu, %s)->", functionPtrCast.c_str(), argc - 1, sclTypeToCType(result, self->member_type).c_str());
                        }
                        if (onSuperType) {
                            append2("$type->super->vtable[%zu], %s, %s)", index, functionArgsToStructBody(self, result).c_str(), args.c_str());
                        } else {
                            append2("$type->vtable[%zu], %s, %s)", index, functionArgsToStructBody(self, result).c_str(), args.c_str());
                        }
                    } else {
                        append2("((%s) scale_positive_offset(%zu, %s)->", functionPtrCast.c_str(), argc - 1, sclTypeToCType(result, self->member_type).c_str());
                        if (onSuperType) {
                            append2("$type->super->vtable[%zu])(%s)", index, args.c_str());
                        } else {
                            append2("$type->vtable[%zu])(%s)", index, args.c_str());
                        }
                    }
                    found = true;
                    break;
                }
                index++;
            }
        } else {
            std::string rtSig = argsToRTSignature(self);
            std::string functionPtrCast = getFunctionType(result, self);
            append("{\n");
            scopeDepth++;
            append(
                "scale_any tmp = scale_get_vtable_function(scale_positive_offset(%zu, scale_any), \"%s%s\"));\n",
                argc - 1,
                self->name_without_overload.c_str(),
                rtSig.c_str()
            );
            if (self->has_async) {
                if (asyncImmediatelyAwaited) {
                    if (rtype != "none" && rtype != "nothing") {
                        append("scale_sync(%s, (%s) tmp, %s, %s)", sclTypeToCType(result, self->return_type).c_str(), functionPtrCast.c_str(), functionArgsToStructBody(self, result).c_str(), args.c_str());
                    } else {
                        append("scale_sync_v((%s) tmp, %s, %s)", functionPtrCast.c_str(), functionArgsToStructBody(self, result).c_str(), args.c_str());
                    }
                } else {
                    append("scale_async((%s) tmp, %s, %s)", functionPtrCast.c_str(), functionArgsToStructBody(self, result).c_str(), args.c_str());
                }
            } else {
                append("((%s) tmp)(%s)", functionPtrCast.c_str(), args.c_str());
            }
            found = true;
        }
        if (closeThePush) {
            append2(")");
        }
        append2(";\n");
        if (self->has_async && !asyncImmediatelyAwaited) {
            typeStack.push_back("async<" + self->return_type + ">");
        } else if (rtype != "none" && rtype != "nothing") {
            if (self->return_type.front() == '@') {
                typeStack.push_back(self->return_type.substr(1));
            } else {
                typeStack.push_back(self->return_type);
            }
        }
        if (!found) {
            transpilerError("Method '" + sclFunctionNameToFriendlyString(self) + "' not found on type '" + self->member_type + "'", i);
            errors.push_back(err);
        }
        if (asyncImmediatelyAwaited) {
            i++;
        }
    }

    bool hasImplementation(TPResult& result, Function* func) {
        for (Function* f : result.functions) {
            if (f->operator==(func)) {
                return true;
            }
        }
        return false;
    }

    std::string opToString(std::string op) {
        if (op == "add") return "+";
        if (op == "sub") return "-";
        if (op == "mul") return "*";
        if (op == "div") return DIR_SEP;
        if (op == "pow") return "**";
        if (op == "eq") return "==";
        if (op == "ne") return "!=";
        if (op == "gt") return ">";
        if (op == "ge") return ">=";
        if (op == "lt") return "<";
        if (op == "le") return "<=";
        if (op == "and") return "&&";
        if (op == "or") return "||";
        if (op == "not") return "!";
        if (op == "mod") return "%";
        if (op == "land") return "&";
        if (op == "lor") return "|";
        if (op == "lxor") return "^";
        if (op == "lnot") return "~";
        if (op == "lsl") return "<<";
        if (op == "lsr") return ">>";
        if (op == "rol") return "<<<";
        if (op == "ror") return ">>>";
        if (op == "at") return "@";
        if (op == "inc") return "++";
        if (op == "dec") return "--";
        return "???";
    }

    template<typename T>
    std::string _reparseArgType(std::string type, const std::unordered_map<std::string, T>& templateArgs, const std::string& target) {
        std::string mods = "";
        bool isVal = type.front() == '@';
        bool isNil = type.back() == '?';
        type = removeTypeModifiers(type);
        if (isVal) {
            return "@" + mods + _reparseArgType(type.substr(1), templateArgs, target);
        } else if (type.front() == '[') {
            std::string inner = type.substr(1, type.size() - 2);
            return "[" + _reparseArgType(inner, templateArgs, target) + "]" + (isNil ? "?" : "");
        }
        if (strstarts(type, "lambda(")) {
            std::string lt = type.substr(0, type.find(':'));
            std::string ret = type.substr(type.find(':') + 1);
            type = lt + ":" + _reparseArgType(ret, templateArgs, target);
        }
        if (templateArgs.find(type) != templateArgs.end()) {
            if constexpr (std::is_same_v<T, Token>) {
                type = templateArgs.at(type).value;
            } else {
                type = templateArgs.at(type);
            }
        }
        return mods + type + (isNil ? "?" : "");
    }

    std::string reparseArgType(std::string type, const std::unordered_map<std::string, std::string>& templateArgs, const std::string& target) {
        return _reparseArgType(type, templateArgs, target);
    }

    std::string reparseArgType(std::string type, const std::unordered_map<std::string, Token>& templateArgs, const std::string& target) {
        return _reparseArgType(type, templateArgs, target);
    }

    std::string argsToRTSignatureIdent(Function* f);

    std::string declassifyReify(const std::string& what) {
        if (what.front() == '@') {
            return declassifyReify(what.substr(1));
        }
        if (what.front() == '[' && what.back() == ']') {
            return declassifyReify(what.substr(1, what.size() - 2));
        }
        if (what.back() == '?') {
            return declassifyReify(what.substr(0, what.size() - 1));
        }
        if (strstarts(what, "lambda(")) {
            return lambdaReturnType(what);
        }
        return what;
    }

    std::string reifyType(const std::string& with, const std::string& stack) {
        if (with.front() == '[' && with.back() == ']') {
            if (stack.front() == '[' && stack.back() == ']') {
                return reifyType(with.substr(1, with.size() - 2), stack.substr(1, stack.size() - 2));
            } else {
                return reifyType(with.substr(1, with.size() - 2), "any");
            }
        } else if (strstarts(with, "lambda(") && strstarts(stack, "lambda(")) {
            return reifyType(lambdaReturnType(with), lambdaReturnType(stack));
        } else if (with.back() == '?') {
            return reifyType(with.substr(0, with.size() - 1), stack);
        }
        return stack;
    }

    Function* generateReifiedFunction(Function* self, std::ostream& fp, TPResult& result, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i, std::vector<std::string>& types) {
        if (!self->has_reified) {
            transpilerError("Non-reified function passed to generateReifiedFunction()", i);
            errors.push_back(err);
            return nullptr;
        }
        if (self->reified_parameters.size() > types.size()) {
            transpilerError("Wrong amount of parameters for reified function call. Expected " + std::to_string(self->reified_parameters.size()) + " but got " + std::to_string(types.size()) + " for function '" + sclFunctionNameToFriendlyString(self) + "'", i);
            errors.push_back(err);
            return nullptr;
        }
        std::unordered_map<std::string, std::string> reified_mappings;
        for (size_t i = 0; i < self->reified_parameters.size(); i++) {
            const std::string& param = self->reified_parameters[i];
            if (param.empty()) {
                continue;
            }
            reified_mappings[declassifyReify(param)] = reifyType(param, removeTypeModifiers(types[i]));
        }
        bool has_A_type = false;
        bool has_B_type = false;
        bool has_AB_type = false;
        if (reified_mappings.size() == 2) {
            for (auto&& f : reified_mappings) {
                if (f.first == "A") {
                    has_A_type = true;
                } else if (f.first == "B") {
                    has_B_type = true;
                }
            }
            has_AB_type = has_A_type && has_B_type;
        }
        if (has_AB_type) {
            std::string biggestType;
            for (auto&& f : reified_mappings) {
                if (!isPrimitiveType(f.second)) {
                    biggestType = f.second;
                    break;
                }
                if (typeEquals(f.second, "float")) {
                    biggestType = "float";
                    break;
                }
                if (
                    typeEquals(f.second, "float32") ||
                    intBitWidth(biggestType) < intBitWidth(f.second) ||
                    (typeIsUnsigned(f.second) && !typeIsUnsigned(biggestType))
                ) {
                    biggestType = f.second;
                }
            }
            if (biggestType.size()) {
                reified_mappings["_ABBigger"] = biggestType;
            }
        }
        Function* f = self->clone();
        if (f->isMethod) {
            ((Method*) f)->force_add = true;
            if (!f->has_nonvirtual) {
                f->addModifier(std::string("nonvirtual"));
            }
        }
        f->clearArgs();
        for (Variable arg : self->args) {
            arg.type = reparseArgType(arg.type, reified_mappings, f->member_type);
            f->addArgument(arg);
        }
        f->return_type = reparseArgType(f->return_type, reified_mappings, f->member_type);
        f->name_token.location = body[i].location;
        std::string sigident = argsToRTSignatureIdent(f);
        f->name = f->name_without_overload + sigident;
        bool contains = false;
        for (Function* f2 : result.functions) {
            if (f2->isMethod != f->isMethod) continue;
            if (f2->name_without_overload != f->name_without_overload) continue;
            if (f2->has_reified) continue;
            if (sigident == argsToRTSignatureIdent(f2)) {
                contains = true;
                break;
            }
        }
        f->overloads.push_back(f);
        if (!contains) {
            result.functions.push_back(f);
            for (size_t i = 0; i < self->overloads.size(); i++) {
                auto it = self->overloads[i];
                f->overloads.push_back(it);
                if (self == it) continue;
                it->overloads.push_back(f);
            }
            self->overloads.push_back(f);
        }
        f->has_reified = 0;
        f->reified_parameters = self->reified_parameters;
        std::string arguments;
        if (f->args.empty() && !f->has_async) {
            arguments = "void";
        } else {
            if (f->has_async) {
                arguments = "struct _args_";
                if (f->isMethod) {
                    arguments += "mt_" + f->member_type + "$";
                } else {
                    arguments += "fn_";
                }
                arguments += f->name + "* scale_args";
            } else {
                if (f->isMethod) {
                    arguments = sclTypeToCType(result, f->args[f->args.size() - 1].type) + " Var_self";
                }
                for (size_t i = 0; i < f->args.size() - (size_t) f->isMethod; i++) {
                    std::string type = sclTypeToCType(result, f->args[i].type);
                    if (i || f->isMethod) arguments += ", ";
                    arguments += type;

                    if (type == "varargs" || type == "...") continue;
                    if (f->args[i].name.size()) {
                        arguments += " Var_" + f->args[i].name;
                    } else {
                        arguments += " param" + std::to_string(i);
                    }
                }
            }
        }
        for (size_t i = 0; i < f->body.size(); i++) {
            if (f->body[i].type != tok_identifier) continue;
            if (reified_mappings.find(f->body[i].value) != reified_mappings.end()) {
                f->body[i] = Token(tok_identifier, reified_mappings.at(f->body[i].value), SourceLocation("<generated>", 1, 1));
            }
        }
        if (f->isMethod) {
            append("%s mt_%s$%s(%s) __asm__(%s);\n", sclTypeToCType(result, f->return_type).c_str(), f->member_type.c_str(), f->name.c_str(), arguments.c_str(), generateSymbolForFunction(f).c_str());
        } else if (!f->has_reified) {
            append("%s fn_%s(%s) __asm__(%s);\n", sclTypeToCType(result, f->return_type).c_str(), f->name.c_str(), arguments.c_str(), generateSymbolForFunction(f).c_str());
        }
        return f;
    }

    Function* reifiedPreamble(Function* self, std::ostream& fp, TPResult& result, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i) {
        std::vector<std::string> types;
        if (i + 2 < body.size() && body[i + 1].type == tok_double_column) {
            safeInc(nullptr);
            safeInc(nullptr);
            if (body[i].type != tok_identifier || body[i].value != "<") {
                transpilerError("Expected '<' to specify argument types, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return nullptr;
            }
            safeInc(nullptr);
            while (body[i].value != ">") {
                FPResult type = parseType(body, i);
                if (!type.success) {
                    errors.push_back(type);
                    return nullptr;
                }
                types.push_back(removeTypeModifiers(type.value));
                safeInc(nullptr);
                if (body[i].type != tok_comma && (body[i].value != ">" || body[i].type != tok_identifier)) {
                    transpilerError("Expected ',' or '>', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return nullptr;
                }
                if (body[i].type == tok_comma) {
                    safeInc(nullptr);
                }
            }
            types.push_back(self->member_type);
            if (types.size() < (self->reified_parameters.size() - self->isMethod)) {
                transpilerError("Wrong amount of parameters for reified function call. Expected " + std::to_string(self->reified_parameters.size()) + " but got " + std::to_string(types.size()) + " for function '" + sclFunctionNameToFriendlyString(self) + "'", i);
                errors.push_back(err);
                return nullptr;
            }
        } else {
            if (typeStack.size() < self->reified_parameters.size()) {
                transpilerError("Wrong amount of parameters for reified function call. Expected " + std::to_string(self->reified_parameters.size()) + " but got " + std::to_string(typeStack.size()) + " for function '" + sclFunctionNameToFriendlyString(self) + "'", i-2);
                errors.push_back(err);
                return nullptr;
            }
            size_t startIndex = typeStack.size() - self->reified_parameters.size();
            types.reserve(typeStack.size() - startIndex);
            for (size_t i = startIndex; i < typeStack.size(); i++) {
                types.push_back(typeStack[i]);
            }
        }
        Function* f = generateReifiedFunction(self, fp, result, errors, body, i, types);
        if (f == nullptr) return nullptr;
        if (f->has_reified) {
            transpilerError("Generated function has 'reified' modifier!", i);
            errors.push_back(err);
            return nullptr;
        }
        return f;
    }

    void createReifiedCall(Function* self, std::ostream& fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i) {
        Function* f = reifiedPreamble(self, fp, result, errors, body, i);
        if (f == nullptr) {
            return;
        }
        if (f->isMethod) {
            methodCall((Method*) f, fp, result, warns, errors, body, i);
        } else {
            functionCall(f, fp, result, warns, errors, body, i);
        }
    }

    void emitFunction(Function* function, std::ostream& fp, TPResult& result, bool isMainFunction, std::vector<FPResult>& errors, std::vector<FPResult>& warns);

    void functionCall(Function* self, std::ostream& fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i, bool withIntPromotion, bool hasToCallStatic, bool checkOverloads) {
        if (!shouldCall(self, warns, errors, body, i)) {
            return;
        }
        if (self->isCVarArgs()) {
            createVariadicCall(self, fp, result, errors, body, i);
            return;
        }
        if (!self->has_operator && self->has_reified) {
            createReifiedCall(self, fp, result, warns, errors, body, i);
            return;
        }
        if (currentFunction) {
            if (self->has_private) {
                if (self->name_token.location.file != currentFunction->name_token.location.file) {
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
        bool argsEqual = checkStackType(result, self->args, withIntPromotion);
        std::vector<Function*>& overloads = self->overloads;

        if (i + 1 < body.size() && body[i + 1].type == tok_double_column) {
            size_t begin = i;
            safeInc();
            safeInc();
            if (body[i].type != tok_identifier || body[i].value != "<") {
                transpilerError("Expected '<' to specify argument types, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            safeInc();
            std::vector<std::string> argTypes;
            while (body[i].value != ">") {
                FPResult type = parseType(body, i);
                if (!type.success) {
                    errors.push_back(type);
                    return;
                }
                argTypes.push_back(removeTypeModifiers(type.value));
                safeInc();
                if (body[i].type != tok_comma && (body[i].value != ">" || body[i].type != tok_identifier)) {
                    transpilerError("Expected ',' or '>', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                if (body[i].type == tok_comma) {
                    safeInc();
                }
            }

            auto argsEqual = [&](std::vector<Variable> args) {
                if (args.size() != argTypes.size()) return false;
                bool foundWithoutPromotion = false;
                for (size_t i = 0; i < args.size(); i++) {
                    if (argTypes[i] == args[i].type) continue;
                    if (typesCompatible(result, argTypes[i], args[i].type, false)) {
                        foundWithoutPromotion = true;
                        continue;
                    }
                    return false;
                }
                if (foundWithoutPromotion) return true;
                for (size_t i = 0; i < args.size(); i++) {
                    if (argTypes[i] == args[i].type) continue;
                    if (typesCompatible(result, argTypes[i], args[i].type, true)) {
                        continue;
                    }
                    return false;
                }
                return true;
            };

            bool found = false;
            Function* have_reified = nullptr;
            for (auto&& overload : self->overloads) {
                if (overload->has_reified && !have_reified) {
                    have_reified = overload;
                    break;
                }
                if (argsEqual(overload->args)) {
                    self = overload;
                    found = true;
                    break;
                }
            }
            if (have_reified) {
                found = true;
                self = generateReifiedFunction(have_reified, fp, result, errors, body, i, argTypes);
                if (self == nullptr) {
                    return;
                }
            }
            if (!found) {
                transpilerError("No overload of '" + self->name + "' with arguments [ " + argVectorToString(argTypes) + " ] found", begin);
                errors.push_back(err);
                return;
            }

            goto callFunction;
        }

        if (checkOverloads && overloads.size() && !argsEqual) {
            for (Function* overload : overloads) {
                for (bool b : bools) {
                    if (overload->isMethod) continue;

                    bool argsEqual = checkStackType(result, overload->args, b);
                    if (argsEqual || overload->has_reified) {
                        functionCall(overload, fp, result, warns, errors, body, i, b, hasToCallStatic, false);
                        return;
                    }
                }
            }
        }

        if (self->has_operator) {
            Method* overloaded = getMethodByName(result, self->name, typeStackTop);
            if (!hasToCallStatic && overloaded) {
                methodCall(overloaded, fp, result, warns, errors, body, i);
                return;
            }

            if (self->has_reified) {
                for (Function* overload : overloads) {
                    if (overload->isMethod) continue;

                    bool argsEqual = checkStackType(result, overload->args, false);
                    if (argsEqual && !overload->has_reified) {
                        if (overload->has_operator) {
                            self = overload;
                            goto after;
                        } else {
                            functionCall(overload, fp, result, warns, errors, body, i, false, hasToCallStatic, false);
                            return;
                        }
                    }
                }
                self = reifiedPreamble(self, fp, result, errors, body, i);
                if (self == nullptr) {
                    return;
                }
            }
        after:
            size_t sym = self->has_operator;

            if (!checkStackType(result, self->args, true)) {
                {
                    transpilerError("Arguments for operator '" + sclFunctionNameToFriendlyString(self->name_without_overload) + "' do not equal inferred stack", i);
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

            std::string op = self->modifiers[sym];

            if (typeStack.size() < self->args.size()) {
                transpilerError("Cannot deduce type for operation '" + opToString(op) +  "'", i);
                errors.push_back(err);
                return;
            }

            std::string type = typeStackTop;
            std::string args = generateArgumentsForFunction(result, self);
            for (size_t m = 0; m < self->args.size(); m++) {
                typePop;
            }
            append("scale_popn(%zu);\n", self->args.size());

            typeStack.push_back(self->return_type);

            append("scale_push(%s, scale_%s(%s));\n", sclTypeToCType(result, typeStackTop).c_str(), op.c_str(), args.c_str());
            
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
            if (hasMethod(result, self->name, typeStackTop) && !hasToCallStatic) {
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

    callFunction:

        append("scale_popn(%zu);\n", self->args.size());
        std::string args = generateArgumentsForFunction(result, self);
        std::string type = typeStackTop;
        for (size_t m = 0; m < self->args.size(); m++) {
            typePop;
        }
        std::string rtype = removeTypeModifiers(self->return_type);
        bool closeThePush = false;
        if (self->return_type.size() && self->return_type.front() == '@' && !self->has_async) {
            const Struct& s = getStructByName(result, self->return_type);
            append("scale_push_value(%s, %s, ", sclTypeToCType(result, self->return_type).c_str(), (s != Struct::Null ? "MEM_FLAG_INSTANCE" : "0"));
            closeThePush = true;
        } else {
            if (rtype != "none" && rtype != "nothing" && !self->has_async) {
                append("scale_push(%s, ", sclTypeToCType(result, self->return_type).c_str());
                closeThePush = true;
            } else {
                append("");
            }
        }
        bool asyncImmediatelyAwaited = self->has_async && (i + 1) < body.size() && body[i + 1].type == tok_await;
        if (self->has_async) {
            if (asyncImmediatelyAwaited) {
                if (rtype != "none" && rtype != "nothing") {
                    append2("scale_sync(%s, fn_%s, %s%s%s)", sclTypeToCType(result, self->return_type).c_str(), self->name.c_str(), functionArgsToStructBody(self, result).c_str(), args.size() ? ", " : "", args.c_str());
                } else {
                    append2("scale_sync_v(fn_%s, %s%s%s)", self->name.c_str(), functionArgsToStructBody(self, result).c_str(), args.size() ? ", " : "", args.c_str());
                }
            } else {
                append2("scale_async(fn_%s, %s%s%s)", self->name.c_str(), functionArgsToStructBody(self, result).c_str(), args.size() ? ", " : "", args.c_str());
            }
        } else {
            append2("fn_%s(%s)", self->name.c_str(), args.c_str());
        }
        if (closeThePush) {
            append2(")");
        }
        append2(";\n");
        if (self->has_async && !asyncImmediatelyAwaited) {
            typeStack.push_back("async<" + self->return_type + ">");
        } else if (rtype != "none" && rtype != "nothing") {
            if (self->return_type.front() == '@') {
                typeStack.push_back(self->return_type.substr(1));
            } else {
                typeStack.push_back(self->return_type);
            }
        }
        if (asyncImmediatelyAwaited) {
            i++;
        }
    }

    std::string sclFunctionNameToFriendlyString(std::string name) {
        name = name.substr(0, name.find("$$ol"));
        for (auto&& p : funcNameIdents) {
            if (p.second == name) {
                name = p.first;
                break;
            }
        }
        name = replaceAll(name, "\\$\\d+", "");
        name = replaceAll(name, "\\$", "::");

        if (strstarts(name, "::lambda")) {
            name = "<lambda" + name.substr(8, name.find_first_of("(") - 8) + ">";
        }

        return name;
    }

    std::string retemplate(std::string type) {
        static std::unordered_map<std::string, std::string> cache;

        auto it = cache.find(type);
        if (it != cache.end()) return it->second;

        if (strstarts(type, "$T")) type = type.substr(2);

        std::string ret = "";
        const char* s = type.c_str();
        while (*s) {
            if (strncmp(s, "$$B", 3) == 0) {
                ret += "<";
                s += 3;
            } else if (strncmp(s, "$$E", 3) == 0) {
                ret += ">";
                s += 3;
            } else if (strncmp(s, "$x", 2) == 0) {
                s += 2;
                const char data[] = {
                    s[0],
                    s[1],
                    0
                };
                s += 2;
                ret += (char) std::strtol(data, nullptr, 16);
            } else if (*s == '$') {
                ret += "::";
                s++;
            } else {
                ret += *s;
                s++;
            }
        }

        return cache[type] = ret;
    }

    std::string sclFunctionNameToFriendlyString(Function* f) {
        std::string name = f->name_without_overload;
        name = sclFunctionNameToFriendlyString(name);
        if (f->isMethod) {
            name = retemplate(f->member_type) + ":" + name;
        } else if (!f->member_type.empty()) {
            name = retemplate(f->member_type) + "::" + name.substr(f->member_type.size() + 1);
        } else if (strcontains(name, "$$B")) {
            name = retemplate(name);
        }
        if (f->has_lambda) {
            name += "[" + f->lambdaName + "]";
        }
        name += "(";
        for (size_t i = 0; i < f->args.size(); i++) {
            const Variable& v = f->args[i];
            if (f->isMethod && v.name == "self") continue;
            if (i) name += ", ";
            name += ":" + retemplate(v.type);
        }
        name += "): " + retemplate(f->return_type);
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

    std::vector<Method*> makeVTable(TPResult& res, std::string name) {
        if (vtables.find(name) != vtables.end()) {
            return vtables[name];
        }
        std::vector<Method*> vtable;
        const Struct& s = getStructByName(res, name);
        if (s.super.size()) {
            auto parent = getStructByName(res, s.super);
            if (parent == Struct::Null) {
                std::cerr << "Error: Struct '" << s.name << "' extends '" << s.super << "', but '" << s.super << "' does not exist." << std::endl;
                exit(1);
            }
            const std::vector<Method*>& parentVTable = makeVTable(res, parent.name);
            for (auto&& m : parentVTable) {
                vtable.push_back(m);
            }
        }
        for (auto&& m : methodsOnType(res, name)) {
            if (m->has_reified) continue;
            long indexInVtable = -1;
            for (size_t i = 0; i < vtable.size(); i++) {
                if (
                    vtable[i]->name == m->name && (argVecEquals(vtable[i]->args, m->args) || m->has_overrides)
                ) {
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
} // namespace sclc
