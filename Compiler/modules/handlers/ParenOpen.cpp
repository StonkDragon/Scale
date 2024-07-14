
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(ParenOpen) {
        noUnused;
        int commas = 0;
        bool isRange = false;
        auto stackSizeHere = typeStack.size();
        safeInc();
        append("{\n");
        scopeDepth++;
        append("scl_uint64* begin_stack_size = _local_stack_ptr;\n");
        while (body[i].type != tok_paren_close) {
            if (body[i].type == tok_comma) {
                commas++;
            } else if (body[i].type == tok_to) {
                isRange = true;
            } else {
                handle(Token);
            }
            safeInc();
        }

        if (commas == 0) {
            if (isRange) {
                // Range expression
                size_t nelems = typeStack.size() - stackSizeHere;
                if (nelems == 2) {
                    Struct range = getStructByName(result, "Range");
                    if (range == Struct::Null) {
                        transpilerError("Struct definition for 'Range' not found", i);
                        errors.push_back(err);
                        return;
                    }
                    append("{\n");
                    scopeDepth++;
                    append("scl_Range tmp = ALLOC(Range);\n");
                    append("_scl_push(scl_Range, tmp);\n");
                    typeStack.push_back("Range");
                    methodCall(getMethodByName(result, "init", "Range"), fp, result, warns, errors, body, i);
                    append("_scl_push(scl_Range, tmp);\n");
                    typeStack.push_back("Range");
                    scopeDepth--;
                    append("}\n");
                } else if (nelems == 1) {
                    Struct range = getStructByName(result, "PartialRange");
                    if (range == Struct::Null) {
                        transpilerError("Struct definition for 'PartialRange' not found", i);
                        errors.push_back(err);
                        return;
                    }
                    Function* f = nullptr;
                    if (body[i - 1].type == tok_to) {
                        f = getFunctionByName(result, "PartialRange$lowerBound");
                    } else {
                        f = getFunctionByName(result, "PartialRange$upperBound");
                    }
                    if (f == nullptr) {
                        transpilerError(std::string("Function definition for '") + (body[i - 1].type == tok_to ? "PartialRange::lowerBound" : "PartialRange::upperBound") + "' not found", i);
                        errors.push_back(err);
                        return;
                    }
                    functionCall(f, fp, result, warns, errors, body, i);
                } else if (nelems == 0) {
                    Struct range = getStructByName(result, "UnboundRange");
                    if (range == Struct::Null) {
                        transpilerError("Struct definition for 'UnboundRange' not found", i);
                        errors.push_back(err);
                        return;
                    }
                    append("{\n");
                    scopeDepth++;
                    append("scl_UnboundRange tmp = ALLOC(UnboundRange);\n");
                    append("_scl_push(scl_UnboundRange, tmp);\n");
                    typeStack.push_back("UnboundRange");
                    methodCall(getMethodByName(result, "init", "UnboundRange"), fp, result, warns, errors, body, i);
                    append("_scl_push(scl_UnboundRange, tmp);\n");
                    typeStack.push_back("UnboundRange");
                    scopeDepth--;
                    append("}\n");
                } else {
                    transpilerError("Range expression must have between 0 and 2 elements", i);
                    errors.push_back(err);
                }
            } else {

                // Last-returns expression
                if (typeStack.size() > stackSizeHere) {
                    std::string returns = typeStackTop;
                    std::string ctype = sclTypeToCType(result, returns);
                    append("%s return_value = _scl_pop(%s);\n", ctype.c_str(), ctype.c_str());
                    append("_local_stack_ptr = begin_stack_size;\n");
                    typeStack.erase(typeStack.begin() + stackSizeHere, typeStack.end());
                    append("_scl_push(%s, return_value);\n", ctype.c_str());
                    typeStack.push_back(returns);
                }
            }
        } else if (commas == 1) {
            Struct pair = getStructByName(result, "Pair");
            if (pair == Struct::Null) {
                transpilerError("Struct definition for 'Pair' not found", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            scopeDepth++;
            append("scl_Pair tmp = ALLOC(Pair);\n");
            append("_scl_push(scl_Pair, tmp);\n");
            typeStack.push_back("Pair");
            methodCall(getMethodByName(result, "init", "Pair"), fp, result, warns, errors, body, i);
            append("_scl_push(scl_Pair, tmp);\n");
            typeStack.push_back("Pair");
            scopeDepth--;
            append("}\n");
        } else if (commas == 2) {
            Struct triple = getStructByName(result, "Triple");
            if (getStructByName(result, "Triple") == Struct::Null) {
                transpilerError("Struct definition for 'Triple' not found", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            scopeDepth++;
            append("scl_Triple tmp = ALLOC(Triple);\n");
            append("_scl_push(scl_Triple, tmp);\n");
            typeStack.push_back("Triple");
            methodCall(getMethodByName(result, "init", "Triple"), fp, result, warns, errors, body, i);
            append("_scl_push(scl_Triple, tmp);\n");
            typeStack.push_back("Triple");
            scopeDepth--;
            append("}\n");
        } else {
            transpilerError("Unsupported tuple-like literal", i);
            errors.push_back(err);
        }
        scopeDepth--;
        append("}\n");
    }
} // namespace sclc

