#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(CharLiteral) {
        noUnused;
        append("_scl_push(scl_int, '%s');\n", body[i].value.c_str());
        typeStack.push("int8");
    }
} // namespace sclc

