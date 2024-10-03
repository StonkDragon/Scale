
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(Goto) {
        noUnused;
        safeInc();
        if (body[i].type != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
            errors.push_back(err);
        }
        append("goto $lbl%s;\n", body[i].value.c_str());
    }
} // namespace sclc

