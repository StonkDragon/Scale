#ifndef _LEX_PARSER_CPP_
#define _LEX_PARSER_CPP_

#include <iostream>
#include <string>
#include <vector>

#include "Tokenizer.cpp"

class Variable
{
private:
    std::string name;
    std::string type;
    bool isConstant;
    bool isArray;
    int scopeLevel;
public:
    Variable(std::string name, std::string type, bool isConstant, bool isArray, int scopeLevel) {
        this->name = name;
        this->type = type;
        this->isConstant = isConstant;
        this->isArray = isArray;
        this->scopeLevel = scopeLevel;
    }
    ~Variable() {}
    std::string getName() { return name; }
    std::string getType() { return type; }
    bool checkIsConstant() { return isConstant; }
    bool checkIsArray() { return isArray; }
    int getScopeLevel() { return scopeLevel; }
};

enum Modifier
{
    mod_static,
    mod_inline,
    mod_extern
};

struct Function
{
    std::string name;
    std::vector<Token> body;
    std::vector<Modifier> modifiers;
    Function(std::string name) {
        this->name = name;
    }
    ~Function() {}
    std::string getName() {
        return name;
    }
    std::vector<Token> getBody() {
        return body;
    }
    void addToken(Token token) {
        body.push_back(token);
    }
    void addModifier(Modifier modifier) {
        modifiers.push_back(modifier);
    }
    std::vector<Modifier> getModifiers() {
        return modifiers;
    }
};

struct Extern
{
    std::string name;
    Extern(std::string name) {
        this->name = name;
    }
    ~Extern() {}
};

struct AnalyzeResult {
    std::vector<Function> functions;
    std::vector<Extern> externs;
};

class Lexer
{
private:
    std::vector<Token> tokens;
    std::vector<Function> functions;
    std::vector<Extern> externs;
public:
    Lexer(std::vector<Token> tokens);
    ~Lexer() {}
    AnalyzeResult lexAnalyze();
    static bool isOperator(Token token);
    static bool isType(Token token);
    static bool canAssign(Token token);
};

Lexer::Lexer(std::vector<Token> tokens)
{
    this->tokens = tokens;
}

inline bool fileExists (const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}

AnalyzeResult Lexer::lexAnalyze()
{
    Function* currentFunction = nullptr;

    bool isInline = false;
    bool isStatic = false;
    bool isExtern = false;

    for (int i = 0; i < tokens.size(); i++)
    {
        Token token = tokens[i];
        if (token.getType() == tok_function) {
            std::string name = tokens[i + 1].getValue();
            Function function(name);
            functions.push_back(function);
            currentFunction = &functions[functions.size() - 1];
            i++;
        } else if (token.getType() == tok_end) {
            if (isInline) {
                currentFunction->addModifier(mod_inline);
                isInline = false;
            }
            if (isStatic) {
                currentFunction->addModifier(mod_static);
                isStatic = false;
            }
            if (isExtern) {
                currentFunction->addModifier(mod_extern);
                isExtern = false;
            }
            currentFunction = nullptr;
        } else if (token.getType() == tok_hash) {
            if (currentFunction == nullptr) {
                if (tokens[i + 1].getType() == tok_identifier) {
                    if (tokens[i + 1].getValue() == "static") {
                        isStatic = true;
                    } else if (tokens[i + 1].getValue() == "inline") {
                        isInline = true;
                    } else if (tokens[i + 1].getValue() == "extern") {
                        isExtern = true;
                    } else {
                        std::cerr << "Error: " << tokens[i + 1].getValue() << " is not a valid modifier." << std::endl;
                    }
                } else {
                    std::cerr << "Error: " << tokens[i + 1].getValue() << " is not a valid modifier." << std::endl;
                }
            } else {
                std::cerr << "Error: Cannot use modifiers inside a function." << std::endl;
            }
        } else if (token.getType() == tok_using) {
            std::string file = tokens[i + 1].getValue();
            Tokenizer tokenizer;
            std::string filename = std::string(getenv("HOME")) + "/Scale/lib/" + file;
            if (!fileExists(filename)) {
                filename = file;
            }
            tokenizer.tokenize(filename);
            std::vector<Token> newTokens = tokenizer.getTokens();
            std::vector<Token> currentTokens = tokens;
            tokens.clear();
            for (int j = 0; j < newTokens.size(); j++) {
                tokens.push_back(newTokens[j]);
            }
            for (int j = 0; j < currentTokens.size(); j++) {
                if (j == i || j == i + 1) {
                    continue;
                }
                tokens.push_back(currentTokens[j]);
            }
            i--;
        } else if (token.getType() == tok_extern) {
            std::string name = tokens[i + 1].getValue();
            Extern externFunction(name);
            externs.push_back(externFunction);
        } else {
            if (currentFunction != nullptr) {
                currentFunction->addToken(token);
            }
        }
    }

    AnalyzeResult result;
    result.functions = functions;
    result.externs = externs;
    return result;
}

#endif
