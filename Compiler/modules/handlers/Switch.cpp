#include <gc/gc_allocator.h>

#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Switch) {
        noUnused;
        std::string type = typeStackTop;
        switchTypes.push_back(type);
        typePop;
        const Struct& s = getStructByName(result, type);
        if (s.super == "Union") {
            append("{\n");
            scopeDepth++;
            append("scl_Union union_switch = _scl_pop(scl_Union);\n");
            append("switch (union_switch->__tag) {\n");
            scopeDepth++;
            varScopePush();
            pushSwitch2();
            return;
        }
        append("switch (_scl_pop(scl_int)) {\n");
        scopeDepth++;
        varScopePush();
        pushSwitch();
    }
} // namespace sclc

