#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"
#include "../../headers/Functions.hpp"

namespace sclc {
    handler(Using) {
        safeInc();
        if (body[i].type == tok_struct_def) {
            std::string file = body[i].location.file;
            safeInc();
            FPResult res = parseType(body, &i, getTemplates(result, function));
            if (!res.success) {
                errors.push_back(res);
                return;
            }
            std::string type = res.value;
            if (usingStructs.find(file) == usingStructs.end()) {
                usingStructs[file] = std::vector<std::string>();
            }
            usingStructs[file].push_back(type);
            return;
        }

        append("{\n");
        scopeDepth++;
        pushOther();
        varScopePush();
        size_t stackSize = typeStack.size();
        while (body[i].type != tok_do) {
            handle(Token);
            safeInc();
        }
        size_t stackEnd = typeStack.size();
        if (stackSize + 1 != stackEnd) {
            transpilerError("Expected expression evaluating to a value after 'using'", i);
            errors.push_back(err);
            return;
        }
        std::string type = typeStackTop;
        typePop;

        std::string name = "it";

        if (i + 1 < body.size() && body[i + 1].type == tok_paren_open && body[i + 1].location.line == body[i].location.line) {
            safeIncN(2);
            if (body[i].type != tok_identifier) {
                transpilerError("Expected identifier after '('", i);
                errors.push_back(err);
                return;
            }
            name = body[i].value;
            safeInc();
            if (body[i].type != tok_paren_close) {
                transpilerError("Expected ')' after identifier", i);
                errors.push_back(err);
                return;
            }
        }
        checkShadow(name, body, i, function, result, warns);
        vars.push_back(Variable(name, type));
        type = sclTypeToCType(result, type);
        append("%s Var_%s = _scl_pop(%s);\n", type.c_str(), name.c_str(), type.c_str());
        usingNames.push_back(name);
        popOther();
        pushUsing();
    }
} // namespace sclc

