#include <gc/gc_allocator.h>

#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Repeat) {
        noUnused;
        safeInc();
        if (body[i].type != tok_number) {
            transpilerError("Expected integer, but got '" + body[i].value + "'", i);
            errors.push_back(err);
            if (i + 1 < body.size() && body[i + 1].type != tok_do)
                goto lbl_repeat_do_tok_chk;
            safeInc();
            return;
        }
        if (parseNumber(body[i].value) <= 0) {
            transpilerError("Repeat count must be greater than 0", i);
            errors.push_back(err);
        }
        if (i + 1 < body.size() && body[i + 1].type != tok_do) {
        lbl_repeat_do_tok_chk:
            transpilerError("Expected 'do', but got '" + body[i + 1].value + "'", i+1);
            errors.push_back(err);
            safeInc();
            return;
        }
        append("for (scl_int %c = 0; %c < (scl_int) %s; %c++) {\n",
            'a' + repeat_depth,
            'a' + repeat_depth,
            body[i].value.c_str(),
            'a' + repeat_depth
        );
        repeat_depth++;
        scopeDepth++;
        varScopePush();
        pushRepeat();
        safeInc();
    }
} // namespace sclc

