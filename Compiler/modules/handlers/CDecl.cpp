#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(CDecl) {
        noUnused;
        safeInc();
        transpilerError("Old inline c is no longer supported. Use 'c!' instead", i);
        errors.push_back(err);
    }
} // namespace sclc

