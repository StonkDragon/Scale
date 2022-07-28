#ifndef _TOKENIZER_CPP_
#define _TOKENIZER_CPP_

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <regex>

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
    return c == '#' || c == '(' || c == ')' || c == ',' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '&' || c == '|' || c == '^' || c == '~' || c == '<' || c == '>';
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
    } else if (c == '\'') {
        c = source[++current];
        if (c == '\\') {
            c = source[++current];
            if (c == 'n') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\n');
                current += 2;
                return Token(tok_number, iStr);
            } else if (c == 't') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\t');
                current += 2;
                return Token(tok_number, iStr);
            } else if (c == 'r') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\r');
                current += 2;
                return Token(tok_number, iStr);
            } else if (c == '\\') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\\');
                current += 2;
                return Token(tok_number, iStr);
            } else if (c == '\'') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\'');
                current += 2;
                return Token(tok_number, iStr);
            } else if (c == '\"') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\"');
                current += 2;
                return Token(tok_number, iStr);
            } else if (c == '0') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", '\0');
                current += 2;
                return Token(tok_number, iStr);
            } else {
                std::cerr << "Unknown escape sequence: '\\" << c << "'" << std::endl;
                exit(1);
            }
        } else {
            if (source[current + 1] == '\'') {
                char* iStr = (char*) malloc(4);
                sprintf(iStr, "%d", c);
                current += 2;
                return Token(tok_number, iStr);
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
    TYPES("load", load);
    TYPES("store", store);
    TYPES("decl", declare);
    TYPES("addr", addr_ref);
    TYPES("nil", nil);
    TYPES("true", true);
    TYPES("false", false);
    TYPES("deref", deref);
    TYPES("ref", ref);
    
    TYPES("#", hash);
    TYPES("(", open_paren);
    TYPES(")", close_paren);
    TYPES(",", comma);
    TYPES("+", add);
    TYPES("-", sub);
    TYPES("*", mul);
    TYPES("/", div);
    TYPES("%", mod);
    TYPES("&", land);
    TYPES("|", lor);
    TYPES("^", lxor);
    TYPES("~", lnot);
    TYPES("<<", lsh);
    TYPES(">>", rsh);
    TYPES("**", pow);

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

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    
    return true;
}

std::string replaceAll(std::string src, std::string from, std::string to) {
    return std::regex_replace(src, std::regex("[\\n\\r\\s]+" + from + "[\\n\\r\\s\\x00]+"), " " + to + " ");
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

        size_t line = 0;

        std::vector<std::tuple<std::string, std::string>> macros;

        while (fgets(buffer, size + 1, fp) != NULL) {
            line++;
            // skip if comment
            if (buffer[0] == '/') {
                if (buffer[1] == '/') {
                    continue;
                }
            }
            // skip if blank
            if (buffer[0] == '\n' || buffer[0] == '\r' || buffer[0] == '\0') {
                continue;
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

            char* lineStr = new char[1024];
            sprintf(lineStr, "%zu", line);
            std::string lineStrStr = std::string(lineStr);
            std::string lineCopy = std::string(buffer);

            lineCopy = replaceAll(lineCopy, "HERE", "\"" + file + ":" + lineStrStr + "\"");
            lineCopy = replaceAll(lineCopy, "__LINE__", lineStrStr);
            lineCopy = replaceAll(lineCopy, "__FILE__", "\"" + file + "\"");
            lineCopy = replaceAll(lineCopy, "_VERSION", "\"" + std::string(VERSION) + "\"");
            lineCopy = replaceAll(lineCopy, "HPUTS", "\"" + file + ":" + lineStrStr + "\" printf \": \" printf");
            strcpy(buffer, lineCopy.c_str());

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


            if (buffer[strlen(buffer) - 1] == '\n') {
                buffer[strlen(buffer) - 1] = '\0';
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
