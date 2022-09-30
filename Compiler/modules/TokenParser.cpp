#ifndef _LEX_PARSER_CPP_
#define _LEX_PARSER_CPP_

#include <iostream>
#include <string>
#include <vector>

#include "../Common.hpp"

namespace sclc
{
    TPResult TokenParser::parse()
    {
        Function* currentFunction = nullptr;
        Container* currentContainer = nullptr;
        Complex* currentComplex = nullptr;

        bool functionPrivateStack = true;
        bool funcNoWarn = false;
        bool funcSAP = false;

        std::vector<std::string> uses;
        std::vector<std::string> globals;
        std::vector<Container> containers;
        std::vector<Complex> complexes;

        std::vector<FPResult> errors;

        for (size_t i = 0; i < tokens.size(); i++)
        {
            Token token = tokens[i];
            if (token.getType() == tok_function) {
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Error: Cannot define function inside another function.";
                    result.column = tokens[i + 1].getColumn();
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Error: Cannot define function inside of a container.";
                    result.column = tokens[i + 1].getColumn();
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentComplex != nullptr) {
                    FPResult result;
                    result.message = "Error: Cannot define function inside of a complex.";
                    result.column = tokens[i + 1].getColumn();
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected itentifier after function keyword";
                    result.column = tokens[i + 1].getColumn();
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                std::string name = tokens[i + 1].getValue();
                Token func = tokens[i + 1];
                currentFunction = new Function(name);
                i += 2;
                if (tokens[i].getType() == tok_open_paren) {
                    i++;
                    while (i < tokens.size() && tokens[i].getType() != tok_close_paren) {
                        if (tokens[i].getType() == tok_identifier) {
                            currentFunction->addArgument(tokens[i].getValue());
                        } else {
                            FPResult result;
                            result.message = "Expected: identifier, but got '" + tokens[i].getValue() + "'";
                            result.column = tokens[i].getColumn();
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            i++;
                            continue;
                        }
                        i++;
                        if (tokens[i].getType() == tok_comma || tokens[i].getType() == tok_close_paren) {
                            if (tokens[i].getType() == tok_comma) {
                                i++;
                            }
                            continue;
                        }
                        FPResult result;
                        result.message = "Expected: ',' or ')', but got '" + tokens[i].getValue() + "'";
                        result.column = tokens[i].getColumn();
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.success = false;
                        errors.push_back(result);
                    }
                }
                if (currentFunction->args.size() > 32) {
                    FPResult result;
                    result.message = "Functions can't have more than 32 Arguments!";
                    result.column = func.getColumn();
                    result.value = func.getValue();
                    result.line = func.getLine();
                    result.in = func.getFile();
                    result.type = func.getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
            } else if (token.getType() == tok_end) {
                if (currentFunction != nullptr) {
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
                } else if (currentContainer != nullptr) {
                    containers.push_back(*currentContainer);
                    currentContainer = nullptr;
                } else if (currentComplex != nullptr) {
                    complexes.push_back(*currentComplex);
                    currentComplex = nullptr;
                } else {
                    FPResult result;
                    result.message = "Unexpected 'end' keyword outside of function, container or complex body.";
                    result.column = token.getColumn();
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
            } else if (token.getType() == tok_proto) {
                std::string name = tokens[++i].getValue();
                std::string argstring = tokens[++i].getValue();
                int argCount = stoi(argstring);
                prototypes.push_back(Prototype(name, argCount));
            } else if (token.getType() == tok_container_def) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a container inside another container.";
                    result.column = token.getColumn();
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a container inside of a function.";
                    result.column = token.getColumn();
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentComplex != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a container inside of a complex.";
                    result.column = token.getColumn();
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected itentifier for variable declaration, but got '" + tokens[i + 1].getValue() + "'";
                    result.column = tokens[i + 1].getColumn();
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                currentContainer = new Container(tokens[i].getValue());
            } else if (token.getType() == tok_complex_def) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a complex inside of a container.";
                    result.column = token.getColumn();
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a complex inside of a function.";
                    result.column = token.getColumn();
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentComplex != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a complex inside another complex.";
                    result.column = token.getColumn();
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected itentifier for variable declaration, but got '" + tokens[i + 1].getValue() + "'";
                    result.column = tokens[i + 1].getColumn();
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                currentComplex = new Complex(tokens[i].getValue());
            } else if (token.getType() == tok_hash) {
                if (currentFunction == nullptr && currentContainer == nullptr) {
                    if (tokens[i + 1].getType() == tok_identifier) {
                        if (tokens[i + 1].getValue() == "nps") {
                            functionPrivateStack = false;
                        } else if (tokens[i + 1].getValue() == "nowarn") {
                            funcNoWarn = true;
                        } else if (tokens[i + 1].getValue() == "sap") {
                            funcSAP = true;
                        } else {
                            FPResult result;
                            result.message = "Error: " + tokens[i + 1].getValue() + " is not a valid modifier.";
                            result.column = tokens[i + 1].getColumn();
                            result.value = tokens[i + 1].getValue();
                            result.line = tokens[i + 1].getLine();
                            result.in = tokens[i + 1].getFile();
                            result.type = tokens[i + 1].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                    } else {
                        FPResult result;
                        result.message = "Error: " + tokens[i + 1].getValue() + " is not a valid modifier.";
                        result.column = tokens[i + 1].getColumn();
                        result.value = tokens[i + 1].getValue();
                        result.line = tokens[i + 1].getLine();
                        result.in = tokens[i + 1].getFile();
                        result.type = tokens[i + 1].getType();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    i++;
                } else {
                    FPResult result;
                    result.message = "Error: Cannot use modifiers inside a function or container.";
                    result.column = token.getColumn();
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
            } else if (token.getType() == tok_extern && currentFunction == nullptr && currentContainer == nullptr && currentComplex == nullptr) {
                std::string name = tokens[i + 1].getValue();
                Extern externFunction(name);
                externs.push_back(externFunction);
                i++;
            } else {
                if (currentFunction != nullptr && currentContainer == nullptr && currentComplex == nullptr) {
                    currentFunction->addToken(token);
                } else {
                    if (token.getType() == tok_declare && currentContainer == nullptr && currentComplex == nullptr) {
                        if (tokens[i + 1].getType() != tok_identifier) {
                            FPResult result;
                            result.message = "Expected itentifier for variable declaration, but got '" + tokens[i + 1].getValue() + "'";
                            result.column = tokens[i + 1].getColumn();
                            result.value = tokens[i + 1].getValue();
                            result.line = tokens[i + 1].getLine();
                            result.in = tokens[i + 1].getFile();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        globals.push_back(tokens[i + 1].getValue());
                        i++;
                    } else if (token.getType() == tok_declare && currentContainer != nullptr) {
                        if (tokens[i + 1].getType() != tok_identifier) {
                            FPResult result;
                            result.message = "Expected itentifier for variable declaration, but got '" + tokens[i + 1].getValue() + "'";
                            result.column = tokens[i + 1].getColumn();
                            result.value = tokens[i + 1].getValue();
                            result.line = tokens[i + 1].getLine();
                            result.in = tokens[i + 1].getFile();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        currentContainer->addMember(tokens[i + 1].getValue());
                        i++;
                    } else if (token.getType() == tok_declare && currentComplex != nullptr) {
                        if (tokens[i + 1].getType() != tok_identifier) {
                            FPResult result;
                            result.message = "Expected itentifier for variable declaration, but got '" + tokens[i + 1].getValue() + "'";
                            result.column = tokens[i + 1].getColumn();
                            result.value = tokens[i + 1].getValue();
                            result.line = tokens[i + 1].getLine();
                            result.in = tokens[i + 1].getFile();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        currentComplex->addMember(tokens[i + 1].getValue());
                        i++;
                    }
                }
            }
        }

        TPResult result;
        result.functions = functions;
        result.externs = externs;
        result.prototypes = prototypes;
        result.globals = globals;
        result.containers = containers;
        result.complexes = complexes;
        result.errors = errors;
        return result;
    }
}
#endif
