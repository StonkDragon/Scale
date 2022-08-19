#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <cstring>

#define TYPES(x, y, line, file, column) if (value == x) return Token(tok_##y, value, line, file, column)
namespace sclc
{
    struct Color {
        static const std::string RESET;
        static const std::string BLACK;
        static const std::string RED;
        static const std::string GREEN;
        static const std::string YELLOW;
        static const std::string BLUE;
        static const std::string MAGENTA;
        static const std::string CYAN;
        static const std::string WHITE;
        static const std::string BOLDBLACK;
        static const std::string BOLDRED;
        static const std::string BOLDGREEN;
        static const std::string BOLDYELLOW;
        static const std::string BOLDBLUE;
        static const std::string BOLDMAGENTA;
        static const std::string BOLDCYAN;
        static const std::string BOLDWHITE;
    };
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

    enum TokenType {
        tok_eof,

        // commands
        tok_return,         // return
        tok_function,       // function
        tok_end,            // end
        tok_true,           // true
        tok_false,          // false
        tok_nil,            // nil
        tok_if,             // if
        tok_else,           // else
        tok_fi,             // fi
        tok_while,          // while
        tok_do,             // do
        tok_done,           // done
        tok_extern,         // extern
        tok_sizeof,         // sizeof
        tok_macro,          // macro
        tok_break,          // break
        tok_continue,       // continue
        tok_for,            // for
        tok_in,             // in
        tok_to,             // to
        tok_proto,          // proto
        tok_ref,            // ref
        tok_deref,          // deref
        tok_addr_ref,       // addr
        tok_load,           // load
        tok_store,          // store
        tok_declare,        // decl

        // operators
        tok_hash,           // #
        tok_open_paren,     // (
        tok_close_paren,    // )
        tok_comma,          // ,
        tok_add,            // +
        tok_sub,            // -
        tok_mul,            // *
        tok_div,            // /
        tok_mod,            // %
        tok_land,           // &
        tok_lor,            // |
        tok_lxor,           // ^
        tok_lnot,           // ~
        tok_lsh,            // <<
        tok_rsh,            // >>
        tok_pow,            // **
        tok_dadd,           // .+
        tok_dsub,           // .-
        tok_dmul,           // .*
        tok_ddiv,           // ./
        tok_sapopen,        // [
        tok_sapclose,       // ]

        tok_identifier,     // foo
        tok_number,         // 123
        tok_number_float,   // 123.456
        tok_string_literal, // "foo"
        tok_char_literal,   // 'a'
        tok_illegal,
        tok_ignore,
        tok_newline,
    };

    struct Token
    {
        TokenType type;
        int line;
        int column;
        std::string file;
        std::string value;
        std::string toString() {
            return "Token(value=" + value + ", type=" + std::to_string(type) + ")";
        }
        Token(TokenType type, std::string value, int line, std::string file, int column) : type(type), value(value) {
            this->line = line;
            this->file = file;
            this->column = column;
        }
        std::string getValue() {
            return value;
        }
        TokenType getType() {
            return type;
        }
        int getLine() {
            return line;
        }
        std::string getFile() {
            return file;
        }
        int getColumn() {
            return column;
        }
    };

    struct ParseResult {
        bool success;
        std::string message;
        std::string in;
        std::vector<ParseResult> errors;
        std::vector<ParseResult> warns;
        std::string token;
        int column;
        int where;
    };

    struct Operation {
        std::string funcName;
        std::vector<std::string> args;
        std::string type;
        std::string varName;
        std::string op;
        std::string left, right;

        std::string getOp() { return op; }
        std::string getLeft() { return left; }
        std::string getType() { return type; }
        std::string getRight() { return right; }
        std::string getVarName() { return varName; }
        std::string getFuncName() { return funcName; }
        std::vector<std::string> getArgs() { return args; }
        void addArg(std::string arg) { args.push_back(arg); }

        Operation(std::string funcName, std::string varName, std::string op, std::string left, std::string right, std::string type) {
            this->funcName = funcName;
            this->varName = varName;
            this->op = op;
            this->left = left;
            this->right = right;
            this->type = type;
        }
    };

    struct ParsedFunction
    {
        std::string name;
        std::vector<Operation> operations;
    };

    enum Modifier
    {
        mod_nps,
        mod_nowarn,
        mod_sap
    };

    struct Function
    {
        std::string name;
        std::vector<Token> body;
        std::vector<Modifier> modifiers;
        std::vector<std::string> args;
        Function(std::string name) {
            this->name = name;
        }
        ~Function() {}
        std::string getName() {
            return name;
        }
        std::vector<Token> getBody() {
            return body;
        }
        void addToken(Token token) {
            body.push_back(token);
        }
        void addModifier(Modifier modifier) {
            modifiers.push_back(modifier);
        }
        std::vector<Modifier> getModifiers() {
            return modifiers;
        }
        void addArgument(std::string arg) {
            args.push_back(arg);
        }
        std::vector<std::string> getArgs() {
            return args;
        }

        bool operator==(const Function& other) const {
            return name == other.name;
        }
    };

    struct Extern
    {
        std::string name;
        Extern(std::string name) {
            this->name = name;
        }
        ~Extern() {}
    };

    struct Prototype
    {
        std::string name;
        Prototype(std::string name) {
            this->name = name;
        }
        ~Prototype() {}
    };


    struct AnalyzeResult {
        std::vector<Function> functions;
        std::vector<Extern> externs;
        std::vector<Prototype> prototypes;
        std::vector<std::string> globals;
    };

    class Lexer
    {
    private:
        std::vector<Token> tokens;
        std::vector<Function> functions;
        std::vector<Extern> externs;
        std::vector<Prototype> prototypes;
    public:
        Lexer(std::vector<Token> tokens) {
            this->tokens = tokens;
        }
        ~Lexer() {}
        AnalyzeResult lexAnalyze();
        static bool isOperator(Token token);
        static bool isType(Token token);
        static bool canAssign(Token token);
    };

    struct Parser
    {
        AnalyzeResult result;

        Parser(AnalyzeResult result)
        {
            this->result = result;
        }
        ~Parser() {}
        bool hasFunction(std::string name) {
            for (Function func : result.functions) {
                if (func.name == name) {
                    return true;
                }
            }
            for (Prototype proto : result.prototypes) {
                if (proto.name == name) {
                    return true;
                }
            }
            return false;
        }
        bool hasExtern(std::string name) {
            for (Extern extern_ : result.externs) {
                if (extern_.name == name) {
                    return true;
                }
            }
            return false;
        }
        ParseResult parse(std::string filename);
        Function getFunctionByName(std::string name);
    };

    class Tokenizer
    {
        std::vector<Token> tokens;
        char* source;
        size_t current;
    public:
        Tokenizer() {current = 0;}
        ~Tokenizer() {}
        void tokenize(std::string source);
        std::vector<Token> getTokens();
        Token nextToken();
        void printTokens();
    };

    typedef struct Main
    {
        Tokenizer* tokenizer;
        Lexer* lexer;
        Parser* parser;
        bool debug;
        std::vector<std::string> frameworkNativeHeaders;
        std::vector<std::string> frameworks;
    } Main;

    Main MAIN = {0, 0, 0, 0, std::vector<std::string>(), std::vector<std::string>()};

    void signalHandler(int signum)
    {
        std::cout << "Signal " << signum << " received." << std::endl;
        if (errno != 0) std::cout << "Error: " << std::strerror(errno) << std::endl;
        exit(signum);
    }

    bool strends(const std::string& str, const std::string& suffix)
    {
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

    inline bool fileExists (const std::string& name) {
        FILE *file;
        if ((file = fopen(name.c_str(), "r")) != NULL) {
            fclose(file);
            return true;
        } else {
            return false;
        }
    }

    template <typename T>
    void addIfAbsent(std::vector<T>& vec, T str) {
        for (size_t i = 0; i < vec.size(); i++) {
            if (vec[i] == str) {
                return;
            }
        }
        vec.push_back(str);
    }
}
#endif // COMMON_H
