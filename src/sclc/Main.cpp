#ifndef MAIN_CPP_
#define MAIN_CPP_

#if __SIZEOF_POINTER__ < 8
#error "Scale is not supported on this platform"
#endif

#include <iostream>
#include <string>
#include <signal.h>
#include <errno.h>
#include <chrono>

#include "Common.hpp"

#include "comp/Tokenizer.cpp"
#include "comp/Lexer.cpp"
#include "comp/Parser.cpp"

#ifndef VERSION
#define VERSION "unknown. Did you forget to build with -DVERSION=<version>?"
#endif

#ifndef PREPROCESSOR
#define PREPROCESSOR "cpp"
#endif

namespace sclc
{
    void usage(std::string programName) {
        std::cout << "Usage: " << programName << " <filename> [args]" << std::endl;
        std::cout << "  --transpile, -t  Transpile only" << std::endl;
        std::cout << "  --help, -h       Show this help" << std::endl;
        std::cout << "  -o <filename>    Specify Output file" << std::endl;
        std::cout << "  -E               Preprocess only" << std::endl;
    }

    int main(int argc, char const *argv[])
    {
        signal(SIGSEGV, signalHandler);
        signal(SIGABRT, signalHandler);
        signal(SIGILL, signalHandler);
        signal(SIGFPE, signalHandler);
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
    #ifdef SIGQUIT
        signal(SIGQUIT, signalHandler);
    #endif
        if (argc < 2) {
            usage(argv[0]);
            return 1;
        }

        auto start = std::chrono::high_resolution_clock::now();

        bool transpileOnly  = false;
        bool preprocessOnly = false;
        bool onlyAssemble   = false;

        std::string outfile = "out.scl";
        std::string cmd     = "clang -I" + std::string(getenv("HOME")) + "/Scale/comp -std=gnu17 -O2 -o " + outfile + " -DVERSION=\"" + std::string(VERSION) + "\" ";
        std::vector<std::string> files;

        for (int i = 1; i < argc; i++) {
            if (strends(std::string(argv[i]), ".scale")) {
                files.push_back(std::string(argv[i]));
            } else {
                if (strcmp(argv[i], "--transpile") == 0 || strcmp(argv[i], "-t") == 0) {
                    transpileOnly = true;
                } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                    usage(std::string(argv[0]));
                    return 0;
                } else if (strcmp(argv[i], "-E") == 0) {
                    preprocessOnly = true;
                } else if (strcmp(argv[i], "-S") == 0) {
                    onlyAssemble = true;
                    cmd += "-S ";
                } else {
                    cmd += std::string(argv[i]) + " ";
                }
            }
        }
        if (!onlyAssemble) {
            cmd += std::string(getenv("HOME")) + "/Scale/comp/scale.c ";
        }

        std::cout << "Scale Compiler version " << std::string(VERSION) << std::endl;
        
        std::vector<Token>  tokens;

        for (int i = 0; i < files.size(); i++) {
            std::string filename = files[i];
            std::cout << "Compiling " << filename << "..." << std::endl;

            std::string preproc_cmd =
                std::string(PREPROCESSOR) + " -I" + std::string(getenv("HOME"))
                + "/Scale/lib " + filename + " " + filename + ".scale-preproc";
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
            auto end = std::chrono::high_resolution_clock::now();
            double duration = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000000000.0;
            std::cout << "Transpiled successfully in " << duration << " seconds." << std::endl;
            return 0;
        }

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

        auto end = std::chrono::high_resolution_clock::now();
        double duration = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000000000.0;
        std::cout << "Took " << duration << " seconds." << std::endl;

        return 0;
    }
}

int main(int argc, char const *argv[])
{
    return sclc::main(argc, argv);
}

#endif
