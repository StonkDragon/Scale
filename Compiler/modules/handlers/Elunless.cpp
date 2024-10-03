
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(Elunless) {
        noUnused;
        varScopePop();
        scopeDepth--;
        
        append("} else if (!({\n");
        scopeDepth++;
        size_t typeStackSize = typeStack.size();
        safeInc();
        varScopePush();
        while (body[i].type != tok_then) {
            handle(Token);
            safeInc();
        }
        varScopePop();
        size_t diff = typeStack.size() - typeStackSize;
        if (diff > 1) {
            {
                transpilerError("More than one value in unless-condition. Maybe you forgot to use '&&' or '||'?", i);
                warns.push_back(err);
            }
            transpilerError("Found: [ " + stackSliceToString(diff) + " ]", i);
            err.isNote = true;
            warns.push_back(err);
        }
        if (diff == 0) {
            transpilerError("Expected one value in unless-condition", i);
            errors.push_back(err);
            return;
        }
        append("scale_pop(scale_int);\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        varScopePush();
    }
} // namespace sclc

