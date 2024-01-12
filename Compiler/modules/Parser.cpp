#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <type_traits>

#include <stdio.h>

#include "../headers/Common.hpp"

namespace sclc
{
    extern Struct currentStruct;
    extern int scopeDepth;
    extern std::map<std::string, std::vector<Method*>> vtables;
    extern StructTreeNode* structTree;

    TPResult& Parser::getResult() {
        return result;
    }

    void generateUnsafeCall(Method* self, FILE* fp, TPResult& result);
    void generateUnsafeCallF(Function* self, FILE* fp, TPResult& result);
    std::string generateSymbolForFunction(Function* f);
    std::string sclTypeToCType(TPResult& result, std::string t);
    std::string sclFunctionNameToFriendlyString(Function* f);
    std::string sclFunctionNameToFriendlyString(std::string name);
    std::string argsToRTSignature(Function* f);

    FPResult Parser::parse(std::string filename) {
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::vector<Variable> globals;

        remove((filename.substr(0, filename.size() - 2) + ".h").c_str());
        remove(filename.c_str());
        FILE* fp = fopen((filename.substr(0, filename.size() - 2) + ".h").c_str(), "a");

        Function* mainFunction = nullptr;
        if (!Main::options::noMain) {
            mainFunction = getFunctionByName(result, "main");
            if (mainFunction == nullptr) {
                FPResult result;
                result.success = false;
                result.message = "No entry point found";
                result.location = SourceLocation(filename, 0, 0);
                errors.push_back(result);

                FPResult parseResult;
                parseResult.success = true;
                parseResult.message = "";
                parseResult.errors = errors;
                parseResult.warns = warns;
                return parseResult;
            }
        }

        if (mainFunction) {
            if (mainFunction->has_async) {
                FPResult result;
                result.success = false;
                result.message = "Entry point function cannot be async";
                result.location = mainFunction->name_token.location;
                result.type = mainFunction->name_token.type;
                result.value = mainFunction->name_token.value;
                errors.push_back(result);
            }
            if (mainFunction->args.size() > 1) {
                FPResult result;
                result.success = false;
                result.message = "Entry point function must have 0 or 1 arguments";
                result.location = mainFunction->name_token.location;
                result.type = mainFunction->name_token.type;
                result.value = mainFunction->name_token.value;
                errors.push_back(result);
            } else if (mainFunction->args.size() == 1) {
                std::string argType = removeTypeModifiers(mainFunction->args[0].type);
                if (argType.front() == '[' && argType.back() == ']') {
                    std::string elemType = removeTypeModifiers(argType.substr(1, argType.size() - 2));
                    if (!Main::options::noScaleFramework) {
                        if (!typeEquals(elemType, std::string("str"))) {
                            FPResult result;
                            result.success = false;
                            result.message = "Entry point function argument must be an array of strings";
                            result.location = mainFunction->name_token.location;
                            result.type = mainFunction->name_token.type;
                            result.value = mainFunction->name_token.value;
                            errors.push_back(result);
                        }
                    } else {
                        if (!typeEquals(elemType, std::string("[int8]"))) {
                            FPResult result;
                            result.success = false;
                            result.message = "Entry point function argument must be an array of strings";
                            result.location = mainFunction->name_token.location;
                            result.type = mainFunction->name_token.type;
                            result.value = mainFunction->name_token.value;
                            errors.push_back(result);
                        }
                    }
                } else {
                    FPResult result;
                    result.success = false;
                    result.message = "Entry point function argument must be an array of strings";
                    result.location = mainFunction->name_token.location;
                    result.type = mainFunction->name_token.type;
                    result.value = mainFunction->name_token.value;
                    errors.push_back(result);
                }
            }
        }

        std::vector<Variable> defaultScope;
        std::vector<std::vector<Variable>> tmp;
        vars.reserve(32);

        ConvertC::writeHeader(fp, errors, warns);
        ConvertC::writeContainers(fp, result, errors, warns);
        ConvertC::writeStructs(fp, result, errors, warns);
        ConvertC::writeGlobals(fp, globals, result, errors, warns);
        ConvertC::writeFunctionHeaders(fp, result, errors, warns);
        ConvertC::writeTables(fp, result, filename);
        ConvertC::writeFunctions(fp, errors, warns, globals, result, filename);

        fp = fopen(filename.c_str(), "a");

        if (structTree) {
            structTree->forEach([fp](StructTreeNode* node) {
                const Struct& s = node->s;
                if (s.isStatic() || s.isExtern() || s.templateInstance) {
                    return;
                }
                auto vtable = vtables.find(s.name);
                if (vtable == vtables.end()) {
                    return;
                }
                append("static const struct _scl_methodinfo _scl_vtable_info_%s[] __asm(\"Lscl_vtable_info_%s\") = {\n", vtable->first.c_str(), vtable->first.c_str());
                scopeDepth++;
                for (auto&& m : vtable->second) {
                    std::string signature = argsToRTSignature(m);
                    std::string friendlyName = sclFunctionNameToFriendlyString(m->finalName());
                    append("(struct _scl_methodinfo) { // %s\n", friendlyName.c_str());
                    scopeDepth++;
                    append(".pure_name = 0x%lxUL, // %s\n", id(friendlyName.c_str()), friendlyName.c_str());
                    append(".signature = 0x%lxUL, // %s\n", id(signature.c_str()), signature.c_str());
                    scopeDepth--;
                    append("},\n");
                }
                append("{0}\n");
                scopeDepth--;
                append("};\n");
                append("static const _scl_lambda _scl_vtable_%s[] __asm(\"Lscl_vtable_%s\") = {\n", vtable->first.c_str(), vtable->first.c_str());
                scopeDepth++;
                for (auto&& m : vtable->second) {
                    append("(const _scl_lambda) mt_%s$%s,\n", m->member_type.c_str(), m->finalName().c_str());
                }
                append("0\n");
                scopeDepth--;
                append("};\n");

                append("const TypeInfo _scl_ti_%s __asm(\"typeinfo for %s\") = {\n", s.name.c_str(), s.name.c_str());
                scopeDepth++;
                append(".type = 0x%lxUL,\n", id(s.name.c_str()));
                append(".type_name = \"%s\",\n", s.name.c_str());
                append(".vtable_info = _scl_vtable_info_%s,\n", s.name.c_str());
                append(".vtable = _scl_vtable_%s,\n", s.name.c_str());
                if (s.super.size()) {
                    append(".super = &_scl_ti_%s,\n", s.super.c_str());
                } else {
                    append(".super = 0,\n");
                }
                append(".size = sizeof(struct Struct_%s),\n", s.name.c_str());
                scopeDepth--;
                append("};\n\n");
            });
        }

        append("extern const ID_t type_id(const char*);\n\n");

        for (Variable& s : result.globals) {
            append("%s Var_%s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
            globals.push_back(s);
        }

        scopeDepth = 0;

        append("_scl_constructor void init_this() {\n");
        scopeDepth++;
        append("_scl_setup();\n");
        append("TRY {\n");
        for (Function* f : result.functions) {
            if (!f->isMethod && isInitFunction(f)) {
                if (f->args.size()) {
                    FPResult result;
                    result.success = false;
                    result.message = "Constructor functions cannot have arguments";
                    result.location = f->name_token.location;
                    result.type = f->name_token.type;
                    result.value = f->name_token.value;
                    errors.push_back(result);
                } else {
                    append("  fn_%s();\n", f->finalName().c_str());
                }
            }
        }
        append("} else {\n");
        append("  _scl_runtime_catch(_scl_exception_handler.exception);\n");
        append("}\n");
        scopeDepth--;
        append("}\n\n");

        append("_scl_destructor void destroy_this() {\n");
        scopeDepth++;
        append("TRY {\n");
        for (Function* f : result.functions) {
            if (!f->isMethod && isDestroyFunction(f)) {
                if (f->args.size()) {
                    FPResult result;
                    result.success = false;
                    result.message = "Finalizer functions cannot have arguments";
                    result.location = f->name_token.location;
                    result.type = f->name_token.type;
                    result.value = f->name_token.value;
                    errors.push_back(result);
                } else {
                    append("  fn_%s();\n", f->finalName().c_str());
                }
            }
        }
        append("} else {\n");
        append("  _scl_runtime_catch(_scl_exception_handler.exception);\n");
        append("}\n");
        scopeDepth--;
        append("}\n\n");

        fclose(fp);

        if (Main::options::Werror) {
            errors.insert(errors.end(), warns.begin(), warns.end());
            warns.clear();
        }

        FPResult parseResult;
        parseResult.success = true;
        parseResult.message = "";
        parseResult.errors = errors;
        parseResult.warns = warns;
        return parseResult;
    }
}
