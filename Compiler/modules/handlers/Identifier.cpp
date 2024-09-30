
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {

    handler(Await) {
        noUnused;
        std::string type = typeStackTop;
        if (!strstarts(type, "async<")) {
            transpilerError("Expected async type, but got '" + type + "'", i);
            errors.push_back(err);
            return;
        }
        type = type.substr(6, type.size() - 7);
        std::string removed = removeTypeModifiers(type);
        typePop;
        if (removed != "none" && removed != "nothing") {
            if (type.front() == '@') type.erase(0, 1);
            typeStack.push_back(type);
            append("scale_await(%s);\n", sclTypeToCType(result, type).c_str());
        } else {
            append("scale_await_void();\n");
        }
    }
    handler(Typeof) {
        noUnused;
        safeInc();
        
        const Variable& var = getVar(body[i].value);
        if (var.name.size()) {
            std::string type = var.type;
            if (type.front() == '@') {
                append("scale_push(scale_str, scale_create_string(scale_typename_or_else((scale_any) &Var_%s, \"%s\")));\n", var.name.c_str(), retemplate(type.substr(1)).c_str());
            } else {
                append("scale_push(scale_str, scale_create_string(scale_typename_or_else(*(scale_any*) &Var_%s, \"%s\")));\n", var.name.c_str(), retemplate(type).c_str());
            }
        } else if (hasFunction(result, body[i].value)) {
            Function* f = getFunctionByName(result, body[i].value);
            std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
            append("scale_push(scale_str, scale_create_string(\"%s\"));\n", lambdaType.c_str());
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
            Enum e = getEnumByName(result, typeStackTop);
            if (s != Struct::Null && !s.isStatic()) {
                append("scale_top(scale_str) = scale_create_string(scale_top(scale_SclObject)->$type->type_name);\n");
            } else if (hasLayout(result, typeStackTop)) {
                append("scale_top(scale_str) = scale_create_string(\"%s\");\n", retemplate(typeStackTop).c_str());
            } else if (e.name.size()) {
                append("scale_top(scale_str) = scale_create_string(((char*[]){\n");
                scopeDepth++;
                for (auto&& x : e.member_types) {
                    append("[%ld] = \"%s\",\n", e.members[x.first], x.second.c_str());
                }
                scopeDepth--;
                append("})[scale_top(%s)]);\n", sclTypeToCType(result, typeStackTop).c_str());
            } else {
                append("scale_top(scale_str) = scale_create_string(scale_typename_or_else(scale_top(%s), \"%s\"));\n", sclTypeToCType(result, typeStackTop).c_str(), retemplate(typeStackTop).c_str());
            }
            scopeDepth--;
            append("}\n");
            typePop;
        } else {
            FPResult res = parseType(body, i);
            if (!res.success) {
                errors.push_back(res);
                return;
            }
            append("scale_push(scale_str, scale_create_string(\"%s\"));\n", retemplate(res.value).c_str());
        }
        typeStack.push_back("str");
    }
    handler(Typeid) {
        noUnused;
        safeInc();
        const Variable& var = getVar(body[i].value);
        if (!var.name.empty()) {
            std::string type = var.type;
            if (type.front() == '@') {
                append("scale_push(scale_uint, scale_typeid_or_else((scale_any) &Var_%s, 0x%016llxULL));\n", var.name.c_str(), id(retemplate(type).c_str()));
            } else {
                append("scale_push(scale_uint, scale_typeid_or_else(*(scale_any*) &Var_%s, 0x%016llxULL));\n", var.name.c_str(), id(retemplate(type).c_str()));
            }
        } else if (hasFunction(result, body[i].value)) {
            Function* f = getFunctionByName(result, body[i].value);
            std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
            append("scale_push(scale_int, 0x%016llx);\n", id(lambdaType.c_str()));
        } else if (body[i].type == tok_paren_open) {
            append("{\n");
            scopeDepth++;
            size_t stack_start = typeStack.size();
            handle(ParenOpen);
            if (typeStack.size() <= stack_start) {
                transpilerError("Expected expression evaluating to a value after 'typeid'", i);
                errors.push_back(err);
            }
            const Struct& s = getStructByName(result, typeStackTop);
            Enum e = getEnumByName(result, typeStackTop);
            if (s != Struct::Null && !s.isStatic()) {
                append("scale_top(scale_uint) = scale_top(scale_SclObject)->$type->type;\n");
            } else if (hasLayout(result, typeStackTop)) {
                append("scale_top(scale_uint) = 0x%016llxULL;\n", id(retemplate(typeStackTop).c_str()));
            } else if (e.name.size()) {
                append("scale_top(scale_uint) = ((scale_uint*[]){\n");
                scopeDepth++;
                for (auto&& x : e.member_types) {
                    append("[%ld] = 0x%026llxULL,\n", e.members[x.first], id(retemplate(x.second).c_str()));
                }
                scopeDepth--;
                append("})[scale_top(%s)];\n", sclTypeToCType(result, typeStackTop).c_str());
            } else {
                append("scale_top(scale_uint) = scale_typeid_or_else(scale_top(%s), 0x%016llxULL);\n", sclTypeToCType(result, typeStackTop).c_str(), id(retemplate(typeStackTop).c_str()));
            }
            scopeDepth--;
            append("}\n");
            typePop;
        } else {
            FPResult res = parseType(body, i);
            if (!res.success) {
                errors.push_back(res);
                return;
            }
            append("scale_push(scale_uint, 0x%016llxULL);\n", id(retemplate(res.value).c_str()));
        }
        typeStack.push_back("uint");
    }
    handler(Nameof) {
        noUnused;
        safeInc();
        if (hasVar(body[i].value)) {
            append("scale_push(scale_str, scale_create_string(\"%s\"));\n", body[i].value.c_str());
            typeStack.push_back("str");
        } else {
            transpilerError("Unknown Variable: '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
    }
    handler(Sizeof) {
        noUnused;
        safeInc();
        if (hasVar(body[i].value)) {
            append("scale_push(scale_int, sizeof(%s));\n", sclTypeToCType(result, getVar(body[i].value).type).c_str());
        } else {
            FPResult type = parseType(body, i);
            if (!type.success) {
                transpilerError("Unknown Type: '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            const Struct& s = getStructByName(result, type.value);
            if (s != Struct::Null && !s.isStatic()) {
                append("scale_push(scale_int, sizeof(struct Struct_%s));\n", type.value.c_str());
            } else if (hasLayout(result, type.value)) {
                append("scale_push(scale_int, sizeof(struct Layout_%s));\n", type.value.c_str());
            } else if (type.value == "none") {
                append("scale_push(scale_int, 0);\n");
            } else {
                append("scale_push(scale_int, sizeof(%s));\n", sclTypeToCType(result, type.value).c_str());
            }
        }
        typeStack.push_back("int");
    }
    handler(Try) {
        noUnused;
        append("{\n");
        scopeDepth++;
        append("TRY {\n");
        scopeDepth++;
        varScopePush();
        pushTry();
    }
    handler(Unsafe) {
        noUnused;
        isInUnsafe++;
        safeInc();
        while (body[i].type != tok_end) {
            handle(Token);
            safeInc();
        }
        isInUnsafe--;
    }
    handler(Assert) {
        noUnused;
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
            append("if (!(scale_pop(scale_int))) {\n");
            typePop;
            handle(ParenOpen);
            append("}\n");
        } else {
            append("scale_assert_fast(scale_pop(scale_int), \"Assertion at %s:%d:%d failed!\");\n", assertToken.location.file.c_str(), assertToken.location.line, assertToken.location.column);
            typePop;
        }
        varScopePop();
        scopeDepth--;
        append("}\n");
    }
    handler(Varargs) {
        noUnused;
        typeStack.push_back("<varargs>");
    }

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
        if (hasVar(body[i].value)) {
        normalVar:
            Variable v = getVar(body[i].value);
            makePath(result, v, false, body, i, errors, false, function, warns, fp, [&](auto path, auto lastType) {
                LOAD_PATH(path, lastType);
            });
        } else if (hasFunction(result, body[i].value)) {
            Function* f = getFunctionByName(result, body[i].value);
            if (f->isMethod) {
                transpilerError("'" + f->name + "' is a method, not a function.", i);
                errors.push_back(err);
                return;
            }
            functionCall(f, fp, result, warns, errors, body, i);
        } else if (hasFunction(result, function->member_type + "$" + body[i].value)) {
            Function* f = getFunctionByName(result, function->member_type + "$" + body[i].value);
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
                    std::string ctype = sclTypeToCType(result, s.name);
                    append("scale_push(%s, ({\n", ctype.c_str());
                    scopeDepth++;
                    append("%s tmp = ALLOC(%s);\n", ctype.c_str(), s.name.c_str());
                    
                    typeStack.push_back(s.name);
                    if (hasInitMethod) {
                        append("scale_push(%s, tmp);\n", ctype.c_str());
                        methodCall(initMethod, fp, result, warns, errors, body, i);
                        typeStack.push_back(s.name);
                    }
                    append("tmp;\n");
                    scopeDepth--;
                    append("}));\n");
                } else if (body[i].value == "local" && !s.isStatic()) {
                    std::string ctype = sclTypeToCType(result, s.name);
                    append("scale_push(%s, ({\n", ctype.c_str());
                    scopeDepth++;
                    append("%s tmp = scale_stack_alloc(%s);\n", ctype.c_str(), s.name.c_str());
                    
                    typeStack.push_back(s.name);
                    if (hasInitMethod) {
                        append("scale_push(%s, tmp);\n", ctype.c_str());
                        methodCall(initMethod, fp, result, warns, errors, body, i);
                        typeStack.push_back(s.name);
                    }
                    append("tmp;\n");
                    scopeDepth--;
                    append("}));\n");
                } else if (body[i].value == "default" && !s.isStatic()) {
                    append("scale_push(%s, ALLOC(%s));\n", sclTypeToCType(result, s.name).c_str(), s.name.c_str());
                    typeStack.push_back(s.name);
                } else {
                    if (hasFunction(result, s.name + "$" + body[i].value)) {
                        Function* f = getFunctionByName(result, s.name + "$" + body[i].value);
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
            } else if (body[i + 1].type == tok_curly_open || (body[i + 1].type == tok_identifier && (body[i + 1].value == "static" || body[i + 1].value == "local"))) {
                safeInc();
                append("scale_push(%s, ({\n", sclTypeToCType(result, s.name).c_str());
                scopeDepth++;
                if (body[i].type == tok_curly_open) {
                    append("%s tmp = ALLOC(%s);\n", sclTypeToCType(result, s.name).c_str(), s.name.c_str());
                } else if (body[i].value == "static") {
                    safeInc();
                    append("%s tmp = scale_uninitialized_constant(%s);\n", sclTypeToCType(result, s.name).c_str(), s.name.c_str());
                } else if (body[i].value == "local") {
                    safeInc();
                    append("%s tmp = scale_stack_alloc(%s);\n", sclTypeToCType(result, s.name).c_str(), s.name.c_str());
                }
                size_t begin = i;
                append("scale_uint64* stack_start = _local_stack_ptr;\n");
                safeInc();
                size_t count = 0;
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
                        append("mt_%s$%s(tmp, scale_pop(%s));\n", mutator->member_type.c_str(), mutator->name.c_str(), sclTypeToCType(result, lastType).c_str());
                    } else if (v.type.front() == '@' && lastType.front() != '@') {
                        append("tmp->%s = *scale_pop(%s);\n", v.name.c_str(), sclTypeToCType(result, lastType).c_str());
                    } else {
                        append("tmp->%s = scale_pop(%s);\n", v.name.c_str(), sclTypeToCType(result, lastType).c_str());
                    }
                    typePop;
                    scopeDepth--;
                    append("}\n");
                    append("_local_stack_ptr = stack_start;\n");
                    safeInc();
                    count++;
                }
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
                Method* initMethod = getMethodByName(result, "init", l.name);
                bool hasInitMethod = initMethod != nullptr;
                std::string ctype = sclTypeToCType(result, l.name);
                if (body[i].value == "new") {
                    append("scale_push(scale_any, ({\n");
                    scopeDepth++;
                    append("%s tmp = scale_alloc(sizeof(struct Layout_%s));\n", ctype.c_str(), l.name.c_str());

                    typeStack.push_back(l.name);
                    if (hasInitMethod) {
                        append("scale_push(%s, tmp);\n", ctype.c_str());
                        methodCall(initMethod, fp, result, warns, errors, body, i);
                        typeStack.push_back(l.name);
                    }
                    
                    append("tmp;\n");
                    scopeDepth--;
                    append("}));\n");
                } else if (body[i].value == "local") {
                    append("scale_push(scale_any, ({\n");
                    scopeDepth++;
                    append("%s tmp = scale_stack_alloc_ctype(struct Layout_%s);\n", ctype.c_str(), l.name.c_str());

                    typeStack.push_back(l.name);
                    if (hasInitMethod) {
                        append("scale_push(%s, tmp);\n", ctype.c_str());
                        methodCall(initMethod, fp, result, warns, errors, body, i);
                        typeStack.push_back(l.name);
                    }
                    
                    append("tmp;\n");
                    scopeDepth--;
                    append("}));\n");
                } else if (hasFunction(result, l.name + "$" + body[i].value)) {
                    Function* f = getFunctionByName(result, l.name + "$" + body[i].value);
                    if (f->isMethod) {
                        transpilerError("'" + f->name + "' is not static", i);
                        errors.push_back(err);
                        return;
                    }
                    if (f->has_private && function->belongsToType(l.name)) {
                        if (f->member_type != function->member_type) {
                            transpilerError("'" + body[i].value + "' has private access in Layout '" + l.name + "'", i);
                            errors.push_back(err);
                            return;
                        }
                    }
                    functionCall(f, fp, result, warns, errors, body, i);
                } else {
                    transpilerError("Unknown static member of layout '" + l.name + "'", i);
                    errors.push_back(err);
                }
            } else if (body[i].type == tok_curly_open || (body[i].type == tok_identifier && (body[i].value == "static" || body[i].value == "local"))) {
                append("scale_push(%s, ({\n", sclTypeToCType(result, l.name).c_str());
                scopeDepth++;
                size_t begin = i - 1;
                if (body[i].type == tok_curly_open) {
                    append("%s tmp = scale_alloc(sizeof(struct Layout_%s));\n", sclTypeToCType(result, l.name).c_str(), l.name.c_str());
                } else if (body[i].value == "static") {
                    safeInc();
                    append("%s tmp = scale_uninitialized_constant_ctype(struct Layout_%s);\n", sclTypeToCType(result, l.name).c_str(), l.name.c_str());
                } else if (body[i].value == "local") {
                    safeInc();
                    append("%s tmp = scale_stack_alloc_ctype(struct Layout_%s);\n", sclTypeToCType(result, l.name).c_str(), l.name.c_str());
                }
                append("scale_uint64* stack_start = _local_stack_ptr;\n");
                safeInc();
                size_t count = 0;
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
                    if (v.type.front() == '@' && lastType.front() != '@') {
                        append("tmp->%s = *scale_pop(%s);\n", body[i].value.c_str(), sclTypeToCType(result, lastType).c_str());
                    } else {
                        append("tmp->%s = scale_pop(%s);\n", body[i].value.c_str(), sclTypeToCType(result, lastType).c_str());
                    }
                    typePop;
                    scopeDepth--;
                    append("}\n");
                    append("_local_stack_ptr = stack_start;\n");
                    safeInc();
                    count++;
                }
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
            append("scale_push(scale_int, %zuUL);\n", e.indexOf(body[i].value));
            typeStack.push_back(e.name);
        } else if (hasVar(function->member_type + "$" + body[i].value)) {
            Variable v = getVar(function->member_type + "$" + body[i].value);
            makePath(result, v, false, body, i, errors, false, function, warns, fp, [&](auto path, auto lastType) {
                LOAD_PATH(path, lastType);
            });
        } else if (function->isMethod) {
            Struct s = getStructByName(result, function->member_type);
            if (s == Struct::Null && hasLayout(result, function->member_type)) {
                const Layout& l = getLayout(result, function->member_type);
                Method* method = getMethodByName(result, body[i].value, l.name);
                Function* f;
                if (method != nullptr) {
                    append("scale_push(typeof(Var_self), Var_self);\n");
                    typeStack.push_back(method->member_type);
                    methodCall(method, fp, result, warns, errors, body, i);
                } else if ((f = getFunctionByName(result, l.name + "$" + body[i].value)) != nullptr) {
                    functionCall(f, fp, result, warns, errors, body, i);
                } else if (l.hasMember(body[i].value)) {
                    Token here = body[i];
                    body.insert(body.begin() + i, Token(tok_dot, ".", here.location));
                    body.insert(body.begin() + i, Token(tok_identifier, "self", here.location));
                    goto normalVar;
                } else if (hasGlobal(result, l.name + "$" + body[i].value)) {
                    Variable v = getVar(l.name + "$" + body[i].value);
                    makePath(result, v, false, body, i, errors, false, function, warns, fp, [&](auto path, auto lastType) {
                        LOAD_PATH(path, lastType);
                    });
                } else {
                    goto unknownIdent;
                }
            } else {
                Method* method = getMethodByName(result, body[i].value, s.name);
                Function* f;
                if (method != nullptr) {
                    append("scale_push(typeof(Var_self), Var_self);\n");
                    typeStack.push_back(method->member_type);
                    methodCall(method, fp, result, warns, errors, body, i);
                } else if ((f = getFunctionByName(result, s.name + "$" + body[i].value)) != nullptr) {
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
            }
        } else {
        unknownIdent:
            transpilerError("Unknown identifier: '" + body[i].value + "'", i);
            errors.push_back(err);
        }
    }
} // namespace sclc

