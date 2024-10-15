
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <list>
#include <array>

#include <Common.hpp>

namespace sclc {
    extern std::unordered_map<std::string, std::vector<std::string>> usingStructs;

    std::string typeToRTSigIdent(std::string type);
    std::string argsToRTSignatureIdent(Function* f);

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
                FPResult r = parseType(tokens, i);
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

        for (auto&& p : funcNameIdents) {
            if (p.first == name) {
                name = p.second;
                break;
            }
        }

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
                        FPResult r = parseType(tokens, i);
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
                    func->addArgument(Variable(name, type));
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
                        FPResult r = parseType(tokens, i);
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
                        func->addArgument(Variable(s, type));
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
                FPResult r = parseType(tokens, i);
                if (!r.success) {
                    errors.push_back(r);
                    return func;
                }
                std::string type = r.value;
                std::string fromTemplate = r.message;
                func->return_type = type;
                if (namedReturn.size()) {
                    func->namedReturnValue = Variable(namedReturn, type).also([fromTemplate](Variable& v) {
                        v.canBeNil = typeCanBeNil(v.type);
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

    Method* parseMethodArguments(Method* method, std::vector<FPResult>& errors, size_t& i, std::vector<Token>& tokens) {
        bool memberByValue = false;
        if (tokens[i].type == tok_paren_open) {
            i++;
            if (tokens[i].type == tok_addr_of) {
                memberByValue = true;
                i++;
                if (tokens[i].type != tok_comma) {
                    FPResult result;
                    result.message = "Type 'none' is only valid for function return types.";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    return method;
                }
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
                        FPResult r = parseType(tokens, i);
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
                    method->addArgument(Variable(name, type));
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
                        FPResult r = parseType(tokens, i);
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
                        method->addArgument(Variable(s, type));
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
                    return method;
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
                return method;
            }
            i++;
            method->addArgument(Variable("self", memberByValue ? "@" + method->member_type : method->member_type));
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

        for (auto&& p : funcNameIdents) {
            if (p.first == name) {
                name = p.second;
                break;
            }
        }

        Method* method = new Method(memberName, name, name_token);
        method->force_add = true;
        if (method->name_without_overload == "init") {
            method->addModifier("<constructor>");
        }
        i += 2;
        if (tokens[i].type == tok_identifier && tokens[i].value == "<") {
            method->reified_parameters = parseReifiedParams(errors, i, tokens);
            method->reified_parameters.push_back("");
        }
        method = parseMethodArguments(method, errors, i, tokens);
        std::string namedReturn = "";
        if (tokens[i].type == tok_identifier) {
            namedReturn = tokens[i].value;
            i++;
        }
        if (tokens[i].type == tok_column) {
            i++;
            FPResult r = parseType(tokens, i);
            if (!r.success) {
                errors.push_back(r);
                return method;
            }

            std::string fromTemplate = r.message;
            std::string type = r.value;
            method->return_type = type;
            if (namedReturn.size()) {
                method->namedReturnValue = Variable(namedReturn, type);
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

    std::vector<Token> parseString(std::string s);

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

    struct SclConfig {
        void* (*str$of)(char*);
        void* (*alloc)(size_t);
        char* (*str$view)(void* str);

        static void* getKey(SclConfig* conf, void* key) __asm__(TO_STRING(__USER_LABEL_PREFIX__) "Config$getKey");
        static long long hasKey(SclConfig* conf, void* key) __asm__(TO_STRING(__USER_LABEL_PREFIX__) "Config$hasKey");
    };

    struct SclParser {
        SclConfig* config;
        std::vector<Token>& tokens;
        size_t& i;
        void* (*str$of)(char*) = nullptr;
        void* (*alloc)(size_t) = nullptr;

        static CToken* peek(SclParser* parser) __asm__(TO_STRING(__USER_LABEL_PREFIX__) "Parser$peek");
        static CToken* consume(SclParser* parser) __asm__(TO_STRING(__USER_LABEL_PREFIX__) "Parser$consume");
    };

    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    extern "C" CToken* SclParser::peek(SclParser* parser) {
        if (parser->i >= parser->tokens.size()) {
            return nullptr;
        }
        return parser->tokens[parser->i].toC(parser->alloc, parser->str$of);
    }

    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    extern "C" CToken* SclParser::consume(SclParser* parser) {
        if (parser->i >= parser->tokens.size()) {
            return nullptr;
        }
        CToken* t = parser->tokens[parser->i].toC(parser->alloc, parser->str$of);
        parser->tokens.erase(parser->tokens.begin() + parser->i);
        return t;
    }

    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    extern "C" void* SclConfig::getKey(SclConfig* conf, void* arg) {
        auto s = Main::config->getStringByPath(conf->str$view(arg));
        return s ? conf->str$of((char*) s->getValue().c_str()) : nullptr;
    }
    
    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    extern "C" long long SclConfig::hasKey(SclConfig* conf, void* arg) {
        return SclConfig::getKey(conf, arg) != nullptr;
    }

    struct NativeMacro: public Macro {
        CToken** (*func)(CSourceLocation*, SclParser*);
        #ifdef _WIN32
        using library_type = HMODULE;
        #else
        using library_type = void*;
        #endif

        library_type lib;
        void* (*Result$getErr)(void* res) = nullptr;
        void* (*Result$getOk)(void* res) = nullptr;
        long (*Result$isErr)(void* res) = nullptr;
        long (*Result$isOk)(void* res) = nullptr;
        void* (*scale_migrate_array)(void* arr, size_t size, size_t elem_size) = nullptr;
        size_t (*scale_array_size)(void* arr) = nullptr;
        void* (*alloc)(size_t) = nullptr;
        void* (*str$of)(char*) = nullptr;
        char* (*str$view)(void* str) = nullptr;

        template<typename Func>
        Func getFunction(library_type lib, const char* name) {
        #ifdef _WIN32
            Func f = (Func) GetProcAddress(lib, name);
            if (!f) {
                std::cout << "Failed to load function '" << name << "': " << GetLastError() << std::endl;
                exit(1);
            }
            return f;
        #else
            Func f = (Func) dlsym(lib, name);
            if (!f) {
                std::cout << "Failed to load function '" << name << "': " << dlerror() << std::endl;
                exit(1);
            }
            return f;
        #endif
        }

        library_type openLibrary(const char* name) {
        #ifdef _WIN32
            library_type lib = LoadLibraryExA(name, nullptr, 0);
            if (!lib) {
                std::cout << "Failed to load library '" << name << "': " << GetLastError() << std::endl;
                exit(1);
            }
            return lib;
        #else
            library_type lib = dlopen(name, RTLD_LAZY | RTLD_GLOBAL);
            if (!lib) {
                std::cout << "Failed to load library '" << name << "': " << dlerror() << std::endl;
                exit(1);
            }
            return lib;
        #endif
        }

        NativeMacro(std::string library, std::string name) : Macro() {
            this->type = Native;
            FPResult res = findFileInIncludePath(library);
            if (!res.success) {
                std::cout << "Failed to find library '" << library << "': " << res.message << std::endl;
                exit(1);
            }
            library = res.location.file;

            lib = openLibrary(library.c_str());
            func = getFunction<typeof(func)>(this->lib, name.c_str());
            Result$getErr = getFunction<typeof(Result$getErr)>(this->lib, "_M6Result6getErra");
            Result$getOk = getFunction<typeof(Result$getOk)>(this->lib, "_M6Result5getOka");
            Result$isErr = getFunction<typeof(Result$isErr)>(this->lib, "_M6Result8getIsErrg");
            Result$isOk = getFunction<typeof(Result$isOk)>(this->lib, "_M6Result7getIsOkg");
            scale_migrate_array = getFunction<typeof(scale_migrate_array)>(this->lib, "scale_migrate_foreign_array");
            scale_array_size = getFunction<typeof(scale_array_size)>(this->lib, "scale_array_size");
            alloc = getFunction<typeof(alloc)>(this->lib, "scale_alloc");
            str$of = getFunction<typeof(str$of)>(this->lib, "_F3str2ofcE");
            str$view = getFunction<typeof(str$view)>(this->lib, "_M3str4viewc");
        }

        ~NativeMacro() {
            #ifdef _WIN32
            FreeLibrary(this->lib);
            #else
            dlclose(this->lib);
            #endif
        }

        void expand(const Token& macroTok, std::vector<Token>& otherTokens, size_t& i, std::vector<Token>& args, std::vector<FPResult>& errors) override {
            (void) args;

            SclConfig* config = (SclConfig*) alloc(sizeof(SclConfig));
            config = new (config) SclConfig{
                str$of,
                alloc,
                str$view
            };

            SclParser* parser = (SclParser*) alloc(sizeof(SclParser));
            parser = new (parser) SclParser{
                config,
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
            size_t numTokens = scale_array_size(theTokens);
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
    
    void SyntaxTree::parse(TPResult& result) {
        Function* currentFunction = nullptr;
        std::vector<Struct*> currentStructs;
        Interface* currentInterface = nullptr;
        Layout currentLayout("");
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
        std::unordered_map<std::string, std::pair<std::string, bool>> scale_typealiases;

        uses.reserve(16);
        nextAttributes.reserve(16);
        globals.reserve(16);
        structs.reserve(64);
        layouts.reserve(16);
        interfaces.reserve(16);
        enums.reserve(16);
        functions.reserve(128);
        typealiases.reserve(16);
        scale_typealiases.reserve(16);

        Variable& lastDeclaredVariable = Variable::emptyVar();

        std::vector<FPResult> errors;
        std::vector<FPResult> warns;

        std::unordered_map<std::string, Macro*> macros;

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type != tok_identifier) {
                continue;
            } else if (tokens[i].value == "macro!") {
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
                            FPResult r = parseType(tokens, i);
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
                    DBG("Defining native macro %s", name.c_str());
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
                #ifdef DEBUG
                if (macros.find(name) == macros.end()) {
                    DBG("Defining macro %s", name.c_str());
                } else {
                    DBG("Re-defining macro %s", name.c_str());
                }
                #endif
                macros[name] = macro;
            }
        }

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type != tok_identifier) {
                continue;
            }

            if (
                i + 1 < tokens.size() &&
                tokens[i + 1].location.line == tokens[i].location.line &&
                tokens[i + 1].value == "!" &&
                macros.find(tokens[i].value) != macros.end()
            ) {
                Macro* macro = macros[tokens[i].value];
                DBG("Expanding macro %s with %zu tokens", tokens[i].value.c_str(), macro->tokens.size());
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
        }

        DBG("Done expanding macros");

        if (errors.size()) {
            result.errors = errors;
            result.warns = warns;
            return;
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
            
            builtinHash->addArgument(Variable("data", "const [int8]"));
            
            builtinHash->return_type = "uint";
            functions.push_back(builtinHash);

            Function* builtinIdentityHash = new Function("builtinIdentityHash", Token(tok_identifier, "builtinIdentityHash"));
            builtinIdentityHash->addModifier("expect");
            builtinIdentityHash->addModifier("cdecl");
            builtinIdentityHash->addModifier("scale_identity_hash");
            
            builtinIdentityHash->addArgument(Variable("obj", "any"));
            
            builtinIdentityHash->return_type = "int";
            functions.push_back(builtinIdentityHash);

            Function* builtinAtomicClone = new Function("builtinAtomicClone", Token(tok_identifier, "builtinAtomicClone"));
            builtinAtomicClone->addModifier("expect");
            builtinAtomicClone->addModifier("cdecl");
            builtinAtomicClone->addModifier("scale_atomic_clone");
            
            builtinAtomicClone->addArgument(Variable("obj", "any"));
            
            builtinAtomicClone->return_type = "any";
            functions.push_back(builtinAtomicClone);

            Function* builtinTypeEquals = new Function("builtinTypeEquals", Token(tok_identifier, "builtinTypeEquals"));
            builtinTypeEquals->addModifier("expect");
            builtinTypeEquals->addModifier("cdecl");
            builtinTypeEquals->addModifier("scale_is_instance_of");

            builtinTypeEquals->addArgument(Variable("obj", "any"));
            builtinTypeEquals->addArgument(Variable("type_id", "uint"));

            builtinTypeEquals->return_type = "int";
            functions.push_back(builtinTypeEquals);

            Function* builtinIsInstance = new Function("builtinIsInstance", Token(tok_identifier, "builtinIsInstance"));
            builtinIsInstance->addModifier("expect");
            builtinIsInstance->addModifier("cdecl");
            builtinIsInstance->addModifier("scale_is_instance");

            builtinIsInstance->addArgument(Variable("obj", "any"));

            builtinIsInstance->return_type = "int";
            functions.push_back(builtinIsInstance);

            Function* builtinIsArray = new Function("builtinIsArray", Token(tok_identifier, "builtinIsArray"));
            builtinIsArray->addModifier("expect");
            builtinIsArray->addModifier("cdecl");
            builtinIsArray->addModifier("scale_is_array");

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
            builtinUnreachable->return_type = "nothing";

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

        struct NewTemplate {
            std::vector<std::string> params;
            std::vector<Token> tokens;
            Token name;
        };

        struct NewTemplateInstance {
            const NewTemplate& templ;
            std::vector<std::pair<std::string, std::vector<Token>>> expansions;

            NewTemplateInstance(const NewTemplate& t) : templ(t) {};

            std::string makeIdentifier(std::string what) {
                static std::unordered_map<std::string, std::string> cache;

                auto it = cache.find(what);
                if (it != cache.end()) {
                    return it->second;
                }

                bool isValidIdent = true;
                for (char c : what) {
                    if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9')) {
                        isValidIdent = false;
                        break;
                    }
                }
                if (isValidIdent) return cache[what] = what;
                std::string s;
                for (char c : what) {
                    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                        s.push_back(c);
                    } else {
                        s += format("$x%02x", c);
                    }
                }
                return cache[what] = s;
            }

            std::string identifier() {
                if (!iden.empty()) return iden;

                std::string ident = "$T" + templ.name.value + makeIdentifier("<");
                size_t x = 0;
                for (auto&& e : expansions) {
                    if (x) ident += makeIdentifier(", ");
                    x++;
                    for (auto&& x : e.second) {
                        if (x.value == ",") {
                            ident += makeIdentifier(", ");
                        } else {
                            std::string y = makeIdentifier(x.value);
                            ident += y;
                        }
                    }
                }
                ident += makeIdentifier(">");
                return (iden = ident);
            }
        private:
            std::string iden;
        };

        std::vector<NewTemplateInstance> inst;
        std::vector<NewTemplate> templates;

        DBG("Parsing templates");

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type != tok_identifier || tokens[i].value != "template") continue;

            size_t start = i;

            NewTemplate t;
            i++;
            if (tokens[i].type != tok_identifier || tokens[i].value != "<") {
                FPResult result;
                result.message = "Expected '<' after 'template'";
                result.value = tokens[i - 1].value;
                result.location = tokens[i - 1].location;
                result.type = tokens[i - 1].type;
                result.success = false;
                errors.push_back(result);
                continue;
            }
            i++;
            while (tokens[i].type != tok_comma && (tokens[i].type != tok_identifier || tokens[i].value != ">")) {
                t.params.push_back(tokens[i].value);
                i++;
                if (tokens[i].type != tok_comma && (tokens[i].type != tok_identifier || tokens[i].value != ">")) {
                    FPResult result;
                    result.message = "Expected ',' but got '" + tokens[i].value;
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                } else if (tokens[i].type == tok_comma) {
                    i++;
                }
            }
            i++;

            auto isScopeEnter = [](const Token& t) {
                return t.type == tok_function ||
                       t.type == tok_struct_def ||
                       t.type == tok_enum ||
                       t.type == tok_union_def ||
                       t.type == tok_interface_def ||
                       (t.type == tok_identifier && t.value == "layout") ||
                       (t.type == tok_identifier && t.value == "init") ||
                       (t.type == tok_identifier && t.value == "deinit");
            };
            auto isScopeEnterInFunc = [](const Token& t) {
                return t.type == tok_function ||
                       t.type == tok_struct_def ||
                       t.type == tok_enum ||
                       t.type == tok_union_def ||
                       t.type == tok_interface_def ||
                       t.type == tok_unsafe ||
                       (t.type == tok_identifier && t.value == "layout");
            };

            std::function<bool(const Token&)> scoper[] = {isScopeEnter, isScopeEnterInFunc};

            int scope = 0;
            bool inFunction = false;
            while (i < tokens.size() && scope >= 0) {
                if (scoper[inFunction](tokens[i])) {
                    if (t.name.value.empty() && !inFunction) {
                        t.name = tokens[i + 1];
                    }
                    if (
                        tokens[i].type == tok_function ||
                       (tokens[i].type == tok_identifier && tokens[i].value == "init") ||
                       (tokens[i].type == tok_identifier && tokens[i].value == "deinit")
                    ) inFunction = true;
                    scope++;
                } else if (tokens[i].type == tok_end) {
                    inFunction = false;
                    scope--;
                    if (scope <= 0) {
                        t.tokens.push_back(tokens[i]);
                        i++;
                        break;
                    }
                }
                t.tokens.push_back(tokens[i]);
                i++;
            }

            tokens.erase(tokens.begin() + start, tokens.begin() + i);
            tokens.insert(tokens.begin() + start, Token(tok_eof, t.name.value));
            i = start - 1;
            templates.push_back(t);
        }

        std::vector<std::string> initialized;

        DBG("Instantiating templates");

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type != tok_identifier) continue;

            for (auto&& t : templates) {
                if (tokens[i] == t.name) {
                    DBG("Instantiating template %s", t.name.value.c_str());
                    SourceLocation where = tokens[i].location;
                    size_t start = i;
                    i++;
                    if (tokens[i].type != tok_identifier || tokens[i].value != "<") {
                        FPResult result;
                        result.message = "Cannot use template '" + t.name.value + "' uninstantiated";
                        result.value = tokens[i - 1].value;
                        result.location = tokens[i - 1].location;
                        result.type = tokens[i - 1].type;
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }
                    i++;
                    
                    NewTemplateInstance ti(t);

                    for (auto&& arg : t.params) {
                        std::vector<Token> params;
                        DBG("  arg: %s", arg.c_str());

                        int scope = 0;
                        while (
                            i < tokens.size() &&
                            (
                                scope > 0 ||
                                (
                                    tokens[i].type != tok_comma &&
                                    (
                                        tokens[i].type != tok_identifier ||
                                        tokens[i].value != ">"
                                    )
                                )
                            )
                        ) {
                            if (tokens[i].type == tok_curly_open || tokens[i].type == tok_paren_open || tokens[i].type == tok_bracket_open || (tokens[i].type == tok_identifier && tokens[i].value == "<")) scope++;
                            if (tokens[i].type == tok_curly_close || tokens[i].type == tok_paren_close || tokens[i].type == tok_bracket_close || (tokens[i].type == tok_identifier && tokens[i].value == ">")) scope--;
                            params.push_back(tokens[i]);
                            DBG("    param: %s", tokens[i].value.c_str());
                            i++;
                        }
                        ti.expansions.push_back({arg, params});
                        i++;
                    }

                    DBG("Erasing instatiating tokens");
                    tokens.erase(tokens.begin() + start, tokens.begin() + i);
                    i = start - 1;

                    auto max = [](auto x, auto y) {
                        return x > y ? x : y;
                    };

                    DBG("Resolving template %s", t.name.value.c_str());
                    tokens.reserve(max(tokens.capacity(), tokens.size() + 2));
                    std::string id = ti.identifier();
                    tokens.insert(tokens.begin() + i + 1, Token(tok_identifier, id, where));

                    if (contains(initialized, id)) {
                        DBG("Template %s already initialized, skipping", t.name.value.c_str());
                        continue;
                    }

                    auto expand = [&]() {
                        std::vector<Token> toks;
                        for (auto&& tok : ti.templ.tokens) {
                            if (tok.type != tok_identifier) {
                                toks.push_back(tok);
                            } else if (tok == ti.templ.name && tok.location == ti.templ.name.location) {
                                toks.push_back(Token(tok_identifier, id, tok.location));
                            } else {
                                bool found = false;
                                for (auto&& x : ti.expansions) {
                                    if (x.first == tok.value) {
                                        for (auto&& t : x.second) {
                                            toks.push_back(t);
                                        }
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) {
                                    // Token t(tok);
                                    // t.location = where;
                                    toks.push_back(tok);
                                }
                            }
                        }
                        return toks;
                    };

                    DBG("Expanding template %s", t.name.value.c_str());
                    std::vector<Token> toks = expand();
                    tokens.reserve(tokens.size() + toks.size());
                    auto it = tokens.begin();
                    for (; it != tokens.end(); ++it) {
                        if (it->type == tok_eof && it->value == t.name.value) {
                            break;
                        }
                    }
                    tokens.insert(it, toks.begin(), toks.end());
                    initialized.push_back(id);
                    i = 0;
                }
            }
        }

        for (size_t i = 0; i < tokens.size(); i++) {
            int count = 0;
            for (count = 0; count < 3; count++) {
                if (i + count >= tokens.size()) {
                    break;
                }
                if (tokens[i + count].type != tok_identifier || tokens[i + count].value != ">") {
                    break;
                }
            }
            if (count < 2) continue;
            SourceLocation begin = tokens[i].location;
            std::string val;
            val.reserve(count);
            for (int x = 0; x < count; x++) {
                tokens.erase(tokens.begin() + i);
                val.push_back('>');
            }
            tokens.insert(tokens.begin() + i, Token(tok_identifier, val, begin));
        }

        DBG("Fixing tokens");

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type == tok_eof) {
                tokens.erase(tokens.begin() + i);
                i--;
            }
        }

        DBG("Parsing tokens");

        for (size_t i = 0; i < tokens.size(); i++) {
            Token& token = tokens[i];

            if (token.type == tok_function) {
                if (currentFunction != nullptr) {
                    FPResult result;
                    result.message = "Cannot define function inside another function. Current function: " + currentFunction->name_without_overload;
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
                        currentFunction = parseFunction(name, func, errors, i, tokens);
                        currentFunction->name = currentStructs.back()->name + "$" + currentFunction->name;
                        currentFunction->name_without_overload = currentStructs.back()->name + "$" + currentFunction->name_without_overload;
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
                        if (currentFunction->has_expect || currentFunction->has_operator) {
                            functions.push_back(currentFunction);
                            currentFunction = nullptr;
                        }
                        nextAttributes.clear();
                    } else {
                        Token& func = tokens[i + 1];
                        std::string name = func.value;
                        currentFunction = parseMethod(name, func, currentStructs.back()->name, errors, i, tokens);
                        if (name == "init") {
                            FPResult result;
                            result.message = "Instance initializers should not be declared like functions.";
                            result.location = func.location;
                            result.type = func.type;
                            result.success = false;
                            warns.push_back(result);
                            FPResult hint;
                            hint.message = "Remove 'function' keyword and return type";
                            hint.location = func.location;
                            hint.type = func.type;
                            hint.success = false;
                            hint.isNote = true;
                            warns.push_back(hint);
                        }
                        currentFunction->deprecated = currentDeprecation;
                        currentDeprecation.clear();
                        for (std::string& s : nextAttributes) {
                            currentFunction->addModifier(s);
                        }
                        Function* f = findMethodByName(currentFunction->name, currentFunction->member_type);
                        if (f) {
                            currentFunction->name = currentFunction->name + "$$ol" + argsToRTSignatureIdent(currentFunction);
                        }
                        if (currentFunction->has_expect || currentFunction->has_operator) {
                            functions.push_back(currentFunction);
                            currentFunction = nullptr;
                        }
                        nextAttributes.clear();
                    }
                } else if (!currentLayout.name.empty()) {
                    if (contains<std::string>(nextAttributes, "static")) {
                        std::string name = tokens[i + 1].value;
                        Token& func = tokens[i + 1];
                        currentFunction = parseFunction(name, func, errors, i, tokens);
                        currentFunction->name = currentLayout.name + "$" + currentFunction->name;
                        currentFunction->name_without_overload = currentLayout.name + "$" + currentFunction->name_without_overload;
                        currentFunction->deprecated = currentDeprecation;
                        currentDeprecation.clear();
                        currentFunction->member_type = currentLayout.name;
                        for (std::string& s : nextAttributes) {
                            currentFunction->addModifier(s);
                        }
                        Function* f = findFunctionByName(currentFunction->name);
                        if (f) {
                            currentFunction->name = currentFunction->name + "$$ol" + argsToRTSignatureIdent(currentFunction);
                        }
                        if (currentFunction->has_expect || currentFunction->has_operator) {
                            functions.push_back(currentFunction);
                            currentFunction = nullptr;
                        }
                        nextAttributes.clear();
                    } else {
                        Token& func = tokens[i + 1];
                        std::string name = func.value;
                        currentFunction = parseMethod(name, func, currentLayout.name, errors, i, tokens);
                        if (name == "init") {
                            FPResult result;
                            result.message = "Instance initializers should not be declared like functions.";
                            result.location = func.location;
                            result.type = func.type;
                            result.success = false;
                            warns.push_back(result);
                            FPResult hint;
                            hint.message = "Remove 'function' keyword and return type";
                            hint.location = func.location;
                            hint.type = func.type;
                            hint.success = false;
                            hint.isNote = true;
                            warns.push_back(hint);
                        }
                        currentFunction->deprecated = currentDeprecation;
                        currentFunction->addModifier("nonvirtual");
                        currentDeprecation.clear();
                        for (std::string& s : nextAttributes) {
                            currentFunction->addModifier(s);
                        }
                        Function* f = findMethodByName(currentFunction->name, currentFunction->member_type);
                        if (f) {
                            currentFunction->name = currentFunction->name + "$$ol" + argsToRTSignatureIdent(currentFunction);
                        }
                        if (currentFunction->has_expect || currentFunction->has_operator) {
                            functions.push_back(currentFunction);
                            currentFunction = nullptr;
                        }
                        nextAttributes.clear();
                    }
                } else if (currentInterface != nullptr) {
                    if (contains<std::string>(nextAttributes, "default")) {
                        Token& func = tokens[i + 1];
                        std::string name = func.value;
                        currentFunction = parseMethod(name, func, "", errors, i, tokens);
                        currentFunction->deprecated = currentDeprecation;
                        currentDeprecation.clear();
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
                } else {
                    i++;
                    if (tokens[i + 1].type == tok_column) {
                        std::string member_type = tokens[i].value;
                        i++;
                        Token& func = tokens[i + 1];
                        std::string name = func.value;
                        currentFunction = parseMethod(name, func, member_type, errors, i, tokens);
                        if (name == "init") {
                            FPResult result;
                            result.message = "Instance initializers should not be declared like functions.";
                            result.location = func.location;
                            result.type = func.type;
                            result.success = false;
                            warns.push_back(result);
                            FPResult hint;
                            hint.message = "Remove 'function' keyword and return type";
                            hint.location = func.location;
                            hint.type = func.type;
                            hint.success = false;
                            hint.isNote = true;
                            warns.push_back(hint);
                        }
                        currentFunction->deprecated = currentDeprecation;
                        currentDeprecation.clear();
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
                            currentFunction = parseFunction(name, func, errors, i, tokens);
                            currentFunction->name = memberType + "$" + currentFunction->name;
                            currentFunction->name_without_overload = memberType + "$" + currentFunction->name_without_overload;
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
                    if (currentFunction->has_expect || currentFunction->has_operator) {
                        functions.push_back(currentFunction);
                        currentFunction = nullptr;
                    }
                    nextAttributes.clear();
                }
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
                    if (!currentLayout.name.empty()) {
                        if (!currentFunction->has_nonvirtual) {
                            currentFunction->addModifier("nonvirtual");
                        }
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
                            } else {
                                if (currentFunction->args == f->args && !(currentFunction->has_reified && f->has_reified)) {
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
                                if (currentFunction->has_reified && f->has_reified) {
                                    currentFunction = nullptr;
                                    continue;
                                }
                                currentFunction->name = currentFunction->name + "$$ol" + argsToRTSignatureIdent(currentFunction);
                                functionWasOverloaded = true;
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
                } else if (currentInterface != nullptr) {
                    if (std::find(interfaces.begin(), interfaces.end(), currentInterface) == interfaces.end()) {
                        interfaces.push_back(currentInterface);
                    }
                    interfaces.push_back(currentInterface);
                    currentInterface = nullptr;
                } else if (!currentLayout.name.empty()) {
                    if (std::find(layouts.begin(), layouts.end(), currentLayout) == layouts.end()) {
                        layouts.push_back(currentLayout);
                    }
                    currentLayout = Layout("");
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
                currentStruct->addModifier("open");
                for (std::string& m : nextAttributes) {
                    currentStruct->addModifier(m);
                }
                nextAttributes.clear();
                currentStruct->super = "Union";
                i++;

                auto makeGetter = [](const Variable& v, const std::string& unionName, int n) {
                    const std::string& varName = v.name;
                    std::string name = "get" + capitalize(varName);

                    Method* getter = new Method(unionName, name, v.name_token);
                    getter->return_type = v.type;
                    getter->force_add = true;
                    getter->addModifier("@getter");
                    getter->addModifier(varName);
                    getter->addArgument(Variable("self", unionName));
                    getter->addToken(Token(tok_number, std::to_string(n), v.name_token.location));
                    getter->addToken(Token(tok_identifier, "self", v.name_token.location));
                    getter->addToken(Token(tok_column, ":", v.name_token.location));
                    getter->addToken(Token(tok_identifier, "expected", v.name_token.location));
                    std::string removed = removeTypeModifiers(v.type);
                    if (removed == "none" || removed == "nothing") {
                        return getter;
                    }
                    getter->addToken(Token(tok_unsafe, "unsafe", v.name_token.location));
                    getter->addToken(Token(tok_addr_ref, "ref", v.name_token.location));
                    getter->addToken(Token(tok_identifier, "self", v.name_token.location));
                    getter->addToken(Token(tok_dot, ".", v.name_token.location));
                    getter->addToken(Token(tok_identifier, "__value", v.name_token.location));
                    getter->addToken(Token(tok_as, "as", v.name_token.location));
                    getter->addToken(Token(tok_identifier, "*", v.name_token.location));
                    getter->addToken(Token(tok_identifier, removeTypeModifiers(v.type), v.name_token.location));
                    getter->addToken(Token(tok_addr_of, "@", v.name_token.location));
                    getter->addToken(Token(tok_return, "return", v.name_token.location));
                    getter->addToken(Token(tok_end, "end", v.name_token.location));
                    return getter;
                };

                auto makeChecker = [](const Variable& v, const std::string& unionName, int n) {
                    const std::string& varName = v.name;
                    std::string name = "get" + capitalize(varName);
                    
                    Method* getter = new Method(unionName, name, v.name_token);
                    getter->return_type = v.type;
                    getter->force_add = true;
                    getter->addModifier("@getter");
                    getter->addModifier(varName);
                    getter->addArgument(Variable("self", unionName));
                    getter->addToken(Token(tok_number, std::to_string(n), v.name_token.location));
                    getter->addToken(Token(tok_identifier, "self", v.name_token.location));
                    getter->addToken(Token(tok_dot, ".", v.name_token.location));
                    getter->addToken(Token(tok_identifier, "__tag", v.name_token.location));
                    getter->addToken(Token(tok_identifier, "==", v.name_token.location));
                    getter->addToken(Token(tok_return, "return", v.name_token.location));
                    return getter;
                };

                auto makeSetter = [](const Variable& v, const std::string& unionName, int n) {
                    Function* setter = new Function(unionName + "$" + v.name, v.name_token);
                    setter->member_type = unionName;
                    setter->return_type = unionName;
                    setter->addModifier("static");
                    std::string removed = removeTypeModifiers(v.type);
                    if (removed != "none" && removed != "nothing") {
                        setter->addArgument(Variable("what", v.type));
                    }
                    setter->addToken(Token(tok_unsafe, "unsafe", v.name_token.location));
                    setter->addToken(Token(tok_number, std::to_string(n), v.name_token.location));
                    if (removed != "none" && removed != "nothing") {
                        setter->addToken(Token(tok_addr_ref, "ref", v.name_token.location));
                        setter->addToken(Token(tok_identifier, "what", v.name_token.location));
                        setter->addToken(Token(tok_as, "as", v.name_token.location));
                        setter->addToken(Token(tok_identifier, "*", v.name_token.location));
                        setter->addToken(Token(tok_identifier, "any", v.name_token.location));
                        setter->addToken(Token(tok_addr_of, "@", v.name_token.location));
                    } else {
                        setter->addToken(Token(tok_nil, "nil", v.name_token.location));
                    }
                    setter->addToken(Token(tok_identifier, unionName, v.name_token.location));
                    setter->addToken(Token(tok_double_column, "::", v.name_token.location));
                    setter->addToken(Token(tok_identifier, "new", v.name_token.location));
                    setter->addToken(Token(tok_return, "return", v.name_token.location));
                    setter->addToken(Token(tok_end, "end", v.name_token.location));
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
                        FPResult result = parseType(tokens, i);
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
                    v.name_token = tokens[start];
                    v.isConst = true;
                    v.isVirtual = true;
                    Variable isv = Variable("is" + capitalize(name), "const bool");
                    isv.name_token = tokens[start];
                    isv.isConst = true;
                    isv.isVirtual = true;
                    currentStruct->addMember(v);
                    currentStruct->addMember(isv);
                    Method* getter = makeGetter(v, currentStruct->name, currentStruct->members.size() / 2);
                    if (currentStruct->isExtern()) {
                        getter->addModifier("expect");
                    }
                    functions.push_back(getter);
                    Method* checker = makeChecker(isv, currentStruct->name, currentStruct->members.size() / 2);
                    if (currentStruct->isExtern()) {
                        checker->addModifier("expect");
                    }
                    functions.push_back(checker);
                    Function* setter = makeSetter(v, currentStruct->name, currentStruct->members.size() / 2);
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
                
                nextAttributes.clear();
                bool hasSuperSpecified = false;
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
                currentLayout = Layout(namePrefix + name);
                currentLayout.name_token = tokens[i];
                if (contains<std::string>(nextAttributes, "expect")) {
                    currentLayout.isExtern = true;
                }
                nextAttributes.clear();
            } else if (token.type == tok_using && currentFunction == nullptr && i + 1 < tokens.size() && tokens[i + 1].type == tok_struct_def) {
                if (usingStructs.find(token.location.file) == usingStructs.end()) {
                    usingStructs[token.location.file] = std::vector<std::string>();
                }
                i += 2;
                FPResult result = parseType(tokens, i);
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
                if (contains<std::string>(nextAttributes, "expect")) {
                    e.isExtern = true;
                }
                nextAttributes.clear();
                e.name_token = tokens[i - 1];
                while (i < tokens.size() && tokens[i].type != tok_end) {
                    long next = e.nextValue;
                    if (tokens[i].type == tok_bracket_open) {
                        i++;
                        expect("number", tokens[i].type == tok_number || tokens[i].type == tok_true || tokens[i].type == tok_false || tokens[i].type == tok_nil);
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
                    std::string mem = tokens[i].value;
                    std::string type = e.name;
                    if (i + 1 < tokens.size() && tokens[i + 1].type == tok_column) {
                        i += 2;
                        FPResult r = parseType(tokens, i);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                    }
                    try {
                        e.addMember(mem, next, type);
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
            } else if (currentFunction != nullptr) {
                if (
                    token.type == tok_lambda &&
                    (((ssize_t) i) - 3 >= 0 && tokens[i - 3].type != tok_declare) &&
                    (((ssize_t) i) - 1 >= 0 && tokens[i - 1].type != tok_as) &&
                    (((ssize_t) i) - 1 >= 0 && tokens[i - 1].type != tok_column) &&
                    (((ssize_t) i) - 1 >= 0 && tokens[i - 1].type != tok_bracket_open) &&
                    (((ssize_t) i) - 2 >= 0 && tokens[i - 2].type != tok_new)
                ) isInLambda++;
                if (token.type == tok_unsafe) {
                    isInUnsafe++;
                }
                currentFunction->addToken(token);
            } else if (token.type == tok_paren_open) {
                currentFunction = new Function("$init" + std::to_string(nInits), Token(tok_identifier, "$init" + std::to_string(nInits), token.location));
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
                if (!currentStructs.empty()) {
                    currentFunction->member_type = currentStructs.back()->name;
                    currentFunction->name = currentFunction->member_type + "$" + currentFunction->name;
                    currentFunction->name_without_overload = currentFunction->member_type + "$" + currentFunction->name_without_overload;
                    for (auto&& s : currentStructs) {
                        currentFunction->addToken(s->name_token);
                        currentFunction->addToken(Token(tok_double_column, "::", tokens[i].location));
                    }
                }
                currentFunction->addToken(tokens[i]);
                i -= 2;
                functions.push_back(currentFunction);
                currentFunction = nullptr;
            } else if (token.type == tok_declare && !currentLayout.name.empty()) {
                if (i + 1 < tokens.size() && tokens[i + 1].type != tok_identifier) {
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
                Variable v(name, type, currentLayout.name);
                v.name_token = tokens[i];
                i++;
                size_t inlineArraySize = -1;
                if (tokens[i].type == tok_column) {
                    i++;
                    if (tokens[i].type == tok_number) {
                        inlineArraySize = parseNumber(tokens[i].value);
                        i++;
                    }
                    FPResult r = parseType(tokens, i);
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
                    v.type = type;
                } else {
                    FPResult result;
                    result.message = "Expected a type, but got '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                
                if (inlineArraySize != -1ULL) {
                    if (!typeIsConst(v.type)) {
                        FPResult result;
                        result.message = "Inline array must be declared 'const'";
                        result.value = v.name_token.value;
                        result.location = v.name_token.location;
                        result.success = false;
                        errors.push_back(result);
                        continue;
                    }

                    v.isVirtual = true;
                    std::string element = removeTypeModifiers(type);
                    element = element.substr(1, element.size() - 2);
                    for (size_t i = 0; i < inlineArraySize; i++) {
                        currentLayout.addMember(Variable(name + "$BACKER" + std::to_string(i), element, currentLayout.name));
                    }
                    Method* getter = new Method(currentLayout.name, "get" + capitalize(name), currentLayout.name_token);
                    getter->return_type = v.type;
                    getter->force_add = true;
                    getter->addModifier("nonvirtual");
                    getter->addModifier("@getter");
                    getter->addModifier(v.name);
                    getter->addArgument(Variable("self", currentLayout.name));
                    getter->addToken(Token(tok_addr_ref, "ref", currentLayout.name_token.location));
                    getter->addToken(Token(tok_identifier, "self", currentLayout.name_token.location));
                    getter->addToken(Token(tok_dot, ".", currentLayout.name_token.location));
                    getter->addToken(Token(tok_identifier, name + "$BACKER0", currentLayout.name_token.location));
                    getter->addToken(Token(tok_return, "return", currentLayout.name_token.location));
                    functions.push_back(getter);

                    lastDeclaredVariable = v;
                }

                currentLayout.addMember(v);
            } else if (token.type == tok_declare && currentStructs.empty()) {
                if (i + 1 < tokens.size() && tokens[i + 1].type != tok_identifier) {
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
                    FPResult r = parseType(tokens, i);
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
                v.name_token = tokens[start];
                v.hasInitializer = i >= 2 && tokens[start - 2].type == tok_store;
                if (contains<std::string>(nextAttributes, "expect")) {
                    v.isExtern = true;
                }
                auto it = std::find(globals.begin(), globals.end(), v);
                if (it == globals.end()) {
                    globals.push_back(v);
                } else if (it->isExtern) {
                    *it = v;
                } else if (!v.isExtern) {
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
                size_t inlineArraySize = -1;
                bool varIsStatic = currentStructs.back()->isStatic() || contains<std::string>(nextAttributes, "static");
                if (tokens[i].type == tok_column) {
                    i++;
                    if (tokens[i].type == tok_number && !varIsStatic) {
                        inlineArraySize = parseNumber(tokens[i].value);
                        i++;
                    }
                    FPResult r = parseType(tokens, i);
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
                } else {
                    FPResult result;
                    result.message = "Expected a type, but got '" + tokens[i].value + "'";
                    result.value = tokens[i].value;
                    result.location = tokens[i].location;
                    result.type = tokens[i].type;
                    result.success = false;
                    errors.push_back(result);
                    continue;
                }
                
                Variable& v = Variable::emptyVar();
                if (varIsStatic) {
                    v = Variable(currentStructs.back()->name + "$" + name, type, currentStructs.back()->name);
                    v.name_token = name_token;
                    v.hasInitializer = i >= 2 && tokens[start - 2].type == tok_store;
                    v.isPrivate = (isPrivate || contains<std::string>(nextAttributes, "private"));
                    if (contains<std::string>(nextAttributes, "expect")) {
                        v.isExtern = true;
                    }
                    nextAttributes.clear();
                    auto it = std::find(globals.begin(), globals.end(), v);
                    if (it == globals.end()) {
                        globals.push_back(v);
                    } else if (it->isExtern) {
                        *it = v;
                    } else if (!v.isExtern) {
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
                    v.name_token = name_token;
                    v.isPrivate = (isPrivate || contains<std::string>(nextAttributes, "private"));
                    if (inlineArraySize != -1ULL) {
                        if (!typeIsConst(v.type)) {
                            FPResult result;
                            result.message = "Inline array must be declared 'const'";
                            result.value = v.name_token.value;
                            result.location = v.name_token.location;
                            result.success = false;
                            errors.push_back(result);
                            continue;
                        }

                        v.isVirtual = true;
                        std::string element = removeTypeModifiers(type);
                        element = element.substr(1, element.size() - 2);
                        for (size_t i = 0; i < inlineArraySize; i++) {
                            currentStructs.back()->addMember(Variable(name + "$BACKER" + std::to_string(i), element, currentStructs.back()->name));
                        }
                        Method* getter = new Method(currentStructs.back()->name, "get" + capitalize(name), name_token);
                        getter->return_type = v.type;
                        getter->force_add = true;
                        getter->addModifier("@getter");
                        getter->addModifier(v.name);
                        getter->addArgument(Variable("self", currentStructs.back()->name));
                        getter->addToken(Token(tok_addr_ref, "ref", name_token.location));
                        getter->addToken(Token(tok_identifier, "self", name_token.location));
                        getter->addToken(Token(tok_dot, ".", name_token.location));
                        getter->addToken(Token(tok_identifier, name + "$BACKER0", name_token.location));
                        getter->addToken(Token(tok_return, "return", name_token.location));
                        functions.push_back(getter);
                    } else {
                        v.isVirtual = contains<std::string>(nextAttributes, "virtual");
                    }
                    currentStructs.back()->addMember(v);
                    nextAttributes.clear();
                }

                lastDeclaredVariable = v;
            } else {

                auto validAttribute = [](Token& t) -> bool {
                    return  t.type == tok_string_literal ||
                            t.type == tok_unsafe ||
                            t.type == tok_cdecl ||
                            (t.type == tok_identifier && (
                                t.value == "deprecated!" ||
                                t.value == "nonvirtual" ||
                                t.value == "construct" ||
                                t.value == "overrides" ||
                                t.value == "operator" ||
                                t.value == "foreign" ||
                                t.value == "virtual" ||
                                t.value == "default" ||
                                t.value == "private" ||
                                t.value == "reified" ||
                                t.value == "static" ||
                                t.value == "expect" ||
                                t.value == "sealed" ||
                                t.value == "const" ||
                                t.value == "final" ||
                                t.value == "async" ||
                                t.value == "open"
                            ));
                };

                if ((currentStructs.size() || !currentLayout.name.empty()) && currentFunction == nullptr && (tokens[i].value == "get" || tokens[i].value == "set")) {
                    std::string varName = lastDeclaredVariable.name;
                    std::string container;
                    if (currentStructs.size()) {
                        container = currentStructs.back()->name;
                    } else {
                        container = currentLayout.name;
                    }
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
                        
                        std::string name = "get";
                        name += (char) std::toupper(varName[0]);
                        name += varName.substr(1);

                        Method* getter = new Method(container, name, getToken);
                        getter->return_type = lastDeclaredVariable.type;
                        getter->force_add = true;
                        getter->addModifier("@getter");
                        getter->addModifier(varName);
                        getter->addArgument(Variable("self", container));
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

                        std::string name = "set";
                        name += (char) std::toupper(varName[0]);
                        name += varName.substr(1);

                        Method* setter = new Method(container, name, setToken);
                        setter->return_type = "none";
                        setter->force_add = true;
                        setter->addModifier("@setter");
                        setter->addModifier(varName);
                        setter->addArgument(Variable(argName, lastDeclaredVariable.type));
                        setter->addArgument(Variable("self", container));
                        currentFunction = setter;
                    }
                } else if (currentStructs.size() && currentFunction == nullptr && (tokens[i].value == "init" || tokens[i].value == "deinit")) {
                    std::string container = currentStructs.back()->name;
                    currentFunction = new Method(container, tokens[i].value, tokens[i]);
                    currentFunction->return_type = "none";
                    ((Method*) currentFunction)->force_add = true;
                    i++;
                    currentFunction = parseMethodArguments((Method*) currentFunction, errors, i, tokens);
                    i--;
                    currentFunction->deprecated = currentDeprecation;
                    currentDeprecation.clear();
                    for (std::string& s : nextAttributes) {
                        currentFunction->addModifier(s);
                    }
                    Function* f = findMethodByName(currentFunction->name, currentFunction->member_type);
                    if (f) {
                        currentFunction->name = currentFunction->name + "$$ol" + argsToRTSignatureIdent(currentFunction);
                    }
                    if (currentFunction->name_without_overload == "init") {
                        currentFunction->addModifier("<constructor>");
                    }
                    if (currentFunction->has_expect) {
                        functions.push_back(currentFunction);
                        currentFunction = nullptr;
                    }
                    nextAttributes.clear();
                } else if (tokens[i].value == "typealias") {
                    i++;
                    std::string type = tokens[i].value;
                    i++;
                    bool nilable = tokens[i].type == tok_question_mark;
                    if (nilable) i++;
                    if (tokens[i].type == tok_string_literal) {
                        std::string replacement = tokens[i].value;
                        typealiases[type] = std::pair(replacement, nilable);
                    } else if (tokens[i].type == tok_is) {
                        i++;
                        FPResult r = parseType(tokens, i);
                        if (!r.success) {
                            errors.push_back(r);
                            continue;
                        }
                        type = r.value;
                    } else {
                        FPResult result;
                        result.message = "Expected 'is' or string literal, but got '" + tokens[i].value + "'";
                        result.value = tokens[i].value;
                        result.location = tokens[i].location;
                        result.type = tokens[i].type;
                        result.success = false;
                        errors.push_back(result);
                    }
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
                } else if (tokens[i].type == tok_addr_of) {
                    i++;
                    if (tokens[i].value == "construct" && currentStructs.size()) {
                        nextAttributes.push_back("private");
                        nextAttributes.push_back("static");
                    }
                    nextAttributes.push_back(tokens[i].value);
                }
            }
        }

        for (Function* self : functions) {
            if (self->isMethod) {
                Function* f2 = self;
                Method* self = (Method*) f2;
                std::string name = self->name_without_overload;
                self->overloads.push_back(self);
                for (Function* f : functions) {
                    if (f == self) continue;
                    if (!f->isMethod) continue;
                    if (f->name_without_overload == name && ((Method*)f)->member_type == self->member_type) {
                        self->overloads.push_back(f);
                    }
                }
                continue;
            }
            std::string name = self->name_without_overload;
            self->overloads.push_back(self);
            for (Function* f : functions) {
                if (f == self) continue;
                if (f->isMethod) continue;
                if (f->name_without_overload == name || f->name == name) {
                    self->overloads.push_back(f);
                }
            }
        }

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
            return;
        }
        auto hasTypeAlias = [&](std::string name) -> bool {
            return result.typealiases.find(name) != result.typealiases.end();
        };
        auto createToStringMethod = [&](const Struct& s) -> Method* {
            Token t(tok_identifier, "toString", s.name_token.location);
            Method* toString = new Method(s.name, std::string("toString"), t);
            if (s.isExtern()) {
                toString->addModifier("expect");
            }
            std::string retemplate(std::string type);
            std::string stringify = retemplate(s.name) + " {";
            for (size_t i = 0; i < s.members.size(); i++) {
                auto member = s.members[i];
                if (i) {
                    stringify += ", ";
                }
                stringify += member.name + ": %s";
            }
            stringify += "}";
            toString->return_type = "str";
            toString->addModifier("<generated>");
            toString->addModifier("const");
            toString->addArgument(Variable("self", s.name));
            toString->addToken(Token(tok_string_literal, stringify, s.name_token.location));
            toString->addToken(Token(tok_varargs, "varargs", s.name_token.location));
            
            for (size_t i = 0; i < s.members.size(); i++) {
                auto member = s.members[i];
                
                auto emitMember = [&](const Variable& member) {
                    toString->addToken(Token(tok_identifier, "self", s.name_token.location));
                    toString->addToken(Token(tok_dot, ".", s.name_token.location));
                    toString->addToken(Token(tok_identifier, member.name, s.name_token.location));
                    bool canBeNil = (member.type.size() > 1 && member.type.back() == '?');
                    std::string type = removeTypeModifiers(member.type);
                    if (hasTypeAlias(type) || strstarts(type, "lambda(") || type == "lambda") {
                        toString->addToken(Token(tok_identifier, "any", s.name_token.location));
                        toString->addToken(Token(tok_double_column, "::", s.name_token.location));
                        toString->addToken(Token(tok_identifier, "toHexString", s.name_token.location));
                    } else {
                        if (canBeNil || type.front() == '*') {
                            toString->addToken(Token(tok_identifier, "builtinToString", s.name_token.location));
                        } else {
                            toString->addToken(Token(tok_column, ":", s.name_token.location));
                            toString->addToken(Token(tok_identifier, "toString", s.name_token.location));
                        }
                    }
                };

                if (strcontains(member.name, "$BACKER0")) {
                    toString->addToken(Token(tok_identifier, "self", s.name_token.location));
                    toString->addToken(Token(tok_dot, ".", s.name_token.location));
                    toString->addToken(Token(tok_identifier, member.name.substr(member.name.find('$')), s.name_token.location));
                    toString->addToken(Token(tok_column, ":", s.name_token.location));
                    toString->addToken(Token(tok_identifier, "toString", s.name_token.location));
                    
                    while (i < s.members.size() && strcontains(s.members[i].name, "$BACKER")) {
                        i++;
                    }
                } else {
                    emitMember(member);
                }
            }
            toString->addToken(Token(tok_identifier, "str", s.name_token.location));
            toString->addToken(Token(tok_double_column, "::", s.name_token.location));
            toString->addToken(Token(tok_identifier, "format", s.name_token.location));
            toString->addToken(Token(tok_return, "return", s.name_token.location));
            toString->force_add = true;
            return toString;
        };
        auto createToStringMethodLayout = [&](const Layout& s) -> Method* {
            Token t(tok_identifier, "toString", s.name_token.location);
            Method* toString = new Method(s.name, std::string("toString"), t);
            if (s.isExtern) {
                toString->addModifier("expect");
            }
            std::string retemplate(std::string type);
            std::string stringify = retemplate(s.name) + " {";
            for (size_t i = 0; i < s.members.size(); i++) {
                auto member = s.members[i];
                if (i) {
                    stringify += ", ";
                }
                stringify += member.name + ": %s";
            }
            stringify += "}";
            toString->return_type = "str";
            toString->addModifier("<generated>");
            toString->addModifier("const");
            toString->addModifier("nonvirtual");
            toString->addArgument(Variable("self", s.name));
            toString->addToken(Token(tok_string_literal, stringify, s.name_token.location));
            toString->addToken(Token(tok_varargs, "varargs", s.name_token.location));
            
            for (size_t i = 0; i < s.members.size(); i++) {
                auto member = s.members[i];
                
                auto emitMember = [&](const Variable& member) {
                    toString->addToken(Token(tok_identifier, "self", s.name_token.location));
                    toString->addToken(Token(tok_dot, ".", s.name_token.location));
                    toString->addToken(Token(tok_identifier, member.name, s.name_token.location));
                    bool canBeNil = (member.type.size() > 1 && member.type.back() == '?');
                    std::string type = removeTypeModifiers(member.type);
                    if (hasTypeAlias(type) || strstarts(type, "lambda(") || type == "lambda") {
                        toString->addToken(Token(tok_identifier, "any", s.name_token.location));
                        toString->addToken(Token(tok_double_column, "::", s.name_token.location));
                        toString->addToken(Token(tok_identifier, "toHexString", s.name_token.location));
                    } else {
                        if (canBeNil || type.front() == '*') {
                            toString->addToken(Token(tok_identifier, "builtinToString", s.name_token.location));
                        } else {
                            toString->addToken(Token(tok_column, ":", s.name_token.location));
                            toString->addToken(Token(tok_identifier, "toString", s.name_token.location));
                        }
                    }
                };

                if (strcontains(member.name, "$BACKER0")) {
                    toString->addToken(Token(tok_identifier, "self", s.name_token.location));
                    toString->addToken(Token(tok_dot, ".", s.name_token.location));
                    toString->addToken(Token(tok_identifier, member.name.substr(member.name.find('$')), s.name_token.location));
                    toString->addToken(Token(tok_column, ":", s.name_token.location));
                    toString->addToken(Token(tok_identifier, "toString", s.name_token.location));
                    
                    while (i < s.members.size() && strcontains(s.members[i].name, "$BACKER")) {
                        i++;
                    }
                } else {
                    emitMember(member);
                }
            }
            toString->addToken(Token(tok_identifier, "str", s.name_token.location));
            toString->addToken(Token(tok_double_column, "::", s.name_token.location));
            toString->addToken(Token(tok_identifier, "format", s.name_token.location));
            toString->addToken(Token(tok_return, "return", s.name_token.location));
            toString->force_add = true;
            return toString;
        };
        auto createToStringMethodEnum = [&](const Enum& s) -> Method* {
            Token t(tok_identifier, "toString", s.name_token.location);
            Method* toString = new Method(s.name, std::string("toString"), t);
            if (s.isExtern) {
                toString->addModifier("expect");
            }
            std::string retemplate(std::string);
            toString->return_type = "str";
            toString->addModifier("<generated>");
            toString->addModifier("const");
            toString->addModifier("nonvirtual");
            toString->addArgument(Variable("self", s.name));
            toString->addToken(Token(tok_identifier, "self", s.name_token.location));
            toString->addToken(Token(tok_switch, "switch", s.name_token.location));

            for (auto&& member : s.members) {
                toString->addToken(Token(tok_case, "case", s.name_token.location));
                toString->addToken(Token(tok_identifier, s.name, s.name_token.location));
                toString->addToken(Token(tok_double_column, "::", s.name_token.location));
                toString->addToken(Token(tok_identifier, member.first, s.name_token.location));
                toString->addToken(Token(tok_string_literal, retemplate(s.name) + "::" + member.first, s.name_token.location));
                toString->addToken(Token(tok_return, "return", s.name_token.location));
                toString->addToken(Token(tok_esac, "esac", s.name_token.location));
            }
            toString->addToken(Token(tok_end, "end", s.name_token.location));
            toString->addToken(Token(tok_identifier, "builtinUnreachable", s.name_token.location));
            toString->force_add = true;
            return toString;
        };
        auto createOrdinalMethod = [&](const Enum& s) -> Method* {
            Token t(tok_identifier, "ordinal", s.name_token.location);
            Method* ordinal = new Method(s.name, std::string("ordinal"), t);
            if (s.isExtern) {
                ordinal->addModifier("expect");
            }
            ordinal->return_type = "int";
            ordinal->addModifier("<generated>");
            ordinal->addModifier("const");
            ordinal->addModifier("nonvirtual");
            ordinal->addArgument(Variable("self", s.name));
            ordinal->addToken(Token(tok_identifier, "self", s.name_token.location));
            ordinal->addToken(Token(tok_as, "as", s.name_token.location));
            ordinal->addToken(Token(tok_identifier, "const", s.name_token.location));
            ordinal->addToken(Token(tok_identifier, "int", s.name_token.location));
            ordinal->addToken(Token(tok_return, "return", s.name_token.location));
            ordinal->force_add = true;
            return ordinal;
        };

        for (const Struct& s : result.structs) {
            if (s.isStatic()) continue;
            bool hasImplementedToString = false;
            Method* toString = nullptr;
            for (auto x : methodsOnType(result, s.name)) {
                if (x->name_without_overload == "toString") {
                    toString = x;
                    break;
                }
            }
            if (toString == nullptr || contains<std::string>(toString->modifiers, "<generated>")) {
                if (s.super != "Union") {
                    result.functions.push_back(createToStringMethod(s));
                    hasImplementedToString = true;
                }
            }

            for (auto&& toImplement : s.toImplementFunctions) {
                if (!hasImplementedToString && toImplement == "toString") {
                    result.functions.push_back(createToStringMethod(s));
                }
            }

            if (s.isFinal()) {
                for (auto&& f : result.functions) {
                    if (!f->isMethod || f->has_nonvirtual || f->member_type != s.name) continue;

                    f->addModifier("nonvirtual");
                }
            }
        }
        for (const Layout& s : result.layouts) {
            Method* toString = nullptr;
            for (auto x : methodsOnType(result, s.name)) {
                if (x->name_without_overload == "toString") {
                    toString = x;
                    break;
                }
            }
            if (toString == nullptr || contains<std::string>(toString->modifiers, "<generated>")) {
                result.functions.push_back(createToStringMethodLayout(s));
            }
        }
        for (const Enum& s : result.enums) {
            Method* toString = nullptr;
            for (auto x : methodsOnType(result, s.name)) {
                if (x->name_without_overload == "toString") {
                    toString = x;
                    break;
                }
            }
            Method* ordinal = nullptr;
            for (auto x : methodsOnType(result, s.name)) {
                if (x->name_without_overload == "ordinal") {
                    ordinal = x;
                    break;
                }
            }
            
            if (toString == nullptr || contains<std::string>(toString->modifiers, "<generated>")) {
                result.functions.push_back(createToStringMethodEnum(s));
            }
            if (ordinal == nullptr || contains<std::string>(ordinal->modifiers, "<generated>")) {
                result.functions.push_back(createOrdinalMethod(s));
            }
        }
        if (Main::options::Werror) {
            for (auto&& warn : result.warns) {
                result.errors.push_back(warn);
            }
            result.warns.clear();
        }

        return;
    }

    std::pair<const std::string, std::string> emptyPair("", "");

    std::pair<const std::string, std::string>& pairAt(std::unordered_map<std::string, std::string> map, size_t index) {
        size_t i = 0;
        for (auto&& pair : map) {
            if (i == index) {
                return pair;
            }
            i++;
        }
        return emptyPair;
    }
}
