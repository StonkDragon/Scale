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
std::string cmd = "clang " + lib + " -o " + outfile + " ";
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
        std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strends(std::string(argv[i]), ".scale")) {
            files.push_back(std::string(argv[i]));
        } else {
            cmd += std::string(argv[i]) + " ";
        }
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
    
    sprintf(source, ".scale-%08x.tmp", (unsigned int) rand());
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

    auto startCodegen = std::chrono::high_resolution_clock::now();
    int ret = system(cmd.c_str());
    auto endCodegen = std::chrono::high_resolution_clock::now();
    double durationCodegen = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(endCodegen - startCodegen).count() / 1000000000.0;

    remove((std::string(source) + ".c").c_str());
    remove((std::string(source) + ".h").c_str());

    if (ret != 0) {
        fail:
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Compilation failed with code " << ret << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        return ret;
    }
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
