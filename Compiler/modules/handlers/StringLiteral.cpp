
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
        findOrAdd(cstrings, str);
        append("scale_push(scale_str, (scale_str) &static_str_%lu.data);\n", index);
        typeStack.push_back("str");
    }
} // namespace sclc

