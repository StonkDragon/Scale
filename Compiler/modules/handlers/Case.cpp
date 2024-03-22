#include <gc/gc_allocator.h>

#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

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
                size_t index = 2;
                if (!s.hasMember(body[i].value)) {
                    transpilerError("Unknown member '" + body[i].value + "' in union '" + s.name + "'", i);
                    errors.push_back(err);
                    return;
                }
                for (; index < s.members.size(); index++) {
                    if (s.members[index].name == body[i].value) {
                        break;
                    }
                }
                append("case %zu: {\n", index - 1);
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
                    safeInc();
                    FPResult res = parseType(body, &i, getTemplates(result, function));
                    if (!res.success) {
                        errors.push_back(res);
                        return;
                    }
                    const Struct& requested = getStructByName(result, res.value);
                    if (requested != Struct::Null && !requested.isStatic()) {
                        append("_scl_checked_cast(union_switch->__value, 0x%lxUL, \"%s\");\n", id(res.value.c_str()), res.value.c_str());
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

