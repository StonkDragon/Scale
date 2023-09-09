#include <filesystem>
#include <stack>
#include <functional>
#include <csignal>
#include <cstdlib>
#include <unistd.h>

#include "../headers/Common.hpp"
#include "../headers/TranspilerDefs.hpp"
#include "../headers/Types.hpp"
#include "../headers/Functions.hpp"

#ifndef VERSION
#define VERSION ""
#endif

namespace sclc {
    extern std::vector<std::string> cflags;

    Function* currentFunction;
    Struct currentStruct("");
    std::map<std::string, std::vector<Method*>> vtables;
    FILE* scale_header = NULL;
    StructTreeNode* structTree;
    int scopeDepth = 0;
    size_t i = 0;
    size_t condCount = 0;
    std::vector<char> whatWasIt;
    std::vector<std::string> switchTypes;
    char repeat_depth = 0;
    int iterator_count = 0;
    int isInUnsafe = 0;
    std::stack<std::string> typeStack;
    std::string return_type = "";
    int lambdaCount = 0;

    void ConvertC::writeHeader(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;

        append("#include <scale_runtime.h>\n");
        for (std::string& header : Main::frameworkNativeHeaders) {
            append("#include <%s>\n", header.c_str());
        }
        append("\n");
    }

    void ConvertC::writeFunctionHeaders(FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        append("/* FUNCTION HEADERS */\n");

        for (Function* function : result.functions) {
            std::string return_type = sclTypeToCType(result, function->return_type);
            auto args = function->args;
            std::string arguments;
            arguments.reserve(32);
            
            if (function->isMethod) {
                currentStruct = getStructByName(result, function->member_type);
                arguments += sclTypeToCType(result, function->member_type) + " Var_self";
                for (long i = 0; i < (long) args.size() - 1; i++) {
                    std::string type = sclTypeToCType(result, args[i].type);
                    arguments += ", " + type;
                    if (type == "varargs" || type == "...") continue;
                    if (args[i].name.size())
                        arguments += " Var_" + args[i].name;
                }
            } else {
                currentStruct = Struct::Null;
                if (!Main::options::noMain && function->name == "main") {
                    arguments = "int __argc, char** __argv";
                } else if (args.size() > 0) {
                    for (long i = 0; i < (long) args.size(); i++) {
                        std::string type = sclTypeToCType(result, args[i].type);
                        if (i) {
                            arguments += ", ";
                        }
                        arguments += type;
                        if (type == "varargs" || type == "...") continue;
                        if (args[i].name.size())
                            arguments += " Var_" + args[i].name;
                    }
                }
            }

            if (arguments.empty()) {
                arguments = "void";
            }

            std::string symbol = generateSymbolForFunction(function);

            if (!function->isMethod) {
                if (function->name == "throw" || function->name == "builtinUnreachable") {
                    append("_scl_no_return ");
                }
                if (!Main::options::noMain && function->name == "main") {
                    append("int fn_%s(%s)\n", function->finalName().c_str(), arguments.c_str());
                } else {
                    append("%s fn_%s(%s)\n", return_type.c_str(), function->finalName().c_str(), arguments.c_str());
                }
                append("    __asm(%s);\n", symbol.c_str());
                if (function->has_export) {
                    if (function->member_type.size()) {
                        if (hasMethod(result, function->name, function->member_type)) {
                            FPResult res;
                            res.success = false;
                            res.message = "Function '" + function->member_type + "::" + function->name + "' conflicts with method '" + function->member_type + ":" + function->name + "'";
                            res.location = function->name_token.location;
                            res.value = function->name_token.value;
                            res.type = function->name_token.type;
                            errors.push_back(res);
                            continue;
                        }
                    }
                    if (scale_header) {
                        fprintf(scale_header, "extern ");
                        if (function->name == "throw" || function->name == "builtinUnreachable") {
                            fprintf(scale_header, "_scl_no_return ");
                        }
                        fprintf(scale_header, "%s %s(%s) __asm(%s);\n", return_type.c_str(), function->finalName().c_str(), arguments.c_str(), symbol.c_str());
                    }
                }
            } else {
                append("%s mt_%s$%s(%s)\n", return_type.c_str(), function->member_type.c_str(), function->finalName().c_str(), arguments.c_str());
                append("    __asm(%s);\n", symbol.c_str());
                if (function->has_export) {
                    if (hasFunction(result, function->member_type + "$" + function->finalName())) {
                        FPResult res;
                        res.success = false;
                        res.message = "Method '" + function->member_type + ":" + function->name + "' conflicts with function '" + function->member_type + "::" + function->name + "'";
                        res.location = function->name_token.location;
                        res.value = function->name_token.value;
                        res.type = function->name_token.type;
                        errors.push_back(res);
                        continue;
                    }
                    if (scale_header) {
                        fprintf(scale_header, "extern %s %s$%s(%s) __asm(%s);\n", return_type.c_str(), function->member_type.c_str(), function->finalName().c_str(), arguments.c_str(), symbol.c_str());
                    }
                }
            }
        }

        if (scale_header) {
            fprintf(scale_header, "#endif\n");
            fclose(scale_header);
            scale_header = NULL;
        }

        append("\n");
    }

    void ConvertC::writeGlobals(FILE* fp, std::vector<Variable>& globals, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        if (result.globals.size() == 0) return;
        append("/* GLOBALS */\n");

        for (Variable& s : result.globals) {
            append("extern %s Var_%s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
            fprintf(scale_header, "extern %s %s __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) \"Var_%s\");\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str(), s.name.c_str());
            globals.push_back(s);
        }
        for (Variable& s : result.extern_globals) {
            append("extern %s Var_%s __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) \"%s\");\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str(), s.name.c_str());
            fprintf(scale_header, "extern %s %s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
            globals.push_back(s);
        }

        append("\n");
    }

    void ConvertC::writeContainers(FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        for (auto ta : result.typealiases) {
            if (ta.first == "nothing" || ta.first == "varargs") continue;
            std::string type = ta.second;
            std::string name = " ta_" + ta.first;
            if (strcontains(type, "%")) {
                type = replace(type, "%", name);
                name = "";
            }
            append("typedef %s%s;\n", type.c_str(), name.c_str());
        }
        append("\n");
        append("/* STRUCT TYPES */\n");
        for (Struct& c : result.structs) {
            if (c.isStatic() || c.name == "str") continue;
            append("typedef struct Struct_%s* scl_%s;\n", c.name.c_str(), c.name.c_str());
        }
        append("\n");
        for (Layout& c : result.layouts) {
            append("typedef struct Layout_%s* scl_%s;\n", c.name.c_str(), c.name.c_str());
        }
        append("\n");
        for (Layout& c : result.layouts) {
            append("struct Layout_%s {\n", c.name.c_str());
            for (Variable& s : c.members) {
                append("  %s %s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
            }
            append("};\n");
        }
        append("\n");
        if (result.containers.size() == 0) return;
        
        append("/* CONTAINERS */\n");
        for (Container& c : result.containers) {
            append("struct _Container_%s {\n", c.name.c_str());
            for (Variable& s : c.members) {
                append("  %s %s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
            }
            append("};\n");
            append("extern struct _Container_%s Container_%s;\n", c.name.c_str(), c.name.c_str());
        }
    }

    void ConvertC::writeStructs(FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;
        if (result.structs.size() == 0) return;
        append("/* STRUCT DEFINITIONS */\n");

        structTree = StructTreeNode::fromArrayOfStructs(result);

        remove("scale_interop.h");
        scale_header = fopen("scale_interop.h", "a+");
        if (!scale_header) {
            std::cerr << "Could not open scale_interop.h" << std::endl;
            std::raise(SIGSEGV);
        }
        fprintf(scale_header, "#if !defined(SCALE_INTEROP_H)\n");
        fprintf(scale_header, "#define SCALE_INTEROP_H\n");
        fprintf(scale_header, "#include <scale_runtime.h>\n\n");

        for (Struct& c : result.structs) {
            if (c.isStatic() || c.name == "str") continue;
            fprintf(scale_header, "typedef struct Struct_%s* scl_%s;\n", c.name.c_str(), c.name.c_str());
        }
        fprintf(scale_header, "\n");

        for (auto&& ta : result.typealiases) {
            if (ta.first == "nothing" || ta.first == "varargs") continue;
            std::string type = ta.second;
            std::string name = " ta_" + ta.first;
            if (strcontains(type, "%")) {
                type = replace(type, "%", name);
                name = "";
            }
            fprintf(scale_header, "typedef %s%s;\n", type.c_str(), name.c_str());
        }
        fprintf(scale_header, "\n");

        for (Struct& c : result.structs) {
            if (c.isStatic()) continue;
            currentStruct = c;
            for (const std::string& i : c.interfaces) {
                Interface* interface = getInterfaceByName(result, i);
                if (interface == nullptr) {
                    FPResult res;
                    Token t = c.name_token;
                    res.success = false;
                    res.message = "Struct '" + c.name + "' implements unknown interface '" + i + "'";
                    res.location = t.location;
                    res.value = t.value;
                    res.type = t.type;
                    errors.push_back(res);
                    continue;
                }
                for (Function* f : interface->toImplement) {
                    if (!hasMethod(result, f->name, c.name)) {
                        FPResult res;
                        Token t = c.name_token;
                        res.success = false;
                        res.message = "No implementation for method '" + f->name + "' on struct '" + c.name + "'";
                        res.location = t.location;
                        res.value = t.value;
                        res.type = t.type;
                        errors.push_back(res);
                        continue;
                    }
                    Method* m = getMethodByName(result, f->name, c.name);
                    if (!argsAreIdentical(m->args, f->args)) {
                        FPResult res;
                        Token t = m->name_token;
                        res.success = false;
                        res.message = "Arguments of method '" + c.name + ":" + m->name + "' do not match implemented method '" + f->name + "'";
                        res.location = t.location;
                        res.value = t.value;
                        res.type = t.type;
                        errors.push_back(res);
                        continue;
                    }
                    Struct argType = getStructByName(result, m->return_type);
                    bool implementedInterface = false;
                    if (argType != Struct::Null) {
                        Interface* interface = getInterfaceByName(result, f->return_type);
                        if (interface != nullptr) {
                            implementedInterface = structImplements(result, argType, interface->name);
                        }
                    }
                    if (!implementedInterface && f->return_type != "?" && !typeEquals(m->return_type, f->return_type)) {
                        FPResult res;
                        Token t = m->name_token;
                        res.success = false;
                        res.message = "Return type of method '" + c.name + ":" + m->name + "' does not match implemented method '" + f->name + "'. Return type should be: '" + f->return_type + "'";
                        res.location = t.location;
                        res.value = t.value;
                        res.type = t.type;
                        errors.push_back(res);
                        continue;
                    }
                    if (f->return_type == "?" && m->return_type == "none") {
                        FPResult res;
                        Token t = m->name_token;
                        res.success = false;
                        res.message = "Cannot return 'none' from method '" + c.name + ":" + m->name + "' when implemented method '" + f->name + "' returns '" + f->return_type + "'";
                        res.location = t.location;
                        res.value = t.value;
                        res.type = t.type;
                        errors.push_back(res);
                        continue;
                    }
                }
            }

            for (Function* f : result.functions) {
                if (!f->isMethod) continue;
                Method* m = (Method*) f;
                if (m->member_type != c.name) continue;

                Struct super = getStructByName(result, c.super);
                while (super != Struct::Null) {
                    Method* other = getMethodByNameOnThisType(result, m->name, super.name);
                    if (!other) goto afterSealedCheck;
                    if (other->has_sealed) {
                        FPResult res;
                        Token t = m->name_token;
                        res.success = false;
                        res.message = "Method '" + f->name + "' overrides 'sealed' method on '" + super.name + "'";
                        res.location = t.location;
                        res.value = t.value;
                        res.type = t.type;
                        errors.push_back(res);
                        FPResult res2;
                        Token t2 = other->name_token;
                        res2.success = false;
                        res2.isNote = true;
                        res2.message = "Declared here:";
                        res2.location = t2.location;
                        res2.value = t2.value;
                        res2.type = t2.type;
                        errors.push_back(res2);
                        break;
                    }
                afterSealedCheck:
                    super = getStructByName(result, super.super);
                }
            }

            makeVTable(result, c.name);

            for (Variable& s : c.members) {
                if (!s.isVirtual) continue;
                Method* getter = attributeAccessor(result, c.name, s.name);
                if (!getter) {
                    FPResult res;
                    res.success = false;
                    res.message = "No getter for virtual member '" + s.name + "'";
                    res.location = s.name_token->location;
                    res.value = s.name_token->value;
                    res.type = s.name_token->type;
                    errors.push_back(res);
                }
                if (!s.isConst) {
                    Method* setter = attributeMutator(result, c.name, s.name);
                    if (!setter) {
                        FPResult res;
                        res.success = false;
                        res.message = "No setter for virtual member '" + s.name + "'";
                        res.location = s.name_token->location;
                        res.value = s.name_token->value;
                        res.type = s.name_token->type;
                        errors.push_back(res);
                    }
                }
            }
        }

        if (structTree) {
            structTree->forEach([&](StructTreeNode* currentNode) {
                Struct c = currentNode->s;
                if (c.isStatic()) return;
                append("struct Struct_%s {\n", c.name.c_str());
                append("  const _scl_lambda* const $fast;\n");
                append("  const TypeInfo* $statics;\n");
                append("  scl_any $mutex;\n");
                
                fprintf(scale_header, "struct Struct_%s {\n", c.name.c_str());
                fprintf(scale_header, "  const _scl_lambda* const $fast;\n");
                fprintf(scale_header, "  const TypeInfo* $statics;\n");
                fprintf(scale_header, "  scl_any $mutex;\n");
                
                for (Variable& s : c.members) {
                    if (s.isVirtual) continue;
                    append("  %s %s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
                    fprintf(scale_header, "  %s %s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
                }

                append("};\n");
                fprintf(scale_header, "};\n");
            });
        }
        

        currentStruct = Struct::Null;
        append("\n");
    }

    handler(Lambda) {
        noUnused;
        Function* f;
        if (function->isMethod) {
            f = new Function("$lambda$" + std::to_string(lambdaCount++) + "$" + function->member_type + "$$" + function->finalName(), body[i]);
        } else {
            f = new Function("$lambda$" + std::to_string(lambdaCount++) + "$" + function->finalName(), body[i]);
        }
        f->addModifier("<lambda>");
        f->addModifier(generateSymbolForFunction(function).substr(44));
        f->return_type = "none";
        safeInc();
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
                        FPResult r = parseType(body, &i, getTemplates(result, function));
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
            FPResult r = parseType(body, &i, getTemplates(result, function));
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
            if (body[i].type == tok_identifier && body[i].value == "lambda") {
                if (((ssize_t) i) - 1 < 0 || body[i - 1].type != tok_column) {
                    lambdaDepth++;
                }
            }
            if (body[i].type == tok_end) {
                lambdaDepth--;
            }
            f->addToken(body[i]);
            safeInc();
        }

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

        std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;

        append("(localstack++)->v = ({\n");
        scopeDepth++;
        append("%s fn_$lambda%d$%s(%s) __asm(%s);\n", sclTypeToCType(result, f->return_type).c_str(), lambdaCount - 1, function->finalName().c_str(), arguments.c_str(), generateSymbolForFunction(f).c_str());
        append("fn_$lambda%d$%s;\n", lambdaCount - 1, function->finalName().c_str());
        scopeDepth--;
        append("});\n");

        typeStack.push(lambdaType);
        result.functions.push_back(f);
    }

    handler(ReturnOnNil) {
        noUnused;
        if (typeCanBeNil(typeStackTop)) {
            std::string type = typeStackTop;
            typePop;
            typeStack.push(type.substr(0, type.size() - 1));
        }
        if (function->return_type == "nothing") {
            transpilerError("Returning from a function with return type 'nothing' is not allowed.", i);
            errors.push_back(err);
            return;
        }
        if (!typeCanBeNil(function->return_type) && function->return_type != "none") {
            transpilerError("Return-if-nil operator '?' behaves like assert-not-nil operator '!!' in not-nil returning function.", i);
            warns.push_back(err);
            append("_scl_assert((*(scl_int*) (localstack - 1)), \"Not nil assertion failed!\");\n");
        } else {
            if (function->return_type == "none") {
                if (!function->isMethod && !Main::options::noMain && function->name == "main") {
                    append("if ((*(scl_int*) (localstack - 1)) == 0) return 0;\n");
                } else {
                    append("if ((*(scl_int*) (localstack - 1)) == 0) return;\n");
                }
            } else {
                append("if ((*(scl_int*) (localstack - 1)) == 0) return 0;\n");
            }
        }
    }

    handler(Catch) {
        noUnused;
        std::string ex = "Exception";
        safeInc();
        if (body[i].value == "typeof") {
            safeInc();
            varScopePop();
            scopeDepth--;
            if (wasTry()) {
                popTry();
                append("} else if (_scl_is_instance_of(_scl_exception_handler.exception, 0x%xU)) {\n", id("Error"));
                append("  fn_throw((scl_Exception) _scl_exception_handler.exception);\n");
            } else if (wasCatch()) {
                popCatch();
            }

            varScopePush();
            pushCatch();
            Struct s = getStructByName(result, body[i].value);
            if (s == Struct::Null) {
                transpilerError("Trying to catch unknown Exception of type '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }

            if (structExtends(result, s, "Error")) {
                transpilerError("Cannot catch Exceptions that extend 'Error'", i);
                errors.push_back(err);
                return;
            }
            append("} else if (_scl_is_instance_of(_scl_exception_handler.exception, 0x%xU)) {\n", id(body[i].value.c_str()));
        } else {
            i--;
            {
                transpilerError("Generic Exception caught here:", i);
                errors.push_back(err);
            }
            transpilerError("Add 'typeof <Exception>' after 'catch' to catch specific Exceptions", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        std::string exName = "exception";
        if (body.size() > i + 1 && body[i + 1].value == "as") {
            safeInc();
            safeInc();
            exName = body[i].value;
        }
        scopeDepth++;
        varScopeTop().push_back(Variable(exName, ex));
        append("scl_%s Var_%s = (scl_%s) _scl_exception_handler.exception;\n", ex.c_str(), exName.c_str(), ex.c_str());
    }

    handler(Pragma) {
        noUnused;
        safeInc();
        if (body[i].value == "print") {
            safeInc();
            std::cout << body[i].location.file
                      << ":"
                      << body[i].location.line
                      << ":"
                      << body[i].location.column
                      << ": "
                      << body[i].value
                      << std::endl;
        } else if (body[i].value == "stackalloc") {
            safeInc();
            transpilerError("Stackalloc arrays have been removed from Scale", i);
            errors.push_back(err);
        } else if (body[i].value == "unset") {
            safeInc();

            std::string var = body[i].value;
            if (!hasVar(var)) {
                transpilerError("Trying to unset unknown variable '" + var + "'", i);
                errors.push_back(err);
                return;
            }
            Variable v = getVar(var);
            if (v.isConst) {
                transpilerError("Trying to unset constant variable '" + var + "'", i);
                errors.push_back(err);
                return;
            }
            if (v.type.size() < 2 || v.type.front() != '[' || v.type.back() != ']') {
                transpilerError("Trying to unset non-reference variable '" + var + "'", i);
                errors.push_back(err);
                return;
            }

            if (removeTypeModifiers(v.type) == "float") {
                append("*Var_%s = 0.0;\n", var.c_str());
            } else {
                append("*Var_%s = NULL;\n", var.c_str());
            }
        } else if (body[i].value == "note" || body[i].value == "warn" || body[i].value == "error") {
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
        } else {
            transpilerError("Unknown pragma '" + body[i].value + "'", i);
            errors.push_back(err);
        }
    }

    handler(Identifier) {
        noUnused;
        if (body[i].value == "drop") {
            append("--localstack;\n");
            typePop;
        } else if (body[i].value == "!!") {
            if (typeCanBeNil(typeStackTop)) {
                std::string type = typeStackTop;
                typePop;
                typeStack.push(type.substr(0, type.size() - 1));
                append("_scl_assert((*(scl_int*) (localstack - 1)), \"Not nil assertion failed!\");\n");
            } else {
                transpilerError("Unnecessary assert-not-nil operator '!!' on not-nil type '" + typeStackTop + "'", i);
                warns.push_back(err);
                append("_scl_assert((*(scl_int*) (localstack - 1)), \"Not nil assertion failed! If you see this, something has gone very wrong!\");\n");
            }
        } else if (body[i].value == "?") {
            handle(ReturnOnNil);
        } else if (body[i].value == "exit") {
            append("exit(*(scl_int*) --localstack);\n");
            typePop;
        } else if (body[i].value == "abort") {
            append("abort();\n");
        } else if (body[i].value == "typeof") {
            safeInc();
            auto templates = currentStruct.templates;
            if (hasVar(body[i].value)) {
                std::string type = getVar(body[i].value).type;
                const Struct& s = getStructByName(result, type);
                if (s != Struct::Null && !s.isStatic()) {
                    if (type.front() == '*') {
                        append("*(scl_str*) (localstack) = _scl_create_string(Var_%s.$statics->type_name);\n", body[i].value.c_str());
                    } else {
                        append("*(scl_str*) (localstack) = _scl_create_string(Var_%s->$statics->type_name);\n", body[i].value.c_str());
                    }
                    append("localstack++;\n");
                } else {
                    append("*(scl_str*) (localstack) = _scl_create_string(_scl_typename_or_else(*(scl_any*) &Var_%s, \"%s\"));\n", getVar(body[i].value).name.c_str(), getVar(body[i].value).type.c_str());
                    append("localstack++;\n");
                }
            } else if (hasFunction(result, body[i].value)) {
                Function* f = getFunctionByName(result, body[i].value);
                std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
                append("*(scl_str*) (localstack) = _scl_create_string(\"%s\");\n", lambdaType.c_str());
                append("localstack++;\n");
            } else if (body[i].type == tok_paren_open) {
                append("{\n");
                scopeDepth++;
                size_t stack_start = typeStack.size();
                handle(ParenOpen);
                if (typeStack.size() <= stack_start) {
                    transpilerError("Expected expression evaluating to a value after 'typeof'", i);
                    errors.push_back(err);
                }
                const Struct& s = getStructByName(result, typeStackTop);
                if (s != Struct::Null && !s.isStatic()) {
                    append("*(scl_str*) (localstack - 1) = _scl_create_string((localstack - 1)->o->$statics->type_name);\n");
                } else {
                    append("*(scl_str*) (localstack - 1) = _scl_create_string(_scl_typename_or_else((localstack - 1)->v, \"%s\"));\n", typeStackTop.c_str());
                }
                scopeDepth--;
                append("}\n");
                typePop;
            } else {
                FPResult res = parseType(body, &i);
                if (!res.success) {
                    errors.push_back(res);
                    return;
                }
                append("*(scl_str*) (localstack) = _scl_create_string(\"%s\");\n", res.value.c_str());
                append("localstack++;\n");
            }
            typeStack.push("str");
        } else if (body[i].value == "nameof") {
            safeInc();
            if (hasVar(body[i].value)) {
                append("*(scl_str*) (localstack) = _scl_create_string(\"%s\");\n", body[i].value.c_str());
                append("localstack++;\n");
                typeStack.push("str");
            } else {
                transpilerError("Unknown Variable: '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (body[i].value == "sizeof") {
            safeInc();
            auto templates = getTemplates(result, function);
            if (body[i].value == "int") {
                append("(localstack++)->i = sizeof(scl_int);\n");
            } else if (body[i].value == "float") {
                append("(localstack++)->i = sizeof(scl_float);\n");
            } else if (body[i].value == "str") {
                append("(localstack++)->i = sizeof(scl_str);\n");
            } else if (body[i].value == "any") {
                append("(localstack++)->i = sizeof(scl_any);\n");
            } else if (body[i].value == "none" || body[i].value == "nothing") {
                append("(localstack++)->i = 0;\n");
            } else if (hasVar(body[i].value)) {
                append("(localstack++)->i = sizeof(%s);\n", sclTypeToCType(result, getVar(body[i].value).type).c_str());
            } else if (getStructByName(result, body[i].value) != Struct::Null) {
                append("(localstack++)->i = sizeof(struct Struct_%s);\n", body[i].value.c_str());
            } else if (hasTypealias(result, body[i].value)) {
                append("(localstack++)->i = sizeof(%s);\n", sclTypeToCType(result, body[i].value).c_str());
            } else if (hasLayout(result, body[i].value)) {
                append("(localstack++)->i = sizeof(struct Layout_%s);\n", body[i].value.c_str());
            } else if (templates.find(body[i].value) != templates.end()) {
                std::string replaceWith = templates[body[i].value];
                if (getStructByName(result, replaceWith) != Struct::Null)
                    append("(localstack++)->i = sizeof(struct Struct_%s);\n", replaceWith.c_str());
                else if (hasTypealias(result, replaceWith))
                    append("(localstack++)->i = sizeof(%s);\n", sclTypeToCType(result, replaceWith).c_str());
                else if (hasLayout(result, replaceWith))
                    append("(localstack++)->i = sizeof(struct Layout_%s);\n", replaceWith.c_str());
                else
                    append("(localstack++)->i = sizeof(%s);\n", sclTypeToCType(result, replaceWith).c_str());
            } else {
                FPResult type = parseType(body, &i, getTemplates(result, function));
                if (!type.success) {
                    transpilerError("Unknown Type: '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                append("(localstack++)->i = sizeof(%s);\n", sclTypeToCType(result, type.value).c_str());
            }
            typeStack.push("int");
        } else if (body[i].value == "try") {
            append("{\n");
            scopeDepth++;
            append("TRY {\n");
            scopeDepth++;
            varScopePush();
            pushTry();
        } else if (body[i].value == "catch") {
            handle(Catch);
        } else if (body[i].value == "lambda") {
            handle(Lambda);
        } else if (body[i].value == "unsafe") {
            isInUnsafe++;
            safeInc();
            while (body[i].type != tok_end) {
                handle(Token);
                safeInc();
            }
            isInUnsafe--;
        } else if (body[i].value == "pragma!") {
            handle(Pragma);
        } else if (body[i].value == "assert") {
            const Token& assertToken = body[i];
            safeInc();
            if (body[i].type != tok_paren_open) {
                transpilerError("Expected '(' after 'assert', but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            append("{\n");
            varScopePush();
            scopeDepth++;
            size_t stack_start = typeStack.size();
            handle(ParenOpen);
            if (stack_start >= typeStack.size()) {
                transpilerError("Expected expression returning a value after 'assert'", i);
                errors.push_back(err);
                return;
            }
            if (i + 1 < body.size() && body[i + 1].type == tok_else) {
                safeInc();
                safeInc();
                if (body[i].type != tok_paren_open) {
                    transpilerError("Expected '(' after 'else', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                append("if (!(*(scl_int*) (localstack - 1))) {\n");
                handle(ParenOpen);
                append("}\n");
            } else {
                append("_scl_assert((*(scl_int*) (localstack - 1)), \"Assertion at %s:%d:%d failed!\");\n", assertToken.location.file.c_str(), assertToken.location.line, assertToken.location.column);
            }
            varScopePop();
            scopeDepth--;
            append("}\n");
        } else if (hasFunction(result, body[i].value)) {
            Function* f = getFunctionByName(result, body[i].value);
            if (f->isCVarArgs()) {
                createVariadicCall(f, fp, result, errors, body, i);
                return;
            }
            if (f->isMethod) {
                transpilerError("'" + f->name + "' is a method, not a function.", i);
                errors.push_back(err);
                return;
            }
            functionCall(f, fp, result, warns, errors, body, i);
        } else if (hasFunction(result, function->member_type + "$" + body[i].value)) {
            Function* f = getFunctionByName(result, function->member_type + "$" + body[i].value);
            if (f->isCVarArgs()) {
                createVariadicCall(f, fp, result, errors, body, i);
                return;
            }
            if (f->isMethod) {
                transpilerError("'" + f->name + "' is a method, not a function.", i);
                errors.push_back(err);
                return;
            }
            functionCall(f, fp, result, warns, errors, body, i);
        } else if (getStructByName(result, body[i].value) != Struct::Null) {
            Struct s = getStructByName(result, body[i].value);
            std::map<std::string, std::string> templates;
            if (body[i + 1].type == tok_identifier && body[i + 1].value == "<") {
                auto nthTemplate = [&](Struct& s, size_t n) -> std::pair<std::string, std::string> {
                    size_t i = 0;
                    for (auto& template_ : s.templates) {
                        if (i == n) {
                            return template_;
                        }
                        safeInc();
                    }
                    return std::pair<std::string, std::string>();
                };
                safeIncN(2);
                while (body[i].value != ">") {
                    if (body[i].type == tok_paren_open || (body[i].type == tok_identifier && body[i].value == "typeof")) {
                        int begin = i;
                        handle(Token);
                        if (removeTypeModifiers(typeStackTop) != "str") {
                            transpilerError("Expected 'str', but got '" + typeStackTop + "'", begin);
                            errors.push_back(err);
                            return;
                        }
                        auto nth = nthTemplate(s, templates.size());
                        append("scl_str $template%s = *(scl_str*) --localstack;\n", nth.first.c_str());
                        typePop;
                        templates[nth.first] = "$template" + nth.first;
                    } else {
                        FPResult type = parseType(body, &i, currentStruct.templates);
                        if (!type.success) {
                            errors.push_back(type);
                            return;
                        }
                        auto t = nthTemplate(s, templates.size());
                        type.value = removeTypeModifiers(type.value);
                        if (type.value.size() > 2 && type.value.front() == '[' && type.value.back() == ']') {
                            transpilerError("Expected scalar type, but got '" + type.value + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (t.second != "any") {
                            Struct f = Struct::Null;
                            if (isPrimitiveIntegerType(t.second)) {
                                if (!isPrimitiveIntegerType(type.value)) {
                                    transpilerError("Expected integer type, but got '" + type.value + "'", i);
                                    errors.push_back(err);
                                    return;
                                }
                            } else if ((f = getStructByName(result, t.second)) != Struct::Null) {
                                Struct g = getStructByName(result, type.value);
                                if (g == Struct::Null) {
                                    transpilerError("Expected struct type, but got '" + type.value + "'", i);
                                    errors.push_back(err);
                                    return;
                                }
                                if (!structExtends(result, g, f.name)) {
                                    transpilerError("Struct '" + g.name + "' is not convertible to '" + f.name + "'", i);
                                    errors.push_back(err);
                                    return;
                                }
                            }
                        }
                        templates[t.first] = type.value;
                    }
                    safeInc();
                    if (body[i].value == ",") {
                        safeInc();
                    }
                }
            }
            if (body[i + 1].type == tok_double_column) {
                safeInc();
                safeInc();
                Method* initMethod = getMethodByName(result, "init", s.name);
                bool hasInitMethod = initMethod != nullptr;
                if (body[i].value == "new" || body[i].value == "default") {
                    if (hasInitMethod && initMethod->has_private) {
                        transpilerError("Direct instantiation of struct '" + s.name + "' is not allowed", i);
                        errors.push_back(err);
                        return;
                    }
                }
                if (body[i].value == "new" && !s.isStatic()) {
                    append("(localstack++)->v = ({\n");
                    scopeDepth++;
                    std::string ctype = sclTypeToCType(result, s.name);
                    append("%s tmp = ALLOC(%s);\n", ctype.c_str(), s.name.c_str());
                    
                    typeStack.push(s.name);
                    if (hasInitMethod) {
                        append("*(%s*) (localstack) = tmp;\n", ctype.c_str());
                        append("localstack++;\n");
                        methodCall(initMethod, fp, result, warns, errors, body, i);
                        typeStack.push(s.name);
                    }
                    append("tmp;\n");
                    scopeDepth--;
                    append("});\n");
                } else if (body[i].value == "default" && !s.isStatic()) {
                    append("(localstack++)->v = ALLOC(%s);\n", s.name.c_str());
                    typeStack.push(s.name);
                } else {
                    if (hasFunction(result, s.name + "$" + body[i].value)) {
                        Function* f = getFunctionByName(result, s.name + "$" + body[i].value);
                        if (f->isCVarArgs()) {
                            createVariadicCall(f, fp, result, errors, body, i);
                            return;
                        }
                        if (f->isMethod) {
                            transpilerError("'" + f->name + "' is not static", i);
                            errors.push_back(err);
                            return;
                        }
                        if (f->has_private && function->belongsToType(s.name)) {
                            if (f->member_type != function->member_type) {
                                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                                errors.push_back(err);
                                return;
                            }
                        }
                        functionCall(f, fp, result, warns, errors, body, i);
                    } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                        Variable v = getVar(s.name + "$" + body[i].value);
                        
                        std::string lastType = "";
                        std::string path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "", &lastType, /* doesWriteAfter */ false);
                        
                        LOAD_PATH(path, lastType);
                    } else {
                        transpilerError("Unknown static member of struct '" + s.name + "'", i);
                        errors.push_back(err);
                    }
                }
            } else if (body[i + 1].type == tok_curly_open) {
                safeInc();
                append("(localstack++)->v = ({\n");
                scopeDepth++;
                size_t begin = i - 1;
                append("scl_%s tmp = ALLOC(%s);\n", s.name.c_str(), s.name.c_str());
                append("_scl_frame_t* stack_start = localstack;\n");
                safeInc();
                size_t count = 0;
                varScopePush();
                if (function->isMethod) {
                    Variable v("super", getVar("self").type);
                    varScopeTop().push_back(v);
                    append("scl_%s Var_super = Var_self;\n", removeTypeModifiers(getVar("self").type).c_str());
                }
                append("scl_%s Var_self = tmp;\n", s.name.c_str());
                varScopeTop().push_back(Variable("self", "mut " + s.name));
                std::vector<std::string> missedMembers;

                size_t membersToInitialize = 0;
                for (auto&& v : s.members) {
                    if (v.name.front() != '$') {
                        missedMembers.push_back(v.name);
                        membersToInitialize++;
                    }
                }

                while (body[i].type != tok_curly_close) {
                    append("{\n");
                    scopeDepth++;
                    handle(Token);
                    safeInc();
                    if (body[i].type != tok_store) {
                        transpilerError("Expected store, but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    safeInc();
                    if (body[i].type != tok_identifier) {
                        transpilerError("'" + body[i].value + "' is not an identifier", i);
                        errors.push_back(err);
                        return;
                    }

                    std::string lastType = typeStackTop;

                    missedMembers = vecWithout(missedMembers, body[i].value);
                    const Variable& v = s.getMember(body[i].value);

                    if (!typesCompatible(result, lastType, v.type, true)) {
                        transpilerError("Incompatible types: '" + v.type + "' and '" + lastType + "'", i);
                        errors.push_back(err);
                        return;
                    }

                    Method* mutator = attributeMutator(result, s.name, body[i].value);
                    
                    if (mutator) {
                        append("mt_%s$%s(tmp, *(%s*) _scl_pop());\n", mutator->member_type.c_str(), mutator->finalName().c_str(), sclTypeToCType(result, lastType).c_str());
                    } else {
                        append("tmp->%s = *(%s*) _scl_pop();\n", body[i].value.c_str(), sclTypeToCType(result, lastType).c_str());
                    }
                    typePop;
                    scopeDepth--;
                    append("}\n");
                    append("localstack = stack_start;\n");
                    safeInc();
                    count++;
                }
                varScopePop();
                append("tmp;\n");
                scopeDepth--;
                append("});\n");
                typeStack.push(s.name);
                if (count < membersToInitialize) {
                    std::string missed = "{ ";
                    for (size_t i = 0; i < missedMembers.size(); i++) {
                        if (i) {
                            missed += ", ";
                        }
                        missed += missedMembers[i];
                    }
                    missed += " }";
                    transpilerError("Not every member was initialized! Missed: " + missed, begin);
                    errors.push_back(err);
                    return;
                }
            }
        } else if (hasLayout(result, body[i].value)) {
            const Layout& l = getLayout(result, body[i].value);
            safeInc();
            if (body[i].type != tok_double_column) {
                transpilerError("Expected '::', but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            safeInc();
            if (body[i].value != "new") {
                transpilerError("Expected 'new' to create new layout, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            append("(localstack++)->v = (scl_any) _scl_alloc(sizeof(struct Layout_%s));\n", l.name.c_str());
            typeStack.push(l.name);
        } else if (hasEnum(result, body[i].value)) {
            Enum e = getEnumByName(result, body[i].value);
            safeInc();
            if (body[i].type != tok_double_column) {
                transpilerError("Expected '::', but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            safeInc();
            if (e.indexOf(body[i].value) == Enum::npos) {
                transpilerError("Unknown member of enum '" + e.name + "': '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            append("(localstack++)->i = %zuUL;\n", e.indexOf(body[i].value));
            typeStack.push("int");
        } else if (hasContainer(result, body[i].value)) {
            std::string containerName = body[i].value;
            safeInc();
            if (body[i].type != tok_dot) {
                transpilerError("Expected '.' to access container contents, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            safeInc();
            std::string memberName = body[i].value;
            Container container = getContainerByName(result, containerName);
            if (!container.hasMember(memberName)) {
                transpilerError("Unknown container member: '" + memberName + "'", i);
                errors.push_back(err);
                return;
            }
            Variable v = container.getMember(memberName);
            std::string lastType = "";
            std::string path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "Container_" + containerName + ".", &lastType, /* doesWriteAfter */ false);
            
            LOAD_PATH(path, lastType);
        
        } else if (hasVar(body[i].value)) {
        normalVar:
            Variable v = getVar(body[i].value);
            std::string lastType = "";
            std::string path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "", &lastType, /* doesWriteAfter */ false);

            LOAD_PATH(path, lastType);
        } else if (hasVar(function->member_type + "$" + body[i].value)) {
            Variable v = getVar(function->member_type + "$" + body[i].value);
            std::string lastType = "";
            std::string path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "", &lastType, /* doesWriteAfter */ false);
            
            LOAD_PATH(path, lastType);
        } else if (function->isMethod) {
            Struct s = getStructByName(result, function->member_type);
            Method* method = getMethodByName(result, body[i].value, s.name);
            Function* f;
            if (method != nullptr) {
                append("(localstack++)->i = (scl_int) Var_self;\n");
                typeStack.push(method->member_type);
                methodCall(method, fp, result, warns, errors, body, i);
            } else if ((f = getFunctionByName(result, s.name + "$" + body[i].value)) != nullptr) {
                if (f->isCVarArgs()) {
                    createVariadicCall(f, fp, result, errors, body, i);
                    return;
                }
                functionCall(f, fp, result, warns, errors, body, i);
            } else if (s.hasMember(body[i].value)) {
                Token here = body[i];
                body.insert(body.begin() + i, Token(tok_dot, ".", here.location));
                body.insert(body.begin() + i, Token(tok_identifier, "self", here.location));
                goto normalVar;
            } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                Variable v = getVar(s.name + "$" + body[i].value);
                std::string lastType = "";
                std::string path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "", &lastType, /* doesWriteAfter */ false);

                LOAD_PATH(path, lastType);
            } else {
                goto unknownIdent;
            }
        } else {
        unknownIdent:
            transpilerError("Unknown identifier: '" + body[i].value + "'", i);
            errors.push_back(err);
        }
    }

    handler(New) {
        noUnused;
        safeInc();
        std::string elemSize = "sizeof(scl_any)";
        std::string typeString = "any";
        if (body[i].type == tok_identifier && body[i].value == "<") {
            safeInc();
            auto type = parseType(body, &i);
            if (!type.success) {
                errors.push_back(type);
                return;
            }
            typeString = removeTypeModifiers(type.value);
            elemSize = "sizeof(" + sclTypeToCType(result, type.value) + ")";
            safeInc();
            safeInc();
        }
        int dimensions = 0;
        std::vector<int> startingOffsets;
        while (body[i].type == tok_bracket_open) {
            dimensions++;
            startingOffsets.push_back(i);
            safeInc();
            if (body[i].type == tok_bracket_close) {
                transpilerError("Empty array dimensions are not allowed", i);
                errors.push_back(err);
                i--;
                return;
            }
            while (body[i].type != tok_bracket_close) {
                handle(Token);
                safeInc();
            }
            safeInc();
        }
        i--;
        if (dimensions == 0) {
            transpilerError("Expected '[', but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        if (dimensions == 1) {
            append("(localstack - 1)->v = _scl_new_array_by_size((*(scl_int*) (localstack - 1)), %s);\n", elemSize.c_str());
        } else {
            append("{\n");
            scopeDepth++;
            std::string dims = "";
            for (int i = 0; i < dimensions; i++) {
                append("scl_int _scl_dim%d = *(scl_int*) _scl_pop();\n", i);
                if (i > 0) dims += ", ";
                dims += "_scl_dim" + std::to_string(i);
                if (!isPrimitiveIntegerType(typeStackTop)) {
                    transpilerError("Array size must be an integer, but got '" + removeTypeModifiers(typeStackTop) + "'", startingOffsets[i]);
                    errors.push_back(err);
                    return;
                }
                typePop;
            }

            append("scl_int tmp[] = {%s};\n", dims.c_str());
            append("(localstack++)->v = _scl_multi_new_array_by_size(%d, tmp, %s);\n", dimensions, elemSize.c_str());
            scopeDepth--;
            append("}\n");
        }
        typeStack.push("[" + typeString + "]");
    }

    handler(Repeat) {
        noUnused;
        safeInc();
        if (body[i].type != tok_number) {
            transpilerError("Expected integer, but got '" + body[i].value + "'", i);
            errors.push_back(err);
            if (i + 1 < body.size() && body[i + 1].type != tok_do)
                goto lbl_repeat_do_tok_chk;
            safeInc();
            return;
        }
        if (parseNumber(body[i].value) <= 0) {
            transpilerError("Repeat count must be greater than 0", i);
            errors.push_back(err);
        }
        if (i + 1 < body.size() && body[i + 1].type != tok_do) {
        lbl_repeat_do_tok_chk:
            transpilerError("Expected 'do', but got '" + body[i + 1].value + "'", i+1);
            errors.push_back(err);
            safeInc();
            return;
        }
        append("for (scl_int %c = 0; %c < (scl_int) %s; %c++) {\n",
            'a' + repeat_depth,
            'a' + repeat_depth,
            body[i].value.c_str(),
            'a' + repeat_depth
        );
        repeat_depth++;
        scopeDepth++;
        varScopePush();
        pushRepeat();
        safeInc();
    }

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
        append("*(scl_int*) --localstack;\n");
        typePop;
        scopeDepth--;
        append("}); Var_%s != ({\n", var.value.c_str());
        scopeDepth++;
        while (body[i].type != tok_step && body[i].type != tok_do) {
            handle(Token);
            safeInc();
        }
        typePop;
        std::string iterator_direction = "++";
        if (body[i].type == tok_step) {
            safeInc();
            if (body[i].type == tok_do) {
                transpilerError("Expected step, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            std::string val = body[i].value;
            if (val == "+") {
                safeInc();
                iterator_direction = " += ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else if (body[i].value == "+") {
                    iterator_direction = "++";
                } else {
                    transpilerError("Expected number or '+', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "-") {
                safeInc();
                iterator_direction = " -= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else if (body[i].value == "-") {
                    iterator_direction = "--";
                } else {
                    transpilerError("Expected number or '-', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "++") {
                iterator_direction = "++";
            } else if (val == "--") {
                iterator_direction = "--";
            } else if (val == "*") {
                safeInc();
                iterator_direction = " *= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else {
                    transpilerError("Expected number, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "/") {
                safeInc();
                iterator_direction = " /= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else {
                    transpilerError("Expected number, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "<<") {
                safeInc();
                iterator_direction = " <<= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else {
                    transpilerError("Expected number, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == ">>") {
                safeInc();
                iterator_direction = " >>= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else {
                    transpilerError("Expected number, but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                }
            } else if (val == "nop") {
                iterator_direction = "";
            }
            safeInc();
        }
        if (body[i].type != tok_do) {
            transpilerError("Expected 'do', but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        append("*(scl_int*) --localstack;\n");
        scopeDepth--;
        if (iterator_direction == "") {
            append("});) {\n");
        } else
            append("}); Var_%s%s) {\n", var.value.c_str(), iterator_direction.c_str());
        varScopePush();
        if (!hasVar(var.value)) {
            varScopeTop().push_back(Variable(var.value, "int"));
        }
        
        iterator_count++;
        scopeDepth++;

        if (hasFunction(result, var.value)) {
            FPResult result;
            result.message = "Variable '" + var.value + "' shadowed by function '" + var.value + "'";
            result.success = false;
            result.location = var.location;
            result.value = var.value;
            result.type =  var.type;
            warns.push_back(result);
        }
        if (hasContainer(result, var.value)) {
            FPResult result;
            result.message = "Variable '" + var.value + "' shadowed by container '" + var.value + "'";
            result.success = false;
            result.location = var.location;
            result.value = var.value;
            result.type =  var.type;
            warns.push_back(result);
        }

        pushOther();
    }

    handler(Foreach) {
        noUnused;
        safeInc();
        if (body[i].type != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        std::string iterator_name = "__it" + std::to_string(iterator_count++);
        Token iter_var_tok = body[i];
        safeInc();
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
            append("%s %s = (%s) *(scl_int*) --localstack;\n", sclTypeToCType(result, typeStackTop).c_str(), iterator_name.c_str(), sclTypeToCType(result, typeStackTop).c_str());
            typePop;
            append("for (scl_int i = 0; i < _scl_array_size(%s); i++) {\n", iterator_name.c_str());
            scopeDepth++;

            varScopePush();
            varScopeTop().push_back(Variable(iter_var_tok.value, type.substr(1, type.size() - 2)));
            append("%s Var_%s = %s[i];\n", sclTypeToCType(result, type.substr(1, type.size() - 2)).c_str(), iter_var_tok.value.c_str(), iterator_name.c_str());
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
        append("%s %s = (%s) *(scl_int*) --localstack;\n", sclTypeToCType(result, typeStackTop).c_str(), iterator_name.c_str(), sclTypeToCType(result, typeStackTop).c_str());
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
        varScopeTop().push_back(Variable(iter_var_tok.value, nextMethod->return_type));
        std::string cType = sclTypeToCType(result, getVar(iter_var_tok.value).type);
        append("while (virtual_call(%s, \"hasNext()i;\")) {\n", iterator_name.c_str());
        scopeDepth++;
        append("%s Var_%s = (%s) (scl_int) virtual_call(%s, \"next%s\");\n", sclTypeToCType(result, nextMethod->return_type).c_str(), iter_var_tok.value.c_str(), cType.c_str(), iterator_name.c_str(), argsToRTSignature(nextMethod).c_str());
    }

    handler(AddrRef) {
        noUnused;
        safeInc();
        Token toGet = body[i];

        Variable v("", "");
        std::string variablePrefix = "";

        std::string lastType;
        std::string path;

        if (hasFunction(result, toGet.value)) {
            Function* f = getFunctionByName(result, toGet.value);
            if (f->isCVarArgs()) {
                transpilerError("Cannot take reference of varargs function '" + f->name + "'", i);
                errors.push_back(err);
                return;
            }
            append("(localstack++)->i = (scl_int) &fn_%s;\n", f->finalName().c_str());
            std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
            typeStack.push(lambdaType);
            return;
        }
        if (!hasVar(body[i].value)) {
            if (body[i + 1].type == tok_double_column) {
                safeInc();
                Struct s = getStructByName(result, body[i - 1].value);
                safeInc();
                if (s != Struct::Null) {
                    if (hasFunction(result, s.name + "$" + body[i].value)) {
                        Function* f = getFunctionByName(result, s.name + "$" + body[i].value);
                        if (f->isCVarArgs()) {
                            transpilerError("Cannot take reference of varargs function '" + f->name + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (f->isMethod) {
                            transpilerError("'" + f->name + "' is not static", i);
                            errors.push_back(err);
                            return;
                        }
                        append("(localstack++)->i = (scl_int) &fn_%s;\n", f->finalName().c_str());
                        std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
                        typeStack.push(lambdaType);
                        return;
                    } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                        std::string loadFrom = s.name + "$" + body[i].value;
                        v = getVar(loadFrom);
                    } else {
                        transpilerError("Struct '" + s.name + "' has no static member named '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                }
            } else if (hasContainer(result, body[i].value)) {
                Container c = getContainerByName(result, body[i].value);
                safeInc();
                safeInc();
                std::string memberName = body[i].value;
                if (!c.hasMember(memberName)) {
                    transpilerError("Unknown container member: '" + memberName + "'", i);
                    errors.push_back(err);
                    return;
                }
                variablePrefix = "Container_" + c.name + ".";
                v = c.getMember(memberName);
            } else if (function->isMethod) {
                Method* m = ((Method*) function);
                Struct s = getStructByName(result, m->member_type);
                if (s.hasMember(body[i].value)) {
                    v = s.getMember(body[i].value);
                } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                    v = getVar(s.name + "$" + body[i].value);
                }
            } else {
                transpilerError("Use of undefined variable '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
        } else if (hasVar(body[i].value) && body[i + 1].type == tok_double_column) {
            Struct s = getStructByName(result, getVar(body[i].value).type);
            safeInc();
            safeInc();
            if (s != Struct::Null) {
                if (hasMethod(result, body[i].value, s.name)) {
                    Method* f = getMethodByName(result, body[i].value, s.name);
                    if (!f->isMethod) {
                        transpilerError("'" + f->name + "' is static", i);
                        errors.push_back(err);
                        return;
                    }
                    append("(localstack++)->i = (scl_int) &mt_%s$%s;\n", s.name.c_str(), f->finalName().c_str());
                    std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
                    typeStack.push(lambdaType);
                    return;
                } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                    std::string loadFrom = s.name + "$" + body[i].value;
                    v = getVar(loadFrom);
                } else {
                    transpilerError("Struct '" + s.name + "' has no static member named '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
            }
        } else {
            v = getVar(body[i].value);
        }
        path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, variablePrefix, &lastType, /* doesWriteAfter */ false);
        append("(localstack++)->i = (scl_int) &(%s);\n", path.c_str());
        typeStack.push("[" + lastType + "]");
    }

    handler(Store) {
        noUnused;
        safeInc();        

        if (body[i].type == tok_paren_open) {
            append("{\n");
            scopeDepth++;
            append("scl_Array tmp = (scl_Array) *(scl_int*) --localstack;\n");
            typePop;
            safeInc();
            int destructureIndex = 0;
            while (body[i].type != tok_paren_close) {
                if (body[i].type == tok_comma) {
                    safeInc();
                    continue;
                }
                if (body[i].type == tok_paren_close)
                    break;
                
                if (body[i].type == tok_addr_of) {
                    safeInc();
                    if (hasContainer(result, body[i].value)) {
                        std::string containerName = body[i].value;
                        safeInc();
                        if (body[i].type != tok_dot) {
                            transpilerError("Expected '.' to access container contents, but got '" + body[i].value + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        safeInc();
                        std::string memberName = body[i].value;
                        Container container = getContainerByName(result, containerName);
                        if (!container.hasMember(memberName)) {
                            transpilerError("Unknown container member: '" + memberName + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!container.getMember(memberName).isWritableFrom(function)) {
                            transpilerError("Container member variable '" + body[i].value + "' is not mutable", i);
                            errors.push_back(err);
                            safeInc();
                            safeInc();
                            return;
                        }
                        append("*((scl_any*) Container_%s.%s) = mt_Array$get(tmp, %d);\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                    } else {
                        if (body[i].type != tok_identifier) {
                            transpilerError("'" + body[i].value + "' is not an identifier", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!hasVar(body[i].value)) {
                            if (function->isMethod) {
                                Method* m = ((Method*) function);
                                Struct s = getStructByName(result, m->member_type);
                                if (s.hasMember(body[i].value)) {
                                    Variable mem = getVar(s.name + "$" + body[i].value);
                                    if (!mem.isWritableFrom(function)) {
                                        transpilerError("Variable '" + body[i].value + "' is not mutable", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("*(scl_any*) Var_self->%s = mt_Array$get(tmp, %d);\n", body[i].value.c_str(), destructureIndex);
                                    destructureIndex++;
                                    continue;
                                } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                                    Variable mem = getVar(s.name + "$" + body[i].value);
                                    if (!mem.isWritableFrom(function)) {
                                        transpilerError("Variable '" + body[i].value + "' is not mutable", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("*(scl_any*) Var_%s$%s = mt_Array$get(tmp, %d);\n", s.name.c_str(), body[i].value.c_str(), destructureIndex);
                                    destructureIndex++;
                                    continue;
                                }
                            }
                            transpilerError("Use of undefined variable '" + body[i].value + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        const Variable& v = getVar(body[i].value);
                        std::string loadFrom = v.name;
                        if (getStructByName(result, v.type) != Struct::Null) {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].value + "' is not mutable", i);
                                errors.push_back(err);
                                safeInc();
                                safeInc();
                                return;
                            }
                            safeInc();
                            if (body[i].type != tok_dot) {
                                append("*(scl_any*) Var_%s = mt_Array$get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                                destructureIndex++;
                                continue;
                            }
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i - 1].value + "' is not mutable", i - 1);
                                errors.push_back(err);
                                safeInc();
                                return;
                            }
                            safeInc();
                            Struct s = getStructByName(result, v.type);
                            if (!s.hasMember(body[i].value)) {
                                std::string help = "";
                                if (getMethodByName(result, body[i].value, s.name)) {
                                    help = ". Maybe you meant to use ':' instead of '.' here";
                                }
                                transpilerError("Struct '" + s.name + "' has no member named '" + body[i].value + "'" + help, i);
                                errors.push_back(err);
                                return;
                            }
                            Variable mem = s.getMember(body[i].value);
                            if (!mem.isWritableFrom(function)) {
                                transpilerError("Member variable '" + body[i].value + "' is not mutable", i);
                                errors.push_back(err);
                                return;
                            }
                            if (!mem.isAccessible(currentFunction)) {
                                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*(scl_any*) Var_%s->%s = mt_Array$get(tmp, %d);\n", loadFrom.c_str(), body[i].value.c_str(), destructureIndex);
                        } else {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].value + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            append("*(scl_any*) Var_%s = mt_Array$get(tmp, %d);\n", loadFrom.c_str(), destructureIndex);
                        }
                    }
                } else {
                    if (hasContainer(result, body[i].value)) {
                        std::string containerName = body[i].value;
                        safeInc();
                        if (body[i].type != tok_dot) {
                            transpilerError("Expected '.' to access container contents, but got '" + body[i].value + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        safeInc();
                        std::string memberName = body[i].value;
                        Container container = getContainerByName(result, containerName);
                        if (!container.hasMember(memberName)) {
                            transpilerError("Unknown container member: '" + memberName + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!container.getMember(memberName).isWritableFrom(function)) {
                            transpilerError("Container member variable '" + body[i].value + "' is const", i);
                            errors.push_back(err);
                            return;
                        }
                        append("{\n");
                        scopeDepth++;
                        append("scl_any tmp%d = mt_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                        if (!typeCanBeNil(container.getMember(memberName).type)) {
                            append("SCL_ASSUME(*(scl_int*) &tmp%d, \"Nil cannot be stored in non-nil variable '%%s'!\", \"%s.%s\");\n", destructureIndex, containerName.c_str(), memberName.c_str());
                        }
                        append("(*(scl_any*) &Container_%s.%s) = tmp%d;\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                        scopeDepth--;
                        append("}\n");
                    } else {
                        if (body[i].type != tok_identifier) {
                            transpilerError("'" + body[i].value + "' is not an identifier", i);
                            errors.push_back(err);
                            return;
                        }
                        if (!hasVar(body[i].value)) {
                            if (function->isMethod) {
                                Method* m = ((Method*) function);
                                Struct s = getStructByName(result, m->member_type);
                                if (s.hasMember(body[i].value)) {
                                    Variable mem = s.getMember(body[i].value);
                                    if (!mem.isWritableFrom(function)) {
                                        transpilerError("Member variable '" + body[i].value + "' is const", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("{\n");
                                    scopeDepth++;
                                    append("scl_any tmp%d = mt_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                                    if (!typeCanBeNil(mem.type)) {
                                        append("SCL_ASSUME(*(scl_int*) &tmp%d, \"Nil cannot be stored in non-nil variable '%%s'!\", \"self.%s\");\n", destructureIndex, mem.name.c_str());
                                    }
                                    append("*(scl_any*) &Var_self->%s = tmp%d;\n", body[i].value.c_str(), destructureIndex);
                                    scopeDepth--;
                                    append("}\n");
                                    destructureIndex++;
                                    continue;
                                } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                                    if (!getVar(s.name + "$" + body[i].value).isWritableFrom(function)) {
                                        transpilerError("Member variable '" + body[i].value + "' is const", i);
                                        errors.push_back(err);
                                        return;
                                    }
                                    append("{\n");
                                    scopeDepth++;
                                    append("scl_any tmp%d = mt_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                                    if (!typeCanBeNil(getVar(s.name + "$" + body[i].value).type)) {
                                        append("SCL_ASSUME(*(scl_int*) &tmp%d, \"Nil cannot be stored in non-nil variable '%%s'!\", \"%s::%s\");\n", destructureIndex, s.name.c_str(), body[i].value.c_str());
                                    }
                                    append("*(scl_any*) &Var_%s$%s = tmp%d;\n", s.name.c_str(), body[i].value.c_str(), destructureIndex);
                                    scopeDepth--;
                                    append("}\n");
                                    continue;
                                }
                            }
                            transpilerError("Use of undefined variable '" + body[i].value + "'", i);
                            errors.push_back(err);
                            return;
                        }
                        const Variable& v = getVar(body[i].value);
                        std::string loadFrom = v.name;
                        if (getStructByName(result, v.type) != Struct::Null) {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].value + "' is const", i);
                                errors.push_back(err);
                                safeInc();
                                safeInc();
                                return;
                            }
                            safeInc();
                            if (body[i].type != tok_dot) {
                                append("{\n");
                                scopeDepth++;
                                append("scl_any tmp%d = mt_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                                if (!typeCanBeNil(v.type)) {
                                    append("SCL_ASSUME(*(scl_int*) &tmp%d, \"Nil cannot be stored in non-nil variable '%%s'!\", \"%s\");\n", destructureIndex, v.name.c_str());
                                }
                                append("(*(scl_any*) &Var_%s) = tmp%d;\n", loadFrom.c_str(), destructureIndex);
                                scopeDepth--;
                                append("}\n");
                                destructureIndex++;
                                continue;
                            }
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i - 1].value + "' is not mutable", i - 1);
                                errors.push_back(err);
                                safeInc();
                                return;
                            }
                            safeInc();
                            Struct s = getStructByName(result, v.type);
                            if (!s.hasMember(body[i].value)) {
                                transpilerError("Struct '" + s.name + "' has no member named '" + body[i].value + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            Variable mem = s.getMember(body[i].value);
                            if (!mem.isWritableFrom(function)) {
                                transpilerError("Member variable '" + body[i].value + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            if (!mem.isAccessible(currentFunction)) {
                                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                                errors.push_back(err);
                                return;
                            }
                            append("{\n");
                            scopeDepth++;
                            append("scl_any tmp%d = mt_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                            if (!typeCanBeNil(mem.type)) {
                                append("SCL_ASSUME(*(scl_int*) &tmp%d, \"Nil cannot be stored in non-nil variable '%%s'!\", \"%s.%s\");\n", destructureIndex, s.name.c_str(), mem.name.c_str());
                            }
                            if (!typeCanBeNil(mem.type)) {
                                append("SCL_ASSUME(*(scl_int*) &tmp%d, \"Nil cannot be stored in non-nil variable '%%s'!\", \"self.%s\");\n", destructureIndex, mem.name.c_str());
                            }
                            Method* f = attributeMutator(result, s.name, mem.name);
                            if (f) {
                                append("mt_%s$%s(Var_%s, *(%s*) &tmp%d);\n", f->member_type.c_str(), f->name.c_str(), loadFrom.c_str(), sclTypeToCType(result, f->args[0].type).c_str(), destructureIndex);
                                return;
                            }
                            append("(*(scl_any*) &Var_%s->%s) = tmp%d;\n", loadFrom.c_str(), body[i].value.c_str(), destructureIndex);
                            scopeDepth--;
                            append("}\n");
                        } else {
                            if (!v.isWritableFrom(function)) {
                                transpilerError("Variable '" + body[i].value + "' is const", i);
                                errors.push_back(err);
                                return;
                            }
                            append("{\n");
                            scopeDepth++;
                            append("scl_any tmp%d = mt_Array$get(tmp, %d);\n", destructureIndex, destructureIndex);
                            if (!typeCanBeNil(v.type)) {
                                append("SCL_ASSUME(*(scl_int*) &tmp%d, \"Nil cannot be stored in non-nil variable '%%s'!\", \"%s\"));\n", destructureIndex, v.name.c_str());
                            }
                            append("(*(scl_any*) &Var_%s) = tmp%d;\n", loadFrom.c_str(), destructureIndex);
                            scopeDepth--;
                            append("}\n");
                        }
                    }
                }
                safeInc();
                destructureIndex++;
            }
            if (destructureIndex == 0) {
                transpilerError("Empty Array destructure", i);
                warns.push_back(err);
            }
            scopeDepth--;
            append("}\n");
        } else if (body[i].type == tok_declare) {
            safeInc();
            if (body[i].type != tok_identifier) {
                transpilerError("'" + body[i].value + "' is not an identifier", i+1);
                errors.push_back(err);
                return;
            }
            if (hasFunction(result, body[i].value)) {
                {
                    transpilerError("Variable '" + body[i].value + "' shadowed by function '" + body[i].value + "'", i);
                    warns.push_back(err);
                }
            }
            if (hasContainer(result, body[i].value)) {
                {
                    transpilerError("Variable '" + body[i].value + "' shadowed by container '" + body[i].value + "'", i);
                    warns.push_back(err);
                }
            }
            if (getStructByName(result, body[i].value) != Struct::Null) {
                {
                    transpilerError("Variable '" + body[i].value + "' shadowed by struct '" + body[i].value + "'", i);
                    warns.push_back(err);
                }
            }
            std::string name = body[i].value;
            std::string type = "any";
            safeInc();
            if (body[i].type == tok_column) {
                safeInc();
                FPResult r = parseType(body, &i, getTemplates(result, function));
                if (!r.success) {
                    errors.push_back(r);
                    return;
                }
                type = r.value;
                if (type == "lambda" && strstarts(typeStackTop, "lambda(")) {
                    type = typeStackTop;
                }
            } else {
                type = typeStackTop;
                i--;
            }
            varScopeTop().push_back(Variable(name, type));
            
            auto funcs = result.functions & std::function<bool(Function*)>([&](Function* f) {
                return f && (f->name == type + "$operator$store" || strstarts(f->name, type + "$operator$store$"));
            });

            for (Function* f : funcs) {
                if (
                    f->isMethod ||
                    f->args.size() != 1 ||
                    f->return_type != type ||
                    !typesCompatible(result, typeStackTop, f->args[0].type, true)
                ) {
                    continue;
                }
                append("%s Var_%s = fn_%s(*(%s*) _scl_pop());\n", sclTypeToCType(result, type).c_str(), name.c_str(), f->finalName().c_str(), sclTypeToCType(result, f->args[0].type).c_str());
                typePop;
                return;
            }
            if (!varScopeTop().back().canBeNil) {
                append("SCL_ASSUME((*(scl_int*) (localstack - 1)), \"Nil cannot be stored in non-nil variable '%%s'!\", \"%s\");\n", varScopeTop().back().name.c_str());
            }
            if (!typesCompatible(result, typeStackTop, type, true)) {
                transpilerError("Incompatible types: '" + type + "' and '" + typeStackTop + "'", i);
                warns.push_back(err);
            }
            if (type.front() == '*') {
                append("%s Var_%s = **(%s**) _scl_pop();\n", sclTypeToCType(result, type).c_str(), name.c_str(), sclTypeToCType(result, type).c_str());
            } else {
                append("%s Var_%s = *(%s*) _scl_pop();\n", sclTypeToCType(result, type).c_str(), name.c_str(), sclTypeToCType(result, type).c_str());
            }
            typePop;
        } else {
            if (body[i].type != tok_identifier && body[i].type != tok_addr_of) {
                transpilerError("'" + body[i].value + "' is not an identifier", i);
                errors.push_back(err);
            }
            Variable v("", "");
            std::string containerBegin = "";
            std::string lastType = "";

            std::string variablePrefix = "";

            bool topLevelDeref = false;

            if (body[i].type == tok_addr_of) {
                safeInc();
                topLevelDeref = true;
                if (body[i].type == tok_paren_open) {
                    if (isInUnsafe) {
                        safeInc();
                        append("{\n");
                        scopeDepth++;
                        append("scl_int _scl_value_to_store = *(scl_int*) --localstack;\n");
                        typePop;
                        append("{\n");
                        scopeDepth++;
                        while (body[i].type != tok_paren_close) {
                            handle(Token);
                            safeInc();
                        }
                        scopeDepth--;
                        append("}\n");
                        typePop;
                        append("*(scl_int*) *(scl_int*) --localstack = _scl_value_to_store;\n");
                        scopeDepth--;
                        append("}\n");
                    } else {
                        transpilerError("Storing at calculated pointer offset is only allowed in 'unsafe' blocks or functions", i - 1);
                        errors.push_back(err);
                    }
                    return;
                }
            }

            int start = i;

            if (hasVar(body[i].value)) {
            normalVar:
                v = getVar(body[i].value);
            } else if (hasContainer(result, body[i].value)) {
                Container c = getContainerByName(result, body[i].value);
                safeInc(); // i -> .
                safeInc(); // i -> member
                std::string memberName = body[i].value;
                if (!c.hasMember(memberName)) {
                    transpilerError("Unknown container member: '" + memberName + "'", i);
                    errors.push_back(err);
                    return;
                }
                variablePrefix = "Container_" + c.name + ".";
                v = c.getMember(memberName);
            } else if (hasVar(function->member_type + "$" + body[i].value)) {
                v = getVar(function->member_type + "$" + body[i].value);
            } else if (body[i + 1].type == tok_double_column) {
                Struct s = getStructByName(result, body[i].value);
                safeInc(); // i -> ::
                safeInc(); // i -> member
                if (s != Struct::Null) {
                    if (!hasVar(s.name + "$" + body[i].value)) {
                        transpilerError("Struct '" + s.name + "' has no static member named '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    v = getVar(s.name + "$" + body[i].value);
                } else {
                    transpilerError("Struct '" + body[i].value + "' does not exist", i);
                    errors.push_back(err);
                    return;
                }
            } else if (function->isMethod) {
                Method* m = ((Method*) function);
                Struct s = getStructByName(result, m->member_type);
                if (s.hasMember(body[i].value)) {
                    // v = s.getMember(body[i].value);
                    Token here = body[i];
                    body.insert(body.begin() + i, Token(tok_dot, ".", here.location));
                    body.insert(body.begin() + i, Token(tok_identifier, "self", here.location));
                    goto normalVar;
                } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                    v = getVar(s.name + "$" + body[i].value);
                } else {
                    transpilerError("Struct '" + s.name + "' has no member named '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
            } else {
                transpilerError("Unknown variable: '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }

            std::string currentType;
            std::string path = makePath(result, v, topLevelDeref, body, i, errors, variablePrefix, &currentType);
            // allow for => x[...], but not => x[(...)]
            if (i + 1 < body.size() && body[i + 1].type == tok_bracket_open && (i + 2 >= body.size() || body[i + 2].type != tok_paren_open)) {
                i = start;
                append("{\n");
                scopeDepth++;
                append("scl_any value = _scl_pop()->v;\n");
                std::string valueType = removeTypeModifiers(typeStackTop);
                typePop;

                append("{\n");
                scopeDepth++;
                while (body[i].type != tok_bracket_open) {
                    handle(Token);
                    safeInc();
                }
                scopeDepth--;
                append("}\n");
                int openBracket = i;
                safeInc();
                std::string arrayType = removeTypeModifiers(typeStackTop);
                append("%s array = (%s) _scl_pop()->v;\n", sclTypeToCType(result, arrayType).c_str(), sclTypeToCType(result, arrayType).c_str());

                typePop;
            reevaluateArrayIndexing:

                append("{\n");
                scopeDepth++;
                while (body[i].type != tok_bracket_close) {
                    handle(Token);
                    safeInc();
                }
                scopeDepth--;
                append("}\n");
                std::string indexingType = "";
                bool multiDimensional = false;
                if (i + 1 < body.size() && body[i + 1].type == tok_bracket_open) {
                    multiDimensional = true;
                    openBracket = i + 1;
                    safeIncN(2);
                }
                if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                    indexingType = "int";
                } else if (hasMethod(result, (multiDimensional ? "[]" : "=>[]"), arrayType)) {
                    Method* m = getMethodByName(result, (multiDimensional ? "[]" : "=>[]"), arrayType);
                    indexingType = m->args[0].type;
                }

                scopeDepth++;
                append("{\n");
                append("%s index = (%s) *(scl_int*) --localstack;\n", sclTypeToCType(result, indexingType).c_str(), sclTypeToCType(result, indexingType).c_str());
                std::string indexType = removeTypeModifiers(typeStackTop);
                if (!typeEquals(indexType, removeTypeModifiers(indexingType))) {
                    transpilerError("'" + arrayType + "' cannot be indexed with '" + indexType + "'", i);
                    errors.push_back(err);
                    return;
                }
                // indexType and indexingType are the same type
                typePop;

                if (multiDimensional) {
                    if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                        if (Main::options::debugBuild) append("_scl_array_check_bounds_or_throw((scl_any*) array, index);\n");
                        append("array = array[index];\n");
                        if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                            arrayType = arrayType.substr(1, arrayType.size() - 2);
                        } else {
                            arrayType = "any";
                        }
                    } else {
                        if (hasMethod(result, "[]", arrayType)) {
                            Method* m = getMethodByName(result, "[]", arrayType);
                            if (m->args.size() != 2) {
                                transpilerError("Method '[]' of type '" + arrayType + "' must have exactly 1 argument! Signature should be: '[](index: " + m->args[0].type + ")'", i);
                                errors.push_back(err);
                                return;
                            }
                            if (removeTypeModifiers(m->return_type) == "none" || removeTypeModifiers(m->return_type) == "nothing") {
                                transpilerError("Method '[]' of type '" + arrayType + "' must return a value", i);
                                errors.push_back(err);
                                return;
                            }
                            append("(localstack++)->v = index;\n");
                            typeStack.push(indexType);
                            append("(localstack++)->v = array;\n");
                            typeStack.push(arrayType);
                            methodCall(m, fp, result, warns, errors, body, i);
                        } else {
                            transpilerError("Type '" + arrayType + "' does not have a method '[]'", openBracket);
                            errors.push_back(err);
                        }
                    }
                    scopeDepth--;
                    append("}\n");
                    goto reevaluateArrayIndexing;
                }
                
                if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                    if (arrayType != "[any]" && !typeEquals(valueType, arrayType.substr(1, arrayType.size() - 2))) {
                        transpilerError("Array of type '" + arrayType + "' cannot contain '" + valueType + "'", start - 1);
                        errors.push_back(err);
                        return;
                    }
                    if (Main::options::debugBuild) append("_scl_array_check_bounds_or_throw((scl_any*) array, index);\n");
                    append("((scl_any*) array)[index] = value;\n");
                } else {
                    if (hasMethod(result, "=>[]", arrayType)) {
                        Method* m = getMethodByName(result, "=>[]", arrayType);
                        if (m->args.size() != 3) {
                            transpilerError("Method '=>[]' of type '" + arrayType + "' does not take 2 arguments! Signature should be: '=>[](index: " + indexType + ", value: " + valueType + ")'", openBracket);
                            errors.push_back(err);
                            return;
                        }
                        if (removeTypeModifiers(m->return_type) != "none") {
                            transpilerError("Method '=>[]' of type '" + arrayType + "' should not return anything", openBracket);
                            errors.push_back(err);
                            return;
                        }
                        append("(localstack++)->v = index;\n");
                        typeStack.push(indexType);
                        append("(localstack++)->v = value;\n");
                        typeStack.push(valueType);
                        append("(localstack++)->v = array;\n");
                        typeStack.push(arrayType);
                        methodCall(m, fp, result, warns, errors, body, i);
                    } else {
                        transpilerError("Type '" + arrayType + "' does not have a method '=>[]'", openBracket);
                        errors.push_back(err);
                    }
                }
                scopeDepth--;
                append("}\n");
                scopeDepth--;
                append("}\n");
                return;
            }

            if (!typesCompatible(result, typeStackTop, currentType, true)) {
                transpilerError("Incompatible types: '" + currentType + "' and '" + typeStackTop + "'", i);
                warns.push_back(err);
            }
            if (!typeCanBeNil(currentType)) {
                append("SCL_ASSUME((*(scl_int*) (localstack - 1)), \"Nil cannot be stored in non-nil variable '%%s'!\", \"%s\");\n", v.name.c_str());
            }
            append("%s;\n", path.c_str());
            typePop;
        }
    }

    handler(Declare) {
        noUnused;
        safeInc();
        if (body[i].type != tok_identifier) {
            transpilerError("'" + body[i].value + "' is not an identifier", i+1);
            errors.push_back(err);
            return;
        }
        if (hasFunction(result, body[i].value)) {
            transpilerError("Variable '" + body[i].value + "' shadowed by function '" + body[i].value + "'", i+1);
            warns.push_back(err);
        }
        if (hasContainer(result, body[i].value)) {
            transpilerError("Variable '" + body[i].value + "' shadowed by container '" + body[i].value + "'", i+1);
            warns.push_back(err);
        }
        if (getStructByName(result, body[i].value) != Struct::Null) {
            transpilerError("Variable '" + body[i].value + "' shadowed by struct '" + body[i].value + "'", i+1);
            warns.push_back(err);
        }
        std::string name = body[i].value;
        size_t start = i;
        std::string type = "any";
        safeInc();
        if (body[i].type == tok_column) {
            safeInc();
            FPResult r = parseType(body, &i, getTemplates(result, function));
            if (!r.success) {
                errors.push_back(r);
                return;
            }
            type = r.value;
        } else {
            transpilerError("A type is required", i);
            errors.push_back(err);
            return;
        }
        varScopeTop().push_back(Variable(name, type));
        if (!varScopeTop().back().canBeNil) {
            transpilerError("Uninitialized variable '" + name + "' with non-nil type '" + type + "'", start);
            errors.push_back(err);
        }
        type = sclTypeToCType(result, type);
        if (type == "scl_float") {
            append("%s Var_%s = 0.0;\n", type.c_str(), name.c_str());
        } else {
            if (hasTypealias(result, type) || hasLayout(result, type)) {
                append("%s Var_%s;\n", type.c_str(), name.c_str());
            } else {
                append("%s Var_%s = 0;\n", type.c_str(), name.c_str());
            }
        }
    }

    handler(CurlyOpen) {
        noUnused;
        std::string struct_ = "Array";
        if (getStructByName(result, struct_) == Struct::Null) {
            transpilerError("Struct definition for 'Array' not found", i);
            errors.push_back(err);
            return;
        }
        append("(localstack++)->v = ({\n");
        scopeDepth++;
        Struct arr = getStructByName(result, "Array");
        append("scl_Array tmp = ALLOC(Array);\n");
        append("mt_Array$init(tmp, 1);\n");
        safeInc();
        while (body[i].type != tok_curly_close) {
            if (body[i].type == tok_comma) {
                safeInc();
                continue;
            }
            bool didPush = false;
            while (body[i].type != tok_comma && body[i].type != tok_curly_close) {
                handle(Token);
                safeInc();
                didPush = true;
            }
            if (didPush) {
                append("mt_Array$push(tmp, _scl_pop()->v);\n");
                typePop;
            }
        }
        append("tmp;\n");
        typeStack.push("Array");
        scopeDepth--;
        append("});\n");
    }

    handler(BracketOpen) {
        noUnused;
        std::string type = removeTypeModifiers(typeStackTop);
        if (i + 1 < body.size()) {
            if (body[i + 1].type == tok_paren_open) {
                safeIncN(2);
                append("{\n");
                scopeDepth++;
                bool existingArrayUsed = false;
                if (body[i].type == tok_number && body[i + 1].type == tok_store) {
                    long long array_size = std::stoll(body[i].value);
                    if (array_size < 1) {
                        transpilerError("Array size must be positive", i);
                        errors.push_back(err);
                        return;
                    }
                    append("scl_int array_size = %s;\n", body[i].value.c_str());
                    safeIncN(2);
                } else {
                    while (body[i].type != tok_store) {
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
                        append("scl_int array_size = _scl_array_size((scl_any*) (localstack - 1)->v);\n");
                    } else {
                        append("scl_int array_size = *(scl_int*) --localstack;\n");
                        typePop;
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
                    append("%s* array = (%s*) _scl_pop()->v;\n", sclTypeToCType(result, elementType).c_str(), sclTypeToCType(result, elementType).c_str());
                } else {
                    append("scl_int* array = _scl_new_array_by_size(array_size, sizeof(%s));\n", sclTypeToCType(result, elementType).c_str());
                }
                append("for (scl_int i = 0; i < array_size; i++) {\n");
                scopeDepth++;
                varScopePush();
                append("const scl_int Var_i = i;\n");
                varScopeTop().push_back(Variable("i", "const int"));
                size_t typeStackSize = typeStack.size();
                if (existingArrayUsed) {
                    append("const %s Var_val = array[i];\n", sclTypeToCType(result, elementType).c_str());
                    varScopeTop().push_back(Variable("val", "const " + elementType));
                }
                while (body[i].type != tok_paren_close) {
                    handle(Token);
                    safeInc();
                }
                for (size_t i = typeStackSize; i < typeStack.size(); i++) {
                    typePop;
                }
                varScopePop();
                typePop;
                append("array[i] = *(scl_int*) --localstack;\n");
                scopeDepth--;
                append("}\n");
                append("(localstack++)->v = array;\n");
                typeStack.push(arrayType);
                scopeDepth--;
                append("}\n");
                safeInc();
                if (body[i].type != tok_bracket_close) {
                    transpilerError("Expected ']', but got '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                return;
            }
        }
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
                    if (Main::options::debugBuild) append("_scl_array_check_bounds_or_throw((scl_any*) (localstack - 1)->v, %s);\n", body[i - 1].value.c_str());
                    append("(localstack - 1)->v = (scl_any) ((%s) (localstack - 1)->v)[%s];\n", sclTypeToCType(result, type).c_str(), body[i - 1].value.c_str());
                    typePop;
                    typeStack.push(type.substr(1, type.size() - 2));
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
            append("%s tmp = (%s) _scl_pop()->v;\n", sclTypeToCType(result, type).c_str(), sclTypeToCType(result, type).c_str());
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
            append("scl_int index = *(scl_int*) --localstack;\n");
            if (Main::options::debugBuild) append("_scl_array_check_bounds_or_throw((scl_any*) tmp, index);\n");
            append("(localstack++)->v = (scl_any) tmp[index];\n");
            typeStack.push(type.substr(1, type.size() - 2));
            scopeDepth--;
            append("}\n");
            return;
        }
        if (hasMethod(result, "[]", typeStackTop)) {
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
            append("scl_any instance = _scl_pop()->v;\n");
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
            append("(localstack++)->v = instance;\n");
            typeStack.push(type);
            methodCall(m, fp, result, warns, errors, body, i);
            scopeDepth--;
            append("}\n");
            return;
        }
        transpilerError("'" + type + "' cannot be indexed", i);
        errors.push_back(err);
    }

    handler(ParenOpen) {
        noUnused;
        int commas = 0;
        auto stackSizeHere = typeStack.size();
        safeInc();
        append("{\n");
        scopeDepth++;
        append("_scl_frame_t* begin_stack_size = localstack;\n");
        while (body[i].type != tok_paren_close) {
            if (body[i].type == tok_comma) {
                commas++;
                safeInc();
            }
            handle(Token);
            safeInc();
        }

        if (commas == 0) {
            // Last-returns expression
            if (typeStack.size() > stackSizeHere) {
                std::string returns = typeStackTop;
                append("_scl_frame_t* return_value = _scl_pop();\n");
                append("localstack = begin_stack_size;\n");
                while (typeStack.size() > stackSizeHere) {
                    typeStack.pop();
                }
                append("(localstack++)->i = return_value->i;\n");
                typeStack.push(returns);
            }
        } else if (commas == 1) {
            Struct pair = getStructByName(result, "Pair");
            if (pair == Struct::Null) {
                transpilerError("Struct definition for 'Pair' not found", i);
                errors.push_back(err);
                return;
            }
            append("(localstack++)->v = ({\n");
            scopeDepth++;
            append("scl_Pair tmp = ALLOC(Pair);\n");
            append("_scl_popn(2);\n");
            typePop;
            typePop;
            append("mt_Pair$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v);\n");
            append("tmp;\n");
            typeStack.push("Pair");
            scopeDepth--;
            append("});\n");
        } else if (commas == 2) {
            Struct triple = getStructByName(result, "Triple");
            if (getStructByName(result, "Triple") == Struct::Null) {
                transpilerError("Struct definition for 'Triple' not found", i);
                errors.push_back(err);
                return;
            }
            append("(localstack++)->v = ({\n");
            scopeDepth++;
            append("scl_Triple tmp = ALLOC(Triple);\n");
            append("_scl_popn(3);\n");
            typePop;
            typePop;
            typePop;
            append("mt_Triple$init(tmp, _scl_positive_offset(0)->v, _scl_positive_offset(1)->v, _scl_positive_offset(2)->v);\n");
            append("tmp;\n");
            typeStack.push("Triple");
            scopeDepth--;
            append("});\n");
        } else {
            transpilerError("Unsupported tuple-like literal", i);
            errors.push_back(err);
        }
        scopeDepth--;
        append("}\n");
    }

    bool isAllowed1ByteChar(char c) {
        return (c < 0x7f && c >= ' ') || c == '\n' || c == '\t' || c == '\r';
    }

    bool checkUTF8(const std::string& str) {
        for (size_t i = 0; i < str.size(); i++) {
            if ((str[i] & 0b10000000) == 0b00000000) {
                if (!isAllowed1ByteChar(str[i])) {
                    return false;
                }
            } else if ((str[i] & 0b11100000) == 0b11000000) {
                if (i + 1 >= str.size()) {
                    return false;
                }
                if ((str[i + 1] & 0b11000000) != 0b10000000) {
                    return false;
                }
                i++;
            } else if ((str[i] & 0b11110000) == 0b11100000) {
                if (i + 2 >= str.size()) {
                    return false;
                }
                if ((str[i + 1] & 0b11000000) != 0b10000000) {
                    return false;
                }
                if ((str[i + 2] & 0b11000000) != 0b10000000) {
                    return false;
                }
                i += 2;
            } else if ((str[i] & 0b11111000) == 0b11110000) {
                if (i + 3 >= str.size()) {
                    return false;
                }
                if ((str[i + 1] & 0b11000000) != 0b10000000) {
                    return false;
                }
                if ((str[i + 2] & 0b11000000) != 0b10000000) {
                    return false;
                }
                if ((str[i + 3] & 0b11000000) != 0b10000000) {
                    return false;
                }
                i += 3;
            } else {
                return false;
            }
        }
        return true;
    }

    handler(StringLiteral) {
        noUnused;
        std::string str = unquote(body[i].value);
        if (body[i].type == tok_utf_string_literal && !checkUTF8(str)) {
            transpilerError("Invalid UTF-8 string", i);
            errors.push_back(err);
            return;
        }
        ID_t hash = id(str.c_str());
        append("(localstack++)->s = _scl_string_with_hash_len(\"%s\", 0x%x, %zu);\n", body[i].value.c_str(), hash, str.length());
        typeStack.push("str");
    }

    handler(CharStringLiteral) {
        noUnused;
        append("(localstack++)->cs = \"%s\";\n", body[i].value.c_str());
        typeStack.push("[int8]");
    }

    handler(CDecl) {
        noUnused;
        safeInc();
        if (body[i].type != tok_string_literal) {
            transpilerError("Expected string literal, but got '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        std::string s = replaceAll(body[i].value, R"(\\\\)", "\\");
        s = replaceAll(s, R"(\\\")", "\"");
        append("%s\n", s.c_str());
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
    }

    handler(ExternC) {
        noUnused;
        if (!isInUnsafe) {
            transpilerError("'c!' is only allowed in unsafe blocks", i);
            errors.push_back(err);
            return;
        }
        append("{// Start C\n");
        scopeDepth++;
        std::string file = body[i].location.file;
        if (strstarts(file, scaleFolder)) {
            file = file.substr(scaleFolder.size() + std::string("/Frameworks/").size());
        } else {
            file = std::filesystem::path(file).relative_path();
        }
        std::string ext = body[i].value;
        for (const Variable& v : vars) {
            append("%s* %s = &Var_%s;\n", sclTypeToCType(result, v.type).c_str(), v.name.c_str(), v.name.c_str());
        }
        append("{\n");
        scopeDepth++;
        std::vector<std::string> lines = split(ext, "\n");
        for (std::string& line : lines) {
            if (ltrim(line).size() == 0)
                continue;
            append("%s\n", ltrim(line).c_str());
        }
        scopeDepth--;
        append("}\n");
        scopeDepth--;
        append("}// End C\n");
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
    }

    handler(IntegerLiteral) {
        noUnused;
        std::string stringValue = body[i].value;

        // only parse to verify bounds and make very large integer literals defined behavior
        try {
            if (strstarts(stringValue, "0x")) {
                std::stoul(stringValue, nullptr, 16);
            } else if (strstarts(stringValue, "0b")) {
                std::stoul(stringValue, nullptr, 2);
            } else if (strstarts(stringValue, "0c")) {
                std::stoul(stringValue, nullptr, 8);
            } else {
                std::stol(stringValue);
            }
        } catch (const std::invalid_argument& e) {
            transpilerError("Invalid integer literal: '" + stringValue + "'", i);
            errors.push_back(err);
            return;
        } catch (const std::out_of_range& e) {
            transpilerError("Integer literal out of range: '" + stringValue + "' " + e.what(), i);
            errors.push_back(err);
            return;
        }
        append("(localstack++)->i = %s;\n", stringValue.c_str());
        typeStack.push("int");
    }

    handler(FloatLiteral) {
        noUnused;

        // only parse to verify
        auto num = parseDouble(body[i]);
        if (num.isError()) {
            errors.push_back(num.unwrapErr());
            return;
        }
        // we still want the original value from source code here
        append("(localstack++)->f = %s;\n", body[i].value.c_str());
        typeStack.push("float");
    }

    handler(FalsyType) {
        noUnused;
        append("(localstack++)->i = (scl_int) 0;\n");
        if (body[i].type == tok_nil) {
            typeStack.push("any");
        } else {
            typeStack.push("bool");
        }
    }

    handler(TruthyType) {
        noUnused;
        append("(localstack++)->i = (scl_int) 1;\n");
        typeStack.push("bool");
    }

    handler(Is) {
        noUnused;
        safeInc();
        FPResult res = parseType(body, &i);
        if (!res.success) {
            errors.push_back(res);
            return;
        }
        std::string type = res.value;
        if (isPrimitiveType(type)) {
            append("(*(scl_int*) (localstack - 1)) = !_scl_is_instance((localstack - 1)->v);\n");
        } else if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            append("(*(scl_int*) (localstack - 1)) = _scl_is_array((localstack - 1)->v);\n");
        } else {
            const Struct& s = getStructByName(result, type);
            Interface* iface = getInterfaceByName(result, type);
            if (s == Struct::Null && iface == nullptr) {
                transpilerError("Usage of undeclared struct '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            } else if (s != Struct::Null) {
                if (!s.isStatic()) {
                    append("(*(scl_int*) (localstack - 1)) = (localstack - 1)->v && _scl_is_instance_of((localstack - 1)->v, 0x%xU);\n", id(type.c_str()));
                } else {
                    append("(*(scl_int*) (localstack - 1)) = 0;\n");
                }
            } else {
                const Struct& stackStruct = getStructByName(result, typeStackTop);
                if (stackStruct == Struct::Null || stackStruct.isStatic()) {
                    append("(*(scl_int*) (localstack - 1)) = 0;\n");
                } else {
                    append("(*(scl_int*) (localstack - 1)) = (localstack - 1)->v && %d;\n", stackStruct.implements(iface->name));
                }
            }
        }
        typePop;
        typeStack.push("bool");
    }

    handler(If) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        append("if (({\n");
        scopeDepth++;
        varScopePush();
        safeInc();
        while (body[i].type != tok_then) {
            handle(Token);
            safeInc();
        }
        safeInc();
        varScopePop();
        append("*(scl_int*) --localstack;\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        typePop;
        varScopePush();
        auto tokenEndsIfBlock = [body](TokenType type) {
            return type == tok_elif ||
                   type == tok_elunless ||
                   type == tok_fi ||
                   type == tok_else;
        };
        bool exhaustive = false;
        auto beginningStackSize = typeStack.size();
        std::vector<std::string> returnedTypes;
    nextIfPart:
        while (!tokenEndsIfBlock(body[i].type)) {
            handle(Token);
            safeInc();
        }
        if (typeStackTop.size() > beginningStackSize) {
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
            typeStack.push(returnType);
        }
        handle(Fi);
    }

    handler(Unless) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        append("if (!({\n");
        scopeDepth++;
        varScopePush();
        safeInc();
        while (body[i].type != tok_then) {
            handle(Token);
            safeInc();
        }
        safeInc();
        varScopePop();
        append("*(scl_int*) --localstack;\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        typePop;
        varScopePush();
        auto tokenEndsIfBlock = [body](TokenType type) {
            return type == tok_elif ||
                   type == tok_elunless ||
                   type == tok_fi ||
                   type == tok_else;
        };
        std::string invalidType = "---------";
        std::string ifBlockReturnType = invalidType;
        bool exhaustive = false;
        auto beginningStackSize = typeStack.size();
        std::string problematicType = "";
    nextIfPart:
        while (!tokenEndsIfBlock(body[i].type)) {
            handle(Token);
            safeInc();
        }
        if (ifBlockReturnType == invalidType && typeStackTop.size()) {
            ifBlockReturnType = typeStackTop;
        } else if (typeStackTop.size() && ifBlockReturnType != typeStackTop) {
            problematicType = "[ " + ifBlockReturnType + ", " + typeStackTop + " ]";
            ifBlockReturnType = "---";
        }
        if (body[i].type != tok_fi) {
            handle(Token);
        }
        if (body[i].type == tok_else) {
            exhaustive = true;
        }
        while (typeStack.size() > beginningStackSize) {
            typePop;
        }
        if (body[i].type != tok_fi) {
            safeInc();
            goto nextIfPart;
        }
        if (ifBlockReturnType == "---") {
            transpilerError("Not every branch of this if-statement returns the same type! Types are: " + problematicType, i);
            errors.push_back(err);
            return;
        }
        bool sizeDecreased = false;
        while (typeStack.size() > beginningStackSize) {
            typePop;
            sizeDecreased = true;
        }
        if (exhaustive && sizeDecreased) {
            typeStack.push(ifBlockReturnType);
        }
        handle(Fi);
    }

    handler(Else) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        varScopePop();
        scopeDepth--;
        append("} else {\n");
        scopeDepth++;
        varScopePush();
    }

    handler(Elif) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        varScopePop();
        scopeDepth--;
        
        append("} else if (({\n");
        scopeDepth++;
        safeInc();
        while (body[i].type != tok_then) {
            handle(Token);
            safeInc();
        }
        append("*(scl_int*) --localstack;\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        varScopePush();
    }

    handler(Elunless) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        varScopePop();
        scopeDepth--;
        
        append("} else if (!({\n");
        scopeDepth++;
        safeInc();
        while (body[i].type != tok_then) {
            handle(Token);
            safeInc();
        }
        append("*(scl_int*) --localstack;\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        varScopePush();
    }

    handler(Fi) {
        noUnused;
        varScopePop();
        scopeDepth--;
        append("}\n");
        condCount--;
    }

    handler(While) {
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
        noUnused;
        append("while (({\n");
        scopeDepth++;
        safeInc();
        while (body[i].type != tok_do) {
            handle(Token);
            safeInc();
        }
        append("*(scl_int*) --localstack;\n");
        scopeDepth--;
        append("})) {\n");
        scopeDepth++;
        pushOther();
        varScopePush();
    }

    handler(Do) {
        noUnused;
        typePop;
        append("if (!_scl_pop()->v) break;\n");
    }

    handler(DoneLike) {
        noUnused;
        varScopePop();
        scopeDepth--;
        if (wasCatch()) {
            append("} else {\n");
            if (function->has_restrict) {
                if (function->isMethod) {
                    append("  _scl_unlock((scl_SclObject) Var_self);\n");
                } else {
                    append("  _scl_unlock(function_lock$%s);\n", function->finalName().c_str());
                }
            }
            append("  fn_throw((scl_Exception) _scl_exception_handler.exception);\n");
            append("}\n");
            scopeDepth--;
        }
        append("}\n");
        if (repeat_depth > 0 && wasRepeat()) {
            repeat_depth--;
        }
        if (wasSwitch()) {
            switchTypes.pop_back();
        }
        popOther();
    }

    handler(Return) {
        noUnused;

        if (function->return_type == "nothing") {
            transpilerError("Cannot return from a function with return type 'nothing'", i);
            errors.push_back(err);
            return;
        }

        auto type = removeTypeModifiers(function->return_type);

        if (type == "none") {
            if (!function->isMethod && !Main::options::noMain && function->name == "main") {
                append("return 0;\n");
            } else {
                append("return;\n");
            }
        } else {
            append("return (%s) ({\n", sclTypeToCType(result, function->return_type).c_str());
            scopeDepth++;

            std::string returningType = typeStackTop;
            if (function->namedReturnValue.name.size()) {
                returningType = function->namedReturnValue.type;
                append("%s retVal = Var_%s;\n", sclTypeToCType(result, returningType).c_str(), function->namedReturnValue.name.c_str());
            } else {
                if (returningType.front() == '*') {
                    append("%s retVal = **(%s**) --localstack;\n", sclTypeToCType(result, returningType).c_str(), sclTypeToCType(result, returningType).c_str());
                } else {
                    append("%s retVal = *(%s*) --localstack;\n", sclTypeToCType(result, returningType).c_str(), sclTypeToCType(result, returningType).c_str());
                }
            }
            if (!typeCanBeNil(function->return_type)) {
                if (typeCanBeNil(returningType)) {
                    transpilerError("Returning maybe-nil type '" + returningType + "' from function with not-nil return type '" + function->return_type + "'", i);
                    errors.push_back(err);
                    return;
                }
                if (!function->namedReturnValue.name.size()) {
                    if (typeCanBeNil(returningType)) {
                        transpilerError("Returning maybe-nil type '" + returningType + "' from function with not-nil return type '" + function->return_type + "'", i);
                        errors.push_back(err);
                        return;
                    }
                    if (!typesCompatible(result, returningType, function->return_type, true)) {
                        transpilerError("Returning type '" + returningType + "' from function with return type '" + function->return_type + "'", i);
                        errors.push_back(err);
                        return;
                    }
                }
                append("SCL_ASSUME(*(scl_int*) &retVal, \"Tried returning nil from function returning not-nil type '%%s'!\", \"%s\");\n", function->return_type.c_str());
            }
            append("retVal;\n");
            scopeDepth--;
            append("});\n");
        }
        for (long t = typeStack.size() - 1; t >= 0; t--) {
            typePop;
        }
    }

    handler(Goto) {
        noUnused;
        safeInc();
        if (body[i].type != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
            errors.push_back(err);
        }
        append("goto %s;\n", body[i].value.c_str());
    }

    handler(Label) {
        noUnused;
        safeInc();
        if (body[i].type != tok_identifier) {
            transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
            errors.push_back(err);
        }
        append("%s:\n", body[i].value.c_str());
    }

    handler(Case) {
        noUnused;
        safeInc();
        if (!wasSwitch()) {
            transpilerError("Unexpected 'case' statement", i);
            errors.push_back(err);
            return;
        }
        if (body[i].type == tok_string_literal) {
            transpilerError("String literal in case statement is not supported", i);
            errors.push_back(err);
        } else {
            const Struct& s = getStructByName(result, switchTypes.back());
            if (s.super == "Union") {
                size_t index = 2;
                if (!s.hasMember(body[i].value)) {
                    transpilerError("Unknown member '" + body[i].value + "' in union '" + s.name + "'", i);
                    errors.push_back(err);
                    return;
                }
                for (; index < s.members.size(); index++) {
                    if (s.members[index].name == body[i].value) {
                        break;
                    }
                }
                append("case %zu: {\n", index - 1);
                scopeDepth++;
                varScopePush();
                if (i + 1 < body.size() && body[i + 1].type == tok_paren_open) {
                    safeInc();
                    safeInc();
                    if (body[i].type != tok_identifier) {
                        transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                    }
                    std::string name = body[i].value;
                    safeInc();
                    if (body[i].type != tok_column) {
                        transpilerError("Expected ':', but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                    }
                    safeInc();
                    FPResult res = parseType(body, &i, getTemplates(result, function));
                    if (!res.success) {
                        errors.push_back(res);
                        return;
                    }
                    const Struct& requested = getStructByName(result, res.value);
                    if (requested != Struct::Null && !requested.isStatic()) {
                        append("_scl_checked_cast((localstack)->u->__value, %d, \"%s\");\n", id(res.value.c_str()), res.value.c_str());
                    }
                    append("%s Var_%s = *(%s*) &((localstack)->u->__value);\n", sclTypeToCType(result, res.value).c_str(), name.c_str(), sclTypeToCType(result, res.value).c_str());
                    varScopeTop().push_back(Variable(name, res.value));
                    safeInc();
                    if (body[i].type != tok_paren_close) {
                        transpilerError("Expected ')', but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                    }
                } else {
                    std::string removed = removeTypeModifiers(s.members[index].type);
                    if (removed != "none" && removed != "nothing") {
                        append("%s Var_it = *(%s*) &(localstack)->u->__value;\n", sclTypeToCType(result, s.members[index].type).c_str(), sclTypeToCType(result, s.members[index].type).c_str());
                        varScopeTop().push_back(Variable("it", s.members[index].type));
                    }
                }
                return;
            }
            append("case %s: {\n", body[i].value.c_str());
            scopeDepth++;
            varScopePush();
        }
    }

    handler(Esac) {
        noUnused;
        if (!wasSwitch()) {
            transpilerError("Unexpected 'esac'", i);
            errors.push_back(err);
            return;
        }
        append("break;\n");
        varScopePop();
        scopeDepth--;
        append("}\n");
    }

    handler(Default) {
        noUnused;
        if (!wasSwitch()) {
            transpilerError("Unexpected 'default' statement", i);
            errors.push_back(err);
            return;
        }
        const Struct& s = getStructByName(result, switchTypes.back());
        if (s.super == "Union") {
            append("default: {\n");
            scopeDepth++;
            varScopePush();
            append("%s Var_it = *(%s*) &(localstack)->u;\n", sclTypeToCType(result, s.name).c_str(), sclTypeToCType(result, s.name).c_str());
            varScopeTop().push_back(Variable("it", s.name));
            return;
        }
        append("default: {\n");
        scopeDepth++;
        varScopePush();
    }

    handler(Switch) {
        noUnused;
        std::string type = typeStackTop;
        switchTypes.push_back(type);
        typePop;
        const Struct& s = getStructByName(result, type);
        if (s.super == "Union") {
            append("switch (_scl_pop()->u->__tag) {\n");
            scopeDepth++;
            varScopePush();
            pushSwitch();
            return;
        }
        append("switch (*(scl_int*) --localstack) {\n");
        scopeDepth++;
        varScopePush();
        pushSwitch();
    }

    handler(Break) {
        noUnused;
        append("break;\n");
    }

    handler(Continue) {
        noUnused;
        append("continue;\n");
    }

    handler(As) {
        noUnused;
        safeInc();
        FPResult type = parseType(body, &i, getTemplates(result, function));
        if (!type.success) {
            errors.push_back(type);
            return;
        }
        if (type.value == "float" && typeStackTop != "float") {
            append("(localstack - 1)->f = (float) (*(scl_int*) (localstack - 1));\n");
            typePop;
            typeStack.push("float");
            return;
        }
        if ((type.value == "int" && typeStackTop != "float") || type.value == "float" || type.value == "any") {
            typePop;
            typeStack.push(type.value);
            return;
        }
        if (type.value == "int") {
            append("(*(scl_int*) (localstack - 1)) = (scl_int) (localstack - 1)->f;\n");
            typePop;
            typeStack.push("int");
            return;
        }
        if (type.value == "lambda" || strstarts(type.value, "lambda(")) {
            typePop;
            append("SCL_ASSUME((*(scl_int*) (localstack - 1)), \"Nil cannot be cast to non-nil type '%%s'!\", \"%s\");\n", type.value.c_str());
            typeStack.push(type.value);
            return;
        }
        if (isPrimitiveIntegerType(type.value)) {
            if (typeStackTop == "float") {
                append("(*(scl_int*) (localstack - 1)) = (scl_int) (localstack - 1)->f;\n");
            }
            typePop;
            typeStack.push(type.value);
            append("(*(scl_int*) (localstack - 1)) &= SCL_%s_MAX;\n", type.value.c_str());
            return;
        }
        auto typeIsPtr = [type]() -> bool {
            std::string t = type.value;
            if (t.size() == 0) return false;
            t = removeTypeModifiers(t);
            t = notNilTypeOf(t);
            return (t.front() == '[' && t.back() == ']');
        };
        if (hasTypealias(result, type.value) || typeIsPtr()) {
            typePop;
            typeStack.push(type.value);
            return;
        }
        if (hasLayout(result, type.value)) {
            append("SCL_ASSUME(_scl_sizeof((localstack - 1)->v) >= sizeof(struct Layout_%s), \"Layout '%%s' requires more memory than the pointer has available (required: \" SCL_INT_FMT \" found: \" SCL_INT_FMT \")\", \"%s\", sizeof(struct Layout_%s), _scl_sizeof((localstack - 1)->v));", type.value.c_str(), type.value.c_str(), type.value.c_str());
            typePop;
            typeStack.push(type.value);
            return;
        }
        if (getStructByName(result, type.value) == Struct::Null) {
            transpilerError("Use of undeclared Struct '" + type.value + "'", i);
            errors.push_back(err);
            return;
        }
        if (i + 1 < body.size() && body[i + 1].value == "?") {
            if (!typeCanBeNil(typeStackTop)) {
                transpilerError("Unneccessary cast to maybe-nil type '" + type.value + "?' from not-nil type '" + typeStackTop + "'", i - 1);
                warns.push_back(err);
            }
            typePop;
            typeStack.push(type.value + "?");
            safeInc();
        } else {
            if (typeCanBeNil(typeStackTop)) {
                append("SCL_ASSUME((*(scl_int*) (localstack - 1)), \"Nil cannot be cast to non-nil type '%%s'!\", \"%s\");\n", type.value.c_str());
            }
            std::string typeStr = removeTypeModifiers(type.value);
            if (getStructByName(result, typeStr) != Struct::Null) {
                append("_scl_checked_cast((localstack - 1)->v, %d, \"%s\");\n", id(typeStr.c_str()), typeStr.c_str());
            }
            typePop;
            typeStack.push(type.value);
        }
    }

    handler(Dot) {
        noUnused;
        std::string type = typeStackTop;
        typePop;

        if (hasLayout(result, type)) {
            Layout l = getLayout(result, type);
            safeInc();
            std::string member = body[i].value;
            if (!l.hasMember(member)) {
                transpilerError("No layout for '" + member + "' in '" + type + "'", i);
                errors.push_back(err);
                return;
            }
            const Variable& v = l.getMember(member);

            if (removeTypeModifiers(v.type) == "float") {
                append("(localstack - 1)->f = ((%s) (*(scl_int*) (localstack - 1)))->%s;\n", sclTypeToCType(result, l.name).c_str(), member.c_str());
            } else {
                append("(*(scl_int*) (localstack - 1)) = (scl_int) ((%s) (*(scl_int*) (localstack - 1)))->%s;\n", sclTypeToCType(result, l.name).c_str(), member.c_str());
            }
            typeStack.push(l.getMember(member).type);
            return;
        }
        std::string tmp = removeTypeModifiers(type);
        Struct s = getStructByName(result, type);
        if (s == Struct::Null) {
            transpilerError("Cannot infer type of stack top: expected valid Struct, but got '" + type + "'", i);
            errors.push_back(err);
            return;
        }
        Token dot = body[i];
        bool deref = body[i + 1].type == tok_addr_of;
        safeIncN(deref + 1);
        if (!s.hasMember(body[i].value)) {
            if (s.name == "Map" || s.super == "Map") {
                Method* f = getMethodByName(result, "get", s.name);
                if (!f) {
                    transpilerError("Could not find method 'get' on struct '" + s.name + "'", i - 1);
                    errors.push_back(err);
                    return;
                }
                
                append("{\n");
                scopeDepth++;
                append("scl_any tmp = _scl_pop()->v;\n");
                std::string t = type;
                append("*(scl_str*) (localstack) = _scl_create_string(\"%s\");\n", body[i].value.c_str());
                append("localstack++;\n");
                typeStack.push("str");
                append("(localstack++)->v = tmp;\n");
                typeStack.push(t);
                scopeDepth--;
                append("}\n");
                methodCall(f, fp, result, warns, errors, body, i);
                return;
            }
            std::string help = "";
            Method* m;
            if ((m = getMethodByName(result, body[i].value, s.name)) != nullptr) {
                std::string lambdaType = "lambda(" + std::to_string(m->args.size()) + "):" + m->return_type;
                append("(localstack++)->v = mt_%s$%s;\n", s.name.c_str(), m->name.c_str());
                typeStack.push(lambdaType);
                return;
            }
            transpilerError("Struct '" + s.name + "' has no member named '" + body[i].value + "'" + help, i);
            errors.push_back(err);
            return;
        }
        Variable mem = s.getMember(body[i].value);
        if (!mem.isAccessible(currentFunction)) {
            if (s.name != function->member_type) {
                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                errors.push_back(err);
                return;
            }
        }
        if (typeCanBeNil(type) && dot.value != "?.") {
            {
                transpilerError("Member access on maybe-nil type '" + type + "'", i);
                errors.push_back(err);
            }
            transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this '.'", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }

        Method* m = attributeAccessor(result, s.name, mem.name);
        if (m) {
            if (dot.value == "?.") {
                if (deref) {
                    append("*(%s*) (localstack - 1) = (*(scl_int*) (localstack - 1)) ? *(mt_%s$%s(*(%s*) (localstack - 1))) : 0%s;\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str(), removeTypeModifiers(m->return_type) == "float" ? ".0" : "");
                    std::string type = m->return_type;
                    if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                        type = type.substr(1, type.size() - 2);
                    } else {
                        type = "any";
                    }
                    typeStack.push(type);
                } else {
                    append("*(%s*) (localstack - 1) = (*(scl_int*) (localstack - 1)) ? mt_%s$%s(*(%s*) (localstack - 1)) : 0%s;\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str(), removeTypeModifiers(m->return_type) == "float" ? ".0" : "");
                    typeStack.push(m->return_type);
                }
            } else {
                if (deref) {
                    append("*(%s*) (localstack - 1) = *(mt_%s$%s(*(%s*) (localstack - 1)));\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str());
                    std::string type = m->return_type;
                    if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                        type = type.substr(1, type.size() - 2);
                    } else {
                        type = "any";
                    }
                    typeStack.push(type);
                } else {
                    append("*(%s*) (localstack - 1) = mt_%s$%s(*(%s*) (localstack - 1));\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str());
                    typeStack.push(m->return_type);
                }
            }
            return;
        }

        if (dot.value == "?.") {
            if (removeTypeModifiers(mem.type) == "float") {
                append("(localstack - 1)->f = (*(scl_int*) (localstack - 1)) ? ((%s) (localstack - 1)->v)->%s : 0.0f;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            } else {
                append("(localstack - 1)->v = (*(scl_int*) (localstack - 1)) ? (scl_any) ((%s) (localstack - 1)->v)->%s : NULL;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            }
        } else {
            if (removeTypeModifiers(mem.type) == "float") {
                append("(localstack - 1)->f = ((%s) (localstack - 1)->v)->%s;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            } else {
                append("(localstack - 1)->v = (scl_any) ((%s) (localstack - 1)->v)->%s;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            }
        }
        typeStack.push(mem.type);
        if (deref) {
            std::string path = dot.value + "@" + body[i].value;
            append("SCL_ASSUME((*(scl_int*) (localstack - 1)), \"Tried dereferencing nil pointer '%%s'!\", \"%s\");", path.c_str());
            append("(localstack - 1)->v = *(localstack - 1)->v;\n");
            std::string type = typeStackTop;
            typePop;
            if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                type = type.substr(1, type.size() - 2);
            } else {
                type = "any";
            }
            typeStack.push(type);
        }
    }

    handler(ColumnOnInterface) {
        noUnused;
        std::string type = typeStackTop;
        Interface* interface = getInterfaceByName(result, type);
        safeInc();
        Method* m = getMethodByNameOnThisType(result, body[i].value, interface->name);
        if (!m) {
            transpilerError("Interface '" + interface->name + "' has no member named '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        if (typeCanBeNil(type)) {
            {
                transpilerError("Calling method on maybe-nil type '" + type + "'", i);
                errors.push_back(err);
            }
            transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this ':'", i - 1);
            err.isNote = true;
            errors.push_back(err);
            return;
        }
        methodCall(m, fp, result, warns, errors, body, i);
    }

    handler(Column) {
        noUnused;
        if (strstarts(typeStackTop, "lambda(") || typeStackTop == "lambda") {
            safeInc();
            if (typeStackTop == "lambda") {
                transpilerError("Generic lambda has to be cast to a typed lambda before calling", i);
                errors.push_back(err);
                return;
            }
            std::string returnType = typeStackTop.substr(typeStackTop.find_last_of("):") + 1);
            std::string op = body[i].value;

            std::string args = typeStackTop.substr(typeStackTop.find_first_of("(") + 1, typeStackTop.find_last_of("):") - typeStackTop.find_first_of("(") - 1);
            size_t argAmount = std::stoi(args);
            
            std::string argTypes = "";
            std::string argGet = "";
            for (size_t argc = argAmount; argc; argc--) {
                argTypes += "scl_any";
                argGet += "_scl_positive_offset(" + std::to_string(argAmount - argc) + ")->v";
                if (argc > 1) {
                    argTypes += ", ";
                    argGet += ", ";
                }
                typePop;
            }

            typePop;
            if (op == "accept") {
                std::string cType = sclTypeToCType(result, returnType) + "(*)(" + argTypes + ")";
                std::string removed = removeTypeModifiers(returnType);
                append("_scl_popn(%zu);\n", argAmount + 1);
                if (removed == "none" || removed == "nothing") {
                    append("");
                } else {
                    append("*(%s*) (localstack) = ", sclTypeToCType(result, returnType).c_str());
                    typeStack.push(returnType);
                }
                append2("((%s) (localstack + %zu)->v)(%s);\n", cType.c_str(), argAmount, argGet.c_str());
                if (removed != "none" && removed != "nothing") {
                    append("localstack++;\n");
                }
            } else {
                transpilerError("Unknown method '" + body[i].value + "' on type 'lambda'", i);
                errors.push_back(err);
            }
            return;
        }
        std::string type = typeStackTop;
        std::string tmp = removeTypeModifiers(type);
        if (tmp.size() > 2 && tmp.front() == '[' && tmp.back() == ']') {
            safeInc();
            if (body[i].type != tok_identifier) {
                transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            std::string op = body[i].value;
            if (op == "resize") {
                append("{\n");
                scopeDepth++;
                append("scl_any* array = *(scl_any**) --localstack;\n");
                typePop;
                append("scl_int size = *(scl_int*) --localstack;\n");
                typePop;
                append("array = (scl_any*) _scl_array_resize((scl_any*) array, size);\n");
                append("(localstack++)->v = array;\n");
                typeStack.push(type);
                scopeDepth--;
                append("}\n");
            } else if (op == "sort") {
                append("(localstack - 1)->v = _scl_array_sort((scl_any*) (localstack - 1)->v);\n");
                typeStack.push(type);
            } else if (op == "reverse") {
                append("(localstack - 1)->v = _scl_array_reverse((scl_any*) (localstack - 1)->v);\n");
                typeStack.push(type);
            } else if (op == "toString") {
                append("*(scl_str*) (localstack - 1) = _scl_array_to_string((scl_any*) (localstack - 1)->v);\n");
                typeStack.push("str");
            } else if (op == "size") {
                append("(*(scl_int*) (localstack - 1)) = _scl_array_size((localstack - 1)->v);\n");
                typeStack.push("int");
            } else {
                transpilerError("Unknown method '" + body[i].value + "' on type '" + type + "'", i);
                errors.push_back(err);
            }
            return;
        }
        Struct s = getStructByName(result, type);
        if (s == Struct::Null) {
            if (getInterfaceByName(result, type) != nullptr) {
                handle(ColumnOnInterface);
                return;
            }
            if (isPrimitiveIntegerType(type)) {
                s = getStructByName(result, "int");
            } else {
                transpilerError("Cannot infer type of stack top: expected valid Struct, but got '" + type + "'", i);
                errors.push_back(err);
                return;
            }
        }
        bool onSuper = function->isMethod && body[i - 1].type == tok_identifier && body[i - 1].value == "super";
        safeInc();
        if (!s.isStatic() && !hasMethod(result, body[i].value, removeTypeModifiers(type))) {
            std::string help = "";
            if (s.hasMember(body[i].value)) {
                help = ". Maybe you meant to use '.' instead of ':' here";

                const Variable& v = s.getMember(body[i].value);
                std::string type = v.type;
                if (!strstarts(removeTypeModifiers(type), "lambda(")) {
                    goto notLambdaType;
                }

                std::string returnType = lambdaReturnType(type);
                size_t argAmount = lambdaArgCount(type);


                append("{\n");
                scopeDepth++;

                append("%s tmp = _scl_pop()->v;\n", sclTypeToCType(result, s.name).c_str());
                typePop;

                if (typeStack.size() < argAmount) {
                    transpilerError("Arguments for lambda '" + s.name + ":" + v.name + "' do not equal inferred stack", i);
                    errors.push_back(err);
                    return;
                }

                std::string argTypes = "";
                std::string argGet = "";
                for (size_t argc = argAmount; argc; argc--) {
                    argTypes += "scl_any";
                    argGet += "_scl_positive_offset(" + std::to_string(argAmount - argc) + ")->v";
                    if (argc > 1) {
                        argTypes += ", ";
                        argGet += ", ";
                    }
                    typePop;
                }
                if (removeTypeModifiers(returnType) == "none" || removeTypeModifiers(returnType) == "nothing") {
                    append("void(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.name.c_str());
                    append("_scl_popn(%zu);\n", argAmount);
                    append("lambda(%s);\n", argGet.c_str());
                } else if (removeTypeModifiers(returnType) == "float") {
                    append("scl_float(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.name.c_str());
                    append("_scl_popn(%zu);\n", argAmount);
                    append("(localstack++)->f = lambda(%s);\n", argGet.c_str());
                    typeStack.push(returnType);
                } else {
                    append("scl_any(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.name.c_str());
                    append("_scl_popn(%zu);\n", argAmount);
                    append("(localstack++)->v = lambda(%s);\n", argGet.c_str());
                    typeStack.push(returnType);
                }
                scopeDepth--;
                append("}\n");
                return;
            }
        notLambdaType:
            transpilerError("Unknown method '" + body[i].value + "' on type '" + removeTypeModifiers(type) + "'" + help, i);
            errors.push_back(err);
            return;
        } else if (s.isStatic() && s.name != "any" && s.name != "int" && !hasFunction(result, removeTypeModifiers(type) + "$" + body[i].value)) {
            transpilerError("Unknown static function '" + body[i].value + "' on type '" + removeTypeModifiers(type) + "'", i);
            errors.push_back(err);
            return;
        }
        if (s.name == "any" || s.name == "int") {
            Function* anyMethod = getFunctionByName(result, s.name + "$" + body[i].value);
            if (!anyMethod) {
                if (s.name == "any") {
                    anyMethod = getFunctionByName(result, "int$" + body[i].value);
                } else {
                    anyMethod = getFunctionByName(result, "any$" + body[i].value);
                }
            }
            if (anyMethod) {
                Method* objMethod = !Main::options::noScaleFramework ? getMethodByName(result, body[i].value, "SclObject") : nullptr;
                if (objMethod) {
                    append("if (_scl_is_instance((localstack - 1)->v)) {\n");
                    scopeDepth++;
                    methodCall(objMethod, fp, result, warns, errors, body, i, true, false);
                    if (objMethod->return_type != "none" && objMethod->return_type != "nothing") {
                        typeStack.pop();
                    }
                    scopeDepth--;
                    append("} else {\n");
                    scopeDepth++;
                }
                functionCall(anyMethod, fp, result, warns, errors, body, i);
                if (objMethod) {
                    scopeDepth--;
                    append("}\n");
                }
            } else {
                transpilerError("Unknown static function '" + body[i].value + "' on type '" + type + "'", i);
                errors.push_back(err);
            }
            return;
        }
        if (s.isStatic()) {
            Function* f = getFunctionByName(result, removeTypeModifiers(type) + "$" + body[i].value);
            if (!f) {
                transpilerError("Unknown static function '" + body[i].value + "' on type '" + removeTypeModifiers(type) + "'", i);
                errors.push_back(err);
                return;
            }
            functionCall(f, fp, result, warns, errors, body, i);
        } else {
            std::string typeRemoved = removeTypeModifiers(type);
            if (typeRemoved == "str") {
                if (body[i].value == "toString") {
                    return;
                } else if (body[i].value == "view") {
                    append("(localstack - 1)->v = (*(scl_str*) (localstack - 1))->data;\n");
                    typePop;
                    typeStack.push("[int8]");
                    return;
                }
            }
            if (typeCanBeNil(type)) {
                {
                    transpilerError("Calling method on maybe-nil type '" + type + "'", i);
                    errors.push_back(err);
                }
                transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this ':'", i - 1);
                err.isNote = true;
                errors.push_back(err);
                return;
            }
            Method* f = getMethodByName(result, body[i].value, s.name);
            if (f->has_private && function->member_type != f->member_type) {
                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                errors.push_back(err);
                return;
            }
            methodCall(f, fp, result, warns, errors, body, i, false, true, false, onSuper);
        }
    }

    handler(INVALID) {
        noUnused;
        if (i < body.size() && ((ssize_t) i) >= 0) {
            transpilerError("Unknown token: '" + body[i].value + "'", i);
            errors.push_back(err);
        } else {
            transpilerError("Unknown token: '" + body.back().value + "'", i);
            errors.push_back(err);
        }
    }

    void (*handleRefs[tok_MAX])(std::vector<Token>&, Function*, std::vector<FPResult>&, std::vector<FPResult>&, FILE*, TPResult&);

    void populateTokenJumpTable() {
        for (int i = 0; i < tok_MAX; i++) {
            handleRefs[i] = handlerRef(INVALID);
        }

        handleRefs[tok_question_mark] = handlerRef(Identifier);
        handleRefs[tok_identifier] = handlerRef(Identifier);
        handleRefs[tok_dot] = handlerRef(Dot);
        handleRefs[tok_column] = handlerRef(Column);
        handleRefs[tok_as] = handlerRef(As);
        handleRefs[tok_string_literal] = handlerRef(StringLiteral);
        handleRefs[tok_utf_string_literal] = handlerRef(StringLiteral);
        handleRefs[tok_char_string_literal] = handlerRef(CharStringLiteral);
        handleRefs[tok_cdecl] = handlerRef(CDecl);
        handleRefs[tok_extern_c] = handlerRef(ExternC);
        handleRefs[tok_number] = handlerRef(IntegerLiteral);
        handleRefs[tok_char_literal] = handlerRef(IntegerLiteral);
        handleRefs[tok_number_float] = handlerRef(FloatLiteral);
        handleRefs[tok_nil] = handlerRef(FalsyType);
        handleRefs[tok_false] = handlerRef(FalsyType);
        handleRefs[tok_true] = handlerRef(TruthyType);
        handleRefs[tok_new] = handlerRef(New);
        handleRefs[tok_is] = handlerRef(Is);
        handleRefs[tok_if] = handlerRef(If);
        handleRefs[tok_unless] = handlerRef(Unless);
        handleRefs[tok_else] = handlerRef(Else);
        handleRefs[tok_elif] = handlerRef(Elif);
        handleRefs[tok_elunless] = handlerRef(Elunless);
        handleRefs[tok_while] = handlerRef(While);
        handleRefs[tok_do] = handlerRef(Do);
        handleRefs[tok_repeat] = handlerRef(Repeat);
        handleRefs[tok_for] = handlerRef(For);
        handleRefs[tok_foreach] = handlerRef(Foreach);
        handleRefs[tok_fi] = handlerRef(Fi);
        handleRefs[tok_done] = handlerRef(DoneLike);
        handleRefs[tok_end] = handlerRef(DoneLike);
        handleRefs[tok_return] = handlerRef(Return);
        handleRefs[tok_addr_ref] = handlerRef(AddrRef);
        handleRefs[tok_store] = handlerRef(Store);
        handleRefs[tok_declare] = handlerRef(Declare);
        handleRefs[tok_continue] = handlerRef(Continue);
        handleRefs[tok_break] = handlerRef(Break);
        handleRefs[tok_goto] = handlerRef(Goto);
        handleRefs[tok_label] = handlerRef(Label);
        handleRefs[tok_case] = handlerRef(Case);
        handleRefs[tok_esac] = handlerRef(Esac);
        handleRefs[tok_default] = handlerRef(Default);
        handleRefs[tok_switch] = handlerRef(Switch);
        handleRefs[tok_curly_open] = handlerRef(CurlyOpen);
        handleRefs[tok_bracket_open] = handlerRef(BracketOpen);
        handleRefs[tok_paren_open] = handlerRef(ParenOpen);
        handleRefs[tok_addr_of] = handlerRef(Identifier);
    }

    handler(Token) {
        noUnused;

        handleRef(handleRefs[body[i].type]);
    }

    extern "C" void ConvertC::writeTables(FILE* fp, TPResult& result, std::string filename) {
        (void) filename;
        (void) result;
        populateTokenJumpTable();

        scopeDepth = 0;

        append("extern const ID_t type_id(const char*);\n\n");

        fclose(fp);
    }

    extern "C" void ConvertC::writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<Variable>& globals, TPResult& result, std::string filename) {
        (void) warns;

        auto filePreamble = [&]() {
            append("#include <scale_runtime.h>\n\n");
            append("#include \"./%s.h\"\n\n", filename.substr(0, filename.size() - 2).c_str());
            currentStruct = Struct::Null;

            append("#if defined(__clang__)\n");
            append("#pragma clang diagnostic push\n");
            append("#pragma clang diagnostic ignored \"-Wint-to-void-pointer-cast\"\n");
            append("#pragma clang diagnostic ignored \"-Wint-to-pointer-cast\"\n");
            append("#pragma clang diagnostic ignored \"-Wpointer-to-int-cast\"\n");
            append("#pragma clang diagnostic ignored \"-Wvoid-pointer-to-int-cast\"\n");
            append("#pragma clang diagnostic ignored \"-Wincompatible-pointer-types\"\n");
            append("#pragma clang diagnostic ignored \"-Wint-conversion\"\n");
            append("#pragma clang diagnostic ignored \"-Winteger-overflow\"\n");
            append("#endif\n\n");
        };

        auto filePostamble = [&]() {
            append("#if defined(__clang__)\n");
            append("#pragma clang diagnostic pop\n");
            append("#endif\n");
        };

        fp = fopen(("./" + filename).c_str(), "a");
        if (!fp) {
            fprintf(stderr, "Failed to open file %s\n", filename.c_str());
            exit(1);
        }
        filePreamble();

        for (size_t f = 0; f < result.functions.size(); f++) {
            Function* function = currentFunction = result.functions[f];
            if (function->has_expect || function->has_binary_inherited || getInterfaceByName(result, function->member_type)) {
                continue;
            }
            if (function->return_type.size() == 0) {
                transpilerErrorTok("Return type required", function->name_token);
                errors.push_back(err);
                continue;
            }

            if (function->isMethod) {
                currentStruct = getStructByName(result, function->member_type);
                if (currentStruct == Struct::Null) {
                    transpilerErrorTok("Method '" + function->name + "' is member of unknown Struct '" + function->member_type + "'", function->name_token);
                    errors.push_back(err);
                    continue;
                }
            } else {
                currentStruct = Struct::Null;
            }

            for (long t = typeStack.size() - 1; t >= 0; t--) {
                typePop;
            }
            vars.clear();
            varScopePush();
            varScopeTop().reserve(globals.size());
            for (const Variable& g : globals) {
                varScopeTop().push_back(g);
            }

            scopeDepth = 0;
            lambdaCount = 0;

            return_type = sclTypeToCType(result, function->return_type);

            std::string arguments;
            if (function->isMethod) {
                arguments = sclTypeToCType(result, function->member_type) + " Var_self";
            } else if (function->args.size() == 0) {
                arguments = "void";
            }
            if (!function->isMethod && !Main::options::noMain && function->name == "main") {
                arguments = "int __argc, char** __argv";
            } else {
                for (size_t i = 0; i < function->args.size() - (size_t) function->isMethod; i++) {
                    std::string type = sclTypeToCType(result, function->args[i].type);
                    if (i || function->isMethod) arguments += ", ";
                    arguments += type;

                    if (type == "varargs" || type == "...") continue;
                    if (function->args[i].name.size())
                        arguments += " Var_" + function->args[i].name;
                }
            }

            if (function->isMethod) {
                if (!((Method*) function)->force_add && currentStruct.isSealed()) {
                    transpilerErrorTok("Cannot add method '" + function->name + "' to sealed Struct '" + currentStruct.name + "'", function->name_token);
                    errors.push_back(err);
                    continue;
                }
                append("%s mt_%s$%s(%s) {\n", return_type.c_str(), function->member_type.c_str(), function->finalName().c_str(), arguments.c_str());
                if (function->name == "init" && currentStruct.super.size()) {
                    Method* parentInit = getMethodByName(result, "init", currentStruct.super);
                    if (parentInit && parentInit->args.size() == 1) {
                        append("  mt_%s$init((%s) Var_self);\n", currentStruct.super.c_str(), sclTypeToCType(result, currentStruct.super).c_str());
                    }
                }
            } else {
                if (function->has_restrict) {
                    if (Main::options::noScaleFramework) {
                        transpilerErrorTok("Function '" + function->name + "' has restrict, but Scale Framework is disabled", function->name_token);
                        errors.push_back(err);
                        continue;
                    }
                    append("static volatile scl_SclObject function_lock$%s = NULL;\n", function->finalName().c_str());
                }
                if (function->name == "throw" || function->name == "builtinUnreachable") {
                    append("_scl_no_return ");
                } else if (function->has_lambda) {
                    append("_scl_noinline %s fn_%s(%s) __asm(%s);\n", return_type.c_str(), function->finalName().c_str(), arguments.c_str(), generateSymbolForFunction(function).c_str());
                    append("_scl_noinline ");
                } else {
                    append("");
                }
                if (!function->isMethod && !Main::options::noMain && function->name == "main") {
                    append("int ");
                } else {
                    append2("%s ", return_type.c_str());
                }
                append2("fn_%s(%s) {\n", function->finalName().c_str(), arguments.c_str());
                if (function->has_restrict) {
                    append("  if (_scl_expect(function_lock$%s == NULL, 0)) function_lock$%s = ALLOC(SclObject);\n", function->finalName().c_str(), function->finalName().c_str());
                }
            }
            append("  _scl_frame_t localstack_backer[%zu];\n", function->stackSize());
            append("  _scl_frame_t* localstack = localstack_backer;\n");
            
            scopeDepth++;
            std::vector<Token> body = function->getBody();

            varScopePush();
            if (function->namedReturnValue.name.size()) {
                std::string nrvName = function->namedReturnValue.name;
                if (hasFunction(result, nrvName)) {
                    transpilerErrorTok("Named return value '" + nrvName + "' shadowed by function '" + nrvName + "'", function->name_token);
                    warns.push_back(err);
                }
                if (hasContainer(result, nrvName)) {
                    transpilerErrorTok("Named return value '" + nrvName + "' shadowed by container '" + nrvName + "'", function->name_token);
                    warns.push_back(err);
                }
                if (getStructByName(result, nrvName) != Struct::Null) {
                    transpilerErrorTok("Named return value '" + nrvName + "' shadowed by struct '" + nrvName + "'", function->name_token);
                    warns.push_back(err);
                }
                append("%s Var_%s;\n", sclTypeToCType(result, function->namedReturnValue.type).c_str(), function->namedReturnValue.name.c_str());

                varScopeTop().push_back(function->namedReturnValue);
            }
            for (size_t i = 0; i < function->args.size(); i++) {
                const Variable& var = function->args[i];
                if (var.type == "varargs") continue;
                varScopeTop().push_back(var);
            }

            append("SCL_BACKTRACE(\"%s\");\n", sclFunctionNameToFriendlyString(function).c_str());
            if (function->isMethod) {
                std::string superType = currentStruct.super;
                if (superType.size() > 0) {
                    append("%s Var_super = (%s) Var_self;\n", sclTypeToCType(result, superType).c_str(), sclTypeToCType(result, superType).c_str());
                    varScopeTop().push_back(Variable("super", "const " + superType));
                }
            }

            if (!function->isMethod && !Main::options::noMain && function->name == "main") {
                if (function->args.size()) {
                    if (!Main::options::noScaleFramework) {
                        append("scl_str* Var_%s = _scl_new_array_by_size(__argc, sizeof(scl_str));\n", function->args[0].name.c_str());
                    } else {
                        append("scl_int8** Var_%s = _scl_new_array_by_size(__argc, sizeof(scl_int8*));\n", function->args[0].name.c_str());
                    }
                    append("for (scl_int i = 0; i < __argc; i++) {\n");
                    if (!Main::options::noScaleFramework) {
                        append("  Var_%s[i] = _scl_create_string(__argv[i]);\n", function->args[0].name.c_str());
                    } else {
                        append("  Var_%s[i] = __argv[i];\n", function->args[0].name.c_str());
                    }
                    append("}\n");
                }
            }

            for (ssize_t a = (ssize_t) function->args.size() - 1; a >= 0; a--) {
                const Variable& arg = function->args[a];
                if (arg.name.size() == 0) continue;
                if (typeCanBeNil(arg.type)) continue;
                if (function->isMethod && arg.name == "self") continue;
                if (arg.type == "varargs") continue;

                append("SCL_ASSUME(*(scl_int*) &Var_%s, \"Argument '%%s' is nil\", \"%s\");\n", arg.name.c_str(), arg.name.c_str());
            }
            
            if (function->has_unsafe) {
                isInUnsafe++;
            }
            if (function->has_restrict) {
                if (!Main::options::noScaleFramework) {
                    transpilerErrorTok("Function '" + function->name + "' has restrict, but Scale Framework is disabled", function->name_token);
                    errors.push_back(err);
                    continue;
                }
                if (function->isMethod) {
                    append("SCL_FUNCTION_LOCK((scl_SclObject) Var_self);\n");
                } else {
                    append("SCL_FUNCTION_LOCK(function_lock$%s);\n", function->finalName().c_str());
                }
            }

            if (function->isCVarArgs() && function->varArgsParam().name.size()) {
                append("va_list _cvarargs;\n");
                append("va_start(_cvarargs, Var_%s$size);\n", function->varArgsParam().name.c_str());
                append("scl_int _cvarargs_count = Var_%s$size;\n", function->varArgsParam().name.c_str());
                append("scl_any* Var_%s SCL_AUTO_DELETE = (scl_any*) _scl_cvarargs_to_array(_cvarargs, _cvarargs_count);\n", function->varArgsParam().name.c_str());
                append("va_end(_cvarargs);\n");
                varScopeTop().push_back(Variable(function->varArgsParam().name, "const [any]"));
            }

            if (function->has_setter || function->has_getter) {
                std::string fieldName;
                if (function->has_setter) {
                    fieldName = function->getModifier(function->has_setter + 1);
                } else {
                    fieldName = function->getModifier(function->has_getter + 1);
                }
                append("#define Var_field (Var_self->%s)\n", fieldName.c_str());

                varScopeTop().push_back(Variable("field", currentStruct.getMember(fieldName).type));
            }

            for (i = 0; i < body.size(); i++) {
                handle(Token);
            }

            if (function->has_setter || function->has_getter) {
                append("#undef Var_field\n");
            }
            
            if (function->has_unsafe) {
                isInUnsafe--;
            }

            if (!function->isMethod && !Main::options::noMain && function->name == "main") {
                append("return 0;\n");
            }

            scopeDepth = 0;
            append("}\n\n");
        }
    
        filePostamble();

        fclose(fp);
    }
} // namespace sclc
