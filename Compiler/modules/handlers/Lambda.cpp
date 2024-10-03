
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(Lambda) {
        noUnused;
        Function* f;
        safeInc();
        std::string name = "";
        Token nameTok;
        bool nameSet = false;
        if (body[i].type == tok_identifier) {
            name = body[i].value;
            safeInc();
            nameSet = true;
            nameTok = body[i];
        } else {
            name = std::to_string(lambdaCount++);
        }
        if (function->isMethod) {
            f = new Function("$lambda$" + name + "$" + function->member_type + "$$" + function->name, body[i]);
        } else {
            f = new Function("$lambda$" + name + "$" + function->name, body[i]);
        }
        f->container = function;
        f->lambdaName = name;
        f->addModifier("<lambda>");
        f->return_type = "none";
        if (body[i].type == tok_bracket_open) {
            while (body[i].type != tok_bracket_close) {
                safeInc();
                if (body[i].type == tok_addr_ref) {
                    safeInc();
                    f->ref_captures.push_back(getVar(body[i].value));
                } else {
                    f->captures.push_back(getVar(body[i].value));
                }
                safeInc();
                if (body[i].type != tok_comma && body[i].type != tok_bracket_close) {
                    transpilerError("Expected comma or ']', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
            }
            safeInc();
        }
        if (body[i].type == tok_paren_open) {
            safeInc();
            while (i < body.size() && body[i].type != tok_paren_close) {
                if (body[i].type == tok_identifier || body[i].type == tok_column) {
                    std::string name = body[i].value;
                    std::string type = "any";
                    if (body[i].type == tok_column) {
                        name = "";
                    } else {
                        safeInc();
                    }
                    if (body[i].type == tok_column) {
                        safeInc();
                        FPResult r = parseType(body, i);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        if (type == "none" || type == "nothing") {
                            transpilerError("Type 'none' is only valid for function return types.", i);
                            errors.push_back(err);
                            continue;
                        }
                    } else {
                        transpilerError("A type is required", i);
                        errors.push_back(err);
                        safeInc();
                        continue;
                    }
                    if (type == "varargs") {
                        transpilerError("Lambdas may not take variadic arguments!", i);
                        errors.push_back(err);
                    }
                    f->addArgument(Variable(name, type));
                } else {
                    transpilerError("Expected identifier for argument name, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    safeInc();
                    continue;
                }
                safeInc();
                if (body[i].type == tok_comma || body[i].type == tok_paren_close) {
                    if (body[i].type == tok_comma) {
                        safeInc();
                    }
                    continue;
                }
                transpilerError("Expected ',' or ')', but got '" + body[i].value + "'", i);
                errors.push_back(err);
                continue;
            }
            safeInc();
        }
        if (body[i].type == tok_column) {
            safeInc();
            FPResult r = parseType(body, i);
            if (!r.success) {
                errors.push_back(r);
                return;
            }
            std::string type = r.value;
            f->return_type = type;
            safeInc();
        }
        int lambdaDepth = 0;
        while (true) {
            if (body[i].type == tok_end && lambdaDepth == 0) {
                break;
            }
            if (body[i].type == tok_lambda) {
                if (((ssize_t) i) - 1 < 0) {
                    lambdaDepth++;
                } else if (body[i - 1].type != tok_as && body[i - 1].type != tok_column && body[i - 1].type != tok_bracket_open) {
                    lambdaDepth++;
                } else if (((ssize_t) i) - 2 < 0) {
                    lambdaDepth++;
                } else if (body[i - 2].type != tok_new) {
                    lambdaDepth++;
                }
            }
            if (body[i].type == tok_end) {
                lambdaDepth--;
            }
            f->addToken(body[i]);
            safeInc();
        }
        f->args.push_back(Variable("$data", "any"));

        std::string arguments = "";
        if (f->isMethod) {
            arguments = sclTypeToCType(result, f->member_type) + " Var_self";
            for (size_t i = 0; i < f->args.size() - 1; i++) {
                std::string type = sclTypeToCType(result, f->args[i].type);
                arguments += ", " + type;
                if (type == "varargs" || type == "...") continue;
                if (f->args[i].name.size())
                    arguments += " Var_" + f->args[i].name;
            }
        } else {
            if (f->args.size() > 0) {
                for (size_t i = 0; i < f->args.size(); i++) {
                    std::string type = sclTypeToCType(result, f->args[i].type);
                    if (i) {
                        arguments += ", ";
                    }
                    arguments += type;
                    if (type == "varargs" || type == "...") continue;
                    if (f->args[i].name.size())
                        arguments += " Var_" + f->args[i].name;
                }
            }
        }

        std::string lambdaType = "lambda(";
        for (size_t i = 0; i < f->args.size() - 1; i++) {
            if (i) lambdaType += ",";
            lambdaType += f->args[i].type;
        }
        lambdaType += "):" + f->return_type;

        append("scale_push(scale_any, ({\n");
        scopeDepth++;
        const std::string sym = generateSymbolForFunction(f);
        append("%s fn_$%s$%s(%s)", sclTypeToCType(result, f->return_type).c_str(), name.c_str(), function->name.c_str(), arguments.c_str());
        append2(" SYMBOL(%s);\n", sym.c_str());
        append("struct l$%s$%s {\n", name.c_str(), function->name.c_str());
        append("  scale_any func;\n");
        for (size_t i = 0; i < f->captures.size(); i++) {
            append("  %s cap_%s;\n", sclTypeToCType(result, f->captures[i].type).c_str(), f->captures[i].name.c_str());
            f->modifiers.push_back(f->captures[i].type);
            f->modifiers.push_back(f->captures[i].name);
        }
        for (size_t i = 0; i < f->ref_captures.size(); i++) {
            append("  %s* ref_%s;\n", sclTypeToCType(result, f->ref_captures[i].type).c_str(), f->ref_captures[i].name.c_str());
            f->modifiers.push_back(f->ref_captures[i].type);
            f->modifiers.push_back(f->ref_captures[i].name);
        }
        append("}* tmp = (typeof(tmp)) scale_alloc(sizeof(*tmp));\n");
        for (size_t i = 0; i < f->captures.size(); i++) {
            append("tmp->cap_%s = Var_%s;\n", f->captures[i].name.c_str(), f->captures[i].name.c_str());
        }
        for (size_t i = 0; i < f->ref_captures.size(); i++) {
            append("tmp->ref_%s = &(Var_%s);\n", f->ref_captures[i].name.c_str(), f->ref_captures[i].name.c_str());
        }
        append("tmp->func = fn_$%s$%s;\n", name.c_str(), function->name.c_str());
        append("(scale_any) tmp;\n");
        scopeDepth--;
        append("}));\n");

        typeStack.push_back(lambdaType);
        result.functions.push_back(f);

        if (nameSet) {
            Variable v(name, typeStackTop);
            vars.push_back(v);
            std::string cType = sclTypeToCType(result, typeStackTop);
            append("%s Var_%s = scale_pop(%s);\n", cType.c_str(), name.c_str(), cType.c_str());
            typeStack.pop_back();
        }
    }
} // namespace sclc
