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

        if (c == '\n' || c == '\0') {
            line++;
            column = 0;
            startColumn = 0;
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
                    value += c;
                    column++;
                }
            } else if (c == '+') {
                if (source[current + 1] == '+') {
                    c = source[++current];
                    value += c;
                    column++;
                }
            } else if (c == '!') {
                if (source[current + 1] == '=') {
                    c = source[++current];
                    value += c;
                    column++;
                }
            } else if (c == '&') {
                if (source[current + 1] == '&') {
                    c = source[++current];
                    value += c;
                    column++;
                }
            } else if (c == '|') {
                if (source[current + 1] == '|') {
                    c = source[++current];
                    value += c;
                    column++;
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

        if (value == "store") {
            FPResult result;
            result.message = "The 'store' keyword is deprecated! Use '=>' instead.";
            result.in = filename;
            result.line = line;
            result.column = startColumn;
            result.value = value;
            warns.push_back(result);
        }

        #define debug(_s) printf(#_s ": %d\n", (_s));
        if (value == "inline_c") {
            value = "";
            startColumn = column;
            while (strncmp("end_inline", (source + current), strlen("end_inline")) != 0) {
                if (c == '\n') {
                    line++;
                    column = 0;
                }
                value += c;
                c = source[current++];
                column++;
            }
            current += strlen("end_inline");
            column += strlen("end_inline");
            return Token(tok_extern_c, value, 0, filename, startColumn);
        }

        TOKEN("function",   tok_function, line, filename, startColumn);
        TOKEN("end",        tok_end, line, filename, startColumn);
        TOKEN("extern",     tok_extern, line, filename, startColumn);
        TOKEN("while",      tok_while, line, filename, startColumn);
        TOKEN("do",         tok_do, line, filename, startColumn);
        TOKEN("done",       tok_done, line, filename, startColumn);
        TOKEN("if",         tok_if, line, filename, startColumn);
        TOKEN("then",       tok_then, line, filename, startColumn);
        TOKEN("else",       tok_else, line, filename, startColumn);
        TOKEN("elif",       tok_elif, line, filename, startColumn);
        TOKEN("fi",         tok_fi, line, filename, startColumn);
        TOKEN("return",     tok_return, line, filename, startColumn);
        TOKEN("break",      tok_break, line, filename, startColumn);
        TOKEN("continue",   tok_continue, line, filename, startColumn);
        TOKEN("for",        tok_for, line, filename, startColumn);
        TOKEN("foreach",    tok_foreach, line, filename, startColumn);
        TOKEN("in",         tok_in, line, filename, startColumn);
        TOKEN("to",         tok_to, line, filename, startColumn);
        TOKEN("load",       tok_load, line, filename, startColumn);
        TOKEN("store",      tok_store, line, filename, startColumn);
        TOKEN("=>",         tok_store, line, filename, startColumn);
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
        TOKEN("switch",     tok_switch, line, filename, startColumn);
        TOKEN("case",       tok_case, line, filename, startColumn);
        TOKEN("esac",       tok_esac, line, filename, startColumn);
        TOKEN("default",    tok_default, line, filename, startColumn);
        TOKEN("step",       tok_step, line, filename, startColumn);
        TOKEN("interface",  tok_interface_def, line, filename, startColumn);
        TOKEN("as",  tok_as, line, filename, startColumn);
        
        if (inFunction) {
            TOKEN("@",      tok_addr_of, line, filename, startColumn);
        } else {
            TOKEN("@",      tok_hash, line, filename, startColumn);
        }
        TOKEN("?",          tok_question_mark, line, filename, startColumn);
        TOKEN("(",          tok_paren_open, line, filename, startColumn);
        TOKEN(")",          tok_paren_close, line, filename, startColumn);
        TOKEN("{",          tok_curly_open, line, filename, startColumn);
        TOKEN("}",          tok_curly_close, line, filename, startColumn);
        TOKEN("[",          tok_bracket_open, line, filename, startColumn);
        TOKEN("]",          tok_bracket_close, line, filename, startColumn);
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
        TOKEN(":",          tok_column, line, filename, startColumn);
        TOKEN("<",          tok_identifier, line, filename, startColumn);
        TOKEN("<=",         tok_identifier, line, filename, startColumn);
        TOKEN(">",          tok_identifier, line, filename, startColumn);
        TOKEN(">=",         tok_identifier, line, filename, startColumn);
        TOKEN("==",         tok_identifier, line, filename, startColumn);
        TOKEN("!",          tok_identifier, line, filename, startColumn);
        TOKEN("!=",         tok_identifier, line, filename, startColumn);
        TOKEN("&&",         tok_identifier, line, filename, startColumn);
        TOKEN("||",         tok_identifier, line, filename, startColumn);
        TOKEN("++",         tok_identifier, line, filename, startColumn);
        TOKEN("--",         tok_identifier, line, filename, startColumn);

        if (current >= strlen(source)) {
            return Token(tok_eof, "", line, filename, startColumn);
        }
        return Token(tok_identifier, value, line, filename, startColumn);
    }

    FPResult Tokenizer::tokenize(std::string source) {
        FILE *fp;

        std::string data = "";

        filename = source;
        line = 1;
        column = 0;
        startColumn = 0;
        
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
            r.column = 0;
            r.success = false;
            errors.push_back(r);
            goto fatal_error;
        }

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

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
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].getType() == tok_function) {
                inFunction = true;
            } else if (tokens[i].getType() == tok_end) {
                inFunction = false;
            }

            if (inFunction)
                continue;
            if (tokens[i].getType() == tok_identifier && tokens[i].getValue() == "import") {
                i++;
                std::string framework = tokens[i].getValue();
                i += 2;
                std::string file = tokens[i].getValue();
                while (tokens[i + 1].getType() == tok_dot) {
                    i++;
                    file += "/" + tokens[i + 1].getValue();
                }

                std::string fullFile = Main.options.mapIncludePathsToFrameworks[framework] + "/" + file + ".scale";
                if (std::find(Main.options.files.begin(), Main.options.files.end(), fullFile) == Main.options.files.end()) {
                    std::cout << "Importing: " << fullFile << std::endl;
                    Main.options.files.push_back(fullFile);
                }
            }
        }
        FPResult r;
        r.success = true;
        return r;
    }

    FPResult findFileInIncludePath(std::string file) {
        for (std::string path : Main.options.includePaths) {
            if (fileExists(path + "/" + file)) {
                FPResult r;
                r.success = true;
                if (path == "." || path == "./") {
                    r.in = file;
                } else {
                    r.in = path + "/" + file;
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
