#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(As) {
        noUnused;
        safeInc();
        FPResult type = parseType(body, &i, getTemplates(result, function));
        if (!type.success) {
            errors.push_back(type);
            return;
        }
        if (type.value == "float" && typeStackTop != "float") {
            append("_scl_top(scl_float) = (float) _scl_top(scl_int);\n");
            typePop;
            typeStack.push_back("float");
            return;
        }
        if ((type.value == "int" && typeStackTop != "float") || type.value == "float" || type.value == "any") {
            typePop;
            typeStack.push_back(type.value);
            return;
        }
        if (type.value == "int") {
            append("_scl_top(scl_int) = (scl_int) _scl_top(scl_float);\n");
            typePop;
            typeStack.push_back("int");
            return;
        }
        if (type.value == "lambda" || strstarts(type.value, "lambda(")) {
            typePop;
            append("SCL_ASSUME(_scl_top(scl_int), \"Nil cannot be cast to non-nil type '%%s'!\", \"%s\");\n", type.value.c_str());
            typeStack.push_back(type.value);
            return;
        }
        if (isPrimitiveIntegerType(type.value)) {
            if (typeStackTop == "float") {
                append("_scl_top(scl_int) = (scl_int) _scl_top(scl_float);\n");
            }
            typePop;
            typeStack.push_back(type.value);
            append("_scl_top(scl_int) &= SCL_%s_MAX;\n", type.value.c_str());
            return;
        }
        auto typeIsPtr = [type]() -> bool {
            std::string t = type.value;
            if (t.empty()) return false;
            t = removeTypeModifiers(t);
            t = notNilTypeOf(t);
            return (t.front() == '[' && t.back() == ']');
        };
        if (hasTypealias(result, type.value) || typeIsPtr()) {
            typePop;
            typeStack.push_back(type.value);
            return;
        }
        if (hasLayout(result, type.value)) {
            append("SCL_ASSUME(_scl_sizeof(_scl_top(scl_any)) >= sizeof(struct Layout_%s), \"Layout '%%s' requires more memory than the pointer has available (required: \" SCL_INT_FMT \" found: \" SCL_INT_FMT \")\", \"%s\", sizeof(struct Layout_%s), _scl_sizeof(_scl_top(scl_any)));", type.value.c_str(), type.value.c_str(), type.value.c_str());
            typePop;
            typeStack.push_back(type.value);
            return;
        }
        if (getStructByName(result, type.value) == Struct::Null) {
            transpilerError("Use of undeclared Struct '" + type.value + "'", i);
            errors.push_back(err);
            return;
        }
        if (i + 1 < body.size() && body[i + 1].value == "?") {
            if (!typeCanBeNil(typeStackTop)) {
                transpilerError("Unneccessary cast to maybe-nil type '" + type.value + "?' from not-nil type '" + typeStackTop + "'", i - 1);
                warns.push_back(err);
            }
            typePop;
            typeStack.push_back(type.value + "?");
            safeInc();
        } else {
            if (typeCanBeNil(typeStackTop)) {
                append("SCL_ASSUME(_scl_top(scl_int), \"Nil cannot be cast to non-nil type '%%s'!\", \"%s\");\n", type.value.c_str());
            }
            std::string destType = removeTypeModifiers(type.value);

            if (getStructByName(result, destType) != Struct::Null) {
                append("_scl_checked_cast(_scl_top(scl_any), 0x%lxUL, \"%s\");\n", id(destType.c_str()), destType.c_str());
            }
            typePop;
            typeStack.push_back(type.value);
        }
    }
} // namespace sclc

