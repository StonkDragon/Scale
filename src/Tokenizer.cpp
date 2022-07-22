#ifndef _TOKENIZER_CPP_
#define _TOKENIZER_CPP_

#include <iostream>
#include <string.h>
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

    // operators
    tok_hash,           // #

    tok_identifier,     // foo
    tok_number,         // 123
    tok_string_literal, // "foo"
    tok_illegal
};

static int isCharacter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int isDigit(char c) {
    return c >= '0' && c <= '9';
}

static int isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int isPrint(char c) {
    return (c >= ' ');
}

static int isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^' || c == '=' || c == '<' || c == '>' || c == '&' || c == '|' || c == '!' || c == '~' || c == '?' || c == ':' || c == ',' || c == ';' || c == '.' || c == '#';
}

static int isBracket(char c) {
    return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
}

static int isHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == 'x' || c == 'X';
}

static int isOctDigit(char c) {
    return c >= '0' && c <= '7' || c == 'o' || c == 'O';
}

static int isBinDigit(char c) {
    return c == '0' || c == '1' || c == 'b' || c == 'B';
}

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

std::vector<Token> Tokenizer::getTokens() {
    return this->tokens;
}

Token Tokenizer::nextToken() {
    if (current >= strlen(source)) {
        return Token(tok_eof, "");
    }
    char c = source[current];
    std::string value = "";
    
    if (isCharacter(c)) {
        while (!isSpace(c) && (isCharacter(c) || isDigit(c))) {
            value += c;
            current++;
            c = source[current];
        }
    } else if (isDigit(c) || isHexDigit(c) || isOctDigit(c) || isBinDigit(c)) {
        while (!isSpace(c) && (isDigit(c) || isHexDigit(c) || isOctDigit(c) || isBinDigit(c))) {
            value += c;
            c = source[++current];
        }
        return Token(tok_number, value);
    } else if (c == '"') {
        c = source[++current];
        while (c != '"' && c != '\n' && c != '\r' && c != '\0') {
            value += c;
            c = source[++current];
        }
        current++;
        return Token(tok_string_literal, value);
    } else if (c == '#') {
        value += c;
        c = source[++current];
    }

    // Not a known token, so probably a space character
    if (value == "") {
        current++;
        return nextToken();
    }

    TYPES("macro", macro);
    TYPES("using", using);
    TYPES("function", function);
    TYPES("end", end);
    TYPES("native", extern);
    TYPES("while", while);
    TYPES("else", else);
    TYPES("do", do);
    TYPES("done", done);
    TYPES("if", if);
    TYPES("fi", fi);
    TYPES("return", return);
    
    TYPES("#", hash);

    if (current >= strlen(source)) {
        return Token(tok_eof, "");
    }
    return Token(tok_identifier, value);
}

void Tokenizer::tokenize(std::string source) {
    FILE *fp = fopen(source.c_str(), "r");

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = new char[size + 1];
    std::string data = "";

    // read the file line by line
    while (fgets(buffer, size + 1, fp) != NULL) {
        // skip if comment
        if (buffer[0] == '/') {
            if (buffer[1] == '/') {
                continue;
            }
        }
        // remove trailing newline
        if (buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
        }
        // remove mid line comments
        char *c = buffer;
        while (*c != '\0') {
            if (*c == '/') {
                if (c[1] == '/') {
                    *c = '\0';
                    break;
                }
            }
            c++;
        }
        // add to data
        data += buffer;
        data += "\n";
    }

    fclose(fp);

    this->source = (char*) data.c_str();

    current = 0;

    Token token = nextToken();
    while (token.type != tok_eof) {
        this->tokens.push_back(token);
        token = nextToken();
    }
}

void Tokenizer::printTokens() {
    for (int i = 0; i < tokens.size(); i++) {
        std::cout << "Token: " << tokens.at(i).toString() << std::endl;
    }
}

#endif // TOKENIZER_HPP
