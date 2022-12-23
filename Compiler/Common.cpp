#include "headers/Common.hpp"

#ifndef _WIN32
#include <execinfo.h>
#endif

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

    std::vector<std::vector<Variable>> vars;
    size_t varDepth = 0;
    _Main Main = _Main();

    void print_trace(void) {
#ifndef _WIN32
        void* array[32];
        char** strings;
        int size, i;

        size = backtrace(array, 32);
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

    int isCharacter(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    int isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    int isSpace(char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    int isPrint(char c) {
        return (c >= ' ');
    }

    int isBracket(char c) {
        return c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']';
    }

    int isHexDigit(char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == 'x' || c == 'X';
    }

    int isOctDigit(char c) {
        return (c >= '0' && c <= '7') || c == 'o' || c == 'O';
    }

    int isBinDigit(char c) {
        return c == '0' || c == '1' || c == 'b' || c == 'B';
    }

    int isOperator(char c) {
        return c == '?' || c == '@' || c == '(' || c == ')' || c == ',' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '&' || c == '|' || c == '^' || c == '~' || c == '<' || c == '>' || c == ':' || c == '=' || c == '!';
    }

    bool isOperator(Token token) {
        TokenType type = token.getType();
        return type == tok_add || type == tok_sub || type == tok_mul || type == tok_div || type == tok_mod || type == tok_land || type == tok_lor || type == tok_lxor || type == tok_lnot || type == tok_lsh || type == tok_rsh || type == tok_pow || type == tok_dadd || type == tok_dsub || type == tok_dmul || type == tok_ddiv;
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
        std::string pre = src.substr(0, index - 1);
        std::string post = src.substr(index + from.length() - 1);
        return pre + to + post;
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
        if (body[*i].getType() == tok_identifier) {
            r.value = body[*i].getValue();
            r.success = true;
        } else if (body[*i].getType() == tok_question_mark) {
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
                r.column = body[*i].getColumn();
                r.value = body[*i].getValue();
                r.line = body[*i].getLine();
                r.in = body[*i].getFile();
                r.type = body[*i].getType();
                r.success = false;
            }
        }
        return r;
    }
} // namespace sclc
