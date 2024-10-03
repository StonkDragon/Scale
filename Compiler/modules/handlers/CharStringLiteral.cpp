
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(CharStringLiteral) {
        noUnused;
        std::string str = unquote(body[i].value);
        append("scale_push(scale_int8*, scale_static_cstring(\"%s\"));\n", body[i].value.c_str());
        typeStack.push_back("[int8]");
    }
} // namespace sclc

