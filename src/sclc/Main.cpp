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
#include "DragonConfig.hpp"

#ifndef VERSION
#define VERSION "unknown. Did you forget to build with -DVERSION=<version>?"
#endif

#ifndef PREPROCESSOR
#define PREPROCESSOR "cpp"
#endif

#ifndef COMPILER_FEATURES
#define COMPILER_FEATURES "-fapprox-func -fgnu-keywords -fjump-tables -fmath-errno"
#endif

namespace sclc
{
    std::string scaleFolder;

    void usage(std::string programName) {
        std::cout << "Usage: " << programName << " <filename> [args]" << std::endl;
        std::cout << "  --transpile, -t  Transpile only" << std::endl;
        std::cout << "  --help, -h       Show this help" << std::endl;
        std::cout << "  -o <filename>    Specify Output file" << std::endl;
        std::cout << "  -E               Preprocess only" << std::endl;
        std::cout << "  -f <framework>   Use Scale Framework" << std::endl;
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

        std::string outfile     = "out.scl";
        scaleFolder             = std::string(getenv("HOME")) + "/Scale";
        std::string cmd         = "clang " + std::string(COMPILER_FEATURES) + " -I" + scaleFolder + "/Frameworks -std=gnu17 -O2 -DVERSION=\"" + std::string(VERSION) + "\" ";
        std::vector<std::string> files;
        std::vector<std::string> frameworks;

        frameworks.push_back("Core");

        for (size_t i = 1; i < args.size(); i++) {
            if (strends(std::string(args[i]), ".scale")) {
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
                    cmd += "-S ";
                } else {
                    cmd += args[i] + " ";
                }
            }
        }

        cmd += "-o \"" + outfile + "\" ";

        std::string globalPreproc = std::string(PREPROCESSOR);
        const double FrameworkMinimumVersion = 2.8;

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

            double versionNum = stod(version);
            double compilerVersionNum = stod(VERSION);
            if (versionNum > compilerVersionNum) {
                std::cerr << "Error: Framework '" << framework << "' requires Scale " << version << " but you are using " << VERSION << std::endl;
                return 1;
            }
            if (versionNum < FrameworkMinimumVersion) {
                fprintf(stderr, "Error: Framework '%s' is too outdated (%.1f). Please update it to at least version %.1f\n", framework.c_str(), versionNum, FrameworkMinimumVersion);
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
                cmd += flag + " ";
            }

            MAIN.frameworks.push_back(framework);

            globalPreproc += " -I" + scaleFolder + "/Frameworks/" + framework + ".framework/" + headerDir;
            unsigned long implementersSize = implementers.size();
            for (unsigned long i = 0; i < implementersSize; i++) {
                std::string implementer = implementers.get(i);
                if (!assembleOnly)
                    cmd += scaleFolder + "/Frameworks/" + framework + ".framework/" + implDir + "/" + implementer + " ";
            }
            for (unsigned long i = 0; i < implHeaders.size(); i++) {
                std::string header = framework + ".framework/" + implHeaderDir + "/" + implHeaders.get(i);
                MAIN.frameworkNativeHeaders.push_back(header);
            }
        }

        std::cout << "Scale Compiler version " << std::string(VERSION) << std::endl;
        
        std::vector<Token>  tokens;

        for (size_t i = 0; i < files.size(); i++) {
            std::string filename = files[i];
            std::cout << "Compiling " << filename << "..." << std::endl;

            std::string preproc_cmd = globalPreproc + " " + filename + " " + filename + ".scale-preproc";
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
            MAIN.tokenizer = &tokenizer;
            MAIN.tokenizer->tokenize(filename + ".scale-preproc");
            std::vector<Token> theseTokens = MAIN.tokenizer->getTokens();

            tokens.insert(tokens.end(), theseTokens.begin(), theseTokens.end());

            remove(std::string(filename + ".scale-preproc").c_str());
        }

        if (preprocessOnly) {
            std::cout << "Preprocessed " << files.size() << " files." << std::endl;
            return 0;
        }

        Lexer lexer(tokens);
        MAIN.lexer = &lexer;
        AnalyzeResult result = MAIN.lexer->lexAnalyze();

        Parser parser(result);
        MAIN.parser = &parser;
        char* source = (char*) malloc(sizeof(char) * 50);
        
        srand(time(NULL));
        if (!transpileOnly) sprintf(source, ".scale-%08x.tmp", (unsigned int) rand());
        else sprintf(source, "out");
        cmd += source;
        cmd += ".c ";

        ParseResult parseResult = MAIN.parser->parse(std::string(source));

        if (parseResult.errors.size() > 0) {
            for (ParseResult error : parseResult.errors) {
                if (error.where == 0) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.message << std::endl;
                    continue;
                }
                FILE* f = fopen(std::string(error.in).c_str(), "r");
                char* line = (char*) malloc(sizeof(char) * 500);
                int i = 1;
                fseek(f, 0, SEEK_SET);
                std::cerr << Color::BOLDRED << "Error: " << Color::RESET << error.in << ":" << error.where << ":" << error.column << ": " << error.message << std::endl;
                i = 1;
                while (fgets(line, 500, f) != NULL) {
                    if (i == error.where) {
                        std::cerr << Color::BOLDRED << "> " << Color::RESET;
                        std::string l = replaceFirstAfter(line, error.token, Color::BOLDRED + error.token + Color::RESET, error.column);
                        if (l.at(l.size() - 1) != '\n') {
                            l += '\n';
                        }
                        std::cerr << l;
                    } else if (i == error.where - 1 || i == error.where - 2) {
                        if (strlen(line) > 0)
                            std::cerr << "  " << line;
                    } else if (i == error.where + 1 || i == error.where + 2) {
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
