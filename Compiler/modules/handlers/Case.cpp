
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(Case) {
        noUnused;
        safeInc();
        if (!wasSwitch() && !wasSwitch2()) {
            transpilerError("Unexpected 'case' statement", i);
            errors.push_back(err);
            return;
        }
        if (body[i].type == tok_string_literal) {
            transpilerError("String literal in case statement is not supported", i);
            errors.push_back(err);
        } else {
            const Struct& s = getStructByName(result, switchTypes.back());
            if (s.super == "Union") {
                const Variable& v = s.getMember(body[i].value);
                if (v.name.empty()) {
                    transpilerError("Unknown member '" + body[i].value + "' in union '" + s.name + "'", i);
                    errors.push_back(err);
                    return;
                }
                size_t index = 0;
                for (; index < s.members.size(); index++) {
                    if (s.members[index].isVirtual && s.members[index].name == body[i].value) {
                        break;
                    }
                }
                append("case %zu: {\n", index / 2);
                scopeDepth++;
                varScopePush();
                if (i + 1 < body.size() && body[i + 1].type == tok_paren_open) {
                    safeInc();
                    safeInc();
                    if (body[i].type != tok_identifier) {
                        transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                    }
                    std::string name = body[i].value;
                    safeInc();
                    if (body[i].type != tok_column) {
                        transpilerError("Expected ':', but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                    }
                    size_t start = i;
                    safeInc();
                    FPResult res = parseType(body, i);
                    if (!res.success) {
                        errors.push_back(res);
                        return;
                    }
                    if (!typesCompatible(result, v.type, res.value, true) && removeTypeModifiers(v.type) != "any") {
                        transpilerError("Type '" + v.type + "' cannot be converted to '" + res.value + "'", start);
                        errors.push_back(err);
                    }
                    const Struct& requested = getStructByName(result, res.value);
                    if (requested != Struct::Null && !requested.isStatic()) {
                        append("scale_checked_cast(union_switch->__value, 0x%lxUL, \"%s\");\n", id(res.value.c_str()), res.value.c_str());
                    }
                    append("%s Var_%s = *(%s*) &(union_switch->__value);\n", sclTypeToCType(result, res.value).c_str(), name.c_str(), sclTypeToCType(result, res.value).c_str());
                    vars.push_back(Variable(name, res.value));
                    safeInc();
                    if (body[i].type != tok_paren_close) {
                        transpilerError("Expected ')', but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                    }
                } else {
                    std::string removed = removeTypeModifiers(s.members[index].type);
                    if (removed != "none" && removed != "nothing") {
                        append("%s Var_it = *(%s*) &union_switch->__value;\n", sclTypeToCType(result, s.members[index].type).c_str(), sclTypeToCType(result, s.members[index].type).c_str());
                        vars.push_back(Variable("it", s.members[index].type));
                    }
                }
                return;
            }
            if (hasEnum(result, body[i].value)) {
                Enum e = getEnumByName(result, body[i].value);
                safeInc();
                if (body[i].type != tok_double_column) {
                    transpilerError("Expected '::', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                safeInc();
                if (!e.hasMember(body[i].value)) {
                    transpilerError("Unknown member '" + body[i].value + "' in enum '" + e.name + "'", i);
                    errors.push_back(err);
                    return;
                }
                size_t index = e.indexOf(body[i].value);
                append("case %zu: {\n", index);
                scopeDepth++;
                varScopePush();
            } else {
                append("case %s: {\n", body[i].value.c_str());
                scopeDepth++;
                varScopePush();
            }
        }
    }
} // namespace sclc

