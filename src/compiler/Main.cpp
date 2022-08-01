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

#include "Tokenizer.cpp"
#include "Lexer.cpp"
#include "Parser.cpp"

void signalHandler(int signum)
{
    std::cout << "Signal " << signum << " received." << std::endl;
    if (errno != 0) std::cout << "Error: " << strerror(errno) << std::endl;
    exit(signum);
}

bool strends(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
}

void usage(std::string programName) {
    std::cout << "Usage: " << programName << " <filename> [args]" << std::endl;
    std::cout << "  --transpile, -t  Transpile only" << std::endl;
    std::cout << "  --nostdlib       Don't link to default libraries" << std::endl;
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
        std::cout << "Usage: " << argv[0] << " <filename> [args]" << std::endl;
        return 1;
    }

    bool transpileOnly = false;
    bool linkToLib = true;
    bool preprocessOnly = false;

    std::string outfile = "out.scl";
    std::string lib = std::string(getenv("HOME")) + "/Scale/comp/scale.o";
    std::string cmd = "clang -I" + std::string(getenv("HOME")) + "/Scale/comp -std=gnu17 -O2 -o " + outfile + " ";
    std::vector<std::string> files;

    for (int i = 1; i < argc; i++) {
        if (strends(std::string(argv[i]), ".scale")) {
            files.push_back(std::string(argv[i]));
        } else {
            if (strcmp(argv[i], "--transpile") == 0 || strcmp(argv[i], "-t") == 0) {
                transpileOnly = true;
            } else if (strcmp(argv[i], "--nostdlib") == 0) {
                linkToLib = false;
                cmd += "-nostdlib ";
            } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                usage(std::string(argv[0]));
                return 0;
            } else if (strcmp(argv[i], "-E") == 0) {
                preprocessOnly = true;
            } else {
                cmd += std::string(argv[i]) + " ";
            }
        }
    }

    if (linkToLib) {
        cmd += lib + " ";
    }

    std::cout << "Scale Compiler version " << std::string(VERSION) << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    std::vector<double> times;

    std::vector<Token> tokens;

    for (int i = 0; i < files.size(); i++) {
        std::string filename = files[i];
        std::cout << "Compiling " << filename << "..." << std::endl;

        auto startpreproc = std::chrono::high_resolution_clock::now();
        std::string preproc_cmd =
              "cpp -I" + std::string(getenv("HOME"))
            + "/Scale/lib -P " + filename + " " + filename + ".scale-preproc";

        auto preprocResult = system(preproc_cmd.c_str());
        if (preprocResult != 0) {
            std::cout << "Error: Preprocessor failed." << std::endl;
            return 1;
        }
        auto endpreproc = std::chrono::high_resolution_clock::now();
        double durationPreproc = std::chrono::duration_cast<std::chrono::nanoseconds>(endpreproc - startpreproc).count() / 1000000000.0;

        if (preprocessOnly) {
            std::cout << "Preprocessed " << filename << " in " << durationPreproc << " seconds." << std::endl;
            continue;
        }

        auto startTokenizer = std::chrono::high_resolution_clock::now();
        Tokenizer tokenizer;
        MAIN.tokenizer = &tokenizer;
        MAIN.tokenizer->tokenize(filename + ".scale-preproc");
        std::vector<Token> theseTokens = MAIN.tokenizer->getTokens();

        tokens.insert(tokens.end(), theseTokens.begin(), theseTokens.end());

        auto endTokenizer = std::chrono::high_resolution_clock::now();
        double durationTokenizer = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(endTokenizer - startTokenizer).count() / 1000000000.0;

        remove(std::string(filename + ".scale-preproc").c_str());
        times.push_back(durationTokenizer + durationPreproc);
    }

    if (preprocessOnly) {
        std::cout << "Preprocessed " << files.size() << " files." << std::endl;
        return 0;
    }

    auto startLexer = std::chrono::high_resolution_clock::now();
    Lexer lexer(tokens);
    MAIN.lexer = &lexer;
    AnalyzeResult result = MAIN.lexer->lexAnalyze();
    auto endLexer = std::chrono::high_resolution_clock::now();
    double durationLexer = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(endLexer - startLexer).count() / 1000000000.0;

    auto startParser = std::chrono::high_resolution_clock::now();
    Parser parser(result);
    MAIN.parser = &parser;
    char* source = (char*) malloc(sizeof(char) * 50);
    
    srand(time(NULL));
    if (!transpileOnly) sprintf(source, ".scale-%08x.tmp", (unsigned int) rand());
    else sprintf(source, "out");
    cmd += source;
    cmd += ".c ";

    ParseResult parseResult = MAIN.parser->parse(std::string(source));
    auto endParser = std::chrono::high_resolution_clock::now();
    double durationParser = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(endParser - startParser).count() / 1000000000.0;

    if (!parseResult.success) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Compilation failed with code 1" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        remove((std::string(source) + ".c").c_str());
        remove((std::string(source) + ".h").c_str());
        return 1;
    }
    times.push_back(durationLexer + durationParser);

    std::cout << "----------------------------------------" << std::endl;

    if (transpileOnly) {
        double total = 0;
        for (int i = 0; i < times.size(); i++) {
            total += times[i];
        }
        std::cout << "Transpiled successfully in " << total << " seconds." << std::endl;
        return 0;
    }

    auto startCodegen = std::chrono::high_resolution_clock::now();
    int ret = system(cmd.c_str());
    auto endCodegen = std::chrono::high_resolution_clock::now();
    double durationCodegen = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(endCodegen - startCodegen).count() / 1000000000.0;

    if (ret != 0) {
        std::cout << "Compilation failed with code " << ret << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        remove((std::string(source) + ".c").c_str());
        remove((std::string(source) + ".h").c_str());
        return ret;
    }
    remove((std::string(source) + ".c").c_str());
    remove((std::string(source) + ".h").c_str());
    
    std::cout << "Compilation finished." << std::endl;
    double total = 0;
    for (int i = 0; i < times.size(); i++) {
        total += times[i];
    }
    std::cout << "Took " << total + durationCodegen << " seconds." << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    return 0;
}

#endif
