#include <iostream>
#include <string>

#include "Tokenizer.cpp"
#include "Lexer.cpp"
#include "Parser.cpp"

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    std::string filename = argv[1];

    Tokenizer tokenizer;
    tokenizer.tokenize(filename);
    std::vector<Token> tokens = tokenizer.getTokens();

    Lexer lexer(tokens);
    AnalyzeResult result = lexer.lexAnalyze();

    Parser parser(result);
    ParseResult parseResult = parser.parse(filename);

    std::string outfile = filename.substr(0, filename.size() - 6) + ".scl";
    std::string cmd = "gcc -o " + outfile + " " + filename + ".c ";
    for (int i = 2; i < argc; i++) {
        cmd += std::string(argv[i]) + " ";
    }
    system(cmd.c_str());

    if (parseResult.success) {
        std::cout << "Success!" << std::endl;
    } else {
        std::cout << "Error!" << std::endl;
        return 1;
    }

    return 0;
}
