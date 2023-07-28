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
        FILE* fp = fopen((filename.substr(0, filename.size() - 2) + ".h").c_str(), "a");

        Function* mainFunction = nullptr;
        if (!Main.options.noMain) {
            mainFunction = getFunctionByName(result, "main");
        }
        if (mainFunction == nullptr && !Main.options.noMain) {
            FPResult result;
            result.success = false;
            result.message = "No entry point found";
            result.line = 0;
            result.in = filename;
            result.column = 0;
            errors.push_back(result);

            FPResult parseResult;
            parseResult.success = true;
            parseResult.message = "";
            parseResult.errors = errors;
            parseResult.warns = warns;
            return parseResult;
        }

        if (mainFunction && !Main.options.noMain) {
            Main.options.mainReturnsNone = mainFunction->getReturnType() == "none" || mainFunction->getReturnType() == "nothing";
            Main.options.mainArgCount  = mainFunction->getArgs().size();
        }

        std::vector<Variable> defaultScope;
        std::vector<std::vector<Variable>> tmp;
        vars.reserve(32);

        auto writeHeaderStart = std::chrono::high_resolution_clock::now();
        ConvertC::writeHeader(fp, errors, warns);
        auto writeHeaderEnd = std::chrono::high_resolution_clock::now();
        Main.writeHeaderTime = std::chrono::duration_cast<std::chrono::microseconds>(writeHeaderEnd - writeHeaderStart).count();

        auto writeContainersStart = std::chrono::high_resolution_clock::now();
        ConvertC::writeContainers(fp, result, errors, warns);
        auto writeContainersEnd = std::chrono::high_resolution_clock::now();
        Main.writeContainersTime = std::chrono::duration_cast<std::chrono::microseconds>(writeContainersEnd - writeContainersStart).count();

        auto writeStructsStart = std::chrono::high_resolution_clock::now();
        ConvertC::writeStructs(fp, result, errors, warns);
        auto writeStructsEnd = std::chrono::high_resolution_clock::now();
        Main.writeStructsTime = std::chrono::duration_cast<std::chrono::microseconds>(writeStructsEnd - writeStructsStart).count();

        auto writeGlobalsStart = std::chrono::high_resolution_clock::now();
        ConvertC::writeGlobals(fp, globals, result, errors, warns);
        auto writeGlobalsEnd = std::chrono::high_resolution_clock::now();
        Main.writeGlobalsTime = std::chrono::duration_cast<std::chrono::microseconds>(writeGlobalsEnd - writeGlobalsStart).count();

        auto writeFunctionHeadersStart = std::chrono::high_resolution_clock::now();
        ConvertC::writeFunctionHeaders(fp, result, errors, warns);
        auto writeFunctionHeadersEnd = std::chrono::high_resolution_clock::now();
        Main.writeFunctionHeadersTime = std::chrono::duration_cast<std::chrono::microseconds>(writeFunctionHeadersEnd - writeFunctionHeadersStart).count();

        auto writeTablesStart = std::chrono::high_resolution_clock::now();
        ConvertC::writeTables(fp, result, filename);
        auto writeTablesEnd = std::chrono::high_resolution_clock::now();
        Main.writeTablesTime = std::chrono::duration_cast<std::chrono::microseconds>(writeTablesEnd - writeTablesStart).count();

        auto writeFunctionsStart = std::chrono::high_resolution_clock::now();
        ConvertC::writeFunctions(fp, errors, warns, globals, result, filename);
        auto writeFunctionsEnd = std::chrono::high_resolution_clock::now();
        Main.writeFunctionsTime = std::chrono::duration_cast<std::chrono::microseconds>(writeFunctionsEnd - writeFunctionsStart).count();

        fclose(fp);
        std::string tmpFile = scaleFolder + "/tmp/" + filename.substr(0, filename.size() - 2) + ".c";
        remove(tmpFile.c_str());
        fopen(tmpFile.c_str(), "a");

        append("#include \"./%s.h\"\n\n", filename.substr(0, filename.size() - 2).c_str());

        Main.options.flags.push_back(tmpFile);

        std::vector<std::string> alreadyGenerated;

        for (auto&& f : result.functions) {
            if (f->isMethod) {
                if (getInterfaceByName(result, f->member_type)) {
                    continue;
                }
                currentStruct = getStructByName(result, f->member_type);
                auto m = (Method*) f;
                append("static void _scl_vt_c_%s$%s() {\n", m->getMemberType().c_str(), f->finalName().c_str());
                scopeDepth++;
                generateUnsafeCall(m, fp, result);
                scopeDepth--;
                append("}\n");
            } else {
                if (f->isExternC) continue;
                std::string fn = generateSymbolForFunction(f);
                if (contains(alreadyGenerated, fn)) continue;
                alreadyGenerated.push_back(fn);

                currentStruct = getStructByName(result, f->member_type);
                append("void _scl_c_%s() __asm(%s\"@CALLER\");\n", f->finalName().c_str(), fn.c_str());
                append("void _scl_c_%s() {\n", f->finalName().c_str());
                scopeDepth++;
                generateUnsafeCallF(f, fp, result);
                scopeDepth--;
                append("}\n");
            }
        }

        for (Struct& s : result.structs) {
            if (s.isStatic()) continue;
            append("const ID_t _scl_supers_%s[] = {\n", s.getName().c_str());
            scopeDepth++;
            Struct current = s;
            while (current.extends().size()) {
                append("0x%xU,\n", id((char*) current.extends().c_str()));
                current = getStructByName(result, current.extends());
            }
            append("0x0U\n");
            scopeDepth--;
            append("};\n");
        }
        for (Struct& s : result.structs) {
            if (s.isStatic()) continue;
            append("const StaticMembers _scl_statics_%s = {\n", s.getName().c_str());
            scopeDepth++;
            append(".type = 0x%xU,\n", id((char*) s.getName().c_str()));
            append(".type_name = \"%s\",\n", s.getName().c_str());
            append(".vtable = _scl_vtable_%s,\n", s.getName().c_str());
            if (s.extends().size()) {
                append(".super_vtable = _scl_vtable_%s,\n", s.extends().c_str());
            } else {
                append(".super_vtable = 0,\n");
            }
            append(".supers = _scl_supers_%s,\n", s.getName().c_str());
            scopeDepth--;
            append("};\n");
        }

        for (auto&& vtable : vtables) {
            append("const struct _scl_methodinfo _scl_vtable_%s[] = {\n", vtable.first.c_str());
            scopeDepth++;
            for (auto&& m : vtable.second) {
                std::string signature = argsToRTSignature(m);
                append("(struct _scl_methodinfo) {\n");
                scopeDepth++;
                append(".ptr = _scl_vt_c_%s$%s,\n", m->getMemberType().c_str(), m->finalName().c_str());
                append(".pure_name = 0x%xU,\n", id((char*) sclFunctionNameToFriendlyString(m->finalName()).c_str()));
                append(".signature = 0x%xU,\n", id((char*) signature.c_str()));
                scopeDepth--;
                append("},\n");
            }
            append("{0}\n");
            scopeDepth--;
            append("};\n\n");
        }

        append("extern const ID_t id(const char*);\n\n");

        for (Variable& s : result.globals) {
            append("%s Var_%s;\n", sclTypeToCType(result, s.getType()).c_str(), s.getName().c_str());
            globals.push_back(s);
        }

        for (Container& c : result.containers) {
            append("struct _Container_%s Container_%s = {0};\n", c.getName().c_str(), c.getName().c_str());
        }

        append("_scl_constructor void init_this() {\n");
        scopeDepth++;
        append("_scl_setup();\n");
        append("TRY {\n");
        for (Function* f : result.functions) {
            if (!f->isMethod && isInitFunction(f)) {
                if (f->getArgs().size()) {
                    FPResult result;
                    result.success = false;
                    result.message = "Constructor functions cannot have arguments";
                    result.line = f->getNameToken().getLine();
                    result.in = f->getNameToken().getFile();
                    result.column = f->getNameToken().getColumn();
                    result.type = f->getNameToken().getType();
                    result.value = f->getNameToken().getValue();
                    errors.push_back(result);
                } else {
                    append("  Function_%s();\n", f->finalName().c_str());
                }
            }
        }
        append("} else {\n");
        append("  _scl_runtime_catch();\n");
        scopeDepth--;
        append("}\n");

        append("}\n\n");
        append("_scl_destructor void destroy_this() {\n");
        append("TRY {\n");
        for (Function* f : result.functions) {
            if (!f->isMethod && isDestroyFunction(f)) {
                if (f->getArgs().size()) {
                    FPResult result;
                    result.success = false;
                    result.message = "Finalizer functions cannot have arguments";
                    result.line = f->getNameToken().getLine();
                    result.in = f->getNameToken().getFile();
                    result.column = f->getNameToken().getColumn();
                    result.type = f->getNameToken().getType();
                    result.value = f->getNameToken().getValue();
                    errors.push_back(result);
                } else {
                    append("  Function_%s();\n", f->finalName().c_str());
                }
            }
        }
        append("} else {\n");
        append("  _scl_runtime_catch();\n");
        append("}\n");
        scopeDepth--;
        append("}\n");

        if (mainFunction && !Main.options.noMain) {
            append("int main(int argc, char** argv) {\n");
            scopeDepth++;
            append("return _scl_run(argc, argv, (mainFunc) &Function_main);\n");
            scopeDepth--;
            append("}\n\n");
        }
        
        fclose(fp);

        FPResult parseResult;
        parseResult.success = true;
        parseResult.message = "";
        parseResult.errors = errors;
        parseResult.warns = warns;
        return parseResult;
    }
}
