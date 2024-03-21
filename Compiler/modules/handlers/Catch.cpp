#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Catch) {
        noUnused;
        std::string ex = "Exception";
        safeInc();
        if (body[i].value == "typeof") {
            safeInc();
            varScopePop();
            scopeDepth--;
            if (wasTry()) {
                popTry();
            } else if (wasCatch()) {
                popCatch();
            }

            varScopePush();
            pushCatch();
            Struct s = getStructByName(result, body[i].value);
            if (s == Struct::Null) {
                transpilerError("Trying to catch unknown Exception of type '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }

            if (structExtends(result, s, "Error")) {
                transpilerError("Cannot catch Exceptions that extend 'Error'", i);
                errors.push_back(err);
                return;
            }
            append("} else if (_scl_is_instance_of(_scl_exception_handler.exception, 0x%lxUL)) {\n", id(body[i].value.c_str()));
        } else {
            i--;
            {
                transpilerError("Generic Exception caught here:", i);
                errors.push_back(err);
            }
            transpilerError("Add 'typeof <Exception>' after 'catch' to catch specific Exceptions", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        std::string exName = "exception";
        if (body.size() > i + 1 && body[i + 1].value == "as") {
            safeInc();
            safeInc();
            exName = body[i].value;
        }
        scopeDepth++;
        vars.push_back(Variable(exName, ex));
        append("scl_%s Var_%s = (scl_%s) _scl_exception_handler.exception;\n", ex.c_str(), exName.c_str(), ex.c_str());
    }
} // namespace sclc

