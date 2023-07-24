#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

namespace sclc
{
    extern std::vector<std::string> keywords;

    struct Token {
        static Token Default;

        TokenType type;
        int line;
        std::string file;
        std::string value;
        int column;
        std::string tostring() const {
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
        std::string formatted() const {
            if (type == tok_eof) return "";
            std::string colorFormat = color();
            if (type == tok_string_literal) {
                return colorFormat + "\"" + value + "\"" + Color::RESET;
            } else if (type == tok_char_string_literal) {
                return colorFormat + "c\"" + value + "\"" + Color::RESET;
            } else if (type == tok_char_literal) {
                auto integerToChar = [](int i) -> std::string {
                    std::string s(1, (char)i);
                    return s;
                };
                return colorFormat + "'" + integerToChar(std::stoi(value)) + "'" + Color::RESET;
            }
            return colorFormat + value + Color::RESET;
        }

        bool isKeyword() const {
            if (type == tok_eof) return false;
            if (type == tok_identifier) {

                auto contains = [](std::vector<std::string> v, std::string val) -> bool {
                    if (v.size() == 0) return false;

                    return std::find(v.begin(), v.end(), val) != v.end();
                };
                return contains(keywords, value);
            }
            return  type == tok_return ||
                    type == tok_function ||
                    type == tok_end ||
                    type == tok_true ||
                    type == tok_false ||
                    type == tok_nil ||
                    type == tok_if ||
                    type == tok_unless ||
                    type == tok_then ||
                    type == tok_elif ||
                    type == tok_elunless ||
                    type == tok_else ||
                    type == tok_fi ||
                    type == tok_while ||
                    type == tok_do ||
                    type == tok_done ||
                    type == tok_extern ||
                    type == tok_extern_c ||
                    type == tok_break ||
                    type == tok_continue ||
                    type == tok_for ||
                    type == tok_foreach ||
                    type == tok_in ||
                    type == tok_step ||
                    type == tok_to ||
                    type == tok_addr_ref ||
                    type == tok_load ||
                    type == tok_store ||
                    type == tok_declare ||
                    type == tok_container_def ||
                    type == tok_repeat ||
                    type == tok_struct_def ||
                    type == tok_new ||
                    type == tok_is ||
                    type == tok_cdecl ||
                    type == tok_label ||
                    type == tok_goto ||
                    type == tok_switch ||
                    type == tok_case ||
                    type == tok_esac ||
                    type == tok_default ||
                    type == tok_interface_def ||
                    type == tok_as ||
                    type == tok_enum ||
                    type == tok_question_mark ||
                    type == tok_addr_of ||
                    type == tok_paren_open ||
                    type == tok_paren_close ||
                    type == tok_curly_open ||
                    type == tok_curly_close ||
                    type == tok_bracket_open ||
                    type == tok_bracket_close ||
                    type == tok_comma ||
                    type == tok_column ||
                    type == tok_double_column ||
                    type == tok_dot;
        }

        std::string color() const {
            if (isKeyword()) {
                return Color::BLUE;
            }
            if (type == tok_number || type == tok_number_float) {
                return Color::GREEN;
            }
            if (type == tok_string_literal || type == tok_char_string_literal) {
                return Color::YELLOW;
            }
            if (type == tok_char_literal) {
                return Color::CYAN;
            }
            return Color::WHITE;
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
