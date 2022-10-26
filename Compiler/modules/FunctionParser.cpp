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
    Function getFunctionByName(TPResult result, std::string name) {
        for (Function func : result.functions) {
            if (func.getName() == name) {
                return func;
            }
        }
        for (Function func : result.extern_functions) {
            if (func.getName() == name) {
                return func;
            }
        }
        return Function("%NULFUNC%");
    }

    Container getContainerByName(TPResult result, std::string name) {
        for (Container container : result.containers) {
            if (container.getName() == name) {
                return container;
            }
        }
        return Container("");
    }

    Complex getComplexByName(TPResult result, std::string name) {
        for (Complex complex : result.complexes) {
            if (complex.getName() == name) {
                return complex;
            }
        }
        return Complex("");
    }

    bool hasFunction(TPResult result, Token name) {
        for (Function func : result.functions) {
            if (func.getName() == name.getValue()) {
                return true;
            }
        }
        for (Function func : result.extern_functions) {
            if (func.getName() == name.getValue()) {
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
        std::vector<std::string> globals;

        remove((filename + std::string(".c")).c_str());
        FILE* fp = fopen((filename + std::string(".c")).c_str(), "a");

        Function mainFunction = getFunctionByName(result, "main");
        if (mainFunction == Function("%NULFUNC%") && !Main.options.noMain) {
            FPResult result;
            result.success = false;
            result.message = "No entry point found";
            result.line = 0;
            result.in = filename;
            result.column = 0;
            errors.push_back(result);
        }

        Transpiler::writeHeader(fp);
        Transpiler::writeFunctionHeaders(fp, result);
        Transpiler::writeExternHeaders(fp, result);
        Transpiler::writeInternalFunctions(fp, result);
        Transpiler::writeGlobals(fp, globals, result);
        Transpiler::writeContainers(fp, result);
        Transpiler::writeComplexes(fp, result);
        Transpiler::writeFunctions(fp, errors, warns, globals, result);

        std::string mainCall = "  fn_main(void);\n";

        std::string mainEntry = 
        "int main(int argc, char const *argv[]) {\n"
        "  signal(SIGINT, process_signal);\n"
        "  signal(SIGILL, process_signal);\n"
        "  signal(SIGABRT, process_signal);\n"
        "  signal(SIGFPE, process_signal);\n"
        "  signal(SIGSEGV, process_signal);\n"
        "  signal(SIGBUS, process_signal);\n"
        "#ifdef SIGTERM\n"
        "  signal(SIGTERM, process_signal);\n"
        "#endif\n"
        "\n"
        "  for (int i = 1; i < argc; i++) {\n"
        "    ctrl_push_string((scl_str) argv[i]);\n"
        "  }\n"
        "\n"
        "  scl_security_required_arg_count(" + std::to_string(mainFunction.getArgs().size()) + ", \"main()\");\n"
        "  fn_main();\n"
        "  return 0;\n"
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
