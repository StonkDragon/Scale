
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(As) {
        noUnused;
        size_t start = i;
        safeInc();
        FPResult type = parseType(body, i);
        if (!type.success) {
            errors.push_back(type);
            return;
        }
        bool constBefore = typeIsConst(typeStackTop);
        bool constAfter = typeIsConst(type.value);
        if (constBefore && !constAfter) {
            transpilerError("Cannot cast away 'const' qualifier of type '" + typeStackTop + "' in cast to '" + type.value + "'", start);
            errors.push_back(err);
            return;
        }
        const Struct& s = getStructByName(result, type.value);
        bool doesCheckedCast = false;
        if (hasLayout(result, type.value)) {
            append("{\n");
            append("  scl_int sz = _scl_sizeof(_scl_top(scl_any));\n");
            append("  _scl_assert(sz >= sizeof(struct Layout_%s), \"Layout '%s' requires more memory than the pointer has available (required: \" SCL_INT_FMT \" found: \" SCL_INT_FMT \")\", sizeof(struct Layout_%s), sz);", type.value.c_str(), type.value.c_str(), type.value.c_str());
            append("}\n");
            typePop;
            typeStack.push_back(type.value);
            return;
        } else if (s != Struct::Null && !s.isStatic()) {
            std::string destType = removeTypeModifiers(type.value);
            append("_scl_checked_cast(_scl_top(scl_any), 0x%lxUL, \"%s\");\n", id(destType.c_str()), destType.c_str());
            doesCheckedCast = true;
        }

        if (!typeCanBeNil(type.value) && !typealiasCanBeNil(result, type.value)) {
            if (!doesCheckedCast && typeCanBeNil(typeStackTop)) {
                append("_scl_assert_fast(_scl_top(scl_int), \"Nil cannot be cast to non-nil type '%s'!\");\n", type.value.c_str());
            }
        } else {
            if (!typeCanBeNil(typeStackTop) && !isPrimitiveType(type.value)) {
                transpilerError("Unneccessary cast to maybe-nil type '" + type.value + "' from not-nil type '" + typeStackTop + "'", start);
                warns.push_back(err);
            }
        }

        append("_scl_cast_stack(%s, %s);\n", sclTypeToCType(result, type.value).c_str(), sclTypeToCType(result, typeStackTop).c_str());
        if (isPrimitiveIntegerType(type.value)) {
            append("_scl_top(scl_int) &= SCL_%s_MASK;\n", removeTypeModifiers(type.value).c_str());
        }
        typePop;
        typeStack.push_back(type.value);
    }
} // namespace sclc

