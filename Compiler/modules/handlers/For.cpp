#include <gc/gc_allocator.h>

#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(For) {
        noUnused;
        safeInc();
        Token var = body[i];
        if (var.type != tok_identifier) {
            transpilerError("Expected identifier, but got: '" + var.value + "'", i);
            errors.push_back(err);
        }
        safeInc();
        if (body[i].type != tok_in) {
            transpilerError("Expected 'in' keyword in for loop header, but got: '" + body[i].value + "'", i);
            errors.push_back(err);
        }
        safeInc();
        std::string var_prefix = "";
        Variable v = Variable::emptyVar();

        if (hasVar(var.value)) {
            v = getVar(var.value);
        } else {
            v = Variable(var.value, "int");
            var_prefix = "scl_int ";
        }
        
        append("for (%sVar_%s = ({\n", var_prefix.c_str(), var.value.c_str());
        scopeDepth++;
        while (body[i].type != tok_to) {
            handle(Token);
            safeInc();
        }
        if (body[i].type != tok_to) {
            transpilerError("Expected 'to', but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        safeInc();
        append("_scl_pop(%s);\n", sclTypeToCType(result, v.type).c_str());
        typePop;
        scopeDepth--;
        append("}); Var_%s != ({\n", var.value.c_str());
        scopeDepth++;
        while (body[i].type != tok_step && body[i].type != tok_do) {
            handle(Token);
            safeInc();
        }
        typePop;
        append("_scl_pop(%s);\n", sclTypeToCType(result, v.type).c_str());
        scopeDepth--;
        

        if (body[i].type == tok_step) {
            safeInc();
            append("}); ({\n");
            scopeDepth++;
            append("_scl_push(%s, Var_%s);\n", sclTypeToCType(result, v.type).c_str(), var.value.c_str());
            typeStack.push_back(v.type);
            while (body[i].type != tok_do) {
                handle(Token);
                safeInc();
            }
            std::string type = removeTypeModifiers(typeStackTop);
            if (!typesCompatible(result, type, v.type, true)) {
                transpilerError("Incompatible types: '" + type + "' and '" + v.type + "'", i);
                errors.push_back(err);
                return;
            }
            typePop;
            append("Var_%s = _scl_pop(%s);\n", var.value.c_str(), sclTypeToCType(result, v.type).c_str());
            scopeDepth--;
            append("})) {\n");
        } else {
            append("}); Var_%s++) {\n", var.value.c_str());
        }
        if (body[i].type != tok_do) {
            transpilerError("Expected 'do', but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        varScopePush();
        if (!hasVar(var.value)) {
            vars.push_back(v);
            checkShadow(var.value, body[i], function, result, warns);
        }
        
        iterator_count++;
        scopeDepth++;
        append("_scl_scope(128*sizeof(scl_int));\n");

        pushOther();
    }
} // namespace sclc

