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
    extern std::map<std::string, std::vector<Method*>> vtables;
    extern int isInUnsafe;

    std::unordered_map<Function*, std::string> functionPtrs;
    std::vector<bool> bools{false, true};

    std::string retemplate(std::string type);
    
    std::string generateArgumentsForFunction(TPResult& result, Function *func) {
        std::string args = "";
        size_t maxValue = func->args.size();
        if (func->isMethod) {
            std::string self_type = func->args[func->args.size() - 1].type;
            if (self_type.front() == '@') {
                args += "*(";
            }
            args += "_scl_positive_offset(" + std::to_string(func->args.size() - 1) + ", " + sclTypeToCType(result, func->member_type) + ")";
            if (self_type.front() == '@') {
                args += ")";
            }
            maxValue--;
        }
        if (func->isCVarArgs())
            maxValue--;

        for (size_t i = 0; i < maxValue; i++) {
            const Variable& arg = func->args[i];
            if (func->isMethod || i)
                args += ", ";

            bool isValueStructParam = arg.type.front() == '@';
            if (isValueStructParam) args += "*(";
            args += "_scl_positive_offset(" + std::to_string(i) + ", " + sclTypeToCType(result, arg.type.substr(isValueStructParam)) + ")";
            if (isValueStructParam) args += ")";
        }
        return args;
    }

    void createVariadicCall(Function* f, std::ostream& fp, TPResult& result, std::vector<FPResult>& errors, std::vector<Token> body, size_t& i) {
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
        
        append("_scl_popn(%zu);\n", amountOfVarargs);

        for (long i = amountOfVarargs - 1; i >= 0; i--) {
            std::string nextType = typeStack.back();
            typeStack.pop_back();
            std::string ctype = sclTypeToCType(result, nextType);
            append("%s vararg%ld = _scl_positive_offset(%ld, %s);\n", ctype.c_str(), i, i, ctype.c_str());
        }

        std::string args = generateArgumentsForFunction(result, f);

        for (size_t i = 0; i < amountOfVarargs; i++) {
            args += ", ";
            if (f->varArgsParam().name.size()) {
                args += "&";
            }
            args += "vararg" + std::to_string(i);
        }

        if (f->varArgsParam().name.size()) {
            append("_scl_push(scl_int, %zu);\n", amountOfVarargs);
            typeStack.push_back("int");
        }

        if (f->args.size() > 1)
            append("_scl_popn(%zu);\n", f->args.size() - 1);

        for (size_t i = 0; i < f->args.size(); i++) {
            typePop;
        }

        bool closeThePush = false;
        if (f->return_type.size() && f->return_type.front() == '@' && !f->has_async) {
            append("{\n");
            scopeDepth++;
            append("%s tmp = ", sclTypeToCType(result, f->return_type).c_str());
        } else {
            if (f->return_type != "none" && f->return_type != "nothing" && !f->has_async) {
                append("_scl_push(%s, ", sclTypeToCType(result, f->return_type).c_str());
                closeThePush = true;
            } else {
                append("");
            }
        }

        if (f->has_async) {
            if (args.size()) {
                append2("_scl_async(fn_%s, fn_%s, %s)", f->name.c_str(), f->name.c_str(), args.c_str());
            } else {
                append2("_scl_async(fn_%s, fn_%s)", f->name.c_str(), f->name.c_str());
            }
        } else {
            append2("fn_%s(%s)", f->name.c_str(), args.c_str());
        }
        if (closeThePush) {
            append2(")");
        }
        append2(";\n");

        if (f->return_type.size() && f->return_type.front() == '@' && !f->has_async) {
            std::string cType = sclTypeToCType(result, f->return_type);
            const Struct& s = getStructByName(result, f->return_type);
            if (s != Struct::Null) {
                append("_scl_push(scl_any, ALLOC(%s));\n", s.name.c_str());
            } else {
                append("_scl_push(scl_any, _scl_alloc(sizeof(%s)));\n", cType.c_str());
            }
            append("memcpy(_scl_top(scl_any), &tmp, sizeof(%s));\n", cType.c_str());
            scopeDepth--;
            append("}\n");
        }
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

        std::string name = f->name;
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
            std::string label = f->getModifier(f->has_asm + 1);

            return "\"" + label + "\"";
        }

        if (f->has_cdecl) {
            std::string cLabel = f->getModifier(f->has_cdecl + 1);

            return "_scl_macro_to_string(__USER_LABEL_PREFIX__) \"" + cLabel + "\"";
        }

        if (!f->isMethod && !Main::options::noMain && f->name == "main") {
            return "_scl_macro_to_string(__USER_LABEL_PREFIX__) \"main\"";
        }

        if (f->has_foreign) {
            if (f->isMethod) {
                return "_scl_macro_to_string(__USER_LABEL_PREFIX__) \"" + f->member_type + "$" + f->name + "\"";
            } else {
                return "_scl_macro_to_string(__USER_LABEL_PREFIX__) \"" + f->name + "\"";
            }
        }

        std::string symbol = (f->isMethod ? "_M" : (f->has_lambda ? "_L" : "_F")) + generateInternal(f);

        return "_scl_macro_to_string(__USER_LABEL_PREFIX__) \"" + symbol + "\"";
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
               (name == "operator$elvis") ||
               (name == "operator$assert_not_nil") ||
               (name == "operator$not");
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

    void methodCall(Method* self, std::ostream& fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i, bool ignoreArgs, bool doActualPop, bool withIntPromotion, bool onSuperType, bool checkOverloads) {
        if (!shouldCall(self, warns, errors, body, i)) {
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
                FPResult type = parseType(body, &i, getTemplates(result, self));
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
        }

    callMethod:

        std::string type = typeStackTop;
        if (doActualPop) {
            if (isSelfType(self->return_type)) {
                for (size_t m = 0; m < self->args.size(); m++) {
                    if (!typesCompatible(result, typeStackTop, type, true)) {
                        if (typeCanBeNil(self->args[m].type) && typeCanBeNil(typeStackTop)) {
                            goto cont;
                        }
                        {
                            transpilerError("Method returning 'self' requires all arguments to be of the same type", i);
                            errors.push_back(err);
                        }
                        transpilerError("Expected '" + type + "', but got '" + typeStackTop + "'", i);
                        err.isNote = true;
                        errors.push_back(err);
                        return;
                    }
                cont:
                    typePop;
                }
            } else {
                for (size_t m = 0; m < self->args.size(); m++) {
                    typePop;
                }
            }
        }
        bool found = false;
        size_t argc;
        if ((argc = self->args.size())) {
            append("_scl_popn(%zu);\n", argc);
        }
        std::string args = generateArgumentsForFunction(result, self);
        if (Main::options::debugBuild) {
            append("CAST0(_scl_top(scl_any), %s, 0x%lxUL);\n", self->member_type.c_str(), id(self->member_type.c_str()));
        }
        bool closeThePush = false;
        if (self->return_type.size() && self->return_type.front() == '@' && !self->has_async) {
            append("{\n");
            scopeDepth++;
            append("%s tmp = ", sclTypeToCType(result, self->return_type).c_str());
        } else {
            if (self->return_type != "none" && self->return_type != "nothing" && !self->has_async) {
                append("_scl_push(%s, ", sclTypeToCType(result, self->return_type).c_str());
                closeThePush = true;
            } else {
                append("");
            }
        }
        if (self->has_nonvirtual) {
            if (self->has_async) {
                append("_scl_async(mt_%s$%s, mt_%s$%s, %s)", self->member_type.c_str(), self->name.c_str(), self->member_type.c_str(), self->name.c_str(), args.c_str());
            } else {
                append("mt_%s$%s(%s)", self->member_type.c_str(), self->name.c_str(), args.c_str());
            }
            found = true;
        } else if (getInterfaceByName(result, self->member_type) == nullptr) {
            auto vtable = vtables[self->member_type];
            size_t index = 0;
            for (auto&& method : vtable) {
                if (method->operator==(self)) {
                    std::string functionPtrCast = getFunctionType(result, self);
                    if (self->has_async) {
                        append2("_scl_async((%s) _scl_positive_offset(%zu, %s)->", functionPtrCast.c_str(), argc - 1, sclTypeToCType(result, self->member_type).c_str());
                        if (onSuperType) {
                            append2("$statics->super_vtable[%zu].ptr, mt_%s$%s, %s)", index, args.c_str(), self->member_type.c_str(), self->name.c_str());
                        } else {
                            append2("$fast[%zu], mt_%s$%s, %s)", index, self->member_type.c_str(), self->name.c_str(), args.c_str());
                        }
                    } else {
                        append2("((%s) _scl_positive_offset(%zu, %s)->", functionPtrCast.c_str(), argc - 1, sclTypeToCType(result, self->member_type).c_str());
                        if (onSuperType) {
                            append2("$statics->super_vtable[%zu].ptr)(%s)", index, args.c_str());
                        } else {
                            append2("$fast[%zu])(%s)", index, args.c_str());
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
            if (self->has_async) {
                transpilerError("Calling async method on interface is not supported", i);
                warns.push_back(err);
            }
            append(
                "((%s) (_scl_get_vtable_function(0, _scl_positive_offset(%zu, scl_any), \"%s%s\")))(%s)",
                functionPtrCast.c_str(),
                argc - 1,
                self->name.c_str(),
                rtSig.c_str(),
                args.c_str()
            );
            found = true;
        }
        if (closeThePush) {
            append2(")");
        }
        append2(";\n");
        if (self->return_type.size() && self->return_type.front() == '@' && !self->has_async) {
            std::string cType = sclTypeToCType(result, self->return_type);
            const Struct& s = getStructByName(result, self->return_type);
            if (s != Struct::Null) {
                append("_scl_push(scl_any, ALLOC(%s));\n", s.name.c_str());
            } else {
                append("_scl_push(scl_any, _scl_alloc(sizeof(%s)));\n", cType.c_str());
            }
            append("memcpy(_scl_top(scl_any), &tmp, sizeof(%s));\n", cType.c_str());
            scopeDepth--;
            append("}\n");
        }
        if (self->has_async) {
            typeStack.push_back("async<" + self->return_type + ">");
        } else if (removeTypeModifiers(self->return_type) != "none" && removeTypeModifiers(self->return_type) != "nothing") {
            if (self->return_type.front() == '@') {
                typeStack.push_back(self->return_type.substr(1));
            } else {
                if (isSelfType(self->return_type)) {
                    std::string retType = "";
                    if (self->return_type.front() == '@') {
                        retType += "@";
                    }
                    retType += removeTypeModifiers(type);
                    if (typeCanBeNil(self->return_type)) {
                        retType += "?";
                    }
                    typeStack.push_back(retType);
                } else {
                    typeStack.push_back(self->return_type);
                }
            }
        }
        if (!found) {
            transpilerError("Method '" + sclFunctionNameToFriendlyString(self) + "' not found on type '" + self->member_type + "'", i);
            errors.push_back(err);
        }
    }

    void generateUnsafeCallF(Function* self, std::ostream& fp, TPResult& result) {
        if (self->args.size() > 0)
            append("_scl_popn(%zu);\n", self->args.size());
        std::string args = generateArgumentsForFunction(result, self).c_str();
        if (self->has_async) {
            if (args.size()) {
                append("_scl_async(fn_%s, fn_%s, %s);\n", self->name.c_str(), self->name.c_str(), args.c_str());
            } else {
                append("_scl_async(fn_%s, fn_%s);\n", self->name.c_str(), self->name.c_str());
            }
        } else {
            if (removeTypeModifiers(self->return_type) == "none" || removeTypeModifiers(self->return_type) == "nothing") {
                append("fn_%s(%s);\n", self->name.c_str(), args.c_str());
            } else {
                append("_scl_push(%s, fn_%s(%s));\n", sclTypeToCType(result, self->return_type).c_str(), self->name.c_str(), args.c_str());
            }
        }
    }

    void generateUnsafeCall(Method* self, std::ostream& fp, TPResult& result) {
        if (self->args.size() > 0)
            append("_scl_popn(%zu);\n", self->args.size());
        std::string args = generateArgumentsForFunction(result, self).c_str();
        if (self->has_async) {
            if (args.size()) {
                append("_scl_async(mt_%s$%s, mt_%s$%s, %s);\n", self->member_type.c_str(), self->name.c_str(), self->member_type.c_str(), self->name.c_str(), args.c_str());
            } else {
                append("_scl_async(mt_%s$%s, mt_%s$%s);\n", self->member_type.c_str(), self->name.c_str(), self->member_type.c_str(), self->name.c_str());
            }
        } else {
            if (removeTypeModifiers(self->return_type) == "none" || removeTypeModifiers(self->return_type) == "nothing") {
                append("mt_%s$%s(%s);\n", self->member_type.c_str(), self->name.c_str(), args.c_str());
            } else {
                append("_scl_push(%s, mt_%s$%s(%s));\n", sclTypeToCType(result, self->return_type).c_str(), self->member_type.c_str(), self->name.c_str(), args.c_str());
            }
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
        if (op == "div") return "/";
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

    void functionCall(Function* self, std::ostream& fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i, bool withIntPromotion, bool hasToCallStatic, bool checkOverloads) {
        if (!shouldCall(self, warns, errors, body, i)) {
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
                FPResult type = parseType(body, &i, getTemplates(result, self));
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
            for (auto&& overload : self->overloads) {
                if (argsEqual(overload->args)) {
                    self = overload;
                    found = true;
                    break;
                }
            }
            if (!found) {
                transpilerError("No overload of '" + self->name + "' with arguments [ " + argVectorToString(argTypes) + " ] found", begin);
                errors.push_back(err);
                return;
            }

            goto callFunction;
        }

        {
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

                Method* overloaded = getMethodByName(result, self->name, typeStackTop);
                if (!hasToCallStatic && overloaded) {
                    methodCall(overloaded, fp, result, warns, errors, body, i);
                    return;
                }

                if (!checkStackType(result, self->args, true)) {
                    {
                        transpilerError("Arguments for operator '" + opToString(self->modifiers[sym]) + "' do not equal inferred stack", i);
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

                std::string argType = "";
                if (op == "at") {
                    argType = typeStackTop;
                }

                std::string type = typeStackTop;
                if (isSelfType(self->return_type)) {
                    for (size_t m = 0; m < self->args.size(); m++) {
                        if (!typesCompatible(result, typeStackTop, type, true)) {
                            if (typeCanBeNil(self->args[m].type) && typeCanBeNil(typeStackTop)) {
                                goto cont;
                            }
                            {
                                transpilerError("Function returning 'self' requires all arguments to be of the same type", i);
                                errors.push_back(err);
                            }
                            transpilerError("Expected '" + type + "', but got '" + typeStackTop + "'", i);
                            err.isNote = true;
                            errors.push_back(err);
                            return;
                        }
                    cont:
                        typePop;
                    }
                } else {
                    for (size_t m = 0; m < self->args.size(); m++) {
                        typePop;
                    }
                }
                append("_scl_popn(%zu);\n", self->args.size());
                std::string args = generateArgumentsForFunction(result, self);

                append("_scl_push(%s, _scl_%s(%s));\n", sclTypeToCType(result, self->return_type).c_str(), op.c_str(), args.c_str());

                if (op == "at") {
                    if (argType.front() == '[' && argType.back() == ']') {
                        argType = argType.substr(1, argType.size() - 2);
                    } else {
                        argType = "any";
                    }
                    typeStack.push_back(argType);
                } else {
                    if (isSelfType(self->return_type)) {
                        std::string retType = "";
                        if (self->return_type.front() == '@') {
                            retType += "@";
                        }
                        retType += removeTypeModifiers(type);
                        if (typeCanBeNil(self->return_type)) {
                            retType += "?";
                        }
                        typeStack.push_back(retType);
                    } else {
                        typeStack.push_back(self->return_type);
                    }
                }

                return;
            }

            if (!hasToCallStatic && opFunc(self->name) && hasMethod(result, self->name, typeStackTop)) {
            makeMethodCallInstead:
                Method* method = getMethodByName(result, self->name, typeStackTop);
                methodCall(method, fp, result, warns, errors, body, i);
                return;
            }
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

        
        if (self->args.size() > 0) {
            append("_scl_popn(%zu);\n", self->args.size());
        }
        std::string type = typeStackTop;
        if (isSelfType(self->return_type)) {
            for (size_t m = 0; m < self->args.size(); m++) {
                if (!typesCompatible(result, typeStackTop, type, true)) {
                    if (typeCanBeNil(self->args[m].type) && typeCanBeNil(typeStackTop)) {
                        goto cont2;
                    }
                    {
                        transpilerError("Function returning 'self' requires all arguments to be of the same type", i);
                        errors.push_back(err);
                    }
                    transpilerError("Expected '" + retemplate(type) + "', but got '" + retemplate(typeStackTop) + "'", i);
                    err.isNote = true;
                    errors.push_back(err);
                    return;
                }
            cont2:
                typePop;
            }
        } else {
            for (size_t m = 0; m < self->args.size(); m++) {
                typePop;
            }
        }
        if (self->name == "throw" || self->name == "builtinUnreachable") {
            if (currentFunction->has_restrict) {
                if (currentFunction->isMethod) {
                    append("_scl_unlock(Var_self);\n");
                } else {
                    append("_scl_unlock(function_lock$%s);\n", currentFunction->name.c_str());
                }
            }
        }
        bool closeThePush = false;
        if (self->return_type.size() && self->return_type.front() == '@' && !self->has_async) {
            append("{\n");
            scopeDepth++;
            append("%s tmp = ", sclTypeToCType(result, self->return_type).c_str());
        } else {
            if (removeTypeModifiers(self->return_type) != "none" && removeTypeModifiers(self->return_type) != "nothing" && !self->has_async) {
                append("_scl_push(%s, ", sclTypeToCType(result, self->return_type).c_str());
                closeThePush = true;
            } else {
                append("");
            }
        }
        std::string args = generateArgumentsForFunction(result, self);
        if (self->has_async) {
            append("_scl_async(fn_%s%s%s, fn_%s)", self->name.c_str(), args.size() ? ", " : "", args.c_str(), self->name.c_str());
        } else {
            append2("fn_%s(%s)", self->name.c_str(), args.c_str());
        }
        if (closeThePush) {
            append2(")");
        }
        append2(";\n");

        if (self->return_type.size() && self->return_type.front() == '@' && !self->has_async) {
            std::string cType = sclTypeToCType(result, self->return_type);
            const Struct& s = getStructByName(result, self->return_type);
            if (s != Struct::Null) {
                append("_scl_push(scl_any, ALLOC(%s));\n", s.name.c_str());
                append("_scl_copy_fields(_scl_top(scl_any), &tmp, sizeof(struct Struct_%s));\n", s.name.c_str());
            } else {
                append("_scl_push(scl_any, _scl_alloc(sizeof(%s)));\n", cType.c_str());
                append("memcpy(_scl_top(scl_any), &tmp, sizeof(%s));\n", cType.c_str());
            }
            scopeDepth--;
            append("}\n");
        }
        if (self->has_async) {
            typeStack.push_back("async<" + self->return_type + ">");
        } else if (removeTypeModifiers(self->return_type) != "none" && removeTypeModifiers(self->return_type) != "nothing") {
            if (self->return_type.front() == '@') {
                typeStack.push_back(self->return_type.substr(1));
            } else {
                if (isSelfType(self->return_type)) {
                    std::string retType = "";
                    if (self->return_type.front() == '@') {
                        retType += "@";
                    }
                    retType += removeTypeModifiers(type);
                    if (typeCanBeNil(self->return_type)) {
                        retType += "?";
                    }
                    typeStack.push_back(retType);
                } else {
                    typeStack.push_back(self->return_type);
                }
            }
        }
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
        else if (name == "operator$elvis") name = "?:";
        name = replaceAll(name, "\\$\\d+", "");
        name = replaceAll(name, "\\$", "::");

        if (strstarts(name, "::lambda")) {
            name = "<lambda" + name.substr(8, name.find_first_of("(") - 8) + ">";
        }

        return name;
    }

    std::string retemplate(std::string type) {
        static std::unordered_map<std::string, std::string> cache;

        if (cache.find(type) != cache.end()) {
            return cache[type];
        }

        std::string ret = "";
        const char* s = type.c_str();
        while (*s) {
            if (strncmp(s, "$$b", 3) == 0) {
                ret += "<";
                s += 3;
            } else if (strncmp(s, "$$e", 3) == 0) {
                ret += ">";
                s += 3;
            } else if (strncmp(s, "$$n", 3) == 0) {
                ret += ", ";
                s += 3;
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
        std::string name = f->name;
        name = sclFunctionNameToFriendlyString(name);
        if (f->isMethod) {
            name = retemplate(f->member_type) + ":" + name;
        }
        if (f->has_lambda) {
            name += "[" + std::to_string(f->lambdaIndex) + "]";
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
