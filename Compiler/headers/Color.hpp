#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

namespace sclc
{
    class Color {
    public:
        static std::string RESET;
        static std::string BLACK;
        static std::string RED;
        static std::string GREEN;
        static std::string YELLOW;
        static std::string BLUE;
        static std::string MAGENTA;
        static std::string CYAN;
        static std::string WHITE;
        static std::string GRAY;
        static std::string BOLDBLACK;
        static std::string BOLDRED;
        static std::string BOLDGREEN;
        static std::string BOLDYELLOW;
        static std::string BOLDBLUE;
        static std::string BOLDMAGENTA;
        static std::string BOLDCYAN;
        static std::string BOLDWHITE;
        static std::string BOLDGRAY;
        static std::string UNDERLINE;
        static std::string BOLD;
        static std::string REVERSE;
        static std::string HIDDEN;
        static std::string ITALIC;
        static std::string STRIKETHROUGH;
    };
} // namespace sclc
