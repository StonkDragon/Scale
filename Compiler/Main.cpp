#ifndef MAIN_CPP_
#define MAIN_CPP_

#if __SIZEOF_POINTER__ < 8
#error "Scale is not supported on this platform"
#endif

#include <iostream>
#include <string>
#include <chrono>
#include <cstdio>
#include <cstdlib>

#include <signal.h>
#include <errno.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "Common.hpp"
#include "modules/DragonConfig.hpp"

#ifndef VERSION
#define VERSION "unknown. Did you forget to build with -DVERSION=<version>?"
#endif

#ifndef COMPILER
#define COMPILER "gcc"
#endif

#ifndef PREPROCESSOR
#define PREPROCESSOR compiler + " -E"
#endif

#ifndef C_VERSION
#define C_VERSION "gnu17"
#endif

#ifndef SCALE_INSTALL_DIR
#define SCALE_INSTALL_DIR "Scale"
#endif

#ifndef DEFAULT_OUTFILE
#define DEFAULT_OUTFILE "out.scl"
#endif

#ifndef FRAMEWORK_VERSION_REQ
#define FRAMEWORK_VERSION_REQ "2.10"
#endif

#ifdef _WIN32
#define HOME getenv("USERPROFILE")
#else
#define HOME getenv("HOME")
#endif

namespace sclc
{
    std::string scaleFolder;

    void usage(std::string programName) {
        std::cout << "Usage: " << programName << " <filename> [args]" << std::endl;
        std::cout << "  -t, --transpile  Transpile only" << std::endl;
        std::cout << "  -h, --help       Show this help" << std::endl;
        std::cout << "  -o <filename>    Specify Output file" << std::endl;
        std::cout << "  -E               Preprocess only" << std::endl;
        std::cout << "  -f <framework>   Use Scale Framework" << std::endl;
        std::cout << "  --no-core        Do not implicitly require Core Framework" << std::endl;
        std::cout << "  --no-main        Do not check for main Function" << std::endl;
        std::cout << "  -v, --version    Show version information" << std::endl;
        std::cout << "  --comp <comp>    Use comp as the compiler instead of gcc" << std::endl;
    }

    bool contains(std::vector<std::string>& vec, std::string& item) {
        for (auto& i : vec) {
            if (i == item) {
                return true;
            }
        }
        return false;
    }

    int main(std::vector<std::string> args)
    {
        if (args.size() < 2) {
            usage(args[0]);
            return 1;
        }

        auto start = std::chrono::high_resolution_clock::now();

        bool transpileOnly      = false;
        bool preprocessOnly     = false;
        bool assembleOnly       = false;
        bool noCoreFramework    = false;

        std::string outfile     = std::string(DEFAULT_OUTFILE);
        std::string compiler    = std::string(COMPILER);
        scaleFolder             = std::string(HOME) + "/" + std::string(SCALE_INSTALL_DIR);
        std::vector<std::string> files;
        std::vector<std::string> frameworks;
        std::vector<std::string> tmpFlags;

        for (size_t i = 1; i < args.size(); i++) {
            if (strends(std::string(args[i]), ".scale")) {
                if (!fileExists(args[i])) {
                    continue;
                }
                files.push_back(args[i]);
            } else {
                if (args[i] == "--transpile" || args[i] == "-t") {
                    transpileOnly = true;
                } else if (args[i] == "--help" || args[i] == "-h") {
                    usage(args[0]);
                    return 0;
                } else if (args[i] == "-E") {
                    preprocessOnly = true;
                } else if (args[i] == "-f") {
                    if (i + 1 < args.size()) {
                        std::string framework = args[i + 1];
                        if (!fileExists(scaleFolder + "/Frameworks/" + framework + ".framework")) {
                            std::cerr << "Framework '" << framework << "' not found" << std::endl;
                            return 1;
                        }
                        frameworks.push_back(framework);
                        i++;
                    } else {
                        std::cerr << "Error: -f requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "-o") {
                    if (i + 1 < args.size()) {
                        outfile = args[i + 1];
                        i++;
                    } else {
                        std::cerr << "Error: -o requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "-S") {
                    assembleOnly = true;
                    tmpFlags.push_back("-S");
                } else if (args[i] == "--no-core") {
                    noCoreFramework = true;
                } else if (args[i] == "--no-main") {
                    Main.options.noMain = true;
                } else if (args[i] == "-v" || args[i] == "--version") {
                    std::cout << "Scale Compiler version " << std::string(VERSION) << std::endl;
                    system(COMPILER" -v");
                    return 0;
                } else if (args[i] == "--comp") {
                    if (i + 1 < args.size()) {
                        compiler = args[i + 1];
                        i++;
                    } else {
                        std::cerr << "Error: --comp requires an argument" << std::endl;
                        return 1;
                    }
                } else {
                    tmpFlags.push_back(args[i]);
                }
            }
        }

        std::vector<std::string> cflags;

        cflags.push_back(compiler);
        cflags.push_back("-I" + scaleFolder + "/Frameworks");
        cflags.push_back("-I" + scaleFolder + "/Internal");
        cflags.push_back(scaleFolder + "/Internal/scale_internal.c");
        cflags.push_back("-std=" + std::string(C_VERSION));
        cflags.push_back("-O2");
        cflags.push_back("-DVERSION=\"" + std::string(VERSION) + "\"");
#ifdef LINK_MATH
        cflags.push_back("-lm");
#endif

        if (files.size() == 0) {
            std::cerr << Color::RED << "No translation units specified." << std::endl;
            return 1;
        }

        cflags.push_back("-o");
        cflags.push_back("\"" + outfile + "\"");

        if (!noCoreFramework)
            frameworks.push_back("Core");

        std::string globalPreproc = std::string(PREPROCESSOR) + " -DVERSION=\"" + std::string(VERSION) + "\" ";
        Version FrameworkMinimumVersion = Version(std::string(FRAMEWORK_VERSION_REQ));

        for (std::string framework : frameworks) {
            DragonConfig::ConfigParser parser;
            DragonConfig::CompoundEntry root = parser.parse(scaleFolder + "/Frameworks/" + framework + ".framework/index.drg").getCompound("framework");
            DragonConfig::ListEntry implementers = root.getList("implementers");
            DragonConfig::ListEntry implHeaders = root.getList("implHeaders");
            DragonConfig::ListEntry depends = root.getList("depends");
            DragonConfig::ListEntry compilerFlags = root.getList("compilerFlags");
            std::string version = root.getString("version").getValue();
            std::string headerDir = root.getString("headerDir").getValue();
            std::string implDir = root.getString("implDir").getValue();
            std::string implHeaderDir = root.getString("implHeaderDir").getValue();

            Version ver = Version(version);
            Version compilerVersion = Version(VERSION);
            if (ver > compilerVersion) {
                std::cerr << "Error: Framework '" << framework << "' requires Scale v" << version << " but you are using " << VERSION << std::endl;
                return 1;
            }
            if (ver < FrameworkMinimumVersion) {
                fprintf(stderr, "Error: Framework '%s' is too outdated (%s). Please update it to at least version %s\n", framework.c_str(), ver.asString().c_str(), FrameworkMinimumVersion.asString().c_str());
                return 1;
            }

            for (size_t i = 0; i < depends.size(); i++) {
                std::string depend = depends.get(i);
                if (!contains(frameworks, depend)) {
                    std::cerr << "Error: Framework '" << framework << "' depends on '" << depend << "' but it is not included" << std::endl;
                    return 1;
                }
            }

            for (size_t i = 0; i < compilerFlags.size(); i++) {
                std::string flag = compilerFlags.get(i);
                cflags.push_back(flag);
            }

            Main.frameworks.push_back(framework);

            globalPreproc += " -I" + scaleFolder + "/Frameworks/" + framework + ".framework/" + headerDir;
            unsigned long implementersSize = implementers.size();
            for (unsigned long i = 0; i < implementersSize; i++) {
                std::string implementer = implementers.get(i);
                if (!assembleOnly) {
                    cflags.push_back(scaleFolder + "/Frameworks/" + framework + ".framework/" + implDir + "/" + implementer);
                }
            }
            for (unsigned long i = 0; i < implHeaders.size(); i++) {
                std::string header = framework + ".framework/" + implHeaderDir + "/" + implHeaders.get(i);
                Main.frameworkNativeHeaders.push_back(header);
            }
        }

        std::cout << "Scale Compiler version " << std::string(VERSION) << std::endl;
        
        std::vector<Token>  tokens;

        for (size_t i = 0; i < files.size(); i++) {
            std::string filename = files[i];
            std::cout << "Compiling " << filename << "..." << std::endl;

            FILE* tmp = fopen(filename.c_str(), "r");
            fseek(tmp, 0, SEEK_END);
            long size = ftell(tmp);
            fseek(tmp, 0, SEEK_SET);

            char* buffer = new char[size];
            fread(buffer, 1, size, tmp);

            FILE* file = fopen((filename + ".c").c_str(), "w");
            fwrite(buffer, size, 1, file);
            fclose(file);
            fclose(tmp);

            std::string preproc_cmd = globalPreproc + " " + filename + ".c -o " + filename + ".scale-preproc";
            int preprocResult = system(preproc_cmd.c_str());

            if (preprocResult != 0) {
                std::cout << "Error: Preprocessor failed." << std::endl;
                return 1;
            }

            if (preprocessOnly) {
                std::cout << "Preprocessed " << filename << std::endl;
                continue;
            }

            Tokenizer tokenizer;
            Main.tokenizer = &tokenizer;
            FPResult result = Main.tokenizer->tokenize(filename + ".scale-preproc");

            if (result.errors.size() > 0) {
                for (FPResult error : result.errors) {
                    if (error.line == 0) {
                        std::cout << Color::BOLDRED << "Fatal Error: " << error.message << std::endl;
                        continue;
                    }
                    FILE* f = fopen(std::string(error.in).c_str(), "r");
                    char* line = (char*) malloc(sizeof(char) * 500);
                    int i = 1;
                    fseek(f, 0, SEEK_SET);
                    std::cerr << Color::BOLDRED << "Error: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                    i = 1;
                    while (fgets(line, 500, f) != NULL) {
                        if (i == error.line) {
                            std::cerr << Color::BOLDRED << "> " << Color::RESET;
                            std::string l;
                            if (error.type == tok_string_literal) {
                                l = replaceFirstAfter(line, "\"" + error.value + "\"", Color::BOLDRED + "\"" + error.value + "\"" + Color::RESET, error.column);
                            } else if (error.type == tok_char_literal) {
                                char* c = new char[4];
                                snprintf(c, 4, "%c", (char) strtol(error.value.c_str(), NULL, 0));
                                l = replaceFirstAfter(line, "'"s + c + "'", Color::BOLDRED + "'"s + c + "'" + Color::RESET, error.column);
                            } else {
                                l = replaceFirstAfter(line, error.value, Color::BOLDRED + error.value + Color::RESET, error.column);
                            }
                            if (l.at(l.size() - 1) != '\n') {
                                l += '\n';
                            }
                            std::cerr << l;
                        } else if (i == error.line - 1 || i == error.line - 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        } else if (i == error.line + 1 || i == error.line + 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        }
                        i++;
                    }
                    fclose(f);
                    std::cerr << std::endl;
                    free(line);
                }
                remove(std::string(filename + ".scale-preproc").c_str());
                return result.errors.size();
            }

            std::vector<Token> theseTokens = Main.tokenizer->getTokens();

            tokens.insert(tokens.end(), theseTokens.begin(), theseTokens.end());

            remove(std::string(filename + ".scale-preproc").c_str());
            remove(std::string(filename + ".c").c_str());
        }

        if (preprocessOnly) {
            std::cout << "Preprocessed " << files.size() << " files." << std::endl;
            return 0;
        }

        TokenParser lexer(tokens);
        Main.lexer = &lexer;
        TPResult result = Main.lexer->parse();

        if (result.errors.size() > 0) {
            for (FPResult error : result.errors) {
                if (error.line == 0) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.message << std::endl;
                    continue;
                }
                FILE* f = fopen(std::string(error.in).c_str(), "r");
                char* line = (char*) malloc(sizeof(char) * 500);
                int i = 1;
                fseek(f, 0, SEEK_SET);
                std::cerr << Color::BOLDRED << "Error: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                i = 1;
                while (fgets(line, 500, f) != NULL) {
                    if (i == error.line) {
                        std::cerr << Color::BOLDRED << "> " << Color::RESET;
                        std::string l;
                        if (error.type == tok_string_literal) {
                            l = replaceFirstAfter(line, "\"" + error.value + "\"", Color::BOLDRED + "\"" + error.value + "\"" + Color::RESET, error.column);
                        } else if (error.type == tok_char_literal) {
                            char* c = new char[4];
                            snprintf(c, 4, "%c", (char) strtol(error.value.c_str(), NULL, 0));
                            l = replaceFirstAfter(line, "'"s + c + "'", Color::BOLDRED + "'"s + c + "'" + Color::RESET, error.column);
                        } else {
                            l = replaceFirstAfter(line, error.value, Color::BOLDRED + error.value + Color::RESET, error.column);
                        }
                        if (l.at(l.size() - 1) != '\n') {
                            l += '\n';
                        }
                        std::cerr << l;
                    } else if (i == error.line - 1 || i == error.line - 2) {
                        if (strlen(line) > 0)
                            std::cerr << "  " << line;
                    } else if (i == error.line + 1 || i == error.line + 2) {
                        if (strlen(line) > 0)
                            std::cerr << "  " << line;
                    }
                    i++;
                }
                fclose(f);
                std::cerr << std::endl;
                free(line);
            }
            return result.errors.size();
        }
        
        FunctionParser parser(result);
        Main.parser = &parser;
        char* source = (char*) malloc(sizeof(char) * 50);
        
        srand(time(NULL));
        if (!transpileOnly) snprintf(source, 21, ".scale-%08x.tmp", (unsigned int) rand());
        else snprintf(source, 4, "out");
        cflags.push_back(std::string(source) + ".c");

        FPResult parseResult = Main.parser->parse(std::string(source));

        if (parseResult.errors.size() > 0) {
            for (FPResult error : parseResult.errors) {
                if (error.line == 0) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.message << std::endl;
                    continue;
                }
                FILE* f = fopen(std::string(error.in).c_str(), "r");
                char* line = (char*) malloc(sizeof(char) * 500);
                int i = 1;
                fseek(f, 0, SEEK_SET);
                std::cerr << Color::BOLDRED << "Error: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                i = 1;
                while (fgets(line, 500, f) != NULL) {
                    if (i == error.line) {
                        std::cerr << Color::BOLDRED << "> " << Color::RESET;
                        std::string l;
                        if (error.type == tok_string_literal) {
                            l = replaceFirstAfter(line, "\"" + error.value + "\"", Color::BOLDRED + "\"" + error.value + "\"" + Color::RESET, error.column);
                        } else if (error.type == tok_char_literal) {
                            char* c = new char[4];
                            snprintf(c, 4, "%c", (char) strtol(error.value.c_str(), NULL, 0));
                            l = replaceFirstAfter(line, "\""s + c + "'", Color::BOLDRED + "'"s + c + "'" + Color::RESET, error.column);
                        } else {
                            l = replaceFirstAfter(line, error.value, Color::BOLDRED + error.value + Color::RESET, error.column);
                        }
                        if (l.at(l.size() - 1) != '\n') {
                            l += '\n';
                        }
                        std::cerr << l;
                    } else if (i == error.line - 1 || i == error.line - 2) {
                        if (strlen(line) > 0)
                            std::cerr << "  " << line;
                    } else if (i == error.line + 1 || i == error.line + 2) {
                        if (strlen(line) > 0)
                            std::cerr << "  " << line;
                    }
                    i++;
                }
                fclose(f);
                std::cerr << std::endl;
                free(line);
            }
            remove((std::string(source) + ".c").c_str());
            remove((std::string(source) + ".h").c_str());
            return parseResult.errors.size();
        }

        if (transpileOnly) {
            auto end = chrono::high_resolution_clock::now();
            double duration = (double) chrono::duration_cast<chrono::nanoseconds>(end - start).count() / 1000000000.0;
            std::cout << "Transpiled successfully in " << duration << " seconds." << std::endl;
            return 0;
        }

        std::string cmd = "";
        for (std::string s : cflags) {
            cmd += s + " ";
        }

        std::cout << "Compiling with " << cmd << std::endl;

        int ret = system(cmd.c_str());

        if (ret != 0) {
            std::cerr << Color::RED << "Compilation failed with code " << ret << Color::RESET << std::endl;
            remove((std::string(source) + ".c").c_str());
            remove((std::string(source) + ".h").c_str());
            return ret;
        }
        remove((std::string(source) + ".c").c_str());
        remove((std::string(source) + ".h").c_str());
        
        std::cout << Color::GREEN << "Compilation finished." << Color::RESET << std::endl;

        auto end = chrono::high_resolution_clock::now();
        double duration = (double) chrono::duration_cast<chrono::nanoseconds>(end - start).count() / 1000000000.0;
        std::cout << "Took " << duration << " seconds." << std::endl;

        return 0;
    }
}

int main(int argc, char const *argv[])
{
    signal(SIGSEGV, sclc::signalHandler);
    signal(SIGABRT, sclc::signalHandler);
    signal(SIGILL, sclc::signalHandler);
    signal(SIGFPE, sclc::signalHandler);
    signal(SIGINT, sclc::signalHandler);
    signal(SIGTERM, sclc::signalHandler);
#ifdef SIGQUIT
    signal(SIGQUIT, sclc::signalHandler);
#endif

    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        args.push_back(argv[i]);
    }
    return sclc::main(args);
}

#endif
