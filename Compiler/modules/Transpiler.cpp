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
    bool noWarns;
    int isInUnsafe = 0;
    std::stack<std::string> typeStack;
    std::string return_type = "";
    int lambdaCount = 0;

    void ConvertC::writeHeader(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns) {
        (void) errors;
        (void) warns;
        int scopeDepth = 0;

        append("#include <scale_runtime.h>\n");
        for (std::string& header : Main.frameworkNativeHeaders) {
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
                if (args.size() > 0) {
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
                append("%s fn_%s(%s)\n", return_type.c_str(), function->finalName().c_str(), arguments.c_str());
                append("    __asm(%s);\n", symbol.c_str());
                if (function->has_export) {
                    if (function->member_type.size()) {
                        if (hasMethod(result, function->name, function->member_type)) {
                            FPResult res;
                            res.success = false;
                            res.message = "Function '" + function->member_type + "::" + function->name + "' conflicts with method '" + function->member_type + ":" + function->name + "'";
                            res.line = function->name_token.line;
                            res.in = function->name_token.file;
                            res.column = function->name_token.column;
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
                        res.line = function->name_token.line;
                        res.in = function->name_token.file;
                        res.column = function->name_token.column;
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
        append("/* STRUCT TYPES */\n");
        for (Struct& c : result.structs) {
            if (c.name == "str" || c.name == "any" || c.name == "int" || c.name == "float" || isPrimitiveIntegerType(c.name)) continue;
            append("typedef struct Struct_%s* scl_%s;\n", c.name.c_str(), c.name.c_str());
        }
        append("\n");
        for (Layout& c : result.layouts) {
            append("typedef struct Layout_%s* scl_%s;\n", c.name.c_str(), c.name.c_str());
        }
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
            if (c.name == "str" || c.name == "any" || c.name == "int" || c.name == "float" || isPrimitiveIntegerType(c.name)) continue;
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
                    res.line = t.line;
                    res.in = t.file;
                    res.column = t.column;
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
                        res.line = t.line;
                        res.in = t.file;
                        res.column = t.column;
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
                        res.line = t.line;
                        res.in = t.file;
                        res.column = t.column;
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
                        res.line = t.line;
                        res.in = t.file;
                        res.column = t.column;
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
                        res.line = t.line;
                        res.in = t.file;
                        res.column = t.column;
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
                        res.line = t.line;
                        res.in = t.file;
                        res.column = t.column;
                        res.value = t.value;
                        res.type = t.type;
                        errors.push_back(res);
                        FPResult res2;
                        Token t2 = other->name_token;
                        res2.success = false;
                        res2.isNote = true;
                        res2.message = "Declared here:";
                        res2.line = t2.line;
                        res2.in = t2.file;
                        res2.column = t2.column;
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
                    res.line = s.name_token->line;
                    res.in = s.name_token->file;
                    res.column = s.name_token->column;
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
                        res.line = s.name_token->line;
                        res.in = s.name_token->file;
                        res.column = s.name_token->column;
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
                append("struct Struct_%s {\n", c.name.c_str());
                append("  const _scl_lambda* const $fast;\n");
                append("  const TypeInfo* $statics;\n");
                append("  mutex_t $mutex;\n");
                
                fprintf(scale_header, "struct Struct_%s {\n", c.name.c_str());
                fprintf(scale_header, "  const _scl_lambda* const $fast;\n");
                fprintf(scale_header, "  const TypeInfo* $statics;\n");
                fprintf(scale_header, "  mutex_t $mutex;\n");
                
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
                expect(body[i].type == tok_identifier || body[i].type == tok_column, "Expected identifier for argument name, but got '" + body[i].value + "'");
                
                std::string name = body[i].value;
                std::string type = "any";
                if (body[i].type == tok_column) {
                    name = "";
                } else {
                    safeInc();
                }
                incrementAndExpect(body[i].type == tok_column, "A type is required");
                FPResult r = parseType(body, &i, getTemplates(result, function));
                if (!r.success) {
                    errors.push_back(r);
                    continue;
                }
                type = r.value;
                expect(type != "none" && type != "nothing", "Type 'none' is only valid for function return types.");
                f->addArgument(Variable(name, type));

                incrementAndExpect(body[i].type == tok_comma || body[i].type == tok_paren_close, "Expected ',' or ')', but got '" + body[i].value + "'");
                if (body[i].type == tok_comma) {
                    safeInc();
                }
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

        append("(_stack.sp++)->v = ({\n");
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
        expect(function->return_type != "nothing", "Returning from a function with return type 'nothing' is not allowed.");
        if (!typeCanBeNil(function->return_type) && function->return_type != "none") {
            warnIf(true, "Return-if-nil operator '?' behaves like assert-not-nil operator '!!' in not-nil returning function.");
            append("_scl_assert((*(scl_int*) (_stack.sp - 1)), \"Not nil assertion failed!\");\n");
        } else {
            if (function->return_type == "none")
                append("if ((*(scl_int*) (_stack.sp - 1)) == 0) return;\n");
            else {
                append("if ((*(scl_int*) (_stack.sp - 1)) == 0) return 0;\n");
            }
        }
    }

    handler(Catch) {
        noUnused;
        std::string ex = "Exception";
        incrementAndExpectWithNote(
            body[i].value == "typeof",
            "Generic Exception caught here:",
            "Add 'typeof Exception' here to fix this:\\insertText; typeof Exception;" + std::to_string(err.line) + ":" + std::to_string(err.column + body[i].value.size())
        );
        safeInc("Expected type name after 'typeof'");
        varScopePop();
        scopeDepth--;
        if (wasTry()) {
            popTry();
            append("  _scl_exception_drop();\n");
        } else if (wasCatch()) {
            popCatch();
        }

        varScopePush();
        pushCatch();
        Struct s = getStructByName(result, body[i].value);
        expect(s != Struct::Null, "Trying to catch unknown Exception of type '" + body[i].value + "'");
        
        append("} else if (_scl_is_instance_of(*(_stack.ex), 0x%xU)) {\n", id(body[i].value.c_str()));
        std::string exName = "exception";
        if (body.size() > i + 1 && body[i + 1].value == "as") {
            safeInc();
            safeInc("Expected identifier after 'as'");
            exName = body[i].value;
        }
        scopeDepth++;
        varScopeTop().push_back(Variable(exName, ex));
        append("scl_%s Var_%s = (scl_%s) *(_stack.ex);\n", ex.c_str(), exName.c_str(), ex.c_str());
    }

    handler(Pragma) {
        noUnused;
        safeInc();
        if (body[i].value == "print") {
            safeInc();
            std::cout << body[i].file
                      << ":"
                      << body[i].line
                      << ":"
                      << body[i].column
                      << ": "
                      << body[i].value
                      << std::endl;
        } else if (body[i].value == "stackalloc") {
            safeInc();
            std::string length = body[i].value;
            long long len = std::stoll(length);
            expect(len > 0, "Can only stackalloc a positive amount of stack frames, but got '" + length + "'");

            append("{\n");
            scopeDepth++;
            append("scl_any* _stackalloc_ptr = (scl_any*) _stack.sp;\n");
            append("_scl_stack_resize_fit(%s);\n", length.c_str());
            append("_scl_add_stackallocation(_stackalloc_ptr, %s);\n", length.c_str());
            append("(_stack.sp++)->v = _stackalloc_ptr;\n");
            scopeDepth--;
            append("}\n");
            typeStack.push("[any]");
        } else if (body[i].value == "unset") {
            safeInc();

            std::string var = body[i].value;
            expect(hasVar(var), "Trying to unset unknown variable '" + var + "'");

            Variable v = getVar(var);
            expect(!v.isConst, "Trying to unset constant variable '" + var + "'");
            expect(v.type.size() > 2 && v.type.front() == '[' && v.type.back() == ']', "Trying to unset non-reference variable '" + var + "'");

            if (removeTypeModifiers(v.type) == "float") {
                append("*Var_%s = 0.0;\n", var.c_str());
            } else {
                append("*Var_%s = NULL;\n", var.c_str());
            }
        }
    }

    handler(Identifier) {
        noUnused;
        if (body[i].value == "drop") {
            append("--_stack.sp;\n");
            typePop;
        } else if (body[i].value == "!!") {
            warnIf(!typeCanBeNil(typeStackTop), "Unnecessary assert-not-nil operator '!!' on not-nil type '" + typeStackTop + "'");
            if (typeCanBeNil(typeStackTop)) {
                std::string type = typeStackTop;
                typePop;
                typeStack.push(type.substr(0, type.size() - 1));
                append("_scl_assert((*(scl_int*) (_stack.sp - 1)), \"Not nil assertion failed!\");\n");
            } else {
                append("_scl_assert((*(scl_int*) (_stack.sp - 1)), \"Not nil assertion failed! If you see this, something has gone very wrong!\");\n");
            }
        } else if (body[i].value == "?") {
            handle(ReturnOnNil);
        } else if (body[i].value == "++") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "++", typeStackTop)) return;
            append("(*(scl_int*) (_stack.sp - 1))++;\n");
            if (typeStack.size() == 0)
                typeStack.push("int");
        } else if (body[i].value == "--") {
            if (handleOverriddenOperator(result, fp, scopeDepth, "--", typeStackTop)) return;
            append("--(*(scl_int*) (_stack.sp - 1));\n");
            if (typeStack.size() == 0)
                typeStack.push("int");
        } else if (body[i].value == "exit") {
            append("exit(*(scl_int*) --_stack.sp);\n");
            typePop;
        } else if (body[i].value == "abort") {
            append("abort();\n");
        } else if (body[i].value == "typeof") {
            safeInc();
            auto templates = currentStruct.templates;
            if (hasVar(body[i].value)) {
                const Struct& s = getStructByName(result, getVar(body[i].value).type);
                if (s != Struct::Null && !s.isStatic()) {
                    append("*(scl_str*) (_stack.sp++) = _scl_create_string(Var_%s->$statics->type_name);\n", body[i].value.c_str());
                } else {
                    append("*(scl_str*) (_stack.sp++) = _scl_create_string(_scl_find_index_of_struct(*(scl_any*) &Var_%s) != -1 ? ((scl_SclObject) Var_%s)->$statics->type_name : \"%s\");\n", getVar(body[i].value).name.c_str(), getVar(body[i].value).name.c_str(), getVar(body[i].value).type.c_str());
                }
            } else if (hasFunction(result, body[i].value)) {
                Function* f = getFunctionByName(result, body[i].value);
                std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
                append("*(scl_str*) (_stack.sp++) = _scl_create_string(\"%s\");\n", lambdaType.c_str());
            } else if (function->isMethod && templates.find(body[i].value) != templates.end()) {
                append("*(scl_str*) (_stack.sp++) = _scl_create_string(Var_self->$template_argname_%s);\n", body[i].value.c_str());
            } else if (body[i].type == tok_paren_open) {
                append("{\n");
                scopeDepth++;
                size_t stack_start = typeStack.size();
                handle(ParenOpen);
                
                expect(typeStack.size() > stack_start, "Expected expression evaluating to a value after 'typeof'");

                const Struct& s = getStructByName(result, typeStackTop);
                if (s != Struct::Null && !s.isStatic()) {
                    append("*(scl_str*) (_stack.sp - 1) = _scl_create_string((_stack.sp - 1)->o->$statics->type_name);\n");
                } else {
                    append("*(scl_str*) (_stack.sp - 1) = _scl_create_string(_scl_find_index_of_struct((_stack.sp - 1)->v) != -1 ? (_stack.sp - 1)->o->$statics->type_name : \"%s\");\n", typeStackTop.c_str());
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
                append("*(scl_str*) (_stack.sp++) = _scl_create_string(\"%s\");\n", res.value.c_str());
            }
            typeStack.push("str");
        } else if (body[i].value == "nameof") {
            safeInc();
            expect(hasVar(body[i].value), "Unknown Variable: '" + body[i].value + "'");
            append("*(scl_str*) (_stack.sp++) = _scl_create_string(\"%s\");\n", body[i].value.c_str());
            typeStack.push("str");
        } else if (body[i].value == "sizeof") {
            safeInc();
            auto templates = getTemplates(result, function);
            if (body[i].value == "int") {
                append("(_stack.sp++)->i = sizeof(scl_int);\n");
            } else if (body[i].value == "float") {
                append("(_stack.sp++)->i = sizeof(scl_float);\n");
            } else if (body[i].value == "str") {
                append("(_stack.sp++)->i = sizeof(scl_str);\n");
            } else if (body[i].value == "any") {
                append("(_stack.sp++)->i = sizeof(scl_any);\n");
            } else if (body[i].value == "none" || body[i].value == "nothing") {
                append("(_stack.sp++)->i = 0;\n");
            } else if (hasVar(body[i].value)) {
                append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, getVar(body[i].value).type).c_str());
            } else if (getStructByName(result, body[i].value) != Struct::Null) {
                append("(_stack.sp++)->i = sizeof(struct Struct_%s);\n", body[i].value.c_str());
            } else if (hasTypealias(result, body[i].value)) {
                append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, body[i].value).c_str());
            } else if (hasLayout(result, body[i].value)) {
                append("(_stack.sp++)->i = sizeof(struct Layout_%s);\n", body[i].value.c_str());
            } else if (templates.find(body[i].value) != templates.end()) {
                std::string replaceWith = templates[body[i].value];
                if (getStructByName(result, replaceWith) != Struct::Null)
                    append("(_stack.sp++)->i = sizeof(struct Struct_%s);\n", replaceWith.c_str());
                else if (hasTypealias(result, replaceWith))
                    append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, replaceWith).c_str());
                else if (hasLayout(result, replaceWith))
                    append("(_stack.sp++)->i = sizeof(struct Layout_%s);\n", replaceWith.c_str());
                else
                    append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, replaceWith).c_str());
            } else {
                FPResult type = parseType(body, &i, getTemplates(result, function));
                if (!type.success) {
                    transpilerError("Unknown Type: '" + body[i].value + "'", i);
                    errors.push_back(err);
                    return;
                }
                append("(_stack.sp++)->i = sizeof(%s);\n", sclTypeToCType(result, type.value).c_str());
            }
            typeStack.push("int");
        } else if (body[i].value == "try") {
            append("_scl_exception_push();\n");
            append("if (setjmp(*(_stack.jmp - 1)) != 666) {\n");
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
            incrementAndExpect(body[i].type == tok_paren_open, "Expected '(' after 'assert', but got '" + body[i].value + "'");

            append("{\n");
            varScopePush();
            scopeDepth++;
            size_t stack_start = typeStack.size();
            handle(ParenOpen);
            expect(stack_start < typeStack.size(), "Expected expression returning a value after 'assert'");

            if (i + 1 < body.size() && body[i + 1].type == tok_else) {
                safeInc();
                incrementAndExpect(body[i].type == tok_paren_open, "Expected '(' after 'else', but got '" + body[i].value + "'");

                append("if (!(*(scl_int*) (_stack.sp - 1))) {\n");
                handle(ParenOpen);
                append("}\n");
            } else {
                append("_scl_assert((*(scl_int*) (_stack.sp - 1)), \"Assertion at %s:%d:%d failed!\");\n", assertToken.file.c_str(), assertToken.line, assertToken.column);
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
            expect(!f->isMethod, "'" + f->name + "' is a method, not a function.");
            functionCall(f, fp, result, warns, errors, body, i);
        } else if (hasFunction(result, function->member_type + "$" + body[i].value)) {
            Function* f = getFunctionByName(result, function->member_type + "$" + body[i].value);
            if (f->isCVarArgs()) {
                createVariadicCall(f, fp, result, errors, body, i);
                return;
            }
            expect(!f->isMethod, "'" + f->name + "' is a method, not a function.");
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
                        handle(Token);
                        expect(removeTypeModifiers(typeStackTop) == "str", "Expected expression resulting in 'str' type, but got '" + typeStackTop + "'");
                        auto nth = nthTemplate(s, templates.size());
                        append("scl_str $template%s = *(scl_str*) --_stack.sp;\n", nth.first.c_str());
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
                        expect(type.value.size() <= 2 || (type.value.front() != '[' && type.value.back() != ']'), "Expected scalar type, but got '" + type.value + "'");
                        if (t.second != "any") {
                            Struct f = Struct::Null;
                            if (isPrimitiveIntegerType(t.second)) {
                                expect(isPrimitiveIntegerType(type.value), "Expected integer type, but got '" + type.value + "'");
                            } else if ((f = getStructByName(result, t.second)) != Struct::Null) {
                                Struct g = getStructByName(result, type.value);
                                expect(g != Struct::Null, "Expected struct type, but got '" + type.value + "'");
                                expect(structExtends(result, g, f.name), "Struct '" + g.name + "' is not convertible to '" + f.name + "'");
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
                    expect(hasInitMethod && !initMethod->has_private, "Direct instantiation of struct '" + s.name + "' is not allowed");
                }
                if (body[i].value == "new" && !s.isStatic()) {
                    append("(_stack.sp++)->v = ({\n");
                    scopeDepth++;
                    std::string ctype = sclTypeToCType(result, s.name);
                    append("%s tmp = ALLOC(%s);\n", ctype.c_str(), s.name.c_str());
                    for (auto&& t : templates) {
                        if (t.second.front() == '$') {
                            append("tmp->$template_arg_%s = %s->hash;\n", t.first.c_str(), t.second.c_str());
                            append("tmp->$template_argname_%s = %s->data;\n", t.first.c_str(), t.second.c_str());
                        } else {
                            append("tmp->$template_arg_%s = 0x%xU;\n", t.first.c_str(), id(t.second.c_str()));
                            append("tmp->$template_argname_%s = (scl_int8*) \"%s\";\n", t.first.c_str(), t.second.c_str());
                        }
                    }
                    
                    typeStack.push(s.name);
                    if (hasInitMethod) {
                        append("*(%s*) (_stack.sp++) = tmp;\n", ctype.c_str());
                        methodCall(initMethod, fp, result, warns, errors, body, i);
                        typeStack.push(s.name);
                    }
                    append("tmp;\n");
                    scopeDepth--;
                    append("});\n");
                } else if (body[i].value == "default" && !s.isStatic()) {
                    append("(_stack.sp++)->v = ALLOC(%s);\n", s.name.c_str());
                    if (templates.size()) {
                        append("{\n");
                        scopeDepth++;
                        append("%s tmp = (%s) (_stack.sp - 1)->v;\n", sclTypeToCType(result, s.name).c_str(), sclTypeToCType(result, s.name).c_str());
                    }
                    for (auto&& t : templates) {
                        if (t.second.front() == '$') {
                            append("tmp->$template_arg_%s = %s->hash;\n", t.first.c_str(), t.second.c_str());
                            append("tmp->$template_argname_%s = %s->data;\n", t.first.c_str(), t.second.c_str());
                        } else {
                            append("tmp->$template_arg_%s = %d;\n", t.first.c_str(), id(t.second.c_str()));
                            append("tmp->$template_argname_%s = (scl_int8*) \"%s\";\n", t.first.c_str(), t.second.c_str());
                        }
                    }
                    if (templates.size()) {
                        scopeDepth--;
                        append("}\n");
                    }
                    typeStack.push(s.name);
                } else {
                    expect(
                        hasFunction(result, s.name + "$" + body[i].value) ||
                        hasGlobal(result, s.name + "$" + body[i].value),
                        "Unknown static member of struct '" + s.name + "'"
                    );
                    if (hasFunction(result, s.name + "$" + body[i].value)) {
                        Function* f = getFunctionByName(result, s.name + "$" + body[i].value);
                        if (f->isCVarArgs()) {
                            createVariadicCall(f, fp, result, errors, body, i);
                            return;
                        }
                        expect(!f->isMethod, "'" + f->name + "' is not static");
                        if (f->has_private && function->belongsToType(s.name)) {
                            expect(f->member_type == function->member_type, "'" + body[i].value + "' has private access in Struct '" + s.name + "'");
                        }
                        functionCall(f, fp, result, warns, errors, body, i);
                    } else {
                        Variable v = getVar(s.name + "$" + body[i].value);
                        
                        std::string lastType = "";
                        std::string path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, "", &lastType, /* doesWriteAfter */ false);
                        
                        LOAD_PATH(path, lastType);
                    }
                }
            } else if (body[i + 1].type == tok_curly_open) {
                safeInc();
                append("(_stack.sp++)->v = ({\n");
                scopeDepth++;
                append("scl_%s tmp = ALLOC(%s);\n", s.name.c_str(), s.name.c_str());
                for (auto&& t : templates) {
                    if (t.second.front() == '$') {
                        append("tmp->$template_arg_%s = %s->hash;\n", t.first.c_str(), t.second.c_str());
                        append("tmp->$template_argname_%s = %s->data;\n", t.first.c_str(), t.second.c_str());
                    } else {
                        append("tmp->$template_arg_%s = %d;\n", t.first.c_str(), id(t.second.c_str()));
                        append("tmp->$template_argname_%s = (scl_int8*) \"%s\";\n", t.first.c_str(), t.second.c_str());
                    }
                }
                append("_scl_frame_t* stack_start = _stack.sp;\n");
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
                    incrementAndExpect(body[i].type == tok_store, "Expected '=>', but got '" + body[i].value + "'");
                    incrementAndExpect(body[i].type == tok_identifier, "'" + body[i].value + "' is not an identifier");

                    std::string lastType = typeStackTop;

                    missedMembers = vecWithout(missedMembers, body[i].value);
                    const Variable& v = s.getMember(body[i].value);

                    expect(typesCompatible(result, lastType, v.type, true), "Incompatible types: '" + v.type + "' and '" + lastType + "'");

                    Method* mutator = attributeMutator(result, s.name, body[i].value);
                    
                    if (mutator) {
                        append("mt_%s$%s(tmp, *(%s*) _scl_pop());\n", mutator->member_type.c_str(), mutator->finalName().c_str(), sclTypeToCType(result, lastType).c_str());
                    } else {
                        append("tmp->%s = *(%s*) _scl_pop();\n", body[i].value.c_str(), sclTypeToCType(result, lastType).c_str());
                    }
                    typePop;
                    scopeDepth--;
                    append("}\n");
                    append("_stack.sp = stack_start;\n");
                    safeInc();
                    count++;
                }
                varScopePop();
                append("tmp;\n");
                scopeDepth--;
                append("});\n");
                typeStack.push(s.name);
                std::string missed = "{ ";
                for (size_t i = 0; i < missedMembers.size(); i++) {
                    if (i) {
                        missed += ", ";
                    }
                    missed += missedMembers[i];
                }
                missed += " }";
                expect(count >= membersToInitialize, "Not every member was initialized! Missed: " + missed);
            }
        } else if (hasLayout(result, body[i].value)) {
            const Layout& l = getLayout(result, body[i].value);
            incrementAndExpect(body[i].type == tok_double_column, "Expected '::', but got '" + body[i].value + "'");
            incrementAndExpect(body[i].value == "new", "Expected 'new' to create new layout, but got '" + body[i].value + "'");
            append("(_stack.sp++)->v = (scl_any) _scl_alloc(sizeof(struct Layout_%s));\n", l.name.c_str());
            typeStack.push(l.name);
        } else if (hasEnum(result, body[i].value)) {
            Enum e = getEnumByName(result, body[i].value);
            
            incrementAndExpect(body[i].type == tok_double_column, "Expected '::', but got '" + body[i].value + "'");
            incrementAndExpect(e.indexOf(body[i].value) != Enum::npos, "Unknown member of enum '" + e.name + "': '" + body[i].value + "'");

            append("(_stack.sp++)->i = %zuUL;\n", e.indexOf(body[i].value));
            typeStack.push("int");
        } else if (hasContainer(result, body[i].value)) {
            std::string containerName = body[i].value;
            incrementAndExpect(body[i].type == tok_dot, "Expected '.' to access container contents, but got '" + body[i].value + "'");
            safeInc("Expected container member name after '.'");

            std::string memberName = body[i].value;
            Container container = getContainerByName(result, containerName);

            expect(container.hasMember(memberName), "Unknown container member: '" + memberName + "'");
            
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
                append("(_stack.sp++)->i = (scl_int) Var_self;\n");
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
                body.insert(body.begin() + i, Token(tok_dot, ".", here.line, here.file));
                body.insert(body.begin() + i, Token(tok_identifier, "self", here.line, here.file));
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
            expect(false, "Unknown identifier: '" + body[i].value + "'");
        }
    }

    handler(New) {
        noUnused;
        std::string elemSize = "sizeof(scl_any)";
        std::string typeString = "any";
        if (i + 1 < body.size() && body[i + 1].type == tok_identifier && body[i + 1].value == "<") {
            safeInc("Expected '<' after 'new'");
            safeInc("Expected type after '<'");
            auto type = parseType(body, &i);
            if (!type.success) {
                errors.push_back(type);
                return;
            }
            typeString = removeTypeModifiers(type.value);
            elemSize = "sizeof(" + sclTypeToCType(result, type.value) + ")";
            safeInc();
        }
        incrementAndExpect(body[i].type == tok_bracket_open, "Expected '[' after 'new'");
        int dimensions = 0;
        std::vector<int> startingOffsets;
        while (body[i].type == tok_bracket_open) {
            dimensions++;
            startingOffsets.push_back(i);
            
            incrementAndExpect(body[i].type != tok_bracket_close, "Empty array dimensions are not allowed");

            while (body[i].type != tok_bracket_close) {
                handle(Token);
                safeInc();
            }
            safeInc();
        }
        i--;
        
        if (dimensions == 1) {
            append("(_stack.sp - 1)->v = _scl_new_array_by_size((*(scl_int*) (_stack.sp - 1)), %s);\n", elemSize.c_str());
        } else {
            append("{\n");
            scopeDepth++;
            std::string dims = "";
            for (int i = 0; i < dimensions; i++) {
                append("scl_int _scl_dim%d = *(scl_int*) _scl_pop();\n", i);
                if (i > 0) dims += ", ";
                dims += "_scl_dim" + std::to_string(i);
                expectWithPos(isPrimitiveIntegerType(typeStackTop), "Array size must be an integer, but got '" + removeTypeModifiers(typeStackTop) + "'", startingOffsets[i]);
                typePop;
            }

            append("scl_int tmp[] = {%s};\n", dims.c_str());
            append("(_stack.sp++)->v = _scl_multi_new_array_by_size(%d, tmp, %s);\n", dimensions, elemSize.c_str());
            scopeDepth--;
            append("}\n");
        }
        typeStack.push("[" + typeString + "]");
    }

    handler(Repeat) {
        noUnused;
        incrementAndExpect(body[i].type == tok_number, "Expected integer, but got '" + body[i].value + "'");
        expect(parseNumber(body[i].value) > 0, "Repeat count must be greater than 0");
        incrementAndExpect(body[i].type == tok_do, "Expected 'do', but got '" + body[i].value + "'");

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
        safeInc("Expected statement after 'do'");
    }

    handler(For) {
        noUnused;
        incrementAndExpect(body[i].type == tok_identifier, "Expected identifier, but got: '" + body[i].value + "'");
        Token var = body[i];
        incrementAndExpect(body[i].type == tok_in, "Expected 'in' keyword in for loop header, but got: '" + body[i].value + "'");
        safeInc("Expected expression after 'in'");
        std::string var_prefix = "";
        if (!hasVar(var.value)) {
            var_prefix = "scl_int ";
        }
        append("for (%sVar_%s = ({\n", var_prefix.c_str(), var.value.c_str());
        scopeDepth++;
        while (body[i].type != tok_to) {
            handle(Token);
            safeInc("Expected expression after 'in'");
        }
        expect(body[i].type == tok_to, "Expected 'to' keyword in for loop header, but got: '" + body[i].value + "'");
        safeInc("Expected expression after 'to'");
        append("*(scl_int*) --_stack.sp;\n");
        typePop;
        scopeDepth--;
        append("}); Var_%s != ({\n", var.value.c_str());
        scopeDepth++;
        while (body[i].type != tok_step && body[i].type != tok_do) {
            handle(Token);
            safeInc("Expected expression after 'to'");
        }
        typePop;
        std::string iterator_direction = "++";
        if (body[i].type == tok_step) {
            safeInc("Expected expression after 'step'");
            expect(body[i].type != tok_do, "Expected step value");
            std::string val = body[i].value;
            if (val == "+") {
                safeInc();
                iterator_direction = " += ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else if (body[i].value == "+") {
                    iterator_direction = "++";
                } else {
                    expect(false, "Expected number or '+', but got '" + body[i].value + "'");
                }
            } else if (val == "-") {
                safeInc();
                iterator_direction = " -= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else if (body[i].value == "-") {
                    iterator_direction = "--";
                } else {
                    expect(false, "Expected number or '-', but got '" + body[i].value + "'");
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
                    expect(false, "Expected number, but got '" + body[i].value + "'");
                }
            } else if (val == "/") {
                safeInc();
                iterator_direction = " /= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else {
                    expect(false, "Expected number, but got '" + body[i].value + "'");
                }
            } else if (val == "<<") {
                safeInc();
                iterator_direction = " <<= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else {
                    expect(false, "Expected number, but got '" + body[i].value + "'");
                }
            } else if (val == ">>") {
                safeInc();
                iterator_direction = " >>= ";
                if (body[i].type == tok_number) {
                    iterator_direction += body[i].value;
                } else {
                    expect(false, "Expected number, but got '" + body[i].value + "'");
                }
            } else if (val == "nop") {
                iterator_direction = "";
            }
            safeInc();
        }
        expect(body[i].type == tok_do, "Expected 'do', but got '" + body[i].value + "'");

        append("*(scl_int*) --_stack.sp;\n");
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

        warnIf(hasFunction(result, var.value), "Variable '" + var.value + "' shadowed by function '" + var.value + "'");
        warnIf(getStructByName(result, var.value) != Struct::Null, "Variable '" + var.value + "' shadowed by struct '" + var.value + "'");
        warnIf(hasLayout(result, var.value), "Variable '" + var.value + "' shadowed by layout '" + var.value + "'");
        warnIf(hasEnum(result, var.value), "Variable '" + var.value + "' shadowed by enum '" + var.value + "'");
        warnIf(hasContainer(result, var.value), "Variable '" + var.value + "' shadowed by container '" + var.value + "'");

        pushOther();
    }

    handler(Foreach) {
        noUnused;
        incrementAndExpect(body[i].type == tok_identifier, "Expected identifier, but got '" + body[i].value + "'");
        std::string iterator_name = "__it" + std::to_string(iterator_count++);
        Token iter_var_tok = body[i];
        
        incrementAndExpect(body[i].type == tok_in, "Expected 'in', but got '" + body[i].value + "'");
        incrementAndExpect(body[i].type != tok_do, "Expected iterable, but got '" + body[i].value + "'");
        while (body[i].type != tok_do) {
            handle(Token);
            safeInc("Expected iterable, but got '" + body[i].value + "'");
        }
        pushIterate();

        std::string type = removeTypeModifiers(typeStackTop);
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            append("%s %s = (%s) *(scl_int*) --_stack.sp;\n", sclTypeToCType(result, typeStackTop).c_str(), iterator_name.c_str(), sclTypeToCType(result, typeStackTop).c_str());
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
        expect(s != Struct::Null, "Can only iterate over structs and arrays, but got '" + typeStackTop + "'");
        expect(structImplements(result, s, "Iterable"), "Struct '" + typeStackTop + "' is not iterable");
        
        Method* iterateMethod = getMethodByName(result, "iterate", typeStackTop);
        expect(iterateMethod, "Could not find method 'iterate' on type '" + typeStackTop + "'");
        
        methodCall(iterateMethod, fp, result, warns, errors, body, i);
        append("%s %s = (%s) *(scl_int*) --_stack.sp;\n", sclTypeToCType(result, typeStackTop).c_str(), iterator_name.c_str(), sclTypeToCType(result, typeStackTop).c_str());
        type = typeStackTop;
        typePop;
        
        Method* nextMethod = getMethodByName(result, "next", type);
        Method* hasNextMethod = getMethodByName(result, "hasNext", type);
        expect(hasNextMethod, "Could not find method 'hasNext' on type '" + type + "'");
        expect(nextMethod, "Could not find method 'next' on type '" + type + "'");

        varScopePush();
        varScopeTop().push_back(Variable(iter_var_tok.value, nextMethod->return_type));
        std::string cType = sclTypeToCType(result, getVar(iter_var_tok.value).type);
        append("while (virtual_call(%s, \"hasNext()i;\")) {\n", iterator_name.c_str());
        scopeDepth++;
        append("%s Var_%s = (%s) virtual_call(%s, \"next%s\");\n", sclTypeToCType(result, nextMethod->return_type).c_str(), iter_var_tok.value.c_str(), cType.c_str(), iterator_name.c_str(), argsToRTSignature(nextMethod).c_str());
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
            expect(!f->isCVarArgs(), "Cannot take reference of varargs function '" + f->name + "'");

            append("(_stack.sp++)->i = (scl_int) &fn_%s;\n", f->finalName().c_str());
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
                        expect(!f->isCVarArgs(), "Cannot take reference of varargs function '" + f->name + "'");
                        expect(!f->isMethod, "'" + f->name + "' is not static");

                        append("(_stack.sp++)->i = (scl_int) &fn_%s;\n", f->finalName().c_str());
                        std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
                        typeStack.push(lambdaType);
                        return;
                    } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                        std::string loadFrom = s.name + "$" + body[i].value;
                        v = getVar(loadFrom);
                    } else {
                        expect(false, "Struct '" + s.name + "' has no static member named '" + body[i].value + "'");
                    }
                }
            } else if (hasContainer(result, body[i].value)) {
                Container c = getContainerByName(result, body[i].value);
                incrementAndExpect(body[i].type == tok_dot, "Expected '.' to access container contents, but got '" + body[i].value + "'");
                incrementAndExpect(body[i].type == tok_identifier, "Expected container member name after '.'");
                std::string memberName = body[i].value;
                expect(c.hasMember(memberName), "Unknown container member: '" + memberName + "'");
                
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
                expect(false, "Use of undefined variable '" + body[i].value + "'");
            }
        } else if (hasVar(body[i].value) && body[i + 1].type == tok_double_column) {
            Struct s = getStructByName(result, getVar(body[i].value).type);
            safeInc();
            safeInc();
            if (s != Struct::Null) {
                if (hasMethod(result, body[i].value, s.name)) {
                    Method* f = getMethodByName(result, body[i].value, s.name);
                    expect(f->isMethod, "'" + f->name + "' is static");

                    append("(_stack.sp++)->i = (scl_int) &mt_%s$%s;\n", s.name.c_str(), f->finalName().c_str());
                    std::string lambdaType = "lambda(" + std::to_string(f->args.size()) + "):" + f->return_type;
                    typeStack.push(lambdaType);
                    return;
                } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                    std::string loadFrom = s.name + "$" + body[i].value;
                    v = getVar(loadFrom);
                } else {
                    expect(false, "Struct '" + s.name + "' has no static member named '" + body[i].value + "'");
                }
            }
        } else {
            v = getVar(body[i].value);
        }
        path = makePath(result, v, /* topLevelDeref */ false, body, i, errors, variablePrefix, &lastType, /* doesWriteAfter */ false);
        append("(_stack.sp++)->i = (scl_int) &(%s);\n", path.c_str());
        typeStack.push("[" + lastType + "]");
    }

    handler(Store) {
        noUnused;
        safeInc("Expected '(', 'decl' or expression after '=>'");

        if (body[i].type == tok_paren_open) {
            append("{\n");
            scopeDepth++;
            append("scl_Array tmp = (scl_Array) *(scl_int*) --_stack.sp;\n");
            typePop;
            safeInc("Expected at least one declared variable name after '('");
            int destructureIndex = 0;
            while (body[i].type != tok_paren_close) {
                if (body[i].type == tok_comma) {
                    safeInc("Expected variable name after ','");
                    continue;
                }
                if (body[i].type == tok_paren_close)
                    break;
                
                if (body[i].type == tok_addr_of) {
                    safeInc("Expected variable name after '@'");
                    if (hasContainer(result, body[i].value)) {
                        std::string containerName = body[i].value;
                        incrementAndExpect(body[i].type == tok_dot, "Expected '.' to access container contents, but got '" + body[i].value + "'");
                        incrementAndExpect(body[i].type == tok_identifier, "Expected container member name after '.'");
                        std::string memberName = body[i].value;
                        Container container = getContainerByName(result, containerName);
                        expect(container.hasMember(memberName), "Unknown container member: '" + memberName + "'");
                        expect(container.getMember(memberName).isWritableFrom(function), "Container member variable '" + memberName + "' is not mutable");
                        
                        append("*((scl_any*) Container_%s.%s) = mt_Array$get(tmp, %d);\n", containerName.c_str(), memberName.c_str(), destructureIndex);
                    } else {
                        expect(body[i].type == tok_identifier, "Expected identifier, but got '" + body[i].value + "'");
                        
                        if (!hasVar(body[i].value)) {
                            expect(function->isMethod, "Use of undefined variable '" + body[i].value + "'");
                            
                            Method* m = ((Method*) function);
                            Struct s = getStructByName(result, m->member_type);
                            if (s.hasMember(body[i].value)) {
                                Variable mem = s.getMember(body[i].value);
                                expect(mem.isWritableFrom(function), "Member variable '" + body[i].value + "' is not mutable");
                                
                                append("*(scl_any*) Var_self->%s = mt_Array$get(tmp, %d);\n", body[i].value.c_str(), destructureIndex);
                                destructureIndex++;
                                continue;
                            } else if (hasGlobal(result, s.name + "$" + body[i].value)) {
                                Variable mem = getVar(s.name + "$" + body[i].value);
                                expect(mem.isWritableFrom(function), "Static variable '" + s.name + "::" + body[i].value + "' is not mutable");

                                append("*(scl_any*) Var_%s$%s = mt_Array$get(tmp, %d);\n", s.name.c_str(), body[i].value.c_str(), destructureIndex);
                                destructureIndex++;
                                continue;
                            }
                        }
                        const Variable& v = getVar(body[i].value);
                        std::string loadFrom = v.name;
                        if (getStructByName(result, v.type) != Struct::Null) {
                            expect(v.isWritableFrom(function), "Variable '" + body[i].value + "' is not mutable");
                            safeInc("Expected '.' after variable name");
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
                append("SCL_ASSUME((*(scl_int*) (_stack.sp - 1)), \"Nil cannot be stored in non-nil variable '%%s'!\", \"%s\");\n", varScopeTop().back().name.c_str());
            }
            if (!typesCompatible(result, typeStackTop, type, true)) {
                transpilerError("Incompatible types: '" + type + "' and '" + typeStackTop + "'", i);
                warns.push_back(err);
            }
            append("%s Var_%s = *(%s*) _scl_pop();\n", sclTypeToCType(result, type).c_str(), name.c_str(), sclTypeToCType(result, type).c_str());
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
                        append("scl_int _scl_value_to_store = *(scl_int*) --_stack.sp;\n");
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
                        append("*(scl_int*) *(scl_int*) --_stack.sp = _scl_value_to_store;\n");
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
                    body.insert(body.begin() + i, Token(tok_dot, ".", here.line, here.file));
                    body.insert(body.begin() + i, Token(tok_identifier, "self", here.line, here.file));
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
                append("%s index = (%s) *(scl_int*) --_stack.sp;\n", sclTypeToCType(result, indexingType).c_str(), sclTypeToCType(result, indexingType).c_str());
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
                        append("_scl_array_check_bounds_or_throw((scl_any*) array, index);\n");
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
                            append("(_stack.sp++)->v = index;\n");
                            typeStack.push(indexType);
                            append("(_stack.sp++)->v = array;\n");
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
                    append("_scl_array_check_bounds_or_throw((scl_any*) array, index);\n");
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
                        append("(_stack.sp++)->v = index;\n");
                        typeStack.push(indexType);
                        append("(_stack.sp++)->v = value;\n");
                        typeStack.push(valueType);
                        append("(_stack.sp++)->v = array;\n");
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
                append("SCL_ASSUME((*(scl_int*) (_stack.sp - 1)), \"Nil cannot be stored in non-nil variable '%%s'!\", \"%s\");\n", v.name.c_str());
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
        append("(_stack.sp++)->v = ({\n");
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
                        append("scl_int array_size = _scl_array_size((scl_any*) (_stack.sp - 1)->v);\n");
                    } else {
                        append("scl_int array_size = *(scl_int*) --_stack.sp;\n");
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
                    append("scl_int* array = _scl_new_array(array_size);\n");
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
                append("array[i] = *(scl_int*) --_stack.sp;\n");
                scopeDepth--;
                append("}\n");
                append("(_stack.sp++)->v = array;\n");
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
                    append("_scl_array_check_bounds_or_throw((scl_any*) (_stack.sp - 1)->v, %s);\n", body[i - 1].value.c_str());
                    append("(_stack.sp - 1)->v = (scl_any) ((%s) (_stack.sp - 1)->v)[%s];\n", sclTypeToCType(result, type).c_str(), body[i - 1].value.c_str());
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
            append("scl_int index = *(scl_int*) --_stack.sp;\n");
            append("_scl_array_check_bounds_or_throw((scl_any*) tmp, index);\n");
            append("(_stack.sp++)->v = (scl_any) tmp[index];\n");
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
            append("(_stack.sp++)->v = instance;\n");
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
        append("_scl_frame_t* begin_stack_size = _stack.sp;\n");
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
                append("_stack.sp = begin_stack_size;\n");
                while (typeStack.size() > stackSizeHere) {
                    typeStack.pop();
                }
                append("(_stack.sp++)->i = return_value->i;\n");
                typeStack.push(returns);
            }
        } else if (commas == 1) {
            Struct pair = getStructByName(result, "Pair");
            if (pair == Struct::Null) {
                transpilerError("Struct definition for 'Pair' not found", i);
                errors.push_back(err);
                return;
            }
            append("(_stack.sp++)->v = ({\n");
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
            append("(_stack.sp++)->v = ({\n");
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

    handler(StringLiteral) {
        noUnused;
        std::string str = unquote(body[i].value);
        ID_t hash = id(str.c_str());
        append("(_stack.sp++)->s = _scl_string_with_hash_len(\"%s\", 0x%x, %zu);\n", body[i].value.c_str(), hash, str.length());
        typeStack.push("str");
    }

    handler(CharStringLiteral) {
        noUnused;
        append("(_stack.sp++)->cs = \"%s\";\n", body[i].value.c_str());
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
        // append("const scl_int8* __prev = *(_stack.tp - 1);\n");
        // append("*(_stack.tp - 1) = \"<%s:native code>\";\n", function->name.c_str());
        std::string file = body[i].file;
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
        // append("*(_stack.tp - 1) = __prev;\n");
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
        append("(_stack.sp++)->i = %s;\n", stringValue.c_str());
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
        append("(_stack.sp++)->f = %s;\n", body[i].value.c_str());
        typeStack.push("float");
    }

    handler(FalsyType) {
        noUnused;
        append("(_stack.sp++)->i = (scl_int) 0;\n");
        if (body[i].type == tok_nil) {
            typeStack.push("any");
        } else {
            typeStack.push("bool");
        }
    }

    handler(TruthyType) {
        noUnused;
        append("(_stack.sp++)->i = (scl_int) 1;\n");
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
            append("(*(scl_int*) (_stack.sp - 1)) = 1;\n");
            typePop;
            typeStack.push("bool");
            return;
        }
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            append("(*(scl_int*) (_stack.sp - 1)) = _scl_is_array((_stack.sp - 1)->v);\n");
            typePop;
            typeStack.push("bool");
            return;
        }
        if (getStructByName(result, type) == Struct::Null && getInterfaceByName(result, type) == nullptr) {
            transpilerError("Usage of undeclared struct '" + body[i].value + "'", i);
            errors.push_back(err);
            return;
        }
        append("(*(scl_int*) (_stack.sp - 1)) = (_stack.sp - 1)->v && _scl_is_instance_of((_stack.sp - 1)->v, 0x%xU);\n", id(type.c_str()));
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
        append("*(scl_int*) --_stack.sp;\n");
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
        append("*(scl_int*) --_stack.sp;\n");
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
        append("*(scl_int*) --_stack.sp;\n");
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
        append("*(scl_int*) --_stack.sp;\n");
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
        append("*(scl_int*) --_stack.sp;\n");
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
                    append("  fn_Process$unlock((scl_SclObject) Var_self);\n");
                } else {
                    append("  fn_Process$unlock(function_lock$%s);\n", function->finalName().c_str());
                }
            }
            append("  fn_throw((scl_Exception) *(_stack.ex));\n");
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
            append("return;\n");
        } else {
            append("return ({\n");
            scopeDepth++;

            std::string templateType = "";
            std::string returningType = typeStackTop;
            if (function->namedReturnValue.name.size()) {
                returningType = function->namedReturnValue.type;
                append("%s retVal = Var_%s;\n", sclTypeToCType(result, returningType).c_str(), function->namedReturnValue.name.c_str());
                templateType = function->namedReturnValue.typeFromTemplate;
            } else {
                append("%s retVal = *(%s*) --_stack.sp;\n", sclTypeToCType(result, returningType).c_str(), sclTypeToCType(result, returningType).c_str());
                templateType = function->templateArg;
            }
            if (templateType.size()) {
                append("_scl_checked_cast(*(scl_any*) &retVal, Var_self->$template_arg_%s, Var_self->$template_argname_%s);\n", templateType.c_str(), templateType.c_str());
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
                        append("_scl_checked_cast((_stack.sp)->u->__value, %d, \"%s\");\n", id(res.value.c_str()), res.value.c_str());
                    }
                    append("%s Var_%s = *(%s*) &((_stack.sp)->u->__value);\n", sclTypeToCType(result, res.value).c_str(), name.c_str(), sclTypeToCType(result, res.value).c_str());
                    varScopeTop().push_back(Variable(name, res.value));
                    safeInc();
                    if (body[i].type != tok_paren_close) {
                        transpilerError("Expected ')', but got '" + body[i].value + "'", i);
                        errors.push_back(err);
                    }
                } else {
                    std::string removed = removeTypeModifiers(s.members[index].type);
                    if (removed != "none" && removed != "nothing") {
                        append("%s Var_it = *(%s*) &(_stack.sp)->u->__value;\n", sclTypeToCType(result, s.members[index].type).c_str(), sclTypeToCType(result, s.members[index].type).c_str());
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
            append("%s Var_it = *(%s*) &(_stack.sp)->u;\n", sclTypeToCType(result, s.name).c_str(), sclTypeToCType(result, s.name).c_str());
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
        append("switch (*(scl_int*) --_stack.sp) {\n");
        scopeDepth++;
        varScopePush();
        pushSwitch();
    }

    handler(AddrOf) {
        noUnused;
        if (handleOverriddenOperator(result, fp, scopeDepth, "@", typeStackTop)) return;
        append("SCL_ASSUME((*(scl_int*) (_stack.sp - 1)), \"Tried dereferencing nil pointer!\");\n");
        std::string ptr = removeTypeModifiers(typeStackTop);
        std::string ctype = sclTypeToCType(result, typePointedTo(ptr));
        append("*(%s*) (_stack.sp - 1) = (*(%s*) (_stack.sp - 1)->v);\n", ctype.c_str(), ctype.c_str());
        typePop;
        typeStack.push(typePointedTo(ptr));
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
            append("(_stack.sp - 1)->f = (float) (*(scl_int*) (_stack.sp - 1));\n");
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
            append("(*(scl_int*) (_stack.sp - 1)) = (scl_int) (_stack.sp - 1)->f;\n");
            typePop;
            typeStack.push("int");
            return;
        }
        if (type.value == "lambda" || strstarts(type.value, "lambda(")) {
            typePop;
            append("_scl_not_nil_cast((*(scl_int*) (_stack.sp - 1)), \"%s\");\n", type.value.c_str());
            append("SCL_ASSUME((*(scl_int*) (_stack.sp - 1)), \"Nil cannot be cast to non-nil type '%%s'!\", \"%s\");\n", type.value.c_str());
            typeStack.push(type.value);
            return;
        }
        if (isPrimitiveIntegerType(type.value)) {
            if (typeStackTop == "float") {
                append("(*(scl_int*) (_stack.sp - 1)) = (scl_int) (_stack.sp - 1)->f;\n");
            }
            typePop;
            typeStack.push(type.value);
            append("(*(scl_int*) (_stack.sp - 1)) &= SCL_%s_MAX;\n", type.value.c_str());
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
            append("SCL_ASSUME(_scl_sizeof((_stack.sp - 1)->v) >= sizeof(struct Layout_%s), \"Layout '%%s' requires more memory than the pointer has available (required: \" SCL_INT_FMT \" found: \" SCL_INT_FMT \")\", \"%s\", sizeof(struct Layout_%s), _scl_sizeof((_stack.sp - 1)->v));", type.value.c_str(), type.value.c_str(), type.value.c_str());
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
                append("_scl_not_nil_cast((*(scl_int*) (_stack.sp - 1)), \"%s\");\n", type.value.c_str());
            }
            std::string typeStr = removeTypeModifiers(type.value);
            if (getStructByName(result, typeStr) != Struct::Null) {
                append("_scl_checked_cast((_stack.sp - 1)->v, %d, \"%s\");\n", id(typeStr.c_str()), typeStr.c_str());
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
                append("(_stack.sp - 1)->f = ((%s) (*(scl_int*) (_stack.sp - 1)))->%s;\n", sclTypeToCType(result, l.name).c_str(), member.c_str());
            } else {
                append("(*(scl_int*) (_stack.sp - 1)) = (scl_int) ((%s) (*(scl_int*) (_stack.sp - 1)))->%s;\n", sclTypeToCType(result, l.name).c_str(), member.c_str());
            }
            typeStack.push(l.getMember(member).type);
            return;
        }
        std::string tmp = removeTypeModifiers(type);
        if (tmp.size() > 2 && tmp.front() == '[' && tmp.back() == ']') {
            safeInc();
            if (body[i].type != tok_identifier) {
                transpilerError("Expected identifier, but got '" + body[i].value + "'", i);
                errors.push_back(err);
                return;
            }
            std::string member = body[i].value;
            if (member == "count") {
                append("(*(scl_int*) (_stack.sp - 1)) = _scl_array_size((_stack.sp - 1)->v);\n");
                typeStack.push("int");
            } else {
                transpilerError("Unknown array member '" + member + "'", i);
                errors.push_back(err);
            }
            return;
        }
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
                append("*(scl_str*) (_stack.sp++) = _scl_create_string(\"%s\");\n", body[i].value.c_str());
                typeStack.push("str");
                append("(_stack.sp++)->v = tmp;\n");
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
                append("(_stack.sp++)->v = mt_%s$%s;\n", s.name.c_str(), m->name.c_str());
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
            transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this '.'\\insertText;!!;" + std::to_string(err.line) + ":" + std::to_string(err.column), i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }

        Method* m = attributeAccessor(result, s.name, mem.name);
        if (m) {
            if (dot.value == "?.") {
                if (deref) {
                    append("*(%s*) (_stack.sp - 1) = (*(scl_int*) (_stack.sp - 1)) ? *(mt_%s$%s(*(%s*) (_stack.sp - 1))) : 0%s;\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str(), removeTypeModifiers(m->return_type) == "float" ? ".0" : "");
                    std::string type = m->return_type;
                    if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                        type = type.substr(1, type.size() - 2);
                    } else {
                        type = "any";
                    }
                    typeStack.push(type);
                } else {
                    append("*(%s*) (_stack.sp - 1) = (*(scl_int*) (_stack.sp - 1)) ? mt_%s$%s(*(%s*) (_stack.sp - 1)) : 0%s;\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str(), removeTypeModifiers(m->return_type) == "float" ? ".0" : "");
                    typeStack.push(m->return_type);
                }
            } else {
                if (deref) {
                    append("*(%s*) (_stack.sp - 1) = *(mt_%s$%s(*(%s*) (_stack.sp - 1)));\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str());
                    std::string type = m->return_type;
                    if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                        type = type.substr(1, type.size() - 2);
                    } else {
                        type = "any";
                    }
                    typeStack.push(type);
                } else {
                    append("*(%s*) (_stack.sp - 1) = mt_%s$%s(*(%s*) (_stack.sp - 1));\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->name.c_str(), sclTypeToCType(result, m->member_type).c_str());
                    typeStack.push(m->return_type);
                }
            }
            return;
        }

        if (dot.value == "?.") {
            if (removeTypeModifiers(mem.type) == "float") {
                append("(_stack.sp - 1)->f = (*(scl_int*) (_stack.sp - 1)) ? ((%s) (_stack.sp - 1)->v)->%s : 0.0f;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            } else {
                append("(_stack.sp - 1)->v = (*(scl_int*) (_stack.sp - 1)) ? (scl_any) ((%s) (_stack.sp - 1)->v)->%s : NULL;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            }
        } else {
            if (removeTypeModifiers(mem.type) == "float") {
                append("(_stack.sp - 1)->f = ((%s) (_stack.sp - 1)->v)->%s;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            } else {
                append("(_stack.sp - 1)->v = (scl_any) ((%s) (_stack.sp - 1)->v)->%s;\n", sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
            }
        }
        typeStack.push(mem.type);
        if (deref) {
            std::string path = dot.value + "@" + body[i].value;
            append("SCL_ASSUME((*(scl_int*) (_stack.sp - 1)), \"Tried dereferencing nil pointer '%%s'!\", \"%s\");", path.c_str());
            append("(_stack.sp - 1)->v = *(_stack.sp - 1)->v;\n");
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
            transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this ':'\\insertText;!!;" + std::to_string(err.line) + ":" + std::to_string(err.column), i - 1);
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
                    append("*(%s*) (_stack.sp++) = ", sclTypeToCType(result, returnType).c_str());
                    typeStack.push(returnType);
                }
                append2("((%s) (_stack.sp + %zu)->v)(%s);\n", cType.c_str(), argAmount, argGet.c_str());
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
                append("scl_any* array = (scl_any*) _scl_pop()->v;\n");
                typePop;
                append("scl_int size = *(scl_int*) --_stack.sp;\n");
                typePop;
                append("array = (scl_any*) _scl_array_resize((scl_any*) array, size);\n");
                append("(_stack.sp++)->v = array;\n");
                typeStack.push(type);
                scopeDepth--;
                append("}\n");
            } else if (op == "sort") {
                append("(_stack.sp - 1)->v = _scl_array_sort((scl_any*) (_stack.sp - 1)->v);\n");
                typeStack.push(type);
            } else if (op == "reverse") {
                append("(_stack.sp - 1)->v = _scl_array_reverse((scl_any*) (_stack.sp - 1)->v);\n");
                typeStack.push(type);
            } else if (op == "toString") {
                append("*(scl_str*) (_stack.sp - 1) = _scl_array_to_string((scl_any*) (_stack.sp - 1)->v);\n");
                typeStack.push("str");
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
                    append("(_stack.sp++)->f = lambda(%s);\n", argGet.c_str());
                    typeStack.push(returnType);
                } else {
                    append("scl_any(*lambda)(%s) = tmp->%s;\n", argTypes.c_str(), v.name.c_str());
                    append("_scl_popn(%zu);\n", argAmount);
                    append("(_stack.sp++)->v = lambda(%s);\n", argGet.c_str());
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
                Method* objMethod = getMethodByName(result, body[i].value, "SclObject");
                if (objMethod) {
                    append("if (_scl_is_instance_of((_stack.sp - 1)->v, 0x%xU)) {\n", id("SclObject"));
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
                    append("(_stack.sp - 1)->v = (*(scl_str*) (_stack.sp - 1))->data;\n");
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
                transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this ':'\\insertText;!!;" + std::to_string(err.line) + ":" + std::to_string(err.column), i - 1);
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
        handleRefs[tok_addr_of] = handlerRef(AddrOf);
    }

    handler(Token) {
        noUnused;

        auto tokenStart = std::chrono::high_resolution_clock::now();

        handleRef(handleRefs[body[i].type]);

        auto tokenEnd = std::chrono::high_resolution_clock::now();
        Main.tokenHandleTime += std::chrono::duration_cast<std::chrono::microseconds>(tokenEnd - tokenStart).count();
    }

    extern "C" void ConvertC::writeTables(FILE* fp, TPResult& result, std::string filename) {
        (void) filename;
        (void) result;
        populateTokenJumpTable();

        scopeDepth = 0;
        append("extern tls _scl_stack_t _stack;\n\n");

        append("extern const ID_t typeid(const char*);\n\n");

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
            if (function->has_expect) continue;
            if (contains<std::string>(function->modifiers, "<binary-inherited>")) {
                continue;
            }
            if (getInterfaceByName(result, function->member_type)) {
                continue;
            }
            if (function->isMethod) {
                currentStruct = getStructByName(result, ((Method*) function)->member_type);
            } else {
                currentStruct = Struct::Null;
            }

            for (long t = typeStack.size() - 1; t >= 0; t--) {
                typePop;
            }
            vars.clear();
            varScopePush();
            for (Variable& g : globals) {
                varScopeTop().push_back(g);
            }

            scopeDepth = 0;
            noWarns = false;
            lambdaCount = 0;

            for (std::string& modifier : function->modifiers) {
                if (modifier == "nowarn") {
                    noWarns = true;
                }
            }

            std::string functionDeclaration = "";

            if (function->isMethod) {
                functionDeclaration += ((Method*) function)->member_type + ":" + function->name + "(";
                for (size_t i = 0; i < function->args.size() - 1; i++) {
                    if (i != 0) {
                        functionDeclaration += ", ";
                    }
                    functionDeclaration += function->args[i].name + ": " + function->args[i].type;
                }
                functionDeclaration += "): " + function->return_type;
            } else {
                functionDeclaration += function->name + "(";
                for (size_t i = 0; i < function->args.size(); i++) {
                    if (i != 0) {
                        functionDeclaration += ", ";
                    }
                    functionDeclaration += function->args[i].name + ": " + function->args[i].type;
                }
                functionDeclaration += "): " + function->return_type;
            }

            return_type = "void";

            if (function->return_type.size() > 0) {
                std::string t = function->return_type;
                return_type = sclTypeToCType(result, t);
            } else {
                FPResult err;
                err.success = false;
                err.message = "Function '" + function->name + "' does not specify a return type, defaults to none.";
                err.line = function->name_token.line;
                err.in = function->name_token.file;
                err.column = function->name_token.column;
                err.value = function->name_token.value;
                err.type = function->name_token.type;
                warns.push_back(err);
            }

            return_type = "void";

            std::string arguments = "";
            if (function->isMethod) {
                arguments = sclTypeToCType(result, function->member_type) + " Var_self";
                for (size_t i = 0; i < function->args.size() - 1; i++) {
                    std::string type = sclTypeToCType(result, function->args[i].type);
                    arguments += ", " + type;
                    if (type == "varargs" || type == "...") continue;
                    if (function->args[i].name.size())
                        arguments += " Var_" + function->args[i].name;
                }
            } else {
                if (function->args.size() > 0) {
                    for (size_t i = 0; i < function->args.size(); i++) {
                        std::string type = sclTypeToCType(result, function->args[i].type);
                        if (i) {
                            arguments += ", ";
                        }
                        arguments += type;
                        if (type == "varargs" || type == "...") continue;
                        if (function->args[i].name.size())
                            arguments += " Var_" + function->args[i].name;
                    }
                }
            }

            if (arguments.empty()) {
                arguments = "void";
            }

            std::string file = function->name_token.file;
            if (strstarts(file, scaleFolder)) {
                file = file.substr(scaleFolder.size() + std::string("/Frameworks/").size());
            } else {
                file = std::filesystem::path(file).relative_path();
            }

            if (function->return_type.size() > 0) {
                std::string t = function->return_type;
                return_type = sclTypeToCType(result, t);
            }

            varScopePush();
            if (function->namedReturnValue.name.size()) {
                varScopeTop().push_back(function->namedReturnValue);
            }

            if (function->isMethod) {
                if (!(((Method*) function))->force_add && getStructByName(result, function->member_type).isSealed()) {
                    FPResult result;
                    result.message = "Struct '" + function->member_type + "' is sealed!";
                    result.value = function->name_token.value;
                    result.line = function->name_token.line;
                    result.in = function->name_token.file;
                    result.column = function->name_token.column;
                    result.type = function->name_token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                append("%s mt_%s$%s(%s) {\n", return_type.c_str(), function->member_type.c_str(), function->finalName().c_str(), arguments.c_str());
                for (size_t i = 0; i < function->args.size(); i++) {
                    const Variable& var = function->args[i];
                    if (var.type == "varargs") continue;
                    varScopeTop().push_back(var);
                }
                if (function->name == "init") {
                    Method* thisMethod = (Method*) function;
                    Struct s = getStructByName(result, thisMethod->member_type);
                    if (s == Struct::Null) {
                        FPResult result;
                        result.message = "Struct '" + thisMethod->member_type + "' does not exist!";
                        result.value = function->name_token.value;
                        result.line = function->name_token.line;
                        result.in = function->name_token.file;
                        result.column = function->name_token.column;
                        result.type = function->name_token.type;
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    if (s.super.size()) {
                        Method* parentInit = getMethodByName(result, "init", s.super);
                        // implicit call to parent init, if it takes no arguments
                        if (parentInit && parentInit->args.size() == 1) {
                            scopeDepth++;
                            append("(_stack.sp++)->v = Var_self;\n");
                            generateUnsafeCall(parentInit, fp, result);
                            scopeDepth--;
                        }
                    }
                }
            } else {
                if (function->has_restrict) {
                    append("static volatile scl_SclObject function_lock$%s = NULL;\n", function->finalName().c_str());
                }
                if (function->name == "throw" || function->name == "builtinUnreachable") {
                    append("_scl_no_return ");
                }
                if (function->has_lambda) {
                    append("_scl_noinline %s fn_%s(%s) __asm(%s);\n", return_type.c_str(), function->finalName().c_str(), arguments.c_str(), generateSymbolForFunction(function).c_str());
                    append("_scl_noinline ");
                }
                append("%s fn_%s(%s) {\n", return_type.c_str(), function->finalName().c_str(), arguments.c_str());
                if (function->has_restrict) {
                    append("if (_scl_expect(function_lock$%s == NULL, 0)) function_lock$%s = ALLOC(SclObject);\n", function->finalName().c_str(), function->finalName().c_str());
                }
                for (size_t i = 0; i < function->args.size(); i++) {
                    const Variable& var = function->args[i];
                    if (var.type == "varargs") continue;
                    varScopeTop().push_back(var);
                }
            }
            
            scopeDepth++;
            std::vector<Token> body = function->getBody();

            append("SCL_BACKTRACE(\"%s\");\n", sclFunctionNameToFriendlyString(function).c_str());
            if (function->namedReturnValue.name.size()) {
                std::string nrvName = function->namedReturnValue.name;
                if (hasFunction(result, nrvName)) {
                    {
                        FPResult err;
                        err.message = "Named return value '" + nrvName + "' shadowed by function '" + nrvName + "'";
                        err.value = function->name_token.value;
                        err.line = function->name_token.line;
                        err.in = function->name_token.file;
                        err.column = function->name_token.column;
                        err.type = function->name_token.type;
                        err.success = false;
                        warns.push_back(err);
                    }
                }
                if (hasContainer(result, nrvName)) {
                    {
                        FPResult err;
                        err.message = "Named return value '" + nrvName + "' shadowed by container '" + nrvName + "'";
                        err.value = function->name_token.value;
                        err.line = function->name_token.line;
                        err.in = function->name_token.file;
                        err.column = function->name_token.column;
                        err.type = function->name_token.type;
                        err.success = false;
                        warns.push_back(err);
                    }
                }
                if (getStructByName(result, nrvName) != Struct::Null) {
                    {
                        FPResult err;
                        err.message = "Named return value '" + nrvName + "' shadowed by struct '" + nrvName + "'";
                        err.value = function->name_token.value;
                        err.line = function->name_token.line;
                        err.in = function->name_token.file;
                        err.column = function->name_token.column;
                        err.type = function->name_token.type;
                        err.success = false;
                        warns.push_back(err);
                    }
                }
                append("%s Var_%s;\n", sclTypeToCType(result, function->namedReturnValue.type).c_str(), function->namedReturnValue.name.c_str());
            }
            if (function->isMethod) {
                Struct s = getStructByName(result, function->member_type);
                std::string superType = s.super;
                if (superType.size() > 0) {
                    append("%s Var_super = (%s) Var_self;\n", sclTypeToCType(result, superType).c_str(), sclTypeToCType(result, superType).c_str());
                    varScopeTop().push_back(Variable("super", "const " + superType));
                }
            }
            append("SCL_SAVE_STACK;\n");

            for (ssize_t a = (ssize_t) function->args.size() - 1; a >= 0; a--) {
                Variable& arg = function->args[a];
                if (arg.name.size() == 0) continue;
                if (!typeCanBeNil(arg.type)) {
                    if (function->isMethod && arg.name == "self") continue;
                    if (arg.type == "varargs") continue;
                    append("SCL_ASSUME(*(scl_int*) &Var_%s, \"Argument '%%s' is nil\", \"%s\");\n", arg.name.c_str(), arg.name.c_str());
                }
                if (arg.typeFromTemplate.size()) {
                    append("_scl_checked_cast(*(scl_any*) &Var_%s, Var_self->$template_arg_%s, Var_self->$template_argname_%s);\n", arg.name.c_str(), arg.typeFromTemplate.c_str(), arg.typeFromTemplate.c_str());
                }
            }
            
            if (function->has_unsafe) {
                isInUnsafe++;
            }
            if (function->has_restrict) {
                if (function->isMethod) {
                    append("SCL_FUNCTION_LOCK((scl_SclObject) Var_self);\n");
                } else {
                    append("SCL_FUNCTION_LOCK(function_lock$%s);\n", function->finalName().c_str());
                }
            }

            if (function->isCVarArgs()) {
                if (function->varArgsParam().name.size()) {
                    append("va_list _cvarargs;\n");
                    append("va_start(_cvarargs, Var_%s$size);\n", function->varArgsParam().name.c_str());
                    append("scl_int _cvarargs_count = Var_%s$size;\n", function->varArgsParam().name.c_str());
                    append("scl_Array Var_%s = (scl_Array) _scl_cvarargs_to_array(_cvarargs, _cvarargs_count);\n", function->varArgsParam().name.c_str());
                    append("va_end(_cvarargs);\n");
                    Variable v(function->varArgsParam().name, "const ReadOnlyArray");
                    varScopeTop().push_back(v);
                }
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
            
            scopeDepth = 1;

            while (vars.size() > 1) {
                varScopePop();
            }

            scopeDepth = 0;
            append("}\n\n");
        }
    
        filePostamble();

        fclose(fp);
    }
} // namespace sclc
