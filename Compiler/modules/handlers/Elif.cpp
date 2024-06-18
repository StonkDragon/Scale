
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Elif) {
        noUnused;
        varScopePop();
        scopeDepth--;

        append("} else if (({\n");
        scopeDepth++;
        size_t typeStackSize = typeStack.size();
        append("_scl_scope(128*sizeof(scl_int));\n");
        safeInc();
        varScopePush();
        while (body[i].type != tok_then) {
            handle(Token);
            safeInc();
        }
        varScopePop();
        size_t diff = typeStack.size() - typeStackSize;
        if (diff > 1) {
            transpilerError("More than one value in if-condition. Maybe you forgot to use '&&' or '||'?", i);
            warns.push_back(err);
        }
        if (diff == 0) {
            transpilerError("Expected one value in if-condition", i);
            errors.push_back(err);
            return;
        }
        append("_scl_pop(scl_int);\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        varScopePush();
        append("_scl_scope(128*sizeof(scl_int));\n");
    }
} // namespace sclc

