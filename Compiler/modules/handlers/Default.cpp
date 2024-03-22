#include <gc/gc_allocator.h>

#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Default) {
        noUnused;
        if (!wasSwitch() && !wasSwitch2()) {
            transpilerError("Unexpected 'default' statement", i);
            errors.push_back(err);
            return;
        }
        const Struct& s = getStructByName(result, switchTypes.back());
        if (s.super == "Union") {
            append("default: {\n");
            scopeDepth++;
            varScopePush();
            append("%s Var_it = union_switch;\n", sclTypeToCType(result, s.name).c_str());
            vars.push_back(Variable("it", s.name));
            return;
        }
        append("default: {\n");
        scopeDepth++;
        varScopePush();
    }
} // namespace sclc

