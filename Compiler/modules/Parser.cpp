
#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <type_traits>
#include <sstream>
#include <random>

#include <stdio.h>

#include "../headers/Common.hpp"
#include "../headers/TranspilerDefs.hpp"
#include "../headers/Types.hpp"
#include "../headers/Functions.hpp"

namespace sclc
{
    extern Struct currentStruct;
    extern int scopeDepth;
    extern std::unordered_map<std::string, std::vector<Method*>> vtables;
    extern std::vector<std::string> typeStack;
    extern StructTreeNode* structTree;

    Parser::Parser(TPResult& result) : result(result) {}

    TPResult& Parser::getResult() {
        return result;
    }

    std::string generateSymbolForFunction(Function* f);
    std::string sclTypeToCType(TPResult& result, std::string t);
    std::string sclFunctionNameToFriendlyString(Function* f);
    std::string sclFunctionNameToFriendlyString(std::string name);
    std::string argsToRTSignature(Function* f);

    void Parser::parse(FPResult& output, std::string func_file, std::string rt_file, std::string header_file) {
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

                output.success = true;
                output.message = "";
                output.errors = errors;
                output.warns = warns;
                return;
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
        t.writeGlobals();
        t.writeFunctionHeaders();

        fp.flush();
        std::ofstream header(header_file, std::ios::out);
        header << fp.str();
        fp = std::ostringstream();

        t.writeFunctions(header_file);

        scopeDepth = 0;

        for (Variable& v : result.globals) {
            const std::string& file = v.name_token.location.file;
            if (!Main::options::noLinkScale && pathstarts(file, scaleFolder + DIR_SEP "Frameworks" DIR_SEP "Scale.framework") && !strstarts(v.name, "$T")) {
                continue;
            }
            if (!v.isExtern) {
                Method* m = nullptr;
                const Struct& s = getStructByName(result, v.type);
                std::string type = v.type;
                if (!v.canBeNil && !v.hasInitializer) {
                    if (s != Struct::Null) {
                        m = getMethodByName(result, "init", type);
                        bool hasDefaultConstructor = false;
                        if (m->args.size() == 1) {
                            hasDefaultConstructor = true;
                        } else {
                            for (Function* over_ : m->overloads) {
                            Method* overload = (Method*) over_;
                                if (overload->args.size() == 1) {
                                    hasDefaultConstructor = true;
                                    m = overload;
                                    break;
                                }
                            }
                        }
                        if (!hasDefaultConstructor) {
                            goto noNil;
                        }
                    } else {
                    noNil:
                        transpilerErrorTok("Uninitialized global '" + v.name + "' with non-nil type '" + type + "'", v.name_token);
                        errors.push_back(err);
                    }
                }
                type = sclTypeToCType(result, type);
                if (type == "scl_float" || type == "scl_float32") {
                    append("%s Var_%s", type.c_str(), v.name.c_str());
                    append2(" __asm__(_scl_macro_to_string(__USER_LABEL_PREFIX__) \"%s\") = 0.0;\n", v.name.c_str());
                } else {
                    if (s != Struct::Null && !s.isStatic()) {
                        append("%s Var_%s", type.c_str(), v.name.c_str());
                        append2(" __asm__(_scl_macro_to_string(__USER_LABEL_PREFIX__) \"%s\") = {0};\n", v.name.c_str());
                        if (m != nullptr) {
                            Function* f = new Function("static_init$" + v.name, v.name_token);
                            f->return_type = "none";
                            initFuncs.push_back(f);
                            append("void fn_static_init$%s() {\n", v.name.c_str());
                            scopeDepth++;
                            append("%s tmp = _scl_uninitialized_constant(%s);\n", type.c_str(), s.name.c_str());
                            append("mt_%s$%s(tmp);\n", m->member_type.c_str(), m->name.c_str());
                            append("Var_%s = tmp;\n", v.name.c_str());
                            scopeDepth--;
                            append("}\n", v.name.c_str());
                        }
                    } else if (hasTypealias(result, type) || hasLayout(result, type)) {
                        append("%s Var_%s", type.c_str(), v.name.c_str());
                        append2(" __asm__(_scl_macro_to_string(__USER_LABEL_PREFIX__) \"%s\");\n", v.name.c_str());
                    } else {
                        append("%s Var_%s", type.c_str(), v.name.c_str());
                        append2(" __asm__(_scl_macro_to_string(__USER_LABEL_PREFIX__) \"%s\") = 0;\n", v.name.c_str());
                    }
                }
            }
        }

        #ifdef _WIN32
        std::random_device rdev;
        std::mt19937_64 rng(rdev());
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        unsigned long long rand = dist(rng);
        #else
        unsigned long long rand = random();
        #endif

        append("\n");
        append("_scl_constructor void init_this%llx() {\n", rand);
        scopeDepth++;
        append("_scl_setup();\n");
        for (auto&& f : initFuncs) {
            const std::string& file = f->name_token.location.file;
            if (!Main::options::noLinkScale && pathstarts(file, scaleFolder + DIR_SEP "Frameworks" DIR_SEP "Scale.framework") && !strstarts(f->name_without_overload, "$T")) {
                continue;
            }
            if (!f->has_expect)
                append("  fn_%s();\n", f->name.c_str());
        }
        scopeDepth--;
        append("}\n");

        append("\n");
        append("_scl_destructor void destroy_this%llx() {\n", rand);
        if (destroyFuncs.size()) {
            scopeDepth++;
            for (auto&& f : destroyFuncs) {
                if (!f->has_expect)
                    append("  fn_%s();\n", f->name.c_str());
            }
            scopeDepth--;
        }
        append("}\n");

        std::ofstream func(func_file, std::ios::out);
        fp.flush();
        func << fp.str();
        fp = std::ostringstream();

        if (structTree) {
            append("#include \"%s\"\n", header_file.c_str());
            structTree->forEach([&fp](StructTreeNode* node) {
                append("\n");
                const Struct& s = node->s;
                if (s.isExtern()) {
                    append("extern expect const TypeInfo _scl_ti_%s __asm__(\"__T%s\");\n", s.name.c_str(), s.name.c_str());
                }
                if (s.isStatic() || s.isExtern()) {
                    return;
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
                
                append("static const _scl_vtable _scl_vtable_%s = {\n", s.name.c_str());
                scopeDepth++;
                append(".layout = {\n");
                scopeDepth++;
                append(".size = %zu * sizeof(_scl_function),\n", vtable->second.size());
                append(".flags = MEM_FLAG_ARRAY,\n");
                append(".array_elem_size = sizeof(_scl_function)\n");
                scopeDepth--;
                append("},\n");
                append(".funcs = {\n");
                scopeDepth++;
                for (auto&& m : vtable->second) {
                    append("(const _scl_function) mt_%s$%s,\n", m->member_type.c_str(), m->name.c_str());
                }
                append("0\n");
                scopeDepth--;
                append("}\n");
                scopeDepth--;
                append("};\n");

                append("const TypeInfo _scl_ti_%s __asm__(\"__T%s\") = {\n", s.name.c_str(), s.name.c_str());
                scopeDepth++;
                append(".type = 0x%lxUL,\n", id(s.name.c_str()));
                append(".type_name = \"%s\",\n", retemplate(s.name).c_str());
                append(".vtable_info = (&_scl_vtable_info_%s)->infos,\n", s.name.c_str());
                append(".vtable = (&_scl_vtable_%s)->funcs,\n", s.name.c_str());
                if (s.super.size()) {
                    append(".super = &_scl_ti_%s,\n", s.super.c_str());
                } else {
                    append(".super = 0,\n");
                }
                append(".size = sizeof(struct Struct_%s),\n", s.name.c_str());
                scopeDepth--;
                append("};\n");
            });

            std::ofstream rt(rt_file, std::ios::out);
            fp.flush();
            rt << fp.str();
        } else {
            std::ofstream rt(rt_file, std::ios::out);
            rt << "";
        }

        if (Main::options::Werror) {
            errors.insert(errors.end(), warns.begin(), warns.end());
            warns.clear();
        }

        output.success = true;
        output.message = "";
        output.errors = errors;
        output.warns = warns;
    }
}
