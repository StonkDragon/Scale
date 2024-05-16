#include <gc/gc_allocator.h>

#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Until) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        append("while (({\n");
        varScopePush();
        scopeDepth++;
        size_t typeStackSize = typeStack.size();
        safeInc();
        while (body[i].type != tok_do) {
            handle(Token);
            safeInc();
        }
        size_t diff = typeStack.size() - typeStackSize;
        if (diff > 1) {
            transpilerError("More than one value in until-condition. Maybe you forgot to use '&&' or '||'?", i);
            warns.push_back(err);
        }
        if (diff == 0) {
            transpilerError("Expected one value in until-condition", i);
            errors.push_back(err);
            return;
        }
        append("!_scl_pop(scl_int);\n");
        varScopePop();
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        pushOther();
        varScopePush();
        append("_scl_scope(128*sizeof(scl_int));\n");
    }
} // namespace sclc

