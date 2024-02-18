#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include "TokenType.hpp"

namespace sclc {
    extern std::vector<std::string> keywords;

    struct CSourceLocation {
        void* file;
        size_t line;
        size_t column;
    };

    struct CToken {
        long type;
        void* value;
        CSourceLocation* location;
    };

    struct SourceLocation {
        static SourceLocation of(CSourceLocation* cloc, char* (*fromString)(void*));

        std::string file;
        int line;
        int column;
        SourceLocation();
        SourceLocation(std::string file, int line, int column);
        SourceLocation(const SourceLocation& other);
        SourceLocation(SourceLocation&& other);
        SourceLocation& operator=(const SourceLocation& other);
        SourceLocation& operator=(SourceLocation&& other);
        ~SourceLocation();
        CSourceLocation* toC(void*(*alloc)(size_t), void*(*toString)(char*)) const;
        std::string toString() const;
    };

    struct Token {
        static Token Default;

        static Token of(CToken* ctok, char* (*fromString)(void*));

        TokenType type;
        std::string value;
        SourceLocation location;

        std::string toString() const;
        Token();
        Token(TokenType type, std::string value);
        Token(TokenType type, std::string value, int line, std::string file, int column);
        Token(TokenType type, std::string value, SourceLocation location);
        Token(const Token& other);
        Token(Token&& other);
        Token& operator=(const Token& other);
        Token& operator=(Token&& other);
        ~Token();
        std::string formatted() const;
        bool isKeyword() const;
        CToken* toC(void*(*alloc)(size_t), void* (*toString)(char*)) const;
        std::string color() const;
        bool operator==(const Token& other) const;
        bool operator!=(const Token& other) const;
        bool operator==(Token& other);
        bool operator!=(Token& other);
    };
} // namespace sclc
