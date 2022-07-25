#ifndef _LEX_PARSER_CPP_
#define _LEX_PARSER_CPP_

#include <iostream>
#include <string>
#include <vector>

#include "Common.h"

#include "Tokenizer.cpp"
#include "Main.cpp"

inline bool fileExists (const std::string& name) {
    FILE *file;
    if ((file = fopen(name.c_str(), "r")) != NULL) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

template <typename T>
void addIfAbsent(std::vector<T>& vec, T str) {
    for (int i = 0; i < vec.size(); i++) {
        if (vec[i].equals(str)) {
            return;
        }
    }
    vec.push_back(str);
}

AnalyzeResult Lexer::lexAnalyze()
{
    Function* currentFunction = nullptr;

    bool isInline = false;
    bool isStatic = false;
    bool isExtern = false;

    std::vector<std::string> uses;

    for (int i = 0; i < tokens.size(); i++)
    {
        Token token = tokens[i];
        if (token.getType() == tok_function) {
            std::string name = tokens[i + 1].getValue();
            Function function(name);
            addIfAbsent<Function>(functions, function);
            currentFunction = &functions[functions.size() - 1];
            i += 2;
            if (tokens[i].getType() == tok_open_paren) {
                i++;
                while (tokens[i].getType() == tok_identifier || tokens[i].getType() == tok_comma) {
                    if (tokens[i].getType() == tok_identifier) {
                        currentFunction->addArgument(tokens[i].getValue());
                    }
                    i++;
                    if (tokens[i].getType() == tok_comma || tokens[i].getType() == tok_close_paren) {
                        if (tokens[i].getType() == tok_comma) {
                            i++;
                        }
                    } else {
                        std::cerr << "Expected: ',' or ')', but got '" << tokens[i].getValue() << "'" << std::endl;
                        exit(1);
                    }
                }
            }
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
        } else if (token.getType() == tok_proto) {
            std::string name = tokens[i + 1].getValue();
            prototypes.push_back(Prototype(name));
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
    result.prototypes = prototypes;
    return result;
}

#endif
