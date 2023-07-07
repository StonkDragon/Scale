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
    TPResult Parser::getResult() {
        return result;
    }

    template<typename T>
    bool compare(T& a, T& b) {
        if constexpr(std::is_pointer<T>::value) {
            return hash1((char*) a->getName().c_str()) < hash1((char*) b->getName().c_str());
        } else {
            return hash1((char*) a.getName().c_str()) < hash1((char*) b.getName().c_str());
        }
    }

    std::string sclFunctionToFriendlyString(Function*);
    std::string argsToGenericString(Function*);

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
        vars = tmp;
        vars.clear();
        vars.push_back(defaultScope);

        if (
            featureEnabled("default-interface-implementation") ||
            featureEnabled("default-interface-impl")
        ) {
            for (Struct s : result.structs) {
                for (std::string i : s.getInterfaces()) {
                    Interface* interface = getInterfaceByName(result, i);
                    for (Method* m : interface->getDefaultImplementations()) {
                        if (!hasMethod(result, m->getName(), s.getName())) {
                            Method* m2 = m->cloneAs(s.getName());
                            std::vector<Variable> args = m2->getArgs();
                            m2->clearArgs();
                            for (Variable v : args) {
                                if (v.getType() == "") {
                                    v.setType(s.getName());
                                }
                                m2->addArgument(v);
                            }
                            result.functions.push_back(m2);
                        }
                    }
                }
            }
        }

        ConvertC::writeHeader(fp, errors, warns);
        ConvertC::writeContainers(fp, result, errors, warns);
        ConvertC::writeStructs(fp, result, errors, warns);
        ConvertC::writeGlobals(fp, globals, result, errors, warns);
        ConvertC::writeFunctionHeaders(fp, result, errors, warns);
        ConvertC::writeExternHeaders(fp, result, errors, warns);
        ConvertC::writeFunctions(fp, errors, warns, globals, result, filename);

        append("_scl_constructor void init_this() {\n");
        scopeDepth++;
        append("_scl_setup();\n");
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
        append("}\n\n");
        append("_scl_destructor void destroy_this() {\n");
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
