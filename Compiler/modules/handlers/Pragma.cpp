
#include <sstream>

#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(Pragma) {
        noUnused;
        safeInc();
        if (body[i].value == "note" || body[i].value == "warn" || body[i].value == "error") {
            safeInc();
            if (body[i].type != tok_string_literal) {
                transpilerError("Expected string literal after '" + body[i - 1].value + "'", i);
                errors.push_back(err);
                return;
            }
            std::string type = body[i - 1].value;
            std::string message = body[i].value;
            transpilerError(message, i - 2);
            if (body[i - 1].value == "note") {
                err.isNote = true;
            }
            if (body[i - 1].value == "warn") {
                warns.push_back(err);
            } else {
                errors.push_back(err);
            }
        } else if (body[i].value == "generic") {
            safeInc();
            if (!hasVar(body[i].value)) {
                transpilerError("Unknown variable: '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            std::string type = getVar(body[i].value).type;
            safeInc();
            if (body[i].type != tok_do) {
                transpilerError("Expected 'do', but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            safeInc();
            bool filledIn = false;
            while (body[i].type != tok_done) {
                if (body[i].type == tok_default) {
                    safeInc();
                    if (body[i].type != tok_store) {
                        transpilerError("Expected '=>', but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    safeInc();
                    if (!filledIn) {
                        handle(Token);
                        filledIn = true;
                    } else {
                        std::ostringstream fp;
                        handle(Token);
                        fp.flush();
                    }
                } else {
                    FPResult r = parseType(body, i);
                    if (!r.success) {
                        errors.push_back(r);
                        return;
                    }
                    safeInc();
                    if (body[i].type != tok_store) {
                        transpilerError("Expected '=>', but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    safeInc();
                    
                    if (!filledIn && r.value == type) {
                        handle(Token);
                        filledIn = true;
                    } else {
                        std::ostringstream fp;
                        handle(Token);
                        fp.flush();
                    }
                }
                safeInc();
            }

            if (body[i].type != tok_done) {
                transpilerError("Expected 'done', but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
        } else {
            transpilerError("Unknown pragma '" + body[i].value + "'", i);
            errors.push_back(err);
        }
    }
} // namespace sclc

