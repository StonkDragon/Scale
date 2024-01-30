#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(TruthyType) {
        noUnused;
        append("_scl_push(scl_int, 1);\n");
        typeStack.push("bool");
    }
} // namespace sclc

