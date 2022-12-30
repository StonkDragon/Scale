#include <iostream>
#include <string>
#include <vector>

#include "../headers/Common.hpp"

namespace sclc
{
    TPResult SyntaxTree::parse() {
        Function* currentFunction = nullptr;
        Container* currentContainer = nullptr;
        Struct* currentStruct = nullptr;
        Interface* currentInterface = nullptr;

        std::vector<std::string> uses;
        std::vector<std::string> nextAttributes;
        std::vector<Variable> globals;
        std::vector<Container> containers;
        std::vector<Struct> structs;
        std::vector<Interface*> interfaces;
        std::unordered_map<std::string, std::string> typealiases;

        std::vector<FPResult> errors;
        std::vector<FPResult> warns;

        for (size_t i = 0; i < tokens.size(); i++)
        {
            Token token = tokens[i];
            // std::cout << "Token: " << token.getValue() << ": " << token.getType() << std::endl;
            if (token.getType() == tok_function) {
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define function inside another function.";
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
                    result.message = "Cannot define function inside of a container.";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentStruct != nullptr) {
                    if (std::find(nextAttributes.begin(), nextAttributes.end(), "static") != nextAttributes.end() || currentStruct->isStatic()) {
                        std::string name = tokens[i + 1].getValue();
                        Token func = tokens[i + 1];
                        currentFunction = new Function(currentStruct->getName() + "$" + name, func);
                        currentFunction->setFile(func.getFile());
                        for (std::string m : nextAttributes) {
                            currentFunction->addModifier(m);
                        }
                        nextAttributes.clear();
                        i += 2;
                        if (tokens[i].getType() == tok_paren_open) {
                            i++;
                            while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                                if (tokens[i].getType() == tok_identifier) {
                                    std::string name = tokens[i].getValue();
                                    std::string type = "any";
                                    if (tokens[i+1].getType() == tok_column) {
                                        i += 2;
                                        FPResult r = parseType(tokens, &i);
                                        if (!r.success) {
                                            errors.push_back(r);
                                            continue;
                                        }
                                        type = r.value;
                                        if (type == "none") {
                                            FPResult result;
                                            result.message = "Type 'none' is only valid for function return types.";
                                            result.value = tokens[i].getValue();
                                            result.line = tokens[i].getLine();
                                            result.in = tokens[i].getFile();
                                            result.type = tokens[i].getType();
                                            result.success = false;
                                            errors.push_back(result);
                                            continue;
                                        }
                                    } else {
                                        FPResult result;
                                        result.message = "A type is required!";
                                        result.value = tokens[i].getValue();
                                        result.line = tokens[i].getLine();
                                        result.in = tokens[i].getFile();
                                        result.type = tokens[i].getType();
                                        result.success = false;
                                        errors.push_back(result);
                                        i++;
                                        continue;
                                    }
                                    currentFunction->addArgument(Variable(name, type));
                                } else {
                                    FPResult result;
                                    result.message = "Expected identifier for argument name, but got '" + tokens[i].getValue() + "'";
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
                                if (tokens[i].getType() == tok_comma || tokens[i].getType() == tok_paren_close) {
                                    if (tokens[i].getType() == tok_comma) {
                                        i++;
                                    }
                                    continue;
                                }
                                FPResult result;
                                result.message = "Expected ',' or ')', but got '" + tokens[i].getValue() + "'";
                                result.value = tokens[i].getValue();
                                result.line = tokens[i].getLine();
                                result.in = tokens[i].getFile();
                                result.type = tokens[i].getType();
                                result.success = false;
                                errors.push_back(result);
                                continue;
                            }
                            if (tokens[i+1].getType() == tok_column) {
                                i += 2;
                                FPResult r = parseType(tokens, &i);
                                if (!r.success) {
                                    errors.push_back(r);
                                    continue;
                                }
                                std::string type = r.value;
                                currentFunction->setReturnType(type);
                            } else {
                                FPResult result;
                                result.message = "A type is required!";
                                result.value = tokens[i].getValue();
                                result.line = tokens[i].getLine();
                                result.in = tokens[i].getFile();
                                result.type = tokens[i].getType();
                                result.success = false;
                                errors.push_back(result);
                                i++;
                                continue;
                            }
                        } else {
                            FPResult result;
                            result.message = "Expected '(', but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        continue;
                    }
                    Token func = tokens[i + 1];
                    std::string name = func.getValue();
                    currentFunction = new Method(currentStruct->getName(), name, func);
                    currentFunction->setFile(func.getFile());
                    static_cast<Method*>(currentFunction)->forceAdd(true);
                    for (std::string m : nextAttributes) {
                        currentFunction->addModifier(m);
                    }
                    nextAttributes.clear();
                    i += 2;
                    if (tokens[i].getType() == tok_paren_open) {
                        i++;
                        while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                            if (tokens[i].getType() == tok_identifier) {
                                std::string name = tokens[i].getValue();
                                std::string type = "any";
                                if (tokens[i+1].getType() == tok_column) {
                                    i += 2;
                                    FPResult r = parseType(tokens, &i);
                                    if (!r.success) {
                                        errors.push_back(r);
                                        continue;
                                    }
                                    type = r.value;
                                    if (type == "none") {
                                        FPResult result;
                                        result.message = "Type 'none' is only valid for function return types.";
                                        result.value = tokens[i].getValue();
                                        result.line = tokens[i].getLine();
                                        result.in = tokens[i].getFile();
                                        result.type = tokens[i].getType();
                                        result.success = false;
                                        errors.push_back(result);
                                        continue;
                                    }
                                } else {
                                    FPResult result;
                                    result.message = "A type is required!";
                                    result.value = tokens[i].getValue();
                                    result.line = tokens[i].getLine();
                                    result.in = tokens[i].getFile();
                                    result.type = tokens[i].getType();
                                    result.success = false;
                                    errors.push_back(result);
                                    i++;
                                    continue;
                                }
                                currentFunction->addArgument(Variable(name, type));
                            } else {
                                FPResult result;
                                result.message = "Expected identifier for method name, but got '" + tokens[i].getValue() + "'";
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
                            if (tokens[i].getType() == tok_comma || tokens[i].getType() == tok_paren_close) {
                                if (tokens[i].getType() == tok_comma) {
                                    i++;
                                }
                                continue;
                            }
                            FPResult result;
                            result.message = "Expected ',' or ')', but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        
                        currentFunction->addArgument(Variable("self", currentStruct->getName()));
                        if (tokens[i+1].getType() == tok_column) {
                            i += 2;
                            FPResult r = parseType(tokens, &i);
                            if (!r.success) {
                                errors.push_back(r);
                                continue;
                            }
                            std::string type = r.value;
                            currentFunction->setReturnType(type);
                            currentFunction->setReturnType(type);
                        } else {
                            FPResult result;
                            result.message = "A type is required!";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            i++;
                            continue;
                        }
                    } else {
                        FPResult result;
                        result.message = "Expected '(', but got '" + tokens[i].getValue() + "'";
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    continue;
                }
                if (currentInterface != nullptr) {
                    std::string name = tokens[i + 1].getValue();
                    Token func = tokens[i + 1];
                    Function* functionToImplement = new Function(name, func);
                    functionToImplement->setFile(func.getFile());
                    for (std::string m : nextAttributes) {
                        functionToImplement->addModifier(m);
                    }
                    nextAttributes.clear();
                    i += 2;
                    if (tokens[i].getType() == tok_paren_open) {
                        i++;
                        while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                            if (tokens[i].getType() == tok_identifier) {
                                std::string name = tokens[i].getValue();
                                std::string type = "any";
                                if (tokens[i+1].getType() == tok_column) {
                                    i += 2;
                                    FPResult r = parseType(tokens, &i);
                                    if (!r.success) {
                                        errors.push_back(r);
                                        continue;
                                    }
                                    type = r.value;
                                    if (type == "none") {
                                        FPResult result;
                                        result.message = "Type 'none' is only valid for function return types.";
                                        result.value = tokens[i].getValue();
                                        result.line = tokens[i].getLine();
                                        result.in = tokens[i].getFile();
                                        result.type = tokens[i].getType();
                                        result.success = false;
                                        errors.push_back(result);
                                        continue;
                                    }
                                } else {
                                    FPResult result;
                                    result.message = "A type is required!";
                                    result.value = tokens[i].getValue();
                                    result.line = tokens[i].getLine();
                                    result.in = tokens[i].getFile();
                                    result.type = tokens[i].getType();
                                    result.success = false;
                                    errors.push_back(result);
                                    i++;
                                    continue;
                                }
                                functionToImplement->addArgument(Variable(name, type));
                            } else {
                                FPResult result;
                                result.message = "Expected identifier for argument name, but got '" + tokens[i].getValue() + "'";
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
                            if (tokens[i].getType() == tok_comma || tokens[i].getType() == tok_paren_close) {
                                if (tokens[i].getType() == tok_comma) {
                                    i++;
                                }
                                continue;
                            }
                            FPResult result;
                            result.message = "Expected ',' or ')', but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        if (tokens[i+1].getType() == tok_column) {
                            i += 2;
                            FPResult r = parseType(tokens, &i);
                            if (!r.success) {
                                errors.push_back(r);
                                continue;
                            }
                            std::string type = r.value;
                            functionToImplement->setReturnType(type);
                        } else {
                            FPResult result;
                            result.message = "A type is required!";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            i++;
                            continue;
                        }
                    } else {
                        FPResult result;
                        result.message = "Expected '(', but got '" + tokens[i].getValue() + "'";
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    currentInterface->addToImplement(functionToImplement);
                    continue;
                }
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected itentifier for function name keyword";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (tokens[i + 2].getType() == tok_column) {
                    std::string member_type = tokens[i + 1].getValue();
                    i += 2;
                    Token func = tokens[i + 1];
                    std::string name = func.getValue();
                    currentFunction = new Method(member_type, name, func);
                    currentFunction->setFile(func.getFile());
                    for (std::string m : nextAttributes) {
                        currentFunction->addModifier(m);
                    }
                    nextAttributes.clear();
                    i += 2;
                    if (tokens[i].getType() == tok_paren_open) {
                        i++;
                        while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                            if (tokens[i].getType() == tok_identifier) {
                                std::string name = tokens[i].getValue();
                                std::string type = "any";
                                if (tokens[i+1].getType() == tok_column) {
                                    i += 2;
                                    FPResult r = parseType(tokens, &i);
                                    if (!r.success) {
                                        errors.push_back(r);
                                        continue;
                                    }
                                    type = r.value;
                                    if (type == "none") {
                                        FPResult result;
                                        result.message = "Type 'none' is only valid for function return types.";
                                        result.value = tokens[i].getValue();
                                        result.line = tokens[i].getLine();
                                        result.in = tokens[i].getFile();
                                        result.type = tokens[i].getType();
                                        result.success = false;
                                        errors.push_back(result);
                                        continue;
                                    }
                                } else {
                                    FPResult result;
                                    result.message = "A type is required!";
                                    result.value = tokens[i].getValue();
                                    result.line = tokens[i].getLine();
                                    result.in = tokens[i].getFile();
                                    result.type = tokens[i].getType();
                                    result.success = false;
                                    errors.push_back(result);
                                    i++;
                                    continue;
                                }
                                currentFunction->addArgument(Variable(name, type));
                            } else {
                                FPResult result;
                                result.message = "Expected identifier for argument name, but got '" + tokens[i].getValue() + "'";
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
                            if (tokens[i].getType() == tok_comma || tokens[i].getType() == tok_paren_close) {
                                if (tokens[i].getType() == tok_comma) {
                                    i++;
                                }
                                continue;
                            }
                            FPResult result;
                            result.message = "Expected ',' or ')', but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        
                        currentFunction->addArgument(Variable("self", member_type));
                        if (tokens[i+1].getType() == tok_column) {
                            i += 2;
                            FPResult r = parseType(tokens, &i);
                            if (!r.success) {
                                errors.push_back(r);
                                continue;
                            }
                            std::string type = r.value;
                            currentFunction->setReturnType(type);
                        } else {
                            FPResult result;
                            result.message = "A type is required!";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            i++;
                            continue;
                        }
                    } else {
                        FPResult result;
                        result.message = "Expected '(', but got '" + tokens[i].getValue() + "'";
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                } else {
                    std::string name = tokens[i + 1].getValue();
                    Token func = tokens[i + 1];
                    currentFunction = new Function(name, func);
                    currentFunction->setFile(func.getFile());
                    for (std::string m : nextAttributes) {
                        currentFunction->addModifier(m);
                    }
                    nextAttributes.clear();
                    i += 2;
                    if (tokens[i].getType() == tok_paren_open) {
                        i++;
                        while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                            if (tokens[i].getType() == tok_identifier) {
                                std::string name = tokens[i].getValue();
                                std::string type = "any";
                                if (tokens[i+1].getType() == tok_column) {
                                    i += 2;
                                    FPResult r = parseType(tokens, &i);
                                    if (!r.success) {
                                        errors.push_back(r);
                                        continue;
                                    }
                                    type = r.value;
                                    if (type == "none") {
                                        FPResult result;
                                        result.message = "Type 'none' is only valid for function return types.";
                                        result.value = tokens[i].getValue();
                                        result.line = tokens[i].getLine();
                                        result.in = tokens[i].getFile();
                                        result.type = tokens[i].getType();
                                        result.success = false;
                                        errors.push_back(result);
                                        continue;
                                    }
                                } else {
                                    FPResult result;
                                    result.message = "A type is required!";
                                    result.value = tokens[i].getValue();
                                    result.line = tokens[i].getLine();
                                    result.in = tokens[i].getFile();
                                    result.type = tokens[i].getType();
                                    result.success = false;
                                    errors.push_back(result);
                                    i++;
                                    continue;
                                }
                                currentFunction->addArgument(Variable(name, type));
                            } else {
                                FPResult result;
                                result.message = "Expected identifier for argument name, but got '" + tokens[i].getValue() + "'";
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
                            if (tokens[i].getType() == tok_comma || tokens[i].getType() == tok_paren_close) {
                                if (tokens[i].getType() == tok_comma) {
                                    i++;
                                }
                                continue;
                            }
                            FPResult result;
                            result.message = "Expected ',' or ')', but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        if (tokens[i+1].getType() == tok_column) {
                            i += 2;
                            FPResult r = parseType(tokens, &i);
                            if (!r.success) {
                                errors.push_back(r);
                                continue;
                            }
                            std::string type = r.value;
                            currentFunction->setReturnType(type);
                        } else {
                            FPResult result;
                            result.message = "A type is required!";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            i++;
                            continue;
                        }
                    } else {
                        FPResult result;
                        result.message = "Expected '(', but got '" + tokens[i].getValue() + "'";
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                }
            } else if (token.getType() == tok_end) {
                if (currentFunction != nullptr) {
                    bool containsB = false;
                    for (size_t i = 0; i < functions.size(); i++) {
                        if (*(functions.at(i)) == *currentFunction) {
                            containsB = true;
                            break;
                        }
                    }
                    if (!containsB) {
                        functions.push_back(currentFunction);
                    } else {
                        FPResult result;
                        result.message = "Function " + currentFunction->getName() + " already exists! Try renaming this function";
                        result.value = currentFunction->getNameToken().getValue();
                        result.line = currentFunction->getNameToken().getLine();
                        result.in = currentFunction->getNameToken().getFile();
                        result.type = currentFunction->getNameToken().getType();
                        result.success = false;
                        errors.push_back(result);
                    }
                    currentFunction = nullptr;
                } else if (currentContainer != nullptr) {
                    containers.push_back(*currentContainer);
                    currentContainer = nullptr;
                } else if (currentStruct != nullptr) {
                    structs.push_back(*currentStruct);
                    currentStruct = nullptr;
                } else if (currentInterface != nullptr) {
                    interfaces.push_back(currentInterface);
                    currentInterface = nullptr;
                } else {
                    FPResult result;
                    result.message = "Unexpected 'end' keyword outside of function, container, struct or interface body. Remove this:";
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
                    result.message = "Cannot define a container inside another container. Maybe you forgot an 'end' somewhere?";
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
                    result.message = "Cannot define a container inside of a function. Maybe you forgot an 'end' somewhere?";
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
                    result.message = "Cannot define a container inside of a struct. Maybe you forgot an 'end' somewhere?";
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentInterface != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a container inside of an interface. Maybe you forgot an 'end' somewhere?";
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
                    result.message = "Expected itentifier for container name, but got '" + tokens[i + 1].getValue() + "'";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                if (tokens[i].getValue() == "str" || tokens[i].getValue() == "int" || tokens[i].getValue() == "float" || tokens[i].getValue() == "none" || tokens[i].getValue() == "any") {
                    FPResult result;
                    result.message = "Invalid name for container: '" + tokens[i + 1].getValue() + "'";
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
                    result.message = "Cannot define a struct inside of a container. Maybe you forgot an 'end' somewhere?";
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
                    result.message = "Cannot define a struct inside of a function. Maybe you forgot an 'end' somewhere?";
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
                    result.message = "Cannot define a struct inside another struct. Maybe you forgot an 'end' somewhere?";
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentInterface != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a struct inside of an interface. Maybe you forgot an 'end' somewhere?";
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
                    result.message = "Expected itentifier for struct name, but got '" + tokens[i + 1].getValue() + "'";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                if (tokens[i].getValue() == "str" || tokens[i].getValue() == "int" || tokens[i].getValue() == "float" || tokens[i].getValue() == "none" || tokens[i].getValue() == "any") {
                    FPResult result;
                    result.message = "Invalid name for struct: '" + tokens[i + 1].getValue() + "'";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                currentStruct = new Struct(tokens[i].getValue(), tokens[i]);
                for (std::string m : nextAttributes) {
                    if (m == "sealed")
                        currentStruct->toggleSealed();
                    if (m == "valuetype")
                        currentStruct->toggleReferenceType();
                    if (m == "referencetype")
                        currentStruct->toggleValueType();
                    if (m == "nowarn")
                        currentStruct->toggleWarnings();
                    if (m == "static") {
                        currentStruct->toggleValueType();
                        currentStruct->toggleReferenceType();
                        currentStruct->toggleStatic();
                    }
                }
                if (!(currentStruct->heapAllocAllowed() || currentStruct->stackAllocAllowed()) && !currentStruct->isStatic()) {
                    FPResult result;
                    result.message = "Struct '" + tokens[i].getValue() + "' cannot be instanciated";
                    result.value = tokens[i].getValue();
                    result.line = tokens[i].getLine();
                    result.in = tokens[i].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                nextAttributes.clear();
                if (tokens[i + 1].getType() == tok_is) {
                    i++;
                    if (tokens[i + 1].getType() != tok_identifier) {
                        FPResult result;
                        result.message = "Expected identifier for interface name, but got'" + tokens[i + 1].getValue() + "'";
                        result.value = tokens[i + 1].getValue();
                        result.line = tokens[i + 1].getLine();
                        result.in = tokens[i + 1].getFile();
                        result.type = tokens[i].getType();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    i++;
                    while (tokens[i].getType() != tok_declare && tokens[i].getType() != tok_end && tokens[i].getType() != tok_function && tokens[i].getType() != tok_extern && tokens[i].getValue() != "static") {
                        if (tokens[i].getType() == tok_identifier)
                            currentStruct->implement(tokens[i].getValue());
                        i++;
                        if (tokens[i].getType() == tok_declare || tokens[i].getType() == tok_end || tokens[i].getType() == tok_function || tokens[i].getType() == tok_extern || tokens[i].getValue() == "static") {
                            i--;
                            break;
                        }
                        if (i >= tokens.size()) {
                            FPResult result;
                            result.message = "Unexpected end of file!";
                            result.value = tokens[i - 1].getValue();
                            result.line = tokens[i - 1].getLine();
                            result.in = tokens[i - 1].getFile();
                            result.type = tokens[i - 1].getType();
                            result.success = false;
                            errors.push_back(result);
                            break;
                        }
                    }
                }
            } else if (token.getType() == tok_interface_def) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an interface inside of a container. Maybe you forgot an 'end' somewhere?";
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
                    result.message = "Cannot define an interface inside of a function. Maybe you forgot an 'end' somewhere?";
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
                    result.message = "Cannot define an interface inside of a struct. Maybe you forgot an 'end' somewhere?";
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentInterface != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an interface inside another interface. Maybe you forgot an 'end' somewhere?";
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
                    result.message = "Expected itentifier for interface declaration, but got '" + tokens[i + 1].getValue() + "'";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                if (tokens[i].getValue() == "str" || tokens[i].getValue() == "int" || tokens[i].getValue() == "float" || tokens[i].getValue() == "none" || tokens[i].getValue() == "any") {
                    FPResult result;
                    result.message = "Invalid name for interface: '" + tokens[i + 1].getValue() + "'";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i].getType();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                currentInterface = new Interface(tokens[i].getValue());
            } else if (token.getType() == tok_addr_of) {
                if (currentFunction == nullptr) {
                    if (tokens[i + 1].getType() == tok_identifier) {
                        nextAttributes.push_back(tokens[i + 1].getValue());
                    } else {
                        FPResult result;
                        result.message = "" + tokens[i + 1].getValue() + " is not a valid modifier.";
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
                    currentFunction->addToken(token);
                }
            } else if (token.getType() == tok_extern && currentFunction == nullptr && currentContainer == nullptr && currentInterface == nullptr) {
                Token extToken = token;
                i++;
                Token type = tokens[i];
                if (type.getType() == tok_function) {
                    if (currentStruct != nullptr) {
                        Token func = tokens[i + 1];
                        std::string name = func.getValue();
                        Function* function = new Method(currentStruct->getName(), name, func);
                        function->setFile(func.getFile());
                        function->isExternC = (extToken.getValue() == "extern_c");
                        static_cast<Method*>(function)->forceAdd(true);
                        for (std::string m : nextAttributes) {
                            function->addModifier(m);
                        }
                        nextAttributes.clear();
                        i += 2;
                        if (tokens[i].getType() == tok_paren_open) {
                            i++;
                            while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                                if (tokens[i].getType() == tok_identifier) {
                                    std::string name = tokens[i].getValue();
                                    std::string type = "any";
                                    if (tokens[i+1].getType() == tok_column) {
                                        i += 2;
                                        FPResult r = parseType(tokens, &i);
                                        if (!r.success) {
                                            errors.push_back(r);
                                            continue;
                                        }
                                        type = r.value;
                                        if (type == "none") {
                                            FPResult result;
                                            result.message = "Type 'none' is only valid for function return types.";
                                            result.value = tokens[i].getValue();
                                            result.line = tokens[i].getLine();
                                            result.in = tokens[i].getFile();
                                            result.type = tokens[i].getType();
                                            result.success = false;
                                            errors.push_back(result);
                                            continue;
                                        }
                                    } else {
                                        FPResult result;
                                        result.message = "A type is required!";
                                        result.value = tokens[i].getValue();
                                        result.line = tokens[i].getLine();
                                        result.in = tokens[i].getFile();
                                        result.type = tokens[i].getType();
                                        result.success = false;
                                        errors.push_back(result);
                                        i++;
                                        continue;
                                    }
                                    function->addArgument(Variable(name, type));
                                } else {
                                    FPResult result;
                                    result.message = "Expected identifier for argument name, but got '" + tokens[i].getValue() + "'";
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
                                if (tokens[i].getType() == tok_comma || tokens[i].getType() == tok_paren_close) {
                                    if (tokens[i].getType() == tok_comma) {
                                        i++;
                                    }
                                    continue;
                                }
                                FPResult result;
                                result.message = "Expected ',' or ')', but got '" + tokens[i].getValue() + "'";
                                result.value = tokens[i].getValue();
                                result.line = tokens[i].getLine();
                                result.in = tokens[i].getFile();
                                result.type = tokens[i].getType();
                                result.success = false;
                                errors.push_back(result);
                                continue;
                            }
                            
                            function->addArgument(Variable("self", currentStruct->getName()));
                            if (tokens[i+1].getType() == tok_column) {
                                i += 2;
                                FPResult r = parseType(tokens, &i);
                                if (!r.success) {
                                    errors.push_back(r);
                                    continue;
                                }
                                std::string type = r.value;
                                function->setReturnType(type);
                            } else {
                                FPResult result;
                                result.message = "A type is required!";
                                result.value = tokens[i].getValue();
                                result.line = tokens[i].getLine();
                                result.in = tokens[i].getFile();
                                result.type = tokens[i].getType();
                                result.success = false;
                                errors.push_back(result);
                                i++;
                                continue;
                            }
                            extern_functions.push_back(function);
                        } else {
                            FPResult result;
                            result.message = "Expected '(', but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        continue;
                    }
                    if (tokens[i + 2].getType() == tok_column) {
                        std::string member_type = tokens[i + 1].getValue();
                        i += 2;
                        if (tokens[i].getType() != tok_column) {
                            FPResult result;
                            result.message = "Expected column, but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        Token func = tokens[i + 1];
                        std::string name = func.getValue();
                        Function* function = new Method(member_type, name, func);
                        function->setFile(func.getFile());
                        function->isExternC = (extToken.getValue() == "extern_c");
                        for (std::string m : nextAttributes) {
                            function->addModifier(m);
                        }
                        nextAttributes.clear();
                        i += 2;
                        if (tokens[i].getType() == tok_paren_open) {
                            i++;
                            while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                                if (tokens[i].getType() == tok_identifier) {
                                    std::string name = tokens[i].getValue();
                                    std::string type = "any";
                                    if (tokens[i+1].getType() == tok_column) {
                                        i += 2;
                                        FPResult r = parseType(tokens, &i);
                                        if (!r.success) {
                                            errors.push_back(r);
                                            continue;
                                        }
                                        type = r.value;
                                        if (type == "none") {
                                            FPResult result;
                                            result.message = "Type 'none' is only valid for function return types.";
                                            result.value = tokens[i].getValue();
                                            result.line = tokens[i].getLine();
                                            result.in = tokens[i].getFile();
                                            result.type = tokens[i].getType();
                                            result.success = false;
                                            errors.push_back(result);
                                            continue;
                                        }
                                    } else {
                                        FPResult result;
                                        result.message = "A type is required!";
                                        result.value = tokens[i].getValue();
                                        result.line = tokens[i].getLine();
                                        result.in = tokens[i].getFile();
                                        result.type = tokens[i].getType();
                                        result.success = false;
                                        errors.push_back(result);
                                        i++;
                                        continue;
                                    }
                                    function->addArgument(Variable(name, type));
                                } else {
                                    FPResult result;
                                    result.message = "Expected identifier for argument name, but got '" + tokens[i].getValue() + "'";
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
                                if (tokens[i].getType() == tok_comma || tokens[i].getType() == tok_paren_close) {
                                    if (tokens[i].getType() == tok_comma) {
                                        i++;
                                    }
                                    continue;
                                }
                                FPResult result;
                                result.message = "Expected ',' or ')', but got '" + tokens[i].getValue() + "'";
                                result.value = tokens[i].getValue();
                                result.line = tokens[i].getLine();
                                result.in = tokens[i].getFile();
                                result.type = tokens[i].getType();
                                result.success = false;
                                errors.push_back(result);
                                continue;
                            }
                            
                            function->addArgument(Variable("self", member_type));
                            if (tokens[i+1].getType() == tok_column) {
                                i += 2;
                                FPResult r = parseType(tokens, &i);
                                if (!r.success) {
                                    errors.push_back(r);
                                    continue;
                                }
                                std::string type = r.value;
                                function->setReturnType(type);
                            } else {
                                FPResult result;
                                result.message = "A type is required!";
                                result.value = tokens[i].getValue();
                                result.line = tokens[i].getLine();
                                result.in = tokens[i].getFile();
                                result.type = tokens[i].getType();
                                result.success = false;
                                errors.push_back(result);
                                i++;
                                continue;
                            }
                            extern_functions.push_back(function);
                        } else {
                            FPResult result;
                            result.message = "Expected '(', but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                    } else {
                        std::string name = tokens[i + 1].getValue();
                        Token funcTok = tokens[i + 1];
                        Function* func = new Function(name, funcTok);
                        func->setFile(funcTok.getFile());
                        func->isExternC = (extToken.getValue() == "extern_c");
                        i += 2;
                        if (tokens[i].getType() == tok_paren_open) {
                            i++;
                            while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                                if (tokens[i].getType() == tok_identifier) {
                                    std::string name = tokens[i].getValue();
                                    std::string type = "any";
                                    if (tokens[i+1].getType() == tok_column) {
                                        i += 2;
                                        FPResult r = parseType(tokens, &i);
                                        if (!r.success) {
                                            errors.push_back(r);
                                            continue;
                                        }
                                        type = r.value;
                                        if (type == "none") {
                                            FPResult result;
                                            result.message = "Type 'none' is only valid for function return types.";
                                            result.value = tokens[i].getValue();
                                            result.line = tokens[i].getLine();
                                            result.in = tokens[i].getFile();
                                            result.type = tokens[i].getType();
                                            result.success = false;
                                            errors.push_back(result);
                                            continue;
                                        }
                                    } else {
                                        FPResult result;
                                        result.message = "A type is required!";
                                        result.value = tokens[i].getValue();
                                        result.line = tokens[i].getLine();
                                        result.in = tokens[i].getFile();
                                        result.type = tokens[i].getType();
                                        result.success = false;
                                        errors.push_back(result);
                                        i++;
                                        continue;
                                    }
                                    func->addArgument(Variable(name, type));
                                } else {
                                    FPResult result;
                                    result.message = "Expected identifier for argument name, but got '" + tokens[i].getValue() + "'";
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
                                if (tokens[i].getType() == tok_comma || tokens[i].getType() == tok_paren_close) {
                                    if (tokens[i].getType() == tok_comma) {
                                        i++;
                                    }
                                    continue;
                                }
                                FPResult result;
                                result.message = "Expected ',' or ')', but got '" + tokens[i].getValue() + "'";
                                result.value = tokens[i].getValue();
                                result.line = tokens[i].getLine();
                                result.in = tokens[i].getFile();
                                result.type = tokens[i].getType();
                                result.success = false;
                                errors.push_back(result);
                                continue;
                            }
                            if (tokens[i+1].getType() == tok_column) {
                                i += 2;
                                FPResult r = parseType(tokens, &i);
                                if (!r.success) {
                                    errors.push_back(r);
                                    continue;
                                }
                                std::string type = r.value;
                                func->setReturnType(type);
                            } else {
                                FPResult result;
                                result.message = "A type is required!";
                                result.value = tokens[i].getValue();
                                result.line = tokens[i].getLine();
                                result.in = tokens[i].getFile();
                                result.type = tokens[i].getType();
                                result.success = false;
                                errors.push_back(result);
                                i++;
                                continue;
                            }
                        } else {
                            FPResult result;
                            result.message = "Expected '(', but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        extern_functions.push_back(func);
                    }
                } else if (type.getType() == tok_declare) {
                    i++;
                    if (tokens[i].getType() != tok_identifier) {
                        FPResult result;
                        result.message = "Expected itentifier for variable name, but got '" + tokens[i].getValue() + "'";
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    std::string name = tokens[i].getValue();
                    std::string type = "any";
                    if (tokens[i+1].getType() == tok_column) {
                        i += 2;
                        FPResult r = parseType(tokens, &i);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        if (type == "none") {
                            FPResult result;
                            result.message = "Type 'none' is only valid for function return types.";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                    }
                    extern_globals.push_back(Variable(name, type));
                } else {
                    FPResult result;
                    result.message = "Expected 'function' or 'decl', but got '" + tokens[i].getValue() + "'";
                    result.value = tokens[i].getValue();
                    result.line = tokens[i].getLine();
                    result.in = tokens[i].getFile();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
            } else if (currentFunction != nullptr && currentContainer == nullptr) {
                currentFunction->addToken(token);
            } else if (token.getType() == tok_declare && currentContainer == nullptr && currentStruct == nullptr) {
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected itentifier for variable name, but got '" + tokens[i + 1].getValue() + "'";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                std::string name = tokens[i].getValue();
                std::string type = "any";
                if (tokens[i+1].getType() == tok_column) {
                    i += 2;
                    FPResult r = parseType(tokens, &i);
                    if (!r.success) {
                        errors.push_back(r);
                        continue;
                    }
                    type = r.value;
                    if (type == "none") {
                        FPResult result;
                        result.message = "Type 'none' is only valid for function return types.";
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                }
                globals.push_back(Variable(name, type));
            } else if (token.getType() == tok_declare && currentContainer != nullptr) {
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected itentifier for variable name, but got '" + tokens[i + 1].getValue() + "'";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                std::string name = tokens[i].getValue();
                std::string type = "any";
                if (tokens[i+1].getType() == tok_column) {
                    i += 2;
                    FPResult r = parseType(tokens, &i);
                    if (!r.success) {
                        errors.push_back(r);
                        continue;
                    }
                    type = r.value;
                    if (type == "none") {
                        FPResult result;
                        result.message = "Type 'none' is only valid for function return types.";
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                }
                currentContainer->addMember(Variable(name, type));
            } else if (token.getType() == tok_declare && currentStruct != nullptr) {
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected itentifier for variable name, but got '" + tokens[i + 1].getValue() + "'";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                std::string name = tokens[i].getValue();
                std::string type = "any";
                if (tokens[i+1].getType() == tok_column) {
                    i += 2;
                    FPResult r = parseType(tokens, &i);
                    if (!r.success) {
                        errors.push_back(r);
                        continue;
                    }
                    type = r.value;
                    if (type == "none") {
                        FPResult result;
                        result.message = "Type 'none' is only valid for function return types.";
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                }
                if (currentStruct->isStatic() || std::find(nextAttributes.begin(), nextAttributes.end(), "static") != nextAttributes.end()) {
                    nextAttributes.clear();
                    globals.push_back(Variable(currentStruct->getName() + "$" + name, type));
                } else {
                    currentStruct->addMember(Variable(name, type));
                }
            } else {
                if (tokens[i].getType() == tok_identifier) {
                    if (tokens[i].getValue() == "typealias") {
                        i++;
                        std::string type = tokens[i].getValue();
                        i++;
                        if (tokens[i].getType() != tok_string_literal) {
                            FPResult result;
                            result.message = "Expected string, but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        std::string replacement = tokens[i].getValue();
                        typealiases[type] = replacement;
                        continue;
                    }
                    nextAttributes.push_back(tokens[i].getValue());
                }
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
        result.warns = warns;
        result.interfaces = interfaces;
        result.typealiases = typealiases;
        return result;
    }
}
