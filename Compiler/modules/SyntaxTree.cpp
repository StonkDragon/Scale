#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <list>
#include <array>

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
    std::map<std::string, Token> templateArgs;

    extern std::unordered_map<std::string, std::vector<std::string>> usingStructs;

    std::string typeToRTSigIdent(std::string type);
    std::string argsToRTSignatureIdent(Function* f);

    bool isSelfType(std::string type);

    std::vector<std::string> parseReifiedParams(std::vector<FPResult>& errors, size_t& i, std::vector<Token>& tokens) {
        std::vector<std::string> params;
        if (tokens[i].type != tok_identifier || tokens[i].value != "<") {
            return params;
        }
        i++;
        while (tokens[i].type != tok_identifier || tokens[i].value != ">") {
            if (tokens[i].type == tok_comma || (tokens[i].type == tok_identifier && tokens[i].value == ">")) {
                params.push_back("");
            } else {
                FPResult r = parseType(tokens, &i);
                if (!r.success) {
                    errors.push_back(r);
                    return params;
                }
                params.push_back(r.value);
                i++;
            }
            if (tokens[i].type == tok_comma) {
                i++;
                if (tokens[i].type == tok_identifier && tokens[i].value == ">") {
                    params.push_back("");
                }
            }
        }
        i++;
        return params;
    }

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
        if (tokens[i].type == tok_identifier && tokens[i].value == "<") {
            func->reified_parameters = parseReifiedParams(errors, i, tokens);
        }
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
            if (isSelfType(func->return_type) && func->args.empty()) {
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
        } else if (name == "@") {
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
        if (tokens[i].type == tok_identifier && tokens[i].value == "<") {
            method->reified_parameters = parseReifiedParams(errors, i, tokens);
        }
        bool memberByValue = false;
        if (tokens[i].type == tok_paren_open) {
            i++;
            if (tokens[i].type == tok_addr_of) {
                memberByValue = true;
                i++;
                if (tokens[i].type == tok_comma) {
                    i++;
                }
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
                    if (type == "varargs" && name.size()) {
                        method->addArgument(Variable(name + "$size", "const int"));
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
            if (isSelfType(method->return_type) && method->args.empty()) {
                FPResult result;
                result.message = "A method with 'self' return type must have at least one argument!";
                result.value = tokens[i].value;
                result.location = tokens[i].location;
                result.type = tokens[i].type;
                result.success = false;
                errors.push_back(result);
                return method;
            }
            method->addArgument(Variable("self", memberByValue ? "@" + memberName : memberName));
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

    SyntaxTree::SyntaxTree(std::vector<Token>& tokens)  {
        this->tokens = tokens;
    }

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

    std::string operator ""_s(const char* str, size_t len) {
        return std::string(str, len);
    }

    std::string operator ""_s(char c) {
        return std::string(1, c);
    }

    std::string specToCIdent(std::string in);

    struct Instanciable {
        std::string structName;
        std::map<std::string, Token> arguments;

        std::string toString() {
            std::string stringName = this->structName + "<";
            for (auto it = this->arguments.begin(); it != this->arguments.end(); it++) {
                if (it != this->arguments.begin()) {
                    stringName += ", ";
                }
                stringName += it->first + ": " + it->second.uncolored_formatted();
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

    const std::array<std::string, 3> removableTypeModifiers = {"mut ", "const ", "readonly "};

    std::string reparseArgType(std::string type, const std::map<std::string, Token>& templateArgs) {
        std::string mods = "";
        bool isVal = type.front() == '@';
        if (isVal) {
            type = type.substr(1);
        }
        for (std::string mod : removableTypeModifiers) {
            if (strstarts(type, mod)) {
                type = type.substr(mod.size());
                if (mod == "mut") {
                    mods += "mut ";
                } else if (mod == "const") {
                    mods += "const ";
                } else if (mod == "readonly") {
                    mods += "readonly ";
                }
            }
        }
        if (isVal) {
            return "@" + mods + reparseArgType(type.substr(1), templateArgs);
        } else if (type.front() == '[') {
            std::string inner = type.substr(1, type.size() - 2);
            return "[" + reparseArgType(inner, templateArgs) + "]";
        }
        if (strstarts(type, "lambda(")) {
            std::string lt = type.substr(0, type.find(':'));
            std::string ret = type.substr(type.find(':') + 1);
            type = lt + ":" + reparseArgType(ret, templateArgs);
        }
        if (templateArgs.find(type) != templateArgs.end()) {
            return mods + templateArgs.at(type).value;
        }
        return mods + type;
    }

    std::vector<Token> parseString(std::string s);

    struct Template {
        static Template empty;

        std::string structName;
        Token instantiatingToken;
        std::map<std::string, Token> arguments;

        void copyTo(const Struct& src, Struct& dest, TPResult& result) {
            dest.super = src.super;
            dest.members = src.members;
            dest.name_token = src.name_token;
            dest.flags = src.flags;
            dest.members = src.members;
            dest.memberInherited = src.memberInherited;
            dest.interfaces = src.interfaces;
            dest.templates = this->arguments;
            dest.templateInstance = false;

            for (auto&& member : dest.members) {
                member.type = reparseArgType(member.type, this->arguments);
            }

            auto methods = methodsOnType(result, src.name);
            for (auto&& method : methods) {
                Method* mt = method->cloneAs(dest.name);
                mt->clearArgs();
                for (Variable arg : method->args) {
                    if (arg.name == "self") {
                        arg.type = dest.name;
                    } else {
                        arg.type = reparseArgType(arg.type, this->arguments);
                    }
                    mt->addArgument(arg);
                }
                mt->return_type = reparseArgType(mt->return_type, this->arguments);
                for (size_t i = 0; i < mt->body.size(); i++) {
                    if (mt->body[i].type != tok_identifier) continue;
                    if (this->arguments.find(mt->body[i].value) != this->arguments.end()) {
                        mt->body[i] = this->arguments.at(mt->body[i].value);
                    }
                }
                result.functions.push_back(mt);
            }
        }

        Token& operator[](std::string key) {
            return this->arguments[key];
        }

        Token& operator[](size_t index) {
            auto it = this->arguments.begin();
            for (size_t i = 0; i < index; i++) {
                it++;
            }
            return it->second;
        }

        void setNth(size_t index, Token value) {
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
            std::string instanceName = this->structName + "$$b";
            for (auto it = this->arguments.begin(); it != this->arguments.end(); it++) {
                if (it != this->arguments.begin()) {
                    instanceName += "$$n";
                }
                if (it->second.type == tok_identifier)
                    instanceName += it->second.value;
            }
            return instanceName + "$$e";
        }

        const std::string toString() const {
            std::string stringName = this->structName + "<";
            for (auto it = this->arguments.begin(); it != this->arguments.end(); it++) {
                if (it != this->arguments.begin()) {
                    stringName += ", ";
                }
                stringName += it->first + ": " + it->second.uncolored_formatted();
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
                    Template t2;
                    for (auto&& arg : inst->arguments) {
                        t2.arguments[arg.first] = arg.second;
                    }
                    auto parsedArg = t2.parse(t2, tokens, i, errors);
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
                    TemplateInstances::addTemplate(parsedArg.value());
                    t.setNth(parameterCount++, Token(tok_identifier, parsedArg.value().nameForInstance()));
                    i++;
                    if (tokens[i].type == tok_comma) {
                        i++;
                    }
                    continue;
                }
                size_t start = i;
                FPResult r = parseType(tokens, &i, {});
                if (r.success) {
                    t.setNth(parameterCount++, Token(tok_identifier, r.value));
                } else {
                    i = start;
                    t.setNth(parameterCount++, tokens[i]);
                }
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
            baseStruct.templateInstance = true;
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

    template<typename T>
    const typename std::list<T>::iterator& operator+(const typename std::list<T>::iterator& it, size_t i) {
        for (size_t j = 0; j < i; j++) {
            ++it;
        }
        return it;
    }

    struct Macro {
        enum MacroType {
            Generic,
            Native
        } type;
        std::vector<MacroArg> args;
        std::vector<Token> tokens;
        bool hasDollar;

        Macro() {
            this->type = Generic;
        }

        virtual ~Macro() {}

        virtual void expand(const Token& macroTok, std::vector<Token>& otherTokens, size_t& i, std::vector<Token>& args, std::vector<FPResult>& errors) {
            (void) errors;
            (void) macroTok;
            #define MacroError(msg) ({ \
                FPResult error; \
                error.success = false; \
                error.message = msg; \
                error.location = tokens[i].location; \
                error.type = tokens[i].type; \
                error.value = tokens[i].value; \
                error; \
            })
            #define MacroError2(msg, tok) ({ \
                FPResult error; \
                error.success = false; \
                error.message = msg; \
                error.location = tok.location; \
                error.type = tok.type; \
                error.value = tok.value; \
                error; \
            })
            if (this->tokens.empty()) {
                return;
            }
            if (!hasDollar) {
                otherTokens.insert(otherTokens.begin() + i, this->tokens.begin(), this->tokens.end());
                return;
            }

            const size_t count = this->tokens.size();
            Token* newToks = new Token[count];
            for (size_t j = 0; j < count; j++) {
                newToks[j] = tokens[j].type == tok_dollar ? args[tokens[j].location.line] : tokens[j];
            }
            otherTokens.insert(otherTokens.begin() + i, newToks, newToks + count);
            delete[] newToks;
        }
    };

    struct SclMacroError {
        void* msg;
        CSourceLocation* location;
    };

    FPResult findFileInIncludePath(std::string file);

    #define TO_STRING2(x) #x
    #define TO_STRING(x) TO_STRING2(x)

    struct SclParser {
        std::vector<Token>& tokens;
        size_t& i;
        void* (*str$of)(char*) = nullptr;
        void* (*alloc)(size_t) = nullptr;

        static CToken* peek(SclParser* parser) __asm(TO_STRING(__USER_LABEL_PREFIX__) "Parser$peek");
        static CToken* consume(SclParser* parser) __asm(TO_STRING(__USER_LABEL_PREFIX__) "Parser$consume");
    };

    CToken* SclParser::peek(SclParser* parser) {
        if (parser->i >= parser->tokens.size()) {
            return nullptr;
        }
        return parser->tokens[parser->i].toC(parser->alloc, parser->str$of);
    }

    CToken* SclParser::consume(SclParser* parser) {
        if (parser->i >= parser->tokens.size()) {
            return nullptr;
        }
        CToken* t = parser->tokens[parser->i].toC(parser->alloc, parser->str$of);
        parser->tokens.erase(parser->tokens.begin() + parser->i);
        return t;
    }

    struct NativeMacro: public Macro {
        CToken** (*func)(CSourceLocation*, SclParser*);
        void* lib;
        void* (*Result$getErr)(void* res) = nullptr;
        void* (*Result$getOk)(void* res) = nullptr;
        long (*Result$isErr)(void* res) = nullptr;
        long (*Result$isOk)(void* res) = nullptr;
        void* (*scl_migrate_array)(void* arr, size_t size, size_t elem_size) = nullptr;
        size_t (*scl_array_size)(void* arr) = nullptr;
        void* (*alloc)(size_t) = nullptr;
        void* (*str$of)(char*) = nullptr;
        char* (*str$view)(void* str) = nullptr;

        NativeMacro(std::string library, std::string name) : Macro() {
            this->type = Native;
            FPResult res = findFileInIncludePath(library);
            if (!res.success) {
                std::cout << "Failed to find library '" << library << "': " << res.message << std::endl;
                exit(1);
            }
            library = res.location.file;

            this->lib = dlopen(library.c_str(), RTLD_LAZY);
            if (!this->lib) {
                std::cout << "Failed to load library '" << library << "': " << dlerror() << std::endl;
                exit(1);
            }
            this->func = (CToken** (*)(CSourceLocation*, SclParser*)) dlsym(this->lib, name.c_str());
            if (!this->func) {
                std::cout << "Failed to load function '" << name << "': " << dlerror() << std::endl;
                exit(1);
            }

            Result$getErr = (typeof(Result$getErr)) dlsym(this->lib, "_M6Result6getErra");
            Result$getOk = (typeof(Result$getOk)) dlsym(this->lib, "_M6Result5getOka");
            Result$isErr = (typeof(Result$isErr)) dlsym(this->lib, "_M6Result5isErrl");
            Result$isOk = (typeof(Result$isOk)) dlsym(this->lib, "_M6Result4isOkl");
            scl_migrate_array = (typeof(scl_migrate_array)) dlsym(this->lib, "_scl_migrate_foreign_array");
            scl_array_size = (typeof(scl_array_size)) dlsym(this->lib, "_scl_array_size");
            alloc = (typeof(alloc)) dlsym(this->lib, "_scl_alloc");

            str$of = (typeof(str$of)) dlsym(this->lib, "_F3str14operator$storecE");
            str$view = (typeof(str$view)) dlsym(this->lib, "_M3str4viewc");
        }

        ~NativeMacro() {
            dlclose(this->lib);
        }

        void expand(const Token& macroTok, std::vector<Token>& otherTokens, size_t& i, std::vector<Token>& args, std::vector<FPResult>& errors) override {
            (void) args;
            SclParser* parser = (SclParser*) alloc(sizeof(SclParser));
            parser = new (parser) SclParser{
                otherTokens,
                i,
                str$of,
                alloc
            };

            CSourceLocation* loc = macroTok.location.toC(alloc, str$of);
            void* result = this->func(loc, parser);

            if (!Result$isOk(result)) {
                SclMacroError* err = (SclMacroError*) Result$getErr(result);
                FPResult error;
                error.success = false;
                error.message = str$view(err->msg);
                error.location = SourceLocation::of(err->location, str$view);
                errors.push_back(error);
                return;
            }

            CToken** theTokens = (CToken**) Result$getOk(result);
            size_t numTokens = scl_array_size(theTokens);
            if (!numTokens) {
                return;
            }
            otherTokens.reserve(otherTokens.size() + numTokens);
            for (size_t j = 0; j < numTokens; j++) {
                if (!theTokens[j]) {
                    errors.push_back(MacroError2("Macro returned invalid token", macroTok));
                    return;
                }
                otherTokens.insert(otherTokens.begin() + i, Token::of(theTokens[j], str$view));
                i++;
            }
        }
    };
    
    TPResult SyntaxTree::parse() {
        Function* currentFunction = nullptr;
        std::vector<Struct*> currentStructs;
        Interface* currentInterface = nullptr;
        Deprecation currentDeprecation;

        int isInLambda = 0;
        int isInUnsafe = 0;
        int nInits = 0;

        std::vector<std::string> uses;
        std::vector<std::string> nextAttributes;
        std::vector<Variable> globals;
        std::vector<Struct> structs;
        std::vector<Layout> layouts;
        std::vector<Interface*> interfaces;
        std::vector<Enum> enums;
        std::vector<Function*> functions;
        std::unordered_map<std::string, std::pair<std::string, bool>> typealiases;

        uses.reserve(16);
        nextAttributes.reserve(16);
        globals.reserve(16);
        structs.reserve(64);
        layouts.reserve(16);
        interfaces.reserve(16);
        enums.reserve(16);
        functions.reserve(128);
        typealiases.reserve(16);

        Variable& lastDeclaredVariable = Variable::emptyVar();

        std::vector<FPResult> errors;
        std::vector<FPResult> warns;

        std::unordered_map<std::string, Macro*> macros;

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type != tok_identifier || tokens[i].value != "macro!") {
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
            std::vector<sclc::MacroArg> args;
            if (tokens[i].type == tok_paren_open) {
                i++;
                while (tokens[i].type != tok_paren_close) {
                    args.push_back(tokens[i].value);
                    i++;
                    if (tokens[i].type == tok_column) {
                        i++;
                        FPResult r = parseType(tokens, &i, {});
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        args.back().type = r.value;
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
            Macro* macro = nullptr;
            if (tokens[i].type == tok_in) {
                i++;
                if (tokens[i].type != tok_string_literal) {
                    errors.push_back(MacroError("Expected string literal after 'in'"));
                    continue;
                }
                std::string library = tokens[i].value;
                i++;
                macro = new NativeMacro(library, name);
                macro->args = args;
                tokens.erase(tokens.begin() + start, tokens.begin() + i);
                i = start;
                i--;
                if (macros.find(name) != macros.end()) {
                    errors.push_back(MacroError("Macro '" + name + "' already defined"));
                    continue;
                }
                macros[name] = macro;
                continue;
            }
            macro = new Macro();
            macro->args = args;

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
                if (tokens[i].type == tok_dollar) {
                    macro->hasDollar = true;
                    i++;
                    if (tokens[i].type != tok_identifier) {
                        errors.push_back(MacroError("Expected identifier after '$'"));
                        continue;
                    }
                    size_t argIndex = -1;
                    for (size_t j = 0; j < macro->args.size(); j++) {
                        if (macro->args[j].name == tokens[i].value) {
                            argIndex = j;
                            break;
                        }
                    }
                    if (argIndex == (size_t) -1) {
                        errors.push_back(MacroError("Unknown argument '" + tokens[i].value + "'"));
                        continue;
                    }

                    macro->tokens.push_back(Token(tok_dollar, std::to_string(argIndex), SourceLocation("", argIndex, 0)));
                } else {
                    macro->tokens.push_back(tokens[i]);
                }
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
            if (tokens[i].type != tok_identifier) {
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
            if (
                macros.find(tokens[i].value) == macros.end() ||
                i + 1 >= tokens.size() ||
                tokens[i + 1].value != "!" ||
                tokens[i + 1].location.line != tokens[i].location.line
            ) {
                continue;
            }
            Macro* macro = macros[tokens[i].value];
            std::vector<Token> args;
            size_t start = i;
            Token macroTok = tokens[i];
            i += 2;
            if (macro->type != Macro::MacroType::Native) {
                for (size_t j = 0; j < macro->args.size(); j++) {
                    args.push_back(tokens[i]);
                    i++;
                }
            }
            // delete macro call
            tokens.erase(tokens.begin() + start, tokens.begin() + i);
            i = start;
            macro->expand(macroTok, tokens, i, args, errors);
            i--;
        }

        if (errors.size()) {
            TPResult result;
            result.errors = errors;
            result.warns = warns;
            return result;
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

            Function* builtinIsArray = new Function("builtinIsArray", Token(tok_identifier, "builtinIsArray"));
            builtinIsArray->addModifier("expect");
            builtinIsArray->addModifier("cdecl");
            builtinIsArray->addModifier("_scl_is_array");

            builtinIsArray->addArgument(Variable("obj", "any"));

            builtinIsArray->return_type = "int";
            functions.push_back(builtinIsArray);

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
                if (tokens[i].type == tok_comma || (tokens[i].type == tok_identifier && tokens[i].value == ">")) {
                    if (tokens[i].value == ",") {
                        i++;
                    }
                    inst.arguments[parameterName] = Token(tok_identifier, "any");
                    continue;
                }
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
                inst.arguments[parameterName] = Token(tok_identifier, result.value);
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
                if (currentStructs.size()) {
                    if (tokens[i + 2].type == tok_column) {
                        std::string name = tokens[i + 1].value;
                        if (name != currentStructs.back()->name) {
                            FPResult result;
                            result.message = "Cannot define method on different struct inside of a struct. Current struct: " + currentStructs.back()->name;
                            result.value = tokens[i + 1].value;
                            result.location = tokens[i + 1].location;
                            result.type = tokens[i + 1].type;
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }
                        i += 2;
                    }
                    if (contains<std::string>(nextAttributes, "static") || currentStructs.back()->isStatic()) {
                        std::string name = tokens[i + 1].value;
                        Token& func = tokens[i + 1];
                        currentFunction = parseFunction(currentStructs.back()->name + "$" + name, func, errors, i, tokens);
                        currentFunction->deprecated = currentDeprecation;
                        currentDeprecation.clear();
                        currentFunction->member_type = currentStructs.back()->name;
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
                    currentFunction = parseMethod(name, func, currentStructs.back()->name, errors, i, tokens);
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
                        currentFunction->addToken(token);
                        continue;
                    }
                    if (isInUnsafe) {
                        isInUnsafe--;
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
                } else if (currentStructs.size()) {
                    if (std::find(structs.begin(), structs.end(), *(currentStructs.back())) == structs.end()) {
                        structs.push_back(*(currentStructs.back()));
                    }
                    currentStructs.pop_back();
                    templateArgs.clear();
                } else if (currentInterface != nullptr) {
                    if (std::find(interfaces.begin(), interfaces.end(), currentInterface) == interfaces.end()) {
                        interfaces.push_back(currentInterface);
                    }
                    interfaces.push_back(currentInterface);
                    currentInterface = nullptr;
                } else {
                    FPResult result;
                    result.message = "Unexpected 'end' keyword outside of function, struct or interface body. Remove this:";
                    result.value = token.value;
                    result.location.line = token.location.line;
                    result.location = token.location;
                    result.type = token.type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
            } else if (token.type == tok_union_def) {
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
                std::string namePrefix = "";
                if (currentStructs.size()) {
                    namePrefix = currentStructs.back()->name + "$";
                }
                currentStructs.push_back(new Struct(namePrefix + tokens[i].value, tokens[i]));
                Struct* currentStruct = currentStructs.back();
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
                currentStructs.pop_back();
            } else if (token.type == tok_struct_def && (i == 0 || tokens[i - 1].type != tok_using)) {
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
                std::string namePrefix = "";
                if (currentStructs.size()) {
                    namePrefix = currentStructs.back()->name + "$";
                }
                Struct* currentStruct = new Struct(namePrefix + tokens[i].value, tokens[i]);
                currentStructs.push_back(currentStruct);
                for (std::string& m : nextAttributes) {
                    currentStruct->addModifier(m);
                }
                bool open = contains<std::string>(nextAttributes, "open");
                
                nextAttributes.clear();
                bool hasSuperSpecified = false;
                if (tokens[i + 1].value == "<") {
                    currentStruct->templateInstance = true;
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
                        if (tokens[i].type == tok_comma || (tokens[i].type == tok_identifier && tokens[i].value == ">")) {
                            if (tokens[i].value == ",") {
                                i++;
                            }
                            currentStruct->required_typed_arguments++;
                            currentStruct->addTemplateArgument(key, Token(tok_identifier, "any"));
                            templateArgs[key] = Token(tok_identifier, "any");
                            continue;
                        }
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
                        currentStruct->addTemplateArgument(key, Token(tok_identifier, value.value));
                        templateArgs[key] = Token(tok_identifier, value.value);
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
                    currentStructs.pop_back();
                }
            } else if (token.type == tok_identifier && token.value == "layout") {
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
                std::string namePrefix = "";
                if (currentStructs.size()) {
                    namePrefix = currentStructs.back()->name + "$";
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
                Layout layout(namePrefix + name);

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
            } else if (token.type == tok_using && currentFunction == nullptr && i + 1 < tokens.size() && tokens[i + 1].type == tok_struct_def) {
                if (usingStructs.find(token.location.file) == usingStructs.end()) {
                    usingStructs[token.location.file] = std::vector<std::string>();
                }
                i += 2;
                FPResult result = parseType(tokens, &i);
                if (!result.success) {
                    errors.push_back(result);
                    continue;
                }
                usingStructs[token.location.file].push_back(result.value);
            } else if (token.type == tok_enum) {
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
                std::string namePrefix = "";
                if (currentStructs.size()) {
                    namePrefix = currentStructs.back()->name + "$";
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
                Enum e = Enum(namePrefix + name);
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
                std::string namePrefix = "";
                if (currentStructs.size()) {
                    namePrefix = currentStructs.back()->name + "$";
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
                currentInterface = new Interface(namePrefix + tokens[i].value);
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
            } else if (currentFunction != nullptr) {
                if (
                    token.type == tok_lambda &&
                    (((ssize_t) i) - 3 >= 0 && tokens[i - 3].type != tok_declare) &&
                    (((ssize_t) i) - 1 >= 0 && tokens[i - 1].type != tok_as) &&
                    (((ssize_t) i) - 1 >= 0 && tokens[i - 1].type != tok_column) &&
                    (((ssize_t) i) - 1 >= 0 && tokens[i - 1].type != tok_bracket_open) &&
                    (((ssize_t) i) - 2 >= 0 && tokens[i - 2].type != tok_new)
                ) isInLambda++;
                if (token.type == tok_identifier && token.value == "unsafe") {
                    isInUnsafe++;
                }
                if (!contains<Function*>(functions, currentFunction))
                    currentFunction->addToken(token);
            } else if (token.type == tok_paren_open) {
                currentFunction = new Function("$init" + std::to_string(nInits), Token(tok_identifier, "$init" + std::to_string(nInits)));
                nInits++;
                currentFunction->addModifier("construct");
                currentFunction->return_type = "none";
                int depth = 0;
                do {
                    if (tokens[i].type == tok_paren_open) {
                        depth++;
                    } else if (tokens[i].type == tok_paren_close) {
                        depth--;
                    }
                    currentFunction->addToken(tokens[i]);
                    i++;
                } while (depth > 0);
                if (tokens[i].type != tok_store) {
                    FPResult result;
                    result.message = "Expected =>, but got: '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                currentFunction->addToken(tokens[i]);
                i++;
                if (tokens[i].type != tok_declare) {
                    FPResult result;
                    result.message = "Expected 'decl', but got: '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                i++;
                if (tokens[i].type != tok_identifier) {
                    FPResult result;
                    result.message = "Expected identifier, but got: '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                currentFunction->addToken(tokens[i]);
                i -= 2;
                functions.push_back(currentFunction);
                currentFunction = nullptr;
            } else if (token.type == tok_declare && currentStructs.empty()) {
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
                size_t start = i;
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
                Variable v(name, type);
                if (contains<std::string>(nextAttributes, "expect")) {
                    v.isExtern = true;
                }
                if (std::find(globals.begin(), globals.end(), v) == globals.end()) {
                    globals.push_back(v);
                } else {
                    FPResult result;
                    result.message = "Variable '" + v.name + "' already declared.";
                    result.value = tokens[start].value;
                    result.location = tokens[start].location;
                    result.type = tokens[start].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                nextAttributes.clear();
            } else if (token.type == tok_declare && currentStructs.size() > 0) {
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
                size_t start = i;
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
                if (currentStructs.back()->isStatic() || contains<std::string>(nextAttributes, "static")) {
                    v = Variable(currentStructs.back()->name + "$" + name, type, currentStructs.back()->name);
                    v.name_token = new Token(name_token);
                    v.isPrivate = (isPrivate || contains<std::string>(nextAttributes, "private"));
                    nextAttributes.clear();
                    if (contains<std::string>(nextAttributes, "expect")) {
                        v.isExtern = true;
                    }
                    if (std::find(globals.begin(), globals.end(), v) == globals.end()) {
                        globals.push_back(v);
                    } else {
                        FPResult result;
                        result.message = "Variable '" + v.name + "' already declared.";
                        result.value = tokens[start].value;
                        result.location = tokens[start].location;
                        result.type = tokens[start].type;
                        result.success = false;
                        errors.push_back(result);
                        continue;
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
                    v = Variable(name, type, currentStructs.back()->name);
                    v.name_token = new Token(name_token);
                    v.typeFromTemplate = fromTemplate;
                    v.isPrivate = (isPrivate || contains<std::string>(nextAttributes, "private"));
                    v.isVirtual = contains<std::string>(nextAttributes, "virtual");
                    currentStructs.back()->addMember(v);
                    nextAttributes.clear();
                }

                lastDeclaredVariable = v;
            } else {

                auto validAttribute = [](Token& t) -> bool {
                    return t.type  == tok_string_literal ||
                           t.value == "sinceVersion:" ||
                           t.value == "replaceWith:" ||
                           t.value == "deprecated!" ||
                           t.value == "nonvirtual" ||
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
                           t.value == "reified" ||
                           t.value == "static" ||
                           t.value == "export" ||
                           t.value == "expect" ||
                           t.value == "sealed" ||
                           t.value == "unsafe" ||
                           t.value == "cdecl" ||
                           t.value == "final" ||
                           t.value == "async" ||
                           t.value == "open" ||
                           t.value == "asm";
                };

                if ((tokens[i].value == "get" || tokens[i].value == "set") && currentStructs.size() && currentFunction == nullptr) {
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
                        if (tokens[i + 1].type == tok_identifier && tokens[i + 1].type == tok_lambda) {
                            i++;
                        }
                        
                        std::string name = "get";
                        name += (char) std::toupper(varName[0]);
                        name += varName.substr(1);

                        Method* getter = new Method(currentStructs.back()->name, name, getToken);
                        getter->return_type = lastDeclaredVariable.type;
                        getter->addModifier("@getter");
                        getter->addModifier(varName);
                        getter->addArgument(Variable("self", currentStructs.back()->name));
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
                        if (tokens[i + 1].type == tok_identifier && tokens[i + 1].type == tok_lambda) {
                            i++;
                        }

                        std::string name = "set";
                        name += (char) std::toupper(varName[0]);
                        name += varName.substr(1);

                        Method* setter = new Method(currentStructs.back()->name, name, setToken);
                        setter->return_type = "none";
                        setter->addModifier("@setter");
                        setter->addModifier(varName);
                        setter->addArgument(Variable(argName, lastDeclaredVariable.type));
                        setter->addArgument(Variable("self", currentStructs.back()->name));
                        currentFunction = setter;
                    }
                } else if (tokens[i].value == "typealias") {
                    i++;
                    std::string type = tokens[i].value;
                    i++;
                    bool nilable = tokens[i].type == tok_question_mark;
                    if (nilable) i++;
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
                    typealiases[type] = std::pair(replacement, nilable);
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
                    if (tokens[i].value == "construct" && currentStructs.size()) {
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
        result.globals = globals;
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
            Token t(tok_identifier, "toString", s.name_token.location);
            Method* toString = new Method(s.name, std::string("toString"), t);
            std::string retemplate(std::string type);
            std::string stringify = retemplate(s.name) + " {";
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
                if (canBeNil || isPointer(type) || hasEnum(result, type)) {
                    if (type == "float") {
                        toString->addToken(Token(tok_identifier, "float"));
                        toString->addToken(Token(tok_double_column, "::"));
                        toString->addToken(Token(tok_identifier, "toString"));
                    } else {
                        toString->addToken(Token(tok_identifier, "builtinToString"));
                    }
                } else if (hasTypeAlias(type) || strstarts(type, "lambda(") || type == "lambda") {
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
