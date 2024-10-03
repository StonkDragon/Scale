
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(While) {
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
            {
                transpilerError("More than one value in while-condition. Maybe you forgot to use '&&' or '||'?", i);
                warns.push_back(err);
            }
            transpilerError("Found: [ " + stackSliceToString(diff) + " ]", i);
            err.isNote = true;
            warns.push_back(err);
        }
        if (diff == 0) {
            transpilerError("Expected one value in while-condition", i);
            errors.push_back(err);
            return;
        }
        append("scale_pop(scale_int);\n");
        varScopePop();
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        pushOther();
        varScopePush();
    }
} // namespace sclc

