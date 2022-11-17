#ifndef _TOKENIZER_CPP_
#define _TOKENIZER_CPP_

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <regex>

#include "../headers/Common.hpp"

#define syntaxError(msg) \
    do { \
    FPResult result; \
    result.message = msg; \
    result.in = filename; \
    result.line = line; \
    result.column = startColumn; \
    result.value = value; \
    errors.push_back(result); \
    } while (0)

namespace sclc
{
    std::vector<Token> Tokenizer::getTokens() {
        return this->tokens;
    }

    static int line = 1;
    static int column = 0;
    static int startColumn = 0;
    static std::string filename;
    static bool inFunction = false;
    static bool isExtern = false;

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
            size_t i = 0;
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
            while ((!isSpace(c) && (isDigit(c) || isHexDigit(c) || isOctDigit(c) || isBinDigit(c))) || c == '.') {
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
                    syntaxError("Unterminated string");
                }
                
                if (c == '\\') {
                    column++;
                    c = source[++current];
                    switch (c)
                    {
                        case '0':
                        case 't':
                        case 'r':
                        case 'n':
                        case '"':
                        case '\\':
                            value += '\\';
                            value += c;
                            c = source[++current];
                            break;
                        
                        default:
                            syntaxError("Unknown escape sequence: '\\" + std::to_string(c) + "'");
                            break;
                    }
                } else {
                    value += c;
                    column++;
                    c = source[++current];
                }
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
                    snprintf(iStr, 23, "%d", '\n');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, startColumn);
                } else if (c == 't') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\t');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, startColumn);
                } else if (c == 'r') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\r');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, startColumn);
                } else if (c == '\\') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\\');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, startColumn);
                } else if (c == '\'') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\'');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, startColumn);
                } else if (c == '\"') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\"');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, startColumn);
                } else if (c == '0') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\0');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, startColumn);
                } else {
                    syntaxError("Unknown escape sequence: '\\" + std::to_string(c) + "'");
                }
            } else {
                if (source[current + 1] == '\'') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", c);
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, startColumn);
                } else {
                    syntaxError("Invalid character literal: '" + std::to_string(c) + "'");
                }
            }
        } else if (isOperator(c) && value != "-") {
            value += c;
            startColumn = column;
            column++;
            if (c == '>') {
                c = source[++current];
                column++;
                if (c == '>') {
                    value += c;
                } else {
                    syntaxError("Expected '>' after '>'");
                }
            } else if (c == '<') {
                c = source[++current];
                column++;
                if (c == '<') {
                    value += c;
                } else {
                    syntaxError("Expected '<' after '<'");
                }
            } else if (c == '*') {
                if (source[current + 1] == '*') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '-') {
                if (source[current + 1] == '>') {
                    c = source[++current];
                    value += c;
                    column++;
                }
            } else if (c == ':') {
                c = source[++current];
                column++;
                if (c == ':') {
                    value += c;
                } else {
                    syntaxError("Expected ':' after ':'");
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
                c = source[++current];
                column++;
                break;
            }
        } else if (isBracket(c)) {
            value += c;
            c = source[++current];
            startColumn = column;
            column++;
        }

        // Not a known token, so probably a space character
        if (value == "") {
            current++;
            column++;
            return nextToken();
        }

        if (value == "function" && !isExtern) inFunction = true;
        if (value == "end") inFunction = false;
        if (value == "extern") isExtern = true;
        else isExtern = false;

        TOKEN("function",   tok_function, line, filename, startColumn);
        TOKEN("end",        tok_end, line, filename, startColumn);
        TOKEN("extern",     tok_extern, line, filename, startColumn);
        TOKEN("while",      tok_while, line, filename, startColumn);
        TOKEN("else",       tok_else, line, filename, startColumn);
        TOKEN("do",         tok_do, line, filename, startColumn);
        TOKEN("done",       tok_done, line, filename, startColumn);
        TOKEN("if",         tok_if, line, filename, startColumn);
        TOKEN("fi",         tok_fi, line, filename, startColumn);
        TOKEN("return",     tok_return, line, filename, startColumn);
        TOKEN("break",      tok_break, line, filename, startColumn);
        TOKEN("continue",   tok_continue, line, filename, startColumn);
        TOKEN("for",        tok_for, line, filename, startColumn);
        TOKEN("in",         tok_in, line, filename, startColumn);
        TOKEN("to",         tok_to, line, filename, startColumn);
        TOKEN("load",       tok_load, line, filename, startColumn);
        TOKEN("store",      tok_store, line, filename, startColumn);
        TOKEN("decl",       tok_declare, line, filename, startColumn);
        TOKEN("addr",       tok_addr_ref, line, filename, startColumn);
        TOKEN("nil",        tok_nil, line, filename, startColumn);
        TOKEN("true",       tok_true, line, filename, startColumn);
        TOKEN("false",      tok_false, line, filename, startColumn);
        TOKEN("container",  tok_container_def, line, filename, startColumn);
        TOKEN("repeat",     tok_repeat, line, filename, startColumn);
        TOKEN("struct",     tok_struct_def, line, filename, startColumn);
        TOKEN("new",        tok_new, line, filename, startColumn);
        TOKEN("is",         tok_is, line, filename, startColumn);
        TOKEN("cdecl",      tok_cdecl, line, filename, startColumn);
        TOKEN("goto",       tok_goto, line, filename, startColumn);
        TOKEN("label",      tok_label, line, filename, startColumn);
        
        if (inFunction) {
            TOKEN("@",      tok_addr_of, line, filename, startColumn);
        } else {
            TOKEN("@",      tok_hash, line, filename, startColumn);
        }
        TOKEN("(",          tok_open_paren, line, filename, startColumn);
        TOKEN(")",          tok_close_paren, line, filename, startColumn);
        TOKEN("{",          tok_curly_open, line, filename, startColumn);
        TOKEN("}",          tok_curly_close, line, filename, startColumn);
        TOKEN(",",          tok_comma, line, filename, startColumn);
        TOKEN("+",          tok_add, line, filename, startColumn);
        TOKEN("-",          tok_sub, line, filename, startColumn);
        TOKEN("*",          tok_mul, line, filename, startColumn);
        TOKEN("/",          tok_div, line, filename, startColumn);
        TOKEN("%",          tok_mod, line, filename, startColumn);
        TOKEN("&",          tok_land, line, filename, startColumn);
        TOKEN("|",          tok_lor, line, filename, startColumn);
        TOKEN("^",          tok_lxor, line, filename, startColumn);
        TOKEN("~",          tok_lnot, line, filename, startColumn);
        TOKEN("<<",         tok_lsh, line, filename, startColumn);
        TOKEN(">>",         tok_rsh, line, filename, startColumn);
        TOKEN("**",         tok_pow, line, filename, startColumn);
        TOKEN(".+",         tok_dadd, line, filename, startColumn);
        TOKEN(".-",         tok_dsub, line, filename, startColumn);
        TOKEN(".*",         tok_dmul, line, filename, startColumn);
        TOKEN("./",         tok_ddiv, line, filename, startColumn);
        TOKEN(".",          tok_dot, line, filename, startColumn);
        TOKEN("[",          tok_sapopen, line, filename, startColumn);
        TOKEN("]",          tok_sapclose, line, filename, startColumn);
        TOKEN("->",         tok_container_acc, line, filename, startColumn);
        TOKEN("::",         tok_double_column, line, filename, startColumn);

        if (current >= strlen(source)) {
            return Token(tok_eof, "", line, filename, startColumn);
        }
        return Token(tok_identifier, value, line, filename, startColumn);
    }

    FPResult Tokenizer::tokenize(std::string source) {
        FILE *fp;

        std::string data = "";

        std::string file = source;
        
        fp = fopen(file.c_str(), "r");
        if (fp == NULL) {
            printf("IO Error: Could not open file %s\n", file.c_str());
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
                std::string filename = std::string(name);
                if (strends(filename, ".c")) {
                    filename = filename.substr(0, std::string(name).size() - 2);
                }
                data += "#LINE:" + std::to_string(line) + ";FILE:" + filename + ";\n";
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
        while (token.getType() != tok_eof) {
            this->tokens.push_back(token);
            token = nextToken();
        }

        FPResult result;
        result.errors = errors;
        return result;
    }

    void Tokenizer::printTokens() {
        for (size_t i = 0; i < tokens.size(); i++) {
            std::cout << "Token: " << tokens.at(i).tostring() << std::endl;
        }
    }
}
#endif // TOKENIZER_HPP
