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
        if (!hasVar(var.value)) {
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
        append("_scl_pop(scl_int);\n");
        typePop;
        scopeDepth--;
        append("}); Var_%s != ({\n", var.value.c_str());
        scopeDepth++;
        while (body[i].type != tok_step && body[i].type != tok_do) {
            handle(Token);
            safeInc();
        }
        typePop;
        append("_scl_pop(scl_int);\n");
        scopeDepth--;
        
        Variable v = Variable::emptyVar();

        if (hasVar(var.value)) {
            v = getVar(var.value);
        } else {
            v = Variable(var.value, "int");
        }

        if (body[i].type == tok_step) {
            safeInc();
            append("}); ({\n");
            scopeDepth++;
            append("_scl_push(scl_int, Var_%s);\n", var.value.c_str());
            typeStack.push("int");
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
            append("Var_%s = _scl_pop(scl_int);\n", var.value.c_str());
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
            varScopeTop().push_back(v);
            checkShadow(var.value, body, i, function, result, warns);
        }
        
        iterator_count++;
        scopeDepth++;

        pushOther();
    }
} // namespace sclc

