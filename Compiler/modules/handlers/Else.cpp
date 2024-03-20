#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

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

