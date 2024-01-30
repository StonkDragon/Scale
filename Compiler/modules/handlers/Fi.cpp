#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Fi) {
        noUnused;
        varScopePop();
        scopeDepth--;
        append("}\n");
        condCount--;
    }
} // namespace sclc

