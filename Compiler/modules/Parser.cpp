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

    extern FILE* support_header;

    template<typename T>
    bool compare(T& a, T& b) {
        if constexpr(std::is_pointer<T>::value) {
            return hash1((char*) a->getName().c_str()) < hash1((char*) b->getName().c_str());
        } else {
            return hash1((char*) a.getName().c_str()) < hash1((char*) b.getName().c_str());
        }
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
        fprintf(support_header, "extern _scl_stack_t stack;\n\n");

        std::sort(result.functions.begin(), result.functions.end(), compare<Function*>);
        std::sort(result.extern_functions.begin(), result.extern_functions.end(), compare<Function*>);
        std::sort(result.containers.begin(), result.containers.end(), compare<Container>);
        std::sort(result.structs.begin(), result.structs.end(), compare<Struct>);
        std::sort(result.globals.begin(), result.globals.end(), compare<Variable>);
        std::sort(result.extern_globals.begin(), result.extern_globals.end(), compare<Variable>);
        std::sort(result.interfaces.begin(), result.interfaces.end(), compare<Interface*>);

        ConvertC::writeHeader(fp, errors, warns);
        ConvertC::writeGlobals(fp, globals, result, errors, warns);
        ConvertC::writeContainers(fp, result, errors, warns);
        ConvertC::writeStructs(fp, result, errors, warns);
        ConvertC::writeFunctionHeaders(fp, result, errors, warns);
        ConvertC::writeExternHeaders(fp, result, errors, warns);
        ConvertC::writeFunctions(fp, errors, warns, globals, result, filename);

        fclose(support_header);

        append("scl_any init_functions[] = {\n");
        for (Function* f : result.functions) {
            if (strncmp(f->getName().c_str(), "__init__", 8) == 0) {
                append("  (scl_any) Function_%s,\n", f->getName().c_str());
            }
        }
        append("  0\n");
        append("};\n");
        append("scl_any destroy_functions[] = {\n");
        for (Function* f : result.functions) {
            if (strncmp(f->getName().c_str(), "__destroy__", 11) == 0) {
                append("  (scl_any) Function_%s,\n", f->getName().c_str());
            }
        }
        append("  0\n");
        append("};\n");

        append("scl_any _scl_get_main_addr() {\n");
        if (mainFunction && !Main.options.noMain) {
            append("  return Function_main;\n");
        } else {
            append("  return NULL;\n");
        }
        append("}\n");

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
