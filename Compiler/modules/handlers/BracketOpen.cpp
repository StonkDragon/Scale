
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(BracketOpen) {
        noUnused;
        std::string type = removeTypeModifiers(typeStackTop);
        
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            if ((i + 2) < body.size()) {
                if (body[i + 1].type == tok_number && body[i + 2].type == tok_bracket_close) {
                    safeIncN(2);
                    long long index = std::stoll(body[i - 1].value);
                    if (index < 0) {
                        transpilerError("Array subscript cannot be negative", i - 1);
                        errors.push_back(err);
                        return;
                    }
                    append("scale_array_check_bounds_or_throw(scale_top(scale_any*), %s);\n", body[i - 1].value.c_str());
                    typePop;
                    typeStack.push_back(type.substr(1, type.size() - 2));
                    if (isPrimitiveIntegerType(typeStackTop)) {
                        append("scale_top(scale_int) = scale_top(%s)[%s];\n", sclTypeToCType(result, type).c_str(), body[i - 1].value.c_str());
                    } else {
                        append("scale_top(%s) = scale_top(%s)[%s];\n", sclTypeToCType(result, typeStackTop).c_str(), sclTypeToCType(result, type).c_str(), body[i - 1].value.c_str());
                    }
                    return;
                }
            }
            safeInc();
            if (body[i].type == tok_bracket_close) {
                transpilerError("Array subscript required", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            scopeDepth++;
            append("%s tmp = scale_pop(%s);\n", sclTypeToCType(result, type).c_str(), sclTypeToCType(result, type).c_str());
            typePop;
            varScopePush();
            while (body[i].type != tok_bracket_close) {
                handle(Token);
                safeInc();
            }
            varScopePop();
            if (!isPrimitiveIntegerType(typeStackTop)) {
                transpilerError("'" + type + "' cannot be indexed with '" + typeStackTop + "'", i);
                errors.push_back(err);
                return;
            }
            typePop;
            typeStack.push_back(type.substr(1, type.size() - 2));
            append("scale_int index = scale_pop(scale_int);\n");
            append("scale_array_check_bounds_or_throw((scale_any*) tmp, index);\n");
            append("scale_push(%s, tmp[index]);\n", sclTypeToCType(result, typeStackTop).c_str());
            scopeDepth--;
            append("}\n");
        } else if (hasMethod(result, "[]", typeStackTop)) {
            Method* m = getMethodByName(result, "[]", typeStackTop);
            std::string type = typeStackTop;
            if (m->args.size() != 2) {
                transpilerError("Method '[]' of type '" + type + "' must have exactly 1 argument! Signature should be: '[](index: " + m->args[0].type + ")'", i);
                errors.push_back(err);
                return;
            }
            if (removeTypeModifiers(m->return_type) == "none" || removeTypeModifiers(m->return_type) == "nothing") {
                transpilerError("Method '[]' of type '" + type + "' must return a value", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            scopeDepth++;
            append("scale_any instance = scale_pop(scale_any);\n");
            std::string indexType = typeStackTop;
            typePop;

            varScopePush();
            safeInc();
            if (body[i].type == tok_bracket_close) {
                transpilerError("Empty array subscript", i);
                errors.push_back(err);
                return;
            }
            while (body[i].type != tok_bracket_close) {
                handle(Token);
                safeInc();
            }
            varScopePop();
            if (!typeEquals(removeTypeModifiers(typeStackTop), removeTypeModifiers(m->args[0].type))) {
                transpilerError("'" + m->member_type + "' cannot be indexed with '" + removeTypeModifiers(typeStackTop) + "'", i);
                errors.push_back(err);
                return;
            }
            append("scale_push(scale_any, instance);\n");
            typeStack.push_back(type);
            methodCall(m, fp, result, warns, errors, body, i);
            scopeDepth--;
            append("}\n");
        } else {
            transpilerError("'" + type + "' cannot be indexed", i);
            errors.push_back(err);
        }
    }
} // namespace sclc
