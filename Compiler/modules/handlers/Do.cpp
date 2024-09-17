
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    static std::vector<std::string> modes{
        "f",
        "m",
        "r",
        "filter",
        "map",
        "reduce",
        "count",
        "each",
        "also",
        "transform",
    };

    handler(Do) {
        noUnused;
    nextDoMode:
        safeInc();
        if (body[i].type != tok_ticked) {
            transpilerError("Expected a ticked identifier after 'do'", i);
            errors.push_back(err);
            return;
        }
        std::string mode = body[i].value;
        if (std::find(modes.begin(), modes.end(), mode) == modes.end()) {
            transpilerError("Unknown do-mode: '" + mode + "'", i);
            errors.push_back(err);
            return;
        }
        safeInc();

        std::string lambdaType;

        append("{\n");
        scopeDepth++;

        if (body[i].type == tok_identifier) {
            if (hasFunction(result, body[i].value)) {
                i--;
                handle(AddrRef);
                lambdaType = typeStackTop;
                typePop;

                append("%s executor = _scl_pop(%s);\n", sclTypeToCType(result, lambdaType).c_str(), sclTypeToCType(result, lambdaType).c_str());
            } else {
                std::string variablePrefix = "";
                Variable v = Variable::emptyVar();

                bool isMember = false;

                if (hasVar(body[i].value)) {
                    v = getVar(body[i].value);
                } else if (hasVar(function->member_type + "$" + body[i].value)) {
                    v = getVar(function->member_type + "$" + body[i].value);
                } else if (body[i + 1].type == tok_double_column) {
                    Struct s = getStructByName(result, body[i].value);
                    safeInc(); // i -> ::
                    safeInc(); // i -> member
                    if (s != Struct::Null) {
                        if (hasFunction(result, s.name + "$" + body[i].value) || hasMethod(result, body[i].value, s.name)) {
                            i -= 3;
                            handle(AddrRef);
                            lambdaType = typeStackTop;
                            typePop;

                            append("%s executor = _scl_pop(%s);\n", sclTypeToCType(result, lambdaType).c_str(), sclTypeToCType(result, lambdaType).c_str());
                            goto done;
                        }
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
                        v = s.getMember(body[i].value);
                        isMember = true;
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

                makePath(result, v, false, body, i, errors, false, function, warns, fp, [&](auto path, auto lambdaType) {
                    if (errors.size()) {
                        return;
                    }

                    #define isLambda(_x) ((_x) == "lambda" || strstarts((_x), "lambda("))

                    if (!isLambda(lambdaType)) {
                        transpilerError("Expected lambda, but got '" + lambdaType + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    
                    append("%s executor = %s%s;\n", sclTypeToCType(result, lambdaType).c_str(), !isMember ? "" : "Var_self->", path.c_str());
                });
            }
        } else if (body[i].type == tok_lambda) {
            handle(Lambda);
            lambdaType = typeStackTop;
            typePop;

            std::string returnType = lambdaReturnType(lambdaType);
            std::string op = body[i].value;

            size_t args = lambdaArgCount(lambdaType);
            
            std::string argTypes = "";
            for (size_t argc = args; argc; argc--) {
                argTypes += "scl_any";
                if (argc > 1) {
                    argTypes += ", ";
                }
            }

            static int typedefCount = 0;

            std::string typedefName = "do$l_" + std::to_string(typedefCount++);
            std::string typeDef = "typedef " + sclTypeToCType(result, returnType) + "(*" + typedefName + ")(" + argTypes + ")";
            append("%s;\n", typeDef.c_str());

            append("%s executor = _scl_pop(%s);\n", typedefName.c_str(), typedefName.c_str());
        } else {
            transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
    done:

        std::string arrayType = removeTypeModifiers(typeStackTop);
        typePop;

        append("%s array = _scl_pop(%s);\n", sclTypeToCType(result, arrayType).c_str(), sclTypeToCType(result, arrayType).c_str());
        if (mode == "m") mode = "map";
        else if (mode == "f") mode = "filter";
        else if (mode == "r") mode = "reduce";

        Function* f = nullptr;
        if (arrayType.front() == '[' && arrayType.back() == ']') {
            f = getFunctionByName(result, "List$" + mode);
        } else {
            f = getFunctionByName(result, arrayType + "$" + mode);
        }

        if (f == nullptr) {
            transpilerError("No Function definition for '" + arrayType + "::" + mode + "' found", i);
            errors.push_back(err);
            return;
        }

        append("_scl_push(%s, array);\n", sclTypeToCType(result, arrayType).c_str());
        typeStack.push_back(arrayType);
        append("_scl_push(%s, executor);\n", sclTypeToCType(result, lambdaType).c_str());
        typeStack.push_back(lambdaType);

        functionCall(f, fp, result, warns, errors, body, i);
        
        scopeDepth--;
        append("}\n");
        if (i + 1 < body.size() && body[i + 1].type == tok_ticked) {
            // safeInc();
            goto nextDoMode;
        }
    }
} // namespace sclc

