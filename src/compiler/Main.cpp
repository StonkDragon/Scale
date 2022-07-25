#ifndef MAIN_CPP_
#define MAIN_CPP_

#include <iostream>
#include <string>
#include <signal.h>
#include <errno.h>
#include <chrono>

#include "Common.h"

std::string outfile = "out.scl";
std::string lib = std::string(getenv("HOME")) + "/Scale/comp/scale.o";
std::string cmd = "clang -o " + outfile + " ";
std::vector<std::string> files;

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

int main(int argc, char const *argv[])
{
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGILL, signalHandler);
    signal(SIGFPE, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <filename> [args]" << std::endl;
        return 1;
    }

    bool transpileOnly = false;
    bool linkToLib = true;

    for (int i = 1; i < argc; i++) {
        if (strends(std::string(argv[i]), ".scale")) {
            files.push_back(std::string(argv[i]));
        } else {
            if (strcmp(argv[i], "--transpile") == 0 || strcmp(argv[i], "-t") == 0) {
                transpileOnly = true;
            } else if (strcmp(argv[i], "--no-scale-lib") == 0) {
                linkToLib = false;
            } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                std::cout << "Usage: " << argv[0] << " <filename> [args]" << std::endl;
                std::cout << "  --transpile, -t  Transpile only" << std::endl;
                std::cout << "  --no-scale-lib   Don't link to scale library" << std::endl;
                std::cout << "  --help, -h       Show this help" << std::endl;
                std::cout << "  -o <filename>    Output file" << std::endl;
                std::cout << std::endl << "  If no output file is specified, the output file will be" << std::endl;
                std::cout << "  out.scl" << std::endl;
                std::cout << std::endl << "  Any other unrecognized arguments will be passed to the compiler" << std::endl;
                return 0;
            } else {
                cmd += std::string(argv[i]) + " ";
            }
        }
    }

    if (linkToLib) {
        cmd += lib + " ";
    }

    std::cout << "----------------------------------------" << std::endl;
    
    std::vector<double> times;

    std::vector<Token> tokens;

    for (int i = 0; i < files.size(); i++) {
        std::string filename = files[i];
        std::cout << "Compiling " << filename << "..." << std::endl;

        auto startTokenizer = std::chrono::high_resolution_clock::now();
        Tokenizer tokenizer;
        MAIN.tokenizer = &tokenizer;
        MAIN.tokenizer->tokenize(filename);
        std::vector<Token> theseTokens = MAIN.tokenizer->getTokens();

        tokens.insert(tokens.end(), theseTokens.begin(), theseTokens.end());

        auto endTokenizer = std::chrono::high_resolution_clock::now();
        double durationTokenizer = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(endTokenizer - startTokenizer).count() / 1000000000.0;

        times.push_back(durationTokenizer);
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
    
    if (!transpileOnly) sprintf(source, ".scale-%08x.tmp", (unsigned int) rand());
    else sprintf(source, "out.c");
    cmd += source;
    cmd += ".c ";

    ParseResult parseResult = MAIN.parser->parse(std::string(source));
    auto endParser = std::chrono::high_resolution_clock::now();
    double durationParser = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(endParser - startParser).count() / 1000000000.0;

    if (!parseResult.success) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Compilation failed with code 1" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
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
        fail:
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Compilation failed with code " << ret << std::endl;
        std::cout << "----------------------------------------" << std::endl;
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
