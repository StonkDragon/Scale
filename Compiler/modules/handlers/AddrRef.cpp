
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    Function* generateReifiedFunction(Function* self, std::ostream& fp, TPResult& result, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i, std::vector<std::string>& types);

    handler(AddrRef) {
        noUnused;
        safeInc();
        Token toGet = body[i];
        Function* f = nullptr;

        Variable v("", "");

        if (hasFunction(result, toGet.value)) {
            f = getFunctionByName(result, toGet.value);
        functionHandling:
            size_t begin = i;
            if (f->isCVarArgs()) {
                transpilerError("Cannot take reference of varargs function '" + f->name + "'", begin);
                errors.push_back(err);
                return;
            }
            if (f->has_reified && !(i + 1 < body.size() && body[i + 1].type == tok_double_column)) {
                {
                    transpilerError("Reified types need to be specified for function '" + sclFunctionNameToFriendlyString(f) + "'", i);
                    errors.push_back(err);
                }
                transpilerError("Add the types like this after the function name: '::<...>'", i);
                err.isNote = true;
                errors.push_back(err);
                return;
            }
            if (i + 1 < body.size() && body[i + 1].type == tok_double_column) {
                safeInc();
                safeInc();
                if (body[i].type != tok_identifier || body[i].value != "<") {
                    transpilerError("Expected '<' to specify argument types, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                safeInc();
                std::vector<std::string> argTypes;
                argTypes.reserve(f->args.size());
                while (i < body.size() && body[i].value != ">") {
                    FPResult type = parseType(body, i);
                    if (!type.success) {
                        errors.push_back(type);
                        return;
                    }
                    argTypes.push_back(removeTypeModifiers(type.value));
                    safeInc();
                    if (body[i].type != tok_comma && (body[i].type != tok_identifier || body[i].value != ">")) {
                        transpilerError("Expected ',' or '>', but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    if (body[i].type == tok_comma) {
                        safeInc();
                    }
                }

                auto argsEqual = [&](const std::vector<Variable>& args) {
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
                for (auto&& overload : f->overloads) {
                    if (overload->has_reified && !have_reified) {
                        have_reified = overload;
                        break;
                    }
                    if (argsEqual(overload->args)) {
                        f = overload;
                        found = true;
                        break;
                    }
                }
                if (have_reified) {
                    found = true;
                    f = generateReifiedFunction(have_reified, fp, result, errors, body, i, argTypes);
                }
                if (!found) {
                    transpilerError("No overload of '" + f->name + "' with arguments [ " + argVectorToString(argTypes) + " ] found", begin);
                    errors.push_back(err);
                    return;
                }
            }

            append("scale_push(scale_any, ({\n");
            append("  scale_any* tmp = scale_alloc(sizeof(scale_any));\n");
            append("  *tmp = %s;\n", f->outputName().c_str());
            append("  tmp;\n");
            append("}));\n");
            std::string lambdaType = "lambda(";
            for (size_t i = 0; i < f->args.size(); i++) {
                if (i) lambdaType += ",";
                lambdaType += f->args[i].type;
            }
            lambdaType += "):" + f->return_type;
            typeStack.push_back(lambdaType);
            return;
        }
        if (!hasVar(body[i].value)) {
            Struct s = getStructByName(result, body[i].value);
            if (s == Struct::Null) {
                if (usingStructs.find(body[i].location.file) != usingStructs.end()) {
                    for (auto& used : usingStructs[body[i].location.file]) {
                        const Struct& s2 = getStructByName(result, used + "$" + body[i].value);
                        if (s2 != Struct::Null) {
                            s = s2;
                            break;
                        }
                    }
                }
            }
        nestedStruct:
            if (body[i + 1].type == tok_double_column) {
                safeInc();
                safeInc();
                if (s != Struct::Null) {
                    Struct s2 = getStructByName(result, s.name + "$" + body[i].value);
                    if (s2 != Struct::Null) {
                        s = s2;
                        goto nestedStruct;
                    }
                    if (hasFunction(result, s.name + "$" + body[i].value)) {
                        f = getFunctionByName(result, s.name + "$" + body[i].value);
                        goto functionHandling;
                    } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                        std::string loadFrom = s.name + "$" + body[i].value;
                        v = getVar(loadFrom);
                    } else if (hasMethod(result, body[i].value, s.name)) {
                        Method* f = getMethodByName(result, body[i].value, s.name);
                        size_t begin = i;
                        if (i + 1 < body.size() && body[i + 1].type == tok_double_column) {
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
                            for (auto&& overload : f->overloads) {
                                if (argsEqual(overload->args) && overload->isMethod) {
                                    f = (Method*) overload;
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                transpilerError("No overload of '" + f->name + "' with arguments [ " + argVectorToString(argTypes) + " ] found", begin);
                                errors.push_back(err);
                                return;
                            }
                        }
                        if (!f->isMethod) {
                            transpilerError("'" + f->name + "' is static", i);
                            errors.push_back(err);
                            return;
                        }
                        if (f->isCVarArgs()) {
                            transpilerError("Cannot convert variadic function to lambda!", begin);
                            errors.push_back(err);
                            return;
                        }
                        append("scale_push(scale_any, ({\n");
                        append("  scale_any* tmp = scale_alloc(sizeof(scale_any));\n");
                        append("  *tmp = %s;\n", f->outputName().c_str());
                        append("  tmp;\n");
                        append("}));\n");
                        std::string lambdaType = "lambda(";
                        for (size_t i = 0; i < f->args.size(); i++) {
                            if (i) lambdaType += ",";
                            lambdaType += f->args[i].type;
                        }
                        lambdaType += "):" + f->return_type;
                        typeStack.push_back(lambdaType);
                        return;
                    } else {
                        transpilerError("Struct '" + s.name + "' has no static member named '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                }
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
                    size_t begin = i;
                    if (i + 1 < body.size() && body[i + 1].type == tok_double_column) {
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
                        for (auto&& overload : f->overloads) {
                            if (argsEqual(overload->args) && overload->isMethod) {
                                f = (Method*) overload;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            transpilerError("No overload of '" + f->name + "' with arguments [ " + argVectorToString(argTypes) + " ] found", begin);
                            errors.push_back(err);
                            return;
                        }
                    }
                    if (!f->isMethod) {
                        transpilerError("'" + f->name + "' is static", i);
                        errors.push_back(err);
                        return;
                    }

                    if (f->isCVarArgs()) {
                        transpilerError("Cannot convert variadic function to lambda!", begin);
                        errors.push_back(err);
                        return;
                    }
                    append("scale_push(scale_any, ({\n");
                    append("  scale_any* tmp = scale_alloc(sizeof(scale_any));\n");
                    append("  *tmp = %s;\n", f->outputName().c_str());
                    append("  tmp;\n");
                    append("}));\n");
                    std::string lambdaType = "lambda(";
                    for (size_t i = 0; i < f->args.size(); i++) {
                        if (i) lambdaType += ",";
                        lambdaType += f->args[i].type;
                    }
                    lambdaType += "):" + f->return_type;
                    typeStack.push_back(lambdaType);
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
        makePath(result, v, false, body, i, errors, false, function, warns, fp, [&](auto path, auto lastType) {
            append("scale_push(typeof(&(%s)), &(%s));\n", path.c_str(), path.c_str());
            typeStack.push_back("*" + lastType);
        });
    }
} // namespace sclc

