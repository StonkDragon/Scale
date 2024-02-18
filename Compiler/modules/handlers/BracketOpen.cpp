#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

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
                    if (Main::options::debugBuild) append("_scl_array_check_bounds_or_throw(_scl_top(scl_any*), %s);\n", body[i - 1].value.c_str());
                    typePop;
                    typeStack.push_back(type.substr(1, type.size() - 2));
                    if (isPrimitiveIntegerType(typeStackTop)) {
                        append("_scl_top(scl_int) = _scl_top(%s)[%s];\n", sclTypeToCType(result, type).c_str(), body[i - 1].value.c_str());
                    } else {
                        append("_scl_top(%s) = _scl_top(%s)[%s];\n", sclTypeToCType(result, typeStackTop).c_str(), sclTypeToCType(result, type).c_str(), body[i - 1].value.c_str());
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
            append("%s tmp = _scl_pop(%s);\n", sclTypeToCType(result, type).c_str(), sclTypeToCType(result, type).c_str());
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
            append("scl_int index = _scl_pop(scl_int);\n");
            if (Main::options::debugBuild) append("_scl_array_check_bounds_or_throw((scl_any*) tmp, index);\n");
            if (isPrimitiveIntegerType(typeStackTop)) {
                append("_scl_push(scl_int, tmp[index]);\n");
            } else {
                append("_scl_push(%s, tmp[index]);\n", sclTypeToCType(result, typeStackTop).c_str());
            }
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
            append("scl_any instance = _scl_pop(scl_any);\n");
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
            append("_scl_push(scl_any, instance);\n");
            typeStack.push_back(type);
            methodCall(m, fp, result, warns, errors, body, i);
            scopeDepth--;
            append("}\n");
        } else if (i + 1 < body.size()) {
            bool isListExpression = false;
            auto start = i;
            int bracketDepth = 0;
            while (body[i].type != tok_bracket_close || bracketDepth > 0) {
                if (body[i].type == tok_bracket_open) bracketDepth++;
                if (body[i].type == tok_bracket_close) bracketDepth--;
                if (body[i].type == tok_do) {
                    isListExpression = true;
                    break;
                }
                safeInc();
            }
            i = start;
            if (isListExpression) {
                safeInc();
                if (body[i].type == tok_paren_open) {
                    transpilerError("Legacy list expression syntax", i);
                    warns.push_back(err);
                }
                append("{\n");
                scopeDepth++;
                bool existingArrayUsed = false;
                std::string iterator_var_name = "i";
                std::string value_var_name = "val";
                if (body[i].type == tok_number && body[i + 1].type == tok_do) {
                    long long array_size = std::stoll(body[i].value);
                    if (array_size < 1) {
                        transpilerError("Array size must be positive", i);
                        errors.push_back(err);
                        return;
                    }
                    append("scl_int array_size = %s;\n", body[i].value.c_str());
                    safeIncN(2);
                } else {
                    while (body[i].type != tok_do) {
                        handle(Token);
                        safeInc();
                    }
                    std::string top = removeTypeModifiers(typeStackTop);
                    if (top.size() > 2 && top.front() == '[' && top.back() == ']') {
                        existingArrayUsed = true;
                    }
                    if (!isPrimitiveIntegerType(typeStackTop) && !existingArrayUsed) {
                        transpilerError("Array size must be an integer", i);
                        errors.push_back(err);
                        return;
                    }
                    if (existingArrayUsed) {
                        append("scl_int array_size = _scl_array_size(_scl_top(scl_any*));\n");
                    } else {
                        append("scl_int array_size = _scl_pop(scl_int);\n");
                        typePop;
                    }
                    safeInc();
                }
                if (body[i].type == tok_as) {
                    safeInc();
                    if (body[i].type != tok_paren_open) {
                        transpilerError("Expected '(' after 'as'", i);
                        errors.push_back(err);
                        return;
                    }
                    safeInc();
                    if (body[i].type != tok_identifier) {
                        transpilerError("Expected identifier for index variable name", i);
                        errors.push_back(err);
                        return;
                    }
                    iterator_var_name = body[i].value == "_" ? "" : body[i].value;
                    safeInc();
                    if (body[i].type == tok_comma) {
                        safeInc();
                        if (body[i].type != tok_identifier) {
                            transpilerError("Expected identifier for value variable name", i);
                            errors.push_back(err);
                            return;
                        }
                        value_var_name = body[i].value == "_" ? "" : body[i].value;
                        safeInc();
                    }
                    if (body[i].type != tok_paren_close) {
                        transpilerError("Expected ',' or ')' after index variable name", i);
                        errors.push_back(err);
                        return;
                    }
                    safeInc();
                }
                std::string arrayType = "[int]";
                std::string elementType = "int";
                if (existingArrayUsed) {
                    arrayType = removeTypeModifiers(typeStackTop);
                    typePop;
                    if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                        elementType = arrayType.substr(1, arrayType.size() - 2);
                    } else {
                        elementType = "any";
                    }
                    append("%s* array = _scl_pop(%s*);\n", sclTypeToCType(result, elementType).c_str(), sclTypeToCType(result, elementType).c_str());
                } else {
                    append("scl_int* array = _scl_new_array_by_size(array_size, sizeof(%s));\n", sclTypeToCType(result, elementType).c_str());
                }
                append("for (scl_int i = 0; i < array_size; i++) {\n");
                scopeDepth++;
                varScopePush();
                if (!iterator_var_name.empty()) {
                    append("const scl_int Var_%s = i;\n", iterator_var_name.c_str());
                    vars.push_back(Variable(iterator_var_name, "const int"));
                }
                size_t typeStackSize = typeStack.size();
                if (existingArrayUsed && !value_var_name.empty()) {
                    append("const %s Var_%s = array[i];\n", sclTypeToCType(result, elementType).c_str(), value_var_name.c_str());
                    vars.push_back(Variable(value_var_name, "const " + elementType));
                }
                while (body[i].type != tok_bracket_close) {
                    handle(Token);
                    safeInc();
                }
                size_t diff = typeStack.size() - typeStackSize;
                if (diff > 1) {
                    transpilerError("Expected less than 2 values on the type stack, but got " + std::to_string(typeStackSize - typeStack.size()), i);
                    errors.push_back(err);
                    return;
                }
                std::string type = removeTypeModifiers(typeStackTop);
                for (size_t i = typeStackSize; i < typeStack.size(); i++) {
                    typePop;
                }
                varScopePop();
                if (diff) {
                    append("array[i] = _scl_pop(scl_int);\n");
                }
                scopeDepth--;
                append("}\n");
                append("_scl_push(scl_any, array);\n");
                typeStack.push_back(arrayType);
                scopeDepth--;
                append("}\n");
                if (body[i].type != tok_bracket_close) {
                    transpilerError("Expected ']', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
            }
        } else {
            transpilerError("'" + type + "' cannot be indexed", i);
            errors.push_back(err);
        }
    }
} // namespace sclc
