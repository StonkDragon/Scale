#ifndef _LEX_PARSER_CPP_
#define _LEX_PARSER_CPP_

#include <iostream>
#include <string>
#include <vector>

#include "../Common.hpp"

namespace sclc
{
    AnalyzeResult Lexer::lexAnalyze()
    {
        Function* currentFunction = nullptr;

        bool functionPrivateStack = true;
        bool funcNoWarn = false;
        bool funcSAP = false;

        std::vector<std::string> uses;
        std::vector<std::string> globals;

        for (size_t i = 0; i < tokens.size(); i++)
        {
            Token token = tokens[i];
            if (token.getType() == tok_function) {
                if (currentFunction != nullptr) {
                    std::cerr << "Error: Cannot define function inside another function" << std::endl;
                    exit(1);
                }
                if (tokens[i + 1].getType() != tok_identifier) {
                    std::cerr << Color::BOLDRED << "Error: " << Color::RESET << tokens[i+1].getFile() << ":";
                    std::cerr << tokens[i+1].getLine() << ":" << tokens[i+1].getColumn();
                    std::cerr << ": " << "Expected identifier after function keyword" << std::endl;
                    exit(1);
                }
                std::string name = tokens[i + 1].getValue();
                currentFunction = new Function(name);
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
            } else if (token.getType() == tok_end && currentFunction != nullptr) {
                if (!functionPrivateStack) {
                    currentFunction->addModifier(mod_nps);
                    functionPrivateStack = true;
                }
                if (funcNoWarn) {
                    currentFunction->addModifier(mod_nowarn);
                    funcNoWarn = false;
                }
                if (funcSAP) {
                    currentFunction->addModifier(mod_sap);
                    funcSAP = false;
                }
                addIfAbsent(functions, *currentFunction);
                currentFunction = nullptr;
            } else if (token.getType() == tok_proto) {
                std::string name = tokens[++i].getValue();
                std::string argstring = tokens[++i].getValue();
                int argCount = stoi(argstring);
                prototypes.push_back(Prototype(name, argCount));
            } else if (token.getType() == tok_hash) {
                if (currentFunction == nullptr) {
                    if (tokens[i + 1].getType() == tok_identifier) {
                        if (tokens[i + 1].getValue() == "nps") {
                            functionPrivateStack = false;
                        } else if (tokens[i + 1].getValue() == "nowarn") {
                            funcNoWarn = true;
                        } else if (tokens[i + 1].getValue() == "sap") {
                            funcSAP = true;
                        } else {
                            std::cerr << "Error: " << tokens[i + 1].getValue() << " is not a valid modifier." << std::endl;
                        }
                    } else {
                        std::cerr << "Error: " << tokens[i + 1].getValue() << " is not a valid modifier." << std::endl;
                    }
                    i++;
                } else {
                    std::cerr << "Error: Cannot use modifiers inside a function." << std::endl;
                }
            } else if (token.getType() == tok_extern) {
                std::string name = tokens[i + 1].getValue();
                Extern externFunction(name);
                externs.push_back(externFunction);
                i++;
            } else {
                if (currentFunction != nullptr) {
                    currentFunction->addToken(token);
                } else {
                    if (token.getType() == tok_declare) {
                        if (tokens[i + 1].getType() != tok_identifier) {
                            std::cerr << "Error: Expected itentifier for variable declaration, but got '" << tokens[i + 1].getValue() + "'" << std::endl;
                        }
                        globals.push_back(tokens[i + 1].getValue());
                        i++;
                    }
                }
            }
        }

        AnalyzeResult result;
        result.functions = functions;
        result.externs = externs;
        result.prototypes = prototypes;
        result.globals = globals;
        return result;
    }
}
#endif
