
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(Fi) {
        noUnused;
        varScopePop();
        scopeDepth--;
        append("}\n");
        condCount--;
    }
} // namespace sclc

