
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(INVALID) {
        noUnused;
        if (i < body.size() && ((ssize_t) i) >= 0) {
            transpilerError("Unknown token: '" + body[i].value + "'", i);
            errors.push_back(err);
        } else {
            transpilerError("Unknown token: '" + body.back().value + "'", i);
            errors.push_back(err);
        }
    }
} // namespace sclc

