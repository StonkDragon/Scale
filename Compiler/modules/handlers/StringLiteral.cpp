
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    std::vector<std::string> strings;
    extern std::vector<std::string> cstrings;
    
    handler(StringLiteral) {
        noUnused;
        std::string str = unquote(body[i].value);
        if (body[i].type == tok_utf_string_literal && !checkUTF8(str)) {
            transpilerError("Invalid UTF-8 string", i);
            errors.push_back(err);
            return;
        }
        str = body[i].value;
        size_t index = findOrAdd(strings, str);
        size_t cindex = findOrAdd(cstrings, str);
        append("scale_mark_static(&static_cstr_%lu.layout);\n", cindex);
        append("scale_push(scale_str, (scale_str) (scale_mark_static(&static_str_%lu.layout) + sizeof(memory_layout_t)));\n", index);
        typeStack.push_back("str");
    }
} // namespace sclc

