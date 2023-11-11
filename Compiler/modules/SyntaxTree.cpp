#include <iostream>
#include <string>
#include <vector>
#include <optional>

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

    bool isSelfType(std::string type);

    Function* parseFunction(std::string name, Token& name_token, std::vector<FPResult>& errors, size_t& i, std::vector<Token>& tokens) {
        if (name == "=>") {
            if (tokens[i + 2].type == tok_bracket_open && tokens[i + 3].type == tok_bracket_close) {
                i += 2;
                name += "[]";
            }
        } else if (name == "[") {
            if (tokens[i + 2].type == tok_bracket_close) {
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
        else if (name == "=>") name = "operator$store";
        else if (name == "=>[]") name = "operator$set";
        else if (strcontains(name, "=>")) name = replaceAll(name, "=>", "operator$store");
        else if (name == "[]") name = "operator$get";
        else if (name == "?") name = "operator$wildcard";
        else if (name == "?:") name = "operator$elvis";

        Function* func = new Function(name, name_token);
        i += 2;
        if (tokens[i].type == tok_paren_open) {
            i++;
            while (i < tokens.size() && tokens[i].type != tok_paren_close) {
                std::string fromTemplate = "";
                if (tokens[i].type == tok_identifier || tokens[i].type == tok_column) {
                    std::string name = tokens[i].value;
                    std::string type = "any";
                    if (tokens[i].type == tok_column) {
                        name = "";
                    } else {
                        i++;
                    }
                    if (tokens[i].type == tok_column) {
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
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                    } else {
                        FPResult result;
                        result.message = "A type is required!";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
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
                } else if (tokens[i].type == tok_curly_open) {
                    std::vector<std::string> multi;
                    multi.reserve(10);
                    i++;
                    while (tokens[i].type != tok_curly_close) {
                        if (tokens[i].type == tok_comma) {
                            i++;
                        }
                        if (tokens[i].type != tok_identifier) {
                            FPResult result;
                            result.message = "Expected identifier for argument name, but got '" + tokens[i].value + "'";
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            errors.push_back(result);
                            i++;
                            continue;
                        }
                        std::string name = tokens[i].value;
                        multi.push_back(name);
                        i++;
                    }
                    i++;
                    std::string type;
                    if (tokens[i].type == tok_column) {
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
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                    } else {
                        FPResult result;
                        result.message = "A type is required!";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
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
                    result.message = "Expected identifier for argument name, but got '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    i++;
                    continue;
                }
                i++;
                if (tokens[i].type == tok_comma || tokens[i].type == tok_paren_close) {
                    if (tokens[i].type == tok_comma) {
                        i++;
                    }
                    continue;
                }
                FPResult result;
                result.message = "Expected ',' or ')', but got '" + tokens[i].value + "'";
                result.value = tokens[i].value;
                result.location = tokens[i].location;
                result.type = tokens[i].type;
                result.success = false;
                errors.push_back(result);
                continue;
            }
            i++;
            std::string namedReturn = "";
            if (tokens[i].type == tok_identifier) {
                namedReturn = tokens[i].value;
                i++;
            }
            if (tokens[i].type == tok_column) {
                i++;
                FPResult r = parseType(tokens, &i, templateArgs);
                if (!r.success) {
                    errors.push_back(r);
                    return func;
                }
                std::string type = r.value;
                std::string fromTemplate = r.message;
                func->return_type = type;
                func->templateArg = r.message;
                if (namedReturn.size()) {
                    func->namedReturnValue = Variable(namedReturn, type).also([fromTemplate](Variable& v) {
                        v.canBeNil = typeCanBeNil(v.type);
                        v.typeFromTemplate = fromTemplate;
                    });
                }
            } else {
                FPResult result;
                result.message = "A type is required!";
                result.value = tokens[i].value;
                result.location = tokens[i].location;
                result.type = tokens[i].type;
                result.success = false;
                errors.push_back(result);
                i++;
                return func;
            }
            if (isSelfType(func->return_type) && func->args.size() == 0) {
                FPResult result;
                result.message = "A function with 'self' return type must have at least one argument!";
                result.value = tokens[i].value;
                result.location = tokens[i].location;
                result.type = tokens[i].type;
                result.success = false;
                errors.push_back(result);
                return func;
            }
        } else {
            FPResult result;
            result.message = "Expected '(', but got '" + tokens[i].value + "'";
            result.value = tokens[i].value;
            result.location = tokens[i].location;
            result.type = tokens[i].type;
            result.success = false;
            errors.push_back(result);
            return func;
        }
        return func;
    }

    Method* parseMethod(std::string name, Token& name_token, std::string memberName, std::vector<FPResult>& errors, size_t& i, std::vector<Token>& tokens) {
        if (name == "=>") {
            if (tokens[i + 2].type == tok_bracket_open && tokens[i + 3].type == tok_bracket_close) {
                i += 2;
                name += "[]";
            }
        } else if (name == "[") {
            if (tokens[i + 2].type == tok_bracket_close) {
                i++;
                name += "]";
            }
        } else if (name == "*") {
            if (tokens[i + 2].type == tok_identifier) {
                i++;
                if (tokens[i + 1].value != memberName) {
                    FPResult result;
                    result.message = "Expected '" + memberName + "', but got '" + tokens[i + 1].value + "'";
                    result.value = tokens[i + 1].value;
                    result.location = tokens[i + 1].location;
                    result.type = tokens[i + 1].type;
                    result.success = false;
                    errors.push_back(result);
                    return nullptr;
                }
                name += tokens[i + 1].value;
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
        else if (name == "=>") name = "operator$store";
        else if (name == "=>[]") name = "operator$set";
        else if (name == "[]") name = "operator$get";
        else if (name == "?") name = "operator$wildcard";
        else if (name == "?:") name = "operator$elvis";

        Method* method = new Method(memberName, name, name_token);
        method->force_add = true;
        if (method->name == "init" || strstarts(method->name, "init")) {
            method->addModifier("<constructor>");
        }
        i += 2;
        bool memberByValue = false;
        if (tokens[i].type == tok_paren_open) {
            i++;
            if (tokens[i].type == tok_identifier && tokens[i].value == "*") {
                memberByValue = true;
                i++;
            }
            while (i < tokens.size() && tokens[i].type != tok_paren_close) {
                std::string fromTemplate = "";
                if (tokens[i].type == tok_identifier || tokens[i].type == tok_column) {
                    std::string name = tokens[i].value;
                    std::string type = "any";
                    if (tokens[i].type == tok_column) {
                        name = "";
                    } else {
                        i++;
                    }
                    if (tokens[i].type == tok_column) {
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
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                    } else {
                        FPResult result;
                        result.message = "A type is required!";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                        i++;
                        continue;
                    }
                    method->addArgument(Variable(name, type).also([fromTemplate](Variable& v) {
                        v.typeFromTemplate = fromTemplate;
                    }));
                } else if (tokens[i].type == tok_curly_open) {
                    std::vector<std::string> multi;
                    i++;
                    while (tokens[i].type != tok_curly_close) {
                        if (tokens[i].type == tok_comma) {
                            i++;
                        }
                        if (tokens[i].type != tok_identifier) {
                            FPResult result;
                            result.message = "Expected identifier for argument name, but got '" + tokens[i].value + "'";
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            errors.push_back(result);
                            i++;
                            continue;
                        }
                        std::string name = tokens[i].value;
                        multi.push_back(name);
                        i++;
                    }
                    i++;
                    std::string type;
                    if (tokens[i].type == tok_column) {
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
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                    } else {
                        FPResult result;
                        result.message = "A type is required!";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
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
                    result.message = "Expected identifier for method argument, but got '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    i++;
                    continue;
                }
                i++;
                if (tokens[i].type == tok_comma || tokens[i].type == tok_paren_close) {
                    if (tokens[i].type == tok_comma) {
                        i++;
                    }
                    continue;
                }
                FPResult result;
                result.message = "Expected ',' or ')', but got '" + tokens[i].value + "'";
                result.value = tokens[i].value;
                result.location = tokens[i].location;
                result.type = tokens[i].type;
                result.success = false;
                errors.push_back(result);
                continue;
            }
            i++;

            std::string namedReturn = "";
            if (tokens[i].type == tok_identifier) {
                namedReturn = tokens[i].value;
                i++;
            }
            if (tokens[i].type == tok_column) {
                i++;
                FPResult r = parseType(tokens, &i, templateArgs);
                if (!r.success) {
                    errors.push_back(r);
                    return method;
                }

                std::string fromTemplate = r.message;
                std::string type = r.value;
                method->return_type = type;
                method->templateArg = fromTemplate;
                if (namedReturn.size()) {
                    method->namedReturnValue = Variable(namedReturn, type).also([fromTemplate](Variable& v) {
                        v.typeFromTemplate = fromTemplate;
                    });
                }
            } else {
                FPResult result;
                result.message = "A type is required!";
                result.value = tokens[i].value;
                result.location = tokens[i].location;
                result.type = tokens[i].type;
                result.success = false;
                errors.push_back(result);
                i++;
                return method;
            }
            if (isSelfType(method->return_type) && method->args.size() == 0) {
                FPResult result;
                result.message = "A method with 'self' return type must have at least one argument!";
                result.value = tokens[i].value;
                result.location = tokens[i].location;
                result.type = tokens[i].type;
                result.success = false;
                errors.push_back(result);
                return method;
            }
            method->addArgument(Variable("self", memberByValue ? "*" + memberName : memberName));
        } else {
            FPResult result;
            result.message = "Expected '(', but got '" + tokens[i].value + "'";
            result.value = tokens[i].value;
            result.location = tokens[i].location;
            result.type = tokens[i].type;
            result.success = false;
            errors.push_back(result);
            return method;
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

    #define expect(_value, ...) if (!(__VA_ARGS__)) { \
            FPResult result; \
            result.message = "Expected " + std::string(_value) + ", but got '" + tokens[i].value + "'"; \
            result.va ## lue = tokens[i].value; \
            result.location = tokens[i].location; \
            result.type = tokens[i].type; \
            result.success = false; \
            errors.push_back(result); \
            break; \
        }

    #define expectr(_value, ...) if (!(__VA_ARGS__)) { \
            FPResult result; \
            result.message = "Expected " + std::string(_value) + ", but got '" + tokens[i].value + "'"; \
            result.va ## lue = tokens[i].value; \
            result.location = tokens[i].location; \
            result.type = tokens[i].type; \
            result.success = false; \
            errors.push_back(result); \
            return; \
        }

    TPResult parseFile(std::string file) {
        FILE* f = fopen(file.c_str(), "rb");

        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        if (!f) {
            FPResult result;
            result.message = "Could not open file '" + file + "'";
            result.success = false;
            result.location = SourceLocation(file, 0, 0);
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
            result.location = SourceLocation(file, 0, 0);
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
        if (v > *Main::version) {
            FPResult result;
            result.message = "This file was compiled with a newer version of the compiler!";
            result.success = false;
            result.location = SourceLocation(file, 0, 0);
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
                func = new Method(memberOf, name, Token(tok_identifier, name));
            } else {
                func = new Function(name, Token(tok_identifier, name));
            }

            func->return_type = returnType;

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

            if (isMethod && contains<std::string>(func->modifiers, "<virtual>")) {
                ((Method*) func)->force_add = true;
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
                    func->addToken(Token(type, value));
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

                Function* func = new Method(memberType, name, Token(tok_identifier, name));

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

    struct Instanciable {
        std::string structName;
        std::map<std::string, std::string> arguments;

        std::string toString() {
            std::string stringName = this->structName + "<";
            for (auto it = this->arguments.begin(); it != this->arguments.end(); it++) {
                if (it != this->arguments.begin()) {
                    stringName += ", ";
                }
                stringName += it->first + ": " + it->second;
            }
            stringName += ">";
            return stringName;
        }
    };

    std::vector<Instanciable> instanciables;

    std::optional<Instanciable> findInstanciable(std::string name) {
        for (auto&& inst : instanciables) {
            if (inst.structName == name) {
                return inst;
            }
        }
        return std::nullopt;
    }

    struct Template;

    struct TemplateInstances {
        static std::vector<Template> templates;
        
        static void addTemplate(const Template& t);
        static bool hasTemplate(std::string name);
        static Template& getTemplate(std::string name);
        static void instantiate(TPResult& result);
    };

    struct Template {
        static Template empty;

        std::string structName;
        Token instantiatingToken;
        std::map<std::string, std::string> arguments;

        void copyTo(const Struct& src, Struct& dest, TPResult& result) {
            dest.super = src.super;
            dest.members = src.members;
            dest.name_token = src.name_token;
            dest.flags = src.flags;
            dest.members = src.members;
            dest.memberInherited = src.memberInherited;
            dest.interfaces = src.interfaces;
            dest.templates = this->arguments;

            for (auto&& member : dest.members) {
                if (member.typeFromTemplate.size()) {
                    member.type = this->arguments[member.typeFromTemplate];
                }
            }

            auto methods = methodsOnType(result, src.name);
            for (auto&& method : methods) {
                Method* mt = method->cloneAs(dest.name);
                mt->clearArgs();
                for (Variable arg : method->args) {
                    if (arg.typeFromTemplate.size()) {
                        arg.type = this->arguments[arg.typeFromTemplate];
                    } else if (arg.name == "self") {
                        arg.type = dest.name;
                    }
                    mt->addArgument(arg);
                }
                if (mt->templateArg.size()) {
                    mt->return_type = this->arguments[mt->templateArg];
                }
                result.functions.push_back(mt);
            }
        }

        std::string& operator[](std::string key) {
            return this->arguments[key];
        }

        std::string& operator[](size_t index) {
            auto it = this->arguments.begin();
            for (size_t i = 0; i < index; i++) {
                it++;
            }
            return it->second;
        }

        void setNth(size_t index, std::string value) {
            std::string key = "";
            for (auto&& arg : this->arguments) {
                if (index == 0) {
                    key = arg.first;
                    break;
                }
                index--;
            }
            this->arguments[key] = value;
        }

        void makeInstance(TPResult& result, const Struct& baseStruct) {
            std::string instanceName = this->nameForInstance();
            Struct instance(instanceName);
            copyTo(baseStruct, instance, result);
            result.structs.push_back(instance);
        }

        std::string nameForInstance() {
            std::string instanceName = this->structName + "$";
            for (auto&& arg : this->arguments) {
                std::string type = arg.second;
                instanceName += "$" + type;
            }
            return instanceName;
        }

        std::string toString() {
            std::string stringName = this->structName + "<";
            for (auto it = this->arguments.begin(); it != this->arguments.end(); it++) {
                if (it != this->arguments.begin()) {
                    stringName += ", ";
                }
                stringName += it->first + ": " + it->second;
            }
            stringName += ">";
            return stringName;
        }

        std::optional<Template> parse(Template& t, std::vector<Token>& tokens, size_t& i, std::vector<FPResult>& errors) {
            i++;
            if (tokens[i].type != tok_identifier || tokens[i].value != "<") {
                i--;
                t = Template::empty;
                return std::nullopt;
            }
            t.instantiatingToken = tokens[i - 1];
            t.structName = tokens[i - 1].value;
            i++;
            size_t parameterCount = 0;
            while (tokens[i].value != ">") {
                auto inst = findInstanciable(tokens[i].value);
                if (inst.has_value()) {
                    Template t;
                    for (auto&& arg : inst->arguments) {
                        t.arguments[arg.first] = arg.second;
                    }
                    auto parsedArg = t.parse(t, tokens, i, errors);
                    if (!parsedArg.has_value()) {
                        FPResult result;
                        result.message = "Expected template argument, but got '" + tokens[i].value + "'";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                        t = Template::empty;
                        return Template::empty;
                    }
                    t.setNth(parameterCount++, parsedArg.value().nameForInstance());
                    i++;
                    if (tokens[i].type == tok_comma) {
                        i++;
                    }
                    continue;
                }
                FPResult r = parseType(tokens, &i, {});
                if (!r.success) {
                    errors.push_back(r);
                    t = Template::empty;
                    return Template::empty;
                }
                t.setNth(parameterCount++, r.value);
                i++;
                if (tokens[i].type == tok_comma) {
                    i++;
                }
            }
            return t;
        }
    };

    Template Template::empty = Template();

    std::vector<Template> TemplateInstances::templates;

    void TemplateInstances::addTemplate(const Template& t) {
        templates.push_back(t);
    }

    bool TemplateInstances::hasTemplate(std::string name) {
        for (Template& t : templates) {
            if (t.structName == name) {
                return true;
            }
        }
        return false;
    }

    Template& TemplateInstances::getTemplate(std::string name) {
        for (Template& t : templates) {
            if (t.structName == name) {
                return t;
            }
        }
        return Template::empty;
    }

    void TemplateInstances::instantiate(TPResult& result) {
        for (auto&& templateInstance : templates) {
            Struct& baseStruct = getStructByName(result, templateInstance.structName);
            bool hadError = false;
            if (!baseStruct.name.size()) {
                FPResult error;
                error.success = false;
                error.message = "Struct for template '" + templateInstance.toString() + "' not found!";
                error.location = templateInstance.instantiatingToken.location;
                error.type = templateInstance.instantiatingToken.type;
                error.value = templateInstance.instantiatingToken.value;
                result.errors.push_back(error);
                hadError = true;
            }
            if (!baseStruct.templates.size()) {
                FPResult error;
                error.success = false;
                error.message = "Struct '" + baseStruct.name + "' is not templatable!";
                error.location = templateInstance.instantiatingToken.location;
                error.type = templateInstance.instantiatingToken.type;
                error.value = templateInstance.instantiatingToken.value;
                result.errors.push_back(error);
                hadError = true;
            }
            if (baseStruct.templates.size() != templateInstance.arguments.size()) {
                FPResult error;
                error.success = false;
                error.message = "Expected " + std::to_string(baseStruct.templates.size()) + " template arguments, but got " + std::to_string(templateInstance.arguments.size());
                error.location = templateInstance.instantiatingToken.location;
                error.type = templateInstance.instantiatingToken.type;
                error.value = templateInstance.instantiatingToken.value;
                result.errors.push_back(error);
                hadError = true;
            }
            if (hadError) {
                continue;
            }
            if (getStructByName(result, templateInstance.nameForInstance()).name.size()) {
                continue;
            }
            templateInstance.makeInstance(result, baseStruct);
        }
    }

    struct MacroArg {
        std::string name;
        std::string type;

        MacroArg(std::string name, std::string type) {
            this->name = name;
            this->type = type;
        }
        MacroArg(const MacroArg& other) {
            this->name = other.name;
            this->type = other.type;
        }
        MacroArg(MacroArg&& other) {
            this->name = static_cast<std::string&&>(other.name);
            this->type = static_cast<std::string&&>(other.type);
        }
        MacroArg() {
            this->name = "";
            this->type = "";
        }
        MacroArg(std::string name) {
            this->name = name;
            this->type = "";
        }

        bool operator==(const MacroArg& other) const {
            return this->name == other.name && this->type == other.type;
        }
        bool operator!=(const MacroArg& other) const {
            return !(*this == other);
        }

        MacroArg& operator=(const MacroArg& other) {
            this->name = other.name;
            this->type = other.type;
            return *this;
        }
        MacroArg& operator=(MacroArg&& other) {
            this->name = static_cast<std::string&&>(other.name);
            this->type = static_cast<std::string&&>(other.type);
            return *this;
        }

        operator std::string() {
            return this->name;
        }
    };

    struct Macro {
        std::vector<MacroArg> args;
        std::vector<Token> tokens;

        virtual void expand(std::vector<Token>& otherTokens, size_t& i, std::unordered_map<std::string, Token>& args, std::vector<FPResult>& errors) {
            #define token this->tokens[j]
            #define nextToken this->tokens[j + 1]
            #define MacroError(msg) ({ \
                FPResult error; \
                error.success = false; \
                error.message = msg; \
                error.location = tokens[i].location; \
                error.type = tokens[i].type; \
                error.value = tokens[i].value; \
                error; \
            })

            struct stackframe {
                std::string name;
                void* value;
            };

            std::stack<stackframe> stack;
            std::unordered_map<std::string, stackframe> vars;
            for (size_t j = 0; j < this->tokens.size(); j++) {
                if (token.type == tok_identifier) {
                    if (token.value == "expand") {
                        j++;
                        if (token.type != tok_curly_open) {
                            errors.push_back(MacroError("Expected '{' after 'expand'"));
                            return;
                        }
                        j++;
                        size_t depth = 1;
                        std::vector<Token> expanded;
                        while (depth > 0) {
                            if (token.type == tok_curly_open) {
                                depth++;
                            } else if (token.type == tok_curly_close) {
                                depth--;
                                if (depth == 0) {
                                    break;
                                }
                            }
                            if (token.type == tok_dollar) {
                                if (nextToken.type != tok_identifier) {
                                    errors.push_back(MacroError("Unknown expansion token"));
                                } else if (args.find(nextToken.value) != args.end()) {
                                    expanded.push_back(args[nextToken.value]);
                                } else if (vars.find(nextToken.value) != vars.end()) {
                                    stackframe var = vars[nextToken.value];
                                    if (var.name == "Token") {
                                        expanded.push_back(*((Token*) var.value));
                                    } else if (var.name == "String") {
                                        expanded.push_back(Token(tok_string_literal, (char*) var.value));
                                    } else if (var.name == "Number") {
                                        expanded.push_back(Token(tok_number, std::to_string((long long) var.value)));
                                    } else {
                                        errors.push_back(MacroError("Expected Token, got " + var.name));
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
                    } else if (token.value == "concat") {
                        j++;
                        if (token.type != tok_curly_open) {
                            errors.push_back(MacroError("Expected '{' after 'expand'"));
                            return;
                        }
                        j++;
                        size_t depth = 1;
                        std::string str = "";
                        while (depth > 0) {
                            if (token.type == tok_curly_open) {
                                depth++;
                            } else if (token.type == tok_curly_close) {
                                depth--;
                                if (depth == 0) {
                                    break;
                                }
                            }
                            if (token.type == tok_dollar) {
                                if (nextToken.type != tok_identifier) {
                                    errors.push_back(MacroError("Unknown expansion token"));
                                } else if (args.find(nextToken.value) != args.end()) {
                                    str += args[nextToken.value].value;
                                } else if (vars.find(nextToken.value) != vars.end()) {
                                    stackframe var = vars[nextToken.value];
                                    if (var.name == "Token") {
                                        str += ((Token*) var.value)->value;
                                    } else if (var.name == "String") {
                                        str += (char*) var.value;
                                    } else if (var.name == "Number") {
                                        str += std::to_string((long long) var.value);
                                    } else {
                                        errors.push_back(MacroError("Expected Token, got " + var.name));
                                    }
                                } else {
                                    errors.push_back(MacroError("Expected identifier after '$'"));
                                }
                                j++;
                            } else {
                                str += token.value;
                            }
                            j++;
                        }
                        stack.push({"String", (void*) strdup(str.c_str())});
                    } else if (token.value == "peekAndConsume") {
                        Token* next = new Token(otherTokens[i]);
                        otherTokens.erase(otherTokens.begin() + i);
                        stack.push({"Token", (void*) next});
                    } else if (token.value == "peek") {
                        Token* next = new Token(otherTokens[i]);
                        stack.push({"Token", (void*) next});
                    } else if (token.value == "drop") {
                        stack.pop();
                    } else if (token.value == "consume") {
                        otherTokens.erase(otherTokens.begin() + i);
                    } else if (token.value == "dup") {
                        stackframe t = stack.top();
                        stack.push(t);
                    } else if (token.value == "swap") {
                        stackframe t1 = stack.top();
                        stack.pop();
                        stackframe t2 = stack.top();
                        stack.pop();
                        stack.push(t1);
                        stack.push(t2);
                    } else {
                        if (vars.find(token.value) != vars.end()) {
                            stack.push(vars[token.value]);
                        } else if (args.find(token.value) != args.end()) {
                            stack.push({"Token", (void*) new Token(args[token.value])});
                        } else {
                            errors.push_back(MacroError("Unknown variable '" + token.value + "'"));
                        }
                    }
                } else if (token.type == tok_number) {
                    long long n = parseNumber(token.value);
                    stack.push({"Number", (void*) n});
                } else if (token.type == tok_store) {
                    stackframe t = stack.top();
                    stack.pop();
                    if (nextToken.type != tok_identifier) {
                        errors.push_back(MacroError("Expected identifier after '=>'"));
                        return;
                    }
                    vars[nextToken.value] = t;
                } else if (token.type == tok_dot) {
                    stackframe t = stack.top();
                    stack.pop();
                    if (t.name == "Token") {
                        j++;
                        if (token.type != tok_identifier) {
                            errors.push_back(MacroError("Expected identifier after '.'"));
                            return;
                        }
                        if (t.value == nullptr) {
                            errors.push_back(MacroError("Expected Token, got nullptr"));
                            return;
                        }
                        Token* next = (Token*) t.value;
                        if (token.value == "type") {
                            stack.push({"Number", (void*) (long long) next->type});
                        } else if (token.value == "value") {
                            stack.push({"String", (void*) strdup((const char*) next->value.c_str())});
                        } else if (token.value == "line") {
                            stack.push({"Number", (void*) (long long) next->location.line});
                        } else if (token.value == "column") {
                            stack.push({"Number", (void*) (long long) next->location.column});
                        } else if (token.value == "file") {
                            stack.push({"String", (void*) strdup((const char*) next->location.file.c_str())});
                        } else {
                            errors.push_back(MacroError("Unknown Token property '" + token.value + "'"));
                            return;
                        }
                    } else {
                        errors.push_back(MacroError("Expected dereferenceable value, got " + t.name));
                        return;
                    }
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

        std::unordered_map<std::string, Macro*> macros;

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type != tok_identifier || (tokens[i].value != "macro!" && tokens[i].value != "delmacro!")) {
                continue;
            }
            if (tokens[i].value == "delmacro!") {
                i++;
                if (tokens[i].type != tok_identifier) {
                    errors.push_back(MacroError("Expected macro name"));
                    continue;
                }
                std::string name = tokens[i].value;
                if (macros.find(name) == macros.end()) {
                    errors.push_back(MacroError("Macro '" + name + "' not defined"));
                    continue;
                }
                macros.erase(name);
                continue;
            }
            size_t start = i;
            i++;
            if (tokens[i].type != tok_identifier) {
                errors.push_back(MacroError("Expected macro name"));
                continue;
            }
            std::string name = tokens[i].value;
            i++;
            Macro* macro = new Macro();
            if (tokens[i].type == tok_paren_open) {
                i++;
                while (tokens[i].type != tok_paren_close) {
                    macro->args.push_back(tokens[i].value);
                    i++;
                    if (tokens[i].type == tok_column) {
                        i++;
                        macro->args.back().type = tokens[i].value;
                        i++;
                    }
                    if (tokens[i].type == tok_comma) {
                        i++;
                    }
                }
            } else {
                errors.push_back(MacroError("Expected '(' after macro name"));
                continue;
            }
            i++;
            if (tokens[i].type != tok_curly_open) {
                errors.push_back(MacroError("Expected '{' after macro name"));
                continue;
            }
            i++;
            ssize_t depth = 1;
            while (depth > 0) {
                if (tokens[i].type == tok_curly_open) {
                    depth++;
                } else if (tokens[i].type == tok_curly_close) {
                    depth--;
                }
                if (depth <= 0) {
                    i++;
                    break;
                }
                macro->tokens.push_back(tokens[i]);
                i++;
            }
            // delete macro declaration
            tokens.erase(tokens.begin() + start, tokens.begin() + i);
            i = start;
            i--;
            if (macros.find(name) != macros.end()) {
                errors.push_back(MacroError("Macro '" + name + "' already defined"));
                continue;
            }
            macros[name] = macro;
        }

        for (size_t i = 0; i < tokens.size(); i++) {
            if (
                tokens[i].type != tok_identifier ||
                macros.find(tokens[i].value) == macros.end() ||
                i + 1 >= tokens.size() ||
                tokens[i + 1].value != "!" ||
                tokens[i + 1].location.line != tokens[i].location.line
            ) {
                continue;
            }
            Macro* macro = macros[tokens[i].value];
            std::unordered_map<std::string, Token> args;
            size_t start = i;
            i += 2;
            for (size_t j = 0; j < macro->args.size(); j++) {
                args[macro->args[j]] = tokens[i];
                i++;
            }
            // delete macro call
            tokens.erase(tokens.begin() + start, tokens.begin() + i);
            i = start;
            macro->expand(tokens, i, args, errors);
            i--;
        }

        // Builtins
        if (!Main::options::noScaleFramework) {
            Function* builtinIsInstanceOf = new Function("builtinIsInstanceOf", Token(tok_identifier, "builtinIsInstanceOf"));
            builtinIsInstanceOf->addModifier("expect");
            builtinIsInstanceOf->addModifier("foreign");

            builtinIsInstanceOf->addArgument(Variable("obj", "any"));
            builtinIsInstanceOf->addArgument(Variable("typeStr", "str"));
            
            builtinIsInstanceOf->return_type = "int";
            functions.push_back(builtinIsInstanceOf);

            Function* builtinHash = new Function("builtinHash", Token(tok_identifier, "builtinHash"));
            builtinHash->addModifier("expect");
            builtinHash->addModifier("cdecl");
            builtinHash->addModifier("type_id");
            
            builtinHash->addArgument(Variable("data", "[int8]"));
            
            builtinHash->return_type = "uint";
            functions.push_back(builtinHash);

            Function* builtinIdentityHash = new Function("builtinIdentityHash", Token(tok_identifier, "builtinIdentityHash"));
            builtinIdentityHash->addModifier("expect");
            builtinIdentityHash->addModifier("cdecl");
            builtinIdentityHash->addModifier("_scl_identity_hash");
            
            builtinIdentityHash->addArgument(Variable("obj", "any"));
            
            builtinIdentityHash->return_type = "int";
            functions.push_back(builtinIdentityHash);

            Function* builtinAtomicClone = new Function("builtinAtomicClone", Token(tok_identifier, "builtinAtomicClone"));
            builtinAtomicClone->addModifier("expect");
            builtinAtomicClone->addModifier("cdecl");
            builtinAtomicClone->addModifier("_scl_atomic_clone");
            
            builtinAtomicClone->addArgument(Variable("obj", "any"));
            
            builtinAtomicClone->return_type = "any";
            functions.push_back(builtinAtomicClone);

            Function* builtinTypeEquals = new Function("builtinTypeEquals", Token(tok_identifier, "builtinTypeEquals"));
            builtinTypeEquals->addModifier("expect");
            builtinTypeEquals->addModifier("cdecl");
            builtinTypeEquals->addModifier("_scl_is_instance_of");

            builtinTypeEquals->addArgument(Variable("obj", "any"));
            builtinTypeEquals->addArgument(Variable("type_id", "int32"));

            builtinTypeEquals->return_type = "int";
            functions.push_back(builtinTypeEquals);

            Function* builtinIsInstance = new Function("builtinIsInstance", Token(tok_identifier, "builtinIsInstance"));
            builtinIsInstance->addModifier("expect");
            builtinIsInstance->addModifier("cdecl");
            builtinIsInstance->addModifier("_scl_is_instance");

            builtinIsInstance->addArgument(Variable("obj", "any"));

            builtinIsInstance->return_type = "int";
            functions.push_back(builtinIsInstance);

            if (!Main::options::noScaleFramework) {
                Function* builtinToString = new Function("builtinToString", Token(tok_identifier, "builtinToString"));
                builtinToString->addModifier("expect");
                builtinToString->addModifier("foreign");
                
                builtinToString->addArgument(Variable("val", "any"));

                builtinToString->return_type = "str";

                // if val is SclObject then val as SclObject:toString return else val int::toString return fi
                builtinToString->addToken(Token(tok_if, "if"));
                builtinToString->addToken(Token(tok_identifier, "val"));
                builtinToString->addToken(Token(tok_identifier, "builtinIsInstance"));
                builtinToString->addToken(Token(tok_then, "then"));
                builtinToString->addToken(Token(tok_identifier, "val"));
                builtinToString->addToken(Token(tok_as, "as"));
                builtinToString->addToken(Token(tok_identifier, "SclObject"));
                builtinToString->addToken(Token(tok_column, ":"));
                builtinToString->addToken(Token(tok_identifier, "toString"));
                builtinToString->addToken(Token(tok_return, "return"));
                builtinToString->addToken(Token(tok_fi, "fi"));
                builtinToString->addToken(Token(tok_identifier, "val"));
                builtinToString->addToken(Token(tok_identifier, "int"));
                builtinToString->addToken(Token(tok_double_column, "::"));
                builtinToString->addToken(Token(tok_identifier, "toString"));
                builtinToString->addToken(Token(tok_return, "return"));

                functions.push_back(builtinToString);
            }

            Function* builtinUnreachable = new Function("builtinUnreachable", Token(tok_identifier, "builtinUnreachable"));
            builtinUnreachable->addModifier("expect");
            builtinUnreachable->addModifier("foreign");
            builtinUnreachable->return_type = "none";

            functions.push_back(builtinUnreachable);
        }

        auto findFunctionByName = [&](std::string name) {
            for (size_t i = 0; i < functions.size(); i++) {
                if (functions[i]->isMethod) {
                    continue;
                }
                if (functions[i]->name == name) {
                    return functions[i];
                }
            }
            for (size_t i = 0; i < functions.size(); i++) {
                if (functions[i]->isMethod) {
                    continue;
                }
                if (functions[i]->name == name) {
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
                if (functions[i]->name == name && ((Method*) functions[i])->member_type == memberType) {
                    return (Method*) functions[i];
                }
            }
            for (size_t i = 0; i < functions.size(); i++) {
                if (!functions[i]->isMethod) {
                    continue;
                }
                if (functions[i]->name == name && ((Method*) functions[i])->member_type == memberType) {
                    return (Method*) functions[i];
                }
            }
            return (Method*) nullptr;
        };
        
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type != tok_struct_def) {
                continue;
            }

            Instanciable inst;
            i++;
            if (tokens[i].type == tok_identifier) {
                inst.structName = tokens[i].value;
            } else {
                FPResult result;
                result.message = "Expected identifier after 'struct'";
                result.value = tokens[i - 1].value;
                result.location = tokens[i - 1].location;
                result.type = tokens[i - 1].type;
                result.success = false;
                errors.push_back(result);
                continue;
            }
            i++;
            if (tokens[i].type != tok_identifier || tokens[i].value != "<") {
                continue;
            }
            i++;
            while (i < tokens.size() && tokens[i].value != ">") {
                if (tokens[i].type != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier after '<'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    break;
                }
                std::string parameterName = tokens[i].value;
                i++;
                if (tokens[i].type != tok_column) {
                    FPResult result;
                    result.message = "Expected ':' after '" + parameterName + "' (" + parameterName + ")";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    break;
                }
                i++;
                FPResult result = parseType(tokens, &i);
                if (!result.success) {
                    errors.push_back(result);
                    break;
                }
                i++;
                inst.arguments[parameterName] = result.value;
                if (i < tokens.size() && tokens[i].value == ",") {
                    i++;
                }
            }
            if (i >= tokens.size()) {
                FPResult result;
                result.message = "Expected '>' after struct parameters";
                result.value = tokens[i - 1].value;
                result.location.line = tokens[i - 1].location.line;
                result.location = tokens[i - 1].location;
                result.type = tokens[i - 1].type;
                result.success = false;
                errors.push_back(result);
                continue;
            }
            i++;
            instanciables.push_back(inst);
        }

        for (auto&& inst : instanciables) {
            for (size_t i = 0; i < tokens.size(); i++) {
                if (i && (tokens[i - 1].type == tok_struct_def || tokens[i - 1].type == tok_dot)) {
                    continue;
                }
                if (tokens[i].type != tok_identifier || tokens[i].value != inst.structName) {
                    continue;
                }
                size_t start = i;
                Template templ;
                for (auto&& arg : inst.arguments) {
                    templ.arguments[arg.first] = arg.second;
                }
                auto t = templ.parse(templ, tokens, i, errors);
                if (!t.has_value()) {
                    FPResult result;
                    result.message = "Usage of uninstantiated template '" + inst.structName + "' is not allowed";
                    result.value = tokens[start].value;
                    result.location.line = tokens[start].location.line;
                    result.location = tokens[start].location;
                    result.type = tokens[start].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                tokens.erase(tokens.begin() + start, tokens.begin() + i);
                i = start;
                tokens.insert(tokens.begin() + i, Token(tok_identifier, templ.nameForInstance(), tokens[i].location));
                TemplateInstances::addTemplate(t.value());
            }
        }

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type != tok_identifier || tokens[i].value != ">") {
                continue;
            }
            if (i + 1 >= tokens.size()) {
                continue;
            }
            if (tokens[i + 1].type != tok_identifier || tokens[i + 1].value != ">") {
                continue;
            }
            if (tokens[i + 1].location.column - 1 != tokens[i].location.column) {
                continue;
            }
            Token begin = tokens[i];
            tokens.erase(tokens.begin() + i);
            tokens.erase(tokens.begin() + i);
            if (i < tokens.size() && tokens[i].type == tok_identifier && tokens[i].value == ">") {
                if (tokens[i].location.column - 2 == tokens[i].location.column) {
                    tokens.erase(tokens.begin() + i);
                    tokens.insert(tokens.begin() + i, Token(tok_identifier, ">>>", begin.location));
                }
            } else {
                tokens.insert(tokens.begin() + i, Token(tok_identifier, ">>", begin.location));
            }
        }

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type == tok_eof) {
                tokens.erase(tokens.begin() + i);
                i--;
            }
        }

        for (size_t i = 0; i < tokens.size(); i++) {
            Token& token = tokens[i];

            if (token.type == tok_function) {
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define function inside another function. Current function: " + currentFunction->name;
                    result.value = tokens[i + 1].value;
                    result.location = tokens[i + 1].location;
                    result.type = tokens[i + 1].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define function inside of a container. Current container: " + currentContainer->name;
                    result.value = tokens[i + 1].value;
                    result.location = tokens[i + 1].location;
                    result.type = tokens[i + 1].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentStruct != nullptr) {
                    if (tokens[i + 2].type == tok_column) {
                        std::string name = tokens[i + 1].value;
                        if (name != currentStruct->name) {
                            FPResult result;
                            result.message = "Cannot define method on different struct inside of a struct. Current struct: " + currentStruct->name;
                            result.value = tokens[i + 1].value;
                            result.location = tokens[i + 1].location;
                            result.type = tokens[i + 1].type;
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        i += 2;
                    }
                    if (contains<std::string>(nextAttributes, "static") || currentStruct->isStatic()) {
                        std::string name = tokens[i + 1].value;
                        Token& func = tokens[i + 1];
                        currentFunction = parseFunction(currentStruct->name + "$" + name, func, errors, i, tokens);
                        currentFunction->deprecated = currentDeprecation;
                        currentDeprecation.clear();
                        currentFunction->member_type = currentStruct->name;
                        for (std::string& s : nextAttributes) {
                            currentFunction->addModifier(s);
                        }
                        Function* f = findFunctionByName(currentFunction->name);
                        if (f) {
                            currentFunction->name = currentFunction->name + "$$ol" + argsToRTSignatureIdent(currentFunction);
                        }
                        if (currentFunction->has_expect) {
                            functions.push_back(currentFunction);
                            currentFunction = nullptr;
                        }
                        if (currentFunction && currentFunction->has_intrinsic) {
                            if (contains<std::string>(intrinsics, "F:" + currentFunction->name)) {
                                currentFunction->addModifier("foreign");
                                currentFunction->addModifier("expect");
                                functions.push_back(currentFunction);
                            }
                        }
                        nextAttributes.clear();
                        continue;
                    }
                    Token& func = tokens[i + 1];
                    std::string name = func.value;
                    currentFunction = parseMethod(name, func, currentStruct->name, errors, i, tokens);
                    for (std::string& s : nextAttributes) {
                        currentFunction->addModifier(s);
                    }
                    Function* f = findMethodByName(currentFunction->name, currentFunction->member_type);
                    if (f) {
                        currentFunction->name = currentFunction->name + "$$ol" + argsToRTSignatureIdent(currentFunction);
                    }
                    if (currentFunction->has_expect) {
                        functions.push_back(currentFunction);
                        currentFunction = nullptr;
                    }
                    if (currentFunction && currentFunction->has_intrinsic) {
                        if (contains<std::string>(intrinsics, "M:" + ((Method*) currentFunction)->member_type + "::" + currentFunction->name)) {
                            currentFunction->addModifier("foreign");
                            currentFunction->addModifier("expect");
                            functions.push_back(currentFunction);
                        }
                    }
                    nextAttributes.clear();
                    continue;
                }
                if (currentInterface != nullptr) {
                    if (contains<std::string>(nextAttributes, "default")) {
                        Token& func = tokens[i + 1];
                        std::string name = func.value;
                        currentFunction = parseMethod(name, func, "", errors, i, tokens);
                        for (std::string& s : nextAttributes) {
                            currentFunction->addModifier(s);
                        }
                        currentInterface->addToImplement(currentFunction);
                        Method* m = new Method(currentInterface->name, name, func);
                        for (Variable& v : currentFunction->args) {
                            m->addArgument(v);
                        }
                        m->addArgument(Variable("self", currentInterface->name));
                        m->return_type = currentFunction->return_type;
                        for (std::string& s : currentFunction->modifiers) {
                            m->addModifier(s);
                        }
                        functions.push_back(m);
                    } else {
                        std::string name = tokens[i + 1].value;
                        Token& func = tokens[i + 1];
                        Function* functionToImplement = parseFunction(name, func, errors, i, tokens);
                        functionToImplement->deprecated = currentDeprecation;
                        currentDeprecation.clear();
                        currentInterface->addToImplement(functionToImplement);

                        Method* m = new Method(currentInterface->name, name, func);
                        for (Variable& v : functionToImplement->args) {
                            m->addArgument(v);
                        }
                        m->addArgument(Variable("self", currentInterface->name));
                        m->return_type = functionToImplement->return_type;
                        for (std::string& s : functionToImplement->modifiers) {
                            m->addModifier(s);
                        }
                        functions.push_back(m);
                    }
                    nextAttributes.clear();
                    continue;
                }
                i++;
                if (tokens[i + 1].type == tok_column) {
                    std::string member_type = tokens[i].value;
                    i++;
                    Token& func = tokens[i + 1];
                    std::string name = func.value;
                    currentFunction = parseMethod(name, func, member_type, errors, i, tokens);
                    if (contains<std::string>(nextAttributes, "private")) {
                        FPResult result;
                        result.message = "Methods cannot be declared 'private' if they are not in the struct body!";
                        result.value = func.value;
                        result.location.line = func.location.line;
                        result.location = func.location;
                        result.type = func.type;
                        result.success = false;
                        errors.push_back(result);
                        nextAttributes.clear();
                        continue;
                    }
                } else {
                    if (tokens[i + 1].type == tok_double_column) {
                        std::string memberType = tokens[i].value;
                        i++;
                        std::string name = tokens[i + 1].value;
                        Token& func = tokens[i + 1];
                        currentFunction = parseFunction(memberType + "$" + name, func, errors, i, tokens);
                        currentFunction->deprecated = currentDeprecation;
                        currentDeprecation.clear();
                        currentFunction->addModifier("static");
                    } else {
                        i--;
                        std::string name = tokens[i + 1].value;
                        Token& func = tokens[i + 1];
                        currentFunction = parseFunction(name, func, errors, i, tokens);
                        currentFunction->deprecated = currentDeprecation;
                        currentDeprecation.clear();
                    }
                }
                for (std::string& s : nextAttributes) {
                    currentFunction->addModifier(s);
                }
                Function* f;
                if (currentFunction->isMethod) {
                    f = findMethodByName(currentFunction->name, ((Method*) currentFunction)->member_type);
                } else {
                    f = findFunctionByName(currentFunction->name);
                }
                if (f) {
                    currentFunction->name = currentFunction->name + "$$ol" + argsToRTSignatureIdent(currentFunction);
                }
                if (currentFunction->has_expect) {
                    functions.push_back(currentFunction);
                    currentFunction = nullptr;
                }
                if (currentFunction && currentFunction->has_intrinsic) {
                    if (contains<std::string>(intrinsics, "F:" + currentFunction->name)) {
                        currentFunction->addModifier("foreign");
                        currentFunction->addModifier("expect");
                        functions.push_back(currentFunction);
                    }
                }
                nextAttributes.clear();
            } else if (token.type == tok_end) {
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
                            f = findMethodByName(currentFunction->name, ((Method*) currentFunction)->member_type);
                        } else {
                            f = findFunctionByName(currentFunction->name);
                        }
                        if (f) {
                            if (f->isCVarArgs()) {
                                FPResult result;
                                result.message = "Cannot overload varargs function " + f->name + "!";
                                result.value = f->name;
                                result.location.line = f->name_token.location.line;
                                result.location = f->name_token.location;
                                result.type = f->name_token.type;
                                result.success = false;
                                errors.push_back(result);
                                nextAttributes.clear();
                                currentFunction = nullptr;
                                continue;
                            } else if (!currentFunction->has_intrinsic) {
                                if (currentFunction->args == f->args) {
                                    FPResult result;
                                    result.message = "Function " + currentFunction->name + " is already defined!";
                                    result.value = currentFunction->name;
                                    result.location.line = currentFunction->name_token.location.line;
                                    result.location = currentFunction->name_token.location;
                                    result.type = currentFunction->name_token.type;
                                    result.success = false;
                                    errors.push_back(result);
                                    nextAttributes.clear();
                                    currentFunction = nullptr;
                                    continue;
                                }
                                currentFunction->name = currentFunction->name + "$$ol" + argsToRTSignatureIdent(currentFunction);
                                functionWasOverloaded = true;
                            } else if (currentFunction != f) {
                                if (currentFunction && currentFunction->has_intrinsic) {
                                    FPResult result;
                                    result.message = "Cannot overload intrinsic function " + currentFunction->name + "!";
                                    result.value = currentFunction->name;
                                    result.location.line = currentFunction->name_token.location.line;
                                    result.location = currentFunction->name_token.location;
                                    result.type = currentFunction->name_token.type;
                                    result.success = false;
                                    errors.push_back(result);
                                } else if (f->has_intrinsic) {
                                    FPResult result;
                                    result.message = "Cannot overload function " + currentFunction->name + " with intrinsic function!";
                                    result.value = currentFunction->name;
                                    result.location.line = currentFunction->name_token.location.line;
                                    result.location = currentFunction->name_token.location;
                                    result.type = currentFunction->name_token.type;
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
                            result.value = currentFunction->name_token.value;
                            result.location.line = currentFunction->name_token.location.line;
                            result.location = currentFunction->name_token.location;
                            result.type = currentFunction->name_token.type;
                            result.success = false;
                            errors.push_back(result);
                        }
                    } else {
                        if (functionWasOverloaded || !contains<Function*>(functions, currentFunction))
                            functions.push_back(currentFunction);
                    }
                    currentFunction = nullptr;
                } else if (currentContainer != nullptr) {
                    if (std::find(containers.begin(), containers.end(), *currentContainer) == containers.end()) {
                        containers.push_back(*currentContainer);
                    }
                    currentContainer = nullptr;
                } else if (currentStruct != nullptr) {
                    if (std::find(structs.begin(), structs.end(), *currentStruct) == structs.end()) {
                        structs.push_back(*currentStruct);
                    }
                    templateArgs.clear();
                    currentStruct = nullptr;
                } else if (currentInterface != nullptr) {
                    if (std::find(interfaces.begin(), interfaces.end(), currentInterface) == interfaces.end()) {
                        interfaces.push_back(currentInterface);
                    }
                    interfaces.push_back(currentInterface);
                    currentInterface = nullptr;
                } else {
                    FPResult result;
                    result.message = "Unexpected 'end' keyword outside of function, container, struct or interface body. Remove this:";
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
            } else if (token.type == tok_container_def) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a container inside another container. Maybe you forgot an 'end' somewhere? Current container: " + currentContainer->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a container inside of a function. Maybe you forgot an 'end' somewhere? Current function: " + currentFunction->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentStruct != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a container inside of a struct. Maybe you forgot an 'end' somewhere? Current struct: " + currentStruct->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentInterface != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a container inside of an interface. Maybe you forgot an 'end' somewhere? Current interface: " + currentInterface->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                if (tokens[i].type != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier for container name, but got '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (tokens[i].value == "str" || tokens[i].value == "int" || tokens[i].value == "float" || tokens[i].value == "none" || tokens[i].value == "nothing" || tokens[i].value == "any" || isPrimitiveIntegerType(tokens[i].value)) {
                    FPResult result;
                    result.message = "Invalid name for container: '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                FPResult result;
                result.message = "Containers are deprecated. Use a static struct instead.";
                result.value = tokens[i].value;
                result.location = tokens[i].location;
                result.type = tokens[i].type;
                result.success = false;
                warns.push_back(result);
                currentContainer = new Container(tokens[i].value);
                currentContainer->name_token = new Token(tokens[i]);
            } else if (token.type == tok_union_def) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a union struct inside of a container. Maybe you forgot an 'end' somewhere? Current container: " + currentContainer->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a union struct inside of a function. Maybe you forgot an 'end' somewhere? Current function: " + currentFunction->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentStruct != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a union struct inside another struct. Maybe you forgot an 'end' somewhere? Current struct: " + currentStruct->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentInterface != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a union struct inside of an interface. Maybe you forgot an 'end' somewhere? Current interface: " + currentInterface->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (tokens[i + 1].type != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier for union struct name, but got '" + tokens[i + 1].value + "'";
                    result.value = tokens[i + 1].value;
                    result.location = tokens[i + 1].location;
                    result.type = tokens[i + 1].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                currentStruct = new Struct(tokens[i].value, tokens[i]);
                for (std::string& m : nextAttributes) {
                    currentStruct->addModifier(m);
                }
                nextAttributes.clear();
                currentStruct->super = "Union";
                i++;

                auto makeGetter = [](const Variable& v, const std::string& unionName, int n) {
                    const std::string& varName = v.name;
                    std::string name = "get";
                    name += (char) std::toupper(varName[0]);
                    name += varName.substr(1);

                    Method* getter = new Method(unionName, name, *(v.name_token));
                    getter->return_type = v.type;
                    getter->addModifier("@getter");
                    getter->addModifier(varName);
                    getter->addArgument(Variable("self", unionName));
                    getter->addToken(Token(tok_number, std::to_string(n), v.name_token->location));
                    getter->addToken(Token(tok_identifier, "self", v.name_token->location));
                    getter->addToken(Token(tok_column, ":", v.name_token->location));
                    getter->addToken(Token(tok_identifier, "expected", v.name_token->location));
                    std::string removed = removeTypeModifiers(v.type);
                    if (removed == "none" || removed == "nothing") {
                        return getter;
                    }
                    getter->addToken(Token(tok_identifier, "self", v.name_token->location));
                    getter->addToken(Token(tok_dot, ".", v.name_token->location));
                    getter->addToken(Token(tok_identifier, "__value", v.name_token->location));
                    getter->addToken(Token(tok_as, "as", v.name_token->location));
                    getter->addToken(Token(tok_identifier, removeTypeModifiers(v.type), v.name_token->location));
                    getter->addToken(Token(tok_return, "return", v.name_token->location));
                    return getter;
                };

                auto makeSetter = [](const Variable& v, const std::string& unionName, int n) {
                    Function* setter = new Function(unionName + "$" + v.name, *(v.name_token));
                    setter->member_type = unionName;
                    setter->return_type = unionName;
                    setter->addModifier("static");
                    std::string removed = removeTypeModifiers(v.type);
                    if (removed != "none" && removed != "nothing") {
                        setter->addArgument(Variable("what", v.type));
                    }
                    setter->addToken(Token(tok_number, std::to_string(n), v.name_token->location));
                    if (removed != "none" && removed != "nothing") {
                        setter->addToken(Token(tok_identifier, "what", v.name_token->location));
                    } else {
                        setter->addToken(Token(tok_nil, "nil", v.name_token->location));
                    }
                    setter->addToken(Token(tok_identifier, unionName, v.name_token->location));
                    setter->addToken(Token(tok_double_column, "::", v.name_token->location));
                    setter->addToken(Token(tok_identifier, "new", v.name_token->location));
                    setter->addToken(Token(tok_return, "return", v.name_token->location));
                    return setter;
                };

                while (tokens[i].type != tok_end) {
                    if (tokens[i].type != tok_identifier) {
                        FPResult result;
                        result.message = "Expected identifier for union struct member name, but got '" + tokens[i].value + "'";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                        i++;
                        continue;
                    }
                    std::string name = tokens[i].value;
                    size_t start = i;
                    i++;
                    std::string type;
                    if (tokens[i].type == tok_column) {
                        i++;
                        FPResult result = parseType(tokens, &i);
                        if (!result.success) {
                            errors.push_back(result);
                            i++;
                            break;
                        }
                        i++;
                        type = result.value;
                    } else {
                        type = name;
                    }
                    Variable v = Variable(name, type);
                    v.name_token = new Token(tokens[start]);
                    v.isConst = true;
                    v.isVirtual = true;
                    currentStruct->addMember(v);
                    Method* getter = makeGetter(v, currentStruct->name, currentStruct->members.size());
                    if (currentStruct->isExtern()) {
                        getter->addModifier("expect");
                    }
                    functions.push_back(getter);
                    Function* setter = makeSetter(v, currentStruct->name, currentStruct->members.size());
                    if (currentStruct->isExtern()) {
                        setter->addModifier("expect");
                    }
                    functions.push_back(setter);
                }
                if (tokens[i].type != tok_end) {
                    FPResult result;
                    result.message = "Expected 'end' for union struct, but got '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                currentStruct->toggleFinal();
                if (std::find(structs.begin(), structs.end(), *currentStruct) == structs.end()) {
                    structs.push_back(*currentStruct);
                }
                currentStruct = nullptr;
            } else if (token.type == tok_struct_def && (i == 0 || (((((long) i) - 1) >= 0) && tokens[i - 1].type != tok_double_column))) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a struct inside of a container. Maybe you forgot an 'end' somewhere? Current container: " + currentContainer->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a struct inside of a function. Maybe you forgot an 'end' somewhere? Current function: " + currentFunction->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentStruct != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a struct inside another struct. Maybe you forgot an 'end' somewhere? Current struct: " + currentStruct->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentInterface != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a struct inside of an interface. Maybe you forgot an 'end' somewhere? Current interface: " + currentInterface->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (tokens[i + 1].type != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier for struct name, but got '" + tokens[i + 1].value + "'";
                    result.value = tokens[i + 1].value;
                    result.location = tokens[i + 1].location;
                    result.type = tokens[i + 1].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                if (tokens[i].value == "none") {
                    FPResult result;
                    result.message = "Invalid name for struct: '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                currentStruct = new Struct(tokens[i].value, tokens[i]);
                for (std::string& m : nextAttributes) {
                    currentStruct->addModifier(m);
                }
                bool open = contains<std::string>(nextAttributes, "open");
                
                nextAttributes.clear();
                bool hasSuperSpecified = false;
                if (tokens[i + 1].value == "<") {
                    i++;
                    // *i = "<"
                    i++;
                    // *i = first template argument
                    while (tokens[i].value != ">") {
                        if (tokens[i].type != tok_identifier) {
                            FPResult result;
                            result.message = "Expected identifier for template argument name, but got '" + tokens[i].value + "'";
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            errors.push_back(result);
                            break;
                        }
                        std::string key = tokens[i].value;
                        i++;
                        // *i = ":"
                        if (tokens[i].type != tok_column) {
                            FPResult result;
                            result.message = "Expected ':' after template argument name, but got '" + tokens[i].value + "'";
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
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
                        templateArgs[key] = value.value;
                        i++;
                        if (tokens[i].value == ",") {
                            i++;
                        }
                    }
                }
                if (tokens[i + 1].type == tok_column) {
                    i += 2;
                    if (currentStruct->name == tokens[i].value) {
                        FPResult result;
                        result.message = "Structs cannot extend themselves.!";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                    } else {
                        if (tokens[i].value == "SclObject" && !Main::options::noScaleFramework) {
                            FPResult result;
                            result.message = "Explicit inherit of struct 'SclObject' not neccessary.";
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = true;
                            warns.push_back(result);
                        }
                        currentStruct->super = (tokens[i].value);
                        hasSuperSpecified = true;
                    }
                }

                if (!hasSuperSpecified) {
                    if (currentStruct->name != "SclObject" && !Main::options::noScaleFramework)
                        currentStruct->super = ("SclObject");
                }
                if (tokens[i + 1].type == tok_is) {
                    i++;
                    do {
                        i++;
                        if (tokens[i].type != tok_identifier) {
                            FPResult result;
                            result.message = "Expected identifier for interface name, but got'" + tokens[i].value + "'";
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            errors.push_back(result);
                            i++;
                            continue;
                        }
                        currentStruct->implement(tokens[i].value);
                        i++;
                    } while (i < tokens.size() && tokens[i].type == tok_comma);
                    i--;
                }

                if (open) {
                    if (std::find(structs.begin(), structs.end(), *currentStruct) == structs.end()) {
                        structs.push_back(*currentStruct);
                    }
                    currentStruct = nullptr;
                }
            } else if (token.type == tok_identifier && token.value == "layout") {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a layout inside of a container. Maybe you forgot an 'end' somewhere? Current container: " + currentContainer->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a layout inside of a function. Maybe you forgot an 'end' somewhere? Current function: " + currentFunction->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentStruct != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a layout inside of a struct. Maybe you forgot an 'end' somewhere? Current struct: " + currentStruct->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentInterface != nullptr) {
                    FPResult result;
                    result.message = "Cannot define a layout inside of an interface. Maybe you forgot an 'end' somewhere? Current interface: " + currentInterface->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                std::string name = tokens[i].value;
                i++;
                Layout layout(name);

                while (tokens[i].type != tok_end) {
                    if (tokens[i].type != tok_declare) {
                        FPResult result;
                        result.message = "Only declarations are allowed inside of a layout.";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                        break;
                    }
                    i++;
                    if (tokens[i].type != tok_identifier) {
                        FPResult result;
                        result.message = "Expected identifier for variable name, but got '" + tokens[i].value + "'";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.success = false;
                        errors.push_back(result);
                        break;
                    }
                    std::string name = tokens[i].value;
                    std::string type = "any";
                    i++;
                    if (tokens[i].type == tok_column) {
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
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            errors.push_back(result);
                            break;
                        }
                    }
                    
                    layout.addMember(Variable(name, type));
                    i++;
                }

                if (std::find(layouts.begin(), layouts.end(), layout) == layouts.end()) {
                    layouts.push_back(layout);
                }
            } else if (token.type == tok_enum) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an enum inside of a container. Maybe you forgot an 'end' somewhere? Current container: " + currentContainer->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an enum inside of a function. Maybe you forgot an 'end' somewhere? Current function: " + currentFunction->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentStruct != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an enum inside of a struct. Maybe you forgot an 'end' somewhere? Current struct: " + currentStruct->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentInterface != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an enum inside of an interface. Maybe you forgot an 'end' somewhere? Current interface: " + currentInterface->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                if (tokens[i].type != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier for enum name, but got '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                std::string name = tokens[i].value;
                i++;
                Enum e = Enum(name);
                e.name_token = new Token(tokens[i - 1]);
                while (tokens[i].type != tok_end) {
                    long next = e.nextValue;
                    if (tokens[i].type == tok_bracket_open) {
                        i++;
                        expect("number", tokens[i].type == tok_number);
                        next = parseNumber(tokens[i].value);
                        i++;
                        expect("]", tokens[i].type == tok_bracket_close);
                        i++;
                    }
                    if (tokens[i].type != tok_identifier) {
                        FPResult result;
                        result.message = "Expected identifier for enum member, but got '" + tokens[i].value + "'";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                        i++;
                        continue;
                    }
                    try {
                        e.addMember(tokens[i].value, next);
                    } catch (const std::runtime_error& e) {
                        FPResult result;
                        result.message = e.what();
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                    }
                    i++;
                }
                enums.push_back(e);
            } else if (token.type == tok_interface_def) {
                if (currentContainer != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an interface inside of a container. Maybe you forgot an 'end' somewhere? Current container: " + currentContainer->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an interface inside of a function. Maybe you forgot an 'end' somewhere? Current function: " + currentFunction->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentStruct != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an interface inside of a struct. Maybe you forgot an 'end' somewhere? Current struct: " + currentStruct->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (currentInterface != nullptr) {
                    FPResult result;
                    result.message = "Cannot define an interface inside another interface. Maybe you forgot an 'end' somewhere? Current interface: " + currentInterface->name;
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                if (tokens[i + 1].type != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier for interface declaration, but got '" + tokens[i + 1].value + "'";
                    result.value = tokens[i + 1].value;
                    result.location = tokens[i + 1].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                if (tokens[i].value == "str" || tokens[i].value == "int" || tokens[i].value == "float" || tokens[i].value == "none" || tokens[i].value == "any" || isPrimitiveIntegerType(tokens[i].value)) {
                    FPResult result;
                    result.message = "Invalid name for interface: '" + tokens[i + 1].value + "'";
                    result.value = tokens[i + 1].value;
                    result.location = tokens[i + 1].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                currentInterface = new Interface(tokens[i].value);
                currentInterface->name_token = new Token(tokens[i]);
            } else if (token.type == tok_addr_of) {
                if (currentFunction == nullptr) {
                    if (tokens[i + 1].type == tok_identifier) {
                        nextAttributes.push_back(tokens[i + 1].value);
                        if (tokens[i + 1].value == "relocatable") {
                            nextAttributes.push_back("export");
                        }
                    } else {
                        FPResult result;
                        result.message = "'" + tokens[i + 1].value + "' is not a valid modifier.";
                        result.value = tokens[i + 1].value;
                        result.location = tokens[i + 1].location;
                        result.type = tokens[i + 1].type;
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
                    token.type == tok_identifier &&
                    token.value == "lambda" &&
                    (((ssize_t) i) - 3 >= 0 && tokens[i - 3].type != tok_declare) &&
                    (((ssize_t) i) - 1 >= 0 && tokens[i - 1].type != tok_as) &&
                    (((ssize_t) i) - 1 >= 0 && tokens[i - 1].type != tok_bracket_open) &&
                    (((ssize_t) i) - 2 >= 0 && tokens[i - 2].type != tok_new)
                    // (((ssize_t) i) - 1 >= 0 && tokens[i - 1].type != tok_comma) &&
                    // (((ssize_t) i) - 1 >= 0 && tokens[i - 1].type != tok_paren_open)
                ) isInLambda++;
                if (token.type == tok_identifier && token.value == "unsafe") {
                    isInUnsafe++;
                }
                if (!contains<Function*>(functions, currentFunction))
                    currentFunction->addToken(token);
            } else if (token.type == tok_declare && currentContainer == nullptr && currentStruct == nullptr) {
                if (tokens[i + 1].type != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier for variable name, but got '" + tokens[i + 1].value + "'";
                    result.value = tokens[i + 1].value;
                    result.location = tokens[i + 1].location;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                std::string name = tokens[i].value;
                std::string type = "any";
                i++;
                if (tokens[i].type == tok_column) {
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
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                }
                if (contains<std::string>(nextAttributes, "expect")) {
                    Variable v(name, type);
                    if (std::find(extern_globals.begin(), extern_globals.end(), v) == extern_globals.end()) {
                        extern_globals.push_back(v);
                    }
                } else {
                    Variable v(name, type);
                    if (std::find(globals.begin(), globals.end(), v) == globals.end()) {
                        globals.push_back(v);
                    }
                }
                nextAttributes.clear();
            } else if (token.type == tok_declare && currentContainer != nullptr) {
                if (tokens[i + 1].type != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier for variable name, but got '" + tokens[i + 1].value + "'";
                    result.value = tokens[i + 1].value;
                    result.location = tokens[i + 1].location;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                std::string name = tokens[i].value;
                std::string type = "any";
                i++;
                if (tokens[i].type == tok_column) {
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
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                }
                nextAttributes.clear();
                currentContainer->addMember(Variable(name, type));
            } else if (token.type == tok_declare && currentStruct != nullptr) {
                if (tokens[i + 1].type != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier for variable name, but got '" + tokens[i + 1].value + "'";
                    result.value = tokens[i + 1].value;
                    result.location = tokens[i + 1].location;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                Token& name_token = tokens[i];
                std::string name = tokens[i].value;
                std::string type = "any";
                i++;
                bool isInternalMut = false;
                bool isPrivate = false;
                std::string fromTemplate = "";
                if (tokens[i].type == tok_column) {
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
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                }
                
                Variable& v = Variable::emptyVar();
                if (currentStruct->isStatic() || contains<std::string>(nextAttributes, "static")) {
                    v = Variable(currentStruct->name + "$" + name, type, currentStruct->name);
                    v.name_token = new Token(name_token);
                    v.isPrivate = (isPrivate || contains<std::string>(nextAttributes, "private"));
                    nextAttributes.clear();
                    if (contains<std::string>(nextAttributes, "expect")) {
                        if (std::find(extern_globals.begin(), extern_globals.end(), v) == extern_globals.end()) {
                            extern_globals.push_back(v);
                        }
                    } else {
                        if (std::find(globals.begin(), globals.end(), v) == globals.end()) {
                            globals.push_back(v);
                        }
                    }
                } else {
                    if (typeIsConst(type) && isInternalMut) {
                        FPResult result;
                        result.message = "The 'const' and 'readonly' modifiers are mutually exclusive!";
                        result.value = name_token.value;
                        result.location = name_token.location;
                        result.type = name_token.type;
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    v = Variable(name, type, currentStruct->name);
                    v.name_token = new Token(name_token);
                    v.typeFromTemplate = fromTemplate;
                    v.isPrivate = (isPrivate || contains<std::string>(nextAttributes, "private"));
                    v.isVirtual = contains<std::string>(nextAttributes, "virtual");
                    currentStruct->addMember(v);
                    nextAttributes.clear();
                }

                lastDeclaredVariable = v;
            } else {

                auto validAttribute = [](Token& t) -> bool {
                    return t.type  == tok_string_literal ||
                           t.value == "sinceVersion:" ||
                           t.value == "replaceWith:" ||
                           t.value == "deprecated!" ||
                           t.value == "overload!" ||
                           t.value == "construct" ||
                           t.value == "intrinsic" ||
                           t.value == "overrides" ||
                           t.value == "stacksize" ||
                           t.value == "autoimpl" ||
                           t.value == "restrict" ||
                           t.value == "operator" ||
                           t.value == "foreign" ||
                           t.value == "virtual" ||
                           t.value == "default" ||
                           t.value == "private" ||
                           t.value == "static" ||
                           t.value == "export" ||
                           t.value == "expect" ||
                           t.value == "sealed" ||
                           t.value == "unsafe" ||
                           t.value == "cdecl" ||
                           t.value == "final" ||
                           t.value == "open" ||
                           t.value == "asm";
                };

                if ((tokens[i].value == "get" || tokens[i].value == "set") && currentStruct != nullptr && currentFunction == nullptr) {
                    std::string varName = lastDeclaredVariable.name;
                    if (tokens[i].value == "get") {
                        Token& getToken = tokens[i];
                        i++;
                        expect("'('", tokens[i].type == tok_paren_open);
                        i++;
                        expect("')'", tokens[i].type == tok_paren_close);
                        if (tokens[i].type == tok_store) {
                            FPResult result;
                            result.message = "Ignored '=>' in getter declaration.";
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            warns.push_back(result);
                            i++;
                        }
                        if (tokens[i + 1].type == tok_identifier && tokens[i + 1].value == "lambda") {
                            i++;
                        }
                        
                        std::string name = "get";
                        name += (char) std::toupper(varName[0]);
                        name += varName.substr(1);

                        Method* getter = new Method(currentStruct->name, name, getToken);
                        getter->return_type = lastDeclaredVariable.type;
                        getter->addModifier("@getter");
                        getter->addModifier(varName);
                        getter->addArgument(Variable("self", currentStruct->name));
                        currentFunction = getter;
                    } else {
                        Token& setToken = tokens[i];
                        i++;
                        expect("'('", tokens[i].type == tok_paren_open);
                        i++;
                        expect("identifier", tokens[i].type == tok_identifier);
                        std::string argName = tokens[i].value;
                        i++;
                        expect("')'", tokens[i].type == tok_paren_close);
                        if (tokens[i].type == tok_store) {
                            FPResult result;
                            result.message = "Ignored '=>' in setter declaration.";
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            warns.push_back(result);
                            i++;
                        }
                        if (tokens[i + 1].type == tok_identifier && tokens[i + 1].value == "lambda") {
                            i++;
                        }

                        std::string name = "set";
                        name += (char) std::toupper(varName[0]);
                        name += varName.substr(1);

                        Method* setter = new Method(currentStruct->name, name, setToken);
                        setter->return_type = "none";
                        setter->addModifier("@setter");
                        setter->addModifier(varName);
                        setter->addArgument(Variable(argName, lastDeclaredVariable.type));
                        setter->addArgument(Variable("self", currentStruct->name));
                        currentFunction = setter;
                    }
                } else if (tokens[i].value == "typealias") {
                    i++;
                    std::string type = tokens[i].value;
                    i++;
                    if (tokens[i].type != tok_string_literal) {
                        FPResult result;
                        result.message = "Expected string, but got '" + tokens[i].value + "'";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    std::string replacement = tokens[i].value;
                    typealiases[type] = replacement;
                } else if (tokens[i].value == "deprecated!") {
                    i++;
                    if (tokens[i].type != tok_bracket_open) {
                        FPResult result;
                        result.message = "Expected '[', but got '" + tokens[i].value + "'";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    i++;
                    #define invalidKey(key, _type) do {\
                        FPResult result; \
                        result.message = "Invalid key for '" + std::string(_type) + "': '" + key + "'"; \
                        result.value = tokens[i].value; \
                        result.location = tokens[i].location; \
                        result.type = tokens[i].type; \
                        result.success = false; \
                        errors.push_back(result); \
                        continue; \
                    } while (0)

                    currentDeprecation[".name"] = "deprecated!";
                    
                    while (tokens[i].type != tok_bracket_close && i < tokens.size()) {
                        std::string key = tokens[i].value;
                        i++;

                        expect("':'", tokens[i].type == tok_column);
                        i++;

                        if (key == "since" || key == "replacement") {
                            expect("string", tokens[i].type == tok_string_literal);
                        } else if (key == "forRemoval") {
                            expect("boolean", tokens[i].type == tok_true || tokens[i].type == tok_false);
                        } else {
                            invalidKey(key, "deprecated!");
                        }
                        std::string value = tokens[i].value;
                        currentDeprecation[key] = value;
                        i++;
                        if (tokens[i].type == tok_comma) {
                            i++;
                        }
                    }

                    #undef invalidKey
                } else if (validAttribute(tokens[i])) {
                    if (tokens[i].value == "construct" && currentStruct != nullptr) {
                        nextAttributes.push_back("private");
                        nextAttributes.push_back("static");
                    }
                    nextAttributes.push_back(tokens[i].value);
                    if (tokens[i].value == "stackalloc") {
                        i++;
                        if (i < tokens.size() && tokens[i].type == tok_number) {
                            nextAttributes.push_back(tokens[i].value);
                        } else {
                            FPResult result;
                            result.message = "Expected number, but got '" + tokens[i].value + "'";
                            result.value = tokens[i].value;
                            result.location = tokens[i].location;
                            result.type = tokens[i].type;
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                    } else if (tokens[i].value == "relocatable") {
                        nextAttributes.push_back("export");
                    }
                }
            }
        }

        for (Function* self : functions) {
            if (self->isMethod) {
                Function* f2 = self;
                Method* self = (Method*) f2;
                std::string name = self->name;
                self->overloads.push_back(self);
                if (name.find("$$ol") != std::string::npos) {
                    name = name.substr(0, name.find("$$ol"));
                }
                for (Function* f : functions) {
                    if (f == self) continue;
                    if (!f->isMethod) continue;
                    if ((strstarts(f->name, name + "$$ol") || f->name == name) && ((Method*)f)->member_type == self->member_type) {
                        self->overloads.push_back(f);
                    }
                }
                continue;
            }
            std::string name = self->name;
            self->overloads.push_back(self);
            if (name.find("$$ol") != std::string::npos) {
                name = name.substr(0, name.find("$$ol"));
            }
            for (Function* f : functions) {
                if (f == self) continue;
                if (f->isMethod) continue;
                if (strstarts(f->name, name + "$$ol") || f->name == name) {
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

        TemplateInstances::instantiate(result);

        for (auto&& s : result.structs) {
            Struct super = getStructByName(result, s.super);
            Struct oldSuper = s;
            while (super.name.size()) {
                if (super.name == s.name) {
                    FPResult r;
                    r.message = "Struct '" + s.name + "' has Struct '" + oldSuper.name + "' in it's inheritance chain, which inherits from '" + super.name + "'";
                    r.value = s.name_token.value;
                    r.location.line = s.name_token.location.line;
                    r.location = s.name_token.location;
                    r.type = s.name_token.type;
                    r.success = false;
                    result.errors.push_back(r);
                    FPResult r2;
                    r2.message = "Struct '" + oldSuper.name + "' declared here:";
                    r2.value = oldSuper.name_token.value;
                    r2.location.line = oldSuper.name_token.location.line;
                    r2.location = oldSuper.name_token.location;
                    r2.type = oldSuper.name_token.type;
                    r2.success = false;
                    r2.isNote = true;
                    result.errors.push_back(r2);
                    break;
                }
                if (super.isFinal()) {
                    FPResult r;
                    r.message = "Struct '" + super.name + "' is final";
                    r.value = s.name_token.value;
                    r.location.line = s.name_token.location.line;
                    r.location = s.name_token.location;
                    r.type = s.name_token.type;
                    r.success = false;
                    result.errors.push_back(r);
                    FPResult r2;
                    r2.message = "Declared here:";
                    r2.value = super.name_token.value;
                    r2.location.line = super.name_token.location.line;
                    r2.location = super.name_token.location;
                    r2.type = super.name_token.type;
                    r2.success = false;
                    r2.isNote = true;
                    result.errors.push_back(r2);
                    goto nextIter;
                }
                std::vector<Variable> vars = super.getDefinedMembers();
                for (ssize_t i = vars.size() - 1; i >= 0; i--) {
                    s.addMember(vars[i], true);
                }
                std::vector<Variable> newMembers;
                newMembers.reserve(s.members.size());
                for (Variable mem : s.members) {
                    if (mem.isInternalMut) {
                        mem.internalMutableFrom = s.name;
                    }
                    newMembers.push_back(mem);
                }
                s.members = static_cast<std::vector<Variable>&&>(newMembers);
                oldSuper = super;
                super = getStructByName(result, super.super);
            }
            newStructs.push_back(s);
            nextIter:;
        }
        result.structs = static_cast<std::vector<sclc::Struct>&&>(newStructs);
        if (result.errors.size()) {
            return result;
        }
        auto hasTypeAlias = [&](std::string name) -> bool {
            return result.typealiases.find(name) != result.typealiases.end();
        };
        auto isPointer = [](std::string s) -> bool {
            return (s.size() > 2 && s.front() == '[' && s.back() == ']');
        };
        auto createToStringMethod = [&](Struct& s) -> Method* {
            Token t(tok_identifier, "toString");
            Method* toString = new Method(s.name, std::string("toString"), t);
            std::string stringify = s.name + " {";
            toString->return_type = "str";
            toString->addModifier("<generated>");
            toString->addArgument(Variable("self", s.name));
            toString->addToken(Token(tok_string_literal, stringify));

            size_t membersAdded = 0;

            for (size_t i = 0; i < s.members.size(); i++) {
                auto member = s.members[i];
                if (member.name.front() == '$') continue;
                if (membersAdded) {
                    toString->addToken(Token(tok_string_literal, ", "));
                    toString->addToken(Token(tok_identifier, "+"));
                }
                membersAdded++;

                toString->addToken(Token(tok_string_literal, member.name + ": "));
                toString->addToken(Token(tok_identifier, "+"));
                toString->addToken(Token(tok_identifier, "self"));
                toString->addToken(Token(tok_dot, "."));
                toString->addToken(Token(tok_identifier, member.name));
                bool canBeNil = typeCanBeNil(member.type);
                std::string type = removeTypeModifiers(member.type);
                if (canBeNil) {
                    toString->addToken(Token(tok_identifier, "builtinToString"));
                } else if (isPointer(type) || hasTypeAlias(type) || strstarts(type, "lambda(") || type == "lambda") {
                    toString->addToken(Token(tok_identifier, "any"));
                    toString->addToken(Token(tok_double_column, "::"));
                    toString->addToken(Token(tok_identifier, "toHexString"));
                } else {
                    toString->addToken(Token(tok_column, ":"));
                    toString->addToken(Token(tok_identifier, "toString"));
                }
                toString->addToken(Token(tok_identifier, "+"));
            }
            toString->addToken(Token(tok_string_literal, "}"));
            toString->addToken(Token(tok_identifier, "+"));
            toString->addToken(Token(tok_return, "return"));
            toString->force_add = true;
            return toString;
        };

        auto indexOfFirstMethodOnType = [&](std::string type) -> size_t {
            for (size_t i = 0; i < result.functions.size(); i++) {
                if (result.functions[i]->member_type == type) return i;
            }
            return result.functions.size();
        };

        for (Struct s : result.structs) {
            if (s.isStatic()) continue;
            bool hasImplementedToString = false;
            Method* toString = getMethodByName(result, "toString", s.name);
            if (toString == nullptr || contains<std::string>(toString->modifiers, "<generated>")) {
                size_t index = indexOfFirstMethodOnType(s.name);
                result.functions.insert(result.functions.begin() + index, createToStringMethod(s));
                hasImplementedToString = true;
            }

            for (auto& toImplement : s.toImplementFunctions) {
                if (!hasImplementedToString && toImplement == "toString") {
                    result.functions.push_back(createToStringMethod(s));
                }
            }
        }

        if (Main::options::Werror) {
            for (auto warn : result.warns) {
                result.errors.push_back(warn);
            }
            result.warns.clear();
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
