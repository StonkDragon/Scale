#include <gc/gc_allocator.h>

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>

#include "../headers/Common.hpp"

#ifndef PATH_SEPARATOR
#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif
#endif

#define syntaxError(msg) \
    do { \
    FPResult result; \
    result.message = msg; \
    result.location = SourceLocation(filename, line, begin); \
    result.value = value; \
    errors.push_back(result); \
    } while (0)

#define syntaxWarn(msg) \
    do { \
    FPResult result; \
    result.message = msg; \
    result.location = SourceLocation(filename, line, begin); \
    result.value = value; \
    warns.push_back(result); \
    } while (0)

namespace sclc
{
    std::vector<Token> Tokenizer::getTokens() {
        return this->tokens;
    }

    enum class StringType {
        CString,
        UnicodeString,
        String,
    };

    Tokenizer::Tokenizer() {current = 0;}
    Tokenizer::~Tokenizer() {}

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

        StringType stringType = StringType::String;

        if (isCharacter(c)) {
            while (!isSpace(c) && (isCharacter(c) || isDigit(c))) {
                value += c;
                current++;
                column++;
                c = source[current];
            }
            if (value == "c" && c == '"') {
                stringType = StringType::CString;
                value = "";
                goto stringLiteral;
            }
            if (value == "u" && c == '"') {
                stringType = StringType::UnicodeString;
                value = "";
                goto stringLiteral;
            }
        } else if ((c == '-' && isDigit(source[current + 1])) || isDigit(c)) {
            if (c == '-') {
                value += c;
                c = source[++current];
                column++;
            }
            bool isFloat = false;
            value += c;
            c = source[++current];
            column++;
            int(*validDigit)(char) = isDigit;
            if (c == 'x' || c == 'X') {
                validDigit = isHexDigit;
            } else if (c == 'o' || c == 'O') {
                validDigit = isOctDigit;
            } else if (c == 'b' || c == 'B') {
                validDigit = isBinDigit;
            }
            while (validDigit(c) || (c == '.' && !isFloat)) {
                value += c;
                if (c == '.') {
                    isFloat = true;
                }
                c = source[++current];
                column++;
            }
            if (isFloat) {
                if (c == 'f' || c == 'F' || c == 'd' || c == 'D') {
                    if (c == 'f' || c == 'F') {
                        value += 'f';
                    } else if (c == 'd' || c == 'D') {
                        value += 'd';
                    }
                    c = source[++current];
                    column++;
                }
                return Token(tok_number_float, value, line, filename, begin);
            } else {
                if (c == 'i' || c == 'u' || c == 'I' || c == 'U') {
                    if (c == 'i' || c == 'I') {
                        value += 'i';
                    } else {
                        value += 'u';
                    }
                    c = source[++current];
                    column++;
                    if (c == '8' || c == '1' || c == '3' || c == '6') {
                        if (c == '8') {
                            value += '8';
                            c = source[++current];
                            column++;
                        } else {
                            value += c;
                            c = source[++current];
                            column++;
                            if (c == '2' || c == '4' || c == '6') {
                                value += c;
                                c = source[++current];
                                column++;
                            } else {
                                syntaxError("Invalid integer suffix");
                            }
                        }
                    }
                } else if (validDigit != isHexDigit && (c == 'f' || c == 'F' || c == 'd' || c == 'D')) {
                    if (c == 'f' || c == 'F') {
                        value += 'f';
                    } else if (c == 'd' || c == 'D') {
                        value += 'd';
                    }
                    c = source[++current];
                    column++;
                    return Token(tok_number_float, value, line, filename, begin);
                }
                return Token(tok_number, value, line, filename, begin);
            }
        } else if (c == '"') {
        stringLiteral:
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
            switch (stringType) {
                case StringType::CString: return Token(tok_char_string_literal, value, line, filename, begin);
                case StringType::UnicodeString: return Token(tok_utf_string_literal, value, line, filename, begin);
                case StringType::String: return Token(tok_string_literal, value, line, filename, begin);
                default: __builtin_unreachable();
            }
        } else if (c == '\'') {
            c = source[++current];
            value += c;
            column++;
            if (c == '\\') {
                c = source[++current];
                value += c;
                column++;
                if (c == 'n' || c == 't' || c == 'r' || c == '\\' || c == '\'' || c == '\"' || c == '0') {
                    current += 2;
                    return Token(tok_char_literal, value, line, filename, begin);
                } else {
                    syntaxError("Unknown escape sequence: '" + value + "'");
                }
            } else {
                if (source[current + 1] == '\'') {
                    current += 2;
                    return Token(tok_char_literal, value, line, filename, begin);
                } else {
                    value = "";
                    while (!isSpace(c) && (isCharacter(c) || isDigit(c))) {
                        value += c;
                        current++;
                        column++;
                        c = source[current];
                    }

                    current++;
                    column++;
                    return Token(tok_ticked, value, line, filename, begin);
                }
            }
        } else if (isOperator(c)) {
            value += c;
            if (c == '>') {
                if (source[current + 1] == '=') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '<') {
                if (source[current + 1] == '<' || source[current + 1] == '=') {
                    c = source[++current];
                    column++;
                    value += c;
                    if (source[current] == '<' && source[current + 1] == '<') {
                        c = source[++current];
                        column++;
                        value += c;
                    }
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
                if (source[current + 1] == '*' || source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '/') {
                if (source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '^') {
                if (source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '-') {
                if (source[current + 1] == '-' || source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '+') {
                if (source[current + 1] == '+' || source[current + 1] == '>') {
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
            } else if (c == '?') {
                if (source[current + 1] == '.' || source[current + 1] == ':') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '&') {
                if (source[current + 1] == '&' || source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '|') {
                if (source[current + 1] == '|' || source[current + 1] == '>') {
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
            } else if (c == '~') {
                if (source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value += c;
                }
            } else if (c == '%') {
                if (source[current + 1] == '>') {
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
        } else if (c == '$') {
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

        if (c == '!') {
            if (value == "pragma" || value == "c" || value == "macro" || value == "deprecated") {
                value += c;
                c = source[++current];
                column++;
            }
        }

        if (value == "c!") {
            value = "";
            int startLine = line;
            int startColumn = column;
            while (strncmp("end", (source + current), 3) != 0) {
                value += c;
                c = source[current++];
                column++;
                if (c == '\n') {
                    line++;
                    column = 0;
                }
            }
            current += 3;
            return Token(tok_extern_c, value, startLine, filename, startColumn);
        }

        TOKEN("function",   tok_function, line, filename);
        TOKEN("end",        tok_end, line, filename);
        TOKEN("expect",     tok_extern, line, filename);
        TOKEN("while",      tok_while, line, filename);
        TOKEN("until",      tok_until, line, filename);
        TOKEN("do",         tok_do, line, filename);
        TOKEN("done",       tok_done, line, filename);
        TOKEN("if",         tok_if, line, filename);
        TOKEN("unless",     tok_unless, line, filename);
        TOKEN("then",       tok_then, line, filename);
        TOKEN("else",       tok_else, line, filename);
        TOKEN("elif",       tok_elif, line, filename);
        TOKEN("elunless",   tok_elunless, line, filename);
        TOKEN("fi",         tok_fi, line, filename);
        TOKEN("return",     tok_return, line, filename);
        TOKEN("break",      tok_break, line, filename);
        TOKEN("continue",   tok_continue, line, filename);
        TOKEN("for",        tok_for, line, filename);
        TOKEN("foreach",    tok_foreach, line, filename);
        TOKEN("in",         tok_in, line, filename);
        TOKEN("to",         tok_to, line, filename);
        TOKEN("load",       tok_load, line, filename);
        TOKEN("=>",         tok_store, line, filename);
        TOKEN("decl",       tok_declare, line, filename);
        TOKEN("ref",        tok_addr_ref, line, filename);
        TOKEN("nil",        tok_nil, line, filename);
        TOKEN("true",       tok_true, line, filename);
        TOKEN("false",      tok_false, line, filename);
        TOKEN("container",  tok_container_def, line, filename);
        TOKEN("repeat",     tok_repeat, line, filename);
        TOKEN("struct",     tok_struct_def, line, filename);
        TOKEN("union",      tok_union_def, line, filename);
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

        TOKEN("swap",       tok_stack_op, line, filename);
        TOKEN("dup",        tok_stack_op, line, filename);
        TOKEN("drop",       tok_stack_op, line, filename);
        TOKEN("over",       tok_stack_op, line, filename);
        TOKEN("sdup2",      tok_stack_op, line, filename);
        TOKEN("swap2",      tok_stack_op, line, filename);
        TOKEN("rot",        tok_stack_op, line, filename);
        TOKEN("unrot",      tok_stack_op, line, filename);

        TOKEN("using",      tok_using, line, filename);
        TOKEN("pragma!",    tok_pragma, line, filename);
        TOKEN("lambda",     tok_lambda, line, filename);
        
        if (value == "+>" || value == "->" || value == "*>" || value == "/>" || value == "&>" || value == "|>" || value == "^>" || value == "%>") {
            additional = true;
            additionalToken = Token(tok_store, "=>", line, filename, begin);
            if (value == "+>") {
                return Token(tok_identifier, "+", line, filename, begin);
            } else if (value == "->") {
                return Token(tok_identifier, "-", line, filename, begin);
            } else if (value == "*>") {
                return Token(tok_identifier, "*", line, filename, begin);
            } else if (value == "/>") {
                return Token(tok_identifier, "/", line, filename, begin);
            } else if (value == "&>") {
                return Token(tok_identifier, "&", line, filename, begin);
            } else if (value == "|>") {
                return Token(tok_identifier, "|", line, filename, begin);
            } else if (value == "^>") {
                return Token(tok_identifier, "^", line, filename, begin);
            } else if (value == "%>") {
                return Token(tok_identifier, "%", line, filename, begin);
            }
            __builtin_unreachable();
        }

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
        TOKEN(".",          tok_dot, line, filename);
        TOKEN("?.",         tok_dot, line, filename);
        TOKEN("$",          tok_dollar, line, filename);

        return Token(tok_identifier, value, line, filename, begin);
    }

    FPResult Tokenizer::tokenize(std::string source) {
        current = 0;
        FILE *fp;

        std::string data = "";

        filename = source;
        line = 1;
        
        fp = fopen(source.c_str(), "r");
        int size;
        char* buffer;
        Token token;
        bool inBlockComment = false;
        if (fp == NULL) {
            std::cerr << "IO Error: Could not open file " << source << std::endl;
            FPResult r;
            r.message = "IO Error: Could not open file " + source;
            r.location = SourceLocation(source, 0, 0);
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
            if (buffer[0] == '\0' || buffer[0] == '\n' || buffer[0] == '\r') {
                data += "\n";
                continue;
            }

            // remove mid line comments
            char *c = buffer;
            int inStr = 0;
            int escaped = 0;
            while (*c != '\0') {
                if (inBlockComment) {
                    if (*c == '`') {
                        inBlockComment = !inBlockComment;
                        *c = ' ';
                    } else {
                        if (*c != '\n')
                            *c = ' ';
                    }
                } else if (*c == '\\') {
                    escaped = !escaped;
                } else {
                    escaped = false;
                    if (*c == '"' && !escaped) {
                        inStr = !inStr;
                    } else if (*c == '#' && !inStr) {
                        *c = '\n';
                        c++;
                        *c = '\0';
                        break;
                    } else if (*c == '`' && !inStr) {
                        inBlockComment = !inBlockComment;
                        *c = ' ';
                    }
                }
                c++;
            }

            // add to data
            data += buffer;
        }

        fclose(fp);

        this->source = (char*) data.c_str();

        token = nextToken();
        while (token.type != tok_eof) {
            this->tokens.push_back(token);
            if (additional) {
                this->tokens.push_back(additionalToken);
                additional = false;
            }
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
            std::cout << "Token: " << tokens[i].toString() << std::endl;
        }
    }

    void Tokenizer::removeInvalidTokens() {
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type == tok_eof) {
                tokens.erase(tokens.begin() + i);
                i--;
            }
        }
    }

    std::string replaceCharWithChar(std::string in, char from, char to) {
        std::string out = "";
        out.reserve(in.size());
        for (size_t i = 0; i < in.size(); i++) {
            if (in[i] == from) {
                out.push_back(to);
            } else {
                out.push_back(in[i]);
            }
        }
        return out;
    }

    FPResult findFileInIncludePath(std::string file);
    FPResult Tokenizer::tryImports() {
        for (ssize_t i = 0; i < (ssize_t) tokens.size(); i++) {
            if (tokens[i].type != tok_identifier) continue;
            if (tokens[i].value != "import") continue;

            i++;
            if (i >= (long) tokens.size()) {
                FPResult r;
                r.message = "Expected module name after import";
                r.location = tokens[i - 1].location;
                r.success = false;
                return r;
            }
            std::string moduleName = tokens[i].value;
            while (i + 1 < (long long) tokens.size() && tokens[i + 1].type == tok_dot) {
                i += 2;
                moduleName += "." + tokens[i].value;
            }
            if (moduleName.empty()) {
                FPResult r;
                r.message = "Expected module name after import";
                r.location = tokens[i - 1].location;
                r.success = false;
                return r;
            }
            bool found = false;
            for (auto config : Main::options::indexDrgFiles) {
                if (!config.second) continue;

                auto modules = config.second->getCompound("modules");
                if (!modules) continue;
                
                auto list = modules->getList(moduleName);
                if (!list) continue;
                found = true;

                for (size_t j = 0; j < list->size(); j++) {
                    FPResult find = findFileInIncludePath(list->getString(j)->getValue());
                    if (!find.success) {
                        FPResult r;
                        r.value = tokens[i].value;
                        r.location = tokens[i].location;
                        r.type = tokens[i].type;
                        r.success = false;
                        r.message = find.message;
                        return r;
                    }
                    auto file = find.location.file;
                    file = std::filesystem::absolute(file).string();
                    if (!contains(Main::options::files, file)) {
                        Main::options::files.push_back(file);
                    }
                }
                break;
            }
            if (found) continue;

            std::string file = replaceCharWithChar(moduleName, '.', PATH_SEPARATOR[0]);
            file += ".scale";
            FPResult r = findFileInIncludePath(file);
            if (!r.success) {
                r.location = tokens[i].location;
                return r;
            }
            file = std::filesystem::absolute(r.location.file).string();
            if (!contains(Main::options::files, file)) {
                Main::options::files.push_back(file);
            }
        }
        FPResult r;
        r.success = true;
        return r;
    }

    FPResult findFileInIncludePath(std::string file) {
        if (file.empty() || file == ".scale") {
            FPResult r;
            r.success = false;
            r.message = "Empty file name!";
            r.location = SourceLocation("", 0, 0);
            return r;
        }
        for (std::string& path : Main::options::includePaths) {
            if (!std::filesystem::exists(path + PATH_SEPARATOR + file)) continue;
            FPResult r;
            r.success = true;
            if (path == "." || path == "./") {
                r.location.file = file;
            } else {
                r.location.file = path + PATH_SEPARATOR + file;
            }
            return r;
        }
        FPResult r;
        r.success = false;
        r.message = "Could not find " + file + " on include path!";
        r.location = SourceLocation(file, 0, 0);
        return r;
    }
}
