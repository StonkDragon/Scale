#include <iostream>
#include <string>
#include <vector>

#include "../headers/Common.hpp"

namespace sclc {
    Function* parseFunction(std::string name, Token nameToken, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<std::string>& nextAttributes, size_t& i, std::vector<Token>& tokens) {
        if (name.find(Main.options.operatorRandomData) == std::string::npos) {

            if namingConvention("Functions", nameToken, flatcase, camelCase, false)
            else if namingConvention("Functions", nameToken, UPPERCASE, camelCase, false)
            else if namingConvention("Functions", nameToken, PascalCase, camelCase, false)
            else if namingConvention("Functions", nameToken, snake_case, camelCase, false)
            else if namingConvention("Functions", nameToken, SCREAMING_SNAKE_CASE, camelCase, false)
            else if namingConvention("Functions", nameToken, IPascalCase, camelCase, false)
        }

        Function* func = new Function(name, nameToken);
        func->setFile(nameToken.getFile());
        for (std::string m : nextAttributes) {
            func->addModifier(m);
        }
        i += 2;
        if (tokens[i].getType() == tok_paren_open) {
            i++;
            while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                if (tokens[i].getType() == tok_identifier) {
                    std::string name = tokens[i].getValue();
                    if namingConvention("Variables", tokens[i], flatcase, camelCase, false)
                    else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase, false)
                    else if namingConvention("Variables", tokens[i], PascalCase, camelCase, false)
                    else if namingConvention("Variables", tokens[i], snake_case, camelCase, false)
                    else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase, false)
                    else if namingConvention("Variables", tokens[i], IPascalCase, camelCase, false)
                    std::string type = "any";
                    i++;
                    bool isConst = false;
                    bool isMut = false;
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        FPResult r = parseType(tokens, &i);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        isConst = typeIsConst(type);
                        isMut = typeIsMut(type);
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
                    Variable v = Variable(name, type, isConst, isMut);
                    v.canBeNil = typeCanBeNil(v.getType());
                    func->addArgument(v);
                } else if (tokens[i].getType() == tok_curly_open) {
                    std::vector<std::string> multi;
                    i++;
                    while (tokens[i].getType() != tok_curly_close) {
                        if (tokens[i].getType() == tok_comma) {
                            i++;
                        }
                        if (tokens[i].getType() != tok_identifier) {
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
                        std::string name = tokens[i].getValue();
                        if namingConvention("Variables", tokens[i], flatcase, camelCase, false)
                        else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase, false)
                        else if namingConvention("Variables", tokens[i], PascalCase, camelCase, false)
                        else if namingConvention("Variables", tokens[i], snake_case, camelCase, false)
                        else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase, false)
                        else if namingConvention("Variables", tokens[i], IPascalCase, camelCase, false)
                        multi.push_back(name);
                        i++;
                    }
                    i++;
                    std::string type;
                    bool isConst = false;
                    bool isMut = false;
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        FPResult r = parseType(tokens, &i);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        isConst = typeIsConst(type);
                        isMut = typeIsMut(type);
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
                    for (std::string s : multi) {
                        Variable v = Variable(s, type, isConst, isMut);
                        v.canBeNil = typeCanBeNil(v.getType());
                        func->addArgument(v);
                    }
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
                if namingConvention("Variables", tokens[i], flatcase, camelCase, false)
                else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase, false)
                else if namingConvention("Variables", tokens[i], PascalCase, camelCase, false)
                else if namingConvention("Variables", tokens[i], snake_case, camelCase, false)
                else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase, false)
                else if namingConvention("Variables", tokens[i], IPascalCase, camelCase, false)
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
                    Variable v = Variable(namedReturn, type, false, true);
                    v.canBeNil = typeCanBeNil(v.getType());
                    func->setNamedReturnValue(v);
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
            if namingConvention("Methods", nameToken, flatcase, camelCase, false)
            else if namingConvention("Methods", nameToken, UPPERCASE, camelCase, false)
            else if namingConvention("Methods", nameToken, PascalCase, camelCase, false)
            else if namingConvention("Methods", nameToken, snake_case, camelCase, false)
            else if namingConvention("Methods", nameToken, SCREAMING_SNAKE_CASE, camelCase, false)
            else if namingConvention("Methods", nameToken, IPascalCase, camelCase, false)
        }

        Method* method = new Method(memberName, name, nameToken);
        method->setFile(nameToken.getFile());
        method->forceAdd(true);
        for (std::string m : nextAttributes) {
            method->addModifier(m);
        }
        i += 2;
        if (tokens[i].getType() == tok_paren_open) {
            i++;
            while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                if (tokens[i].getType() == tok_identifier) {
                    std::string name = tokens[i].getValue();
                    if namingConvention("Variables", tokens[i], flatcase, camelCase, false)
                    else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase, false)
                    else if namingConvention("Variables", tokens[i], PascalCase, camelCase, false)
                    else if namingConvention("Variables", tokens[i], snake_case, camelCase, false)
                    else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase, false)
                    else if namingConvention("Variables", tokens[i], IPascalCase, camelCase, false)
                    std::string type = "any";
                    bool isConst = false;
                    bool isMut = false;
                    i++;
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        FPResult r = parseType(tokens, &i);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        isConst = typeIsConst(type);
                        isMut = typeIsMut(type);
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
                    Variable v = Variable(name, type, isConst, isMut);
                    v.canBeNil = typeCanBeNil(v.getType());
                    method->addArgument(v);
                } else if (tokens[i].getType() == tok_curly_open) {
                    std::vector<std::string> multi;
                    i++;
                    while (tokens[i].getType() != tok_curly_close) {
                        if (tokens[i].getType() == tok_comma) {
                            i++;
                        }
                        if (tokens[i].getType() != tok_identifier) {
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
                        std::string name = tokens[i].getValue();
                        if namingConvention("Variables", tokens[i], flatcase, camelCase, false)
                        else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase, false)
                        else if namingConvention("Variables", tokens[i], PascalCase, camelCase, false)
                        else if namingConvention("Variables", tokens[i], snake_case, camelCase, false)
                        else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase, false)
                        else if namingConvention("Variables", tokens[i], IPascalCase, camelCase, false)
                        multi.push_back(name);
                        i++;
                    }
                    i++;
                    std::string type;
                    bool isConst = false;
                    bool isMut = false;
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        FPResult r = parseType(tokens, &i);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        isConst = typeIsConst(type);
                        isMut = typeIsMut(type);
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
                    for (std::string s : multi) {
                        Variable v = Variable(s, type, isConst, isMut);
                        v.canBeNil = typeCanBeNil(v.getType());
                        method->addArgument(v);
                    }
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
            Variable self = Variable("self", "mut " + memberName);
            self.canBeNil = false;
            method->addArgument(self);

            std::string namedReturn = "";
            if (tokens[i].getType() == tok_identifier) {
                namedReturn = tokens[i].getValue();
                if namingConvention("Variables", tokens[i], flatcase, camelCase, false)
                else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase, false)
                else if namingConvention("Variables", tokens[i], PascalCase, camelCase, false)
                else if namingConvention("Variables", tokens[i], snake_case, camelCase, false)
                else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase, false)
                else if namingConvention("Variables", tokens[i], IPascalCase, camelCase, false)
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
                bool canReturnNil = typeCanBeNil(type);
                method->setReturnType(type);
                if (namedReturn.size()) {
                    Variable v = Variable(namedReturn, type, false, true);
                    v.canBeNil = canReturnNil;
                    method->setNamedReturnValue(v);
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

        bool isInLambda = false;

        std::vector<std::string> uses;
        std::vector<std::string> nextAttributes;
        std::vector<Variable> globals;
        std::vector<Container> containers;
        std::vector<Struct> structs;
        std::vector<Interface*> interfaces;
        std::vector<Enum> enums;
        std::unordered_map<std::string, std::string> typealiases;

        std::vector<FPResult> errors;
        std::vector<FPResult> warns;

        // Builtins
        {
            Function* builtinIsInstanceOf = new Function("builtinIsInstanceOf", Token(tok_identifier, "builtinIsInstanceOf", 0, "<builtin>"));
            builtinIsInstanceOf->addModifier("export");

            builtinIsInstanceOf->addArgument(Variable("obj", "any"));
            builtinIsInstanceOf->addArgument(Variable("typeStr", "str"));

            builtinIsInstanceOf->setReturnType("int");
            
            // if typeStr "any" == then true return
            builtinIsInstanceOf->addToken(Token(tok_if, "if", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "typeStr", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_string_literal, "any", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "==", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_then, "then", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_true, "true", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_return, "return", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_fi, "fi", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "obj", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "typeStr", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_column, ":", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "view", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "builtinHash", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "builtinTypeEquals", 0, "<builtin>"));
            builtinIsInstanceOf->addToken(Token(tok_return, "return", 0, "<builtin>"));
            functions.push_back(builtinIsInstanceOf);

            Function* builtinHash = new Function("builtinHash", Token(tok_identifier, "builtinHash", 0, "<builtin>"));
            builtinHash->isExternC = true;
            builtinHash->addModifier("extern");
            builtinHash->addModifier("cdecl");
            builtinHash->addModifier("hash1");
            
            builtinHash->addArgument(Variable("data", "[int8]"));
            
            builtinHash->setReturnType("int32");
            extern_functions.push_back(builtinHash);

            Function* builtinTypeEquals = new Function("builtinTypeEquals", Token(tok_identifier, "builtinTypeEquals", 0, "<builtin>"));
            builtinTypeEquals->isExternC = true;
            builtinTypeEquals->addModifier("extern");
            builtinTypeEquals->addModifier("cdecl");
            builtinTypeEquals->addModifier("_scl_struct_is_type");

            builtinTypeEquals->addArgument(Variable("obj", "any"));
            builtinTypeEquals->addArgument(Variable("typeId", "int32"));

            builtinTypeEquals->setReturnType("int");
            extern_functions.push_back(builtinTypeEquals);
        }
        
        for (size_t i = 0; i < tokens.size(); i++) {
            Token token = tokens[i];

            // std::cout << token.tostring() << std::endl;

            if (token.getType() == tok_function) {
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define function inside another function. Current function: " + currentFunction->getName();
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
                    result.message = "Cannot define function inside of a container. Current container: " + currentContainer->getName();
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
                        currentFunction->isPrivate = std::find(nextAttributes.begin(), nextAttributes.end(), "private") != nextAttributes.end();
                        for (std::string s : nextAttributes) {
                            currentFunction->addModifier(s);
                        }
                        nextAttributes.clear();
                        continue;
                    }
                    Token func = tokens[i + 1];
                    std::string name = func.getValue();
                    currentFunction = parseMethod(name, func, currentStruct->getName(), errors, warns, nextAttributes, i, tokens);
                    currentFunction->isPrivate = std::find(nextAttributes.begin(), nextAttributes.end(), "private") != nextAttributes.end();
                    for (std::string s : nextAttributes) {
                        currentFunction->addModifier(s);
                    }
                    nextAttributes.clear();
                    continue;
                }
                if (currentInterface != nullptr) {
                    if (contains<std::string>(nextAttributes, "default")) {
                        Token func = tokens[i + 1];
                        std::string name = func.getValue();
                        currentFunction = parseMethod(name, func, "", errors, warns, nextAttributes, i, tokens);
                        for (std::string s : nextAttributes) {
                            currentFunction->addModifier(s);
                        }
                        if (!(
                            featureEnabled("default-interface-implementation") ||
                            featureEnabled("default-interface-impl")
                        )) {
                            FPResult warn;
                            warn.message = "Default implementations are locked behind the 'default-interface-impl' feature flag. Treating as standard interface method declaration";
                            warn.value = currentFunction->getNameToken().getValue();
                            warn.line = currentFunction->getNameToken().getLine();
                            warn.in = currentFunction->getNameToken().getFile();
                            warn.type = currentFunction->getNameToken().getType();
                            warn.column = currentFunction->getNameToken().getColumn();
                            warn.success = true;
                            warns.push_back(warn);
                            currentInterface->addToImplement(currentFunction);
                        }
                    } else {
                        std::string name = tokens[i + 1].getValue();
                        Token func = tokens[i + 1];
                        Function* functionToImplement = parseFunction(name, func, errors, warns, nextAttributes, i, tokens);
                        currentInterface->addToImplement(functionToImplement);
                    }
                    nextAttributes.clear();
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
                    nextAttributes.clear();
                    continue;
                }
                i++;
                if (tokens[i + 1].getType() == tok_column) {
                    std::string member_type = tokens[i].getValue();
                    i++;
                    Token func = tokens[i + 1];
                    std::string name = func.getValue();
                    currentFunction = parseMethod(name, func, member_type, errors, warns, nextAttributes, i, tokens);
                    if (std::find(nextAttributes.begin(), nextAttributes.end(), "private") != nextAttributes.end()) {
                        FPResult result;
                        result.message = "Methods cannot be declared 'private', if they are not in the struct body!";
                        result.value = tokens[i + 1].getValue();
                        result.line = tokens[i + 1].getLine();
                        result.in = tokens[i + 1].getFile();
                        result.type = tokens[i + 1].getType();
                        result.column = tokens[i + 1].getColumn();
                        result.success = false;
                        errors.push_back(result);
                        nextAttributes.clear();
                        continue;
                    }
                } else {
                    i--;
                    std::string name = tokens[i + 1].getValue();
                    Token func = tokens[i + 1];
                    currentFunction = parseFunction(name, func, errors, warns, nextAttributes, i, tokens);
                }
                for (std::string s : nextAttributes) {
                    currentFunction->addModifier(s);
                }
                nextAttributes.clear();

            } else if (token.getType() == tok_end) {
                if (currentFunction != nullptr) {
                    if (isInLambda) {
                        isInLambda = false;
                        currentFunction->addToken(token);
                        continue;
                    }
                    bool containsB = false;
                    for (size_t i = 0; i < functions.size(); i++) {
                        if (*(functions.at(i)) == *currentFunction) {
                            containsB = true;
                            break;
                        }
                    }

                    if (currentInterface != nullptr) {
                        if (currentFunction->isMethod) {
                            currentInterface->addDefaultImplementation(static_cast<Method*>(currentFunction));
                        } else {
                            FPResult result;
                            result.message = "Expected a method, but got a function. If you see this error, something has gone very wrong! Report this: ERR_INTERFACE_DEFAULT_NO_METHOD";
                            result.value = currentFunction->getNameToken().getValue();
                            result.line = currentFunction->getNameToken().getLine();
                            result.in = currentFunction->getNameToken().getFile();
                            result.type = currentFunction->getNameToken().getType();
                            result.column = currentFunction->getNameToken().getColumn();
                            result.success = false;
                            errors.push_back(result);
                        }
                    } else if (currentFunction->isMethod || !containsB) {
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
                    result.message = "Cannot define a container inside another container. Maybe you forgot an 'end' somewhere? Current container: " + currentContainer->getName();
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
                    result.message = "Cannot define a container inside of a function. Maybe you forgot an 'end' somewhere? Current function: " + currentFunction->getName();
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
                    result.message = "Cannot define a container inside of a struct. Maybe you forgot an 'end' somewhere? Current struct: " + currentStruct->getName();
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
                    result.message = "Cannot define a container inside of an interface. Maybe you forgot an 'end' somewhere? Current interface: " + currentInterface->getName();
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
                if namingConvention("Containers", tokens[i], flatcase, PascalCase, true)
                else if namingConvention("Containers", tokens[i], UPPERCASE, PascalCase, true)
                else if namingConvention("Containers", tokens[i], camelCase, PascalCase, true)
                else if namingConvention("Containers", tokens[i], snake_case, PascalCase, true)
                else if namingConvention("Containers", tokens[i], SCREAMING_SNAKE_CASE, PascalCase, true)
                else if namingConvention("Containers", tokens[i], IPascalCase, PascalCase, true)
                currentContainer = new Container(tokens[i].getValue());
            } else if (token.getType() == tok_struct_def) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a struct inside of a container. Maybe you forgot an 'end' somewhere? Current container: " + currentContainer->getName();
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
                    result.message = "Cannot define a struct inside of a function. Maybe you forgot an 'end' somewhere? Current function: " + currentFunction->getName();
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
                    result.message = "Cannot define a struct inside another struct. Maybe you forgot an 'end' somewhere? Current struct: " + currentStruct->getName();
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
                    result.message = "Cannot define a struct inside of an interface. Maybe you forgot an 'end' somewhere? Current interface: " + currentInterface->getName();
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
                if (tokens[i].getValue() == "int" || tokens[i].getValue() == "float" || tokens[i].getValue() == "none" || tokens[i].getValue() == "any") {
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
                if (tokens[i].getValue() != "str") {
                    if namingConvention("Structs", tokens[i], flatcase, PascalCase, true)
                    else if namingConvention("Structs", tokens[i], UPPERCASE, PascalCase, true)
                    else if namingConvention("Structs", tokens[i], camelCase, PascalCase, true)
                    else if namingConvention("Structs", tokens[i], snake_case, PascalCase, true)
                    else if namingConvention("Structs", tokens[i], SCREAMING_SNAKE_CASE, PascalCase, true)
                    else if namingConvention("Structs", tokens[i], IPascalCase, PascalCase, true)
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
                    result.column = tokens[i].getColumn();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                nextAttributes.clear();
                bool hasSuperSpecified = false;
                if (tokens[i + 1].getType() == tok_column) {
                    i += 2;
                    if (currentStruct->getName() == tokens[i].getValue()) {
                        FPResult result;
                        result.message = "Structs cannot extend themselves. Defaulted to 'SclObject'!";
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.column = tokens[i].getColumn();
                        result.success = false;
                        errors.push_back(result);
                    } else {
                        if (tokens[i].getValue() == "SclObject") {
                            FPResult result;
                            result.message = "Explicit inherit of struct 'SclObject' not neccessary.";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.column = tokens[i].getColumn();
                            result.success = true;
                            warns.push_back(result);
                        }
                        currentStruct->setExtends(tokens[i].getValue());
                        hasSuperSpecified = true;
                    }
                }
                if (!hasSuperSpecified) {
                    if (currentStruct->getName() != "SclObject")
                        currentStruct->setExtends("SclObject");
                }
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
                    while (tokens[i].getType() != tok_declare && tokens[i].getType() != tok_end && tokens[i].getType() != tok_function && tokens[i].getType() != tok_extern && tokens[i].getValue() != "static" && tokens[i].getValue() != "private") {
                        if (tokens[i].getType() == tok_identifier)
                            currentStruct->implement(tokens[i].getValue());
                        i++;
                        if (tokens[i].getType() == tok_declare || tokens[i].getType() == tok_end || tokens[i].getType() == tok_function || tokens[i].getType() == tok_extern || tokens[i].getValue() == "static" || tokens[i].getValue() == "private") {
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
            } else if (token.getType() == tok_enum) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an enum inside of a container. Maybe you forgot an 'end' somewhere? Current container: " + currentContainer->getName();
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
                    result.message = "Cannot define an enum inside of a function. Maybe you forgot an 'end' somewhere? Current function: " + currentFunction->getName();
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
                    result.message = "Cannot define an enum inside of a struct. Maybe you forgot an 'end' somewhere? Current struct: " + currentStruct->getName();
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
                    result.message = "Cannot define an enum inside of an interface. Maybe you forgot an 'end' somewhere? Current interface: " + currentInterface->getName();
                    result.value = token.getValue();
                    result.line = token.getLine();
                    result.in = token.getFile();
                    result.type = token.getType();
                    result.column = token.getColumn();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                if (tokens[i].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected itentifier for enum name, but got '" + tokens[i].getValue() + "'";
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
                i++;
                if namingConvention("Enums", tokens[i], flatcase, PascalCase, false)
                else if namingConvention("Enums", tokens[i], UPPERCASE, PascalCase, false)
                else if namingConvention("Enums", tokens[i], camelCase, PascalCase, false)
                else if namingConvention("Enums", tokens[i], snake_case, PascalCase, false)
                else if namingConvention("Enums", tokens[i], SCREAMING_SNAKE_CASE, PascalCase, false)
                else if namingConvention("Enums", tokens[i], IPascalCase, PascalCase, false)
                Enum e = Enum(name);
                while (tokens[i].getType() != tok_end) {
                    if namingConvention("Enum members", tokens[i], flatcase, camelCase, false)
                    else if namingConvention("Enum members", tokens[i], UPPERCASE, camelCase, false)
                    else if namingConvention("Enum members", tokens[i], PascalCase, camelCase, false)
                    else if namingConvention("Enum members", tokens[i], snake_case, camelCase, false)
                    else if namingConvention("Enum members", tokens[i], SCREAMING_SNAKE_CASE, camelCase, false)
                    else if namingConvention("Enum members", tokens[i], IPascalCase, camelCase, false)

                    if (tokens[i].getType() != tok_identifier) {
                        FPResult result;
                        result.message = "Expected itentifier for enum member, but got '" + tokens[i].getValue() + "'";
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
                    if (e.indexOf(tokens[i].getValue()) != Enum::npos) {
                        FPResult result;
                        result.message = "Duplicate member of enum '" + e.getName() + "': '" + tokens[i].getValue() + "'";
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
                    e.addMember(tokens[i].getValue());
                    i++;
                }
                enums.push_back(e);
            } else if (token.getType() == tok_interface_def) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an interface inside of a container. Maybe you forgot an 'end' somewhere? Current container: " + currentContainer->getName();
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
                    result.message = "Cannot define an interface inside of a function. Maybe you forgot an 'end' somewhere? Current function: " + currentFunction->getName();
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
                    result.message = "Cannot define an interface inside of a struct. Maybe you forgot an 'end' somewhere? Current struct: " + currentStruct->getName();
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
                    result.message = "Cannot define an interface inside another interface. Maybe you forgot an 'end' somewhere? Current interface: " + currentInterface->getName();
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
                if namingConvention("Interfaces", tokens[i], flatcase, IPascalCase, true)
                else if namingConvention("Interfaces", tokens[i], UPPERCASE, IPascalCase, true)
                else if namingConvention("Interfaces", tokens[i], camelCase, IPascalCase, true)
                else if namingConvention("Interfaces", tokens[i], snake_case, IPascalCase, true)
                else if namingConvention("Interfaces", tokens[i], SCREAMING_SNAKE_CASE, IPascalCase, true)
                else if namingConvention("Interfaces", tokens[i], PascalCase, IPascalCase, true)
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
                if (tokens[i].getValue() == "static") {
                    nextAttributes.push_back("static");
                    i++;
                }
                Token type = tokens[i];
                if (type.getType() == tok_function) {
                    if (currentStruct != nullptr) {
                        if (std::find(nextAttributes.begin(), nextAttributes.end(), "static") != nextAttributes.end() || currentStruct->isStatic()) {
                            std::string name = tokens[i + 1].getValue();
                            Token func = tokens[i + 1];
                            Function* function = parseFunction(currentStruct->getName() + "$" + name, func, errors, warns, nextAttributes, i, tokens);
                            function->isExternC = true;
                            nextAttributes.clear();
                            extern_functions.push_back(function);
                            continue;
                        }
                        Token func = tokens[i + 1];
                        std::string name = func.getValue();
                        Function* function = parseMethod(name, func, currentStruct->getName(), errors, warns, nextAttributes, i, tokens);
                        function->isExternC = true;
                        extern_functions.push_back(function);
                        nextAttributes.clear();
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
                    nextAttributes.clear();
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
                    if namingConvention("Variables", tokens[i], flatcase, camelCase, false)
                    else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase, false)
                    else if namingConvention("Variables", tokens[i], PascalCase, camelCase, false)
                    else if namingConvention("Variables", tokens[i], snake_case, camelCase, false)
                    else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase, false)
                    else if namingConvention("Variables", tokens[i], IPascalCase, camelCase, false)
                    std::string type = "any";
                    i++;
                    bool isConst = false;
                    bool isMut = false;
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        FPResult r = parseType(tokens, &i);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        isConst = typeIsConst(type);
                        isMut = typeIsMut(type);
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
                    Variable v = Variable(name, type, isConst, isMut);
                    v.canBeNil = typeCanBeNil(v.getType());
                    extern_globals.push_back(v);
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
                if (token.getValue() == "lambda" && (((ssize_t) i) - 3 >= 0 && tokens[i - 3].getType() != tok_declare)) isInLambda = true;
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
                if namingConvention("Variables", tokens[i], flatcase, camelCase, false)
                else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase, false)
                else if namingConvention("Variables", tokens[i], PascalCase, camelCase, false)
                else if namingConvention("Variables", tokens[i], snake_case, camelCase, false)
                else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase, false)
                else if namingConvention("Variables", tokens[i], IPascalCase, camelCase, false)
                Token tok = tokens[i];
                std::string type = "any";
                i++;
                bool isConst = false;
                bool isMut = false;
                if (tokens[i].getType() == tok_column) {
                    i++;
                    FPResult r = parseType(tokens, &i);
                    if (!r.success) {
                        errors.push_back(r);
                        continue;
                    }
                    type = r.value;
                    isConst = typeIsConst(type);
                    isMut = typeIsMut(type);
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
                Variable v = Variable(name, type, isConst, isMut);
                v.canBeNil = typeCanBeNil(v.getType());
                globals.push_back(v);
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
                if namingConvention("Variables", tokens[i], flatcase, camelCase, false)
                else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase, false)
                else if namingConvention("Variables", tokens[i], PascalCase, camelCase, false)
                else if namingConvention("Variables", tokens[i], snake_case, camelCase, false)
                else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase, false)
                else if namingConvention("Variables", tokens[i], IPascalCase, camelCase, false)
                Token tok = tokens[i];
                std::string type = "any";
                i++;
                bool isConst = false;
                bool isMut = false;
                if (tokens[i].getType() == tok_column) {
                    i++;
                    FPResult r = parseType(tokens, &i);
                    if (!r.success) {
                        errors.push_back(r);
                        continue;
                    }
                    type = r.value;
                    isConst = typeIsConst(type);
                    isMut = typeIsMut(type);
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
                Variable v = Variable(name, type, isConst, isMut);
                v.canBeNil = typeCanBeNil(v.getType());
                currentContainer->addMember(v);
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
                if namingConvention("Variables", tokens[i], flatcase, camelCase, false)
                else if namingConvention("Variables", tokens[i], UPPERCASE, camelCase, false)
                else if namingConvention("Variables", tokens[i], PascalCase, camelCase, false)
                else if namingConvention("Variables", tokens[i], snake_case, camelCase, false)
                else if namingConvention("Variables", tokens[i], SCREAMING_SNAKE_CASE, camelCase, false)
                else if namingConvention("Variables", tokens[i], IPascalCase, camelCase, false)
                std::string type = "any";
                i++;
                bool isConst = false;
                bool isMut = false;
                bool isInternalMut = false;
                bool isPrivate = false;
                if (tokens[i].getType() == tok_column) {
                    i++;
                    FPResult r = parseType(tokens, &i);
                    if (!r.success) {
                        errors.push_back(r);
                        continue;
                    }
                    type = r.value;
                    isConst = typeIsConst(type);
                    isMut = typeIsMut(type);
                    isInternalMut = typeIsReadonly(type);
                    isPrivate = contains<std::string>(nextAttributes, "private");
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
                if (currentStruct->isStatic() || contains<std::string>(nextAttributes, "static")) {
                    Variable v = Variable(currentStruct->getName() + "$" + name, type, isConst, isMut);
                    v.isPrivate = (contains<std::string>(nextAttributes, "private") || isPrivate);
                    nextAttributes.clear();
                    v.canBeNil = typeCanBeNil(v.getType());
                    globals.push_back(v);
                } else {
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
                        Variable v = Variable(name, type, isConst, isMut);
                        v.isPrivate = (std::find(nextAttributes.begin(), nextAttributes.end(), "private") != nextAttributes.end() || isPrivate);
                        v.canBeNil = typeCanBeNil(v.getType());
                        currentStruct->addMember(v);
                    } else {
                        if (isInternalMut) {
                            Variable v = Variable(name, type, isConst, isMut, currentStruct->getName());
                            v.isPrivate = (std::find(nextAttributes.begin(), nextAttributes.end(), "private") != nextAttributes.end() || isPrivate);
                            v.canBeNil = typeCanBeNil(v.getType());
                            currentStruct->addMember(v);
                        } else {
                            Variable v = Variable(name, type, isConst, isMut);
                            v.isPrivate = (std::find(nextAttributes.begin(), nextAttributes.end(), "private") != nextAttributes.end() || isPrivate);
                            v.canBeNil = typeCanBeNil(v.getType());
                            currentStruct->addMember(v);
                        }
                    }
                    nextAttributes.clear();
                }
            } else {

                auto validAttribute = [](Token& t) -> bool {
                    return t.getValue() == "expect" ||
                           t.getValue() == "export" ||
                           t.getValue() == "no_cleanup" ||
                           t.getValue() == "cdecl" ||
                           t.getValue() == "default" ||
                           t.getValue() == "static" ||
                           t.getValue() == "private" ||
                           t.getValue() == "asm" ||
                           t.getType()  == tok_string_literal;
                };

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
                } else if (validAttribute(tokens[i])) {
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
        result.enums = enums;

        std::vector<Struct> newStructs;
        
        for (Struct s : result.structs) {
            Struct super = getStructByName(result, s.extends());
            Struct oldSuper = s;
            while (super.getName().size()) {
                if (super.getName() == s.getName()) {
                    FPResult r;
                    r.message = "Struct '" + s.getName() + "' has Struct '" + oldSuper.getName() + "' in it's inheritance chain, which inherits from '" + super.getName() + "'";
                    r.value = s.nameToken().getValue();
                    r.line = s.nameToken().getLine();
                    r.in = s.nameToken().getFile();
                    r.type = s.nameToken().getType();
                    r.column = s.nameToken().getColumn();
                    r.column = s.nameToken().getColumn();
                    r.success = false;
                    result.errors.push_back(r);
                    FPResult r2;
                    r2.message = "Struct '" + oldSuper.getName() + "' declared here:";
                    r2.value = oldSuper.nameToken().getValue();
                    r2.line = oldSuper.nameToken().getLine();
                    r2.in = oldSuper.nameToken().getFile();
                    r2.type = oldSuper.nameToken().getType();
                    r2.column = oldSuper.nameToken().getColumn();
                    r2.column = oldSuper.nameToken().getColumn();
                    r2.success = false;
                    r2.isNote = true;
                    result.errors.push_back(r2);
                    oldSuper = super;
                    super = getStructByName(result, "SclObject");
                }
                std::vector<Variable> vars = super.getDefinedMembers();
                for (ssize_t i = vars.size() - 1; i >= 0; i--) {
                    s.addMember(vars[i], true);
                }
                oldSuper = super;
                super = getStructByName(result, super.extends());
            }
            newStructs.push_back(s);
        }
        result.structs = newStructs;
        return result;
    }
}
