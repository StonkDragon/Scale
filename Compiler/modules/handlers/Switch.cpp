
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

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
            append("scale_Union union_switch = scale_pop(scale_Union);\n");
            append("switch (union_switch->__tag) {\n");
            scopeDepth++;
            varScopePush();
            pushSwitch2();
            return;
        }
        append("switch (scale_pop(scale_int)) {\n");
        scopeDepth++;
        varScopePush();
        pushSwitch();
    }
} // namespace sclc

