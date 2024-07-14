
#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(If) {
        auto beginningStackSize = typeStack.size();
        noUnused;
        append("if (({\n");
        scopeDepth++;
        safeInc();
        size_t typeStackSize = typeStack.size();
        varScopePush();
        while (body[i].type != tok_then) {
            handle(Token);
            safeInc();
        }
        varScopePop();
        varScopePush();
        safeInc();
        size_t diff = typeStack.size() - typeStackSize;
        if (diff > 1) {
            transpilerError("More than one value in if-condition. Maybe you forgot to use '&&' or '||'?", i);
            warns.push_back(err);
        }
        if (diff == 0) {
            transpilerError("Expected one value in if-condition", i);
            errors.push_back(err);
            return;
        }
        append("_scl_pop(scl_int);\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        typePop;
        auto tokenEndsIfBlock = [body](TokenType type) {
            return type == tok_elif ||
                   type == tok_elunless ||
                   type == tok_fi ||
                   type == tok_else;
        };
        bool exhaustive = false;
        std::vector<std::string> returnedTypes;
    nextIfPart:
        while (!tokenEndsIfBlock(body[i].type)) {
            handle(Token);
            safeInc();
        }
        if (typeStack.size() > beginningStackSize) {
            returnedTypes.push_back(typeStackTop);
        }
        while (typeStack.size() > beginningStackSize) {
            typePop;
        }
        if (body[i].type != tok_fi) {
            if (body[i].type == tok_else) {
                exhaustive = true;
            }
            handle(Token);
            safeInc();
            goto nextIfPart;
        }
        while (typeStack.size() > beginningStackSize) {
            typePop;
        }
        if (exhaustive && returnedTypes.size() > 0) {
            std::string returnType = returnedTypes[0];
            for (std::string& type : returnedTypes) {
                if (type != returnType) {
                    transpilerError("Not every branch of this if-statement returns the same type! Types are: [ " + returnType + ", " + type + " ]", i);
                    errors.push_back(err);
                    return;
                }
            }
            typeStack.push_back(returnType);
        }
        handle(Fi);
    }
} // namespace sclc

