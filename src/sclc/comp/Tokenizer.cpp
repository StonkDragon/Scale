#ifndef _TOKENIZER_CPP_
#define _TOKENIZER_CPP_

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <regex>

#include "../Common.hpp"

#include "Lexer.cpp"

namespace sclc
{
    std::vector<Token> Tokenizer::getTokens() {
        return this->tokens;
    }

    static int line = 1;
    static int column = 0;
    static int startColumn = 0;
    static std::string filename;
    Token Tokenizer::nextToken() {
        if (current >= strlen(source)) {
            return Token(tok_eof, "", line, filename, startColumn);
        }
        char c = source[current];
        std::string value = "";

        if (c == '\n') {
            line++;
            column = 0;
            startColumn = 0;
        }

        if (c == '#') {
            char* comment = (char*) malloc(strlen(source + current));
            int i = 0;
            startColumn = column;
            while (c != '\n' && c != '\r') {
                c = source[++current];
                column++;
                comment[i++] = c;
            }
            comment[i-1] = '\0';
            comment += 5;
            column += 5;
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
            startColumn = column;
            while (!isSpace(c) && (isCharacter(c) || isDigit(c))) {
                value += c;
                current++;
                c = source[current];
                column++;
            }
        } else if ((c == '-' && (isDigit(source[current + 1]) || isHexDigit(source[current + 1]) || isOctDigit(source[current + 1]) || isBinDigit(source[current + 1]) || (source[current + 1] == '.' && isDigit(source[current + 2])))) || isDigit(c) || isHexDigit(c) || isOctDigit(c) || isBinDigit(c) || (c == '.' && isDigit(source[current + 1]))) {
            bool isFloat = false;
            if (c == '.') {
                isFloat = true;
            }
            value += c;
            startColumn = column;
            column++;
            c = source[++current];
            while (!isSpace(c) && (isDigit(c) || isHexDigit(c) || isOctDigit(c) || isBinDigit(c)) || c == '.') {
                value += c;
                if (c == '.') {
                    isFloat = true;
                }
                column++;
                c = source[++current];
            }
            if (isFloat) {
                return Token(tok_number_float, value, line, filename, startColumn);
            } else {
                return Token(tok_number, value, line, filename, startColumn);
            }
        } else if (c == '"') {
            startColumn = column;
            column++;
            c = source[++current];
            while (c != '"') {
                if (c == '\n' || c == '\r' || c == '\0') {
                    std::cout << Color::BOLDRED << "Error: " << Color::RESET << filename << ":" << line << ":" << startColumn << ": " << "Unterminated string" << std::endl;
                    exit(1);
                }
                value += c;
                column++;
                c = source[++current];
            }
            current++;
            column++;
            return Token(tok_string_literal, value, line, filename, startColumn);
        } else if (c == '\'') {
            c = source[++current];
            startColumn = column;
            column++;
            if (c == '\\') {
                c = source[++current];
                column++;
                if (c == 'n') {
                    char* iStr = (char*) malloc(4);
                    sprintf(iStr, "%d", '\n');
                    current += 2;
                    return Token(tok_number, iStr, line, filename, startColumn);
                } else if (c == 't') {
                    char* iStr = (char*) malloc(4);
                    sprintf(iStr, "%d", '\t');
                    current += 2;
                    return Token(tok_number, iStr, line, filename, startColumn);
                } else if (c == 'r') {
                    char* iStr = (char*) malloc(4);
                    sprintf(iStr, "%d", '\r');
                    current += 2;
                    return Token(tok_number, iStr, line, filename, startColumn);
                } else if (c == '\\') {
                    char* iStr = (char*) malloc(4);
                    sprintf(iStr, "%d", '\\');
                    current += 2;
                    return Token(tok_number, iStr, line, filename, startColumn);
                } else if (c == '\'') {
                    char* iStr = (char*) malloc(4);
                    sprintf(iStr, "%d", '\'');
                    current += 2;
                    return Token(tok_number, iStr, line, filename, startColumn);
                } else if (c == '\"') {
                    char* iStr = (char*) malloc(4);
                    sprintf(iStr, "%d", '\"');
                    current += 2;
                    return Token(tok_number, iStr, line, filename, startColumn);
                } else if (c == '0') {
                    char* iStr = (char*) malloc(4);
                    sprintf(iStr, "%d", '\0');
                    current += 2;
                    return Token(tok_number, iStr, line, filename, startColumn);
                } else {
                    std::cerr << "Unknown escape sequence: '\\" << c << "'" << std::endl;
                    exit(1);
                }
            } else {
                if (source[current + 1] == '\'') {
                    char* iStr = (char*) malloc(4);
                    sprintf(iStr, "%d", c);
                    current += 2;
                    return Token(tok_number, iStr, line, filename, startColumn);
                } else {
                    std::cerr << "Error: Invalid character literal: '" << c << "'" << std::endl;
                    exit(1);
                }
            }
        } else if (isOperator(c)) {
            value += c;
            startColumn = column;
            column++;
            if (c == '>') {
                c = source[++current];
                column++;
                if (c == '>') {
                    value += c;
                } else {
                    std::cerr << "Error: Expected '>' after '>'\n";
                    exit(1);
                }
            } else if (c == '<') {
                c = source[++current];
                column++;
                if (c == '<') {
                    value += c;
                } else {
                    std::cerr << "Error: Expected '<' after '<'\n";
                    exit(1);
                }
            } else if (c == '*') {
                if (source[current + 1] == '*') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            }
            c = source[++current];
        } else if (c == '.') {
            value += c;
            c = source[++current];
            startColumn = column;
            column++;
            switch (c)
            {
            case '+':
            case '-':
            case '*':
            case '/':
                value += c;
                break;
            
            default:
                std::cerr << "Error: Expected '+', '-', '*', or '/' after '.' for double operation, but got: '" << c << "'" << std::endl;
                exit(1);
            }
            c = source[++current];
            column++;
        }

        // Not a known token, so probably a space character
        if (value == "") {
            current++;
            column++;
            return nextToken();
        }

        TYPES("function", function, line, filename, startColumn);
        TYPES("end", end, line, filename, startColumn);
        TYPES("extern", extern, line, filename, startColumn);
        TYPES("while", while, line, filename, startColumn);
        TYPES("else", else, line, filename, startColumn);
        TYPES("do", do, line, filename, startColumn);
        TYPES("done", done, line, filename, startColumn);
        TYPES("if", if, line, filename, startColumn);
        TYPES("fi", fi, line, filename, startColumn);
        TYPES("return", return, line, filename, startColumn);
        TYPES("break", break, line, filename, startColumn);
        TYPES("continue", continue, line, filename, startColumn);
        TYPES("for", for, line, filename, startColumn);
        TYPES("in", in, line, filename, startColumn);
        TYPES("to", to, line, filename, startColumn);
        TYPES("proto", proto, line, filename, startColumn);
        TYPES("load", load, line, filename, startColumn);
        TYPES("store", store, line, filename, startColumn);
        TYPES("decl", declare, line, filename, startColumn);
        TYPES("addr", addr_ref, line, filename, startColumn);
        TYPES("nil", nil, line, filename, startColumn);
        TYPES("true", true, line, filename, startColumn);
        TYPES("false", false, line, filename, startColumn);
        TYPES("deref", deref, line, filename, startColumn);
        TYPES("ref", ref, line, filename, startColumn);
        
        TYPES("@", hash, line, filename, startColumn);
        TYPES("(", open_paren, line, filename, startColumn);
        TYPES(")", close_paren, line, filename, startColumn);
        TYPES(",", comma, line, filename, startColumn);
        TYPES("+", add, line, filename, startColumn);
        TYPES("-", sub, line, filename, startColumn);
        TYPES("*", mul, line, filename, startColumn);
        TYPES("/", div, line, filename, startColumn);
        TYPES("%", mod, line, filename, startColumn);
        TYPES("&", land, line, filename, startColumn);
        TYPES("|", lor, line, filename, startColumn);
        TYPES("^", lxor, line, filename, startColumn);
        TYPES("~", lnot, line, filename, startColumn);
        TYPES("<<", lsh, line, filename, startColumn);
        TYPES(">>", rsh, line, filename, startColumn);
        TYPES("**", pow, line, filename, startColumn);
        TYPES(".+", dadd, line, filename, startColumn);
        TYPES(".-", dsub, line, filename, startColumn);
        TYPES(".*", dmul, line, filename, startColumn);
        TYPES("./", ddiv, line, filename, startColumn);

        if (current >= strlen(source)) {
            return Token(tok_eof, "", line, filename, startColumn);
        }
        return Token(tok_identifier, value, line, filename, startColumn);
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
}
#endif // TOKENIZER_HPP
