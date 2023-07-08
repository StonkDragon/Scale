#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

namespace sclc
{
    struct Token {
        static Token Default;

        TokenType type;
        int line;
        std::string file;
        std::string value;
        int column;
        std::string tostring() {
            return "Token(value=" + value + ", type=" + std::to_string(type) + ", line=" + std::to_string(line) + ", column=" + std::to_string(column) + ", file=" + file + ")";
        }
        Token() : Token(tok_eof, "", 0, "") {}
        Token(TokenType type, std::string value, int line, std::string file) : type(type), value(value) {
            this->line = line;
            this->file = file;
            this->column = 0;
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
            return this->column;
        }
        bool operator==(const Token& other) const {
            return this->type == other.type && this->value == other.value;
        }
        bool operator!=(const Token& other) const {
            return !(*this == other);
        }
        bool operator==(Token& other) {
            return this->type == other.type && this->value == other.value;
        }
        bool operator!=(Token& other) {
            return !(*this == other);
        }
    };
} // namespace sclc
