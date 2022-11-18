#ifndef _LEX_PARSER_CPP_
#define _LEX_PARSER_CPP_

#include <iostream>
#include <string>
#include <vector>

#include "../headers/Common.hpp"

namespace sclc
{
    TPResult TokenParser::parse() {
        Function* currentFunction = nullptr;
        Container* currentContainer = nullptr;
        Struct* currentStruct = nullptr;

        bool funcPrivateStack = true;
        bool funcNoWarn = false;
        bool funcSAP = false;
        bool funcNoMangle = false;

        std::vector<std::string> uses;
        std::vector<std::string> globals;
        std::vector<Container> containers;
        std::vector<Struct> structs;

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
                if (currentStruct != nullptr) {
                    FPResult result;
                    result.message = "Error: Cannot define function inside of a struct.";
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
                currentFunction->setFile(func.getFile());
                if (!funcPrivateStack) currentFunction->addModifier(mod_nps);
                if (funcNoWarn) currentFunction->addModifier(mod_nowarn);
                if (funcSAP) currentFunction->addModifier(mod_sap);
                if (funcNoMangle) {
                    if (currentFunction->getName() == "main") {
                        FPResult result;
                        result.message = "Main function must be mangled!";
                        result.column = tokens[i + 1].getColumn();
                        result.value = tokens[i + 1].getValue();
                        result.line = tokens[i + 1].getLine();
                        result.in = tokens[i + 1].getFile();
                        result.type = tokens[i + 1].getType();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    currentFunction->addModifier(mod_nomangle);
                }
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
                } else {
                    FPResult result;
                    result.message = "Expected: '(', but got '" + tokens[i].getValue() + "'";
                    result.column = tokens[i].getColumn();
                    result.value = tokens[i].getValue();
                    result.line = tokens[i].getLine();
                    result.in = tokens[i].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                }
            } else if (token.getType() == tok_end) {
                if (currentFunction != nullptr) {
                    funcPrivateStack = true;
                    funcNoWarn = false;
                    funcSAP = false;
                    funcNoMangle = false;
                    addIfAbsent(functions, *currentFunction);
                    currentFunction = nullptr;
                } else if (currentContainer != nullptr) {
                    containers.push_back(*currentContainer);
                    currentContainer = nullptr;
                } else if (currentStruct != nullptr) {
                    structs.push_back(*currentStruct);
                    currentStruct = nullptr;
                } else {
                    FPResult result;
                    result.message = "Unexpected 'end' keyword outside of function, container or struct body.";
                    result.column = token.getColumn();
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
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
                if (currentStruct != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a container inside of a struct.";
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
                if (tokens[i].getValue() == "str" || tokens[i].getValue() == "int" || tokens[i].getValue() == "float") {
                    FPResult result;
                    result.message = "Invalid name for container: '" + tokens[i + 1].getValue() + "'";
                    result.column = tokens[i + 1].getColumn();
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                currentContainer = new Container(tokens[i].getValue());
            } else if (token.getType() == tok_struct_def) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a struct inside of a container.";
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
                    result.message = "Cannot define a struct inside of a function.";
                    result.column = token.getColumn();
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentStruct != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a struct inside another struct.";
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
                if (tokens[i].getValue() == "str" || tokens[i].getValue() == "int" || tokens[i].getValue() == "float") {
                    FPResult result;
                    result.message = "Invalid name for struct: '" + tokens[i + 1].getValue() + "'";
                    result.column = tokens[i + 1].getColumn();
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                currentStruct = new Struct(tokens[i].getValue());
            } else if (token.getType() == tok_hash) {
                if (currentFunction == nullptr && currentContainer == nullptr) {
                    if (tokens[i + 1].getType() == tok_identifier) {
                        if (tokens[i + 1].getValue() == "nps") {
                            funcPrivateStack = false;
                        } else if (tokens[i + 1].getValue() == "nowarn") {
                            funcNoWarn = true;
                        } else if (tokens[i + 1].getValue() == "sap") {
                            funcSAP = true;
                        } else if (tokens[i + 1].getValue() == "nomangle") {
                            funcNoMangle = true;
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
            } else if (token.getType() == tok_extern && currentFunction == nullptr && currentContainer == nullptr && currentStruct == nullptr) {
                i++;
                Token type = tokens[i];
                if (type.getType() == tok_function) {
                    std::string name = tokens[i + 1].getValue();
                    Token funcTok = tokens[i + 1];
                    Function func = Function(name);
                    i += 2;
                    if (tokens[i].getType() == tok_open_paren) {
                        i++;
                        while (i < tokens.size() && tokens[i].getType() != tok_close_paren) {
                            if (tokens[i].getType() == tok_identifier) {
                                func.addArgument(tokens[i].getValue());
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
                    } else {
                        FPResult result;
                        result.message = "Expected: '(', but got '" + tokens[i].getValue() + "'";
                        result.column = tokens[i].getColumn();
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.success = false;
                        errors.push_back(result);
                    }
                    extern_functions.push_back(func);
                } else if (type.getType() == tok_declare) {
                    i++;
                    if (tokens[i].getType() != tok_identifier) {
                        FPResult result;
                        result.message = "Expected itentifier for variable declaration, but got '" + tokens[i].getValue() + "'";
                        result.column = tokens[i].getColumn();
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    extern_globals.push_back(tokens[i].getValue());
                } else {
                    FPResult result;
                    result.message = "Expected 'function' or 'decl', but got '" + tokens[i].getValue() + "'";
                    result.column = tokens[i].getColumn();
                    result.value = tokens[i].getValue();
                    result.line = tokens[i].getLine();
                    result.in = tokens[i].getFile();
                    result.success = false;
                    errors.push_back(result);
                }
                } else if (currentFunction != nullptr && currentContainer == nullptr && currentStruct == nullptr) {
                currentFunction->addToken(token);
            } else if (token.getType() == tok_declare && currentContainer == nullptr && currentStruct == nullptr) {
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
            } else if (token.getType() == tok_declare && currentStruct != nullptr) {
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
                currentStruct->addMember(tokens[i + 1].getValue());
                i++;
            }
        }

        TPResult result;
        result.functions = functions;
        result.extern_functions = extern_functions;
        result.extern_globals = extern_globals;
        result.globals = globals;
        result.containers = containers;
        result.structs = structs;
        result.errors = errors;
        return result;
    }
}
#endif
