#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Identifier) {
        noUnused;
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
        if (body[i].value == "swap") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            typeStack.push_back(a);
            typeStack.push_back(b);
            append("_scl_swap();\n");
        } else if (body[i].value == "dup") {
            std::string a = typeStackTop;
            typeStack.push_back(a);
            append("_scl_dup();\n");
        } else if (body[i].value == "drop") {
            typePop;
            append("_scl_drop();\n");
        } else if (body[i].value == "over") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(a);
            typeStack.push_back(b);
            typeStack.push_back(c);
            append("_scl_over();\n");
        } else if (body[i].value == "sdup2") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            typeStack.push_back(b);
            typeStack.push_back(a);
            typeStack.push_back(b);
            append("_scl_sdup2();\n");
        } else if (body[i].value == "swap2") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(b);
            typeStack.push_back(c);
            typeStack.push_back(a);
            append("_scl_swap2();\n");
        } else if (body[i].value == "rot") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(b);
            typeStack.push_back(a);
            typeStack.push_back(c);
            append("_scl_rot();\n");
        } else if (body[i].value == "unrot") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(a);
            typeStack.push_back(c);
            typeStack.push_back(b);
            append("_scl_unrot();\n");
        } else if (body[i].value == "?") {
            handle(ReturnOnNil);
        } else if (body[i].value == "exit") {
            append("exit(_scl_pop(scl_int));\n");
            typePop;
        } else if (body[i].value == "abort") {
            append("abort();\n");
        } else if (body[i].value == "using") {
            handle(Using);
        } else if (body[i].value == "await") {
            std::string type = typeStackTop;
            if (!strstarts(type, "async<")) {
                transpilerError("Expected async type, but got '" + type + "'", i);
                errors.push_back(err);
                return;
            }
            type = type.substr(6, type.size() - 7);
            std::string removed = type;
            typePop;
            if (removed != "none" && removed != "nothing") {
                typeStack.push_back(type);
                append("_scl_await(%s);\n", sclTypeToCType(result, type).c_str());
            } else {
                append("_scl_await_void();\n");
            }
        } else if (body[i].value == "typeof") {
            safeInc();
            auto templates = currentStruct.templates;
            if (hasVar(body[i].value)) {
                std::string type = getVar(body[i].value).type;
                const Struct& s = getStructByName(result, type);
                if (s != Struct::Null && !s.isStatic()) {
                    if (type.front() == '*') {
                        append("_scl_push(scl_str, _scl_create_string(Var_%s.$statics->type_name));\n", body[i].value.c_str());
                    } else {
                        append("_scl_push(scl_str, _scl_create_string(Var_%s->$statics->type_name));\n", body[i].value.c_str());
                    }
                } else {
                    append("_scl_push(scl_str, _scl_create_string(_scl_typename_or_else(*(scl_any*) &Var_%s, \"%s\")));\n", getVar(body[i].value).name.c_str(), retemplate(getVar(body[i].value).type).c_str());
                }
            } else if (hasFunction(result, body[i].value)) {
                Function* f = getFunctionByName(result, body[i].value);
                std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
                append("_scl_push(scl_str, _scl_create_string(\"%s\"));\n", lambdaType.c_str());
            } else if (body[i].type == tok_paren_open) {
                append("{\n");
                scopeDepth++;
                size_t stack_start = typeStack.size();
                handle(ParenOpen);
                if (typeStack.size() <= stack_start) {
                    transpilerError("Expected expression evaluating to a value after 'typeof'", i);
                    errors.push_back(err);
                }
                const Struct& s = getStructByName(result, typeStackTop);
                if (s != Struct::Null && !s.isStatic()) {
                    append("_scl_top(scl_str) = _scl_create_string(_scl_top(scl_SclObject)->$statics->type_name);\n");
                } else {
                    append("_scl_top(scl_str) = _scl_create_string(_scl_typename_or_else(_scl_top(scl_any), \"%s\"));\n", retemplate(typeStackTop).c_str());
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
                append("_scl_push(scl_str, _scl_create_string(\"%s\"));\n", retemplate(res.value).c_str());
            }
            typeStack.push_back("str");
        } else if (body[i].value == "nameof") {
            safeInc();
            if (hasVar(body[i].value)) {
                append("_scl_push(scl_str, _scl_create_string(\"%s\"));\n", body[i].value.c_str());
                typeStack.push_back("str");
            } else {
                transpilerError("Unknown Variable: '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (body[i].value == "sizeof") {
            safeInc();
            auto templates = getTemplates(result, function);
            if (body[i].value == "int") {
                append("_scl_push(scl_int, sizeof(scl_int));\n");
            } else if (body[i].value == "float") {
                append("_scl_push(scl_int, sizeof(scl_float));\n");
            } else if (body[i].value == "str") {
                append("_scl_push(scl_int, sizeof(scl_str));\n");
            } else if (body[i].value == "any") {
                append("_scl_push(scl_int, sizeof(scl_any));\n");
            } else if (body[i].value == "none" || body[i].value == "nothing") {
                append("_scl_push(scl_int, 0);\n");
            } else if (hasVar(body[i].value)) {
                append("_scl_push(scl_int, sizeof(%s));\n", sclTypeToCType(result, getVar(body[i].value).type).c_str());
            } else if (getStructByName(result, body[i].value) != Struct::Null) {
                append("_scl_push(scl_int, sizeof(struct Struct_%s));\n", body[i].value.c_str());
            } else if (hasTypealias(result, body[i].value)) {
                append("_scl_push(scl_int, sizeof(%s));\n", sclTypeToCType(result, body[i].value).c_str());
            } else if (hasLayout(result, body[i].value)) {
                append("_scl_push(scl_int, sizeof(struct Layout_%s));\n", body[i].value.c_str());
            } else if (templates.find(body[i].value) != templates.end()) {
                std::string replaceWith = templates[body[i].value];
                if (getStructByName(result, replaceWith) != Struct::Null)
                    append("_scl_push(scl_int, sizeof(struct Struct_%s));\n", replaceWith.c_str());
                else if (hasTypealias(result, replaceWith))
                    append("_scl_push(scl_int, sizeof(%s));\n", sclTypeToCType(result, replaceWith).c_str());
                else if (hasLayout(result, replaceWith))
                    append("_scl_push(scl_int, sizeof(struct Layout_%s));\n", replaceWith.c_str());
                else
                    append("_scl_push(scl_int, sizeof(%s));\n", sclTypeToCType(result, replaceWith).c_str());
            } else {
                FPResult type = parseType(body, &i, getTemplates(result, function));
                if (!type.success) {
                    transpilerError("Unknown Type: '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                append("_scl_push(scl_int, sizeof(%s));\n", sclTypeToCType(result, type.value).c_str());
            }
            typeStack.push_back("int");
        } else if (body[i].value == "try") {
            append("{\n");
            scopeDepth++;
            append("TRY {\n");
            scopeDepth++;
            varScopePush();
            pushTry();
        } else if (body[i].value == "catch") {
            handle(Catch);
        } else if (body[i].value == "lambda") {
            handle(Lambda);
        } else if (body[i].value == "unsafe") {
            isInUnsafe++;
            safeInc();
            while (body[i].type != tok_end) {
                handle(Token);
                safeInc();
            }
            isInUnsafe--;
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
                append("if (!(_scl_pop(scl_int))) {\n");
                typePop;
                handle(ParenOpen);
                append("}\n");
            } else {
                append("_scl_assert(_scl_pop(scl_int), \"Assertion at %s:%d:%d failed!\");\n", assertToken.location.file.c_str(), assertToken.location.line, assertToken.location.column);
                typePop;
            }
            varScopePop();
            scopeDepth--;
            append("}\n");
        } else if (hasVar(body[i].value)) {
        normalVar:
            Variable v = getVar(body[i].value);
            makePath(result, v, false, body, i, errors, false, function, warns, fp, [&](auto path, auto lastType) {
                LOAD_PATH(path, lastType);
            });
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
        } else if (s != Struct::Null) {
        nestedStruct:
            if (body[i + 1].type == tok_double_column) {
                safeInc();
                safeInc();
                Struct s2 = getStructByName(result, s.name + "$" + body[i].value);
                if (s2 != Struct::Null) {
                    s = s2;
                    goto nestedStruct;
                }

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
                    append("_scl_push(scl_any, ({\n");
                    scopeDepth++;
                    std::string ctype = sclTypeToCType(result, s.name);
                    append("%s tmp = ALLOC(%s);\n", ctype.c_str(), s.name.c_str());
                    
                    typeStack.push_back(s.name);
                    if (hasInitMethod) {
                        append("_scl_push(%s, tmp);\n", ctype.c_str());
                        methodCall(initMethod, fp, result, warns, errors, body, i);
                        typeStack.push_back(s.name);
                    }
                    append("tmp;\n");
                    scopeDepth--;
                    append("}));\n");
                } else if (body[i].value == "default" && !s.isStatic()) {
                    append("_scl_push(scl_any, ALLOC(%s));\n", s.name.c_str());
                    typeStack.push_back(s.name);
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
                        if (f->has_private && function->belongsToType(s.name)) {
                            if (f->member_type != function->member_type) {
                                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                                errors.push_back(err);
                                return;
                            }
                        }
                        functionCall(f, fp, result, warns, errors, body, i);
                    } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                        Variable v = getVar(s.name + "$" + body[i].value);
                        
                        makePath(result, v, false, body, i, errors, false, function, warns, fp, [&](std::string path, std::string lastType) {
                            LOAD_PATH(path, lastType);
                        });
                    } else {
                        transpilerError("Unknown static member of struct '" + s.name + "'", i);
                        errors.push_back(err);
                    }
                }
            } else if (body[i + 1].type == tok_curly_open) {
                safeInc();
                append("_scl_push(scl_any, ({\n");
                scopeDepth++;
                size_t begin = i - 1;
                append("scl_%s tmp = ALLOC(%s);\n", s.name.c_str(), s.name.c_str());
                append("scl_int stack_start = ls_ptr;\n");
                safeInc();
                size_t count = 0;
                varScopePush();
                if (function->isMethod) {
                    Variable v("super", getVar("self").type);
                    vars.push_back(v);
                    append("%s Var_super = Var_self;\n", sclTypeToCType(result, getVar("self").type).c_str());
                }
                append("scl_%s Var_self = tmp;\n", s.name.c_str());
                vars.push_back(Variable("self", "mut " + s.name));
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
                        append("mt_%s$%s(tmp, _scl_pop(%s));\n", mutator->member_type.c_str(), mutator->name.c_str(), sclTypeToCType(result, lastType).c_str());
                    } else {
                        append("tmp->%s = _scl_pop(%s);\n", body[i].value.c_str(), sclTypeToCType(result, lastType).c_str());
                    }
                    typePop;
                    scopeDepth--;
                    append("}\n");
                    append("ls_ptr = stack_start;\n");
                    safeInc();
                    count++;
                }
                varScopePop();
                append("tmp;\n");
                scopeDepth--;
                append("}));\n");
                typeStack.push_back(s.name);
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
        } else if (hasLayout(result, body[i].value)) {
            const Layout& l = getLayout(result, body[i].value);
            safeInc();
            if (body[i].type == tok_double_column) {
                safeInc();
                if (body[i].value != "new") {
                    transpilerError("Expected 'new' to create new layout, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                append("_scl_push(scl_any, _scl_alloc(sizeof(struct Layout_%s)));\n", l.name.c_str());
                typeStack.push_back(l.name);
            } else if (body[i].type == tok_curly_open) {
                append("_scl_push(scl_any, ({\n");
                scopeDepth++;
                size_t begin = i - 1;
                append("scl_%s tmp = _scl_alloc(sizeof(struct Layout_%s));\n", l.name.c_str(), l.name.c_str());
                append("scl_int stack_start = ls_ptr;\n");
                safeInc();
                size_t count = 0;
                varScopePush();
                if (function->isMethod) {
                    Variable v("super", getVar("self").type);
                    vars.push_back(v);
                    append("%s Var_super = Var_self;\n", sclTypeToCType(result, getVar("self").type).c_str());
                }
                append("scl_%s Var_self = tmp;\n", l.name.c_str());
                vars.push_back(Variable("self", "mut " + l.name));
                std::vector<std::string> missedMembers;

                size_t membersToInitialize = 0;
                for (auto v : l.members) {
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
                    const Variable& v = [&]() -> const Variable& {
                        for (auto&& v : l.members) {
                            if (v.name == body[i].value) {
                                return v;
                            }
                        }
                        return Variable::emptyVar();
                    }();

                    if (!typesCompatible(result, lastType, v.type, true)) {
                        transpilerError("Incompatible types: '" + v.type + "' and '" + lastType + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    append("tmp->%s = _scl_pop(%s);\n", body[i].value.c_str(), sclTypeToCType(result, lastType).c_str());
                    typePop;
                    scopeDepth--;
                    append("}\n");
                    append("ls_ptr = stack_start;\n");
                    safeInc();
                    count++;
                }
                varScopePop();
                append("tmp;\n");
                scopeDepth--;
                append("}));\n");
                typeStack.push_back(l.name);
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
            } else {
                transpilerError("Expected '{' or '::', but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
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
            append("_scl_push(scl_int, %zuUL);\n", e.indexOf(body[i].value));
            typeStack.push_back("int");
        } else if (hasVar(function->member_type + "$" + body[i].value)) {
            Variable v = getVar(function->member_type + "$" + body[i].value);
            makePath(result, v, false, body, i, errors, false, function, warns, fp, [&](auto path, auto lastType) {
                LOAD_PATH(path, lastType);
            });
        } else if (function->isMethod) {
            Struct s = getStructByName(result, function->member_type);
            Method* method = getMethodByName(result, body[i].value, s.name);
            Function* f;
            if (method != nullptr) {
                append("_scl_push(typeof(Var_self), Var_self);\n");
                typeStack.push_back(method->member_type);
                methodCall(method, fp, result, warns, errors, body, i);
            } else if ((f = getFunctionByName(result, s.name + "$" + body[i].value)) != nullptr) {
                if (f->isCVarArgs()) {
                    createVariadicCall(f, fp, result, errors, body, i);
                    return;
                }
                functionCall(f, fp, result, warns, errors, body, i);
            } else if (s.hasMember(body[i].value)) {
                Token here = body[i];
                body.insert(body.begin() + i, Token(tok_dot, ".", here.location));
                body.insert(body.begin() + i, Token(tok_identifier, "self", here.location));
                goto normalVar;
            } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                Variable v = getVar(s.name + "$" + body[i].value);
                makePath(result, v, false, body, i, errors, false, function, warns, fp, [&](auto path, auto lastType) {
                    LOAD_PATH(path, lastType);
                });
            } else {
                goto unknownIdent;
            }
        } else {
        unknownIdent:
            transpilerError("Unknown identifier: '" + body[i].value + "'", i);
            errors.push_back(err);
        }
    }
} // namespace sclc

