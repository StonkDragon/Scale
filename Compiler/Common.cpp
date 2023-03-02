#include "headers/Common.hpp"

#if !defined(_WIN32) && !defined(__wasm__)
#include <execinfo.h>
#endif

namespace sclc
{

    std::regex flatcase_regex("^[a-z]+$");
    std::regex UPPERCASE_regex("^[A-Z]+$");
    std::regex camelCase_regex("^[a-z]+([A-Z][a-z]*)*$");
    std::regex PascalCase_regex("^([A-Z][a-z]*)+$");
    std::regex IPascalCase_regex("^I([A-Z][a-z]*)+$");
    std::regex snake_case_regex("^[a-z]+(_[a-z]+)*$");
    std::regex SCREAMING_SNAKE_CASE_regex("^[A-Z]+(_[A-Z]+)*$");

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

    std::vector<std::vector<Variable>> vars;
    size_t varDepth = 0;
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

    Variable getVar(Token name) {
        for (std::vector<Variable> var : vars) {
            for (Variable v : var) {
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

    hash hash1(char* data) {
        if (strlen(data) == 0) return 0;
        hash h = 7;
        for (size_t i = 0; i < strlen(data); i++) {
            h = h * 31 + data[i];
        }
        return h;
    }

    FPResult parseType(std::vector<Token> body, size_t* i) {
        // int, str, any, none, Struct
        FPResult r;
        r.success = false;
        r.value = "";
        r.line = body[*i].getLine();
        r.in = body[*i].getFile();
        r.column = body[*i].getColumn();
        r.value = body[*i].getValue();
        r.type = body[*i].getType();
        r.message = "Failed to parse type! Token for type: " + body[*i].tostring();
        if (body[*i].getType() == tok_identifier) {
            r.value = body[*i].getValue();
            r.success = true;
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
            r.value = "?";
            r.success = true;
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
                r.success = true;
                r.value = type;
            } else {
                r.message = "Expected ']', but got '" + body[*i].getValue() + "'";
                r.value = body[*i].getValue();
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
        return r;
    }

    Function* getFunctionByName(TPResult result, std::string name) {
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

    Struct getStructByName(TPResult result, std::string name) {
        name = sclConvertToStructType(name);
        for (Struct struct_ : result.structs) {
            if (struct_.getName() == name) {
                return struct_;
            }
        }
        return Struct("");
    }

    bool hasFunction(TPResult result, Token name) {
        for (Function* func : result.functions) {
            if (func->isMethod) continue;
            if (func->getName() == name.getValue()) {
                return true;
            }
        }
        for (Function* func : result.extern_functions) {
            if (func->isMethod) continue;
            if (func->getName() == name.getValue()) {
                return true;
            }
        }
        return false;
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
            if (m->getName() == "init") {
                return true;
            }
        }
        return false;
    }

#define debugDump(_var) std::cout << #_var << ": " << _var << std::endl
    bool Variable::isWritableFrom(Function* f, VarAccess accessType)  {

        if (isConst && !isMut) {
            if (isInitFunction(f)) {
                if (f->isMethod) {
                    Method* m = static_cast<Method*>(f);
                    if (m->getMemberType() == internalMutableFrom) {
                        return true;
                    }
                } else {
                    return true;
                }
            }
            return false;
        }
        if (isConst && isMut) {
            if (accessType == VarAccess::Write) {
                return false;
            }
            if (isInternalMut || internalMutableFrom.size()) {
                if (!f->isMethod) {
                    return false;
                }
                Method* m = static_cast<Method*>(f);
                if (m->getMemberType() == internalMutableFrom) {
                    if (isInitFunction(f)) {
                        if (f->isMethod) {
                            Method* m = static_cast<Method*>(f);
                            if (m->getMemberType() == internalMutableFrom) {
                                return true;
                            }
                        } else {
                            return true;
                        }
                    }
                    return false;
                } else {
                    return false;
                }
            }
            if (isInitFunction(f)) {
                if (f->isMethod) {
                    Method* m = static_cast<Method*>(f);
                    if (m->getMemberType() == internalMutableFrom) {
                        return true;
                    }
                } else {
                    return true;
                }
            }
            return true;
        }
        if (!isConst && isMut) {
            if (isInternalMut || internalMutableFrom.size()) {
                if (!f->isMethod) {
                    return false;
                }
                Method* m = static_cast<Method*>(f);
                if (m->getMemberType() == internalMutableFrom) {
                    return true;
                } else {
                    return false;
                }
            }
            return true;
        }
        if (!isConst && !isMut) {
            if (accessType == VarAccess::Dereference) {
                return false;
            }
            if (isInternalMut || internalMutableFrom.size()) {
                if (!f->isMethod) {
                    return false;
                }
                Method* m = static_cast<Method*>(f);
                if (m->getMemberType() == internalMutableFrom) {
                    return true;
                } else {
                    return false;
                }
            }
            return true;
        }
        std::cerr << __func__ << ": Should not reach here" << std::endl;
        exit(-1);
    }

    std::string sclConvertToStructType(std::string type) {
        while (typeCanBeNil(type))
            type = type.substr(0, type.size() - 1);

        return type;
    }

    bool sclIsProhibitedInit(std::string s) {
        if (s.size() > 1 && s.back() == '?') s = s.substr(0, s.size() - 1);

        return false;
    }

    bool typeCanBeNil(std::string s) {
        return isPrimitiveType(s) || (s.size() > 1 && s.back() == '?');
    }

    bool isPrimitiveType(std::string s) {
        return s == "int" || s == "float" || s == "any" || s == "bool";
    }
} // namespace sclc
