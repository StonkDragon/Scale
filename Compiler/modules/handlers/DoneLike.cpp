#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(DoneLike) {
        noUnused;
        if (wasUsing()) {
            std::string name = usingNames.back();
            usingNames.pop_back();
            const Variable& v = getVar(name);
            append("_scl_push(%s, Var_%s);\n", sclTypeToCType(result, v.type).c_str(), v.name.c_str());
            typeStack.push_back(v.type);
            Struct& s = getStructByName(result, v.type);
            if (!s.implements("Closeable")) {
                transpilerError("Type '" + v.type + "' does not implement 'Closeable'", i);
                errors.push_back(err);
                return;
            }
            Method* closeMethod = getMethodByName(result, "close", v.type);
            if (closeMethod == nullptr) {
                transpilerError("No method 'close' found in type '" + v.type + "'", i);
                errors.push_back(err);
                return;
            }
            methodCall(closeMethod, fp, result, warns, errors, body, i);
        }
        varScopePop();
        scopeDepth--;
        if (wasCatch()) {
            append("} else {\n");
            if (function->has_restrict) {
                if (function->isMethod) {
                    append("  _scl_unlock((scl_SclObject) Var_self);\n");
                } else {
                    append("  _scl_unlock(function_lock$%s);\n", function->name.c_str());
                }
            }
            append("  fn_throw((scl_Exception) _scl_exception_handler.exception);\n");
            append("}\n");
            scopeDepth--;
        }
        append("}\n");
        if (repeat_depth > 0 && wasRepeat()) {
            repeat_depth--;
        }
        if (wasSwitch() || wasSwitch2()) {
            switchTypes.pop_back();
            if (wasSwitch2()) {
                scopeDepth--;
                append("}\n");
            }
        }
        popOther();
    }
} // namespace sclc

