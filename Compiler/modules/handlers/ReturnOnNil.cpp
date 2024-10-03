
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(ReturnOnNil) {
        noUnused;
        if (typeCanBeNil(typeStackTop)) {
            std::string type = typeStackTop;
            typePop;
            typeStack.push_back(type.substr(0, type.size() - 1));
        }
        if (function->return_type == "nothing") {
            transpilerError("Returning from a function with return type 'nothing' is not allowed.", i);
            errors.push_back(err);
            return;
        }
        if (!typeCanBeNil(function->return_type) && function->return_type != "none") {
            transpilerError("Return-if-nil operator '?' behaves like assert-not-nil operator '!!' in not-nil returning function.", i);
            warns.push_back(err);
            append("scale_assert_fast((scale_top(scale_int)), \"Not nil assertion failed!\");\n");
        } else {
            if (function->return_type == "none") {
                if (!function->isMethod && !Main::options::noMain && function->name == "main") {
                    append("if ((scale_top(scale_int)) == 0) return 0;\n");
                } else {
                    append("if ((scale_top(scale_int)) == 0) return;\n");
                }
            } else {
                append("if ((scale_top(scale_int)) == 0) return 0;\n");
            }
        }
    }
} // namespace sclc
