
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    char unquoteChar(const std::string& str);

    handler(CharLiteral) {
        noUnused;
        append("scale_push(scale_int, %d & SCALE_int8_MASK);\n", unquoteChar(body[i].value));
        typeStack.push_back("int8");
    }
} // namespace sclc
