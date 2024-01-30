#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(FloatLiteral) {
        noUnused;

        // only parse to verify
        auto num = parseDouble(body[i]);
        if (num.isError()) {
            errors.push_back(num.unwrapErr());
            return;
        }
        // we still want the original value from source code here
        append("_scl_push(scl_float, %s);\n", body[i].value.c_str());
        typeStack.push("float");
    }
} // namespace sclc

