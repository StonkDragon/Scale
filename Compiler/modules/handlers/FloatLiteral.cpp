
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(FloatLiteral) {
        noUnused;

        std::string type = "float";
        if (body[i].value.back() == 'f') {
            type = "float32";
        }

        // only parse to verify
        auto num = parseDouble(body[i]);
        if (num.isError()) {
            errors.push_back(num.unwrapErr());
            return;
        }
        append("scale_push(scale_%s, %s);\n", type.c_str(), std::to_string(num.unwrap()).c_str());
        typeStack.push_back(type);
    }
} // namespace sclc

