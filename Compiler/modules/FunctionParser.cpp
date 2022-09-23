#ifndef _PARSER_CPP_
#define _PARSER_CPP_

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <stdio.h>

#include "../Common.hpp"

namespace sclc
{
    Function getFunctionByName(TPResult result, std::string name) {
        for (Function func : result.functions) {
            if (func.name == name) {
                return func;
            }
        }
        for (Prototype proto : result.prototypes) {
            if (proto.name == name) {
                Function func(name);
                func.args = std::vector<std::string>();
                for (int i = 0; i < proto.argCount; i++) {
                    func.args.push_back("$arg" + std::to_string(i));
                }
                return func;
            }
        }
        return Function("%NULFUNC%");
    }

    Container getContainerByName(TPResult result, std::string name) {
        for (Container container : result.containers) {
            if (container.name == name) {
                return container;
            }
        }
        return Container("");
    }

    bool hasFunction(TPResult result, Token name) {
        for (Function func : result.functions) {
            if (func.name == name.getValue()) {
                return true;
            }
        }
        for (Prototype proto : result.prototypes) {
            if (proto.name == name.getValue()) {
                bool hasFunction = false;
                for (Function func : result.functions) {
                    if (func.name == proto.name) {
                        hasFunction = true;
                        break;
                    }
                }
                if (!hasFunction) {
                    std::cerr << name.getFile() << ":" << name.getLine() << ":" << name.getColumn() << ": Error: Missing Implementation for function " << name.getValue() << std::endl;
                    exit(1);
                }
                return true;
            }
        }
        return false;
    }
    bool hasExtern(TPResult result, Token name) {
        for (Extern extern_ : result.externs) {
            if (extern_.name == name.getValue()) {
                return true;
            }
        }
        return false;
    }
    bool hasContainer(TPResult result, Token name) {
        for (Container container_ : result.containers) {
            if (container_.name == name.getValue()) {
                return true;
            }
        }
        return false;
    }

    FPResult FunctionParser::parse(std::string filename) {
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
        Transpiler::writeFunctions(fp, errors, warns, globals, result);

        std::string mainCall = "  fn_main(";
        for (ssize_t j = (ssize_t) mainFunction.getArgs().size() - 1; j >= 0; j--) {
            if (j != (ssize_t) mainFunction.getArgs().size() - 1) {
                mainCall += ", ";
            }
            mainCall += "ctrl_pop()";
        }
        mainCall += ");\n";

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
        "  for (int i = argc - 1; i > 0; i--) {\n"
        "    ctrl_push_string(argv[i]);\n"
        "  }\n"
        "\n" +
        mainCall +
        "  heap_collect();\n"
        "  return 0;\n"
        "}\n";

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
