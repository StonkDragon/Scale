
#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>

#include <Common.hpp>

#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR DIR_SEP
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

    Tokenizer::Tokenizer() { reset(); }
    Tokenizer::~Tokenizer() {}

    void Tokenizer::reset() {
        tokens.clear();
        errors.clear();
        warns.clear();
        source = nullptr;
        current = 0;
        additional = false;
        line = 1;
        column = 1;
        begin = 1;
        filename = "";
        sourceLen = 0;
    }

    Token Tokenizer::nextToken() {
        if (current >= sourceLen) {
            return Token(tok_eof, "", line, filename, begin);
        }
        char c = source[current];
        if (c == '\n') {
            line++;
            column = 0;
        }
        std::string value;
        value.reserve(32);

        begin = column;

        StringType stringType = StringType::String;

        if (isCharacter(c)) {
            while (!isSpace(c) && (isCharacter(c) || isDigit(c))) {
                value.push_back(c);
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
                value.push_back(c);
                c = source[++current];
                column++;
            }
            bool isFloat = false;
            value.push_back(c);
            c = source[++current];
            column++;
            int(*validDigit)(char);
            switch (c) {
                case 'x': case 'X': validDigit = isHexDigit; break;
                case 'o': case 'O': validDigit = isOctDigit; break;
                case 'b': case 'B': validDigit = isBinDigit; break;
                default: validDigit = isDigit; break;
            }
            while (validDigit(c) || (c == '.' && !isFloat)) {
                value.push_back(c);
                if (c == '.') {
                    isFloat = true;
                }
                c = source[++current];
                column++;
            }
            if (isFloat) {
                if (c == 'f' || c == 'F' || c == 'd' || c == 'D') {
                    if (c == 'f' || c == 'F') {
                        value.push_back('f');
                    } else if (c == 'd' || c == 'D') {
                        value.push_back('d');
                    }
                    c = source[++current];
                    column++;
                }
                return Token(tok_number_float, value, line, filename, begin);
            } else {
                if (c == 'i' || c == 'u' || c == 'I' || c == 'U') {
                    if (c == 'i' || c == 'I') {
                        value.push_back('i');
                    } else {
                        value.push_back('u');
                    }
                    c = source[++current];
                    column++;
                    if (c == '8' || c == '1' || c == '3' || c == '6') {
                        if (c == '8') {
                            value.push_back('8');
                            c = source[++current];
                            column++;
                        } else {
                            value.push_back(c);
                            c = source[++current];
                            column++;
                            if (c == '2' || c == '4' || c == '6') {
                                value.push_back(c);
                                c = source[++current];
                                column++;
                            } else {
                                syntaxError("Invalid integer suffix");
                            }
                        }
                    }
                } else if (validDigit == isDigit && (c == 'f' || c == 'F' || c == 'd' || c == 'D')) {
                    if (c == 'f' || c == 'F') {
                        value.push_back('f');
                    } else if (c == 'd' || c == 'D') {
                        value.push_back('d');
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
                            value.push_back('\\');
                            value.push_back(c);
                            c = source[++current];
                            column++;
                            break;
                        
                        default:
                            syntaxError("Unknown escape sequence: '\\" + std::to_string(c) + "'");
                            break;
                    }
                } else {
                    value.push_back(c);
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
            value.push_back(c);
            column++;
            if (c == '\\') {
                c = source[++current];
                value.push_back(c);
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
                        value.push_back(c);
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
            value.push_back(c);
            if (c == '>') {
                if (source[current + 1] == '=' || source[current + 1] == '.') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == '<') {
                if (source[current + 1] == '<' || source[current + 1] == '=' || source[current + 1] == '.') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                    if (source[current] == '<' && source[current + 1] == '<') {
                        c = source[++current];
                        column++;
                        value.push_back(c);
                    }
                }
            } else if (c == '=') {
                c = source[++current];
                column++;
                if (c == '=' || c == '>') {
                    value.push_back(c);
                } else {
                    syntaxError("Expected '=' or '>' after '='");
                }
            } else if (c == '*') {
                if (source[current + 1] == '*' || source[current + 1] == '>' || source[current + 1] == '.') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == '/') {
                if (source[current + 1] == '>' || source[current + 1] == '.') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == '^') {
                if (source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == '-') {
                if (source[current + 1] == '-' || source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == '+') {
                if (source[current + 1] == '+' || source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == '!') {
                if (source[current + 1] == '=' || source[current + 1] == '!') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == '?') {
                if (source[current + 1] == '.' || source[current + 1] == ':') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == '&') {
                if (source[current + 1] == '&' || source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == '|') {
                if (source[current + 1] == '|' || source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == ':') {
                if (source[current + 1] == ':') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == '~') {
                if (source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            } else if (c == '%') {
                if (source[current + 1] == '>') {
                    c = source[++current];
                    column++;
                    value.push_back(c);
                }
            }
            c = source[++current];
            column++;
        } else if (c == '.') {
            value.push_back(c);
            c = source[++current];
            column++;
        } else if (isBracket(c)) {
            value.push_back(c);
            c = source[++current];
            column++;
        } else if (c == '$') {
            value.push_back(c);
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
                value.push_back(c);
                c = source[++current];
                column++;
            }
        }

        if (value == "c!") {
            value = "";
            int startLine = line;
            int startColumn = column;
            while (strncmp("end", (source + current), 3) != 0) {
                value.push_back(c);
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
        
        if (value == "+>" || value == "->" || value == "*>" || value == "/>" || value == "&>" || value == "|>" || value == "^>" || value == "%>") {
            additional = true;
            additionalToken = Token(tok_store, "=>", line, filename, begin);
            return Token(tok_identifier, std::string(1, value.front()), line, filename, begin);
        }

        TOKEN("as",         tok_as, line, filename);
        TOKEN("break",      tok_break, line, filename);
        TOKEN("continue",   tok_continue, line, filename);
        TOKEN("do",         tok_do, line, filename);
        TOKEN("done",       tok_done, line, filename);
        TOKEN("end",        tok_end, line, filename);
        TOKEN("else",       tok_else, line, filename);
        TOKEN("elif",       tok_elif, line, filename);
        TOKEN("elunless",   tok_elunless, line, filename);
        TOKEN("fi",         tok_fi, line, filename);
        TOKEN("function",   tok_function, line, filename);
        TOKEN("while",      tok_while, line, filename);
        TOKEN("until",      tok_until, line, filename);
        TOKEN("if",         tok_if, line, filename);
        TOKEN("unless",     tok_unless, line, filename);
        TOKEN("then",       tok_then, line, filename);
        TOKEN("return",     tok_return, line, filename);
        TOKEN("for",        tok_for, line, filename);
        TOKEN("foreach",    tok_foreach, line, filename);
        TOKEN("in",         tok_in, line, filename);
        TOKEN("to",         tok_to, line, filename);
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
        TOKEN("switch",     tok_switch, line, filename);
        TOKEN("case",       tok_case, line, filename);
        TOKEN("esac",       tok_esac, line, filename);
        TOKEN("default",    tok_default, line, filename);
        TOKEN("step",       tok_step, line, filename);
        TOKEN("interface",  tok_interface_def, line, filename);
        TOKEN("enum",       tok_enum, line, filename);
        TOKEN("using",      tok_using, line, filename);
        TOKEN("pragma!",    tok_pragma, line, filename);
        TOKEN("lambda",     tok_lambda, line, filename);
        TOKEN("await",      tok_await, line, filename);
        TOKEN("typeof",     tok_typeof, line, filename);
        TOKEN("typeid",     tok_typeid, line, filename);
        TOKEN("nameof",     tok_nameof, line, filename);
        TOKEN("sizeof",     tok_sizeof, line, filename);
        TOKEN("try",        tok_try, line, filename);
        TOKEN("catch",      tok_catch, line, filename);
        TOKEN("unsafe",     tok_unsafe, line, filename);
        TOKEN("assert",     tok_assert, line, filename);
        TOKEN("varargs",    tok_varargs, line, filename);

        TOKEN("swap",       tok_stack_op, line, filename);
        TOKEN("dup",        tok_stack_op, line, filename);
        TOKEN("drop",       tok_stack_op, line, filename);
        TOKEN("over",       tok_stack_op, line, filename);
        TOKEN("sdup2",      tok_stack_op, line, filename);
        TOKEN("swap2",      tok_stack_op, line, filename);
        TOKEN("rot",        tok_stack_op, line, filename);
        TOKEN("unrot",      tok_stack_op, line, filename);

        TOKEN("=>",         tok_store, line, filename);

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
        int size;
        char* buffer;
        Token token;
        bool inBlockComment = false;
        
        fp = fopen(source.c_str(), "r");
        if (fp == NULL) {
            FPResult r;
            r.message = "IO Error: Could not open file " + source + ": " + strerror(errno);
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
        this->sourceLen = strlen(this->source);

        token = nextToken();
        
        this->tokens.reserve(data.size() + this->tokens.capacity());

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
        FPResult res;
        for (ssize_t i = 0; i < (ssize_t) tokens.size(); i++) {
            if (tokens[i].type != tok_identifier) continue;
            if (tokens[i].value != "import") continue;

            ssize_t start = i;
            SourceLocation beg = tokens[i].location;

            i++;
            if (i >= (long) tokens.size()) {
                FPResult r;
                r.message = "Expected module name after import";
                r.location = tokens[i - 1].location;
                r.success = false;
                res.errors.push_back(r);
                continue;
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
                res.errors.push_back(r);
                continue;
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
                        find.location = beg;
                        res.errors.push_back(find);
                        goto cont;
                    }
                    auto file = find.location.file;
                    file = std::filesystem::absolute(file).string();
                    if (!contains(Main::options::files, file)) {
                        Main::options::files.push_back(file);
                    }
                }
                break;
            }
            
            tokens.erase(tokens.begin() + start, tokens.begin() + i + 1);
            i = start - 1;

            if (!found) {
                std::string file = replaceCharWithChar(moduleName, '.', PATH_SEPARATOR[0]);
                file += ".scale";
                FPResult r = findFileInIncludePath(file);
                if (!r.success) {
                    r.location = beg;
                    res.errors.push_back(r);
                    goto cont;
                }
                file = std::filesystem::absolute(r.location.file).string();
                if (!contains(Main::options::files, file)) {
                    Main::options::files.push_back(file);
                }
            }
        cont:;
        }
        res.success = res.errors.empty();
        return res;
    }

    FPResult findFileInIncludePath(std::string file) {
        if (file.empty() || file == ".scale") {
            FPResult r;
            r.success = false;
            r.message = "Empty file name!";
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
        return r;
    }
}
