#ifndef _PARSER_CPP_
#define _PARSER_CPP_

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "Common.h"

#include "Lexer.cpp"
#include "Tokenizer.cpp"

#undef INT_MAX
#undef INT_MIN
#undef LONG_MAX
#undef LONG_MIN

#define INT_MAX 0x7FFFFFFF
#define INT_MIN 0x80000000
#define LONG_MAX 0x7FFFFFFFFFFFFFFF
#define LONG_MIN 0x8000000000000000ll

#define LINE_LENGTH 48

ParseResult Parser::parse(std::string filename) {
    std::fstream fp((filename + std::string(".c")).c_str(), std::ios::out);
    std::fstream header((filename + std::string(".h")).c_str(), std::ios::out);
    header << "#ifndef __SCALE_COMP_H" << std::endl;
    header << "#define __SCALE_COMP_H" << std::endl;

    fp << "#ifdef __cplusplus" << std::endl;
    fp << "extern \"C\" {" << std::endl;
    fp << "#endif" << std::endl;

    header << "#ifdef __cplusplus" << std::endl;
    header << "extern \"C\" {" << std::endl;
    header << "#endif" << std::endl;

    fp << "#include \"" << getenv("HOME") << "/Scale/comp/scale.h" << "\"" << std::endl;
    header << "#include \"" << getenv("HOME") << "/Scale/comp/scale.h" << "\"" << std::endl;

    for (Function function : result.functions) {
        for (Modifier modifier : function.modifiers) {
            switch (modifier) {
                case mod_static:
                    fp << "static ";
                    break;
                case mod_inline:
                    fp << "inline ";
                    break;
                case mod_extern:
                    fp << "extern ";
                    break;
            
                default:
                    break;
            }
        }
        fp << "void scale_func_" << function.getName() << "();" << std::endl;
        header << "void scale_func_" << function.getName() << "();" << std::endl;
    }

    for (Extern extern_ : result.externs) {
        fp << "void scale_extern_" << extern_.name << "();" << std::endl;
    }

    for (Function function : result.functions)
    {
        bool isStatic = false;
        bool isExtern = false;
        bool isInline = false;
        std::streampos lstart = fp.tellp();
        for (Modifier modifier : function.modifiers) {
            switch (modifier) {
                case mod_static:
                    isStatic = true;
                    fp << "static ";
                    break;
                case mod_inline:
                    isInline = true;
                    fp << "inline ";
                    break;
                case mod_extern:
                    isExtern = true;
                    fp << "extern ";
                    break;
            
                default:
                    break;
            }
        }
        fp << "void scale_func_" << function.getName() << "() {";
        std::streampos pos = fp.tellp();
        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
            fp << " ";
        }
        fp << "/* function " << function.getName() << "() */" << std::endl;
        if (!isInline) fp << "stacktrace_push(\"" << function.getName() << "\");" << std::endl;
        std::vector<Token> body = function.getBody();
        int scopeDepth = 1;
        for (int i = 0; i < body.size(); i++)
        {
            if (body[i].getType() == tok_ignore) continue;
            if (body[i].getType() == tok_identifier && hasFunction(body[i].getValue())) {
                lstart = fp.tellp();
                fp << "scale_func_" << body[i].getValue() << "();";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */" << std::endl;
            } else if (body[i].getType() == tok_identifier && hasExtern(body[i].getValue())) {
                fp << "stacktrace_push(\"native:" << body[i].getValue() << "\");" << std::endl;
                lstart = fp.tellp();
                fp << "scale_extern_" << body[i].getValue() << "();";
                pos = fp.tellp();
                for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                    fp << " ";
                }
                fp << "/* " << body[i].getValue() << " */" << std::endl;
                fp << "stacktrace_pop();" << std::endl;
            } else {
                if (body[i].getType() == tok_string_literal) {
                    lstart = fp.tellp();
                    fp << "scale_push_string(\"" << body[i].getValue() << "\");";
                    pos = fp.tellp();
                    for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                        fp << " ";
                    }
                    fp << "/* " << body[i].getValue() << " */" << std::endl;
                } else if (body[i].getType() == tok_number) {
                    
                    long long num;
                    try
                    {
                        if (body[i].getValue().substr(0, 2) == "0x") {
                            num = std::stoll(body[i].getValue().substr(2), nullptr, 16);
                        } else if (body[i].getValue().substr(0, 2) == "0b") {
                            num = std::stoll(body[i].getValue().substr(2), nullptr, 2);
                        } else if (body[i].getValue().substr(0, 2) == "0o") {
                            num = std::stoll(body[i].getValue().substr(2), nullptr, 8);
                        } else {
                            num = std::stoll(body[i].getValue());
                        }
                        lstart = fp.tellp();
                        fp << "scale_push_long(" << num << ");";
                        pos = fp.tellp();
                        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                            fp << " ";
                        }
                        fp << "/* " << body[i].getValue() << " */" << std::endl;
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << "Number out of range: " << body[i].getValue() << std::endl;
                        ParseResult result;
                        result.success = false;
                        return result;
                    }
                } else if (body[i].getType() == tok_nil) {
                    lstart = fp.tellp();
                    fp << "scale_push((void*) 0);";
                    pos = fp.tellp();
                    for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                        fp << " ";
                    }
                    fp << "/* " << body[i].getValue() << " */" << std::endl;
                } else {
                    if (body[i].getType() == tok_if) {
                        scopeDepth++;
                        lstart = fp.tellp();
                        fp << "if (scale_pop_long()) {";
                        pos = fp.tellp();
                        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                            fp << " ";
                        }
                        fp << "/* " << body[i].getValue() << " */";
                        fp << std::endl;
                    } else if (body[i].getType() == tok_else) {
                        scopeDepth--;
                        scopeDepth++;
                        lstart = fp.tellp();
                        fp << "} else {";
                        pos = fp.tellp();
                        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                            fp << " ";
                        }
                        fp << "/* " << body[i].getValue() << " */";
                        fp << std::endl;
                    } else if (body[i].getType() == tok_while) {
                        scopeDepth++;
                        lstart = fp.tellp();
                        fp << "while (1) {";
                        pos = fp.tellp();
                        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                            fp << " ";
                        }
                        fp << "/* " << body[i].getValue() << " */";
                        fp << std::endl;
                    } else if (body[i].getType() == tok_do) {
                        lstart = fp.tellp();
                        fp << "if (!scale_pop_long()) break;";
                        pos = fp.tellp();
                        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                            fp << " ";
                        }
                        fp << "/* " << body[i].getValue() << " */";
                        fp << std::endl;
                    } else if (body[i].getType() == tok_done || body[i].getType() == tok_fi || body[i].getType() == tok_end) {
                        scopeDepth--;
                        lstart = fp.tellp();
                        fp << "}";
                        pos = fp.tellp();
                        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                            fp << " ";
                        }
                        fp << "/* " << body[i].getValue() << " */";
                        fp << std::endl;
                    } else if (body[i].getType() == tok_return) {
                        lstart = fp.tellp();
                        fp << "return;";
                        pos = fp.tellp();
                        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                            fp << " ";
                        }
                        fp << "/* " << body[i].getValue() << " */";
                        fp << std::endl;
                    } else if (body[i].getType() == tok_number_float) {
                        double num;
                        try
                        {
                            num = std::stold(body[i].getValue());
                            lstart = fp.tellp();
                            fp << "scale_push_double(" << num << ");";
                            pos = fp.tellp();
                            for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                                fp << " ";
                            }
                            fp << "/* " << body[i].getValue() << " */" << std::endl;
                        }
                        catch(const std::exception& e)
                        {
                            std::cerr << "Number out of range: " << body[i].getValue() << std::endl;
                            ParseResult result;
                            result.success = false;
                            return result;
                        }
                    } else if (body[i].getType() == tok_addr_ref) {
                        std::string toGet = body[i + 1].getValue();
                        lstart = fp.tellp();
                        if (hasExtern(toGet)) {
                            fp << "scale_push((void*) &scale_extern_" << toGet << ");";
                        } else if (hasFunction(toGet)) {
                            fp << "scale_push((void*) &scale_func_" << toGet << ");";
                        } else {
                            fp << "scale_push((void*) &" << toGet << ");";
                        }
                        pos = fp.tellp();
                        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                            fp << " ";
                        }
                        fp << "/* " << body[i].getValue() << body[i + 1].getValue() << " */" << std::endl;
                        i++;
                    } else {
                        std::cerr << "Unknown token: " << body[i].getValue() << std::endl;
                        ParseResult parseResult;
                        parseResult.success = false;
                        return parseResult;
                    }
                }
            }
        }
        lstart = fp.tellp();
        if (!isInline) fp << "stacktrace_pop();";
        pos = fp.tellp();
        for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
            fp << " ";
        }
        fp << "/* end */" << std::endl;
        fp << "}";
        fp << std::endl;
    }

    fp << "#ifdef __cplusplus" << std::endl;
    fp << "}" << std::endl;
    fp << "#endif" << std::endl;
    fp << std::endl;

    header << "#endif" << std::endl;
    header << std::endl;
    header << "#ifdef __cplusplus" << std::endl;
    header << "}" << std::endl;
    header << "#endif" << std::endl;
    
    fp.close();
    header.close();

    ParseResult parseResult;
    parseResult.success = true;
    return parseResult;
}

#endif // _PARSER_CPP_