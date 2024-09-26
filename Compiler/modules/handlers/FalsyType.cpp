
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(FalsyType) {
        noUnused;
        append("scale_push(scale_int, 0);\n");
        if (body[i].type == tok_nil) {
            typeStack.push_back("any");
        } else {
            typeStack.push_back("bool");
        }
    }
} // namespace sclc

