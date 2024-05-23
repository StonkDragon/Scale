#include <gc/gc_allocator.h>

#include "../headers/Common.hpp"

namespace sclc {
    SourceLocation SourceLocation::of(CSourceLocation* cloc, char* (*fromString)(void*)) {
        return SourceLocation(std::string(fromString(cloc->file)), cloc->line, cloc->column);
    }

    SourceLocation::SourceLocation() : SourceLocation("", 0, 0) {}
    SourceLocation::SourceLocation(std::string file, int line, int column) : file(file), line(line), column(column) {}
    SourceLocation::SourceLocation(const SourceLocation& other) {
        this->file = other.file;
        this->line = other.line;
        this->column = other.column;
    }
    SourceLocation::SourceLocation(SourceLocation&& other) {
        this->file = static_cast<std::string&&>(other.file);
        this->line = other.line;
        this->column = other.column;
    }

    SourceLocation& SourceLocation::operator=(const SourceLocation& other) {
        this->file = other.file;
        this->line = other.line;
        this->column = other.column;
        return *this;
    }
    SourceLocation& SourceLocation::operator=(SourceLocation&& other) {
        this->file = static_cast<std::string&&>(other.file);
        this->line = other.line;
        this->column = other.column;
        return *this;
    }

    SourceLocation::~SourceLocation() {}

    CSourceLocation* SourceLocation::toC(void*(*alloc)(size_t), void*(*toString)(char*)) const {
        CSourceLocation* loc = (CSourceLocation*) alloc(sizeof(CSourceLocation));
        loc->file = toString(strdup(file.c_str()));
        loc->line = line;
        loc->column = column;
        return loc;
    }

    std::string SourceLocation::toString() const {
        return "SourceLocation(file=" + file + ", line=" + std::to_string(line) + ", column=" + std::to_string(column) + ")";
    }

    Token Token::Default(tok_identifier, "");

    Token Token::of(CToken* ctok, char* (*fromString)(void*)) {
        return Token {
            static_cast<TokenType>(ctok->type),
            std::string(fromString(ctok->value)),
            SourceLocation::of(ctok->location, fromString)
        };
    }

    std::string Token::toString() const {
        return "Token(value=" + std::string(value) + ", type=" + std::to_string(type) + ", location=" + location.toString() + ")";
    }
    Token::Token() : Token(tok_eof, "") {}
    Token::Token(TokenType type, std::string value) : type(type), value(value), location("", 0, 0) {}
    Token::Token(TokenType type, std::string value, int line, std::string file, int column) : type(type), value(value), location(file, line, column) {}
    Token::Token(TokenType type, std::string value, SourceLocation location) : type(type), value(value), location(location) {}
    Token::Token(const Token& other) : value(other.value.c_str(), other.value.size()), location(other.location) {
        this->type = other.type;
        this->location = other.location;
    }
    Token::Token(Token&& other) : value(static_cast<std::string&&>(other.value)), location(static_cast<SourceLocation&&>(other.location)) {
        this->type = other.type;
    }
    Token& Token::operator=(const Token& other) {
        this->type = other.type;
        this->value = std::string(other.value.c_str(), other.value.size());
        this->location = other.location;
        return *this;
    }
    Token& Token::operator=(Token&& other) {
        this->type = other.type;
        this->value = static_cast<std::string&&>(other.value);
        this->location = static_cast<SourceLocation&&>(other.location);
        return *this;
    }
    Token::~Token() {}
    std::string Token::formatted() const {
        return color() + uncolored_formatted() + Color::RESET;
    }
    std::string Token::uncolored_formatted() const {
        std::string val(value);
        if (type == tok_eof) return "";
        if (type == tok_string_literal) {
            return "\"" + val + "\"";
        } else if (type == tok_char_string_literal) {
            return "c\"" + val + "\"";
        } else if (type == tok_char_literal) {
            auto integerToChar = [](int i) -> std::string {
                std::string s(1, (char)i);
                return s;
            };
            return "'" + integerToChar(std::stoi(val)) + "'";
        }
        return val;
    }

    bool Token::isKeyword() const {
        if (type == tok_eof) return false;
        if (type == tok_identifier) {

            auto contains = [](std::vector<std::string> v, std::string val) -> bool {
                if (v.empty()) return false;

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

    CToken* Token::toC(void*(*alloc)(size_t), void* (*toString)(char*)) const {
        CToken* tok = (CToken*) alloc(sizeof(CToken));
        tok->type = static_cast<long>(type);
        tok->value = toString(strdup(value.data()));
        tok->location = location.toC(alloc, toString);
        return tok;
    }

    std::string Token::color() const {
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
    bool Token::operator==(const Token& other) const {
        return this->type == other.type && this->value == other.value;
    }
    bool Token::operator!=(const Token& other) const {
        return !(*this == other);
    }
    bool Token::operator==(Token& other) {
        return this->type == other.type && this->value == other.value;
    }
    bool Token::operator!=(Token& other) {
        return !(*this == other);
    }
} // namespace sclc
