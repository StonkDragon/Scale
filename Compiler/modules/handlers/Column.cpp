
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
            std::string returnType = lambdaReturnType(typeStackTop);
            size_t argAmount = lambdaArgCount(typeStackTop);
            std::string op = body[i].value;
            
            std::string argTypes = "";
            std::string argGet = "";
            for (size_t argc = argAmount; argc; argc--) {
                argTypes += "scl_any";
                argGet += "_scl_positive_offset(" + std::to_string(argAmount - argc) + ", scl_any)";
                if (argc > 1) {
                    argTypes += ", ";
                    argGet += ", ";
                }
                typePop;
            }

            typePop;
            if (op == "accept") {
                static int typedefCount = 0;
                std::string typedefName = "td$l_" + std::to_string(typedefCount++);
                std::string typeDef = "typedef " + sclTypeToCType(result, returnType) + "(*" + typedefName + ")(" + argTypes + ")";
                std::string removed = removeTypeModifiers(returnType);
                append("%s;\n", typeDef.c_str());
                append("_scl_popn(%zu);\n", argAmount + 1);
                if (removed == "none" || removed == "nothing") {
                    append("");
                } else {
                    append("_scl_push(%s, ", sclTypeToCType(result, returnType).c_str());
                    typeStack.push_back(returnType);
                }
                append2("_scl_positive_offset(%zu, %s)(%s)", argAmount, typedefName.c_str(), argGet.c_str());
                if (removed != "none" && removed != "nothing") {
                    append(")");
                }
                append(";\n");
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
        if (s == Struct::Null) {
            Layout l = getLayout(result, type);
            Enum e = getEnumByName(result, type);
            if (l.name.size() || e.name.size()) {
                safeInc();
                Method* f;
                if (l.name.size()) {
                    f = getMethodByName(result, body[i].value, l.name);
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
            std::string help = "";
            if (s.hasMember(body[i].value)) {
                const Variable& v = s.getMember(body[i].value);
                std::string type = v.type;
                if (!strstarts(removeTypeModifiers(type), "lambda(")) {
                    help = ". Maybe you meant to use '.' instead of ':' here";
                    goto notLambdaType;
                }

                std::string returnType = lambdaReturnType(type);
                size_t argAmount = lambdaArgCount(type);


                append("{\n");
                scopeDepth++;

                append("%s tmp = _scl_pop(scl_any);\n", sclTypeToCType(result, s.name).c_str());
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
                    argGet += "_scl_positive_offset(" + std::to_string(argAmount - argc) + ", scl_any)";
                    if (argc > 1) {
                        argTypes += ", ";
                        argGet += ", ";
                    }
                    typePop;
                }
                if (removeTypeModifiers(returnType) == "none" || removeTypeModifiers(returnType) == "nothing") {
                    append("void(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.name.c_str());
                    append("_scl_popn(%zu);\n", argAmount);
                    append("lambda(%s);\n", argGet.c_str());
                } else if (removeTypeModifiers(returnType) == "float") {
                    append("scl_float(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.name.c_str());
                    append("_scl_popn(%zu);\n", argAmount);
                    append("_scl_push(scl_float, lambda(%s));\n", argGet.c_str());
                    typeStack.push_back(returnType);
                } else {
                    append("scl_any(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.name.c_str());
                    append("_scl_popn(%zu);\n", argAmount);
                    append("_scl_push(scl_any, lambda(%s));\n", argGet.c_str());
                    typeStack.push_back(returnType);
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
        Method* objMethod = nullptr;
        if (s.name == "any") {
            objMethod = !Main::options::noScaleFramework ? getMethodByName(result, body[i].value, "SclObject") : nullptr;
            if (objMethod) {
                append("if (_scl_is_instance(_scl_top(scl_any))) {\n");
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

