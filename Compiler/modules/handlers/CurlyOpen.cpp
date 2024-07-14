
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(CurlyOpen) {
        noUnused;
        append("_scl_push(scl_any, ({\n");
        scopeDepth++;
        safeInc();
        int n = 0;
        while (body[i].type != tok_curly_close) {
            if (body[i].type == tok_comma) {
                safeInc();
                continue;
            }
            bool didPush = false;
            while (body[i].type != tok_comma && body[i].type != tok_curly_close) {
                handle(Token);
                safeInc();
                didPush = true;
            }
            if (didPush) {
                append("scl_any tmp%d = _scl_pop(scl_any);\n", n);
                n++;
                typePop;
            }
        }
        std::string type = "any";
        if (i + 1 < body.size() && body[i + 1].type == tok_as) {
            safeIncN(2);
            FPResult r = parseType(body, i);
            if (!r.success) {
                errors.push_back(r);
                return;
            }
            type = r.value;
            if (type.front() == '[') {
                type = type.substr(1, type.size() - 2);
            } else {
                type = "any";
            }
        }
        std::string ctype = sclTypeToCType(result, type);
        append("%s* tmp = (%s*) _scl_new_array_by_size(%d, sizeof(%s));\n", ctype.c_str(), ctype.c_str(), n, ctype.c_str());
        for (int i = 0; i < n; i++) {
            append("tmp[%d] = *(%s*) &tmp%d;\n", i, ctype.c_str(), i);
        }
        append("tmp;\n");
        typeStack.push_back("[" + type + "]");
        scopeDepth--;
        append("}));\n");
    }
} // namespace sclc

