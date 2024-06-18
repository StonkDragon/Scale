
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(New) {
        noUnused;
        safeInc();
        std::string elemSize = "sizeof(scl_any)";
        std::string typeString = "any";
        if (body[i].type == tok_identifier && body[i].value == "<") {
            safeInc();
            auto type = parseType(body, i);
            if (!type.success) {
                errors.push_back(type);
                return;
            }
            typeString = removeTypeModifiers(type.value);
            elemSize = "sizeof(" + sclTypeToCType(result, type.value) + ")";
            safeInc();
            safeInc();
        }
        int dimensions = 0;
        if (body[i].type == tok_curly_open) {
            size_t stack_start = typeStack.size();
            size_t nelems = 0;
            size_t start_i = i;
            append("{\n");
            scopeDepth++;
            safeInc();
            if (body[i].type != tok_curly_close) {
                while (body[i].type != tok_curly_close) {
                    handle(Token);
                    safeInc();
                    if (body[i].type == tok_comma || body[i].type == tok_curly_close) {
                        if (body[i].type == tok_comma) {
                            safeInc();
                        }
                        nelems++;
                    }
                }
            } else {
                append("_scl_push(scl_any, _scl_new_array_by_size(0, %s));\n", elemSize.c_str());
                typeStack.push_back("[" + typeString + "]");
                scopeDepth--;
                append("}\n");
                return;
            }

            size_t stack_end = typeStack.size();
            if (stack_end - stack_start != nelems) {
                {
                    transpilerError("Expected expression evaluating to a value after '{'", start_i);
                    errors.push_back(err);
                }
                transpilerError("Expected " + std::to_string(nelems) + " expressions, but got " + std::to_string(stack_end - stack_start), start_i);
                err.isNote = true;
                errors.push_back(err);
                return;
            }
            dimensions = 1;
            append("scl_int len = %zu;\n", nelems);
            append("%s* arr = _scl_new_array_by_size(len, %s);\n", sclTypeToCType(result, typeString).c_str(), elemSize.c_str());
            append("for (scl_int index = 0; index < len; index++) {\n");
            scopeDepth++;
            append("arr[len - index - 1] = _scl_pop(%s);\n", sclTypeToCType(result, typeString).c_str());
            for (size_t i = 0; i < nelems; i++) {
                typePop;
            }
            scopeDepth--;
            append("}\n");
            append("_scl_push(%s*, arr);\n", sclTypeToCType(result, typeString).c_str());

            scopeDepth--;
            append("}\n");

            typeStack.push_back("[" + typeString + "]");
            return;
        }
        std::vector<int> startingOffsets;
        while (body[i].type == tok_bracket_open) {
            dimensions++;
            startingOffsets.push_back(i);
            safeInc();
            if (body[i].type == tok_bracket_close) {
                transpilerError("Empty array dimensions are not allowed", i);
                errors.push_back(err);
                i--;
                return;
            }
            while (body[i].type != tok_bracket_close) {
                handle(Token);
                safeInc();
            }
            if (i + 1 >= body.size()) {
                goto lastTokenInBody;
            }
            safeInc();
        }
        i--;
    lastTokenInBody:
        if (dimensions == 0) {
            transpilerError("Expected '[', but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        if (dimensions == 1) {
            append("_scl_top(scl_any) = _scl_new_array_by_size(_scl_top(scl_int), %s);\n", elemSize.c_str());
        } else {
            append("{\n");
            scopeDepth++;
            std::string dims = "";
            for (int i = dimensions - 1; i >= 0; i--) {
                append("scl_int _scl_dim%d = _scl_pop(scl_int);\n", i);
                if (dimensions - i - 1) dims += ", ";
                dims += "_scl_dim" + std::to_string(dimensions - i - 1);
                if (!isPrimitiveIntegerType(typeStackTop)) {
                    transpilerError("Array size must be an integer, but got '" + removeTypeModifiers(typeStackTop) + "'", startingOffsets[i]);
                    errors.push_back(err);
                    return;
                }
                typePop;
            }

            append("scl_int tmp[] = {%s};\n", dims.c_str());
            append("_scl_push(scl_any, _scl_multi_new_array_by_size(%d, tmp, %s));\n", dimensions, elemSize.c_str());
            scopeDepth--;
            append("}\n");
        }
        std::string type = typeString;
        for (int i = 0; i < dimensions; i++) {
            type = "[" + type + "]";
        }
        typeStack.push_back(type);
    }
} // namespace sclc

