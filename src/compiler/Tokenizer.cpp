#ifndef _TOKENIZER_CPP_
#define _TOKENIZER_CPP_

#include <iostream>
#include <string.h>
#include <vector>

#include "Common.h"

#include "Lexer.cpp"

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

std::vector<Token> Tokenizer::getTokens() {
    return this->tokens;
}

static bool nextIsVar = false;
Token Tokenizer::nextToken() {
    if (current >= strlen(source)) {
        return Token(tok_eof, "");
    }
    char c = source[current];
    std::string value = "";
    
    if (nextIsVar) {
        nextIsVar = false;
        value = "$";
    }

    if (isCharacter(c)) {
        while (!isSpace(c) && (isCharacter(c) || isDigit(c))) {
            value += c;
            current++;
            c = source[current];
        }
    } else if (c == '-' || isDigit(c) || isHexDigit(c) || isOctDigit(c) || isBinDigit(c) || (c == '.' && isDigit(source[current + 1]))) {
        bool isFloat = false;
        if (c == '.') {
            isFloat = true;
        }
        value += c;
        c = source[++current];
        while (!isSpace(c) && (isDigit(c) || isHexDigit(c) || isOctDigit(c) || isBinDigit(c)) || c == '.') {
            value += c;
            if (c == '.') {
                isFloat = true;
            }
            c = source[++current];
        }
        if (isFloat) {
            return Token(tok_number_float, value);
        } else {
            return Token(tok_number, value);
        }
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
    } else if (c == '&') {
        value += c;
        c = source[++current];
    } else if (c == '*') {
        value += c;
        c = source[++current];
        nextIsVar = true;
    } else if (c == '>') {
        value += c;
        c = source[++current];
        nextIsVar = true;
    } else if (c == ':') {
        value += c;
        c = source[++current];
        nextIsVar = true;
    } else if (c == '(') {
        value += c;
        c = source[++current];
    } else if (c == ')') {
        value += c;
        c = source[++current];
    } else if (c == ',') {
        value += c;
        c = source[++current];
    } 

    // Not a known token, so probably a space character
    if (value == "") {
        current++;
        return nextToken();
    }

    TYPES("using", using);
    TYPES("function", function);
    TYPES("end", end);
    TYPES("extern", extern);
    TYPES("while", while);
    TYPES("else", else);
    TYPES("do", do);
    TYPES("done", done);
    TYPES("if", if);
    TYPES("fi", fi);
    TYPES("return", return);
    TYPES("break", break);
    TYPES("continue", continue);
    TYPES("for", for);
    TYPES("in", in);
    TYPES("to", to);
    TYPES("proto", proto);
    
    TYPES("#", hash);
    TYPES("&", addr_ref);
    TYPES("*", star);
    TYPES(">", dollar);
    TYPES(":", column);
    TYPES("(", open_paren);
    TYPES(")", close_paren);
    TYPES(",", comma);

    if (current >= strlen(source)) {
        return Token(tok_eof, "");
    }
    return Token(tok_identifier, value);
}

bool hasUsing(std::string name) {
    for (int i = 0; i < MAIN.usings.size(); i++) {
        if (MAIN.usings[i] == name) {
            return true;
        }
    }
    return false;
}

void Tokenizer::addUsing(std::string name) {
    if (!hasUsing(name)) {
        MAIN.usings.push_back(name);
    }
}

void Tokenizer::tokenize(std::string source) {
    addUsing(source);

    FILE *fp;

    std::string data = "";

    for (int i = 0; i < MAIN.usings.size(); i++) {
        std::string file = MAIN.usings[i];

        if (!fileExists(file)) {
            file = std::string(getenv("HOME")) + "/Scale/lib/" + file;
        }
        fp = fopen(file.c_str(), "r");
        if (fp == NULL) {
            printf("Error: Could not open file %s\n", file.c_str());
            exit(1);
        }

        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char *buffer = new char[size + 1];

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

            if (strncmp(buffer, "using", 5) == 0) {
                std::string file = "";
                char c;
                int i = 5;
                while ((c = buffer[i]) != '"') i++;
                i++;
                while ((c = buffer[i]) != '"') {
                    file += c;
                    i++;
                }
                addUsing(file);
                continue;
            }

            // add to data
            data += buffer;
            data += "\n";
        }

        fclose(fp);
    }

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
