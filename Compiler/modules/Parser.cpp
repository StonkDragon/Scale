#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <type_traits>
#include <sstream>

#include <stdio.h>

#include "../headers/Common.hpp"

namespace sclc
{
    extern Struct currentStruct;
    extern int scopeDepth;
    extern std::map<std::string, std::vector<Method*>> vtables;
    extern StructTreeNode* structTree;

    Parser::Parser(TPResult& result) : result(result) {}

    TPResult& Parser::getResult() {
        return result;
    }

    void generateUnsafeCall(Method* self, std::ostream& fp, TPResult& result);
    void generateUnsafeCallF(Function* self, std::ostream& fp, TPResult& result);
    std::string generateSymbolForFunction(Function* f);
    std::string sclTypeToCType(TPResult& result, std::string t);
    std::string sclFunctionNameToFriendlyString(Function* f);
    std::string sclFunctionNameToFriendlyString(std::string name);
    std::string argsToRTSignature(Function* f);

    FPResult Parser::parse(std::string func_file, std::string rt_file, std::string header_file) {
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::vector<Variable> globals;

        std::ostringstream fp;

        Function* mainFunction = nullptr;
        if (!Main::options::noMain) {
            mainFunction = getFunctionByName(result, "main");
            if (mainFunction == nullptr) {
                FPResult result;
                result.success = false;
                result.message = "No entry point found";
                result.location = SourceLocation(func_file, 0, 0);
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

        std::vector<Function*> initFuncs;
        std::vector<Function*> destroyFuncs;
        
        for (Function* f : result.functions) {
            bool isInit = isInitFunction(f);
            if (!f->isMethod && (isInit || isDestroyFunction(f))) {
                if (f->args.size()) {
                    FPResult result;
                    result.success = false;
                    result.message = isInit ? "Constructor" : "Finalizer" + std::string(" functions cannot have arguments");
                    result.location = f->name_token.location;
                    result.type = f->name_token.type;
                    result.value = f->name_token.value;
                    errors.push_back(result);
                } else {
                    if (isInit) {
                        initFuncs.push_back(f);
                    } else {
                        destroyFuncs.push_back(f);
                    }
                }
            }
        }

        std::vector<Variable> defaultScope;
        std::vector<std::vector<Variable>> tmp;
        vars.reserve(32);

        Transpiler t(result, errors, warns, fp);

        t.writeHeader();
        t.writeContainers();
        t.writeStructs();
        t.writeGlobals();
        t.writeFunctionHeaders();

        fp.flush();
        std::ofstream header(header_file, std::ios::out);
        header << fp.str();
        fp = std::ostringstream();

        t.writeFunctions(header_file);

        scopeDepth = 0;

        for (Variable& s : result.globals) {
            if (!Main::options::noLinkScale && strstarts(s.name_token.location.file, scaleFolder + "/Frameworks/Scale.framework") && !Main::options::noMain) {
                const std::string& file = s.name_token.location.file;
                if (!strcontains(file, "/compiler/") && !strcontains(file, "/macros/") && !strcontains(file, "/__")) {
                    continue;
                }
            }
            if (!s.isExtern) {
                append("%s Var_%s;\n", sclTypeToCType(result, s.type).c_str(), s.name.c_str());
            }
        }

        append("\n");
        append("_scl_constructor void init_this%llx() {\n", random());
        scopeDepth++;
        append("_scl_setup();\n");
        if (initFuncs.size()) {
            append("TRY {\n");
            for (auto&& f : initFuncs) {
                if (!Main::options::noLinkScale && strstarts(f->name_token.location.file, scaleFolder + "/Frameworks/Scale.framework") && !Main::options::noMain) {
                    const std::string& file = f->name_token.location.file;
                    if (!strcontains(file, "/compiler/") && !strcontains(file, "/macros/") && !strcontains(file, "/__")) {
                        continue;
                    }
                }
                append("  fn_%s();\n", f->name.c_str());
            }
            append("} else {\n");
            append("  _scl_runtime_catch(_scl_exception_handler.exception);\n");
            append("}\n");
        }
        scopeDepth--;
        append("}\n");

        if (destroyFuncs.size()) {
            append("\n");
            append("_scl_destructor void destroy_this%llx() {\n", random());
            scopeDepth++;
            append("TRY {\n");
            for (auto&& f : destroyFuncs) {
                if (!Main::options::noLinkScale && strstarts(f->name_token.location.file, scaleFolder + "/Frameworks/Scale.framework") && !Main::options::noMain) {
                    const std::string& file = f->name_token.location.file;
                    if (!strcontains(file, "/compiler/") && !strcontains(file, "/macros/") && !strcontains(file, "/__")) {
                        continue;
                    }
                }
                append("  fn_%s();\n", f->name.c_str());
            }
            append("} else {\n");
            append("  _scl_runtime_catch(_scl_exception_handler.exception);\n");
            append("}\n");
            scopeDepth--;
            append("}\n");
        }

        std::ofstream func(func_file, std::ios::out);
        fp.flush();
        func << fp.str();
        fp = std::ostringstream();

        if (structTree) {
            append("#include \"%s\"\n", header_file.c_str());
            structTree->forEach([&fp](StructTreeNode* node) {
                append("\n");
                const Struct& s = node->s;
                if (s.isStatic() || s.isExtern() || s.templateInstance) {
                    return;
                }
                if (!Main::options::noLinkScale && s.templates.size() == 0 && strstarts(s.name_token.location.file, scaleFolder + "/Frameworks/Scale.framework") && !Main::options::noMain) {
                    const std::string& file = s.name_token.location.file;
                    if (!strcontains(file, "/compiler/") && !strcontains(file, "/macros/") && !strcontains(file, "/__")) {
                        append("extern const TypeInfo _scl_ti_%s __asm(\"__T%s\");\n", s.name.c_str(), s.name.c_str());
                        return;
                    }
                }
                auto vtable = vtables.find(s.name);
                if (vtable == vtables.end()) {
                    return;
                }
                append("static const _scl_methodinfo_t _scl_vtable_info_%s = {\n", vtable->first.c_str());
                scopeDepth++;
                append(".layout = {\n");
                scopeDepth++;
                append(".size = %zu * sizeof(struct _scl_methodinfo),\n", vtable->second.size());
                append(".flags = MEM_FLAG_ARRAY,\n");
                append(".array_elem_size = sizeof(struct _scl_methodinfo)\n");
                scopeDepth--;
                append("},\n");
                append(".infos = {\n");
                scopeDepth++;

                for (auto&& m : vtable->second) {
                    std::string signature = argsToRTSignature(m);
                    std::string friendlyName = sclFunctionNameToFriendlyString(m->name);
                    append("(struct _scl_methodinfo) { // %s\n", friendlyName.c_str());
                    scopeDepth++;
                    append(".pure_name = 0x%lxUL, // %s\n", id(friendlyName.c_str()), friendlyName.c_str());
                    append(".signature = 0x%lxUL, // %s\n", id(signature.c_str()), signature.c_str());
                    scopeDepth--;
                    append("},\n");
                }
                append("{0}\n");
                scopeDepth--;
                append("}\n");
                scopeDepth--;
                append("};\n");

                append("const TypeInfo _scl_ti_%s __asm(\"__T%s\") = {\n", s.name.c_str(), s.name.c_str());
                scopeDepth++;
                append(".type = 0x%lxUL,\n", id(s.name.c_str()));
                append(".type_name = \"%s\",\n", s.name.c_str());
                append(".vtable_info = (&_scl_vtable_info_%s)->infos,\n", s.name.c_str());
                // append(".vtable = (&_scl_vtable_%s)->funcs,\n", s.name.c_str());
                if (s.super.size()) {
                    append(".super = &_scl_ti_%s,\n", s.super.c_str());
                } else {
                    append(".super = 0,\n");
                }
                append(".size = sizeof(struct Struct_%s),\n", s.name.c_str());
                append(".vtable = {\n");
                scopeDepth++;
                for (auto&& m : vtable->second) {
                    append("(const _scl_lambda) mt_%s$%s,\n", m->member_type.c_str(), m->name.c_str());
                }
                append("0\n");
                scopeDepth--;
                append("}\n");
                scopeDepth--;
                append("};\n");
            });

            std::ofstream rt(rt_file, std::ios::out);
            fp.flush();
            rt << fp.str();
        }

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
