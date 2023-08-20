#include "headers/Common.hpp"

#if !defined(_WIN32) && !defined(__wasm__)
#include <execinfo.h>
#endif

#include <unordered_set>
#include <stack>

namespace sclc
{
    #ifdef _WIN32
    std::string Color::RESET = "";
    std::string Color::BLACK = "";
    std::string Color::RED = "";
    std::string Color::GREEN = "";
    std::string Color::YELLOW = "";
    std::string Color::BLUE = "";
    std::string Color::MAGENTA = "";
    std::string Color::CYAN = "";
    std::string Color::WHITE = "";
    std::string Color::BOLDBLACK = "";
    std::string Color::BOLDRED = "";
    std::string Color::BOLDGREEN = "";
    std::string Color::BOLDYELLOW = "";
    std::string Color::BOLDBLUE = "";
    std::string Color::BOLDMAGENTA = "";
    std::string Color::BOLDCYAN = "";
    std::string Color::BOLDWHITE = "";
    #else
    std::string Color::RESET = "\033[0m";
    std::string Color::BLACK = "\033[30m";
    std::string Color::RED = "\033[31m";
    std::string Color::GREEN = "\033[32m";
    std::string Color::YELLOW = "\033[33m";
    std::string Color::BLUE = "\033[34m";
    std::string Color::MAGENTA = "\033[35m";
    std::string Color::CYAN = "\033[36m";
    std::string Color::WHITE = "\033[37m";
    std::string Color::BOLDBLACK = "\033[1m\033[30m";
    std::string Color::BOLDRED = "\033[1m\033[31m";
    std::string Color::BOLDGREEN = "\033[1m\033[32m";
    std::string Color::BOLDYELLOW = "\033[1m\033[33m";
    std::string Color::BOLDBLUE = "\033[1m\033[34m";
    std::string Color::BOLDMAGENTA = "\033[1m\033[35m";
    std::string Color::BOLDCYAN = "\033[1m\033[36m";
    std::string Color::BOLDWHITE = "\033[1m\033[37m";
    #endif

    std::vector<std::string> keywords({
        "import",
        "construct",
        "final",
        "ref",
        "typeof",
        "nameof",
        "sizeof",
        "typealias",
        "layout",
        "asm",
        "nothing",
        "none",
        "int",
        "float",
        "int32",
        "int16",
        "int8",
        "uint32",
        "uint16",
        "uint8",
        "uint",
        "int64",
        "uint64",
        "any",
        "str",
        "varargs",
        "self",
        "super",
        "bool",
        "sealed",
        "static",
        "private",
        "export",
        "expect",
        "unsafe",
        "restrict",
        "const",
        "readonly",
        "mut",
        "try",
        "catch",
        "throw",
        "open",
        "intrinsic",
        "lambda",
    });

    Struct Struct::Null = Struct("");
    Token Token::Default(tok_identifier, "", 0, "");

    std::vector<Variable> vars;
    std::vector<size_t> var_indices;
    _Main Main = _Main();

    void print_trace(void) {
#if !defined(_WIN32) && !defined(__wasm__)
        void* array[64];
        char** strings;
        int size, i;

        size = backtrace(array, 64);
        strings = backtrace_symbols(array, size);
        if (strings != NULL) {
            for (i = 0; i < size; i++)
                fprintf(stderr, "  %s\n", strings[i]);
        }

        free(strings);
#endif
    }

    void signalHandler(int signum) {
        std::cerr << "Signal " << signum << " received." << std::endl;
        if (errno != 0) std::cerr << "Error: " << std::strerror(errno) << std::endl;
        print_trace();
        exit(signum);
    }

    bool strends(const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
    }

    bool strstarts(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
    }

    bool strcontains(const std::string& str, const std::string& substr) {
        return str.find(substr) != std::string::npos;
    }

    int isCharacter(char c) {
        return (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '_';
    }

    int isDigit(char c) {
        return c >= '0' &&
            c <= '9';
    }

    int isSpace(char c) {
        return c == ' ' ||
            c == '\t' ||
            c == '\n' ||
            c == '\r';
    }

    int isPrint(char c) {
        return (c >= ' ');
    }

    int isBracket(char c) {
        return c == '(' ||
            c == ')' ||
            c == '{' ||
            c == '}' ||
            c == '[' ||
            c == ']';
    }

    int isHexDigit(char c) {
        return (c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F') ||
            c == 'x' ||
            c == 'X';
    }

    int isOctDigit(char c) {
        return (c >= '0' && c <= '7') ||
            c == 'o' ||
            c == 'O';
    }

    int isBinDigit(char c) {
        return c == '0' ||
            c == '1' ||
            c == 'b' ||
            c == 'B';
    }

    int isOperator(char c) {
        return c == '?' ||
            c == '@' ||
            c == '(' ||
            c == ')' ||
            c == ',' ||
            c == '+' ||
            c == '-' ||
            c == '*' ||
            c == '/' ||
            c == '%' ||
            c == '&' ||
            c == '|' ||
            c == '^' ||
            c == '~' ||
            c == '<' ||
            c == '>' ||
            c == ':' ||
            c == '=' ||
            c == '!';
    }

    bool fileExists(const std::string& name) {
        FILE *file;
        if ((file = fopen(name.c_str(), "r")) != NULL) {
            fclose(file);
            return true;
        } else {
            return false;
        }
    }

    void addIfAbsent(std::vector<Function*>& vec, Function* str) {
        for (size_t i = 0; i < vec.size(); i++) {
            if (str->isMethod && vec[i]->isMethod) {
                if (vec[i]->name == str->name && vec[i]->member_type == str->member_type) {
                    return;
                }
            } else if (!str->isMethod && !vec[i]->isMethod) {
                if (*vec[i] == *str) {
                    return;
                }
            }
        }
        vec.push_back(str);
    }

    std::string replaceAll(const std::string& src, const std::string& from, const std::string& to) {
        try {
            return regex_replace(src, std::regex(from), to);
        } catch (std::regex_error& e) {
            return src;
        }
    }

    std::string replaceFirstAfter(const std::string& src, const std::string& from, const std::string& to, int index) {
        try {
            size_t actual = src.find(from, index - 1);
            if (actual == std::string::npos) {
                return src;
            }
            std::string pre = src.substr(0, actual);
            std::string post = src.substr(actual + from.length());
            return pre + to + post;
        } catch (std::out_of_range const&) {
            return src;
        }
    }

    int lastIndexOf(char* src, char c) {
        int i = strlen(src) - 1;
        while (i >= 0 && src[i] != c) {
            i--;
        }
        return i;
    }

    bool hasVar(const std::string& name) {
        for (auto&& v : vars) {
            if (v.name == name) {
                return true;
            }
        }
        return false;
    }

    Variable getVar(const std::string& name) {
        for (auto&& v : vars) {
            if (v.name == name) {
                return v;
            }
        }
        return Variable("", "");
    }

    long long parseNumber(std::string str) {
        long long value;
        if (str.substr(0, 2) == "0x") {
            value = std::stoll(str.substr(2), nullptr, 16);
        } else if (str.substr(0, 2) == "0b") {
            value = std::stoll(str.substr(2), nullptr, 2);
        } else if (str.substr(0, 2) == "0o") {
            value = std::stoll(str.substr(2), nullptr, 8);
        } else {
            value = std::stoll(str);
        }
        return value;
    }

    Result<double, FPResult> parseDouble(const Token& tok) {
        double num;
        try
        {
            num = std::stold(tok.value);
        }
        catch(const std::exception& e)
        {
            FPResult r;
            r.message = "Number out of range: " + tok.value;
            r.column = tok.column;
            r.type = tok.type;
            r.in = tok.file;
            r.line = tok.line;
            r.value = tok.value;
            r.success = false;
            return Result<double, FPResult>(r);
        }
        return num;
    }

    FPResult parseType(std::vector<Token>& body, size_t* i, const std::map<std::string, std::string>& typeReplacements) {
        FPResult r;
        r.success = true;
        r.value = "";
        r.line = body[*i].line;
        r.in = body[*i].file;
        r.column = body[*i].column;
        r.value = body[*i].value;
        r.type = body[*i].type;
        r.message = "";
        r.value = "";
        std::string type_mods = "";
        bool isConst = false;
        bool isMut = false;
        bool isReadonly = false;

        if (body[*i].type == tok_identifier && body[*i].value == "*") {
            type_mods += "*";
            (*i)++;
        }

        while (body[(*i)].value == "const" || body[(*i)].value == "mut" || body[(*i)].value == "readonly") {
            if (!isConst && body[(*i)].value == "const") {
                type_mods += "const ";
                isConst = true;
                if (isMut) {
                    r.success = false;
                    r.line = body[*i].line;
                    r.in = body[*i].file;
                    r.column = body[*i].column;
                    r.value = body[*i].value;
                    r.type = body[*i].type;
                    r.message = "Cannot specify 'const' on already 'mut' type!";
                }
            } else if (!isMut && body[(*i)].value == "mut") {
                type_mods += "mut ";
                isMut = true;
                if (isConst) {
                    r.success = false;
                    r.line = body[*i].line;
                    r.in = body[*i].file;
                    r.column = body[*i].column;
                    r.value = body[*i].value;
                    r.type = body[*i].type;
                    r.message = "Cannot specify 'mut' on already 'const' type!";
                }
            } else if (!isReadonly && body[(*i)].value == "readonly") {
                type_mods += "readonly ";
                isReadonly = true;
            }
            (*i)++;
        }
        if (body[*i].type == tok_identifier) {
            r.value = type_mods + body[*i].value;
            if (typeReplacements.find(body[*i].value) != typeReplacements.end()) {
                r.value = type_mods + typeReplacements.at(body[*i].value);
                r.message = body[*i].value;
            }
            if (r.value == "lambda") {
                (*i)++;
                if (body[*i].type != tok_paren_open) {
                    (*i)--;
                } else {
                    (*i)++;
                    r.value += "(";
                    int count = 0;
                    while (body[*i].type != tok_paren_close) {
                        FPResult tmp = parseType(body, i);
                        if (!tmp.success) return tmp;
                        (*i)++;
                        if (body[*i].type == tok_comma) {
                            (*i)++;
                        }
                        count++;
                    }
                    (*i)++;
                    r.value += std::to_string(count) + ")";
                    if (body[*i].type == tok_column) {
                        (*i)++;
                        FPResult tmp = parseType(body, i);
                        if (!tmp.success) return tmp;
                        r.value += ":" + tmp.value;
                    } else {
                        (*i)--;
                        r.value += ":none";
                    }
                }
            }
        } else if (body[*i].type == tok_question_mark || body[*i].value == "?") {
            r.value = type_mods + "?";
        } else if (body[*i].type == tok_bracket_open) {
            std::string type = "[";
            std::string size = "";
            (*i)++;
            if (body[*i].type == tok_number) {
                size = body[*i].value;
                (*i)++;
            }
            r = parseType(body, i);
            if (!r.success) {
                return r;
            }
            type += r.value;
            if (size.size()) {
                type += ";" + size;
            }
            (*i)++;
            if (body[*i].type == tok_bracket_close) {
                type += "]";
                r.value = type_mods + type;
            } else {
                r.message = "Expected ']', but got '" + body[*i].value + "'";
                r.value = type_mods + body[*i].value;
                r.line = body[*i].line;
                r.in = body[*i].file;
                r.type = body[*i].type;
                r.success = false;
            }
        } else {
            r.success = false;
            r.line = body[*i].line;
            r.in = body[*i].file;
            r.column = body[*i].column;
            r.value = body[*i].value;
            r.type = body[*i].type;
            r.message = "Unexpected token: '" + body[*i].tostring() + "'";
        }
        if (body[(*i + 1)].value == "?") {
            (*i)++;
            r.value += "?";
        }
        return r;
    }
    
    bool checkStackType(TPResult& result, std::vector<Variable>& args, bool allowIntPromotion = false);

    Function* getFunctionByNameWithArgs(TPResult& result, const std::string& name2, bool doCheck) {
        (void) doCheck;

        std::string name;
        if (name2 == "+") name = "operator$add";
        else if (name2 == "-") name = "operator$sub";
        else if (name2 == "*") name = "operator$mul";
        else if (name2 == "/") name = "operator$div";
        else if (name2 == "%") name = "operator$mod";
        else if (name2 == "&") name = "operator$logic_and";
        else if (name2 == "|") name = "operator$logic_or";
        else if (name2 == "^") name = "operator$logic_xor";
        else if (name2 == "~") name = "operator$logic_not";
        else if (name2 == "<<") name = "operator$logic_lsh";
        else if (name2 == "<<<") name = "operator$logic_rol";
        else if (name2 == ">>") name = "operator$logic_rsh";
        else if (name2 == ">>>") name = "operator$logic_ror";
        else if (name2 == "**") name = "operator$pow";
        else if (name2 == ".") name = "operator$dot";
        else if (name2 == "<") name = "operator$less";
        else if (name2 == "<=") name = "operator$less_equal";
        else if (name2 == ">") name = "operator$more";
        else if (name2 == ">=") name = "operator$more_equal";
        else if (name2 == "==") name = "operator$equal";
        else if (name2 == "!") name = "operator$not";
        else if (name2 == "!!") name = "operator$assert_not_nil";
        else if (name2 == "!=") name = "operator$not_equal";
        else if (name2 == "&&") name = "operator$bool_and";
        else if (name2 == "||") name = "operator$bool_or";
        else if (name2 == "++") name = "operator$inc";
        else if (name2 == "--") name = "operator$dec";
        else if (name2 == "@") name = "operator$at";
        else if (name2 == "=>") name = "operator$store";
        else if (name2 == "=>[]") name = "operator$set";
        else if (name2 == "[]") name = "operator$get";
        else if (name2 == "?") name = "operator$wildcard";
        else name = name2;

        for (Function* func : result.functions) {
            if (func == nullptr) continue;
            if (func->isMethod) continue;
            if (func->name == name) {
                return func;
            }
        }
        return nullptr;
    }

    Function* getFunctionByName(TPResult& result, const std::string& name) {
        return getFunctionByNameWithArgs(result, name, false);
    }

    bool hasEnum(TPResult& result, const std::string& name) {
        return getEnumByName(result, name).name.size() != 0;
    }

    Enum getEnumByName(TPResult& result, const std::string& name) {
        for (Enum& e : result.enums) {
            if (e.name == name) {
                return e;
            }
        }
        return Enum("");
    }
    
    Interface* getInterfaceByName(TPResult& result, const std::string& name) {
        for (Interface* i : result.interfaces) {
            if (i->name == name) {
                return i;
            }
        }
        return nullptr;
    }

    Method* getMethodWithActualName(TPResult& result, const std::string& name, const std::string& type, bool doCheck) {
        if (type == "") {
            return nullptr;
        }

        for (Function* func : result.functions) {
            if (!func->isMethod) continue;
            if (func->name == name && func->member_type == type) {
                return (Method*) func;
            }
        }
        Struct s = getStructByName(result, type);
        if (getInterfaceByName(result, type)) {
            return nullptr;
        }
        return getMethodByNameWithArgs(result, name, s.super, doCheck);
    }

    Method* getMethodByNameWithArgs(TPResult& result, const std::string& name2, const std::string& type2, bool doCheck) {
        (void) doCheck;
        std::string type = removeTypeModifiers(type2);

        std::string name;
        if (name2 == "+") name = "operator$add";
        else if (name2 == "-") name = "operator$sub";
        else if (name2 == "*") name = "operator$mul";
        else if (name2 == "/") name = "operator$div";
        else if (name2 == "%") name = "operator$mod";
        else if (name2 == "&") name = "operator$logic_and";
        else if (name2 == "|") name = "operator$logic_or";
        else if (name2 == "^") name = "operator$logic_xor";
        else if (name2 == "~") name = "operator$logic_not";
        else if (name2 == "<<") name = "operator$logic_lsh";
        else if (name2 == "<<<") name = "operator$logic_rol";
        else if (name2 == ">>") name = "operator$logic_rsh";
        else if (name2 == ">>>") name = "operator$logic_ror";
        else if (name2 == "**") name = "operator$pow";
        else if (name2 == ".") name = "operator$dot";
        else if (name2 == "<") name = "operator$less";
        else if (name2 == "<=") name = "operator$less_equal";
        else if (name2 == ">") name = "operator$more";
        else if (name2 == ">=") name = "operator$more_equal";
        else if (name2 == "==") name = "operator$equal";
        else if (name2 == "!") name = "operator$not";
        else if (name2 == "!!") name = "operator$assert_not_nil";
        else if (name2 == "!=") name = "operator$not_equal";
        else if (name2 == "&&") name = "operator$bool_and";
        else if (name2 == "||") name = "operator$bool_or";
        else if (name2 == "++") name = "operator$inc";
        else if (name2 == "--") name = "operator$dec";
        else if (name2 == "@") name = "operator$at";
        else if (name2 == "=>") name = "operator$store";
        else if (name2 == "=>[]") name = "operator$set";
        else if (name2 == "[]") name = "operator$get";
        else if (name2 == "?") name = "operator$wildcard";
        else name = name2;
        name = name.substr(0, name.find("$$ol"));

        return getMethodWithActualName(result, name, type, doCheck);
    }

    Method* getMethodByName(TPResult& result, const std::string& name, const std::string& type) {
        return getMethodByNameWithArgs(result, name, type, false);
    }
    Method* getMethodByNameOnThisType(TPResult& result, const std::string& name, const std::string& type) {
        Method* method = getMethodByName(result, name, type);
        return (method == nullptr || method->member_type != removeTypeModifiers(type)) ? nullptr : method;
    }

    std::vector<Method*> methodsOnType(TPResult& res, std::string type) {
        type = removeTypeModifiers(type);

        std::vector<Method*> methods;
        methods.reserve(res.functions.size());

        for (Function* func : res.functions) {
            if (!func->isMethod) continue;
            if (((Method*) func)->member_type == type) {
                methods.push_back((Method*) func);
            }
        }
        return methods;
    }

    Container EmptyContainer = Container("");

    Container getContainerByName(TPResult& result, const std::string& name) {
        for (Container& container : result.containers) {
            if (container.name == name) {
                return container;
            }
        }
        return EmptyContainer;
    }

    std::string removeTypeModifiers(std::string t);

#define debugDump(_var) std::cout << #_var << ": " << _var << std::endl

    const Struct& getStructByName(TPResult& result, const std::string& name) {
        const std::string& typeName = removeTypeModifiers(name);
        for (const Struct& s : result.structs) {
            if (s.name == typeName) {
                return s;
            }
        }
        return Struct::Null;
    }

    Layout EmptyLayout = Layout("");

    Layout getLayout(TPResult& result, const std::string& name) {
        const std::string& typeName = removeTypeModifiers(name);
        for (Layout& layout : result.layouts) {
            if (layout.name == typeName) {
                return layout;
            }
        }
        return EmptyLayout;
    }

    bool hasLayout(TPResult& result, const std::string& name) {
        return getLayout(result, name).name.size() != 0;
    }

    bool hasFunction(TPResult& result, const std::string& name2) {
        std::string name;

        if (name2 == "+") name = "operator$add";
        else if (name2 == "-") name = "operator$sub";
        else if (name2 == "*") name = "operator$mul";
        else if (name2 == "/") name = "operator$div";
        else if (name2 == "%") name = "operator$mod";
        else if (name2 == "&") name = "operator$logic_and";
        else if (name2 == "|") name = "operator$logic_or";
        else if (name2 == "^") name = "operator$logic_xor";
        else if (name2 == "~") name = "operator$logic_not";
        else if (name2 == "<<") name = "operator$logic_lsh";
        else if (name2 == "<<<") name = "operator$logic_rol";
        else if (name2 == ">>") name = "operator$logic_rsh";
        else if (name2 == ">>>") name = "operator$logic_ror";
        else if (name2 == "**") name = "operator$pow";
        else if (name2 == ".") name = "operator$dot";
        else if (name2 == "<") name = "operator$less";
        else if (name2 == "<=") name = "operator$less_equal";
        else if (name2 == ">") name = "operator$more";
        else if (name2 == ">=") name = "operator$more_equal";
        else if (name2 == "==") name = "operator$equal";
        else if (name2 == "!") name = "operator$not";
        else if (name2 == "!!") name = "operator$assert_not_nil";
        else if (name2 == "!=") name = "operator$not_equal";
        else if (name2 == "&&") name = "operator$bool_and";
        else if (name2 == "||") name = "operator$bool_or";
        else if (name2 == "++") name = "operator$inc";
        else if (name2 == "--") name = "operator$dec";
        else if (name2 == "@") name = "operator$at";
        else if (name2 == "=>") name = "operator$store";
        else if (name2 == "=>[]") name = "operator$set";
        else if (name2 == "[]") name = "operator$get";
        else if (name2 == "?") name = "operator$wildcard";
        else name = name2;

        for (Function* func : result.functions) {
            if (func->isMethod) continue;
            std::string funcName = func->name.substr(0, func->name.find("$$ol"));
            if (funcName == name) {
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> supersToVector(TPResult& r, Struct& s) {
        std::vector<std::string> v;
        v.reserve(16);
        v.push_back(s.name);
        Struct super = getStructByName(r, s.super);
        while (super.name.size()) {
            v.push_back(super.name);
            super = getStructByName(r, super.super);
        }
        return v;
    }
    std::string supersToHashedCList(TPResult& r, Struct& s) {
        std::string list = "(scl_int[]) {";
        std::vector<std::string> v = supersToVector(r, s);
        for (size_t i = 0; i < v.size(); i++) {
            if (i) {
                list += ", ";
            }
            std::string super = v[i];
            list += std::to_string(id(super.c_str()));
        }
        list += "}";
        return list;
    }
    std::string supersToCList(TPResult& r, Struct& s) {
        std::string list = "(scl_str*) {\"" + s.name + "\"";
        Struct super = getStructByName(r, s.super);
        while (super.name.size()) {
            list += "\"" + super.name + "\"";
            super = getStructByName(r, super.super);
            if (super.name.size()) {
                list += ", ";
            }
        }
        list += "}";
        return list;
    }

    bool hasMethod(TPResult& result, const std::string& name, const std::string& type) {
        return getMethodByName(result, name, type) != nullptr;
    }

    bool hasContainer(TPResult& result, std::string name) {
        for (Container& container_ : result.containers) {
            if (container_.name == name) {
                return true;
            }
        }
        return false;
    }

    bool hasGlobal(TPResult& result, std::string name) {
        for (Variable& v : result.globals) {
            if (v.name == name) {
                return true;
            }
        }
        for (Variable& v : result.extern_globals) {
            if (v.name == name) {
                return true;
            }
        }
        return false;
    }

    bool isInitFunction(Function* f) {
        if (strstarts(f->name, "__init__")) {
            return true;
        }
        if (f->isMethod) {
            Method* m = static_cast<Method*>(f);
            return m->has_constructor || strstarts(m->name, "init");
        } else {
            return f->has_construct;
        }
    }

    bool isDestroyFunction(Function* f) {
        return f->has_final || strstarts(f->name, "__destroy__");
    }

    bool memberOfStruct(const Variable* self, Function* f) {
        return f->member_type == self->internalMutableFrom || strstarts(self->name, f->member_type + "$");
    }

    bool Variable::isAccessible(Function* f) const {
        if (this->isPrivate) {
            return memberOfStruct(this, f);
        }
        return true;
    }

    bool Variable::isWritableFrom(Function* f) const {
        if (this->isReadonly || this->isPrivate) {
            if (this->isReadonly && strstarts(this->name, f->member_type + "$")) {
                return true;
            }
            return memberOfStruct(this, f);
        }
        if (isConst) {
            return isInitFunction(f);
        }
        return true;
    }

    bool sclIsProhibitedInit(std::string s) {
        // Kept because i am too lazy to refactor
        (void) s;
        return false;
    }

    bool typeCanBeNil(std::string s) {
        return isPrimitiveType(s) || (s.size() > 1 && s.back() == '?');
    }

    bool typeIsReadonly(std::string s) {
        if (s.front() == '*') s.erase(0, 1);
        while (strstarts(s, "mut ") || strstarts(s, "const ")) {
            s.erase(0, strstarts(s, "mut ") ? 4 : 6);
        }
        return strstarts(s, "readonly ");
    }

    bool typeIsConst(std::string s) {
        if (s.front() == '*') s.erase(0, 1);
        while (strstarts(s, "mut ") || strstarts(s, "readonly ")) {
            s.erase(0, strstarts(s, "mut ") ? 4 : 9);
        }
        return strstarts(s, "const ");
    }

    bool typeIsMut(std::string s) {
        if (s.front() == '*') s.erase(0, 1);
        while (strstarts(s, "const ") || strstarts(s, "readonly ")) {
            s.erase(0, strstarts(s, "const ") ? 6 : 9);
        }
        return strstarts(s, "mut ");
    }

    bool isPrimitiveIntegerType(std::string type);
    
    bool isPrimitiveType(std::string s) {
        s = removeTypeModifiers(s);
        return isPrimitiveIntegerType(s) || s == "float" || s == "any" || s == "bool";
    }

    bool featureEnabled(std::string feat) {
        return contains<std::string>(Main.options.features, feat);
    }

    time_t file_modified_time(const std::string& path) {
        struct stat attr;
        stat(path.c_str(), &attr);
        return attr.st_mtime;
    }

    ID_t id(const char* data) {
        if (strlen(data) == 0) return 0;
        ID_t h = 3323198485UL;
        for (;*data;++data) {
            h ^= *data;
            h *= 0x5BD1E995;
            h ^= h >> 15;
        }
        return h;
    }

    std::string& operator*(std::string& toRepeat, const size_t count) {
        if (count == 0) {
            toRepeat.clear();
            return toRepeat;
        }
        if (count == 1) {
            return toRepeat;
        }
        const size_t oldSize = toRepeat.size();
        const size_t newSize = oldSize * count;
        toRepeat.resize(newSize);
        for (size_t i = 1; i < count; i++) {
            memcpy(&toRepeat[0] + (i * oldSize), &toRepeat[0], oldSize);
        }
        return toRepeat;
    }
} // namespace sclc
