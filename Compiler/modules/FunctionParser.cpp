#ifndef _PARSER_CPP_
#define _PARSER_CPP_

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <stdio.h>

#include "../headers/Common.hpp"

namespace sclc
{
    Function* getFunctionByName(TPResult result, std::string name) {
        for (Function* func : result.functions) {
            if (func->isMethod) continue;
            if (func->getName() == name) {
                return func;
            }
        }
        for (Function* func : result.extern_functions) {
            if (func->isMethod) continue;
            if (func->getName() == name) {
                return func;
            }
        }
        return nullptr;
    }
    Method* getMethodByName(TPResult result, std::string name, std::string type) {
        for (Function* func : result.functions) {
            if (!func->isMethod) continue;
            if (func->getName() == name && ((Method*) func)->getMemberType() == type) {
                return (Method*) func;
            }
        }
        for (Function* func : result.extern_functions) {
            if (!func->isMethod) continue;
            if (func->getName() == name && ((Method*) func)->getMemberType() == type) {
                return (Method*) func;
            }
        }
        return nullptr;
    }

    Container getContainerByName(TPResult result, std::string name) {
        for (Container container : result.containers) {
            if (container.getName() == name) {
                return container;
            }
        }
        return Container("");
    }

    Struct getStructByName(TPResult result, std::string name) {
        for (Struct struct_ : result.structs) {
            if (struct_.getName() == name) {
                return struct_;
            }
        }
        return Struct("");
    }

    bool hasFunction(TPResult result, Token name) {
        for (Function* func : result.functions) {
            if (func->isMethod) continue;
            if (func->getName() == name.getValue()) {
                return true;
            }
        }
        for (Function* func : result.extern_functions) {
            if (func->isMethod) continue;
            if (func->getName() == name.getValue()) {
                return true;
            }
        }
        return false;
    }
    bool hasMethod(TPResult result, Token name, std::string type) {
        for (Function* func : result.functions) {
            if (!func->isMethod) continue;
            if (func->getName() == name.getValue() && ((Method*) func)->getMemberType() == type) {
                return true;
            }
        }
        for (Function* func : result.extern_functions) {
            if (!func->isMethod) continue;
            if (func->getName() == name.getValue() && ((Method*) func)->getMemberType() == type) {
                return true;
            }
        }
        return false;
    }
    bool hasContainer(TPResult result, Token name) {
        for (Container container_ : result.containers) {
            if (container_.getName() == name.getValue()) {
                return true;
            }
        }
        return false;
    }

    TPResult FunctionParser::getResult() {
        return result;
    }

    FPResult FunctionParser::parse(std::string filename) {
        int scopeDepth = 0;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::vector<Variable> globals;

        remove(filename.c_str());
        FILE* fp = fopen(filename.c_str(), "a");

        Function* mainFunction = getFunctionByName(result, "main");
        if (mainFunction == nullptr && !Main.options.noMain) {
            FPResult result;
            result.success = false;
            result.message = "No entry point found";
            result.line = 0;
            result.in = filename;
            result.column = 0;
            errors.push_back(result);
        }

        ConvertC::writeHeader(fp);
        ConvertC::writeFunctionHeaders(fp, result);
        ConvertC::writeExternHeaders(fp, result);
        ConvertC::writeInternalFunctions(fp, result);
        ConvertC::writeGlobals(fp, globals, result);
        ConvertC::writeContainers(fp, result);
        ConvertC::writeStructs(fp, result);
        ConvertC::writeFunctions(fp, errors, warns, globals, result);

        std::string push_args = "";
        if (mainFunction->getArgs().size() > 0) {
            push_args = "  ctrl_push_args(argc, argv);\n";
        }

        std::string main = "";
        if (mainFunction->getReturnType() == "none") {
            main = "  Function_main();\n"
            "  return 0;\n";
        } else {
            main = "  return Function_main();\n";
        }

        std::string mainEntry = 
        "int main(int argc, char** argv) {\n"
        "#ifdef SIGINT\n"
        "  signal(SIGINT, process_signal);\n"
        "#endif\n"
        "#ifdef SIGABRT\n"
        "  signal(SIGABRT, process_signal);\n"
        "#endif\n"
        "#ifdef SIGSEGV\n"
        "  signal(SIGSEGV, process_signal);\n"
        "#endif\n"
        "#ifdef SIGBUS\n"
        "  signal(SIGBUS, process_signal);\n"
        "#endif\n"
        "\n" +
        push_args +
        main +
        "}\n";

        if (!Main.options.noMain)
            append("%s\n", mainEntry.c_str());

        append("#ifdef __cplusplus\n");
        append("}\n");
        append("#endif\n");
        append("/* END OF GENERATED CODE */\n");

        fclose(fp);

        FPResult parseResult;
        parseResult.success = true;
        parseResult.message = "";
        parseResult.errors = errors;
        parseResult.warns = warns;
        return parseResult;
    }
}
#endif // _PARSER_CPP_
