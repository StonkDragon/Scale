
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Store) {
        static bool doCheckTypes = true;
        noUnused;
        safeInc();

        if (body[i].type == tok_as) {
            handle(As);
            safeInc();
        }

        if (body[i].type == tok_paren_open) {
            Method* m = nullptr;
            std::string type = typeStackTop;
            if (type.size() < 2 || type.front() != '[') {
                m = getMethodByName(result, "[]", typeStackTop);
                if (!m) {
                    transpilerError("Type '" + typeStackTop + "' is not indexable", i);
                    errors.push_back(err);
                    return;
                }
                auto overloads = m->overloads.begin();
                auto end = m->overloads.end();
            nextCheck:
                if (!isPrimitiveIntegerType(m->args[0].type)) {
                    if (overloads == end) {
                        transpilerError("Index must be an integer, but got '" + m->args[0].type + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    m = (Method*) *(overloads++);
                    goto nextCheck;
                }
            }
            std::vector<std::vector<Token>> targets;
            std::vector<Token> current;
            current.push_back(Token(tok_store, "=>"));
            int parenDepth = 0;
            safeInc();
            while (body[i].type != tok_paren_close || parenDepth != 0) {
                if (body[i].type == tok_paren_open) {
                    parenDepth++;
                } else if (body[i].type == tok_paren_close) {
                    parenDepth--;
                }
                if (body[i].type == tok_comma && parenDepth == 0) {
                    targets.push_back(current);
                    current.clear();
                    current.push_back(Token(tok_store, "=>"));
                    safeInc();
                    continue;
                }
                current.push_back(body[i]);
                safeInc();
                if (parenDepth == 0 && body[i].type == tok_paren_close) {
                    targets.push_back(current);
                    break;
                }
            }

            append("{\n");
            scopeDepth++;
            typePop;
            append("%s tmp = _scl_pop(%s);\n", sclTypeToCType(result, type).c_str(), sclTypeToCType(result, type).c_str());
            if (m) {
                std::string return_type = sclTypeToCType(result, m->return_type);

                for (int i = targets.size() - 1; i >= 0; i--) {
                    append("_scl_push(%s, mt_%s$%s(tmp, %d));\n", return_type.c_str(), type.c_str(), m->name.c_str(), i);
                    typeStack.push_back(m->return_type);
                }
            } else {
                std::string return_type = sclTypeToCType(result, type.substr(1, type.size() - 2));
                append("scl_int size = _scl_array_size(tmp);\n");
                append("_scl_assert_fast(size >= %zu, \"Array too small for destructuring\");\n", targets.size());
                for (int i = targets.size() - 1; i >= 0; i--) {
                    append("_scl_push(%s, tmp[%d]);\n", return_type.c_str(), i);
                    typeStack.push_back(type.substr(1, type.size() - 2));
                }
            }

            scopeDepth--;
            append("}\n");

            size_t iSave = i;

            bool checkTypesSafe = doCheckTypes;
            doCheckTypes = false;
            for (auto body : targets) {
                i = 0;
                handle(Store);
            }
            doCheckTypes = checkTypesSafe;

            i = iSave;
        } else if (body[i].type == tok_declare) {
            safeInc();
            if (body[i].type != tok_identifier) {
                transpilerError("'" + body[i].value + "' is not an identifier", i+1);
                errors.push_back(err);
                return;
            }
            checkShadow(body[i].value, body[i], function, result, warns);
            std::string name = body[i].value;
            std::string type = "any";
            if (i + 1 < body.size() && body[i + 1].type == tok_column) {
                safeInc();
                safeInc();
                FPResult r = parseType(body, i);
                if (!r.success) {
                    errors.push_back(r);
                    return;
                }
                type = r.value;
                if (type == "lambda" && strstarts(typeStackTop, "lambda(")) {
                    type = typeStackTop;
                }
            } else {
                type = typeStackTop;
            }
            Variable v(name, type);
            vars.push_back(v);
            
            std::vector<Function*> funcs;
            for (auto&& f : result.functions) {
                if (f->name_without_overload == v.type + "$operator$store" || f->name_without_overload == v.type + "$=>") {
                    funcs.push_back(f);
                }
            }

            for (Function* f : funcs) {
                if (
                    f->isMethod ||
                    f->args.size() != 1 ||
                    f->return_type != v.type ||
                    !typesCompatible(result, typeStackTop, f->args[0].type, true)
                ) {
                    continue;
                }
                functionCall(f, fp, result, warns, errors, body, i);
                break;
            }
            #define TYPEALIAS_CAN_BE_NIL(result, ta) (hasTypealias(result, ta) && typealiasCanBeNil(result, ta))
            if (!v.canBeNil && !TYPEALIAS_CAN_BE_NIL(result, v.type)) {
                if (v.type.front() == '@' && typeStackTop.front() != '@') {
                    append("_scl_assert_fast(_scl_top(scl_int), \"Tried dereferencing nil pointer!\");\n");
                } else {
                    append("_scl_assert_fast(_scl_top(scl_int), \"Nil cannot be stored in non-nil variable '%s'!\");\n", v.name.c_str());
                }
            }
            if (doCheckTypes && !typesCompatible(result, typeStackTop, v.type, true)) {
                transpilerError("Incompatible types: '" + v.type + "' and '" + typeStackTop + "'", i);
                errors.push_back(err);
            }
            std::string ctype = sclTypeToCType(result, v.type);
            append("%s Var_%s;\n", ctype.c_str(), v.name.c_str());
            append("_scl_putlocal(Var_%s, ", v.name.c_str());
            if (v.type.front() == '@' && typeStackTop.front() != '@') {
                append2("*_scl_pop(%s*)", ctype.c_str());
            } else {
                append2("_scl_pop(%s)", ctype.c_str());
            }
            append2(");\n");
            typePop;
        } else {
            if (body[i].type != tok_identifier && body[i].type != tok_addr_of) {
                transpilerError("'" + body[i].value + "' is not an identifier", i);
                errors.push_back(err);
            }
            Variable v("", "");
            std::string containerBegin = "";
            std::string lastType = "";

            std::string variablePrefix = "";

            bool topLevelDeref = false;

            if (body[i].type == tok_addr_of) {
                safeInc();
                topLevelDeref = true;
                if (body[i].type == tok_paren_open) {
                    if (isInUnsafe) {
                        safeInc();
                        append("{\n");
                        scopeDepth++;
                        std::string type = sclTypeToCType(result, typeStackTop);
                        typePop;
                        append("%s _scl_value_to_store = _scl_pop(%s);\n", type.c_str(), type.c_str());
                        append("{\n");
                        scopeDepth++;
                        while (body[i].type != tok_paren_close) {
                            handle(Token);
                            safeInc();
                        }
                        scopeDepth--;
                        append("}\n");
                        type = sclTypeToCType(result, typeStackTop);
                        typePop;
                        append("*_scl_pop(%s*) = _scl_value_to_store;\n", type.c_str());
                        scopeDepth--;
                        append("}\n");
                    } else {
                        transpilerError("Storing at calculated pointer offset is only allowed in 'unsafe' blocks or functions", i - 1);
                        errors.push_back(err);
                    }
                    return;
                }
            }

            if (hasVar(body[i].value)) {
            normalVar:
                v = getVar(body[i].value);
            } else if (hasVar(function->member_type + "$" + body[i].value)) {
                v = getVar(function->member_type + "$" + body[i].value);
            } else if (body[i + 1].type == tok_double_column) {
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
                safeInc(); // i -> ::
                safeInc(); // i -> member
                if (s != Struct::Null) {
                    Struct s2 = getStructByName(result, s.name + "$" + body[i].value);
                    if (s2 != Struct::Null) {
                        s = s2;
                        goto nestedStruct;
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
                const Struct& s = getStructByName(result, m->member_type);
                if (s == Struct::Null) {
                    const Layout& l = getLayout(result, m->member_type);
                    if (l.hasMember(body[i].value)) {
                        Token here = body[i];
                        body.insert(body.begin() + i, Token(tok_dot, ".", here.location));
                        body.insert(body.begin() + i, Token(tok_identifier, "self", here.location));
                        goto normalVar;
                    } else {
                        transpilerError("Layout '" + l.name + "' has no member named '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                } else {
                    if (s.hasMember(body[i].value)) {
                        Token here = body[i];
                        body.insert(body.begin() + i, Token(tok_dot, ".", here.location));
                        body.insert(body.begin() + i, Token(tok_identifier, "self", here.location));
                        goto normalVar;
                    } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                        v = getVar(s.name + "$" + body[i].value);
                    } else {
                        transpilerError("Struct '" + s.name + "' has no member named '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                }
            } else {
                transpilerError("Unknown variable: '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }

            append("{\n");
            scopeDepth++;
            makePath(result, v, topLevelDeref, body, i, errors, true, function, warns, fp, [&](auto path, auto currentType) {
                if (doCheckTypes && !typesCompatible(result, typeStackTop, currentType, true)) {
                    transpilerError("Incompatible types: '" + currentType + "' and '" + typeStackTop + "'", i);
                    errors.push_back(err);
                }
                if (!typeCanBeNil(currentType) && !TYPEALIAS_CAN_BE_NIL(result, currentType)) {
                    append("_scl_assert_fast(_scl_top(scl_int), \"Nil cannot be stored in non-nil variable '%s'!\");\n", v.name.c_str());
                }
                append("%s tmp = _scl_pop(%s);\n", sclTypeToCType(result, typeStackTop).c_str(), sclTypeToCType(result, typeStackTop).c_str());
                append("%s;\n", path.c_str());
                typePop;
            });
            scopeDepth--;
            append("}\n");
        }
    }
} // namespace sclc

