#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>

#include "../headers/Common.hpp"

#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

#define syntaxError(msg) \
    do { \
    FPResult result; \
    result.message = msg; \
    result.in = filename; \
    result.line = line; \
    result.column = begin; \
    result.value = value; \
    errors.push_back(result); \
    } while (0)

#define syntaxWarn(msg) \
    do { \
    FPResult result; \
    result.message = msg; \
    result.in = filename; \
    result.line = line; \
    result.column = begin; \
    result.value = value; \
    warns.push_back(result); \
    } while (0)

namespace sclc
{
    std::vector<Token> Tokenizer::getTokens() {
        return this->tokens;
    }

    Token Tokenizer::nextToken() {
        if (current >= strlen(source)) {
            return Token(tok_eof, "", line, filename, begin);
        }
        char c = source[current];
        if (c == '\n') {
            line++;
            column = 0;
        }
        std::string value = "";

        begin = column;

        if (isCharacter(c)) {
            while (!isSpace(c) && (isCharacter(c) || isDigit(c))) {
                value += c;
                current++;
                column++;
                c = source[current];
            }
        } else if ((c == '-' && isHexDigit(source[current + 1])) || isHexDigit(c)) {
            bool isFloat = false;
            value += c;
            c = source[++current];
            column++;
            while ((!isSpace(c) && isHexDigit(c)) || (c == '.' && !isFloat)) {
                value += c;
                if (c == '.') {
                    isFloat = true;
                }
                c = source[++current];
                column++;
            }
            if (isFloat) {
                return Token(tok_number_float, value, line, filename, begin);
            } else {
                return Token(tok_number, value, line, filename, begin);
            }
        } else if (c == '"') {
            c = source[++current];
            column++;
            while (c != '"') {
                if (c == '\n' || c == '\r' || c == '\0') {
                    syntaxError("Unterminated string");
                    break;
                }
                
                if (c == '\\') {
                    c = source[++current];
                    column++;
                    switch (c) {
                        case '0':
                        case 't':
                        case 'r':
                        case 'n':
                        case '"':
                        case '\\':
                            value += '\\';
                            value += c;
                            c = source[++current];
                            column++;
                            break;
                        
                        default:
                            syntaxError("Unknown escape sequence: '\\" + std::to_string(c) + "'");
                            break;
                    }
                } else {
                    value += c;
                    c = source[++current];
                    column++;
                }
            }
            current++;
            column++;
            return Token(tok_string_literal, value, line, filename, begin);
        } else if (c == '\'') {
            c = source[++current];
            column++;
            if (c == '\\') {
                c = source[++current];
                column++;
                if (c == 'n') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\n');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, begin);
                } else if (c == 't') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\t');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, begin);
                } else if (c == 'r') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\r');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, begin);
                } else if (c == '\\') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\\');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, begin);
                } else if (c == '\'') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\'');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, begin);
                } else if (c == '\"') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\"');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, begin);
                } else if (c == '0') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", '\0');
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, begin);
                } else {
                    syntaxError("Unknown escape sequence: '\\" + std::to_string(c) + "'");
                }
            } else {
                if (source[current + 1] == '\'') {
                    char* iStr = (char*) malloc(4);
                    snprintf(iStr, 23, "%d", c);
                    current += 2;
                    return Token(tok_char_literal, iStr, line, filename, begin);
                } else {
                    syntaxError("Invalid character literal: '" + std::to_string(c) + "'");
                }
            }
        } else if (isOperator(c) /* && value != "-" */) {
            value += c;
            if (c == '>') {
                if (source[current + 1] == '>' || source[current + 1] == '=') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '<') {
                if (source[current + 1] == '<' || source[current + 1] == '=') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '=') {
                c = source[++current];
                column++;
                if (c == '=' || c == '>') {
                    value += c;
                } else {
                    syntaxError("Expected '=' or '>' after '='");
                }
            } else if (c == '*') {
                if (source[current + 1] == '*') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '-') {
                if (source[current + 1] == '-') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '+') {
                if (source[current + 1] == '+') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '!') {
                if (source[current + 1] == '=' || source[current + 1] == '!') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '&') {
                if (source[current + 1] == '&') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '|') {
                if (source[current + 1] == '|') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == ':') {
                if (source[current + 1] == ':') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            }
            c = source[++current];
            column++;
        } else if (c == '.') {
            value += c;
            c = source[++current];
            column++;
        } else if (isBracket(c)) {
            value += c;
            c = source[++current];
            column++;
        }

        // Not a known token, so probably a space character
        if (value == "") {
            current++;
            column++;
            return nextToken();
        }

        if (value == "store") {
            syntaxWarn("The 'store' keyword is deprecated! Use '=>' instead.");
        }

        if (value == "inline_c") {
            value = "";
            int startLine = line;
            int startColumn = column;
            while (strncmp("end_inline", (source + current), strlen("end_inline")) != 0) {
                value += c;
                c = source[current++];
                column++;
                if (c == '\n') {
                    line++;
                    column = 0;
                }
            }
            current += strlen("end_inline");
            return Token(tok_extern_c, value, startLine, filename, startColumn);
        }

        if (value == "extern") {
            syntaxWarn("'extern' is deprecated! Use 'expect' instead!");
        }

        TOKEN("function",   tok_function, line, filename);
        TOKEN("end",        tok_end, line, filename);
        TOKEN("extern",     tok_extern, line, filename);
        TOKEN("expect",     tok_extern, line, filename);
        TOKEN("while",      tok_while, line, filename);
        TOKEN("do",         tok_do, line, filename);
        TOKEN("done",       tok_done, line, filename);
        TOKEN("if",         tok_if, line, filename);
        TOKEN("then",       tok_then, line, filename);
        TOKEN("else",       tok_else, line, filename);
        TOKEN("elif",       tok_elif, line, filename);
        TOKEN("fi",         tok_fi, line, filename);
        TOKEN("return",     tok_return, line, filename);
        TOKEN("break",      tok_break, line, filename);
        TOKEN("continue",   tok_continue, line, filename);
        TOKEN("for",        tok_for, line, filename);
        TOKEN("foreach",    tok_foreach, line, filename);
        TOKEN("in",         tok_in, line, filename);
        TOKEN("to",         tok_to, line, filename);
        TOKEN("load",       tok_load, line, filename);
        TOKEN("store",      tok_store, line, filename);
        TOKEN("=>",         tok_store, line, filename);
        TOKEN("decl",       tok_declare, line, filename);
        TOKEN("addr",       tok_addr_ref, line, filename);
        TOKEN("nil",        tok_nil, line, filename);
        TOKEN("true",       tok_true, line, filename);
        TOKEN("false",      tok_false, line, filename);
        TOKEN("container",  tok_container_def, line, filename);
        TOKEN("repeat",     tok_repeat, line, filename);
        TOKEN("struct",     tok_struct_def, line, filename);
        TOKEN("new",        tok_new, line, filename);
        TOKEN("is",         tok_is, line, filename);
        TOKEN("cdecl",      tok_cdecl, line, filename);
        TOKEN("goto",       tok_goto, line, filename);
        TOKEN("label",      tok_label, line, filename);
        TOKEN("switch",     tok_switch, line, filename);
        TOKEN("case",       tok_case, line, filename);
        TOKEN("esac",       tok_esac, line, filename);
        TOKEN("default",    tok_default, line, filename);
        TOKEN("step",       tok_step, line, filename);
        TOKEN("interface",  tok_interface_def, line, filename);
        TOKEN("as",         tok_as, line, filename);
        TOKEN("enum",       tok_enum, line, filename);
        
        TOKEN("@",          tok_addr_of, line, filename);
        TOKEN("?",          tok_question_mark, line, filename);
        TOKEN("(",          tok_paren_open, line, filename);
        TOKEN(")",          tok_paren_close, line, filename);
        TOKEN("{",          tok_curly_open, line, filename);
        TOKEN("}",          tok_curly_close, line, filename);
        TOKEN("[",          tok_bracket_open, line, filename);
        TOKEN("]",          tok_bracket_close, line, filename);
        TOKEN(",",          tok_comma, line, filename);
        TOKEN(":",          tok_column, line, filename);
        TOKEN("::",         tok_double_column, line, filename);
        TOKEN("+",          tok_add, line, filename);
        TOKEN("-",          tok_sub, line, filename);
        TOKEN("*",          tok_mul, line, filename);
        TOKEN("/",          tok_div, line, filename);
        TOKEN("%",          tok_mod, line, filename);
        TOKEN("&",          tok_land, line, filename);
        TOKEN("|",          tok_lor, line, filename);
        TOKEN("^",          tok_lxor, line, filename);
        TOKEN("~",          tok_lnot, line, filename);
        TOKEN("<<",         tok_lsh, line, filename);
        TOKEN(">>",         tok_rsh, line, filename);
        TOKEN("**",         tok_pow, line, filename);
        TOKEN(".",          tok_dot, line, filename);
        TOKEN("<",          tok_identifier, line, filename);
        TOKEN("<=",         tok_identifier, line, filename);
        TOKEN(">",          tok_identifier, line, filename);
        TOKEN(">=",         tok_identifier, line, filename);
        TOKEN("==",         tok_identifier, line, filename);
        TOKEN("!",          tok_identifier, line, filename);
        TOKEN("!!",         tok_identifier, line, filename);
        TOKEN("!=",         tok_identifier, line, filename);
        TOKEN("&&",         tok_identifier, line, filename);
        TOKEN("||",         tok_identifier, line, filename);
        TOKEN("++",         tok_identifier, line, filename);
        TOKEN("--",         tok_identifier, line, filename);

        if (current >= strlen(source)) {
            return Token(tok_eof, "", line, filename, begin);
        }
        return Token(tok_identifier, value, line, filename, begin);
    }

    FPResult Tokenizer::tokenize(std::string source) {
        FILE *fp;

        std::string data = "";

        filename = source;
        line = 1;
        
        fp = fopen(source.c_str(), "r");
        int size;
        char* buffer;
        Token token;
        if (fp == NULL) {
            printf("IO Error: Could not open file %s\n", source.c_str());
            FPResult r;
            r.message = "IO Error: Could not open file " + source;
            r.in = "";
            r.line = 0;
            r.success = false;
            errors.push_back(r);
            goto fatal_error;
        }

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (size == 0) {
            goto fatal_error;
        }

        buffer = new char[size + 1];

        while (fgets(buffer, size + 1, fp) != NULL) {
            // skip if comment
            if (std::string(buffer).length() <= 1 || buffer[0] == '\0' || buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '\r') {
                data += "\n";
                continue;
            }

            // remove mid line comments
            char *c = buffer;
            while (*c != '\0') {
                if (*c == '#') {
                    *c = '\n';
                    c++;
                    *c = '\0';
                    break;
                }
                c++;
            }

            // add to data
            data += buffer;
        }

        fclose(fp);

        this->source = (char*) data.c_str();

        current = 0;

        token = nextToken();
        while (token.getType() != tok_eof) {
            this->tokens.push_back(token);
            token = nextToken();
        }


    fatal_error:
        FPResult result;
        result.errors = errors;
        result.warns = warns;
        return result;
    }

    void Tokenizer::printTokens() {
        for (size_t i = 0; i < tokens.size(); i++) {
            std::cout << "Token: " << tokens.at(i).tostring() << std::endl;
        }
    }

    FPResult findFileInIncludePath(std::string file);
    FPResult Tokenizer::tryImports() {
        bool inFunction = false;
        for (ssize_t i = 0; i < (ssize_t) tokens.size(); i++) {
            if (tokens[i].getType() == tok_function && (i - 1 >= 0 ? tokens[i - 1].getValue() != "expect" : true)) {
                inFunction = true;
            } else if (tokens[i].getType() == tok_end) {
                inFunction = false;
            }

            if (inFunction)
                continue;
            if (tokens[i].getType() == tok_identifier && tokens[i].getValue() == "import") {
                i++;
                std::string file = "";
                FPResult r;
                r.column = tokens[i].getColumn();
                r.value = tokens[i].getValue();
                r.in = tokens[i].getFile();
                r.line = tokens[i].getLine();
                r.type = tokens[i].getType();
                while (true) {
                    if (file.size() == 0)
                        file = tokens[i].getValue();
                    else
                        file += PATH_SEPARATOR + tokens[i].getValue();
                    
                    if (tokens[i + 1].getType() != tok_dot) {
                        i--;
                        break;
                    }
                    i += 2;
                }
                file += ".scale";
                FPResult find = findFileInIncludePath(file);
                if (!find.success) {
                    r.success = false;
                    r.message = find.message;
                    return r;
                }
                file = find.in;
                if (std::find(Main.options.files.begin(), Main.options.files.end(), file) == Main.options.files.end()) {
                    Main.options.files.push_back(file);
                }
            }
        }
        FPResult r;
        r.success = true;
        return r;
    }

    FPResult findFileInIncludePath(std::string file) {
        for (std::string path : Main.options.includePaths) {
            using namespace std::filesystem;
            if (exists(path + PATH_SEPARATOR + file)) {
                FPResult r;
                r.success = true;
                if (path == "." || path == "./") {
                    r.in = file;
                } else {
                    r.in = path + PATH_SEPARATOR + file;
                }
                return r;
            }
        }
        FPResult r;
        r.success = false;
        r.message = "Could not find " + file + " on include path!";
        return r;
    }
}
