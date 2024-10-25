
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    std::vector<std::string> cstrings;

    handler(CharStringLiteral) {
        noUnused;
        std::string str = body[i].value;
        size_t index = findOrAdd(cstrings, str);
        append("scale_push(scale_int8*, (scale_int8*) static_cstr_%lu.data);\n", index);
        typeStack.push_back("[int8]");
    }
} // namespace sclc
