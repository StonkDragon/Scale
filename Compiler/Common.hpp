#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>

#define TYPES(x, y, line, file, column) if (value == x) return Token(tok_##y, value, line, file, column)

#undef INT_MAX
#undef INT_MIN
#undef LONG_MAX
#undef LONG_MIN

#define INT_MAX 0x7FFFFFFF
#define INT_MIN 0x80000000
#define LONG_MAX 0x7FFFFFFFFFFFFFFFll
#define LONG_MIN 0x8000000000000000ll

#define LINE_LENGTH 48

typedef unsigned long long hash;

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

    struct Version {
        int major;
        int minor;
        int patch;

        Version(std::string str) {
            std::string::difference_type n = std::count(str.begin(), str.end(), '.');
            if (n == 1) {
                sscanf(str.c_str(), "%d.%d", &major, &minor);
                patch = 0;
            } else if (n == 2) {
                sscanf(str.c_str(), "%d.%d.%d", &major, &minor, &patch);
            } else if (n == 0) {
                sscanf(str.c_str(), "%d", &major);
                minor = 0;
                patch = 0;
            } else {
                std::cerr << "Unknown version number: " << str << std::endl;
                exit(1);
            }
        }
        ~Version() {}

        inline bool operator==(Version& v) {
            return (major == v.major) && (minor == v.minor) && (patch == v.patch);
        }

        inline bool operator>(Version& v) {
            if (major > v.major) {
                return true;
            } else if (major == v.major) {
                if (minor > v.minor) {
                    return true;
                } else if (minor == v.minor) {
                    if (patch > v.patch) {
                        return true;
                    }
                    return false;
                }
                return false;
            }
            return false;
        }

        inline bool operator>=(Version& v) {
            return ((*this) > v) || ((*this) == v);
        }

        inline bool operator<=(Version& v) {
            return !((*this) > v);
        }

        inline bool operator<(Version& v) {
            return !((*this) >= v);
        }

        inline bool operator!=(Version& v) {
            return !((*this) == v);
        }

        std::string asString() {
            return std::to_string(this->major) + "." + std::to_string(this->minor) + "." + std::to_string(this->patch);
        }
    };
    
    extern std::vector<std::string> vars;

    long long parseNumber(std::string str);
    double parseDouble(std::string str);

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
        tok_container_def,  // container
        tok_repeat,         // repeat

        // operators
        tok_hash,           // #
        tok_open_paren,     // (
        tok_close_paren,    // )
        tok_curly_open,     // {
        tok_curly_close,    // }
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
        tok_container_acc,  // ->

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
        std::string tostring() {
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

    struct FPResult {
        bool success;
        std::string message;
        std::string in;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::string value;
        int column;
        int line;
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

    struct Container {
        std::string name;
        std::vector<std::string> members;
        Container(std::string name) {
            this->name = name;
        }
        void addMember(std::string member) {
            members.push_back(member);
        }
        bool hasMember(std::string member) {
            for (std::string m : members) {
                if (m == member) {
                    return true;
                }
            }
            return false;
        }
    };

    struct Prototype
    {
        std::string name;
        int argCount;
        Prototype(std::string name, int argCount) {
            this->name = name;
            this->argCount = argCount;
        }
        ~Prototype() {}
    };

    struct TPResult {
        std::vector<Function> functions;
        std::vector<Extern> externs;
        std::vector<Prototype> prototypes;
        std::vector<std::string> globals;
        std::vector<Container> containers;
        std::vector<FPResult> errors;
    };

    class TokenParser
    {
    private:
        std::vector<Token> tokens;
        std::vector<Function> functions;
        std::vector<Extern> externs;
        std::vector<Prototype> prototypes;
    public:
        TokenParser(std::vector<Token> tokens) {
            this->tokens = tokens;
        }
        ~TokenParser() {}
        TPResult parse();
        static bool isOperator(Token token);
        static bool isType(Token token);
        static bool canAssign(Token token);
    };

    struct FunctionParser
    {
        TPResult result;

        FunctionParser(TPResult result)
        {
            this->result = result;
        }
        ~FunctionParser() {}
        bool hasFunction(Token name) {
            for (Function func : result.functions) {
                if (func.name == name.getValue()) {
                    return true;
                }
            }
            for (Prototype proto : result.prototypes) {
                if (proto.name == name.getValue()) {
                    bool hasFunction = false;
                    for (Function func : result.functions) {
                        if (func.name == proto.name) {
                            hasFunction = true;
                            break;
                        }
                    }
                    if (!hasFunction) {
                        std::cerr << name.getFile() << ":" << name.getLine() << ":" << name.getColumn() << ": Error: Missing Implementation for function " << name.getValue() << std::endl;
                        exit(1);
                    }
                    return true;
                }
            }
            return false;
        }
        bool hasExtern(Token name) {
            for (Extern extern_ : result.externs) {
                if (extern_.name == name.getValue()) {
                    return true;
                }
            }
            return false;
        }
        bool hasContainer(Token name) {
            for (Container container_ : result.containers) {
                if (container_.name == name.getValue()) {
                    return true;
                }
            }
            return false;
        }
        FPResult parse(std::string filename);
        Function getFunctionByName(std::string name);
        Container getContainerByName(std::string name);
    };

    class Tokenizer
    {
        std::vector<Token> tokens;
        std::vector<FPResult> errors;
        char* source;
        size_t current;
    public:
        Tokenizer() {current = 0;}
        ~Tokenizer() {}
        FPResult tokenize(std::string source);
        std::vector<Token> getTokens();
        Token nextToken();
        void printTokens();
    };

    typedef struct _Main
    {
        Tokenizer* tokenizer;
        TokenParser* lexer;
        FunctionParser* parser;
        bool debug;
        std::vector<std::string> frameworkNativeHeaders;
        std::vector<std::string> frameworks;
    } _Main;

    extern _Main Main;

    void signalHandler(int signum);
    bool strends(const std::string& str, const std::string& suffix);
    int isCharacter(char c);
    int isDigit(char c);
    int isSpace(char c);
    int isPrint(char c);
    int isBracket(char c);
    int isHexDigit(char c);
    int isOctDigit(char c);
    int isBinDigit(char c);
    int isOperator(char c);
    bool isOperator(Token token);
    bool fileExists(const std::string& name);
    void addIfAbsent(std::vector<Function>& vec, Function str);
    std::string replaceAll(std::string src, std::string from, std::string to);
    std::string replaceFirstAfter(std::string src, std::string from, std::string to, int index);
    int lastIndexOf(char* src, char c);
    bool hasVar(Token name);
    hash hash1(char* data);
}
#endif // COMMON_H
