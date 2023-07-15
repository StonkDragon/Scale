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
    TPResult& Parser::getResult() {
        return result;
    }

    FPResult Parser::parse(std::string filename) {
        int scopeDepth = 0;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::vector<Variable> globals;

        remove(filename.c_str());
        remove((filename.substr(0, filename.size() - 2) + ".h").c_str());
        remove((filename.substr(0, filename.size() - 2) + ".typeinfo.h").c_str());
        FILE* fp = fopen((filename.substr(0, filename.size() - 2) + ".h").c_str(), "a");

        Function* mainFunction = getFunctionByName(result, "main");
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

        auto writeFunctionsStart = std::chrono::high_resolution_clock::now();
        ConvertC::writeFunctions(fp, errors, warns, globals, result, filename);
        auto writeFunctionsEnd = std::chrono::high_resolution_clock::now();
        Main.writeFunctionsTime = std::chrono::duration_cast<std::chrono::microseconds>(writeFunctionsEnd - writeFunctionsStart).count();

        append("_scl_constructor void init_this() {\n");
        scopeDepth++;
        append("_scl_setup();\n");
        append("TRY {\n");
        scopeDepth++;
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
                    append("Function_%s();\n", f->finalName().c_str());
                }
            }
        }
        scopeDepth--;
        append("} else {\n");
        append("    _scl_runtime_catch();\n");
        append("}\n");

        scopeDepth--;
        append("}\n\n");
        append("_scl_destructor void destroy_this() {\n");
        append("TRY {\n");
        scopeDepth++;
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
                    append("Function_%s();\n", f->finalName().c_str());
                }
            }
        }
        scopeDepth--;
        append("} else {\n");
        append("    _scl_runtime_catch();\n");
        append("}\n");
        append("}\n\n");
        
        fclose(fp);

        FPResult parseResult;
        parseResult.success = true;
        parseResult.message = "";
        parseResult.errors = errors;
        parseResult.warns = warns;
        return parseResult;
    }
}
