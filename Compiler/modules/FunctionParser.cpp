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
    Interface* getInterfaceByName(TPResult result, std::string name) {
        for (Interface* i : result.interfaces) {
            if (i->getName() == name) {
                return i;
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
            if (func->getName() == name.getValue() && (type == "*" || ((Method*) func)->getMemberType() == type)) {
                return true;
            }
        }
        for (Function* func : result.extern_functions) {
            if (!func->isMethod) continue;
            if (func->getName() == name.getValue() && (type == "*" || ((Method*) func)->getMemberType() == type)) {
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
    bool hasGlobal(TPResult result, std::string name) {
        for (Variable v : result.globals) {
            if (v.getName() == name) {
                return true;
            }
        }
        for (Variable v : result.extern_globals) {
            if (v.getName() == name) {
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

        std::vector<Variable> defaultScope;
        std::vector<std::vector<Variable>> tmp;
        vars = tmp;
        vars.clear();
        vars.push_back(defaultScope);

        ConvertC::writeHeader(fp, errors, warns);
        ConvertC::writeGlobals(fp, globals, result, errors, warns);
        ConvertC::writeContainers(fp, result, errors, warns);
        ConvertC::writeStructs(fp, result, errors, warns);
        ConvertC::writeFunctionHeaders(fp, result, errors, warns);
        ConvertC::writeExternHeaders(fp, result, errors, warns);
        ConvertC::writeFunctions(fp, errors, warns, globals, result);

        std::string push_args = "";
        if (mainFunction->getArgs().size() > 0) {
            push_args = "  ctrl_push_args(argc, argv);\n";
        }

        std::string main = "";
        if (mainFunction->getReturnType() == "none") {
            main = "Function_main();\n";
        } else {
            main = "return_value = Function_main();\n";
        }

        if (Main.options.noMain)
            goto after_main;
        append("int main(int argc, char** argv) {\n");
        append("#ifdef SIGINT\n");
        append("  signal(SIGINT, process_signal);\n");
        append("#endif\n");
        append("#ifdef SIGABRT\n");
        append("  signal(SIGABRT, process_signal);\n");
        append("#endif\n");
        append("#ifdef SIGSEGV\n");
        append("  signal(SIGSEGV, process_signal);\n");
        append("#endif\n");
        append("#ifdef SIGBUS\n");
        append("  signal(SIGBUS, process_signal);\n");
        append("#endif\n\n");
        append("  int return_value = 0;\n");
        if (push_args.size() > 0)
            append("  %s", push_args.c_str());
        for (Function* f : result.functions) {
            if (strncmp(f->getName().c_str(), "__init__", 8) == 0) {
                append("  Function_%s();\n", f->getName().c_str());
            }
        }
        append("  %s", main.c_str());
        for (Function* f : result.functions) {
            if (strncmp(f->getName().c_str(), "__destroy__", 11) == 0) {
                append("  Function_%s();\n", f->getName().c_str());
            }
        }
        append("  return return_value;\n");
        append("}\n");

    after_main:

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
