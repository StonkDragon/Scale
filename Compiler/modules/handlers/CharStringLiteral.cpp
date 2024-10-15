
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
        append("scale_push(scale_int8*, (scale_str) (scale_mark_static(&static_cstr_%lu.layout) + sizeof(memory_layout_t)));\n", index);
        typeStack.push_back("[int8]");
    }
} // namespace sclc
