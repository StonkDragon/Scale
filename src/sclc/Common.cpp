#include "Common.hpp"

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

    std::vector<std::string> vars;
    Main MAIN = {0, 0, 0, 0, std::vector<std::string>(), std::vector<std::string>()};

    void signalHandler(int signum) {
        std::cout << "Signal " << signum << " received." << std::endl;
        if (errno != 0) std::cout << "Error: " << std::strerror(errno) << std::endl;
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
        return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
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
        return c == '@' || c == '(' || c == ')' || c == ',' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '&' || c == '|' || c == '^' || c == '~' || c == '<' || c == '>';
    }

    bool isOperator(Token token) {
        return token.type == tok_add || token.type == tok_sub || token.type == tok_mul || token.type == tok_div || token.type == tok_mod || token.type == tok_land || token.type == tok_lor || token.type == tok_lxor || token.type == tok_lnot || token.type == tok_lsh || token.type == tok_rsh || token.type == tok_pow || token.type == tok_dadd || token.type == tok_dsub || token.type == tok_dmul || token.type == tok_ddiv;
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

    void addIfAbsent(std::vector<Function>& vec, Function str) {
        for (size_t i = 0; i < vec.size(); i++) {
            if (vec[i] == str) {
                return;
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
        for (std::string var : vars) {
            if (var == name.getValue()) {
                return true;
            }
        }
        return false;
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
} // namespace sclc
