#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Foreach) {
        noUnused;
        safeInc();
        if (body[i].type != tok_identifier && body[i].type != tok_paren_open) {
            transpilerError("Expected identifier or '(', but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        std::string iterator_name = "__it" + std::to_string(iterator_count++);
        Token iter_var_tok;
        std::string iterType = "";
        Token index_var_tok;
        if (body[i].type == tok_paren_open) {
            // (x, i)
            safeInc();
            if (body[i].type != tok_identifier) {
                transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            iter_var_tok = body[i];
            safeInc();
            if (body[i].type == tok_column) {
                safeInc();
                FPResult type = parseType(body, &i);
                if (!type.success) {
                    errors.push_back(type);
                    return;
                }
                iterType = removeTypeModifiers(type.value);
                safeInc();
            }
            if (body[i].type == tok_comma) {
                safeInc();
                if (body[i].type != tok_identifier) {
                    transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                index_var_tok = body[i];
                safeInc();
            }
            if (body[i].type != tok_paren_close) {
                transpilerError("Expected ')', but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            safeInc();
        } else {
            iter_var_tok = body[i];
            checkShadow(iter_var_tok.value, body, i, function, result, warns);
            safeInc();
            if (body[i].type == tok_column) {
                safeInc();
                FPResult type = parseType(body, &i);
                if (!type.success) {
                    errors.push_back(type);
                    return;
                }
                iterType = removeTypeModifiers(type.value);
                safeInc();
            }
        }
        if (body[i].type != tok_in) {
            transpilerError("Expected 'in', but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        safeInc();
        if (body[i].type == tok_do) {
            transpilerError("Expected iterable, but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        while (body[i].type != tok_do) {
            handle(Token);
            safeInc();
        }
        pushIterate();

        std::string type = removeTypeModifiers(typeStackTop);
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            append("%s %s = _scl_pop(%s);\n", sclTypeToCType(result, typeStackTop).c_str(), iterator_name.c_str(), sclTypeToCType(result, typeStackTop).c_str());
            typePop;
            append("for (scl_int i = 0; i < _scl_array_size(%s); i++) {\n", iterator_name.c_str());
            scopeDepth++;

            varScopePush();
            vars.push_back(Variable(iter_var_tok.value, type.substr(1, type.size() - 2)));
            bool typeSpecified = true;
            if (!iterType.size()) {
                typeSpecified = false;
                iterType = type.substr(1, type.size() - 2);
            }
            append("%s Var_%s = %s[i];\n", sclTypeToCType(result, iterType).c_str(), iter_var_tok.value.c_str(), iterator_name.c_str());
            if (index_var_tok.value.size()) {
                vars.push_back(Variable(index_var_tok.value, "const int"));
                append("scl_int Var_%s = i;\n", index_var_tok.value.c_str());
            }
            if (typeSpecified) {
                append("_scl_checked_cast(Var_%s, 0x%lxUL, \"%s\");\n", iter_var_tok.value.c_str(), id(iterType.c_str()), iterType.c_str());
            }
            pushOther();
            return;
        }
        
        Struct s = getStructByName(result, typeStackTop);
        if (s == Struct::Null) {
            transpilerError("Can only iterate over structs and arrays, but got '" + typeStackTop + "'", i);
            errors.push_back(err);
            return;
        }
        if (!structImplements(result, s, "Iterable")) {
            transpilerError("Struct '" + typeStackTop + "' is not iterable", i);
            errors.push_back(err);
            return;
        }
        Method* iterateMethod = getMethodByName(result, "iterate", typeStackTop);
        if (iterateMethod == nullptr) {
            transpilerError("Could not find method 'iterate' on type '" + typeStackTop + "'", i);
            errors.push_back(err);
            return;
        }
        methodCall(iterateMethod, fp, result, warns, errors, body, i);
        append("%s %s = _scl_pop(%s);\n", sclTypeToCType(result, typeStackTop).c_str(), iterator_name.c_str(), sclTypeToCType(result, typeStackTop).c_str());
        type = typeStackTop;
        typePop;
        
        Method* nextMethod = getMethodByName(result, "next", type);
        Method* hasNextMethod = getMethodByName(result, "hasNext", type);
        if (hasNextMethod == nullptr) {
            transpilerError("Could not find method 'hasNext' on type '" + type + "'", i);
            errors.push_back(err);
            return;
        }
        if (nextMethod == nullptr) {
            transpilerError("Could not find method 'next' on type '" + type + "'", i);
            errors.push_back(err);
            return;
        }
        varScopePush();
        if (iterType.size()) {
            vars.push_back(Variable(iter_var_tok.value, iterType));
        } else {
            vars.push_back(Variable(iter_var_tok.value, nextMethod->return_type));
        }
        std::string cType = sclTypeToCType(result, getVar(iter_var_tok.value).type);
        if (index_var_tok.value.size()) {
            vars.push_back(Variable(index_var_tok.value, "const int"));
            append("scl_int %s_ind = 0;\n", iterator_name.c_str());
        }
        std::string iteratingType = sclTypeToCType(result, type);
        append("scl_int(*hasNext_%s)(%s) = func_ptr_on(%s, \"hasNext()i;\");\n", iterator_name.c_str(), iteratingType.c_str(), iterator_name.c_str());
        append("%s(*next_%s)(%s) = func_ptr_on(%s, \"next\");\n", cType.c_str(), iterator_name.c_str(), iteratingType.c_str(), iterator_name.c_str());
        append("while (hasNext_%s(%s)) {\n", iterator_name.c_str(), iterator_name.c_str());
        scopeDepth++;
        append("%s Var_%s = next_%s(%s);\n", cType.c_str(), iter_var_tok.value.c_str(), iterator_name.c_str(), iterator_name.c_str());
        if (iterType.size()) {
            append("_scl_checked_cast(Var_%s, 0x%lxUL, \"%s\");\n", iter_var_tok.value.c_str(), id(iterType.c_str()), iterType.c_str());
        }
        if (index_var_tok.value.size()) {
            append("scl_int Var_%s = %s_ind++;\n", index_var_tok.value.c_str(), iterator_name.c_str());
        }
    }
} // namespace sclc

