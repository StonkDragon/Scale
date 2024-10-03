
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(TruthyType) {
        noUnused;
        append("scale_push(scale_int, 1);\n");
        typeStack.push_back("bool");
    }
} // namespace sclc

