#include <iostream>
#include <string>
#include <vector>

#include "../headers/Common.hpp"

const std::vector<std::string> intrinsics({
    "F:int$toInt8",
    "F:int$toInt16",
    "F:int$toInt32",
    "F:int$toInt",
    "F:int$toUInt8",
    "F:int$toUInt16",
    "F:int$toUInt32",
    "F:int$toUInt",
    "F:int$isValidInt8",
    "F:int$isValidInt16",
    "F:int$isValidInt32",
    "F:int$isValidUInt8",
    "F:int$isValidUInt16",
    "F:int$isValidUInt32",
    "F:int$toString",
    "F:int$toHexString",
    "F:int8$toString",
    "F:float$toString",
    "F:float$toPrecisionString",
    "F:float$toHexString",
    "F:float$fromBits",
    "F:system",
    "F:getenv",
    "F:time",
    "F:read",
    "F:write",
});

namespace sclc {
    std::map<std::string, std::string> templateArgs;

    std::string typeToRTSigIdent(std::string type);
    std::string argsToRTSignatureIdent(Function* f);

    Function* parseFunction(std::string name, Token& nameToken, std::vector<FPResult>& errors, size_t& i, std::vector<Token>& tokens) {
        if (name == "=>") {
            if (tokens[i + 2].getType() == tok_bracket_open && tokens[i + 3].getType() == tok_bracket_close) {
                i += 2;
                name += "[]";
            }
        } else if (name == "[") {
            if (tokens[i + 2].getType() == tok_bracket_close) {
                i++;
                name += "]";
            }
        }

        if (name == "+") name = "operator$add";
        else if (name == "-") name = "operator$sub";
        else if (name == "*") name = "operator$mul";
        else if (name == "/") name = "operator$div";
        else if (name == "%") name = "operator$mod";
        else if (name == "&") name = "operator$logic_and";
        else if (name == "|") name = "operator$logic_or";
        else if (name == "^") name = "operator$logic_xor";
        else if (name == "~") name = "operator$logic_not";
        else if (name == "<<") name = "operator$logic_lsh";
        else if (name == "<<<") name = "operator$logic_rol";
        else if (name == ">>") name = "operator$logic_rsh";
        else if (name == ">>>") name = "operator$logic_ror";
        else if (name == "**") name = "operator$pow";
        else if (name == ".") name = "operator$dot";
        else if (name == "<") name = "operator$less";
        else if (name == "<=") name = "operator$less_equal";
        else if (name == ">") name = "operator$more";
        else if (name == ">=") name = "operator$more_equal";
        else if (name == "==") name = "operator$equal";
        else if (name == "!") name = "operator$not";
        else if (name == "!!") name = "operator$assert_not_nil";
        else if (name == "!=") name = "operator$not_equal";
        else if (name == "&&") name = "operator$bool_and";
        else if (name == "||") name = "operator$bool_or";
        else if (name == "++") name = "operator$inc";
        else if (name == "--") name = "operator$dec";
        else if (name == "@") name = "operator$at";
        else if (name == "=>[]") name = "operator$set";
        else if (name == "[]") name = "operator$get";
        else if (name == "?") name = "operator$wildcard";

        Function* func = new Function(name, nameToken);
        func->setFile(nameToken.getFile());
        i += 2;
        if (tokens[i].getType() == tok_paren_open) {
            i++;
            while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                std::string fromTemplate = "";
                if (tokens[i].getType() == tok_identifier || tokens[i].getType() == tok_column) {
                    std::string name = tokens[i].getValue();
                    std::string type = "any";
                    if (tokens[i].getType() == tok_column) {
                        name = "";
                    } else {
                        i++;
                    }
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        FPResult r = parseType(tokens, &i, templateArgs);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        fromTemplate = r.message;
                        if (type == "none" || type == "nothing") {
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
                    if (type == "varargs" && name.size()) {
                        func->addArgument(Variable(name + "$size", "const int"));
                    }
                    func->addArgument(Variable(name, type).also([fromTemplate](Variable& v) {
                        v.typeFromTemplate = fromTemplate;
                    }));
                } else if (tokens[i].getType() == tok_curly_open) {
                    std::vector<std::string> multi;
                    multi.reserve(10);
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
                        multi.push_back(name);
                        i++;
                    }
                    i++;
                    std::string type;
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        FPResult r = parseType(tokens, &i, templateArgs);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        fromTemplate = r.message;
                        if (type == "none" || type == "nothing") {
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
                    for (std::string& s : multi) {
                        func->addArgument(Variable(s, type).also([fromTemplate](Variable& v) {
                            v.typeFromTemplate = fromTemplate;
                        }));
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
                i++;
            }
            if (tokens[i].getType() == tok_column) {
                i++;
                FPResult r = parseType(tokens, &i, templateArgs);
                if (!r.success) {
                    errors.push_back(r);
                    return nullptr;
                }
                std::string type = r.value;
                std::string fromTemplate = r.message;
                func->setReturnType(type);
                func->templateArg = r.message;
                if (namedReturn.size()) {
                    func->setNamedReturnValue(Variable(namedReturn, "mut " + type).also([fromTemplate](Variable& v) {
                        v.canBeNil = typeCanBeNil(v.getType());
                        v.typeFromTemplate = fromTemplate;
                    }));
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

    Method* parseMethod(std::string name, Token& nameToken, std::string memberName, std::vector<FPResult>& errors, size_t& i, std::vector<Token>& tokens) {
        if (name == "=>") {
            if (tokens[i + 2].getType() == tok_bracket_open && tokens[i + 3].getType() == tok_bracket_close) {
                i += 2;
                name += "[]";
            }
        } else if (name == "[") {
            if (tokens[i + 2].getType() == tok_bracket_close) {
                i++;
                name += "]";
            }
        } else if (name == "*") {
            if (tokens[i + 2].getType() == tok_identifier) {
                i++;
                if (tokens[i + 1].getValue() != memberName) {
                    FPResult result;
                    result.message = "Expected '" + memberName + "', but got '" + tokens[i + 1].getValue() + "'";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.type = tokens[i + 1].getType();
                    result.column = tokens[i + 1].getColumn();
                    result.success = false;
                    errors.push_back(result);
                    return nullptr;
                }
                name += tokens[i + 1].getValue();
            }
        }

        if (name == "+") name = "operator$add";
        else if (name == "-") name = "operator$sub";
        else if (name == "*") name = "operator$mul";
        else if (name == "/") name = "operator$div";
        else if (name == "%") name = "operator$mod";
        else if (name == "&") name = "operator$logic_and";
        else if (name == "|") name = "operator$logic_or";
        else if (name == "^") name = "operator$logic_xor";
        else if (name == "~") name = "operator$logic_not";
        else if (name == "<<") name = "operator$logic_lsh";
        else if (name == "<<<") name = "operator$logic_rol";
        else if (name == ">>") name = "operator$logic_rsh";
        else if (name == ">>>") name = "operator$logic_ror";
        else if (name == "**") name = "operator$pow";
        else if (name == ".") name = "operator$dot";
        else if (name == "<") name = "operator$less";
        else if (name == "<=") name = "operator$less_equal";
        else if (name == ">") name = "operator$more";
        else if (name == ">=") name = "operator$more_equal";
        else if (name == "==") name = "operator$equal";
        else if (name == "!") name = "operator$not";
        else if (name == "!!") name = "operator$assert_not_nil";
        else if (name == "!=") name = "operator$not_equal";
        else if (name == "&&") name = "operator$bool_and";
        else if (name == "||") name = "operator$bool_or";
        else if (name == "++") name = "operator$inc";
        else if (name == "--") name = "operator$dec";
        else if (name == "@") name = "operator$at";
        else if (name == "=>[]") name = "operator$set";
        else if (name == "[]") name = "operator$get";
        else if (name == "?") name = "operator$wildcard";

        Method* method = new Method(memberName, name, nameToken);
        method->setFile(nameToken.getFile());
        method->forceAdd(true);
        if (method->getName() == "init" || strstarts(method->getName(), "init")) {
            method->addModifier("<constructor>");
        }
        i += 2;
        if (tokens[i].getType() == tok_paren_open) {
            i++;
            while (i < tokens.size() && tokens[i].getType() != tok_paren_close) {
                std::string fromTemplate = "";
                if (tokens[i].getType() == tok_identifier || tokens[i].getType() == tok_column) {
                    std::string name = tokens[i].getValue();
                    std::string type = "any";
                    if (tokens[i].getType() == tok_column) {
                        name = "";
                    } else {
                        i++;
                    }
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        FPResult r = parseType(tokens, &i, templateArgs);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        fromTemplate = r.message;
                        if (type == "none" || type == "nothing") {
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
                    method->addArgument(Variable(name, type).also([fromTemplate](Variable& v) {
                        v.typeFromTemplate = fromTemplate;
                    }));
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
                        multi.push_back(name);
                        i++;
                    }
                    i++;
                    std::string type;
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        FPResult r = parseType(tokens, &i, templateArgs);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        fromTemplate = r.message;
                        if (type == "none" || type == "nothing") {
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
                    for (std::string& s : multi) {
                        method->addArgument(Variable(s, type).also([fromTemplate](Variable& v) {
                            v.typeFromTemplate = fromTemplate;
                        }));
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
            method->addArgument(Variable("self", "mut " + memberName));

            std::string namedReturn = "";
            if (tokens[i].getType() == tok_identifier) {
                namedReturn = tokens[i].getValue();
                i++;
            }
            if (tokens[i].getType() == tok_column) {
                i++;
                FPResult r = parseType(tokens, &i, templateArgs);
                if (!r.success) {
                    errors.push_back(r);
                    return nullptr;
                }

                std::string fromTemplate = r.message;
                std::string type = r.value;
                method->setReturnType(type);
                method->templateArg = fromTemplate;
                if (namedReturn.size()) {
                    method->setNamedReturnValue(Variable(namedReturn, type).also([fromTemplate](Variable& v) {
                        v.typeFromTemplate = fromTemplate;
                    }));
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

    bool isPrimitiveIntegerType(std::string);

    template<typename T>
    auto joinVecs(std::vector<T> a, std::vector<T> b) {
        std::vector<T> c;
        c.reserve(a.size() + b.size());
        c.insert(c.end(), a.begin(), a.end());
        c.insert(c.end(), b.begin(), b.end());
        return c;
    };

    #define expect(value, ...) if (!(__VA_ARGS__)) { \
            FPResult result; \
            result.message = "Expected " + std::string(value) + ", but got '" + tokens[i].getValue() + "'"; \
            result.va ## lue = tokens[i].getValue(); \
            result.line = tokens[i].getLine(); \
            result.in = tokens[i].getFile(); \
            result.type = tokens[i].getType(); \
            result.column = tokens[i].getColumn(); \
            result.success = false; \
            errors.push_back(result); \
            continue; \
        }

    TPResult parseFile(std::string file) {
        FILE* f = fopen(file.c_str(), "rb");

        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        if (!f) {
            FPResult result;
            result.message = "Could not open file '" + file + "'";
            result.success = false;
            result.in = file;
            errors.push_back(result);
            TPResult r;
            r.errors = errors;
            return r;
        }

        char* magic = (char*) malloc(4);
        fread(magic, 1, 4, f);
        if (strncmp(magic + 1, "SCL", 3) != 0) {
            FPResult result;
            result.message = "Invalid file format!";
            result.success = false;
            result.in = file;
            errors.push_back(result);
            TPResult r;
            r.errors = errors;
            return r;
        }

        bool ast = magic[0] == 0;

        uint32_t version = 0;
        fread(&version, sizeof(uint32_t), 1, f);
        uint8_t maj = (version >> 16) & 0xFF;
        uint8_t min = (version >> 8) & 0xFF;
        uint8_t patch = version & 0xFF;

        Version v(maj, min, patch);
        if (v > *Main.version) {
            FPResult result;
            result.message = "This file was compiled with a newer version of the compiler!";
            result.success = false;
            result.in = file;
            errors.push_back(result);
            TPResult r;
            r.errors = errors;
            return r;
        }

        uint32_t numContainers = 0;
        fread(&numContainers, sizeof(uint32_t), 1, f);

        uint32_t numStructs = 0;
        fread(&numStructs, sizeof(uint32_t), 1, f);

        uint32_t numEnums = 0;
        fread(&numEnums, sizeof(uint32_t), 1, f);

        uint32_t numFunctions = 0;
        fread(&numFunctions, sizeof(uint32_t), 1, f);

        uint32_t numGlobals = 0;
        fread(&numGlobals, sizeof(uint32_t), 1, f);

        uint32_t numExternGlobals = 0;
        fread(&numExternGlobals, sizeof(uint32_t), 1, f);

        uint32_t numInterfaces = 0;
        fread(&numInterfaces, sizeof(uint32_t), 1, f);

        uint32_t numLayouts = 0;
        fread(&numLayouts, sizeof(uint32_t), 1, f);

        uint32_t numTypealiases = 0;
        fread(&numTypealiases, sizeof(uint32_t), 1, f);

        std::vector<Container> containers;
        for (uint32_t i = 0; i < numContainers; i++) {
            uint32_t nameLength = 0;
            fread(&nameLength, sizeof(uint32_t), 1, f);
            char* name = (char*) malloc(nameLength + 1);
            fread(name, 1, nameLength, f);
            name[nameLength] = 0;

            uint32_t numMembers = 0;
            fread(&numMembers, sizeof(uint32_t), 1, f);
            Container c(name);

            for (uint32_t j = 0; j < numMembers; j++) {
                uint32_t memberNameLength = 0;
                fread(&memberNameLength, sizeof(uint32_t), 1, f);
                char* memberName = (char*) malloc(memberNameLength + 1);
                fread(memberName, 1, memberNameLength, f);

                uint32_t memberTypeLength = 0;
                fread(&memberTypeLength, sizeof(uint32_t), 1, f);
                char* memberType = (char*) malloc(memberTypeLength + 1);
                fread(memberType, 1, memberTypeLength, f);

                c.addMember(Variable(memberName, memberType));
            }

            containers.push_back(c);
        }

        std::vector<Struct> structs;
        for (uint32_t i = 0; i < numStructs; i++) {
            uint32_t nameLength = 0;
            fread(&nameLength, sizeof(uint32_t), 1, f);
            char* name = (char*) malloc(nameLength + 1);
            fread(name, 1, nameLength, f);
            name[nameLength] = 0;

            Struct s(name);

            fread(&s.flags, sizeof(uint32_t), 1, f);

            uint32_t numMembers = 0;
            fread(&numMembers, sizeof(uint32_t), 1, f);
            
            for (uint32_t j = 0; j < numMembers; j++) {
                uint32_t memberNameLength = 0;
                fread(&memberNameLength, sizeof(uint32_t), 1, f);
                char* memberName = (char*) malloc(memberNameLength + 1);
                fread(memberName, 1, memberNameLength, f);

                uint32_t memberTypeLength = 0;
                fread(&memberTypeLength, sizeof(uint32_t), 1, f);
                char* memberType = (char*) malloc(memberTypeLength + 1);
                fread(memberType, 1, memberTypeLength, f);

                s.addMember(Variable(memberName, memberType));
            }

            structs.push_back(s);
        }

        std::vector<Enum> enums;
        for (uint32_t i = 0; i < numEnums; i++) {
            uint32_t nameLength = 0;
            fread(&nameLength, sizeof(uint32_t), 1, f);
            char* name = (char*) malloc(nameLength + 1);
            fread(name, 1, nameLength, f);
            name[nameLength] = 0;

            Enum e(name);

            uint32_t numMembers = 0;
            fread(&numMembers, sizeof(uint32_t), 1, f);
            
            for (uint32_t j = 0; j < numMembers; j++) {
                uint32_t memberNameLength = 0;
                fread(&memberNameLength, sizeof(uint32_t), 1, f);
                char* memberName = (char*) malloc(memberNameLength + 1);
                fread(memberName, 1, memberNameLength, f);

                long memberValue = -1;
                fread(&memberValue, sizeof(long), 1, f);
                
                e.addMember(memberName, memberValue);
            }

            enums.push_back(e);
        }

        std::vector<Function*> functions;
        for (uint32_t i = 0; i < numFunctions; i++) {
            uint8_t isMethod = 0;
            fread(&isMethod, sizeof(uint8_t), 1, f);

            uint32_t nameLength = 0;
            fread(&nameLength, sizeof(uint32_t), 1, f);
            char* name = (char*) malloc(nameLength + 1);
            fread(name, 1, nameLength, f);
            name[nameLength] = 0;

            std::string memberOf = "";
            if (isMethod) {
                uint32_t memberTypeLength = 0;
                fread(&memberTypeLength, sizeof(uint32_t), 1, f);
                char* memberType = (char*) malloc(memberTypeLength + 1);
                fread(memberType, 1, memberTypeLength, f);
                memberType[memberTypeLength] = 0;
                memberOf = memberType;
            }

            uint32_t returnTypeLength = 0;
            fread(&returnTypeLength, sizeof(uint32_t), 1, f);
            char* returnType = (char*) malloc(returnTypeLength + 1);
            fread(returnType, 1, returnTypeLength, f);
            returnType[returnTypeLength] = 0;

            Function* func;
            if (isMethod) {
                func = new Method(memberOf, name, Token(tok_identifier, name, 0, file));
            } else {
                func = new Function(name, Token(tok_identifier, name, 0, file));
            }

            func->setReturnType(returnType);

            func->isExternC = func->has_expect;

            uint32_t numModifiers = 0;
            fread(&numModifiers, sizeof(uint32_t), 1, f);
            for (uint32_t j = 0; j < numModifiers; j++) {
                uint32_t modifierLength = 0;
                fread(&modifierLength, sizeof(uint32_t), 1, f);
                char* modifier = (char*) malloc(modifierLength + 1);
                fread(modifier, 1, modifierLength, f);
                modifier[modifierLength] = 0;

                func->addModifier(modifier);
            }

            if (isMethod && contains<std::string>(func->getModifiers(), "<virtual>")) {
                ((Method*) func)->forceAdd(true);
            }

            if (!ast)
                func->addModifier("<binary-inherited>");

            uint32_t numArguments = 0;
            fread(&numArguments, sizeof(uint32_t), 1, f);

            for (uint32_t j = 0; j < numArguments; j++) {
                std::string varName = "";

                if (ast) {
                    uint32_t varNameLength = 0;
                    fread(&varNameLength, sizeof(uint32_t), 1, f);
                    char* varNameC = (char*) malloc(varNameLength + 1);
                    fread(varNameC, 1, varNameLength, f);
                    varNameC[varNameLength] = 0;
                    varName = varNameC;
                }

                uint32_t typeLength = 0;
                fread(&typeLength, sizeof(uint32_t), 1, f);
                char* type = (char*) malloc(typeLength + 1);
                fread(type, 1, typeLength, f);
                type[typeLength] = 0;

                func->addArgument(Variable(varName, type));
            }

            if (ast) {
                uint32_t numStatements = 0;
                fread(&numStatements, sizeof(uint32_t), 1, f);

                for (uint32_t j = 0; j < numStatements; j++) {
                    TokenType type = tok_eof;
                    fread(&type, sizeof(TokenType), 1, f);

                    uint32_t length = 0;
                    fread(&length, sizeof(uint32_t), 1, f);
                    char* value = (char*) malloc(length + 1);
                    fread(value, 1, length, f);
                    value[length] = 0;
                    func->addToken(Token(type, value, 0, file));
                }
            }

            functions.push_back(func);
        }

        std::vector<Variable> globals;
        for (uint32_t i = 0; i < numGlobals; i++) {
            uint32_t nameLength = 0;
            fread(&nameLength, sizeof(uint32_t), 1, f);
            char* name = (char*) malloc(nameLength + 1);
            fread(name, 1, nameLength, f);
            name[nameLength] = 0;

            uint32_t typeLength = 0;
            fread(&typeLength, sizeof(uint32_t), 1, f);
            char* type = (char*) malloc(typeLength + 1);
            fread(type, 1, typeLength, f);
            type[typeLength] = 0;

            globals.push_back(Variable(name, type));
        }

        std::vector<Variable> extern_globals;
        for (uint32_t i = 0; i < numExternGlobals; i++) {
            uint32_t nameLength = 0;
            fread(&nameLength, sizeof(uint32_t), 1, f);
            char* name = (char*) malloc(nameLength + 1);
            fread(name, 1, nameLength, f);
            name[nameLength] = 0;

            uint32_t typeLength = 0;
            fread(&typeLength, sizeof(uint32_t), 1, f);
            char* type = (char*) malloc(typeLength + 1);
            fread(type, 1, typeLength, f);
            type[typeLength] = 0;

            extern_globals.push_back(Variable(name, type));
        }

        std::vector<Interface*> interfaces;
        for (uint32_t i = 0; i < numInterfaces; i++) {
            uint32_t nameLength = 0;
            fread(&nameLength, sizeof(uint32_t), 1, f);
            char* name = (char*) malloc(nameLength + 1);
            fread(name, 1, nameLength, f);
            name[nameLength] = 0;

            Interface* interface = new Interface(name);

            uint32_t numFunctions = 0;
            fread(&numFunctions, sizeof(uint32_t), 1, f);

            for (uint32_t j = 0; j < numFunctions; j++) {
                uint32_t nameLength = 0;
                fread(&nameLength, sizeof(uint32_t), 1, f);
                char* name = (char*) malloc(nameLength + 1);
                fread(name, 1, nameLength, f);
                name[nameLength] = 0;

                uint32_t memberTypeLength = 0;
                fread(&memberTypeLength, sizeof(uint32_t), 1, f);
                char* memberType = (char*) malloc(memberTypeLength + 1);
                fread(memberType, 1, memberTypeLength, f);
                memberType[memberTypeLength] = 0;

                uint32_t returnTypeLength = 0;
                fread(&returnTypeLength, sizeof(uint32_t), 1, f);
                char* returnType = (char*) malloc(returnTypeLength + 1);
                fread(returnType, 1, returnTypeLength, f);
                returnType[returnTypeLength] = 0;

                Function* func = new Method(memberType, name, Token(tok_identifier, name, 0, file));

                uint32_t numModifiers = 0;
                fread(&numModifiers, sizeof(uint32_t), 1, f);
                for (uint32_t k = 0; k < numModifiers; k++) {
                    uint32_t modifierLength = 0;
                    fread(&modifierLength, sizeof(uint32_t), 1, f);
                    char* modifier = (char*) malloc(modifierLength + 1);
                    fread(modifier, 1, modifierLength, f);
                    modifier[modifierLength] = 0;

                    func->addModifier(modifier);
                }

                uint32_t numArguments = 0;
                fread(&numArguments, sizeof(uint32_t), 1, f);

                for (uint32_t k = 0; k < numArguments; k++) {
                    std::string name = "";

                    if (ast) {
                        uint32_t nameLength = 0;
                        fread(&nameLength, sizeof(uint32_t), 1, f);
                        char* name = (char*) malloc(nameLength + 1);
                        fread(name, 1, nameLength, f);
                        name[nameLength] = 0;
                    }

                    uint32_t typeLength = 0;
                    fread(&typeLength, sizeof(uint32_t), 1, f);
                    char* type = (char*) malloc(typeLength + 1);
                    fread(type, 1, typeLength, f);
                    type[typeLength] = 0;

                    func->addArgument(Variable(name, type));
                }
            }

            interfaces.push_back(interface);
        }

        std::vector<Layout> layouts;
        for (uint32_t i = 0; i < numLayouts; i++) {
            uint32_t nameLength = 0;
            fread(&nameLength, sizeof(uint32_t), 1, f);
            char* name = (char*) malloc(nameLength + 1);
            fread(name, 1, nameLength, f);
            name[nameLength] = 0;

            Layout layout(name);

            uint32_t numFields = 0;
            fread(&numFields, sizeof(uint32_t), 1, f);

            for (uint32_t j = 0; j < numFields; j++) {
                uint32_t nameLength = 0;
                fread(&nameLength, sizeof(uint32_t), 1, f);
                char* name = (char*) malloc(nameLength + 1);
                fread(name, 1, nameLength, f);
                name[nameLength] = 0;

                uint32_t typeLength = 0;
                fread(&typeLength, sizeof(uint32_t), 1, f);
                char* type = (char*) malloc(typeLength + 1);
                fread(type, 1, typeLength, f);
                type[typeLength] = 0;

                layout.addMember(Variable(name, type));
            }

            layouts.push_back(layout);
        }

        Deprecation typealiases;
        for (uint32_t i = 0; i < numTypealiases; i++) {
            uint32_t nameLength = 0;
            fread(&nameLength, sizeof(uint32_t), 1, f);
            char* name = (char*) malloc(nameLength + 1);
            fread(name, 1, nameLength, f);
            name[nameLength] = 0;

            uint32_t typeLength = 0;
            fread(&typeLength, sizeof(uint32_t), 1, f);
            char* type = (char*) malloc(typeLength + 1);
            fread(type, 1, typeLength, f);
            type[typeLength] = 0;

            typealiases[name] = type;
        }

        TPResult result;
        result.errors = errors;
        result.warns = warns;
        result.containers = containers;
        result.structs = structs;
        result.interfaces = interfaces;
        result.enums = enums;
        result.globals = globals;
        result.layouts = layouts;
        result.extern_globals = extern_globals;
        result.typealiases = typealiases;
        result.functions = functions;
        return result;
    }

    std::string operator ""_s(const char* str, size_t len) {
        return std::string(str, len);
    }

    std::string operator ""_s(char c) {
        return std::string(1, c);
    }

    std::string specToCIdent(std::string in);

    struct Macro {
        std::vector<std::string> args;
        std::vector<Token> tokens;

        void expand(std::vector<Token>& otherTokens, size_t& i, std::unordered_map<std::string, Token>& args, std::vector<FPResult>& errors) {
            #define token this->tokens[j]
            #define nextToken this->tokens[j + 1]
            #define MacroError(msg) ({ \
                FPResult error; \
                error.success = false; \
                error.message = msg; \
                error.in = tokens[i].getFile(); \
                error.line = tokens[i].getLine(); \
                error.column = tokens[i].getColumn(); \
                error.type = tokens[i].getType(); \
                error.value = tokens[i].getValue(); \
                error; \
            })
            for (size_t j = 0; j < this->tokens.size(); j++) {
                if (token.getType() == tok_identifier && token.getValue() == "expand") {
                    j++;
                    if (token.getType() != tok_curly_open) {
                        errors.push_back(MacroError("Expected '{' after 'expand'"));
                        return;
                    }
                    j++;
                    size_t depth = 1;
                    std::vector<Token> expanded;
                    while (depth > 0) {
                        if (token.getType() == tok_curly_open) {
                            depth++;
                        } else if (token.getType() == tok_curly_close) {
                            depth--;
                        }
                        if (token.getType() == tok_dollar) {
                            if (nextToken.getType() == tok_identifier) {
                                if (args.find(nextToken.getValue()) != args.end()) {
                                    expanded.push_back(args[nextToken.getValue()]);
                                } else {
                                    errors.push_back(MacroError("Unknown macro argument '" + nextToken.getValue() + "'"));
                                }
                            } else {
                                errors.push_back(MacroError("Expected identifier after '$'"));
                            }
                            j++;
                        } else {
                            expanded.push_back(token);
                        }
                        j++;
                    }
                    otherTokens.insert(otherTokens.begin() + i, expanded.begin(), expanded.end());
                    i += expanded.size();
                }
            }
            #undef token
            #undef nextToken
        }
    };
    
    TPResult SyntaxTree::parse(std::vector<std::string>& binaryHeaders) {
        Function* currentFunction = nullptr;
        Container* currentContainer = nullptr;
        Struct* currentStruct = nullptr;
        std::vector<Struct> currentStructs;
        Interface* currentInterface = nullptr;
        Deprecation currentDeprecation;

        int isInLambda = 0;
        int isInUnsafe = 0;

        std::vector<std::string> uses;
        std::vector<std::string> nextAttributes;
        std::vector<Variable> globals;
        std::vector<Container> containers;
        std::vector<Struct> structs;
        std::vector<Layout> layouts;
        std::vector<Interface*> interfaces;
        std::vector<Enum> enums;
        std::vector<Function*> functions;
        std::vector<Variable> extern_globals;
        std::unordered_map<std::string, std::string> typealiases;

        uses.reserve(100);
        nextAttributes.reserve(100);
        globals.reserve(100);
        containers.reserve(100);
        structs.reserve(100);
        layouts.reserve(100);
        interfaces.reserve(100);
        enums.reserve(100);
        functions.reserve(100);
        extern_globals.reserve(100);
        typealiases.reserve(100);

        Variable& lastDeclaredVariable = Variable::emptyVar();

        std::vector<FPResult> errors;
        std::vector<FPResult> warns;

        for (std::string& binaryHeader : binaryHeaders) {
            const TPResult& tmp = parseFile(binaryHeader);

            if (!tmp.errors.empty()) {
                return tmp;
            }

            containers.insert(containers.end(), tmp.containers.begin(), tmp.containers.end());
            structs.insert(structs.end(), tmp.structs.begin(), tmp.structs.end());
            enums.insert(enums.end(), tmp.enums.begin(), tmp.enums.end());
            functions.insert(functions.end(), tmp.functions.begin(), tmp.functions.end());
            globals.insert(globals.end(), tmp.globals.begin(), tmp.globals.end());
            extern_globals.insert(extern_globals.end(), tmp.extern_globals.begin(), tmp.extern_globals.end());
            interfaces.insert(interfaces.end(), tmp.interfaces.begin(), tmp.interfaces.end());
            layouts.insert(layouts.end(), tmp.layouts.begin(), tmp.layouts.end());
            for (auto it = tmp.typealiases.begin(); it != tmp.typealiases.end(); it++) {
                typealiases[it->first] = it->second;
            }
        }

        std::unordered_map<std::string, Macro> macros;
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].getType() != tok_identifier || tokens[i].getValue() != "macro!") {
                continue;
            }
            size_t start = i;
            i++;
            std::string name = tokens[i].getValue();
            i++;
            Macro macro;
            if (tokens[i].getType() == tok_paren_open) {
                i++;
                while (tokens[i].getType() != tok_paren_close) {
                    macro.args.push_back(tokens[i].getValue());
                    i++;
                    if (tokens[i].getType() == tok_comma) {
                        i++;
                    }
                }
            } else {
                errors.push_back(MacroError("Expected '(' after macro name"));
            }
            i++;
            if (tokens[i].getType() != tok_curly_open) {
                errors.push_back(MacroError("Expected '{' after macro name"));
            }
            i++;
            ssize_t depth = 1;
            while (depth > 0) {
                if (tokens[i].getType() == tok_curly_open) {
                    depth++;
                } else if (tokens[i].getType() == tok_curly_close) {
                    depth--;
                }
                if (depth <= 0) {
                    i++;
                    break;
                }
                macro.tokens.push_back(tokens[i]);
                i++;
            }
            // delete macro declaration
            tokens.erase(tokens.begin() + start, tokens.begin() + i);
            i = start;
            i--;
            macros[name] = macro;
        }

        for (size_t i = 0; i < tokens.size(); i++) {
            if (
                tokens[i].getType() != tok_identifier ||
                macros.find(tokens[i].getValue()) == macros.end() ||
                i + 1 >= tokens.size() ||
                tokens[i + 1].getValue() != "!" ||
                tokens[i + 1].getLine() != tokens[i].getLine()
            ) {
                continue;
            }
            Macro& macro = macros[tokens[i].getValue()];
            std::unordered_map<std::string, Token> args;
            size_t start = i;
            i += 2;
            for (size_t j = 0; j < macro.args.size(); j++) {
                args[macro.args[j]] = tokens[i];
                i++;
            }
            // delete macro call
            tokens.erase(tokens.begin() + start, tokens.begin() + i);
            i = start;
            macro.expand(tokens, i, args, errors);
            i--;
        }

        // Builtins
        if (!Main.options.noScaleFramework) {
            Function* builtinIsInstanceOf = new Function("builtinIsInstanceOf", Token(tok_identifier, "builtinIsInstanceOf", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addModifier("export");

            builtinIsInstanceOf->addArgument(Variable("obj", "any"));
            builtinIsInstanceOf->addArgument(Variable("typeStr", "str"));

            builtinIsInstanceOf->setReturnType("int");
            
            // if typeStr "any" == then true return
            builtinIsInstanceOf->addToken(Token(tok_if, "if", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "typeStr", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_string_literal, "any", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "==", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_then, "then", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_true, "true", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_return, "return", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_fi, "fi", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "obj", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "typeStr", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_column, ":", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "view", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "builtinHash", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_identifier, "builtinTypeEquals", 0, "builtinIsInstanceOf.scale@"));
            builtinIsInstanceOf->addToken(Token(tok_return, "return", 0, "builtinIsInstanceOf.scale@"));
            functions.push_back(builtinIsInstanceOf);

            Function* builtinHash = new Function("builtinHash", Token(tok_identifier, "builtinHash", 0, "builtinHash.scale@"));
            builtinHash->isExternC = true;
            builtinHash->addModifier("extern");
            builtinHash->addModifier("cdecl");
            builtinHash->addModifier("id");
            
            builtinHash->addArgument(Variable("data", "[int8]"));
            
            builtinHash->setReturnType("int32");
            functions.push_back(builtinHash);

            Function* builtinIdentityHash = new Function("builtinIdentityHash", Token(tok_identifier, "builtinIdentityHash", 0, "builtinIdentityHash.scale@"));
            builtinIdentityHash->isExternC = true;
            builtinIdentityHash->addModifier("extern");
            builtinIdentityHash->addModifier("cdecl");
            builtinIdentityHash->addModifier("_scl_identity_hash");
            
            builtinIdentityHash->addArgument(Variable("obj", "any"));
            
            builtinIdentityHash->setReturnType("int");
            functions.push_back(builtinIdentityHash);

            Function* builtinAtomicClone = new Function("builtinAtomicClone", Token(tok_identifier, "builtinAtomicClone", 0, "builtinAtomicClone.scale@"));
            builtinAtomicClone->isExternC = true;
            builtinAtomicClone->addModifier("extern");
            builtinAtomicClone->addModifier("cdecl");
            builtinAtomicClone->addModifier("_scl_atomic_clone");
            
            builtinAtomicClone->addArgument(Variable("obj", "any"));
            
            builtinAtomicClone->setReturnType("any");
            functions.push_back(builtinAtomicClone);

            Function* builtinTypeEquals = new Function("builtinTypeEquals", Token(tok_identifier, "builtinTypeEquals", 0, "builtinTypeEquals.scale@"));
            builtinTypeEquals->isExternC = true;
            builtinTypeEquals->addModifier("extern");
            builtinTypeEquals->addModifier("cdecl");
            builtinTypeEquals->addModifier("_scl_is_instance_of");

            builtinTypeEquals->addArgument(Variable("obj", "any"));
            builtinTypeEquals->addArgument(Variable("typeId", "int32"));

            builtinTypeEquals->setReturnType("int");
            functions.push_back(builtinTypeEquals);

            Function* builtinToString = new Function("builtinToString", Token(tok_identifier, "builtinToString", 0, "builtinToString.scale@"));
            builtinToString->addModifier("export");
            
            builtinToString->addArgument(Variable("val", "any"));

            builtinToString->setReturnType("str");

            // if val is SclObject then val as SclObject:toString return else val int::toString return fi
            builtinToString->addToken(Token(tok_if, "if", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_identifier, "val", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_is, "is", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_identifier, "SclObject", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_then, "then", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_identifier, "val", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_as, "as", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_identifier, "SclObject", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_column, ":", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_identifier, "toString", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_return, "return", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_fi, "fi", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_identifier, "val", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_identifier, "int", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_double_column, "::", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_identifier, "toString", 0, "builtinToString.scale@"));
            builtinToString->addToken(Token(tok_return, "return", 0, "builtinToString.scale@"));

            functions.push_back(builtinToString);

            Function* builtinUnreachable = new Function("builtinUnreachable", Token(tok_identifier, "builtinUnreachable", 0, "builtinUnreachable.scale@"));
            builtinUnreachable->isExternC = true;
            builtinUnreachable->addModifier("extern");
            builtinUnreachable->setReturnType("none");

            functions.push_back(builtinUnreachable);
        }

        auto findFunctionByName = [&](std::string name) {
            for (size_t i = 0; i < functions.size(); i++) {
                if (functions[i]->isMethod) {
                    continue;
                }
                if (functions[i]->getName() == name) {
                    return functions[i];
                }
            }
            for (size_t i = 0; i < functions.size(); i++) {
                if (functions[i]->isMethod) {
                    continue;
                }
                if (functions[i]->getName() == name) {
                    return functions[i];
                }
            }
            return (Function*) nullptr;
        };

        auto findMethodByName = [&](std::string name, std::string memberType) {
            for (size_t i = 0; i < functions.size(); i++) {
                if (!functions[i]->isMethod) {
                    continue;
                }
                if (functions[i]->getName() == name && ((Method*) functions[i])->getMemberType() == memberType) {
                    return (Method*) functions[i];
                }
            }
            for (size_t i = 0; i < functions.size(); i++) {
                if (!functions[i]->isMethod) {
                    continue;
                }
                if (functions[i]->getName() == name && ((Method*) functions[i])->getMemberType() == memberType) {
                    return (Method*) functions[i];
                }
            }
            return (Method*) nullptr;
        };
        
        for (size_t i = 0; i < tokens.size(); i++) {
            Token& token = tokens[i];

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
                    if (tokens[i + 2].getType() == tok_column) {
                        std::string name = tokens[i + 1].getValue();
                        if (name != currentStruct->getName()) {
                            FPResult result;
                            result.message = "Cannot define method on different struct inside of a struct. Current struct: " + currentStruct->getName();
                            result.value = tokens[i + 1].getValue();
                            result.line = tokens[i + 1].getLine();
                            result.in = tokens[i + 1].getFile();
                            result.type = tokens[i + 1].getType();
                            result.column = tokens[i + 1].getColumn();
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        i += 2;
                    }
                    if (contains<std::string>(nextAttributes, "static") || currentStruct->isStatic()) {
                        std::string name = tokens[i + 1].getValue();
                        Token& func = tokens[i + 1];
                        currentFunction = parseFunction(currentStruct->getName() + "$" + name, func, errors, i, tokens);
                        currentFunction->deprecated = currentDeprecation;
                        currentDeprecation.clear();
                        currentFunction->member_type = currentStruct->getName();
                        currentFunction->isPrivate = contains<std::string>(nextAttributes, "private");
                        for (std::string& s : nextAttributes) {
                            currentFunction->addModifier(s);
                        }
                        Function* f = findFunctionByName(currentFunction->getName());
                        if (f) {
                            currentFunction->setName(currentFunction->getName() + "$$ol" + argsToRTSignatureIdent(currentFunction));
                        }
                        if (contains<std::string>(nextAttributes, "expect")) {
                            currentFunction->isExternC = true;
                            functions.push_back(currentFunction);
                            currentFunction = nullptr;
                        }
                        if (contains<std::string>(nextAttributes, "intrinsic")) {
                            if (contains<std::string>(intrinsics, "F:" + currentFunction->getName())) {
                                currentFunction->isExternC = true;
                                functions.push_back(currentFunction);
                            }
                        }
                        nextAttributes.clear();
                        continue;
                    }
                    Token& func = tokens[i + 1];
                    std::string name = func.getValue();
                    currentFunction = parseMethod(name, func, currentStruct->getName(), errors, i, tokens);
                    currentFunction->isPrivate = contains<std::string>(nextAttributes, "private");
                    for (std::string& s : nextAttributes) {
                        currentFunction->addModifier(s);
                    }
                    Function* f = findMethodByName(currentFunction->getName(), ((Method*) currentFunction)->getMemberType());
                    if (f) {
                        currentFunction->setName(currentFunction->getName() + "$$ol" + argsToRTSignatureIdent(currentFunction));
                    }
                    if (contains<std::string>(nextAttributes, "expect")) {
                        currentFunction->isExternC = true;
                        functions.push_back(currentFunction);
                        currentFunction = nullptr;
                    }
                    if (contains<std::string>(nextAttributes, "intrinsic")) {
                        if (contains<std::string>(intrinsics, "M:" + ((Method*) currentFunction)->getMemberType() + "::" + currentFunction->getName())) {
                            currentFunction->isExternC = true;
                            functions.push_back(currentFunction);
                        }
                    }
                    nextAttributes.clear();
                    continue;
                }
                if (currentInterface != nullptr) {
                    if (contains<std::string>(nextAttributes, "default")) {
                        Token& func = tokens[i + 1];
                        std::string name = func.getValue();
                        currentFunction = parseMethod(name, func, "", errors, i, tokens);
                        for (std::string& s : nextAttributes) {
                            currentFunction->addModifier(s);
                        }
                        currentInterface->addToImplement(currentFunction);
                        Method* m = new Method(currentInterface->getName(), name, func);
                        for (Variable& v : currentFunction->getArgs()) {
                            m->addArgument(v);
                        }
                        m->addArgument(Variable("self", "mut " + currentInterface->getName()));
                        m->setReturnType(currentFunction->getReturnType());
                        for (std::string& s : currentFunction->getModifiers()) {
                            m->addModifier(s);
                        }
                        functions.push_back(m);
                    } else {
                        std::string name = tokens[i + 1].getValue();
                        Token& func = tokens[i + 1];
                        Function* functionToImplement = parseFunction(name, func, errors, i, tokens);
                        functionToImplement->deprecated = currentDeprecation;
                        currentDeprecation.clear();
                        currentInterface->addToImplement(functionToImplement);

                        Method* m = new Method(currentInterface->getName(), name, func);
                        for (Variable& v : functionToImplement->getArgs()) {
                            m->addArgument(v);
                        }
                        m->addArgument(Variable("self", "mut " + currentInterface->getName()));
                        m->setReturnType(functionToImplement->getReturnType());
                        for (std::string& s : functionToImplement->getModifiers()) {
                            m->addModifier(s);
                        }
                        functions.push_back(m);
                    }
                    nextAttributes.clear();
                    continue;
                }
                i++;
                if (tokens[i + 1].getType() == tok_column) {
                    std::string member_type = tokens[i].getValue();
                    i++;
                    Token& func = tokens[i + 1];
                    std::string name = func.getValue();
                    currentFunction = parseMethod(name, func, member_type, errors, i, tokens);
                    if (contains<std::string>(nextAttributes, "private")) {
                        FPResult result;
                        result.message = "Methods cannot be declared 'private' if they are not in the struct body!";
                        result.value = func.getValue();
                        result.line = func.getLine();
                        result.in = func.getFile();
                        result.type = func.getType();
                        result.column = func.getColumn();
                        result.success = false;
                        errors.push_back(result);
                        nextAttributes.clear();
                        continue;
                    }
                } else {
                    i--;
                    std::string name = tokens[i + 1].getValue();
                    Token& func = tokens[i + 1];
                    currentFunction = parseFunction(name, func, errors, i, tokens);
                    currentFunction->deprecated = currentDeprecation;
                    currentDeprecation.clear();
                }
                for (std::string& s : nextAttributes) {
                    currentFunction->addModifier(s);
                }
                Function* f;
                if (currentFunction->isMethod) {
                    f = findMethodByName(currentFunction->getName(), ((Method*) currentFunction)->getMemberType());
                } else {
                    f = findFunctionByName(currentFunction->getName());
                }
                if (f) {
                    currentFunction->setName(currentFunction->getName() + "$$ol" + argsToRTSignatureIdent(currentFunction));
                }
                if (contains<std::string>(nextAttributes, "expect")) {
                    currentFunction->isExternC = true;
                    functions.push_back(currentFunction);
                    currentFunction = nullptr;
                }
                if (contains<std::string>(nextAttributes, "intrinsic")) {
                    if (contains<std::string>(intrinsics, "F:" + currentFunction->getName())) {
                        currentFunction->isExternC = true;
                        functions.push_back(currentFunction);
                    }
                }
                nextAttributes.clear();
            } else if (token.getType() == tok_end) {
                if (currentFunction != nullptr) {
                    if (isInLambda) {
                        isInLambda--;
                        if (!contains<Function*>(functions, currentFunction))
                            currentFunction->addToken(token);
                        continue;
                    }
                    if (isInUnsafe) {
                        isInUnsafe--;
                        if (!contains<Function*>(functions, currentFunction))
                            currentFunction->addToken(token);
                        continue;
                    }

                    bool functionWasOverloaded = false;

                    if (currentInterface == nullptr) {
                        Function* f;
                        if (currentFunction->isMethod) {
                            f = findMethodByName(currentFunction->getName(), ((Method*) currentFunction)->getMemberType());
                        } else {
                            f = findFunctionByName(currentFunction->getName());
                        }
                        if (f) {
                            if (f->isCVarArgs()) {
                                FPResult result;
                                result.message = "Cannot overload varargs function " + f->getName() + "!";
                                result.value = f->getName();
                                result.line = f->getNameToken().getLine();
                                result.in = f->getNameToken().getFile();
                                result.type = f->getNameToken().getType();
                                result.column = f->getNameToken().getColumn();
                                result.success = false;
                                errors.push_back(result);
                                nextAttributes.clear();
                                currentFunction = nullptr;
                                continue;
                            } else if (!currentFunction->has_intrinsic) {
                                if (currentFunction->getArgs() == f->getArgs()) {
                                    FPResult result;
                                    result.message = "Function " + currentFunction->getName() + " is already defined!";
                                    result.value = currentFunction->getName();
                                    result.line = currentFunction->getNameToken().getLine();
                                    result.in = currentFunction->getNameToken().getFile();
                                    result.type = currentFunction->getNameToken().getType();
                                    result.column = currentFunction->getNameToken().getColumn();
                                    result.success = false;
                                    errors.push_back(result);
                                    nextAttributes.clear();
                                    currentFunction = nullptr;
                                    continue;
                                }
                                currentFunction->setName(currentFunction->getName() + "$$ol" + argsToRTSignatureIdent(currentFunction));
                                functionWasOverloaded = true;
                            } else if (currentFunction != f) {
                                if (currentFunction->has_intrinsic) {
                                    FPResult result;
                                    result.message = "Cannot overload intrinsic function " + currentFunction->getName() + "!";
                                    result.value = currentFunction->getName();
                                    result.line = currentFunction->getNameToken().getLine();
                                    result.in = currentFunction->getNameToken().getFile();
                                    result.type = currentFunction->getNameToken().getType();
                                    result.column = currentFunction->getNameToken().getColumn();
                                    result.success = false;
                                    errors.push_back(result);
                                } else if (f->has_intrinsic) {
                                    FPResult result;
                                    result.message = "Cannot overload function " + currentFunction->getName() + " with intrinsic function!";
                                    result.value = currentFunction->getName();
                                    result.line = currentFunction->getNameToken().getLine();
                                    result.in = currentFunction->getNameToken().getFile();
                                    result.type = currentFunction->getNameToken().getType();
                                    result.column = currentFunction->getNameToken().getColumn();
                                    result.success = false;
                                    errors.push_back(result);
                                }
                                nextAttributes.clear();
                                currentFunction = nullptr;
                                continue;
                            }
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
                    } else {
                        if (functionWasOverloaded || !contains<Function*>(functions, currentFunction))
                            functions.push_back(currentFunction);
                    }
                    currentFunction = nullptr;
                } else if (currentContainer != nullptr) {
                    containers.push_back(*currentContainer);
                    currentContainer = nullptr;
                } else if (currentStruct != nullptr) {
                    structs.push_back(*currentStruct);
                    templateArgs.clear();
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
                    result.message = "Expected identifier for container name, but got '" + tokens[i + 1].getValue() + "'";
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
                if (tokens[i].getValue() == "str" || tokens[i].getValue() == "int" || tokens[i].getValue() == "float" || tokens[i].getValue() == "none" || tokens[i].getValue() == "nothing" || tokens[i].getValue() == "any" || isPrimitiveIntegerType(tokens[i].getValue())) {
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
                currentContainer = new Container(tokens[i].getValue());
                currentContainer->name_token = new Token(tokens[i]);
            } else if (token.getType() == tok_struct_def && (i == 0 || (((((long) i) - 1) >= 0) && tokens[i - 1].getType() != tok_double_column))) {
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
                    result.message = "Expected identifier for struct name, but got '" + tokens[i + 1].getValue() + "'";
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
                if (tokens[i].getValue() == "none") {
                    FPResult result;
                    result.message = "Invalid name for struct: '" + tokens[i].getValue() + "'";
                    result.value = tokens[i].getValue();
                    result.line = tokens[i].getLine();
                    result.in = tokens[i].getFile();
                    result.type = tokens[i].getType();
                    result.column = tokens[i].getColumn();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                currentStruct = new Struct(tokens[i].getValue(), tokens[i]);
                for (std::string& m : nextAttributes) {
                    currentStruct->addModifier(m);
                }
                bool open = contains<std::string>(nextAttributes, "open");
                
                nextAttributes.clear();
                bool hasSuperSpecified = false;
                if (tokens[i + 1].getValue() == "<") {
                    i++;
                    // *i = "<"
                    i++;
                    // *i = first template argument
                    while (tokens[i].getValue() != ">") {
                        if (tokens[i].getType() != tok_identifier) {
                            FPResult result;
                            result.message = "Expected identifier for template argument name, but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.column = tokens[i].getColumn();
                            result.success = false;
                            errors.push_back(result);
                            break;
                        }
                        std::string key = tokens[i].getValue();
                        i++;
                        // *i = ":"
                        if (tokens[i].getType() != tok_column) {
                            FPResult result;
                            result.message = "Expected ':' after template argument name, but got '" + tokens[i].getValue() + "'";
                            result.value = tokens[i].getValue();
                            result.line = tokens[i].getLine();
                            result.in = tokens[i].getFile();
                            result.type = tokens[i].getType();
                            result.column = tokens[i].getColumn();
                            result.success = false;
                            errors.push_back(result);
                            break;
                        }
                        i++;
                        FPResult value = parseType(tokens, &i, templateArgs);
                        if (!value.success) {
                            errors.push_back(value);
                            break;
                        }
                        currentStruct->required_typed_arguments++;
                        currentStruct->addTemplateArgument(key, value.value);
                        currentStruct->addMember(Variable("$template_arg_" + key, "uint32"));
                        currentStruct->addMember(Variable("$template_argname_" + key, "[int8]"));
                        templateArgs[key] = value.value;
                        i++;
                        if (tokens[i].getValue() == ",") {
                            i++;
                        }
                    }
                }
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

                if (open) {
                    structs.push_back(*currentStruct);
                    currentStruct = nullptr;
                }
            } else if (token.getType() == tok_identifier && token.getValue() == "layout") {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a layout inside of a container. Maybe you forgot an 'end' somewhere? Current container: " + currentContainer->getName();
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
                    result.message = "Cannot define a layout inside of a function. Maybe you forgot an 'end' somewhere? Current function: " + currentFunction->getName();
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
                    result.message = "Cannot define a layout inside of a struct. Maybe you forgot an 'end' somewhere? Current struct: " + currentStruct->getName();
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
                    result.message = "Cannot define a layout inside of an interface. Maybe you forgot an 'end' somewhere? Current interface: " + currentInterface->getName();
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
                std::string name = tokens[i].getValue();
                i++;
                Layout layout(name);

                while (tokens[i].getType() != tok_end) {
                    if (tokens[i].getType() != tok_declare) {
                        FPResult result;
                        result.message = "Only declarations are allowed inside of a layout.";
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.column = tokens[i].getColumn();
                        result.success = false;
                        errors.push_back(result);
                        break;
                    }
                    i++;
                    if (tokens[i].getType() != tok_identifier) {
                        FPResult result;
                        result.message = "Expected identifier for variable name, but got '" + tokens[i].getValue() + "'";
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.column = tokens[i].getColumn();
                        result.success = false;
                        errors.push_back(result);
                        break;
                    }
                    std::string name = tokens[i].getValue();
                    std::string type = "any";
                    i++;
                    if (tokens[i].getType() == tok_column) {
                        i++;
                        FPResult r = parseType(tokens, &i, templateArgs);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                        if (type == "none" || type == "nothing") {
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
                            break;
                        }
                    }
                    
                    layout.addMember(Variable(name, type));
                    i++;
                }

                layouts.push_back(layout);
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
                    result.message = "Expected identifier for enum name, but got '" + tokens[i].getValue() + "'";
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
                Enum e = Enum(name);
                e.name_token = new Token(tokens[i - 1]);
                while (tokens[i].getType() != tok_end) {
                    long next = e.nextValue;
                    if (tokens[i].getType() == tok_bracket_open) {
                        i++;
                        expect("number", tokens[i].getType() == tok_number);
                        next = parseNumber(tokens[i].getValue());
                        i++;
                        expect("]", tokens[i].getType() == tok_bracket_close);
                        i++;
                    }
                    if (tokens[i].getType() != tok_identifier) {
                        FPResult result;
                        result.message = "Expected identifier for enum member, but got '" + tokens[i].getValue() + "'";
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
                    try {
                        e.addMember(tokens[i].getValue(), next);
                    } catch (const std::runtime_error& e) {
                        FPResult result;
                        result.message = e.what();
                        result.value = tokens[i].getValue();
                        result.line = tokens[i].getLine();
                        result.in = tokens[i].getFile();
                        result.type = tokens[i].getType();
                        result.column = tokens[i].getColumn();
                        result.success = false;
                        errors.push_back(result);
                    }
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
                    result.message = "Expected identifier for interface declaration, but got '" + tokens[i + 1].getValue() + "'";
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
                if (tokens[i].getValue() == "str" || tokens[i].getValue() == "int" || tokens[i].getValue() == "float" || tokens[i].getValue() == "none" || tokens[i].getValue() == "any" || isPrimitiveIntegerType(tokens[i].getValue())) {
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
                currentInterface = new Interface(tokens[i].getValue());
                currentInterface->name_token = new Token(tokens[i]);
            } else if (token.getType() == tok_addr_of) {
                if (currentFunction == nullptr) {
                    if (tokens[i + 1].getType() == tok_identifier) {
                        nextAttributes.push_back(tokens[i + 1].getValue());
                        if (tokens[i + 1].getValue() == "relocatable") {
                            nextAttributes.push_back("export");
                        }
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
            } else if (currentFunction != nullptr && currentContainer == nullptr) {
                if (
                    token.getType() == tok_identifier &&
                    token.getValue() == "lambda" &&
                    (((ssize_t) i) - 3 >= 0 && tokens[i - 3].getType() != tok_declare) &&
                    (((ssize_t) i) - 1 >= 0 && tokens[i - 1].getType() != tok_as) &&
                    (((ssize_t) i) - 1 >= 0 && tokens[i - 1].getType() != tok_comma) &&
                    (((ssize_t) i) - 1 >= 0 && tokens[i - 1].getType() != tok_paren_open)
                ) isInLambda++;
                if (token.getType() == tok_identifier && token.getValue() == "unsafe") {
                    isInUnsafe++;
                }
                if (!contains<Function*>(functions, currentFunction))
                    currentFunction->addToken(token);
            } else if (token.getType() == tok_declare && currentContainer == nullptr && currentStruct == nullptr) {
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier for variable name, but got '" + tokens[i + 1].getValue() + "'";
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
                i++;
                if (tokens[i].getType() == tok_column) {
                    i++;
                    FPResult r = parseType(tokens, &i, templateArgs);
                    if (!r.success) {
                        errors.push_back(r);
                        continue;
                    }
                    type = r.value;
                    if (type == "none" || type == "nothing") {
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
                if (contains<std::string>(nextAttributes, "expect")) {
                    extern_globals.push_back(Variable(name, type));
                } else {
                    globals.push_back(Variable(name, type));
                }
                nextAttributes.clear();
            } else if (token.getType() == tok_declare && currentContainer != nullptr) {
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier for variable name, but got '" + tokens[i + 1].getValue() + "'";
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
                i++;
                if (tokens[i].getType() == tok_column) {
                    i++;
                    FPResult r = parseType(tokens, &i, templateArgs);
                    if (!r.success) {
                        errors.push_back(r);
                        continue;
                    }
                    type = r.value;
                    if (type == "none" || type == "nothing") {
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
                currentContainer->addMember(Variable(name, type));
            } else if (token.getType() == tok_declare && currentStruct != nullptr) {
                if (tokens[i + 1].getType() != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier for variable name, but got '" + tokens[i + 1].getValue() + "'";
                    result.value = tokens[i + 1].getValue();
                    result.line = tokens[i + 1].getLine();
                    result.in = tokens[i + 1].getFile();
                    result.column = tokens[i + 1].getColumn();
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                Token& nameToken = tokens[i];
                std::string name = tokens[i].getValue();
                std::string type = "any";
                i++;
                bool isInternalMut = false;
                bool isPrivate = false;
                std::string fromTemplate = "";
                if (tokens[i].getType() == tok_column) {
                    i++;
                    FPResult r = parseType(tokens, &i, templateArgs);
                    if (!r.success) {
                        errors.push_back(r);
                        continue;
                    }
                    fromTemplate = r.message;
                    type = r.value;
                    isInternalMut = typeIsReadonly(type);
                    isPrivate = contains<std::string>(nextAttributes, "private");
                    if (type == "none" || type == "nothing") {
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
                
                Variable& v = Variable::emptyVar();
                if (currentStruct->isStatic() || contains<std::string>(nextAttributes, "static")) {
                    v = Variable(currentStruct->getName() + "$" + name, type, currentStruct->getName());
                    v.name_token = new Token(nameToken);
                    v.isPrivate = (isPrivate || contains<std::string>(nextAttributes, "private"));
                    nextAttributes.clear();
                    globals.push_back(v);
                } else {
                    if (typeIsConst(type) && isInternalMut) {
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
                    v = Variable(name, type, currentStruct->getName());
                    v.name_token = new Token(nameToken);
                    v.typeFromTemplate = fromTemplate;
                    v.isPrivate = (isPrivate || contains<std::string>(nextAttributes, "private"));
                    currentStruct->addMember(v);
                    nextAttributes.clear();
                }

                lastDeclaredVariable = v;
            } else {

                auto validAttribute = [](Token& t) -> bool {
                    return t.getType()  == tok_string_literal ||
                           t.getValue() == "sinceVersion:" ||
                           t.getValue() == "replaceWith:" ||
                           t.getValue() == "deprecated!" ||
                           t.getValue() == "overload!" ||
                           t.getValue() == "construct" ||
                           t.getValue() == "intrinsic" ||
                           t.getValue() == "autoimpl" ||
                           t.getValue() == "restrict" ||
                           t.getValue() == "operator" ||
                           t.getValue() == "default" ||
                           t.getValue() == "private" ||
                           t.getValue() == "static" ||
                           t.getValue() == "export" ||
                           t.getValue() == "expect" ||
                           t.getValue() == "sealed" ||
                           t.getValue() == "unsafe" ||
                           t.getValue() == "cdecl" ||
                           t.getValue() == "final" ||
                           t.getValue() == "open" ||
                           t.getValue() == "asm";
                };

                if ((tokens[i].getValue() == "get" || tokens[i].getValue() == "set") && currentStruct != nullptr && currentFunction == nullptr) {
                    std::string varName = lastDeclaredVariable.getName();
                    if (tokens[i].getValue() == "get") {
                        Token& getToken = tokens[i];
                        i++;
                        expect("'('", tokens[i].getType() == tok_paren_open);
                        i++;
                        expect("')'", tokens[i].getType() == tok_paren_close);
                        i++;
                        expect("'=>'", tokens[i].getType() == tok_store);
                        if (tokens[i + 1].getType() == tok_identifier && tokens[i + 1].getValue() == "lambda") {
                            i++;
                        }
                        
                        std::string name = "get";
                        name += (char) std::toupper(varName[0]);
                        name += varName.substr(1);

                        Method* getter = new Method(currentStruct->getName(), name, getToken);
                        getter->setReturnType(lastDeclaredVariable.getType());
                        getter->addModifier("@getter");
                        getter->addModifier(varName);
                        getter->addArgument(Variable("self", "mut " + currentStruct->getName()));
                        currentFunction = getter;
                    } else {
                        Token& setToken = tokens[i];
                        i++;
                        expect("'('", tokens[i].getType() == tok_paren_open);
                        i++;
                        expect("identifier", tokens[i].getType() == tok_identifier);
                        std::string argName = tokens[i].getValue();
                        i++;
                        expect("')'", tokens[i].getType() == tok_paren_close);
                        i++;
                        expect("'=>'", tokens[i].getType() == tok_store);
                        if (tokens[i + 1].getType() == tok_identifier && tokens[i + 1].getValue() == "lambda") {
                            i++;
                        }

                        std::string name = "set";
                        name += (char) std::toupper(varName[0]);
                        name += varName.substr(1);

                        Method* setter = new Method(currentStruct->getName(), name, setToken);
                        setter->setReturnType("none");
                        setter->addModifier("@setter");
                        setter->addModifier(varName);
                        setter->addArgument(Variable(argName, lastDeclaredVariable.getType()));
                        setter->addArgument(Variable("self", "mut " + currentStruct->getName()));
                        currentFunction = setter;
                    }
                } else if (tokens[i].getValue() == "typealias") {
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
                } else if (tokens[i].getValue() == "deprecated!") {
                    i++;
                    if (tokens[i].getType() != tok_bracket_open) {
                        FPResult result;
                        result.message = "Expected '[', but got '" + tokens[i].getValue() + "'";
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
                    i++;
                    #define invalidKey(key, type) do {\
                        FPResult result; \
                        result.message = "Invalid key for '" + std::string(type) + "': '" + key + "'"; \
                        result.value = tokens[i].getValue(); \
                        result.line = tokens[i].getLine(); \
                        result.in = tokens[i].getFile(); \
                        result.ty ## pe = tokens[i].getType(); \
                        result.column = tokens[i].getColumn(); \
                        result.success = false; \
                        errors.push_back(result); \
                        continue; \
                    } while (0)

                    currentDeprecation[".name"] = "deprecated!";
                    
                    while (tokens[i].getType() != tok_bracket_close && i < tokens.size()) {
                        std::string key = tokens[i].getValue();
                        i++;

                        expect("':'", tokens[i].getType() == tok_column);
                        i++;

                        if (key == "since" || key == "replacement") {
                            expect("string", tokens[i].getType() == tok_string_literal);
                        } else if (key == "forRemoval") {
                            expect("boolean", tokens[i].getType() == tok_true || tokens[i].getType() == tok_false);
                        } else {
                            invalidKey(key, "deprecated!");
                        }
                        std::string value = tokens[i].getValue();
                        currentDeprecation[key] = value;
                        i++;
                        if (tokens[i].getType() == tok_comma) {
                            i++;
                        }
                    }

                    #undef invalidKey
                } else if (validAttribute(tokens[i])) {
                    if (tokens[i].getValue() == "construct" && currentStruct != nullptr) {
                        nextAttributes.push_back("private");
                        nextAttributes.push_back("static");
                    }
                    nextAttributes.push_back(tokens[i].getValue());
                    if (tokens[i].getValue() == "relocatable") {
                        nextAttributes.push_back("export");
                    }
                }
            }
        }

        for (Function* self : functions) {
            if (self->isMethod) {
                Function* f2 = self;
                Method* self = (Method*) f2;
                std::string name = self->getName();
                self->overloads.push_back(self);
                if (name.find("$$ol") != std::string::npos) {
                    name = name.substr(0, name.find("$$ol"));
                }
                for (Function* f : functions) {
                    if (f == self) continue;
                    if (!f->isMethod) continue;
                    if ((strstarts(f->getName(), name + "$$ol") || f->getName() == name) && ((Method*)f)->getMemberType() == self->getMemberType()) {
                        self->overloads.push_back(f);
                    }
                }
                continue;
            }
            std::string name = self->getName();
            self->overloads.push_back(self);
            if (name.find("$$ol") != std::string::npos) {
                name = name.substr(0, name.find("$$ol"));
            }
            for (Function* f : functions) {
                if (f == self) continue;
                if (f->isMethod) continue;
                if (strstarts(f->getName(), name + "$$ol") || f->getName() == name) {
                    self->overloads.push_back(f);
                }
            }
        }

        TPResult result;
        result.functions = functions;
        result.functions = functions;
        result.extern_globals = extern_globals;
        result.globals = globals;
        result.containers = containers;
        result.structs = structs;
        result.layouts = layouts;
        result.errors = errors;
        result.warns = warns;
        result.interfaces = interfaces;
        result.typealiases = typealiases;
        result.enums = enums;

        std::vector<Struct> newStructs;

        for (auto&& s : result.structs) {
            Struct super = getStructByName(result, s.extends());
            for (auto t : s.templates) {
                if (t.second.size() > 2 && t.second.front() == '[' && t.second.back() == ']') {
                    FPResult r;
                    r.message = "Default value for template '" + t.first + "' is an array, which is not allowed";
                    r.value = t.second;
                    r.line = s.getNameToken().getLine();
                    r.in = s.getNameToken().getFile();
                    r.type = s.getNameToken().getType();
                    r.column = s.getNameToken().getColumn();
                    r.column = s.getNameToken().getColumn();
                    r.success = false;
                    result.errors.push_back(r);
                    continue;
                }
                if (!isPrimitiveType(t.second) && getStructByName(result, t.second) == Struct::Null) {
                    FPResult r;
                    r.message = "Template '" + t.first + "' has default value of '" + t.second + "', which does not appear to be a struct or primitive type";
                    r.value = t.second;
                    r.line = s.getNameToken().getLine();
                    r.in = s.getNameToken().getFile();
                    r.type = s.getNameToken().getType();
                    r.column = s.getNameToken().getColumn();
                    r.column = s.getNameToken().getColumn();
                    r.success = false;
                    result.errors.push_back(r);
                    continue;
                }
            }
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
                if (super.isFinal()) {
                    FPResult r;
                    r.message = "Struct '" + super.getName() + "' is final";
                    r.value = s.nameToken().getValue();
                    r.line = s.nameToken().getLine();
                    r.in = s.nameToken().getFile();
                    r.type = s.nameToken().getType();
                    r.column = s.nameToken().getColumn();
                    r.column = s.nameToken().getColumn();
                    r.success = false;
                    result.errors.push_back(r);
                    FPResult r2;
                    r2.message = "Declared here:";
                    r2.value = super.nameToken().getValue();
                    r2.line = super.nameToken().getLine();
                    r2.in = super.nameToken().getFile();
                    r2.type = super.nameToken().getType();
                    r2.column = super.nameToken().getColumn();
                    r2.column = super.nameToken().getColumn();
                    r2.success = false;
                    r2.isNote = true;
                    result.errors.push_back(r2);
                    goto nextIter;
                }
                if (super.templates.size() && s.templates.size() == 0) {
                    for (auto kv : super.templates) {
                        s.addTemplateArgument(kv.first, kv.second);
                    }
                    s.required_typed_arguments = super.required_typed_arguments;
                }
                if (super.templates.size() && super.templates.size() != s.templates.size()) {
                    FPResult r;
                    r.message = "Struct '" + s.getName() + "' has " + std::to_string(s.templates.size()) + " template arguments, but '" + super.getName() + "' has " + std::to_string(super.templates.size());
                    r.value = s.nameToken().getValue();
                    r.line = s.nameToken().getLine();
                    r.in = s.nameToken().getFile();
                    r.type = s.nameToken().getType();
                    r.column = s.nameToken().getColumn();
                    r.column = s.nameToken().getColumn();
                    r.success = false;
                    result.errors.push_back(r);
                    goto nextIter;
                }
                std::vector<Variable> vars = super.getDefinedMembers();
                for (ssize_t i = vars.size() - 1; i >= 0; i--) {
                    s.addMember(vars[i], true);
                }
                oldSuper = super;
                super = getStructByName(result, super.extends());
            }
            newStructs.push_back(s);
            nextIter:;
        }
        result.structs = std::move(newStructs);
        if (result.errors.size()) {
            return result;
        }
        auto hasTypeAlias = [&](std::string name) -> bool {
            return result.typealiases.find(name) != result.typealiases.end();
        };
        auto createToStringMethod = [&](Struct& s) -> Method* {
            Token t(tok_identifier, "toString", 0, s.name_token.file);
            Method* toString = new Method(s.getName(), std::string("toString"), t);
            std::string stringify = s.getName() + " {";
            toString->setReturnType("str");
            toString->addModifier("<generated>");
            toString->addArgument(Variable("self", "mut " + s.getName()));
            toString->addToken(Token(tok_string_literal, stringify, 0, s.getName() + ":toString"));

            size_t membersAdded = 0;

            for (size_t i = 0; i < s.getMembers().size(); i++) {
                auto member = s.getMembers()[i];
                if (member.getName().front() == '$') continue;
                if (membersAdded) {
                    toString->addToken(Token(tok_string_literal, ", ", 0, s.getName() + ":toString"));
                    toString->addToken(Token(tok_identifier, "+", 0, s.getName() + ":toString"));
                }
                membersAdded++;

                toString->addToken(Token(tok_string_literal, member.getName() + ": ", 0, s.getName() + ":toString"));
                toString->addToken(Token(tok_identifier, "+", 0, s.getName() + ":toString"));
                toString->addToken(Token(tok_identifier, "self", 0, s.getName() + ":toString"));
                toString->addToken(Token(tok_dot, ".", 0, s.getName() + ":toString"));
                toString->addToken(Token(tok_identifier, member.getName(), 0, s.getName() + ":toString"));
                if (typeCanBeNil(member.getType())) {
                    toString->addToken(Token(tok_identifier, "builtinToString", 0, s.getName() + ":toString"));
                } else if (removeTypeModifiers(member.getType()) == "lambda" || strstarts(removeTypeModifiers(member.getType()), "lambda(") || hasTypeAlias(removeTypeModifiers(member.getType()))) {
                    toString->addToken(Token(tok_identifier, "any", 0, s.getName() + ":toString"));
                    toString->addToken(Token(tok_double_column, "::", 0, s.getName() + ":toString"));
                    toString->addToken(Token(tok_identifier, "toHexString", 0, s.getName() + ":toString"));
                } else {
                    toString->addToken(Token(tok_column, ":", 0, s.getName() + ":toString"));
                    toString->addToken(Token(tok_identifier, "toString", 0, s.getName() + ":toString"));
                }
                toString->addToken(Token(tok_identifier, "+", 0, s.getName() + ":toString"));
            }
            toString->addToken(Token(tok_string_literal, "}", 0, s.getName() + ":toString"));
            toString->addToken(Token(tok_identifier, "+", 0, s.getName() + ":toString"));
            toString->addToken(Token(tok_return, "return", 0, s.getName() + ":toString"));
            toString->forceAdd(true);
            return toString;
        };

        auto indexOfFirstMethodOnType = [&](std::string type) -> size_t {
            for (size_t i = 0; i < result.functions.size(); i++) {
                if (result.functions[i]->getMemberType() == type) return i;
            }
            return result.functions.size();
        };

        for (Struct s : result.structs) {
            if (s.isStatic()) continue;
            bool hasImplementedToString = false;
            Method* toString = getMethodByName(result, "toString", s.getName());
            if (toString == nullptr || contains<std::string>(toString->getModifiers(), "<generated>")) {
                size_t index = indexOfFirstMethodOnType(s.getName());
                result.functions.insert(result.functions.begin() + index, createToStringMethod(s));
                hasImplementedToString = true;
            }

            for (auto& toImplement : s.toImplementFunctions) {
                if (!hasImplementedToString && toImplement == "toString") {
                    result.functions.push_back(createToStringMethod(s));
                }
            }
        }
        return result;
    }

    std::pair<const std::string, std::string> emptyPair("", "");

    std::pair<const std::string, std::string>& pairAt(std::map<std::string, std::string> map, size_t index) {
        size_t i = 0;
        for (auto&& pair : map) {
            if (i == index) {
                return pair;
            }
            i++;
        }
        return emptyPair;
    }

    std::string specToCIdent(std::string in) {
        if (in.find("<") == std::string::npos) return in;
        // replace all < and > with $$
        std::string result = in;
        for (size_t i = 0; i < result.size(); i++) {
            if (result[i] == '<') {
                result[i] = '$';
                result.insert(i + 1, "$");
            } else if (result[i] == ',') {
                result[i] = '$';
            } else if (result[i] == '>') {
                result.erase(i, 1);
            }
        }
        return result;
    }
}
