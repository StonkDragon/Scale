#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
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
                    FPResult type = parseType(body, &i, getTemplates(result, f));
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
                for (auto&& overload : f->overloads) {
                    if (argsEqual(overload->args)) {
                        f = overload;
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

            if (f->isCVarArgs()) {
                transpilerError("Cannot take reference of varargs function '" + f->name + "'", begin);
                errors.push_back(err);
                return;
            }

            append("_scl_push(typeof(&fn_%s), &fn_%s);\n", f->name.c_str(), f->name.c_str());
            std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
            typeStack.push(lambdaType);
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
                                FPResult type = parseType(body, &i, getTemplates(result, f));
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
                        append("_scl_push(typeof(&mt_%s$%s), &mt_%s$%s);\n", s.name.c_str(), f->name.c_str(), s.name.c_str(), f->name.c_str());
                        std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
                        typeStack.push(lambdaType);
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
                            FPResult type = parseType(body, &i, getTemplates(result, f));
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
                    append("_scl_push(typeof(&mt_%s$%s), &mt_%s$%s);\n", s.name.c_str(), f->name.c_str(), s.name.c_str(), f->name.c_str());
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
        makePath(result, v, false, body, i, errors, false, function, warns, fp, [&](auto path, auto lastType) {
            append("_scl_push(typeof(&(%s)), &(%s));\n", path.c_str(), path.c_str());
            typeStack.push("[" + lastType + "]");
        });
    }
} // namespace sclc

