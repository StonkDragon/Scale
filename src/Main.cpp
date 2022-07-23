#ifndef MAIN_CPP_
#define MAIN_CPP_

#include <iostream>
#include <string>
#include <signal.h>
#include <errno.h>

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

    Tokenizer tokenizer;
    MAIN.tokenizer = &tokenizer;
    MAIN.tokenizer->tokenize(filename);
    std::vector<Token> tokens = MAIN.tokenizer->getTokens();

    Lexer lexer(tokens, filename);
    MAIN.lexer = &lexer;
    AnalyzeResult result = MAIN.lexer->lexAnalyze();

    Parser parser(result);
    MAIN.parser = &parser;
    ParseResult parseResult = MAIN.parser->parse(filename);

    if (!parseResult.success) {
        std::cout << "Parse error" << std::endl;
        return 1;
    }

    std::string outfile = filename.substr(0, filename.size() - 6) + ".scl";
    std::string lib = std::string(getenv("HOME")) + "/Scale/comp/scale.o";
    std::string cmd = "clang " + lib + " -o " + outfile + " " + filename + ".c ";
    for (int i = 2; i < argc; i++) {
        cmd += std::string(argv[i]) + " ";
    }
    std::cout << cmd << std::endl;
    int ret = system(cmd.c_str());

    if (ret == 0) {
        std::cout << "Success!" << std::endl;
    } else {
        std::cout << "Error!" << std::endl;
        return 1;
    }

    return 0;
}

#endif
