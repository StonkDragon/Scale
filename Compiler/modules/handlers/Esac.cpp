
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(Esac) {
        noUnused;
        if (!wasSwitch() && !wasSwitch2()) {
            transpilerError("Unexpected 'esac'", i);
            errors.push_back(err);
            return;
        }
        append("break;\n");
        varScopePop();
        scopeDepth--;
        append("}\n");
    }
} // namespace sclc

