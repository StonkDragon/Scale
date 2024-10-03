
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Column) {
        noUnused;
        if (strstarts(typeStackTop, "lambda(") || typeStackTop == "lambda") {
            safeInc();
            if (typeStackTop == "lambda") {
                transpilerError("Generic lambda has to be cast to a typed lambda before calling", i);
                errors.push_back(err);
                return;
            }
            std::string lambdaType = typeStackTop;
            typePop;
            std::string returnType = lambdaReturnType(lambdaType);
            std::string rType = removeTypeModifiers(returnType);
            auto args = lambdaArgs(lambdaType);
            size_t argAmount = args.size();
            std::string op = body[i].value;
            
            bool argsEqual = checkStackType(result, args);
            if (!argsEqual && argsContainsIntType(args)) {
                argsEqual = checkStackType(result, args, true);
            }
            if (!argsEqual) {
                {
                    transpilerError("Arguments for lambda '" + lambdaType + "' do not equal inferred stack", i);
                    errors.push_back(err);
                }
                transpilerError("Expected: [ " + Color::BLUE + argVectorToString(args) + Color::RESET + " ], but got: [ " + Color::RED + stackSliceToString(args.size()) + Color::RESET + " ]", i);
                err.isNote = true;
                errors.push_back(err);
                return;
            }
            
            std::string argTypes = "";
            std::string argGet = "";
            for (size_t argc = argAmount; argc; argc--) {
                argTypes += sclTypeToCType(result, args[argAmount - argc].type);
                argGet += "scale_positive_offset(" + std::to_string(argAmount - argc) + ", " + sclTypeToCType(result, args[argAmount - argc].type) + ")";
                argTypes += ", ";
                argGet += ", ";
                typePop;
            }
            argGet += "lambda";
            argTypes += "void*";

            if (op == "accept") {
                append("{\n");
                scopeDepth++;
                append("%s(*(*lambda))(%s);\n", sclTypeToCType(result, returnType).c_str(), argTypes.c_str());
                append("lambda = scale_pop(typeof(lambda));\n");
                append("scale_popn(%zu);\n", argAmount);
                if (rType == "none" || rType == "nothing") {
                    append("(*lambda)(%s);\n", argGet.c_str());
                } else {
                    append("scale_push(%s, (*lambda)(%s));\n", sclTypeToCType(result, returnType).c_str(), argGet.c_str());
                    typeStack.push_back(returnType);
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
        bool isConst = typeIsConst(type);
        std::string tmp = removeTypeModifiers(type);
        if (tmp.size() > 2 && tmp.front() == '[' && tmp.back() == ']') {
            safeInc();
            if (body[i].type != tok_identifier) {
                transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            std::string op = body[i].value;

            Function* f = getFunctionByName(result, "List$" + op);
            if (f == nullptr) {
                transpilerError("No Function definition for '" + tmp + "::" + op + "' found", i);
                errors.push_back(err);
                return;
            }

            functionCall(f, fp, result, warns, errors, body, i);
            
            return;
        }
        Struct s = getStructByName(result, type);

        auto callContainedLambda = [&fp, &errors, &body, &result, &i](const Variable& v, const std::string& container, std::string& help) {
            std::string type = v.type;
            if (!strstarts(removeTypeModifiers(type), "lambda(")) {
                help = ". Maybe you meant to use '.' instead of ':' here";
                return false;
            }

            std::string returnType = lambdaReturnType(type);
            auto args = lambdaArgs(type);

            append("{\n");
            scopeDepth++;

            append("%s tmp = scale_pop(scale_any);\n", sclTypeToCType(result, container).c_str());
            typePop;
            
            bool argsEqual = checkStackType(result, args);
            if (!argsEqual && argsContainsIntType(args)) {
                argsEqual = checkStackType(result, args, true);
            }
            if (!argsEqual) {
                {
                    transpilerError("Arguments for lambda '" + type + "' do not equal inferred stack", i);
                    errors.push_back(err);
                }
                transpilerError("Expected: [ " + Color::BLUE + argVectorToString(args) + Color::RESET + " ], but got: [ " + Color::RED + stackSliceToString(args.size()) + Color::RESET + " ]", i);
                err.isNote = true;
                errors.push_back(err);
                return false;
            }
            
            size_t argAmount = args.size();
            std::string argTypes = "";
            std::string argGet = "";
            for (size_t argc = argAmount; argc; argc--) {
                argTypes += sclTypeToCType(result, args[argAmount - argc].type);
                argGet += "scale_positive_offset(" + std::to_string(argAmount - argc) + ", " + sclTypeToCType(result, args[argAmount - argc].type) + ")";
                argTypes += ", ";
                argGet += ", ";
                typePop;
            }
            argGet += "lambda";
            argTypes += "void*";

            append("%s(*(*lambda))(%s) = (typeof(lambda)) tmp->%s;\n", sclTypeToCType(result, returnType).c_str(), argTypes.c_str(), v.name.c_str());
            append("scale_popn(%zu);\n", argAmount);

            if (removeTypeModifiers(returnType) == "none" || removeTypeModifiers(returnType) == "nothing") {
                append("(*lambda)(%s);\n", argGet.c_str());
            } else {
                append("scale_push(%s, (*lambda)(%s));\n", sclTypeToCType(result, returnType).c_str(), argGet.c_str());
                typeStack.push_back(returnType);
            }
            scopeDepth--;
            append("}\n");
            return true;
        };

        if (s == Struct::Null) {
            const Layout& l = getLayout(result, type);
            const Enum& e = getEnumByName(result, type);
            if (l.name.size() || e.name.size()) {
                safeInc();
                Method* f;
                if (l.name.size()) {
                    f = getMethodByName(result, body[i].value, l.name);
                    if (f == nullptr) {
                        std::string help;
                        if (!l.hasMember(body[i].value) || !callContainedLambda(l.getMember(body[i].value), l.name, help)) {
                            transpilerError("Unknown method '" + body[i].value + "' on type '" + removeTypeModifiers(type) + "'" + help, i);
                            errors.push_back(err);
                        }
                        return;
                    }
                } else {
                    f = getMethodByName(result, body[i].value, e.name);
                }
                if (f->has_private && function->member_type != f->member_type) {
                    transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                    errors.push_back(err);
                    return;
                }
                methodCall(f, fp, result, warns, errors, body, i, false, true, false);
                return;
            }
            if (getInterfaceByName(result, type) != nullptr) {
                handle(ColumnOnInterface);
                return;
            }
            if (isPrimitiveIntegerType(type)) {
                s = getStructByName(result, "int");
            } else {
                transpilerError("Cannot infer type of stack top for ':': expected valid Struct, but got '" + type + "'", i);
                errors.push_back(err);
                return;
            }
        }
        bool onSuper = function->isMethod && body[i - 1].type == tok_identifier && body[i - 1].value == "super";
        safeInc();
        if (!s.isStatic() && !hasMethod(result, body[i].value, removeTypeModifiers(type))) {
            std::string help;
            if (!s.hasMember(body[i].value) || !callContainedLambda(s.getMember(body[i].value), s.name, help)) {
                transpilerError("Unknown method '" + body[i].value + "' on type '" + removeTypeModifiers(type) + "'" + help, i);
                errors.push_back(err);
            }
            return;
        } else if (s.isStatic() && s.name != "any" && s.name != "int" && !hasFunction(result, removeTypeModifiers(type) + "$" + body[i].value)) {
            transpilerError("Unknown static function '" + body[i].value + "' on type '" + removeTypeModifiers(type) + "'", i);
            errors.push_back(err);
            return;
        }
        Method* objMethod = nullptr;
        if (s.name == "any") {
            objMethod = !Main::options::noScaleFramework ? getMethodByName(result, body[i].value, "SclObject") : nullptr;
            if (objMethod) {
                append("if (scale_is_instance(scale_top(scale_any))) {\n");
                scopeDepth++;
                methodCall(objMethod, fp, result, warns, errors, body, i, true, false);
                if (objMethod->return_type != "none" && objMethod->return_type != "nothing") {
                    typeStack.pop_back();
                }
                scopeDepth--;
                append("} else {\n");
                scopeDepth++;
            }
        }
        if (s.isStatic()) {
            Function* f = getFunctionByName(result, removeTypeModifiers(type) + "$" + body[i].value);
            if (!f) {
                transpilerError("Unknown static function '" + body[i].value + "' on type '" + removeTypeModifiers(type) + "'", i);
                errors.push_back(err);
                return;
            }
            functionCall(f, fp, result, warns, errors, body, i);
            if (s.name == "any") {
                if (objMethod) {
                    scopeDepth--;
                    append("}\n");
                }
            }
        } else {
            if (typeCanBeNil(type)) {
                {
                    transpilerError("Calling method on maybe-nil type '" + type + "'", i);
                    errors.push_back(err);
                }
                transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this ':'", i - 1);
                err.isNote = true;
                errors.push_back(err);
                return;
            }
            Method* f = getMethodByName(result, body[i].value, s.name);
            if (f->has_private && function->member_type != f->member_type) {
                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                errors.push_back(err);
                return;
            }
            if (!f->has_const && isConst) {
                transpilerError("Cannot call non-const qualified method '" + body[i].value + "' on const instance", i);
                errors.push_back(err);
                return;
            }
            methodCall(f, fp, result, warns, errors, body, i, false, true, false, onSuper);
        }
    }
} // namespace sclc

