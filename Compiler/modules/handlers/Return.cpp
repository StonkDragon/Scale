#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Return) {
        noUnused;

        if (function->return_type == "nothing") {
            transpilerError("Cannot return from a function with return type 'nothing'", i);
            errors.push_back(err);
            return;
        }

        auto type = removeTypeModifiers(function->return_type);

        if (type == "none") {
            if (!function->isMethod && !Main::options::noMain && function->name == "main") {
                append("return 0;\n");
            } else {
                append("return;\n");
            }
        } else {
            if (function->return_type.front() == '*' && function->has_async) {
                append("return (%s*) ({\n", sclTypeToCType(result, function->return_type).c_str());
            } else {
                append("return (%s) ({\n", sclTypeToCType(result, function->return_type).c_str());
            }
            scopeDepth++;

            std::string returningType = typeStackTop;
            if (function->namedReturnValue.name.size()) {
                returningType = function->namedReturnValue.type;
                append("%s retVal = Var_%s;\n", sclTypeToCType(result, returningType).c_str(), function->namedReturnValue.name.c_str());
            } else {
                if (function->return_type.front() == '*') {
                    if (!function->has_async) {
                        append("%s retVal = *_scl_pop(%s*);\n", sclTypeToCType(result, function->return_type).c_str(), sclTypeToCType(result, function->return_type).c_str());
                    } else {
                        append("%s* retVal = _scl_pop(%s*);\n", sclTypeToCType(result, function->return_type).c_str(), sclTypeToCType(result, function->return_type).c_str());
                    }
                } else {
                    append("%s retVal = _scl_pop(%s);\n", sclTypeToCType(result, function->return_type).c_str(), sclTypeToCType(result, function->return_type).c_str());
                }
            }
            if (function->return_type.front() != '*' && function->has_async) {
                bool cantBeNil = !typeCanBeNil(function->return_type) && !hasEnum(result, function->return_type);
                if (cantBeNil) {
                    #define TYPEALIAS_CAN_BE_NIL(result, ta) (hasTypealias(result, ta) && typealiasCanBeNil(result, ta))
                    if (TYPEALIAS_CAN_BE_NIL(result, function->return_type)) {
                        cantBeNil = false;
                    }
                }
                if (cantBeNil) {
                    if (typeCanBeNil(returningType) && !TYPEALIAS_CAN_BE_NIL(result, returningType)) {
                        transpilerError("Returning maybe-nil type '" + returningType + "' from function with not-nil return type '" + function->return_type + "'", i);
                        errors.push_back(err);
                    }
                    if (!function->namedReturnValue.name.size()) {
                        if (typeCanBeNil(returningType)) {
                            transpilerError("Returning maybe-nil type '" + returningType + "' from function with not-nil return type '" + function->return_type + "'", i);
                            errors.push_back(err);
                        }
                        if (!typesCompatible(result, returningType, function->return_type, true)) {
                            transpilerError("Returning type '" + returningType + "' from function with return type '" + function->return_type + "'", i);
                            errors.push_back(err);
                        }
                    }
                    append("SCL_ASSUME(*(scl_int*) &retVal, \"Tried returning nil from function returning not-nil type '%%s'!\", \"%s\");\n", function->return_type.c_str());
                }
            }
            append("retVal;\n");
            scopeDepth--;
            append("});\n");
        }
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
    }
} // namespace sclc

