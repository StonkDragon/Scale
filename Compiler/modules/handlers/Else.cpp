
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(Else) {
        noUnused;
        varScopePop();
        scopeDepth--;
        append("} else {\n");
        scopeDepth++;
        varScopePush();
    }
} // namespace sclc

