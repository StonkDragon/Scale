#ifndef _TOKENIZER_CPP_
#define _TOKENIZER_CPP_

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <regex>

#include "../Common.hpp"

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

static int isOperator(char c) {
    return c == '@' || c == '(' || c == ')' || c == ',' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '&' || c == '|' || c == '^' || c == '~' || c == '<' || c == '>';
}

std::vector<Token> Tokenizer::getTokens() {
    return this->tokens;
}

static int line = 1;
static std::string filename;
Token Tokenizer::nextToken() {
    if (current >= strlen(source)) {
        return Token(tok_eof, "", line, filename);
    }
    char c = source[current];
    std::string value = "";

    if (c == '\n') {
        line++;
    }

    if (c == '#') {
        char* comment = (char*) malloc(strlen(source + current));
        int i = 0;
        while (c != '\n' && c != '\r') {
            c = source[++current];
            comment[i++] = c;
        }
        comment[i-1] = '\0';
        comment += 5;
        char* _line = strtok(comment, ";");
        if (_line != NULL) {
            line = atoi(_line) - 1;
        }
        char* _filename = comment + strlen(_line) + 6;
        _filename[strlen(_filename) - 1] = '\0';
        filename = _filename;
        free(comment - 5);
        return nextToken();
    }

    if (isCharacter(c)) {
        while (!isSpace(c) && (isCharacter(c) || isDigit(c))) {
            value += c;
            current++;
            c = source[current];
        }
    } else if ((c == '-' && (isDigit(source[current + 1]) || isHexDigit(source[current + 1]) || isOctDigit(source[current + 1]) || isBinDigit(source[current + 1]) || (source[current + 1] == '.' && isDigit(source[current + 2])))) || isDigit(c) || isHexDigit(c) || isOctDigit(c) || isBinDigit(c) || (c == '.' && isDigit(source[current + 1]))) {
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
            return Token(tok_number_float, value, line, filename);
        } else {
            return Token(tok_number, value, line, filename);
        }
    } else if (c == '"') {
        c = source[++current];
        while (c != '"' && c != '\n' && c != '\r' && c != '\0') {
            value += c;
            c = source[++current];
        }
        current++;
        return Token(tok_string_literal, value, line, filename);
    } else if (c == '\'') {
        c = source[++current];
        if (c == '\\') {
            c = source[++current];
            if (c == 'n') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\n');
                current += 2;
                return Token(tok_number, iStr, line, filename);
            } else if (c == 't') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\t');
                current += 2;
                return Token(tok_number, iStr, line, filename);
            } else if (c == 'r') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\r');
                current += 2;
                return Token(tok_number, iStr, line, filename);
            } else if (c == '\\') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\\');
                current += 2;
                return Token(tok_number, iStr, line, filename);
            } else if (c == '\'') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\'');
                current += 2;
                return Token(tok_number, iStr, line, filename);
            } else if (c == '\"') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\"');
                current += 2;
                return Token(tok_number, iStr, line, filename);
            } else if (c == '0') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\0');
                current += 2;
                return Token(tok_number, iStr, line, filename);
            } else {
                std::cerr << "Unknown escape sequence: '\\" << c << "'" << std::endl;
                exit(1);
            }
        } else {
            if (source[current + 1] == '\'') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", c);
                current += 2;
                return Token(tok_number, iStr, line, filename);
            } else {
                std::cerr << "Error: Invalid character literal: '" << c << "'" << std::endl;
                exit(1);
            }
        }
    } else if (isOperator(c)) {
        value += c;
        if (c == '>') {
            c = source[++current];
            if (c == '>') {
                value += c;
            } else {
                std::cerr << "Error: Expected '>' after '>'\n";
                exit(1);
            }
        } else if (c == '<') {
            c = source[++current];
            if (c == '<') {
                value += c;
            } else {
                std::cerr << "Error: Expected '<' after '<'\n";
                exit(1);
            }
        } else if (c == '*') {
            if (source[current + 1] == '*') {
                c = source[++current];
                value += c;
            }
        }
        c = source[++current];
    }

    // Not a known token, so probably a space character
    if (value == "") {
        current++;
        return nextToken();
    }

    TYPES("function", function, line, filename);
    TYPES("end", end, line, filename);
    TYPES("extern", extern, line, filename);
    TYPES("while", while, line, filename);
    TYPES("else", else, line, filename);
    TYPES("do", do, line, filename);
    TYPES("done", done, line, filename);
    TYPES("if", if, line, filename);
    TYPES("fi", fi, line, filename);
    TYPES("return", return, line, filename);
    TYPES("break", break, line, filename);
    TYPES("continue", continue, line, filename);
    TYPES("for", for, line, filename);
    TYPES("in", in, line, filename);
    TYPES("to", to, line, filename);
    TYPES("proto", proto, line, filename);
    TYPES("load", load, line, filename);
    TYPES("store", store, line, filename);
    TYPES("decl", declare, line, filename);
    TYPES("addr", addr_ref, line, filename);
    TYPES("nil", nil, line, filename);
    TYPES("true", true, line, filename);
    TYPES("false", false, line, filename);
    TYPES("deref", deref, line, filename);
    TYPES("ref", ref, line, filename);
    
    TYPES("@", hash, line, filename);
    TYPES("(", open_paren, line, filename);
    TYPES(")", close_paren, line, filename);
    TYPES(",", comma, line, filename);
    TYPES("+", add, line, filename);
    TYPES("-", sub, line, filename);
    TYPES("*", mul, line, filename);
    TYPES("/", div, line, filename);
    TYPES("%", mod, line, filename);
    TYPES("&", land, line, filename);
    TYPES("|", lor, line, filename);
    TYPES("^", lxor, line, filename);
    TYPES("~", lnot, line, filename);
    TYPES("<<", lsh, line, filename);
    TYPES(">>", rsh, line, filename);
    TYPES("**", pow, line, filename);

    if (current >= strlen(source)) {
        return Token(tok_eof, "", line, filename);
    }
    return Token(tok_identifier, value, line, filename);
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    
    return true;
}

std::string replaceAll(std::string src, std::string from, std::string to) {
    try {
        return std::regex_replace(src, std::regex(from), to);
    } catch (std::regex_error& e) {
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

void Tokenizer::tokenize(std::string source) {
    FILE *fp;

    std::string data = "";

    std::string file = source;
    
    fp = fopen(file.c_str(), "r");
    if (fp == NULL) {
        printf("Error: Could not open file %s\n", file.c_str());
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = new char[size + 1];

    size_t line = 1;

    while (fgets(buffer, size + 1, fp) != NULL) {
        if (buffer[0] == '#') {
            strtok(buffer, " ");
            char* lineStr = strtok(NULL, " ");
            char* name = lineStr + strlen(lineStr) + 1;
            name[lastIndexOf(name, '"')] = '\0';
            name++;

            line = atoi(lineStr);
            data += "#LINE:" + std::to_string(line) + ";FILE:" + name + ";\n";
            continue;
        }
        // skip if comment
        if (buffer[0] == '/') {
            if (buffer[1] == '/') {
                data += "\n";
                continue;
            }
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

        line++;

        // add to data
        data += buffer;
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
