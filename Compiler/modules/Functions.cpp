#include <csignal>

#include "../headers/Common.hpp"
#include "../headers/Types.hpp"
#include "../headers/TranspilerDefs.hpp"
#include "../headers/Functions.hpp"

#ifndef VERSION
#define VERSION ""
#endif

namespace sclc {
    extern int scopeDepth;
    extern std::stack<std::string> typeStack;
    extern Function* currentFunction;
    extern Struct currentStruct;
    extern std::map<std::string, std::vector<Method*>> vtables;
    extern int isInUnsafe;

    std::unordered_map<Function*, std::string> functionPtrs;
    std::vector<bool> bools{false, true};
    
    std::string generateArgumentsForFunction(TPResult& result, Function *func) {
        std::string args = "";
        size_t maxValue = func->args.size();
        if (func->isMethod) {
            args = "*(" + sclTypeToCType(result, func->member_type) + "*) _scl_positive_offset(" + std::to_string(func->args.size() - 1) + ")";
            maxValue--;
        }
        if (func->isCVarArgs())
            maxValue--;

        for (size_t i = 0; i < maxValue; i++) {
            const Variable& arg = func->args[i];
            if (func->isMethod || i)
                args += ", ";

            bool isValueStructParam = arg.type.front() == '*';
            if (isValueStructParam) args += "*(";
            args += "*(" + sclTypeToCType(result, arg.type.substr(isValueStructParam)) + "*) _scl_positive_offset(" + std::to_string(i) + ")";
            if (isValueStructParam) args += ")";
        }
        return args;
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
        
        append("_scl_popn(%zu);\n", amountOfVarargs);

        for (long i = amountOfVarargs - 1; i >= 0; i--) {
            std::string nextType = typeStack.top();
            typeStack.pop();
            std::string ctype = sclTypeToCType(result, nextType);
            append("%s vararg%ld = *(%s*) _scl_positive_offset(%ld);\n", ctype.c_str(), i, ctype.c_str(), i);
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
            append("(_stack.sp++)->i = %zu;\n", amountOfVarargs);
            typeStack.push("int");
        }

        if (f->args.size() > 1)
            append("_scl_popn(%zu);\n", f->args.size() - 1);

        for (size_t i = 0; i < f->args.size(); i++) {
            typePop;
        }

        if (f->return_type.size() && f->return_type.front() == '*') {
            append("{\n");
            scopeDepth++;
            append("%s tmp = ", sclTypeToCType(result, f->return_type).c_str());
        } else {
            if (f->return_type != "none" && f->return_type != "nothing") {
                append("*(%s*) _stack.sp++ = ", sclTypeToCType(result, f->return_type).c_str());
            } else {
                append("");
            }
        }

        append2("fn_%s(%s);\n", f->name.c_str(), args.c_str());
        if (f->return_type.size() && f->return_type.front() == '*') {
            append("(_stack.sp++)->v = _scl_alloc(sizeof(%s));\n", sclTypeToCType(result, f->return_type).c_str());
            append("memcpy((_stack.sp - 1)->v, &tmp, sizeof(%s));\n", sclTypeToCType(result, f->return_type).c_str());
            scopeDepth--;
            append("}\n");
        }
        scopeDepth--;
        append("}\n");
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
            symbol = sclFunctionNameToFriendlyString(f);
        } else if (f->isMethod) {
            Method* m = ((Method*) f);
            if (f->has_foreign) {
                symbol = m->member_type + "$" + f->finalName();
            } else {
                symbol = sclFunctionNameToFriendlyString(f);
            }
        } else {
            std::string finalName = symbol;
            if (f->has_foreign) {
                symbol = finalName;
            } else {
                symbol = sclFunctionNameToFriendlyString(f);
            }
        }

        return "_scl_macro_to_string(__USER_LABEL_PREFIX__) \"" + symbol + "\"";
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

    void methodCall(Method* self, FILE* fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t i, bool ignoreArgs, bool doActualPop, bool withIntPromotion, bool onSuperType, bool checkOverloads) {
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
        
        if (doActualPop) {
            for (size_t m = 0; m < self->args.size(); m++) {
                typePop;
            }
        }
        bool found = false;
        size_t argc;
        if ((argc = self->args.size())) {
            append("_scl_popn(%zu);\n", argc);
        }
        std::string args = generateArgumentsForFunction(result, self);
        if (Main.options.debugBuild) {
            append("CAST0((_stack.sp - 1)->v, %s, 0x%x);\n", self->member_type.c_str(), id(self->member_type.c_str()));
        }
        if (self->return_type.size() && self->return_type.front() == '*') {
            append("{\n");
            scopeDepth++;
            append("%s tmp = ", sclTypeToCType(result, self->return_type).c_str());
        } else {
            if (self->return_type != "none" && self->return_type != "nothing") {
                append("*(%s*) _stack.sp++ = ", sclTypeToCType(result, self->return_type).c_str());
            } else {
                append("");
            }
        }
        if (getInterfaceByName(result, self->member_type) == nullptr) {
            auto vtable = vtables[self->member_type];
            size_t index = 0;
            for (auto&& method : vtable) {
                if (method->operator==(self)) {
                    std::string functionPtrCast = getFunctionType(result, self);

                    append2("((%s) (*(%s*) _scl_positive_offset(%zu))->", functionPtrCast.c_str(), sclTypeToCType(result, self->member_type).c_str(), argc - 1);
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
            std::string functionPtrCast = getFunctionType(result, self);
            append(
                "((%s) (_scl_get_vtable_function(0, *(scl_any*) _scl_positive_offset(%zu), \"%s%s\")))(%s);\n",
                functionPtrCast.c_str(),
                argc - 1,
                self->name.c_str(),
                rtSig.c_str(),
                args.c_str()
            );
            found = true;
        }
        if (self->return_type.size() && self->return_type.front() == '*') {
            append("(_stack.sp++)->v = _scl_alloc(sizeof(%s));\n", sclTypeToCType(result, self->return_type).c_str());
            append("memcpy((_stack.sp - 1)->v, &tmp, sizeof(%s));\n", sclTypeToCType(result, self->return_type).c_str());
            scopeDepth--;
            append("}\n");
        }
        if (removeTypeModifiers(self->return_type) != "none" && removeTypeModifiers(self->return_type) != "nothing") {
            typeStack.push(self->return_type.substr(self->return_type.front() == '*'));
        }
        if (!found) {
            transpilerError("Method '" + sclFunctionNameToFriendlyString(self) + "' not found on type '" + self->member_type + "'", i);
            errors.push_back(err);
        }
    }

    void generateUnsafeCallF(Function* self, FILE* fp, TPResult& result) {
        if (self->args.size() > 0)
            append("_scl_popn(%zu);\n", self->args.size());
        if (removeTypeModifiers(self->return_type) == "none" || removeTypeModifiers(self->return_type) == "nothing") {
            append("fn_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
        } else {
            append("*(%s*) (_stack.sp++) = fn_%s(%s);\n", sclTypeToCType(result, self->return_type).c_str(), self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
        }
    }

    void generateUnsafeCall(Method* self, FILE* fp, TPResult& result) {
        if (self->args.size() > 0)
            append("_scl_popn(%zu);\n", self->args.size());
        if (removeTypeModifiers(self->return_type) == "none" || removeTypeModifiers(self->return_type) == "nothing") {
            append("mt_%s$%s(%s);\n", self->member_type.c_str(), self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
        } else {
            append("*(%s*) (_stack.sp++) = mt_%s$%s(%s);\n", sclTypeToCType(result, self->return_type).c_str(), self->member_type.c_str(), self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
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
                append("_scl_popn(2);\n");
            } else {
                typeStack.pop();
                append("_scl_popn(1);\n");
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
        
        if (self->args.size() > 0) {
            append("_scl_popn(%zu);\n", self->args.size());
        }
        for (size_t m = 0; m < self->args.size(); m++) {
            typePop;
        }
        if (self->name == "throw" || self->name == "builtinUnreachable") {
            if (currentFunction->has_restrict) {
                if (currentFunction->isMethod) {
                    append("fn_Process$unlock((scl_SclObject) Var_self);\n");
                } else {
                    append("fn_Process$unlock(function_lock$%s);\n", currentFunction->finalName().c_str());
                }
            }
        }
        if (self->return_type.size() && self->return_type.front() == '*') {
            append("{\n");
            scopeDepth++;
            append("%s tmp = ", sclTypeToCType(result, self->return_type).c_str());
        } else {
            if (removeTypeModifiers(self->return_type) != "none" && removeTypeModifiers(self->return_type) != "nothing") {
                append("*(%s*) _stack.sp++ = ", sclTypeToCType(result, self->return_type).c_str());
                typeStack.push(self->return_type);
            } else {
                append("");
            }
        }
        append2("fn_%s(%s);\n", self->finalName().c_str(), generateArgumentsForFunction(result, self).c_str());
        if (self->return_type.size() && self->return_type.front() == '*') {
            append("(_stack.sp++)->v = _scl_alloc(sizeof(%s));\n", sclTypeToCType(result, self->return_type).c_str());
            append("memcpy((_stack.sp - 1)->v, &tmp, sizeof(%s));\n", sclTypeToCType(result, self->return_type).c_str());
            scopeDepth--;
            append("}\n");
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
            name += ":" + v.type;
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
