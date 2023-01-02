#include <iostream>
#include <string>
#include <vector>

#include "../headers/Common.hpp"

namespace sclc {
    Function* parseFunction(std::string name, Token nameToken, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<std::string>& nextAttributes, size_t& i, std::vector<Token>& tokens) {
        if (name.find(Main.options.operatorRandomData) == std::string::npos) {

            if namingConvention("Functions", nameToken, flatcase, snakeCase)
            else if namingConvention("Functions", nameToken, UPPERCASE, snakeCase)
            else if namingConvention("Functions", nameToken, PascalCase, snakeCase)
            else if namingConvention("Functions", nameToken, snake_case, snakeCase)
            else if namingConvention("Functions", nameToken, SCREAMING_SNAKE_CASE, snakeCase)
        }

        Function* func = new Function(name, nameToken);
        func->setFile(nameToken.getFile());
        for (std::string m : nextAttributes) {
            func->addModifier(m);
        }
        nextAttributes.clear();
        i += 2;
        if (tokens[i].getType() == tok_paren_open) {
            i++;
            while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                if (tokens[i].getType() == tok_identifier) {
                    std::string name = tokens[i].getValue();
                    if namingConvention("Variables", tokens[i], flatcase, snakeCase)
                    else if namingConvention("Variables", tokens[i], UPPERCASE, snakeCase)
                    else if namingConvention("Variables", tokens[i], PascalCase, snakeCase)
                    else if namingConvention("Variables", tokens[i], snake_case, snakeCase)
                    else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, snakeCase)
                    std::string type = "any";
                    i++;
                    bool isConst = false;
                    bool isMut = false;
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        while (tokens[i].getValue() == "const" || tokens[i].getValue() == "mut") {
                            if (tokens[i].getValue() == "const") {
                                isConst = true;
                            } else if (tokens[i].getValue() == "mut") {
                                isMut = true;
                            }
                            i++;
                        }
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
                            result.column = tokens[i].getColumn();
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
                        result.column = tokens[i].getColumn();
                        result.success = false;
                        errors.push_back(result);
                        i++;
                        continue;
                    }
                    func->addArgument(Variable(name, type, isConst, isMut));
                } else {
                    FPResult result;
                    result.message = "Expected identifier for argument name, but got '" + tokens[i].getValue() + "'";
                    result.value = tokens[i].getValue();
                    result.line = tokens[i].getLine();
                    result.in = tokens[i].getFile();
                    result.type = tokens[i].getType();
                    result.column = tokens[i].getColumn();
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
                result.column = tokens[i].getColumn();
                result.success = false;
                errors.push_back(result);
                continue;
            }
            i++;
            std::string namedReturn = "";
            if (tokens[i].getType() == tok_identifier) {
                namedReturn = tokens[i].getValue();
                if namingConvention("Variables", tokens[i], flatcase, snakeCase)
                else if namingConvention("Variables", tokens[i], UPPERCASE, snakeCase)
                else if namingConvention("Variables", tokens[i], PascalCase, snakeCase)
                else if namingConvention("Variables", tokens[i], snake_case, snakeCase)
                else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, snakeCase)
                i++;
            }
            if (tokens[i].getType() == tok_column) {
                i++;
                FPResult r = parseType(tokens, &i);
                if (!r.success) {
                    errors.push_back(r);
                    return nullptr;
                }
                std::string type = r.value;
                func->setReturnType(type);
                if (namedReturn.size()) {
                    func->setNamedReturnValue(Variable(namedReturn, type, false, true));
                }
            } else {
                FPResult result;
                result.message = "A type is required!";
                result.value = tokens[i].getValue();
                result.line = tokens[i].getLine();
                result.in = tokens[i].getFile();
                result.type = tokens[i].getType();
                result.column = tokens[i].getColumn();
                result.success = false;
                errors.push_back(result);
                i++;
                return nullptr;
            }
        } else {
            FPResult result;
            result.message = "Expected '(', but got '" + tokens[i].getValue() + "'";
            result.value = tokens[i].getValue();
            result.line = tokens[i].getLine();
            result.in = tokens[i].getFile();
            result.type = tokens[i].getType();
            result.column = tokens[i].getColumn();
            result.success = false;
            errors.push_back(result);
            return nullptr;
        }
        return func;
    }

    Method* parseMethod(std::string name, Token nameToken, std::string memberName, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<std::string>& nextAttributes, size_t& i, std::vector<Token>& tokens) {
        if (name == "+") name = "operator_" + Main.options.operatorRandomData + "_add";
        if (name == "-") name = "operator_" + Main.options.operatorRandomData + "_sub";
        if (name == "*") name = "operator_" + Main.options.operatorRandomData + "_mul";
        if (name == "/") name = "operator_" + Main.options.operatorRandomData + "_div";
        if (name == "%") name = "operator_" + Main.options.operatorRandomData + "_mod";
        if (name == "&") name = "operator_" + Main.options.operatorRandomData + "_logic_and";
        if (name == "|") name = "operator_" + Main.options.operatorRandomData + "_logic_or";
        if (name == "^") name = "operator_" + Main.options.operatorRandomData + "_logic_xor";
        if (name == "~") name = "operator_" + Main.options.operatorRandomData + "_logic_not";
        if (name == "<<") name = "operator_" + Main.options.operatorRandomData + "_logic_lsh";
        if (name == ">>") name = "operator_" + Main.options.operatorRandomData + "_logic_rsh";
        if (name == "**") name = "operator_" + Main.options.operatorRandomData + "_pow";
        if (name == ".") name = "operator_" + Main.options.operatorRandomData + "_dot";
        if (name == "<") name = "operator_" + Main.options.operatorRandomData + "_less";
        if (name == "<=") name = "operator_" + Main.options.operatorRandomData + "_less_equal";
        if (name == ">") name = "operator_" + Main.options.operatorRandomData + "_more";
        if (name == ">=") name = "operator_" + Main.options.operatorRandomData + "_more_equal";
        if (name == "==") name = "operator_" + Main.options.operatorRandomData + "_equal";
        if (name == "!") name = "operator_" + Main.options.operatorRandomData + "_not";
        if (name == "!!") name = "operator_" + Main.options.operatorRandomData + "_assert_not_nil";
        if (name == "!=") name = "operator_" + Main.options.operatorRandomData + "_not_equal";
        if (name == "&&") name = "operator_" + Main.options.operatorRandomData + "_bool_and";
        if (name == "||") name = "operator_" + Main.options.operatorRandomData + "_bool_or";
        if (name == "++") name = "operator_" + Main.options.operatorRandomData + "_inc";
        if (name == "--") name = "operator_" + Main.options.operatorRandomData + "_dec";
        if (name == "@") name = "operator_" + Main.options.operatorRandomData + "_at";
        if (name == "?") name = "operator_" + Main.options.operatorRandomData + "_wildcard";

        if (name.find(Main.options.operatorRandomData) == std::string::npos) {
            if namingConvention("Methods", nameToken, flatcase, snakeCase)
            else if namingConvention("Methods", nameToken, UPPERCASE, snakeCase)
            else if namingConvention("Methods", nameToken, PascalCase, snakeCase)
            else if namingConvention("Methods", nameToken, snake_case, snakeCase)
            else if namingConvention("Methods", nameToken, SCREAMING_SNAKE_CASE, snakeCase)
        }

        Method* method = new Method(memberName, name, nameToken);
        method->setFile(nameToken.getFile());
        method->forceAdd(true);
        for (std::string m : nextAttributes) {
            method->addModifier(m);
        }
        nextAttributes.clear();
        i += 2;
        if (tokens[i].getType() == tok_paren_open) {
            i++;
            while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                if (tokens[i].getType() == tok_identifier) {
                    std::string name = tokens[i].getValue();
                    if namingConvention("Variables", tokens[i], flatcase, snakeCase)
                    else if namingConvention("Variables", tokens[i], UPPERCASE, snakeCase)
                    else if namingConvention("Variables", tokens[i], PascalCase, snakeCase)
                    else if namingConvention("Variables", tokens[i], snake_case, snakeCase)
                    else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, snakeCase)
                    std::string type = "any";
                    bool isConst = false;
                    bool isMut = false;
                    i++;
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        while (tokens[i].getValue() == "const" || tokens[i].getValue() == "mut") {
                            if (tokens[i].getValue() == "const") {
                                isConst = true;
                            } else if (tokens[i].getValue() == "mut") {
                                isMut = true;
                            }
                            i++;
                        }
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
                            result.column = tokens[i].getColumn();
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
                        result.column = tokens[i].getColumn();
                        result.success = false;
                        errors.push_back(result);
                        i++;
                        continue;
                    }
                    method->addArgument(Variable(name, type, isConst, isMut));
                } else {
                    FPResult result;
                    result.message = "Expected identifier for method argument, but got '" + tokens[i].getValue() + "'";
                    result.value = tokens[i].getValue();
                    result.line = tokens[i].getLine();
                    result.in = tokens[i].getFile();
                    result.type = tokens[i].getType();
                    result.column = tokens[i].getColumn();
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
                result.column = tokens[i].getColumn();
                result.success = false;
                errors.push_back(result);
                continue;
            }
            i++;
            
            method->addArgument(Variable("self", memberName));

            std::string namedReturn = "";
            if (tokens[i].getType() == tok_identifier) {
                namedReturn = tokens[i].getValue();
                if namingConvention("Variables", tokens[i], flatcase, snakeCase)
                else if namingConvention("Variables", tokens[i], UPPERCASE, snakeCase)
                else if namingConvention("Variables", tokens[i], PascalCase, snakeCase)
                else if namingConvention("Variables", tokens[i], snake_case, snakeCase)
                else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, snakeCase)
                i++;
            }
            if (tokens[i].getType() == tok_column) {
                i++;
                FPResult r = parseType(tokens, &i);
                if (!r.success) {
                    errors.push_back(r);
                    return nullptr;
                }
                std::string type = r.value;
                method->setReturnType(type);
                if (namedReturn.size()) {
                    method->setNamedReturnValue(Variable(namedReturn, type, false, true));
                }
            } else {
                FPResult result;
                result.message = "A type is required!";
                result.value = tokens[i].getValue();
                result.line = tokens[i].getLine();
                result.in = tokens[i].getFile();
                result.type = tokens[i].getType();
                result.column = tokens[i].getColumn();
                result.success = false;
                errors.push_back(result);
                i++;
                return nullptr;
            }
        } else {
            FPResult result;
            result.message = "Expected '(', but got '" + tokens[i].getValue() + "'";
            result.value = tokens[i].getValue();
            result.line = tokens[i].getLine();
            result.in = tokens[i].getFile();
            result.type = tokens[i].getType();
            result.column = tokens[i].getColumn();
            result.success = false;
            errors.push_back(result);
            return nullptr;
        }
        return method;
    }

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
            if (token.getType() == tok_function) {
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define function inside another function.";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i + 1].getType();
                    result.column = tokens[i + 1].getColumn();
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
                    result.type = tokens[i + 1].getType();
                    result.column = tokens[i + 1].getColumn();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentStruct != nullptr) {
                    if (std::find(nextAttributes.begin(), nextAttributes.end(), "static") != nextAttributes.end() || currentStruct->isStatic()) {
                        std::string name = tokens[i + 1].getValue();
                        Token func = tokens[i + 1];
                        currentFunction = parseFunction(currentStruct->getName() + "$" + name, func, errors, warns, nextAttributes, i, tokens);
                        continue;
                    }
                    Token func = tokens[i + 1];
                    std::string name = func.getValue();
                    currentFunction = parseMethod(name, func, currentStruct->getName(), errors, warns, nextAttributes, i, tokens);
                    continue;
                }
                if (currentInterface != nullptr) {
                    std::string name = tokens[i + 1].getValue();
                    Token func = tokens[i + 1];
                    Function* functionToImplement = parseFunction(name, func, errors, warns, nextAttributes, i, tokens);
                    currentInterface->addToImplement(functionToImplement);
                    continue;
                }
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected itentifier for function name keyword";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i + 1].getType();
                    result.column = tokens[i + 1].getColumn();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                if (tokens[i + 1].getType() == tok_column) {
                    std::string member_type = tokens[i].getValue();
                    i++;
                    Token func = tokens[i + 1];
                    std::string name = func.getValue();
                    currentFunction = parseMethod(name, func, member_type, errors, warns, nextAttributes, i, tokens);
                } else {
                    i--;
                    std::string name = tokens[i + 1].getValue();
                    Token func = tokens[i + 1];
                    currentFunction = parseFunction(name, func, errors, warns, nextAttributes, i, tokens);
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
                        result.column = currentFunction->getNameToken().getColumn();
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
                    result.type = token.getType();
                    result.column = token.getColumn();
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
                    result.type = token.getType();
                    result.column = token.getColumn();
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
                    result.type = token.getType();
                    result.column = token.getColumn();
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
                    result.type = token.getType();
                    result.column = token.getColumn();
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
                    result.type = token.getType();
                    result.column = token.getColumn();
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
                    result.type = tokens[i + 1].getType();
                    result.column = tokens[i + 1].getColumn();
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
                    result.type = tokens[i + 1].getType();
                    result.column = tokens[i + 1].getColumn();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if namingConvention("Containers", tokens[i], flatcase, PascalCase)
                else if namingConvention("Containers", tokens[i], UPPERCASE, PascalCase)
                else if namingConvention("Containers", tokens[i], camelCase, PascalCase)
                else if namingConvention("Containers", tokens[i], snake_case, PascalCase)
                else if namingConvention("Containers", tokens[i], SCREAMING_SNAKE_CASE, PascalCase)
                currentContainer = new Container(tokens[i].getValue());
            } else if (token.getType() == tok_struct_def) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a struct inside of a container. Maybe you forgot an 'end' somewhere?";
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = token.getType();
                    result.column = token.getColumn();
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
                    result.type = token.getType();
                    result.column = token.getColumn();
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
                    result.type = token.getType();
                    result.column = token.getColumn();
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
                    result.type = token.getType();
                    result.column = token.getColumn();
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
                    result.type = tokens[i + 1].getType();
                    result.column = tokens[i + 1].getColumn();
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
                    result.type = tokens[i + 1].getType();
                    result.column = tokens[i + 1].getColumn();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if namingConvention("Structs", tokens[i], flatcase, PascalCase)
                else if namingConvention("Structs", tokens[i], UPPERCASE, PascalCase)
                else if namingConvention("Structs", tokens[i], camelCase, PascalCase)
                else if namingConvention("Structs", tokens[i], snake_case, PascalCase)
                else if namingConvention("Structs", tokens[i], SCREAMING_SNAKE_CASE, PascalCase)
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
                    result.column = tokens[i].getColumn();
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
                        result.type = tokens[i + 1].getType();
                        result.column = tokens[i + 1].getColumn();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    i++;
                    while (tokens[i].getType() != tok_declare && tokens[i].getType() != tok_end && tokens[i].getType() != tok_function && tokens[i].getType() != tok_extern && tokens[i].getValue() != "static" && tokens[i].getValue() != "const" && tokens[i].getValue() != "readonly") {
                        if (tokens[i].getType() == tok_identifier)
                            currentStruct->implement(tokens[i].getValue());
                        i++;
                        if (tokens[i].getType() == tok_declare || tokens[i].getType() == tok_end || tokens[i].getType() == tok_function || tokens[i].getType() == tok_extern || tokens[i].getValue() == "static" || tokens[i].getValue() == "const" || tokens[i].getValue() == "readonly") {
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
                            result.column = tokens[i - 1].getColumn();
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
                    result.type = token.getType();
                    result.column = token.getColumn();
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
                    result.column = tokens[i].getColumn();
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
                    result.column = tokens[i].getColumn();
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
                    result.column = tokens[i].getColumn();
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
                    result.column = tokens[i].getColumn();
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
                    result.column = tokens[i].getColumn();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if namingConvention("Interfaces", tokens[i], flatcase, PascalCase)
                else if namingConvention("Interfaces", tokens[i], UPPERCASE, PascalCase)
                else if namingConvention("Interfaces", tokens[i], camelCase, PascalCase)
                else if namingConvention("Interfaces", tokens[i], snake_case, PascalCase)
                else if namingConvention("Interfaces", tokens[i], SCREAMING_SNAKE_CASE, PascalCase)
                currentInterface = new Interface(tokens[i].getValue());
            } else if (token.getType() == tok_addr_of) {
                if (currentFunction == nullptr) {
                    if (tokens[i + 1].getType() == tok_identifier) {
                        nextAttributes.push_back(tokens[i + 1].getValue());
                    } else {
                        FPResult result;
                        result.message = "'" + tokens[i + 1].getValue() + "' is not a valid modifier.";
                        result.value = tokens[i + 1].getValue();
                        result.line = tokens[i + 1].getLine();
                        result.in = tokens[i + 1].getFile();
                        result.type = tokens[i + 1].getType();
                        result.column = tokens[i + 1].getColumn();
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
                        Function* function = parseMethod(name, func, currentStruct->getName(), errors, warns, nextAttributes, i, tokens);
                        function->isExternC = true;
                        extern_functions.push_back(function);
                        continue;
                    }
                    if (tokens[i + 2].getType() == tok_column) {
                        std::string member_type = tokens[i + 1].getValue();
                        i += 2;
                        Token func = tokens[i + 1];
                        std::string name = func.getValue();
                        Function* function = parseMethod(name, func, member_type, errors, warns, nextAttributes, i, tokens);
                        function->isExternC = true;
                        extern_functions.push_back(function);
                    } else {
                        std::string name = tokens[i + 1].getValue();
                        Token funcTok = tokens[i + 1];
                        Function* func = parseFunction(name, funcTok, errors, warns, nextAttributes, i, tokens);
                        func->isExternC = true;
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
                        result.type = tokens[i].getType();
                        result.column = tokens[i].getColumn();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    std::string name = tokens[i].getValue();
                    if namingConvention("Variables", tokens[i], flatcase, camelCase)
                    else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase)
                    else if namingConvention("Variables", tokens[i], PascalCase, camelCase)
                    else if namingConvention("Variables", tokens[i], snake_case, camelCase)
                    else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase)
                    std::string type = "any";
                    i++;
                    bool isConst = false;
                    bool isMut = false;
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        while (tokens[i].getValue() == "const" || tokens[i].getValue() == "mut" || tokens[i].getValue() == "readonly") {
                            if (tokens[i].getValue() == "const") {
                                isConst = true;
                            } else if (tokens[i].getValue() == "mut") {
                                isMut = true;
                            }
                            i++;
                        }
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
                            result.column = tokens[i].getColumn();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                    }
                    extern_globals.push_back(Variable(name, type, isMut, isConst));
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
                if namingConvention("Variables", tokens[i], flatcase, camelCase)
                else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase)
                else if namingConvention("Variables", tokens[i], PascalCase, camelCase)
                else if namingConvention("Variables", tokens[i], snake_case, camelCase)
                else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase)
                Token tok = tokens[i];
                std::string type = "any";
                i++;
                bool isConst = false;
                bool isMut = false;
                if (tokens[i].getType() == tok_column) {
                    i++;
                    while (tokens[i].getValue() == "const" || tokens[i].getValue() == "mut" || tokens[i].getValue() == "readonly") {
                        if (tokens[i].getValue() == "const") {
                            isConst = true;
                        } else if (tokens[i].getValue() == "mut") {
                            isMut = true;
                        }
                        i++;
                    }
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
                        result.column = tokens[i].getColumn();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                }
                nextAttributes.clear();
                globals.push_back(Variable(name, type, isConst, isMut));
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
                if namingConvention("Variables", tokens[i], flatcase, camelCase)
                else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase)
                else if namingConvention("Variables", tokens[i], PascalCase, camelCase)
                else if namingConvention("Variables", tokens[i], snake_case, camelCase)
                else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase)
                Token tok = tokens[i];
                std::string type = "any";
                i++;
                bool isConst = false;
                bool isMut = false;
                if (tokens[i].getType() == tok_column) {
                    i++;
                    while (tokens[i].getValue() == "const" || tokens[i].getValue() == "mut" || tokens[i].getValue() == "readonly") {
                        if (tokens[i].getValue() == "const") {
                            isConst = true;
                        } else if (tokens[i].getValue() == "mut") {
                            isMut = true;
                        }
                        i++;
                    }
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
                        result.column = tokens[i].getColumn();
                        result.column = tokens[i].getColumn();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                }
                nextAttributes.clear();
                currentContainer->addMember(Variable(name, type, isConst, isMut));
            } else if (token.getType() == tok_declare && currentStruct != nullptr) {
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected itentifier for variable name, but got '" + tokens[i + 1].getValue() + "'";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.column = tokens[i + 1].getColumn();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                Token nameToken = tokens[i];
                std::string name = tokens[i].getValue();
                if namingConvention("Variables", tokens[i], flatcase, camelCase)
                else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase)
                else if namingConvention("Variables", tokens[i], PascalCase, camelCase)
                else if namingConvention("Variables", tokens[i], snake_case, camelCase)
                else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase)
                std::string type = "any";
                i++;
                bool isConst = false;
                bool isMut = false;
                bool isInternalMut = false;
                if (tokens[i].getType() == tok_column) {
                    i++;
                    while (tokens[i].getValue() == "const" || tokens[i].getValue() == "mut" || tokens[i].getValue() == "readonly") {
                        if (tokens[i].getValue() == "const") {
                            isConst = true;
                        } else if (tokens[i].getValue() == "mut") {
                            isMut = true;
                        } else if (tokens[i].getValue() == "readonly") {
                            isInternalMut = true;
                        }
                        i++;
                    }
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
                        result.column = tokens[i].getColumn();
                        result.column = tokens[i].getColumn();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                }
                if (currentStruct->isStatic() || std::find(nextAttributes.begin(), nextAttributes.end(), "static") != nextAttributes.end()) {
                    nextAttributes.clear();
                    globals.push_back(Variable(currentStruct->getName() + "$" + name, type, isConst, isMut));
                } else {
                    nextAttributes.clear();
                    if (isConst && isInternalMut) {
                        FPResult result;
                        result.message = "The 'const' and 'readonly' modifiers are mutually exclusive!";
                        result.value = nameToken.getValue();
                        result.line = nameToken.getLine();
                        result.in = nameToken.getFile();
                        result.column = nameToken.getColumn();
                        result.type = nameToken.getType();
                        result.column = nameToken.getColumn();
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    if (isConst) {
                        currentStruct->addMember(Variable(name, type, isConst, isMut));
                    } else {
                        if (isInternalMut) {
                            currentStruct->addMember(Variable(name, type, isConst, isMut, currentStruct->getName()));
                        } else {
                            currentStruct->addMember(Variable(name, type, isConst, isMut));
                        }
                    }
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
                            result.column = tokens[i].getColumn();
                            result.column = tokens[i].getColumn();
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
