#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Pragma) {
        noUnused;
        safeInc();
        if (body[i].value == "stackalloc") {
            safeInc();
            transpilerError("Stackalloc arrays have been removed from Scale", i);
            errors.push_back(err);
        } else if (body[i].value == "note" || body[i].value == "warn" || body[i].value == "error") {
            safeInc();
            if (body[i].type != tok_string_literal) {
                transpilerError("Expected string literal after '" + body[i - 1].value + "'", i);
                errors.push_back(err);
                return;
            }
            std::string type = body[i - 1].value;
            std::string message = body[i].value;
            transpilerError(message, i - 2);
            if (body[i - 1].value == "note") {
                err.isNote = true;
            }
            if (body[i - 1].value == "warn") {
                warns.push_back(err);
            } else {
                errors.push_back(err);
            }
        } else {
            transpilerError("Unknown pragma '" + body[i].value + "'", i);
            errors.push_back(err);
        }
    }
} // namespace sclc

