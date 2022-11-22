#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>

#define TOKEN(x, y, line, file, column) if (value == x) return Token(y, value, line, file, column)
#define append(...) do { for (int j = 0; j < scopeDepth; j++) { fprintf(fp, "  "); } fprintf(fp, __VA_ARGS__); } while (0)

#undef INT_MAX
#undef INT_MIN
#undef LONG_MAX
#undef LONG_MIN

#define INT_MAX 0x7FFFFFFF
#define INT_MIN 0x80000000
#define LONG_MAX 0x7FFFFFFFFFFFFFFFll
#define LONG_MIN 0x8000000000000000ll

#define LINE_LENGTH 48

#ifndef ASM_FN_FMT
#ifdef __APPLE__
#define ASM_FN_FMT "[%s]"
#else
#define ASM_FN_FMT "\\\"[%s]\\\""
#endif
#endif

namespace sclc
{
    typedef unsigned long long hash;

    class Color {
    public:
        static const std::string RESET;
        static const std::string BLACK;
        static const std::string RED;
        static const std::string GREEN;
        static const std::string YELLOW;
        static const std::string BLUE;
        static const std::string MAGENTA;
        static const std::string CYAN;
        static const std::string WHITE;
        static const std::string BOLDBLACK;
        static const std::string BOLDRED;
        static const std::string BOLDGREEN;
        static const std::string BOLDYELLOW;
        static const std::string BOLDBLUE;
        static const std::string BOLDMAGENTA;
        static const std::string BOLDCYAN;
        static const std::string BOLDWHITE;
    };

    class Version {
        int major;
        int minor;
        int patch;

    public:
        Version(std::string str) {
            std::string::difference_type n = std::count(str.begin(), str.end(), '.');
            if (n == 1) {
                sscanf(str.c_str(), "%d.%d", &major, &minor);
                patch = 0;
            } else if (n == 2) {
                sscanf(str.c_str(), "%d.%d.%d", &major, &minor, &patch);
            } else if (n == 0) {
                sscanf(str.c_str(), "%d", &major);
                minor = 0;
                patch = 0;
            } else {
                std::cerr << "Unknown version number: " << str << std::endl;
                exit(1);
            }
        }
        ~Version() {}

        inline bool operator==(Version& v) const {
            return (major == v.major) && (minor == v.minor) && (patch == v.patch);
        }

        inline bool operator>(Version& v) const {
            if (major > v.major) {
                return true;
            } else if (major == v.major) {
                if (minor > v.minor) {
                    return true;
                } else if (minor == v.minor) {
                    if (patch > v.patch) {
                        return true;
                    }
                    return false;
                }
                return false;
            }
            return false;
        }

        inline bool operator>=(Version& v) const {
            return ((*this) > v) || ((*this) == v);
        }

        inline bool operator<=(Version& v) const {
            return !((*this) > v);
        }

        inline bool operator<(Version& v) const {
            return !((*this) >= v);
        }

        inline bool operator!=(Version& v) const {
            return !((*this) == v);
        }

        std::string asString() {
            return std::to_string(this->major) + "." + std::to_string(this->minor) + "." + std::to_string(this->patch);
        }
    };

    enum TokenType {
        tok_eof,

        // commands
        tok_return,         // return
        tok_function,       // function
        tok_end,            // end
        tok_true,           // true
        tok_false,          // false
        tok_nil,            // nil
        tok_if,             // if
        tok_else,           // else
        tok_fi,             // fi
        tok_while,          // while
        tok_do,             // do
        tok_done,           // done
        tok_extern,         // extern
        tok_break,          // break
        tok_continue,       // continue
        tok_for,            // for
        tok_in,             // in
        tok_to,             // to
        tok_addr_ref,       // addr
        tok_load,           // load
        tok_store,          // store
        tok_declare,        // decl
        tok_container_def,  // container
        tok_repeat,         // repeat
        tok_struct_def,     // struct
        tok_new,            // new
        tok_is,             // is
        tok_cdecl,          // cdecl
        tok_label,          // label
        tok_goto,           // goto

        // operators
        tok_hash,           // @
        tok_addr_of,        // @
        tok_open_paren,     // (
        tok_close_paren,    // )
        tok_curly_open,     // {
        tok_curly_close,    // }
        tok_comma,          // ,
        tok_add,            // +
        tok_sub,            // -
        tok_mul,            // *
        tok_div,            // /
        tok_mod,            // %
        tok_land,           // &
        tok_lor,            // |
        tok_lxor,           // ^
        tok_lnot,           // ~
        tok_lsh,            // <<
        tok_rsh,            // >>
        tok_pow,            // **
        tok_dadd,           // .+
        tok_dsub,           // .-
        tok_dmul,           // .*
        tok_ddiv,           // ./
        tok_column,         // :
        tok_dot,            // .

        tok_identifier,     // foo
        tok_number,         // 123
        tok_number_float,   // 123.456
        tok_string_literal, // "foo"
        tok_char_literal,   // 'a'
        tok_illegal,
        tok_ignore,
        tok_newline,
    };

    class Token
    {
        TokenType type;
        int line;
        int column;
        std::string file;
        std::string value;
    public:
        std::string tostring() {
            return "Token(value=" + value + ", type=" + std::to_string(type) + ")";
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
            return column;
        }
    };

    class FPResult {
    public:
        bool success;
        std::string message;
        std::string in;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::string value;
        int column;
        int line;
        TokenType type;
    };

    enum Modifier
    {
        mod_nps,
        mod_nowarn,
        mod_sap,
        mod_nomangle
    };

    class Variable {
        std::string name;
        std::string type;
    public:
        Variable(std::string name, std::string type) {
            this->name = name;
            this->type = type;
        }
        ~Variable() {}
        std::string getName() {
            return name;
        }
        std::string getType() {
            return type;
        }
        void setName(std::string name) {
            this->name = name;
        }
        void setType(std::string type) {
            this->type = type;
        }
    };

    class Function
    {
        std::string name;
        std::string file;
        std::string return_type;
        std::vector<Token> body;
        std::vector<Modifier> modifiers;
        std::vector<Variable> args;
    public:
        bool isMethod;
        Function(std::string name) {
            this->name = name;
            this->isMethod = false;
        }
        Function(std::string name, bool isMethod) {
            this->name = name;
            this->isMethod = isMethod;
        }
        virtual ~Function() {}
        virtual std::string getName() {
            return name;
        }
        virtual std::vector<Token> getBody() {
            return body;
        }
        virtual void addToken(Token token) {
            body.push_back(token);
        }
        virtual void addModifier(Modifier modifier) {
            modifiers.push_back(modifier);
        }
        virtual std::vector<Modifier> getModifiers() {
            return modifiers;
        }
        virtual void addArgument(Variable arg) {
            args.push_back(arg);
        }
        virtual std::vector<Variable> getArgs() {
            return args;
        }
        virtual std::string getFile() {
            return file;
        }
        virtual void setFile(std::string file) {
            this->file = file;
        }
        virtual void setName(std::string name) {
            this->name = name;
        }
        virtual std::string getReturnType() {
            return return_type;
        }
        virtual void setReturnType(std::string type) {
            return_type = type;
        }

        virtual bool operator==(const Function& other) const {
            return name == other.name;
        }
    };

    class Method : public Function {
        std::string member_type;
    public:
        Method(std::string member_type, std::string name) : Function(name, true) {
            this->member_type = member_type;
            this->isMethod = true;
        }
        std::string getMemberType() {
            return member_type;
        }
        void setMemberType(std::string member_type) {
            this->member_type = member_type;
        }
    };

    class Container {
        std::string name;
        std::vector<Variable> members;
    public:
        Container(std::string name) {
            this->name = name;
        }
        void addMember(Variable member) {
            members.push_back(member);
        }
        bool hasMember(std::string member) {
            for (Variable m : members) {
                if (m.getName() == member) {
                    return true;
                }
            }
            return false;
        }
        std::string getName() { return name; }
        std::vector<Variable> getMembers() { return members; }
        std::string getMemberType(std::string member) {
            for (Variable m : members) {
                if (m.getName() == member) {
                    return m.getType();
                }
            }
            return "";
        }
    };

    class Struct {
        std::string name;
        std::vector<Variable> members;
    public:
        Struct(std::string name) {
            this->name = name;
        }
        void addMember(Variable member) {
            members.push_back(member);
        }
        bool hasMember(std::string member) {
            for (Variable m : members) {
                if (m.getName() == member) {
                    return true;
                }
            }
            return false;
        }
        int indexOfMember(std::string member) {
            for (size_t i = 0; i < members.size(); i++) {
                if (members[i].getName() == member) {
                    return ((int) i) * 8;
                }
            }
            return -1;
        }
        inline bool operator==(const Struct& other) const {
            return other.name == this->name;
        }
        inline bool operator!=(const Struct& other) const {
            return other.name != this->name;
        }
        std::string getName() { return name; }
        std::vector<Variable> getMembers() { return members; }
        void setName(const std::string& name) { this->name = name; }
    };

    class Prototype
    {
        std::string name;
        std::string return_type;
        int argCount;
    public:
        Prototype(std::string name, int argCount) {
            this->name = name;
            this->argCount = argCount;
        }
        ~Prototype() {}
        std::string getName() { return name; }
        int getArgCount() { return argCount; }
        std::string getReturnType() { return return_type; }
        void setReturnType(std::string type) { return_type = type; }
        void setArgCount(int argCount) { this->argCount = argCount; }
        void setName(std::string name) { this->name = name; }
    };

    class TPResult {
    public:
        std::vector<Function*> functions;
        std::vector<Function*> extern_functions;
        std::vector<Variable> extern_globals;
        std::vector<Variable> globals;
        std::vector<Container> containers;
        std::vector<FPResult> errors;
        std::vector<Struct> structs;
    };

    class TokenParser
    {
    private:
        std::vector<Token> tokens;
        std::vector<Function*> functions;
        std::vector<Function*> extern_functions;
        std::vector<Variable> extern_globals;
    public:
        TokenParser(std::vector<Token> tokens) {
            this->tokens = tokens;
        }
        ~TokenParser() {}
        TPResult parse();
        static bool isOperator(Token token);
        static bool isType(Token token);
        static bool canAssign(Token token);
    };

    class FunctionParser
    {
        TPResult result;

    public:
        FunctionParser(TPResult result) {
            this->result = result;
        }
        ~FunctionParser() {}
        FPResult parse(std::string filename);
        TPResult getResult();
    };

    class Tokenizer
    {
        std::vector<Token> tokens;
        std::vector<FPResult> errors;
        char* source;
        size_t current;
    public:
        Tokenizer() {current = 0;}
        ~Tokenizer() {}
        FPResult tokenize(std::string source);
        std::vector<Token> getTokens();
        Token nextToken();
        void printTokens();
    };
    
    class ConvertC {
    public:
        static void writeHeader(FILE* fp);
        static void writeFunctionHeaders(FILE* fp, TPResult result);
        static void writeExternHeaders(FILE* fp, TPResult result);
        static void writeInternalFunctions(FILE* fp, TPResult result);
        static void writeGlobals(FILE* fp, std::vector<Variable>& globals, TPResult result);
        static void writeContainers(FILE* fp, TPResult result);
        static void writeStructs(FILE* fp, TPResult result);
        static void writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<Variable>& globals, TPResult result);        
    };

    struct _Main {
        Tokenizer* tokenizer;
        TokenParser* lexer;
        FunctionParser* parser;
        std::vector<std::string> frameworkNativeHeaders;
        std::vector<std::string> frameworks;
        struct options {
            bool noMain;
            bool transpileOnly;
            bool debugBuild;
            bool assembleOnly;
            bool noCoreFramework;
            bool doRun;
            bool printCflags;
            bool dontSpecifyOutFile;
            bool preprocessOnly;
            bool Werror;
            std::string optimizer;
        } options;
    };

    extern _Main Main;
    extern std::string scaleFolder;
    extern std::vector<Variable> vars;

    long long parseNumber(std::string str);
    double parseDouble(std::string str);
    void signalHandler(int signum);
    bool strends(const std::string& str, const std::string& suffix);
    int isCharacter(char c);
    int isDigit(char c);
    int isSpace(char c);
    int isPrint(char c);
    int isBracket(char c);
    int isHexDigit(char c);
    int isOctDigit(char c);
    int isBinDigit(char c);
    int isOperator(char c);
    bool isOperator(Token token);
    bool fileExists(const std::string& name);
    void addIfAbsent(std::vector<Function*>& vec, Function* str);
    std::string replaceAll(std::string src, std::string from, std::string to);
    std::string replaceFirstAfter(std::string src, std::string from, std::string to, int index);
    int lastIndexOf(char* src, char c);
    bool hasVar(Token name);
    Variable getVar(Token name);
    hash hash1(char* data);
    FPResult handleOperator(FILE* fp, Token token, int scopeDepth);
    FPResult handleNumber(FILE* fp, Token token, int scopeDepth);
    FPResult handleFor(Token keywDeclare, Token loopVar, Token keywIn, Token from, Token keywTo, Token to, Token keywDo, std::vector<Variable>* vars, FILE* fp, int* scopeDepth);
    FPResult handleDouble(FILE* fp, Token token, int scopeDepth);
    Function* getFunctionByName(TPResult result, std::string name);
    Method* getMethodByName(TPResult result, std::string name, std::string type);
    Container getContainerByName(TPResult result, std::string name);
    Struct getStructByName(TPResult result, std::string name);
    bool hasFunction(TPResult result, Token name);
    bool hasMethod(TPResult result, Token name, std::string type);
    bool hasExtern(TPResult result, Token name);
    bool hasContainer(TPResult result, Token name);
}
#endif // COMMON_H
