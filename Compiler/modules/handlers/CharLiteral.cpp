#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(CharLiteral) {
        noUnused;
        append("_scl_push(scl_int8, '%s');\n", body[i].value.c_str());
        typeStack.push_back("int8");
    }
} // namespace sclc

