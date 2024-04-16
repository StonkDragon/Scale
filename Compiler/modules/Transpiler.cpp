#include <gc/gc_allocator.h>

#include <filesystem>
#include <stack>
#include <functional>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#ifndef _WIN32
// #include <unistd.h>
#endif

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

    typedef void(*HandlerType)(std::vector<Token>&, Function*, std::vector<FPResult>&, std::vector<FPResult>&, std::ostream&, TPResult&);

    std::unordered_map<TokenType, HandlerType> handleRefs = {
        std::pair(tok_question_mark, handlerRef(Identifier)),
        std::pair(tok_identifier, handlerRef(Identifier)),
        std::pair(tok_dot, handlerRef(Dot)),
        std::pair(tok_column, handlerRef(Column)),
        std::pair(tok_as, handlerRef(As)),
        std::pair(tok_string_literal, handlerRef(StringLiteral)),
        std::pair(tok_utf_string_literal, handlerRef(StringLiteral)),
        std::pair(tok_char_string_literal, handlerRef(CharStringLiteral)),
        std::pair(tok_cdecl, handlerRef(CDecl)),
        std::pair(tok_extern_c, handlerRef(ExternC)),
        std::pair(tok_number, handlerRef(IntegerLiteral)),
        std::pair(tok_char_literal, handlerRef(CharLiteral)),
        std::pair(tok_number_float, handlerRef(FloatLiteral)),
        std::pair(tok_nil, handlerRef(FalsyType)),
        std::pair(tok_false, handlerRef(FalsyType)),
        std::pair(tok_true, handlerRef(TruthyType)),
        std::pair(tok_new, handlerRef(New)),
        std::pair(tok_is, handlerRef(Is)),
        std::pair(tok_if, handlerRef(If)),
        std::pair(tok_unless, handlerRef(Unless)),
        std::pair(tok_else, handlerRef(Else)),
        std::pair(tok_elif, handlerRef(Elif)),
        std::pair(tok_elunless, handlerRef(Elunless)),
        std::pair(tok_while, handlerRef(While)),
        std::pair(tok_until, handlerRef(Until)),
        std::pair(tok_do, handlerRef(Do)),
        std::pair(tok_repeat, handlerRef(Repeat)),
        std::pair(tok_for, handlerRef(For)),
        std::pair(tok_foreach, handlerRef(Foreach)),
        std::pair(tok_fi, handlerRef(Fi)),
        std::pair(tok_done, handlerRef(DoneLike)),
        std::pair(tok_end, handlerRef(DoneLike)),
        std::pair(tok_return, handlerRef(Return)),
        std::pair(tok_addr_ref, handlerRef(AddrRef)),
        std::pair(tok_store, handlerRef(Store)),
        std::pair(tok_declare, handlerRef(Declare)),
        std::pair(tok_continue, handlerRef(Continue)),
        std::pair(tok_break, handlerRef(Break)),
        std::pair(tok_goto, handlerRef(Goto)),
        std::pair(tok_label, handlerRef(Label)),
        std::pair(tok_case, handlerRef(Case)),
        std::pair(tok_esac, handlerRef(Esac)),
        std::pair(tok_default, handlerRef(Default)),
        std::pair(tok_switch, handlerRef(Switch)),
        std::pair(tok_curly_open, handlerRef(CurlyOpen)),
        std::pair(tok_bracket_open, handlerRef(BracketOpen)),
        std::pair(tok_paren_open, handlerRef(ParenOpen)),
        std::pair(tok_addr_of, handlerRef(Identifier)),
        std::pair(tok_stack_op, handlerRef(StackOp)),
        std::pair(tok_using, handlerRef(Using)),
        std::pair(tok_lambda, handlerRef(Lambda)),
        std::pair(tok_pragma, handlerRef(Pragma)),
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
    std::vector<std::string> typeStack;
    std::string return_type = "";
    int lambdaCount = 0;

    Transpiler::Transpiler(TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::ostream& fp) : result(result), errors(errors), warns(warns), fp(fp) {}
    Transpiler::~Transpiler() {}

    void Transpiler::writeHeader() {
        int scopeDepth = 0;

        append("#include <scale_runtime.h>\n");
        for (size_t i = 0; i < Main::frameworkNativeHeaders.size(); i++) {
            append("#include <%s>\n", Main::frameworkNativeHeaders[i].c_str());
        }
        append("\n");
    }

    void Transpiler::writeFunctionHeaders() {
        int scopeDepth = 0;
        append("/* FUNCTION HEADERS */\n");

        for (Function* function : result.functions) {
            std::string return_type = sclTypeToCType(result, function->return_type);
            auto args = function->args;
            std::string arguments;
            arguments.reserve(32);
            
            if (function->isMethod) {
                currentStruct = getStructByName(result, function->member_type);
                arguments = sclTypeToCType(result, function->args[function->args.size() - 1].type) + " Var_self";
            } else {
                currentStruct = Struct::Null;
            }
            bool isMain = !Main::options::noMain && function->name == "main";
            if (isMain) {
                arguments = "int __argc, char** __argv";
            } else if (args.empty()) {
                arguments = "void";
            } else {
                for (long i = 0; i < (long) args.size() - (long) function->isMethod; i++) {
                    std::string type = sclTypeToCType(result, args[i].type);
                    if (i || function->isMethod) {
                        arguments += ", ";
                    }
                    arguments += type;
                }
            }

            std::string symbol = generateSymbolForFunction(function);

            if (!function->isMethod) {
                if (UNLIKELY(function->name == "throw" || function->name == "builtinUnreachable")) {
                    append("_scl_no_return ");
                }
                if (isMain) {
                    append("int fn_%s(%s)", function->name.c_str(), arguments.c_str());
                } else {
                    if (UNLIKELY(function->has_async)) {
                        append("struct _args_fn_%s {\n", function->name.c_str());
                        for (size_t i = 0; i < function->args.size(); i++) {
                            append("  %s _Var_%s;\n", sclTypeToCType(result, function->args[i].type).c_str(), function->args[i].name.c_str());
                        }
                        append("};\n");
                        if (function->return_type.front() != '@') {
                            append("%s fn_%s(struct _args_fn_%s*)", return_type.c_str(), function->name.c_str(), function->name.c_str());
                        } else {
                            append("%s* fn_%s(struct _args_fn_%s*)", return_type.c_str(), function->name.c_str(), function->name.c_str());
                        }
                    } else {
                        append("%s fn_%s(%s)", return_type.c_str(), function->name.c_str(), arguments.c_str());
                    }
                }
                append2(" __asm(%s);\n", symbol.c_str());
                if (UNLIKELY(function->has_export)) {
                    if (UNLIKELY(function->member_type.size() && hasMethod(result, function->name, function->member_type))) {
                        FPResult res;
                        res.success = false;
                        res.message = "Function '" + function->member_type + "::" + function->name + "' conflicts with method '" + function->member_type + ":" + function->name + "'";
                        res.location = function->name_token.location;
                        res.value = function->name_token.value;
                        res.type = function->name_token.type;
                        errors.push_back(res);
                        continue;
                    }
                    if (scale_header) {
                        fprintf(scale_header, "extern ");
                        if (UNLIKELY(function->name == "throw" || function->name == "builtinUnreachable")) {
                            fprintf(scale_header, "_scl_no_return ");
                        }
                        fprintf(scale_header, "%s %s(%s) __asm(%s);\n", return_type.c_str(), function->name.c_str(), arguments.c_str(), symbol.c_str());
                    }
                }
            } else {
                if (UNLIKELY(function->has_async)) {
                    append("struct _args_mt_%s$%s {\n", function->member_type.c_str(), function->name.c_str());
                    for (size_t i = 0; i < function->args.size(); i++) {
                        append("  %s _Var_%s;\n", sclTypeToCType(result, function->args[i].type).c_str(), function->args[i].name.c_str());
                    }
                    append("};\n");
                    if (function->return_type.front() != '@') {
                        append("%s mt_%s$%s(struct _args_mt_%s$%s*)", return_type.c_str(), function->member_type.c_str(), function->name.c_str(), function->member_type.c_str(), function->name.c_str());
                    } else {
                        append("%s* mt_%s$%s(struct _args_mt_%s$%s*)", return_type.c_str(), function->member_type.c_str(), function->name.c_str(), function->member_type.c_str(), function->name.c_str());
                    }
                } else {
                    append("%s mt_%s$%s(%s)", return_type.c_str(), function->member_type.c_str(), function->name.c_str(), arguments.c_str());
                }
                append2(" __asm(%s);\n", symbol.c_str());
                if (UNLIKELY(function->has_export && scale_header)) {
                    fprintf(scale_header, "extern %s %s$%s(%s) __asm(%s);\n", return_type.c_str(), function->member_type.c_str(), function->name.c_str(), arguments.c_str(), symbol.c_str());
                }
            }
        }

        if (UNLIKELY(scale_header)) {
            fprintf(scale_header, "#endif\n");
            fclose(scale_header);
            scale_header = NULL;
        }

        append("\n");
    }

    void Transpiler::writeGlobals() {
        int scopeDepth = 0;
        if (result.globals.empty()) return;
        append("/* GLOBALS */\n");

        for (Variable& s : result.globals) {
            append("extern %s Var_%s", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
            fprintf(scale_header, "extern %s %s __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) \"%s%s\");\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str(), s.isExtern ? "" : "Var_" ,s.name.c_str());
            if (s.isExtern) {
                append2("__asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) \"%s\")", s.name.c_str());
            }
            append2(";\n");
        }

        append("\n");
    }

    void Transpiler::writeContainers() {
        int scopeDepth = 0;
        for (auto ta : result.typealiases) {
            if (ta.first == "nothing" || ta.first == "varargs") continue;
            std::string type = ta.second.first;
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

    void Transpiler::writeStructs() {
        int scopeDepth = 0;
        if (result.structs.empty()) return;
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
            std::string type = ta.second.first;
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
                    res.location = s.name_token.location;
                    res.value = s.name_token.value;
                    res.type = s.name_token.type;
                    errors.push_back(res);
                }
                if (!s.isConst) {
                    Method* setter = attributeMutator(result, c.name, s.name);
                    if (!setter) {
                        FPResult res;
                        res.success = false;
                        res.message = "No setter for virtual member '" + s.name + "'";
                        res.location = s.name_token.location;
                        res.value = s.name_token.value;
                        res.type = s.name_token.type;
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
                append("  const TypeInfo* $type;\n");
                append("  scl_any $mutex;\n");
                
                fprintf(scale_header, "struct Struct_%s {\n", c.name.c_str());
                fprintf(scale_header, "  const TypeInfo* $type;\n");
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

    void Transpiler::filePreamble(const std::string& header_file) {
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
        append("#pragma clang diagnostic ignored \"-Wout-of-scope-function\"\n");
        append("#pragma clang diagnostic ignored \"-Wconstant-conversion\"\n");
        append("#pragma clang diagnostic ignored \"-Wpointer-integer-compare\"\n");
        append("#endif\n\n");
    }

    void Transpiler::filePostamble() {
        append("#if defined(__clang__)\n");
        append("#pragma clang diagnostic pop\n");
        append("#endif\n");
    }

    void Transpiler::writeFunctions(const std::string& header_file) {
        filePreamble(header_file);

        vars.clear();
        varScopePush();
        vars.reserve(result.globals.size());
        for (const Variable& g : result.globals) {
            vars.push_back(g);
        }

        for (size_t f = 0; f < result.functions.size(); f++) {
            Function* function = currentFunction = result.functions[f];
            if (function->isMethod) {
                if (function->has_reified && !contains<std::string>(function->modifiers, "nonvirtual")) {
                    transpilerErrorTok("'reified' modifier implies 'nonvirtual' modifier on methods", function->name_token);
                    warns.push_back(err);
                }
            }
        }

        int n_captures = 0;
        for (size_t f = 0; f < result.functions.size(); f++) {
            Function* function = currentFunction = result.functions[f];
            if (UNLIKELY(function->has_reified || function->has_expect || function->has_binary_inherited || getInterfaceByName(result, function->member_type))) {
                continue;
            }
            if (UNLIKELY(function->return_type.empty())) {
                transpilerErrorTok("Return type required", function->name_token);
                errors.push_back(err);
                continue;
            }

            if (function->isMethod) {
                currentStruct = getStructByName(result, function->member_type);
                if (UNLIKELY(currentStruct == Struct::Null)) {
                    if (!hasLayout(result, function->member_type) && !hasEnum(result, function->member_type)) {
                        transpilerErrorTok("Method '" + function->name + "' is member of unknown Struct '" + function->member_type + "'", function->name_token);
                        errors.push_back(err);
                        continue;
                    }
                } else if (UNLIKELY(currentStruct.templateInstance)) {
                    continue;
                }
            } else {
                currentStruct = Struct::Null;
            }

            if (!Main::options::noLinkScale && currentStruct.templates.size() == 0 && function->reified_parameters.size() == 0 && strstarts(function->name_token.location.file, scaleFolder + "/Frameworks/Scale.framework") && !Main::options::noMain) {
                const std::string& file = function->name_token.location.file;
                if (!strcontains(file, "/compiler/") && !strcontains(file, "/macros/") && !strcontains(file, "/__")) {
                    continue;
                }
            }

            typeStack.clear();

            bool isMainFunction = !function->isMethod && !Main::options::noMain && function->name == "main";

            scopeDepth = 0;
            lambdaCount = 0;

            return_type = sclTypeToCType(result, function->return_type);

            std::string arguments;
            if (isMainFunction) {
                arguments = "int __argc, char** __argv";
            } else if (function->args.empty() && !function->has_async) {
                arguments = "void";
            } else {
                if (function->has_async) {
                    arguments = "struct _args_";
                    if (function->isMethod) {
                        arguments += "mt_" + function->member_type + "$";
                    } else {
                        arguments += "fn_";
                    }
                    arguments += function->name + "* __args";
                } else {
                    if (function->isMethod) {
                        arguments = sclTypeToCType(result, function->args[function->args.size() - 1].type) + " Var_self";
                    }
                    for (size_t i = 0; i < function->args.size() - (size_t) function->isMethod; i++) {
                        std::string type = sclTypeToCType(result, function->args[i].type);
                        if (i || function->isMethod) arguments += ", ";
                        arguments += type;

                        if (type == "varargs" || type == "...") continue;
                        if (function->args[i].name.size()) {
                            arguments += " Var_" + function->args[i].name;
                        } else {
                            arguments += " param" + std::to_string(i);
                        }
                    }
                }
            }

            if (function->has_lambda) {
                append("%s fn_%s(%s) __asm(%s);\n", return_type.c_str(), function->name.c_str(), arguments.c_str(), generateSymbolForFunction(function).c_str());
            } else if (UNLIKELY(!function->isMethod && function->has_restrict)) {
                append("static volatile scl_any function_lock$%s = NULL;\n", function->name.c_str());
            }

            if (isMainFunction) {
                append("int ");
            } else if (UNLIKELY(function->return_type.front() == '@' && function->has_async)) {
                append("%s* ", return_type.c_str());
            } else {
                if (UNLIKELY(removeTypeModifiers(function->return_type) == "nothing")) {
                    append("_scl_no_return ");
                }
                append("%s ", return_type.c_str());
            }

            if (function->isMethod) {
                if (UNLIKELY(!((Method*) function)->force_add && currentStruct.isSealed())) {
                    transpilerErrorTok("Cannot add method '" + function->name + "' to sealed Struct '" + currentStruct.name + "'", function->name_token);
                    errors.push_back(err);
                    continue;
                }
                append2("mt_%s$%s(%s) {\n", function->member_type.c_str(), function->name.c_str(), arguments.c_str());
                if (function->name_without_overload == "init" && currentStruct.super.size()) {
                    Method* parentInit = getMethodByName(result, "init", currentStruct.super);
                    if (parentInit && parentInit->args.size() == 1) {
                        append("  mt_%s$%s((%s) ", currentStruct.super.c_str(), parentInit->name.c_str(), sclTypeToCType(result, currentStruct.super).c_str());
                        if (UNLIKELY(function->has_async)) {
                            append2("__args->_");
                        }
                        append2("Var_self);\n");
                    }
                }
            } else {
                append2("fn_%s(%s) {\n", function->name.c_str(), arguments.c_str());
            }
            varScopePush();
            if (UNLIKELY(function->has_async)) {
                for (Variable& var : function->args) {
                    append("  %s Var_%s = __args->_Var_%s;\n", sclTypeToCType(result, var.type).c_str(), var.name.c_str(), var.name.c_str());
                }
            } else if (UNLIKELY(function->has_lambda)) {
                if (function->captures.size()) {
                    append("  extern struct {\n");
                    for (Variable cap : function->captures) {
                        append("    %s %s_;\n", sclTypeToCType(result, cap.type).c_str(), cap.name.c_str());
                        vars.push_back(Variable(cap.name, cap.type));
                    }
                    append("  } cap%d __asm(%s\".caps\");\n", n_captures, generateSymbolForFunction(function).c_str());
                    for (Variable cap : function->captures) {
                        append("  %s Var_%s = (cap%d.%s_);\n", sclTypeToCType(result, cap.type).c_str(), cap.name.c_str(), n_captures, cap.name.c_str());
                    }
                    n_captures++;
                }
                if (function->ref_captures.size()) {
                    append("  extern struct {\n");
                    for (Variable cap : function->ref_captures) {
                        append("    %s* %s_;\n", sclTypeToCType(result, cap.type).c_str(), cap.name.c_str());
                        vars.push_back(Variable(cap.name, cap.type));
                    }
                    append("  } cap%d __asm(%s\".refs\");\n", n_captures, generateSymbolForFunction(function).c_str());
                    for (Variable cap : function->ref_captures) {
                        append("  #define Var_%s (*cap%d.%s_)\n", cap.name.c_str(), n_captures, cap.name.c_str());
                    }
                    n_captures++;
                }
            }
            append("  scl_uint64* _local_stack = alloca(%zu * sizeof(scl_uint64));\n", Main::options::stackSize);
            append("  scl_uint64* _local_stack_ptr = _local_stack;\n");
            
            scopeDepth++;
            std::vector<Token> body = function->getBody();
            
            if (UNLIKELY(!function->namedReturnValue.name.empty())) {
                const std::string& nrvName = function->namedReturnValue.name;
                checkShadow(nrvName, function->namedReturnValue.name_token, function, result, warns);
                append("%s Var_%s;\n", sclTypeToCType(result, function->namedReturnValue.type).c_str(), function->namedReturnValue.name.c_str());

                vars.push_back(function->namedReturnValue);
            }
            for (size_t i = 0; i < function->args.size(); i++) {
                const Variable& var = function->args[i];
                if (var.type != "varargs") {
                    vars.push_back(var);
                }
            }

            append("SCL_BACKTRACE(\"%s\");\n", sclFunctionNameToFriendlyString(function).c_str());
            if (function->isMethod) {
                const std::string& superType = currentStruct.super;
                if (LIKELY(superType.size() > 0)) {
                    append("%s Var_super = (%s) %sVar_self;\n", sclTypeToCType(result, superType).c_str(), sclTypeToCType(result, superType).c_str(), function->args[function->args.size() - 1].type.front() == '@' ? "&" : "");
                    vars.push_back(Variable("super", "const " + superType));
                }
            }

            if (isMainFunction && function->args.size()) {
                if (!Main::options::noScaleFramework) {
                    append("scl_str* ");
                } else {
                    append("scl_int8** ");
                }
                append2("Var_%s = _scl_new_array_by_size(__argc, sizeof(Var_%s));\n", function->args[0].name.c_str(), function->args[0].name.c_str());
                append("for (scl_int i = 0; i < __argc; i++) {\n");
                append("  Var_%s[i] = ", function->args[0].name.c_str());
                if (!Main::options::noScaleFramework) {
                    append2("_scl_create_string(__argv[i]);\n");
                } else {
                    append2("__argv[i];\n");
                }
                append("}\n");
            }

            for (ssize_t a = (ssize_t) function->args.size() - 1; a >= 0; a--) {
                const Variable& arg = function->args[a];
                if (UNLIKELY(typeCanBeNil(arg.type) || hasEnum(result, arg.type) || arg.type == "varargs" || (hasTypealias(result, arg.type) && typealiasCanBeNil(result, arg.type)))) continue;

                if (!arg.name.empty()) {
                    append("SCL_ASSUME(*(scl_int*) &Var_%s, \"Argument '%%s' is nil\", \"%s\");\n", arg.name.c_str(), arg.name.c_str());
                }
            }
            
            if (function->has_unsafe) {
                isInUnsafe++;
            }
            if (function->has_restrict) {
                if (function->isMethod) {
                    append("cxx_std_recursive_mutex_lock(&(Var_self->$mutex));\n");
                } else {
                    append("cxx_std_recursive_mutex_lock(&(function_lock$%s));\n", function->name.c_str());
                }
            }

            if (UNLIKELY(function->isCVarArgs() && function->varArgsParam().name.size())) {
                append("va_list _cvarargs;\n");
                append("va_start(_cvarargs, Var_%s$size);\n", function->varArgsParam().name.c_str());
                append("scl_int _cvarargs_count = Var_%s$size;\n", function->varArgsParam().name.c_str());
                append("scl_any* Var_%s SCL_AUTO_DELETE = (scl_any*) _scl_cvarargs_to_array(_cvarargs, _cvarargs_count);\n", function->varArgsParam().name.c_str());
                append("va_end(_cvarargs);\n");
                vars.push_back(Variable(function->varArgsParam().name, "const [any]"));
            }

            if (UNLIKELY(function->has_setter || function->has_getter)) {
                std::string fieldName;
                if (function->has_setter) {
                    fieldName = function->getModifier(function->has_setter + 1);
                } else {
                    fieldName = function->getModifier(function->has_getter + 1);
                }
                append("#define Var_field (Var_self->%s)\n", fieldName.c_str());

                vars.push_back(Variable("field", currentStruct.getMember(fieldName).type));
            }

            for (i = 0; i < body.size(); i++) {
                handle(Token);
            }

            varScopePop();

            if (UNLIKELY(function->has_setter || function->has_getter)) {
                append("#undef Var_field\n");
            } else if (UNLIKELY(function->has_lambda)) {
                for (Variable cap : function->ref_captures) {
                    append("#undef Var_%s\n", cap.name.c_str());
                }
            }
            
            if (function->has_unsafe) {
                isInUnsafe--;
            }

            if (isMainFunction) {
                append("return 0;\n");
            }

            scopeDepth = 0;
            append("}\n\n");
        }

        filePostamble();
    }
} // namespace sclc
