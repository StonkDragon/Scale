
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    char unquoteChar(const std::string& str);

    handler(CharLiteral) {
        noUnused;
        append("_scl_push(scl_int, %d & SCL_int8_MASK);\n", unquoteChar(body[i].value));
        typeStack.push_back("int8");
    }
} // namespace sclc
