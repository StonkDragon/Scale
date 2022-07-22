#ifndef _PARSER_CPP_
#define _PARSER_CPP_

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

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

struct ParseResult {
    bool success;
};

struct Operation {
    std::string funcName;
    std::vector<std::string> args;
    std::string type;
    std::string varName;
    std::string op;
    std::string left, right;

    std::string getOp() { return op; }
    std::string getLeft() { return left; }
    std::string getType() { return type; }
    std::string getRight() { return right; }
    std::string getVarName() { return varName; }
    std::string getFuncName() { return funcName; }
    std::vector<std::string> getArgs() { return args; }
    void addArg(std::string arg) { args.push_back(arg); }

    Operation(std::string funcName, std::string varName, std::string op, std::string left, std::string right, std::string type) {
        this->funcName = funcName;
        this->varName = varName;
        this->op = op;
        this->left = left;
        this->right = right;
        this->type = type;
    }
};

struct ParsedFunction
{
    std::string name;
    std::vector<Variable> parameters;
    std::vector<Operation> operations;
};

class Parser
{
private:
    AnalyzeResult result;
public:
    Parser(AnalyzeResult result)
    {
        this->result = result;
    }
    ~Parser() {}
    bool hasFunction(std::string name) {
        for (Function func : result.functions) {
            if (func.name == name) {
                return true;
            }
        }
        return false;
    }
    bool hasExtern(std::string name) {
        for (Extern extern_ : result.externs) {
            if (extern_.name == name) {
                return true;
            }
        }
        return false;
    }
    ParseResult parse(std::string filename)
    {
        std::fstream fp((filename + std::string(".c")).c_str(), std::ios::out);

        fp << "#include \"" << getenv("HOME") << "/Scale/comp/scale.c" << "\"" << std::endl;

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
            fp << "int scale_native_" << function.getName() << "();" << std::endl;
        }

        for (Extern extern_ : result.externs) {
            fp << "int scale_extern_" << extern_.name << "();" << std::endl;
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
            fp << "int scale_native_" << function.getName() << "() {";
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
                if (hasFunction(body[i].getValue())) {
                    lstart = fp.tellp();
                    fp << "scale_native_" << body[i].getValue() << "();";
                    pos = fp.tellp();
                    for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                        fp << " ";
                    }
                    fp << "/* " << body[i].getValue() << " */" << std::endl;
                } else if (hasExtern(body[i].getValue())) {
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
                            fp << "if (scale_pop_int()) {";
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
                            fp << "if (!scale_pop_int()) break;";
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
                            fp << "return scale_pop_int();";
                            pos = fp.tellp();
                            for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                                fp << " ";
                            }
                            fp << "/* " << body[i].getValue() << " */";
                            fp << std::endl;
                        }
                    }
                }
            }
            if (!isInline) fp << "stacktrace_pop();" << std::endl;
            lstart = fp.tellp();
            fp << "return 0;";
            pos = fp.tellp();
            for (int i = (pos - lstart); i < LINE_LENGTH; i++) {
                fp << " ";
            }
            fp << "/* end */" << std::endl;
            fp << "}";
            fp << std::endl;
        }

        fp << "int main(int argc, char* argv[]) {" << std::endl;
        fp << "    for (int i = argc; i > 1; i--) {" << std::endl;
        fp << "        scale_push_string(argv[i]);" << std::endl;
        fp << "    }" << std::endl;
        fp << "    scale_native_main();" << std::endl;
        fp << "    return scale_pop_int();" << std::endl;
        fp << "}" << std::endl;
        fp << std::endl;

        ParseResult parseResult;
        parseResult.success = true;
        return parseResult;
    }
};

#endif // _PARSER_CPP_