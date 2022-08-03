#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>

#define TYPES(x, y, line, file) if (value == x) return Token(tok_##y, value, line, file)

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

    // operators
    tok_hash,           // #
    tok_addr_ref,       // addr
    tok_load,           // load
    tok_store,          // store
    tok_declare,        // decl
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
    std::string file;
    std::string value;
    std::string toString() {
        return "Token(value=" + value + ", type=" + std::to_string(type) + ")";
    }
    Token(TokenType type, std::string value, int line, std::string file) : type(type), value(value) {
        this->line = line;
        this->file = file;
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
};

struct ParseResult {
    bool success;
    std::string message;
    std::string in;
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
    mod_nowarn
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
    bool equals(Function other) {
        return name == other.name;
    }
    void addArgument(std::string arg) {
        args.push_back(arg);
    }
    std::vector<std::string> getArgs() {
        return args;
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
} Main;

Main MAIN = {0, 0, 0};

#endif // COMMON_H