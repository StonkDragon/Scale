
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(CDecl) {
        noUnused;
        safeInc();
        transpilerError("Old inline c is no longer supported. Use 'c!' instead", i);
        errors.push_back(err);
    }
} // namespace sclc

