#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Declare) {
        noUnused;
        safeInc();
        if (body[i].type != tok_identifier) {
            transpilerError("'" + body[i].value + "' is not an identifier", i+1);
            errors.push_back(err);
            return;
        }
        std::string name = body[i].value;
        checkShadow(name, body, i, function, result, warns);
        size_t start = i;
        std::string type = "any";
        safeInc();
        if (body[i].type == tok_column) {
            safeInc();
            FPResult r = parseType(body, &i, getTemplates(result, function));
            if (!r.success) {
                errors.push_back(r);
                return;
            }
            type = r.value;
        } else {
            transpilerError("A type is required", i);
            errors.push_back(err);
            return;
        }
        Variable v(name, type);
        vars.push_back(v);
        const Struct& s = getStructByName(result, type);
        Method* m = nullptr;
        if (!v.canBeNil) {
            if (s != Struct::Null) {
                m = getMethodByName(result, "init", type);
                bool hasDefaultConstructor = false;
                if (m->args.size() == 1) {
                    hasDefaultConstructor = true;
                } else {
                    for (Function* over_ : m->overloads) {
                    Method* overload = (Method*) over_;
                        if (overload->args.size() == 1) {
                            hasDefaultConstructor = true;
                            m = overload;
                            break;
                        }
                    }
                }
                if (!hasDefaultConstructor) {
                    goto noNil;
                }
            } else {
            noNil:
                transpilerError("Uninitialized variable '" + name + "' with non-nil type '" + type + "'", start);
                errors.push_back(err);
            }
        }
        type = sclTypeToCType(result, type);
        if (type == "scl_float") {
            append("%s Var_%s = 0.0;\n", type.c_str(), v.name.c_str());
        } else {
            if (s != Struct::Null && !s.isStatic() && m != nullptr) {
                append("%s Var_%s = ALLOC(%s);\n", type.c_str(), v.name.c_str(), s.name.c_str());
                append("_scl_push(%s, Var_%s);\n", type.c_str(), v.name.c_str());
                typeStack.push_back(removeTypeModifiers(v.type));
                methodCall(m, fp, result, warns, errors, body, i);
            } else if (hasTypealias(result, type) || hasLayout(result, type)) {
                append("%s Var_%s;\n", type.c_str(), v.name.c_str());
            } else {
                append("%s Var_%s = 0;\n", type.c_str(), v.name.c_str());
            }
        }
    }
} // namespace sclc

