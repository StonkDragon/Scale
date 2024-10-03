
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(Continue) {
        noUnused;
        append("continue;\n");
    }
} // namespace sclc

