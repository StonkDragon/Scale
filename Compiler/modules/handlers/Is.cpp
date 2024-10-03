
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(Is) {
        noUnused;
        safeInc();
        FPResult res = parseType(body, i);
        if (!res.success) {
            errors.push_back(res);
            return;
        }
        std::string type = res.value;
        if (isPrimitiveType(type)) {
            append("scale_top(scale_int) = !scale_is_instance(scale_top(scale_any));\n");
        } else if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            append("scale_top(scale_int) = scale_is_array(scale_top(scale_any));\n");
        } else if (hasLayout(result, type)) {
            append("scale_top(scale_int) = scale_sizeof(scale_top(scale_any)) >= sizeof(struct Layout_%s);", type.c_str());
        } else {
            const Struct& s = getStructByName(result, type);
            Interface* iface = getInterfaceByName(result, type);
            if (s == Struct::Null && iface == nullptr) {
                transpilerError("Usage of undeclared struct '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            } else if (s != Struct::Null) {
                if (!s.isStatic()) {
                    append("scale_top(scale_int) = scale_top(scale_any) && scale_is_instance_of(scale_top(scale_any), 0x%lxUL);\n", id(type.c_str()));
                } else {
                    append("scale_top(scale_int) = 0;\n");
                }
            } else {
                const Struct& stackStruct = getStructByName(result, typeStackTop);
                if (stackStruct == Struct::Null || stackStruct.isStatic()) {
                    append("scale_top(scale_int) = 0;\n");
                } else {
                    append("scale_top(scale_int) = scale_top(scale_any) && %d;\n", stackStruct.implements(iface->name));
                }
            }
        }
        typePop;
        typeStack.push_back("bool");
    }
} // namespace sclc

