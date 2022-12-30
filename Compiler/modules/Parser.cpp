#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <stdio.h>

#include "../headers/Common.hpp"

namespace sclc
{
    TPResult Parser::getResult() {
        return result;
    }

    extern FILE* support_header;

    FPResult Parser::parse(std::string filename) {
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

        remove("scale_support.h");
        support_header = fopen("scale_support.h", "a");
        fprintf(support_header, "#include <scale_internal.h>\n\n");
        fprintf(support_header, "#define ssize_t signed long\n");
        fprintf(support_header, "typedef void* scl_any;\n");
        fprintf(support_header, "typedef long long scl_int;\n");
        fprintf(support_header, "typedef char* scl_str;\n");
        fprintf(support_header, "typedef double scl_float;\n\n");
        fprintf(support_header, "extern scl_stack_t stack;\n\n");

        ConvertC::writeHeader(fp, errors, warns);
        ConvertC::writeGlobals(fp, globals, result, errors, warns);
        ConvertC::writeContainers(fp, result, errors, warns);
        ConvertC::writeStructs(fp, result, errors, warns);
        ConvertC::writeFunctionHeaders(fp, result, errors, warns);
        ConvertC::writeExternHeaders(fp, result, errors, warns);
        ConvertC::writeFunctions(fp, errors, warns, globals, result);

        fclose(support_header);

        std::string push_args = "";
        if (mainFunction->getArgs().size() > 0) {
            push_args = "ctrl_push_args(argc, argv);\n";
        }

        std::string sclTypeToCType(TPResult result, std::string t);

        std::string main = "";
        if (mainFunction->getReturnType() == "none") {
            if (mainFunction->getArgs().size() != 0)
                main = "Function_main((" + sclTypeToCType(result, mainFunction->getArgs()[0].getType()) + ") stack.data[--stack.ptr].v);\n";
            else
                main = "Function_main();\n";
        } else {
            if (mainFunction->getArgs().size() != 0)
                main = "return_value = Function_main((" + sclTypeToCType(result, mainFunction->getArgs()[0].getType()) + ") stack.data[--stack.ptr].v);\n";
            else
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
