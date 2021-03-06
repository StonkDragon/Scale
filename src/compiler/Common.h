#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>

#define TYPE(x) if (value == #x) return Token(tok_##x, value)
#define TYPES(x, y) if (value == x) return Token(tok_##y, value)

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
    tok_using,          // using
    tok_break,          // break
    tok_continue,       // continue
    tok_for,            // for
    tok_in,             // in
    tok_to,             // to

    // operators
    tok_hash,           // #
    tok_addr_ref,       // &
    tok_star,           // *
    tok_dollar,         // $
    tok_column,         // :

    tok_identifier,     // foo
    tok_number,         // 123
    tok_number_float,   // 123.456
    tok_string_literal, // "foo"
    tok_illegal,
    tok_ignore
};

struct Token
{
    TokenType type;
    std::string value;
    std::string toString() {
        return "Token(value=" + value + ", type=" + std::to_string(type) + ")";
    }
    Token(TokenType type, std::string value) : type(type), value(value) {}
    std::string getValue() {
        return value;
    }
    TokenType getType() {
        return type;
    }
};

struct ParseResult {
    bool success;
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
    mod_static,
    mod_inline,
    mod_extern
};

struct Function
{
    std::string name;
    std::vector<Token> body;
    std::vector<Modifier> modifiers;
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
};

struct Extern
{
    std::string name;
    Extern(std::string name) {
        this->name = name;
    }
    ~Extern() {}
};

struct AnalyzeResult {
    std::vector<Function> functions;
    std::vector<Extern> externs;
};

class Lexer
{
private:
    std::vector<Token> tokens;
    std::vector<Function> functions;
    std::vector<Extern> externs;
    std::string fileName;
public:
    Lexer(std::vector<Token> tokens, std::string fileName) {
        this->tokens = tokens;
        this->fileName = fileName;
    }
    ~Lexer() {}
    AnalyzeResult lexAnalyze();
    static bool isOperator(Token token);
    static bool isType(Token token);
    static bool canAssign(Token token);
};

class Parser
{
private:
    AnalyzeResult result;
public:
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
};

class Tokenizer
{
    std::vector<Token> tokens;
    std::vector<std::string> usings;
    char* source;
    size_t current;
public:
    Tokenizer() {current = 0;}
    ~Tokenizer() {}
    void tokenize(std::string source);
    std::vector<Token> getTokens();
    Token nextToken();
    void printTokens();
    void addUsing(std::string name);
};

typedef struct Main
{
    Tokenizer* tokenizer;
    Lexer* lexer;
    Parser* parser;
} Main;

Main MAIN = {0, 0, 0};

#endif // COMMON_H