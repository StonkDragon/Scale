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
    std::vector<std::string> modes {
        "f",
        "m",
        "r",
        "filter",
        "map",
        "reduce",
        "count",
        "each",
        "also",
        "transform",
    };

    void (*handleRefs[tok_MAX])(std::vector<Token>&, Function*, std::vector<FPResult>&, std::vector<FPResult>&, FILE*, TPResult&) = {
        [tok_question_mark] = handlerRef(Identifier),
        [tok_identifier] = handlerRef(Identifier),
        [tok_dot] = handlerRef(Dot),
        [tok_column] = handlerRef(Column),
        [tok_as] = handlerRef(As),
        [tok_string_literal] = handlerRef(StringLiteral),
        [tok_utf_string_literal] = handlerRef(StringLiteral),
        [tok_char_string_literal] = handlerRef(CharStringLiteral),
        [tok_cdecl] = handlerRef(CDecl),
        [tok_extern_c] = handlerRef(ExternC),
        [tok_number] = handlerRef(IntegerLiteral),
        [tok_char_literal] = handlerRef(CharLiteral),
        [tok_number_float] = handlerRef(FloatLiteral),
        [tok_nil] = handlerRef(FalsyType),
        [tok_false] = handlerRef(FalsyType),
        [tok_true] = handlerRef(TruthyType),
        [tok_new] = handlerRef(New),
        [tok_is] = handlerRef(Is),
        [tok_if] = handlerRef(If),
        [tok_unless] = handlerRef(Unless),
        [tok_else] = handlerRef(Else),
        [tok_elif] = handlerRef(Elif),
        [tok_elunless] = handlerRef(Elunless),
        [tok_while] = handlerRef(While),
        [tok_do] = handlerRef(Do),
        [tok_repeat] = handlerRef(Repeat),
        [tok_for] = handlerRef(For),
        [tok_foreach] = handlerRef(Foreach),
        [tok_fi] = handlerRef(Fi),
        [tok_done] = handlerRef(DoneLike),
        [tok_end] = handlerRef(DoneLike),
        [tok_return] = handlerRef(Return),
        [tok_addr_ref] = handlerRef(AddrRef),
        [tok_store] = handlerRef(Store),
        [tok_declare] = handlerRef(Declare),
        [tok_continue] = handlerRef(Continue),
        [tok_break] = handlerRef(Break),
        [tok_goto] = handlerRef(Goto),
        [tok_label] = handlerRef(Label),
        [tok_case] = handlerRef(Case),
        [tok_esac] = handlerRef(Esac),
        [tok_default] = handlerRef(Default),
        [tok_switch] = handlerRef(Switch),
        [tok_curly_open] = handlerRef(CurlyOpen),
        [tok_bracket_open] = handlerRef(BracketOpen),
        [tok_paren_open] = handlerRef(ParenOpen),
        [tok_addr_of] = handlerRef(Identifier),
    };

    Function* currentFunction = nullptr;
    Struct currentStruct("");
    std::map<std::string, std::vector<Method*>> vtables;
    FILE* scale_header = NULL;
    StructTreeNode* structTree;
    int scopeDepth = 0;
    size_t i = 0;
    size_t condCount = 0;
    std::vector<short> whatWasIt;
    std::vector<std::string> switchTypes;
    std::vector<std::string> usingNames;
    std::unordered_map<std::string, std::vector<std::string>> usingStructs;
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
                arguments += sclTypeToCType(result, function->args[function->args.size() - 1].type) + " Var_self";
                for (long i = 0; i < (long) args.size() - 1; i++) {
                    std::string type = sclTypeToCType(result, args[i].type);
                    arguments += ", " + type;
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
                    append("int fn_%s(%s)", function->name.c_str(), arguments.c_str());
                } else {
                    if (function->has_async) {
                        append("struct _args_fn_%s {\n", function->name.c_str());
                        for (Variable& arg : function->args) {
                            append("  %s _Var_%s;\n", sclTypeToCType(result, arg.type).c_str(), arg.name.c_str());
                        }
                        append("};\n");
                        append("%s fn_%s(struct _args_fn_%s*)", return_type.c_str(), function->name.c_str(), function->name.c_str());
                    } else {
                        append("%s fn_%s(%s)", return_type.c_str(), function->name.c_str(), arguments.c_str());
                    }
                }
                append2(" __asm(%s);\n", symbol.c_str());
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
                        fprintf(scale_header, "%s %s(%s) __asm(%s);\n", return_type.c_str(), function->name.c_str(), arguments.c_str(), symbol.c_str());
                    }
                }
            } else {
                if (function->has_async) {
                    append("struct _args_mt_%s$%s {\n", function->member_type.c_str(), function->name.c_str());
                    for (Variable& arg : function->args) {
                        append("  %s _Var_%s;\n", sclTypeToCType(result, arg.type).c_str(), arg.name.c_str());
                    }
                    append("};\n");
                    append("%s mt_%s$%s(struct _args_mt_%s$%s*)", return_type.c_str(), function->member_type.c_str(), function->name.c_str(), function->member_type.c_str(), function->name.c_str());
                } else {
                    append("%s mt_%s$%s(%s)", return_type.c_str(), function->member_type.c_str(), function->name.c_str(), arguments.c_str());
                }
                append2(" __asm(%s);\n", symbol.c_str());
                if (function->has_export) {
                    if (hasFunction(result, function->member_type + "$" + function->name)) {
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
                        fprintf(scale_header, "extern %s %s$%s(%s) __asm(%s);\n", return_type.c_str(), function->member_type.c_str(), function->name.c_str(), arguments.c_str(), symbol.c_str());
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
                if (c.isStatic() || c.templateInstance) return;
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

    handler(Token) {
        noUnused;

        handleRef(handleRefs[body[i].type]);
    }

    extern "C" void ConvertC::writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<Variable>& globals, TPResult& result, const std::string& header_file) {
        (void) warns;

        auto filePreamble = [&]() {
            append("#include <scale_runtime.h>\n\n");
            append("#include \"%s\"\n\n", header_file.c_str());
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
                } else if (currentStruct.templateInstance) {
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
                arguments = sclTypeToCType(result, function->args[function->args.size() - 1].type) + " Var_self";
            } else if (function->args.size() == 0) {
                arguments = "void";
            }
            if (!function->isMethod && !Main::options::noMain && function->name == "main") {
                arguments = "int __argc, char** __argv";
            } else {
                if (function->has_async) {
                    arguments = "struct _args_";
                    if (function->isMethod) {
                        arguments += "mt_" + function->member_type + "$";
                    } else {
                        arguments += "fn_";
                    }
                    arguments += function->name + "* __args";
                }
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
                append("%s mt_%s$%s(%s) {\n", return_type.c_str(), function->member_type.c_str(), function->name.c_str(), arguments.c_str());
                if (function->name == "init" && currentStruct.super.size()) {
                    Method* parentInit = getMethodByName(result, "init", currentStruct.super);
                    if (parentInit && parentInit->args.size() == 1) {
                        if (function->has_async) {
                            append("  mt_%s$init((%s) __args->_Var_self);\n", currentStruct.super.c_str(), sclTypeToCType(result, currentStruct.super).c_str());
                        } else {
                            append("  mt_%s$init((%s) Var_self);\n", currentStruct.super.c_str(), sclTypeToCType(result, currentStruct.super).c_str());
                        }
                    }
                }
            } else {
                if (function->has_restrict) {
                    if (Main::options::noScaleFramework) {
                        transpilerErrorTok("Function '" + function->name + "' has restrict, but Scale Framework is disabled", function->name_token);
                        errors.push_back(err);
                        continue;
                    }
                    append("static volatile scl_SclObject function_lock$%s = NULL;\n", function->name.c_str());
                }
                if (function->name == "throw" || function->name == "builtinUnreachable") {
                    append("_scl_no_return ");
                } else if (function->has_lambda) {
                    append("_scl_noinline %s fn_%s(%s) __asm(%s);\n", return_type.c_str(), function->name.c_str(), arguments.c_str(), generateSymbolForFunction(function).c_str());
                    append("_scl_noinline ");
                } else {
                    append("");
                }
                if (!function->isMethod && !Main::options::noMain && function->name == "main") {
                    append("int ");
                } else {
                    append2("%s ", return_type.c_str());
                }
                append2("fn_%s(%s) {\n", function->name.c_str(), arguments.c_str());
                if (function->has_restrict) {
                    append("  if (_scl_expect(function_lock$%s == NULL, 0)) function_lock$%s = ALLOC(SclObject);\n", function->name.c_str(), function->name.c_str());
                }
            }
            if (function->has_async) {
                for (Variable& var : function->args) {
                    append("  %s Var_%s = __args->_Var_%s;\n", sclTypeToCType(result, var.type).c_str(), var.name.c_str(), var.name.c_str());
                }
            }
            append("  scl_int ls_ptr = 0;\n");
            append("  scl_int ls[%zu];\n", Main::options::stackSize);
            
            scopeDepth++;
            std::vector<Token> body = function->getBody();

            varScopePush();
            if (function->namedReturnValue.name.size()) {
                std::string nrvName = function->namedReturnValue.name;
                if (hasFunction(result, nrvName)) {
                    transpilerErrorTok("Named return value '" + nrvName + "' shadowed by function '" + nrvName + "'", function->name_token);
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
                    append("%s Var_super = (%s) %sVar_self;\n", sclTypeToCType(result, superType).c_str(), sclTypeToCType(result, superType).c_str(), function->args[function->args.size() - 1].type.front() == '*' ? "&" : "");
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
                    append("SCL_FUNCTION_LOCK(function_lock$%s);\n", function->name.c_str());
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
    }
} // namespace sclc
