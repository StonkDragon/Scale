#ifndef MAIN_CPP_
#define MAIN_CPP_

#include <iostream>
#include <string>
#include <signal.h>
#include <errno.h>
#include <chrono>

#include "Common.h"

#include "Tokenizer.cpp"
#include "Lexer.cpp"
#include "Parser.cpp"

void signalHandler(int signum)
{
    std::cout << "Signal " << signum << " received." << std::endl;
    std::cout << "Error: " << strerror(errno) << std::endl;
    exit(signum);
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
        std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    std::string filename = argv[1];
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Compiling " << filename << "..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    auto startTokenizer = std::chrono::high_resolution_clock::now();

    Tokenizer tokenizer;
    MAIN.tokenizer = &tokenizer;
    MAIN.tokenizer->tokenize(filename);
    std::vector<Token> tokens = MAIN.tokenizer->getTokens();

    auto endTokenizer = std::chrono::high_resolution_clock::now();
    double durationTokenizer = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(endTokenizer - startTokenizer).count() / 1000000000.0;

    auto startLexer = std::chrono::high_resolution_clock::now();
    Lexer lexer(tokens, filename);
    MAIN.lexer = &lexer;
    AnalyzeResult result = MAIN.lexer->lexAnalyze();
    auto endLexer = std::chrono::high_resolution_clock::now();
    double durationLexer = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(endLexer - startLexer).count() / 1000000000.0;

    auto startParser = std::chrono::high_resolution_clock::now();
    Parser parser(result);
    MAIN.parser = &parser;
    ParseResult parseResult = MAIN.parser->parse(filename);
    auto endParser = std::chrono::high_resolution_clock::now();
    double durationParser = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(endParser - startParser).count() / 1000000000.0;

    if (!parseResult.success) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Compilation failed with code 1" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        return 1;
    }

    auto startCodegen = std::chrono::high_resolution_clock::now();
    std::string outfile = filename.substr(0, filename.size() - 6) + ".scl";
    std::string lib = std::string(getenv("HOME")) + "/Scale/comp/scale.o";
    std::string cmd = "clang " + lib + " -o " + outfile + " " + filename + ".c ";
    for (int i = 2; i < argc; i++) {
        cmd += std::string(argv[i]) + " ";
    }
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
    std::cout << "Compilation finished." << std::endl;
    std::cout << "Took " << durationTokenizer + durationLexer + durationParser + durationCodegen << " seconds." << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    return 0;
}

#endif
