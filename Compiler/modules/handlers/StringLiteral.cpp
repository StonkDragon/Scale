
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(StringLiteral) {
        noUnused;
        std::string str = unquote(body[i].value);
        if (body[i].type == tok_utf_string_literal && !checkUTF8(str)) {
            transpilerError("Invalid UTF-8 string", i);
            errors.push_back(err);
            return;
        }
        ID_t hash = id(str.c_str());
        append("_scl_push(scl_str, _scl_static_string(\"%s\", 0x%lxUL));\n", body[i].value.c_str(), hash);
        typeStack.push_back("str");
    }
} // namespace sclc

