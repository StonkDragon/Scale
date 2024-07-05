
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

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
            append("_scl_top(scl_int) = !_scl_is_instance(_scl_top(scl_any));\n");
        } else if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            append("_scl_top(scl_int) = _scl_is_array(_scl_top(scl_any));\n");
        } else if (hasLayout(result, type)) {
            append("_scl_top(scl_int) = _scl_sizeof(_scl_top(scl_any)) >= sizeof(struct Layout_%s);", type.c_str());
        } else {
            const Struct& s = getStructByName(result, type);
            Interface* iface = getInterfaceByName(result, type);
            if (s == Struct::Null && iface == nullptr) {
                transpilerError("Usage of undeclared struct '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            } else if (s != Struct::Null) {
                if (!s.isStatic()) {
                    append("_scl_top(scl_int) = _scl_top(scl_any) && _scl_is_instance_of(_scl_top(scl_any), 0x%lxUL);\n", id(type.c_str()));
                } else {
                    append("_scl_top(scl_int) = 0;\n");
                }
            } else {
                const Struct& stackStruct = getStructByName(result, typeStackTop);
                if (stackStruct == Struct::Null || stackStruct.isStatic()) {
                    append("_scl_top(scl_int) = 0;\n");
                } else {
                    append("_scl_top(scl_int) = _scl_top(scl_any) && %d;\n", stackStruct.implements(iface->name));
                }
            }
        }
        typePop;
        typeStack.push_back("bool");
    }
} // namespace sclc

