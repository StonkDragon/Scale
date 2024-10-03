
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(Break) {
        noUnused;
        append("break;\n");
    }
} // namespace sclc

