#include "headers/Common.hpp"

#if !defined(_WIN32) && !defined(__wasm__)
#include <execinfo.h>
#endif

#include <unordered_set>

namespace sclc
{
    #ifdef _WIN32
    const std::string Color::RESET = "";
    const std::string Color::BLACK = "";
    const std::string Color::RED = "";
    const std::string Color::GREEN = "";
    const std::string Color::YELLOW = "";
    const std::string Color::BLUE = "";
    const std::string Color::MAGENTA = "";
    const std::string Color::CYAN = "";
    const std::string Color::WHITE = "";
    const std::string Color::BOLDBLACK = "";
    const std::string Color::BOLDRED = "";
    const std::string Color::BOLDGREEN = "";
    const std::string Color::BOLDYELLOW = "";
    const std::string Color::BOLDBLUE = "";
    const std::string Color::BOLDMAGENTA = "";
    const std::string Color::BOLDCYAN = "";
    const std::string Color::BOLDWHITE = "";
    #else
    const std::string Color::RESET = "\033[0m";
    const std::string Color::BLACK = "\033[30m";
    const std::string Color::RED = "\033[31m";
    const std::string Color::GREEN = "\033[32m";
    const std::string Color::YELLOW = "\033[33m";
    const std::string Color::BLUE = "\033[34m";
    const std::string Color::MAGENTA = "\033[35m";
    const std::string Color::CYAN = "\033[36m";
    const std::string Color::WHITE = "\033[37m";
    const std::string Color::BOLDBLACK = "\033[1m\033[30m";
    const std::string Color::BOLDRED = "\033[1m\033[31m";
    const std::string Color::BOLDGREEN = "\033[1m\033[32m";
    const std::string Color::BOLDYELLOW = "\033[1m\033[33m";
    const std::string Color::BOLDBLUE = "\033[1m\033[34m";
    const std::string Color::BOLDMAGENTA = "\033[1m\033[35m";
    const std::string Color::BOLDCYAN = "\033[1m\033[36m";
    const std::string Color::BOLDWHITE = "\033[1m\033[37m";
    #endif

    Struct Struct::Null = Struct("");

    std::vector<std::vector<Variable>> vars;
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
                printf("  %s\n", strings[i]);
        }

        free(strings);
#endif
    }

    void signalHandler(int signum) {
        std::cout << "Signal " << signum << " received." << std::endl;
        if (errno != 0) std::cout << "Error: " << std::strerror(errno) << std::endl;
        print_trace();
        exit(signum);
    }

    bool strends(const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
    }

    bool strstarts(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
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

    bool isOperator(Token token) {
        TokenType type = token.getType();
        return type == tok_add || type == tok_sub || type == tok_mul || type == tok_div || type == tok_mod || type == tok_land || type == tok_lor || type == tok_lxor || type == tok_lnot || type == tok_lsh || type == tok_rsh || type == tok_pow;
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
                if (vec[i]->getName() == str->getName() && static_cast<Method*>(vec[i])->getMemberType() == static_cast<Method*>(str)->getMemberType()) {
                    // std::cout << "Method " << str->getName() << " is " << static_cast<Method*>(vec[i])->getMemberType() << std::endl;
                    return;
                }
            } else if (!str->isMethod && !vec[i]->isMethod) {
                if (*vec[i] == *str) {
                    // std::cout << "Function " << str->getName() << " exists" << std::endl;
                    return;
                }
            }
        }
        vec.push_back(str);
    }

    std::string replaceAll(std::string src, std::string from, std::string to) {
        try {
            return regex_replace(src, std::regex(from), to);
        } catch (std::regex_error& e) {
            return src;
        }
    }

    std::string replaceFirstAfter(std::string src, std::string from, std::string to, int index) {
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

    bool hasVar(std::string name) {
        return hasVar(Token(tok_identifier, name, 0, ""));
    }

    bool hasVar(Token name) {
        for (std::vector<Variable> var : vars) {
            for (Variable v : var) {
                if (v.getName() == name.getValue()) {
                    return true;
                }
            }
        }
        return false;
    }

    Variable getVar(std::string name) {
        return getVar(Token(tok_identifier, name, 0, ""));
    }

    Variable getVar(Token name) {
        for (ssize_t i = vars.size() - 1; i >= 0; i--) {
            for (Variable v : vars[i]) {
                if (v.getName() == name.getValue()) {
                    return v;
                }
            }
        }
        return Variable("","");
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

    double parseDouble(std::string str) {
        double num;
        try
        {
            num = std::stold(str);
        }
        catch(const std::exception& e)
        {
            std::cerr << "Number out of range: " << str << std::endl;
            return 0.0;
        }
        return num;
    }

    FPResult parseType(std::vector<Token> body, size_t* i) {
        // int, str, any, none, Struct
        FPResult r;
        r.success = true;
        r.value = "";
        r.line = body[*i].getLine();
        r.in = body[*i].getFile();
        r.column = body[*i].getColumn();
        r.value = body[*i].getValue();
        r.type = body[*i].getType();
        r.message = "Failed to parse type! Token for type: " + body[*i].tostring();
        r.value = "";
        std::string type_mods = "";
        bool isConst = false;
        bool isMut = false;
        bool isReadonly = false;
        while (body[(*i)].getValue() == "const" || body[(*i)].getValue() == "mut" || body[(*i)].getValue() == "readonly") {
            if (!isConst && body[(*i)].getValue() == "const") {
                type_mods += "const ";
                isConst = true;
                if (isMut) {
                    r.success = false;
                    r.line = body[*i].getLine();
                    r.in = body[*i].getFile();
                    r.column = body[*i].getColumn();
                    r.value = body[*i].getValue();
                    r.type = body[*i].getType();
                    r.message = "Cannot specify 'const' on already 'mut' type!";
                }
            } else if (!isMut && body[(*i)].getValue() == "mut") {
                type_mods += "mut ";
                isMut = true;
                if (isConst) {
                    r.success = false;
                    r.line = body[*i].getLine();
                    r.in = body[*i].getFile();
                    r.column = body[*i].getColumn();
                    r.value = body[*i].getValue();
                    r.type = body[*i].getType();
                    r.message = "Cannot specify 'mut' on already 'const' type!";
                }
            } else if (!isReadonly && body[(*i)].getValue() == "readonly") {
                type_mods += "readonly ";
                isReadonly = true;
            }
            (*i)++;
        }
        if (body[*i].getType() == tok_identifier) {
            r.value = type_mods + body[*i].getValue();
            if (r.value == "lambda") {
                (*i)++;
                if (body[*i].getType() != tok_paren_open) {
                    (*i)--;
                } else {
                    (*i)++;
                    r.value += "(";
                    int count = 0;
                    while (body[*i].getType() != tok_paren_close) {
                        FPResult tmp = parseType(body, i);
                        if (!tmp.success) return tmp;
                        (*i)++;
                        if (body[*i].getType() == tok_comma) {
                            (*i)++;
                        }
                        count++;
                    }
                    (*i)++;
                    r.value += std::to_string(count) + ")";
                    if (body[*i].getType() == tok_column) {
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
        } else if (body[*i].getType() == tok_question_mark || body[*i].getValue() == "?") {
            r.value = type_mods + "?";
        } else if (body[*i].getType() == tok_bracket_open) {
            std::string type = "[";
            (*i)++;
            r = parseType(body, i);
            if (!r.success) {
                return r;
            }
            type += r.value;
            (*i)++;
            if (body[*i].getType() == tok_bracket_close) {
                type += "]";
                r.value = type_mods + type;
            } else {
                r.message = "Expected ']', but got '" + body[*i].getValue() + "'";
                r.value = type_mods + body[*i].getValue();
                r.line = body[*i].getLine();
                r.in = body[*i].getFile();
                r.type = body[*i].getType();
                r.success = false;
            }
        } else {
            r.success = false;
            r.line = body[*i].getLine();
            r.in = body[*i].getFile();
            r.column = body[*i].getColumn();
            r.value = body[*i].getValue();
            r.type = body[*i].getType();
            r.message = "Unexpected token: '" + body[*i].tostring() + "'";
        }
        if (body[(*i + 1)].getValue() == "?") {
            (*i)++;
            r.value += "?";
        }
        return r;
    }

    Function* getFunctionByName(TPResult result, std::string name) {
        if (name == "+") name = "operator$add";
        if (name == "-") name = "operator$sub";
        if (name == "*") name = "operator$mul";
        if (name == "/") name = "operator$div";
        if (name == "%") name = "operator$mod";
        if (name == "&") name = "operator$logic_and";
        if (name == "|") name = "operator$logic_or";
        if (name == "^") name = "operator$logic_xor";
        if (name == "~") name = "operator$logic_not";
        if (name == "<<") name = "operator$logic_lsh";
        if (name == ">>") name = "operator$logic_rsh";
        if (name == "**") name = "operator$pow";
        if (name == ".") name = "operator$dot";
        if (name == "<") name = "operator$less";
        if (name == "<=") name = "operator$less_equal";
        if (name == ">") name = "operator$more";
        if (name == ">=") name = "operator$more_equal";
        if (name == "==") name = "operator$equal";
        if (name == "!") name = "operator$not";
        if (name == "!!") name = "operator$assert_not_nil";
        if (name == "!=") name = "operator$not_equal";
        if (name == "&&") name = "operator$bool_and";
        if (name == "||") name = "operator$bool_or";
        if (name == "++") name = "operator$inc";
        if (name == "--") name = "operator$dec";
        if (name == "@") name = "operator$at";
        if (name == "?") name = "operator$wildcard";

        for (Function* func : result.functions) {
            if (func == nullptr) continue;
            if (func->isMethod) continue;
            if (func->getName() == name) {
                return func;
            }
        }
        for (Function* func : result.extern_functions) {
            if (func == nullptr) continue;
            if (func->isMethod) continue;
            if (func->getName() == name) {
                return func;
            }
        }
        return nullptr;
    }

    bool hasEnum(TPResult result, std::string name) {
        return getEnumByName(result, name).getName().size() != 0;
    }

    Enum getEnumByName(TPResult result, std::string name) {
        for (Enum e : result.enums) {
            if (e.getName() == name) {
                return e;
            }
        }
        return Enum("");
    }
    
    Interface* getInterfaceByName(TPResult result, std::string name) {
        for (Interface* i : result.interfaces) {
            if (i->getName() == name) {
                return i;
            }
        }
        return nullptr;
    }
    
    Method* getMethodByName(TPResult result, std::string name, std::string type) {
        type = sclConvertToStructType(type);
        if (name == "+") name = "operator$add";
        if (name == "-") name = "operator$sub";
        if (name == "*") name = "operator$mul";
        if (name == "/") name = "operator$div";
        if (name == "%") name = "operator$mod";
        if (name == "&") name = "operator$logic_and";
        if (name == "|") name = "operator$logic_or";
        if (name == "^") name = "operator$logic_xor";
        if (name == "~") name = "operator$logic_not";
        if (name == "<<") name = "operator$logic_lsh";
        if (name == ">>") name = "operator$logic_rsh";
        if (name == "**") name = "operator$pow";
        if (name == ".") name = "operator$dot";
        if (name == "<") name = "operator$less";
        if (name == "<=") name = "operator$less_equal";
        if (name == ">") name = "operator$more";
        if (name == ">=") name = "operator$more_equal";
        if (name == "==") name = "operator$equal";
        if (name == "!") name = "operator$not";
        if (name == "!!") name = "operator$assert_not_nil";
        if (name == "!=") name = "operator$not_equal";
        if (name == "&&") name = "operator$bool_and";
        if (name == "||") name = "operator$bool_or";
        if (name == "++") name = "operator$inc";
        if (name == "--") name = "operator$dec";
        if (name == "@") name = "operator$at";
        if (name == "?") name = "operator$wildcard";

        if (type == "") {
            return nullptr;
        }

        for (Function* func : result.functions) {
            if (!func->isMethod) continue;
            if (func->getName() == name && ((Method*) func)->getMemberType() == type) {
                return (Method*) func;
            }
        }
        for (Function* func : result.extern_functions) {
            if (!func->isMethod) continue;
            if (func->getName() == name && ((Method*) func)->getMemberType() == type) {
                return (Method*) func;
            }
        }
        Struct s = getStructByName(result, type);
        return getMethodByName(result, name, s.extends());
    }

    Method* getMethodByNameOnThisType(TPResult result, std::string name, std::string type) {
        Method* method = getMethodByName(result, name, type);
        return (method == nullptr || method->getMemberType() != sclConvertToStructType(type)) ? nullptr : method;
    }

    std::vector<Method*> methodsOnType(TPResult res, std::string type) {
        type = sclConvertToStructType(type);

        std::vector<Method*> methods;

        for (Function* func : res.functions) {
            if (!func->isMethod) continue;
            if (((Method*) func)->getMemberType() == type) {
                methods.push_back((Method*) func);
            }
        }
        for (Function* func : res.extern_functions) {
            if (!func->isMethod) continue;
            if (((Method*) func)->getMemberType() == type) {
                methods.push_back((Method*) func);
            }
        }
        return methods;
    }

    Container getContainerByName(TPResult result, std::string name) {
        for (Container container : result.containers) {
            if (container.getName() == name) {
                return container;
            }
        }
        return Container("");
    }

    std::string removeTypeModifiers(std::string t);

#define debugDump(_var) std::cout << #_var << ": " << _var << std::endl

    Struct getStructByName(TPResult result, std::string name) {
        name = removeTypeModifiers(name);
        name = sclConvertToStructType(name);
        for (Struct struct_ : result.structs) {
            if (struct_.getName() == name) {
                return struct_;
            }
        }
        return Struct::Null;
    }

    Layout getLayout(TPResult result, std::string name) {
        name = removeTypeModifiers(name);
        name = sclConvertToStructType(name);
        for (Layout layout : result.layouts) {
            if (layout.getName() == name) {
                return layout;
            }
        }
        return Layout("");
    }

    bool hasLayout(TPResult result, std::string name) {
        return getLayout(result, name).getName().size() != 0;
    }

    bool hasFunction(TPResult result, std::string name) {
        if (name == "+") name = "operator$add";
        if (name == "-") name = "operator$sub";
        if (name == "*") name = "operator$mul";
        if (name == "/") name = "operator$div";
        if (name == "%") name = "operator$mod";
        if (name == "&") name = "operator$logic_and";
        if (name == "|") name = "operator$logic_or";
        if (name == "^") name = "operator$logic_xor";
        if (name == "~") name = "operator$logic_not";
        if (name == "<<") name = "operator$logic_lsh";
        if (name == ">>") name = "operator$logic_rsh";
        if (name == "**") name = "operator$pow";
        if (name == ".") name = "operator$dot";
        if (name == "<") name = "operator$less";
        if (name == "<=") name = "operator$less_equal";
        if (name == ">") name = "operator$more";
        if (name == ">=") name = "operator$more_equal";
        if (name == "==") name = "operator$equal";
        if (name == "!") name = "operator$not";
        if (name == "!!") name = "operator$assert_not_nil";
        if (name == "!=") name = "operator$not_equal";
        if (name == "&&") name = "operator$bool_and";
        if (name == "||") name = "operator$bool_or";
        if (name == "++") name = "operator$inc";
        if (name == "--") name = "operator$dec";
        if (name == "@") name = "operator$at";
        if (name == "?") name = "operator$wildcard";

        for (Function* func : result.functions) {
            if (func->isMethod) continue;
            std::string funcName = func->getName();
            if (funcName.find("$$ol") != std::string::npos) {
                funcName = funcName.substr(0, funcName.find("$$ol"));
            }
            if (funcName == name) {
                return true;
            }
        }
        for (Function* func : result.extern_functions) {
            if (func->isMethod) continue;
            std::string funcName = func->getName();
            if (funcName.find("$$ol") != std::string::npos) {
                funcName = funcName.substr(0, funcName.find("$$ol"));
            }
            if (funcName == name) {
                return true;
            }
        }
        return false;
    }

    bool hasFunction(TPResult result, Token name) {
        return hasFunction(result, name.getValue());
    }


    std::vector<std::string> supersToVector(TPResult r, Struct s) {
        std::vector<std::string> v;
        v.push_back(s.getName());
        Struct super = getStructByName(r, s.extends());
        while (super.getName().size()) {
            v.push_back(super.getName());
            super = getStructByName(r, super.extends());
        }
        return v;
    }
    std::string supersToHashedCList(TPResult r, Struct s) {
        std::string list = "(scl_int[]) {";
        std::vector<std::string> v = supersToVector(r, s);
        for (size_t i = 0; i < v.size(); i++) {
            if (i) {
                list += ", ";
            }
            std::string super = v[i];
            list += std::to_string(hash1((char*) super.c_str()));
        }
        list += "}";
        return list;
    }
    std::string supersToCList(TPResult r, Struct s) {
        std::string list = "(scl_str*) {\"" + s.getName() + "\"";
        Struct super = getStructByName(r, s.extends());
        while (super.getName().size()) {
            list += "\"" + super.getName() + "\"";
            super = getStructByName(r, super.extends());
            if (super.getName().size()) {
                list += ", ";
            }
        }
        list += "}";
        return list;
    }

    bool hasMethod(TPResult result, std::string name, std::string type) {
        return getMethodByName(result, name, type) != nullptr;
    }

    bool hasMethod(TPResult result, Token name, std::string type) {
        return hasMethod(result, name.getValue(), type);
    }

    bool hasContainer(TPResult result, Token name) {
        for (Container container_ : result.containers) {
            if (container_.getName() == name.getValue()) {
                return true;
            }
        }
        return false;
    }

    bool hasContainer(TPResult result, std::string name) {
        return hasContainer(result, Token(tok_identifier, name, 0, ""));
    }

    bool hasGlobal(TPResult result, std::string name) {
        for (Variable v : result.globals) {
            if (v.getName() == name) {
                return true;
            }
        }
        for (Variable v : result.extern_globals) {
            if (v.getName() == name) {
                return true;
            }
        }
        return false;
    }

    bool isInitFunction(Function* f) {
        if (strstarts(f->getName(), "__init__")) {
            return true;
        }
        if (f->isMethod) {
            Method* m = static_cast<Method*>(f);
            return contains<std::string>(m->getModifiers(), "<constructor>") || strstarts(m->getName(), "init");
        } else {
            return contains<std::string>(f->getModifiers(), "construct");
        }
    }

    bool isDestroyFunction(Function* f) {
        return strstarts(f->getName(), "__destroy__") || contains<std::string>(f->getModifiers(), "final");
    }

    bool Variable::isWritableFrom(Function* f, VarAccess accessType)  {
        auto memberOfStruct = [this](Function* f) -> bool {
            if (f->isMethod) {
                Method* m = static_cast<Method*>(f);
                if (m->getMemberType() == this->internalMutableFrom) {
                    return true;
                }
            } else if (strstarts(this->getName(), f->member_type + "$")) {
                return true;
            }
            return false;
        };

        if (typeIsReadonly(getType())) {
            if (strstarts(this->getName(), f->member_type + "$")) {
                return true;
            }
            return memberOfStruct(f);
        }
        if (typeIsConst(getType())) {
            return isInitFunction(f);
        }
        if (typeIsMut(getType())) {
            return true;
        }

        return accessType != VarAccess::Dereference;
    }

    std::string sclConvertToStructType(std::string type) {
        while (type.size() > 1 && type.back() == '?')
            type = type.substr(0, type.size() - 1);

        type = removeTypeModifiers(type);

        return type;
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
        while (strstarts(s, "mut ") || strstarts(s, "const ") || strstarts(s, "ref ")) {
            while (strstarts(s, "mut ")) {
                s = s.substr(4);
            }
            while (strstarts(s, "ref ")) {
                s = s.substr(4);
            }
            while (strstarts(s, "const ")) {
                s = s.substr(6);
            }
        }
        return strstarts(s, "readonly ");
    }

    bool typeIsConst(std::string s) {
        while (strstarts(s, "mut ") || strstarts(s, "readonly ") || strstarts(s, "ref ")) {
            while (strstarts(s, "mut ")) {
                s = s.substr(4);
            }
            while (strstarts(s, "ref ")) {
                s = s.substr(4);
            }
            while (strstarts(s, "readonly ")) {
                s = s.substr(9);
            }
        }
        return strstarts(s, "const ");
    }

    bool typeIsMut(std::string s) {
        while (strstarts(s, "const ") || strstarts(s, "readonly ") || strstarts(s, "ref ")) {
            while (strstarts(s, "const ")) {
                s = s.substr(6);
            }
            while (strstarts(s, "readonly ")) {
                s = s.substr(9);
            }
            while (strstarts(s, "ref ")) {
                s = s.substr(4);
            }
        }
        return strstarts(s, "mut ");
    }

    bool typeIsRef(std::string s) {
        while (strstarts(s, "const ") || strstarts(s, "readonly ") || strstarts(s, "mut ")) {
            while (strstarts(s, "const ")) {
                s = s.substr(6);
            }
            while (strstarts(s, "readonly ")) {
                s = s.substr(9);
            }
            while (strstarts(s, "mut ")) {
                s = s.substr(4);
            }
        }
        return strstarts(s, "ref ");
    }

    bool isPrimitiveIntegerType(std::string type);
    
    bool isPrimitiveType(std::string s) {
        s = removeTypeModifiers(s);
        return isPrimitiveIntegerType(s) || s == "float" || s == "any" || s == "bool";
    }

    bool featureEnabled(std::string feat) {
        return contains<std::string>(Main.options.features, feat);
    }
} // namespace sclc
