#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#define TOKEN(x, y, line, file) if (value == x) return Token(y, value, line, file)
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
        tok_then,           // then
        tok_elif,           // elif
        tok_else,           // else
        tok_fi,             // fi
        tok_while,          // while
        tok_do,             // do
        tok_done,           // done
        tok_extern,         // extern
        tok_break,          // break
        tok_continue,       // continue
        tok_for,            // for
        tok_foreach,        // foreach
        tok_in,             // in
        tok_step,           // step
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
        tok_switch,         // switch
        tok_case,           // case
        tok_esac,           // esac
        tok_default,        // default
        tok_interface_def,  // interface
        tok_as,             // as

        // operators
        tok_question_mark,  // ?
        tok_hash,           // @
        tok_addr_of,        // @
        tok_paren_open,     // (
        tok_paren_close,    // )
        tok_curly_open,     // {
        tok_curly_close,    // }
        tok_bracket_open,   // [
        tok_bracket_close,  // ]
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
        tok_extern_c,
        tok_illegal,
        tok_ignore,
        tok_newline,
    };

    class Token
    {
        TokenType type;
        int line;
        std::string file;
        std::string value;
    public:
        std::string tostring() {
            return "Token(value=" + value + ", type=" + std::to_string(type) + ")";
        }
        Token() : Token(tok_ignore, "", 0, "") {}
        Token(TokenType type, std::string value, int line, std::string file) : type(type), value(value) {
            this->line = line;
            this->file = file;
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
        inline bool operator==(const Variable& other) const {
            if (this->type == "?" || other.type == "?") {
                return this->name == other.name;
            }
            return this->name == other.name && this->type == other.type;
        }
        inline bool operator!=(const Variable& other) const {
            return !((*this) == other);
        }
    };

    class Function
    {
        std::string name;
        std::string file;
        std::string return_type;
        std::vector<Token> body;
        std::vector<std::string> modifiers;
        std::vector<Variable> args;
        Token nameToken;
    public:
        bool isMethod;
        Function(std::string name, Token nameToken);
        Function(std::string name, bool isMethod, Token nameToken);
        virtual ~Function() {}
        virtual std::string getName();
        virtual std::vector<Token> getBody();
        virtual void addToken(Token token);
        virtual void addModifier(std::string modifier);
        virtual std::vector<std::string> getModifiers();
        virtual void addArgument(Variable arg);
        virtual std::vector<Variable> getArgs();
        virtual std::string getFile();
        virtual void setFile(std::string file);
        virtual void setName(std::string name);
        virtual std::string getReturnType();
        virtual void setReturnType(std::string type);
        virtual Token getNameToken();
        virtual void setNameToken(Token t);

        virtual bool operator==(const Function& other) const;
    };
    
    class Method : public Function {
        std::string member_type;
        bool force_add;
    public:
        Method(std::string member_type, std::string name, Token& nameToken) : Function(name, true, nameToken) {
            this->member_type = member_type;
            this->isMethod = true;
            this->force_add = false;
        }
        std::string getMemberType() {
            return member_type;
        }
        void setMemberType(std::string member_type) {
            this->member_type = member_type;
        }
        bool addAnyway() {
            return force_add;
        }
        void forceAdd(bool force) {
            force_add = force;
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

    class Interface {
        std::string name;
        std::vector<Function*> to_implement;
    public:
        Interface(std::string name) {
            this->name = name;
        }
        bool hasToImplement(std::string func) {
            for (Function* f : to_implement) {
                if (f->getName() == func) {
                    return true;
                }
            }
            return false;
        }
        Function* getToImplement(std::string func) {
            for (Function* f : to_implement) {
                if (f->getName() == func) {
                    return f;
                }
            }
            return nullptr;
        }
        std::vector<Function*> getImplements() {
            return to_implement;
        }
        void addToImplement(Function* func) {
            to_implement.push_back(func);
        }
        std::string getName() {
            return name;
        }
        void setName(std::string name) {
            this->name = name;
        }

        inline bool operator==(const Interface& other) const {
            return this->name == other.name;
        }
        inline bool operator!=(const Interface& other) const {
            return !((*this) == other);
        }
    };

    class Struct {
        std::string name;
        Token name_token;
        int flags;
        std::vector<Variable> members;
        std::vector<std::string> interfaces;
    public:
        Struct(std::string name) : Struct(name, Token(tok_ignore, "", 0, "")) {
            
        }
        Struct(std::string name, Token t) {
            this->name = name;
            this->name_token = t;
            this->flags = 0;
            toggleWarnings();
            toggleValueType();
            toggleReferenceType();
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
        bool implements(std::string name) {
            return std::find(interfaces.begin(), interfaces.end(), name) != interfaces.end();
        }
        void implement(std::string interface) {
            interfaces.push_back(interface);
        }
        std::vector<std::string> getInterfaces() {
            return interfaces;
        }
        Token nameToken() {
            return name_token;
        }
        void setNameToken(Token t) {
            this->name_token = t;
        }
        bool heapAllocAllowed() {
            return (flags & 0b00000001) != 0;
        }
        bool stackAllocAllowed() {
            return (flags & 0b00000010) != 0;
        }
        bool isSealed() {
            return (flags & 0b00000100) != 0;
        }
        bool warnsEnabled() {
            return (flags & 0b00001000) != 0;
        }
        bool isStatic() {
            return (flags & 0b00010000) != 0;
        }
        void toggleReferenceType() {
            flags ^= 0b00000001;
        }
        void toggleValueType() {
            flags ^= 0b00000010;
        }
        void toggleSealed() {
            flags ^= 0b00000100;
        }
        void toggleWarnings() {
            flags ^= 0b00001000;
        }
        void toggleStatic() {
            flags ^= 0b00010000;
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
        std::vector<Interface*> interfaces;
        std::vector<Variable> extern_globals;
        std::vector<Variable> globals;
        std::vector<Container> containers;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::vector<Struct> structs;
        std::unordered_map<std::string, std::string> typealiases;
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
        std::vector<FPResult> warns;
        char* source;
        size_t current;
    public:
        Tokenizer() {current = 0;}
        ~Tokenizer() {}
        FPResult tokenize(std::string source);
        std::vector<Token> getTokens();
        Token nextToken();
        void printTokens();
        FPResult tryImports();
    };
    
    class ConvertC {
    public:
        static void writeHeader(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns);
        static void writeFunctionHeaders(FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns);
        static void writeExternHeaders(FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns);
        static void writeGlobals(FILE* fp, std::vector<Variable>& globals, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns);
        static void writeContainers(FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns);
        static void writeStructs(FILE* fp, TPResult result, std::vector<FPResult>& errors, std::vector<FPResult>& warns);
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
            bool noScaleFramework;
            bool doRun;
            bool printCflags;
            bool dontSpecifyOutFile;
            bool preprocessOnly;
            bool Werror;
            std::string optimizer;
            std::vector<std::string> files;
            std::vector<std::string> includePaths;
            std::unordered_map<std::string, std::string> mapIncludePathsToFrameworks;
        } options;
    };

    extern _Main Main;
    extern std::string scaleFolder;
    extern std::vector<std::vector<Variable>> vars;
    extern size_t varDepth;

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
    FPResult handleDouble(FILE* fp, Token token, int scopeDepth);
    Function* getFunctionByName(TPResult result, std::string name);
    Interface* getInterfaceByName(TPResult result, std::string name);
    Method* getMethodByName(TPResult result, std::string name, std::string type);
    Container getContainerByName(TPResult result, std::string name);
    Struct getStructByName(TPResult result, std::string name);
    bool hasFunction(TPResult result, Token name);
    bool hasMethod(TPResult result, Token name, std::string type);
    bool hasExtern(TPResult result, Token name);
    bool hasContainer(TPResult result, Token name);
    bool hasGlobal(TPResult result, std::string name);
    FPResult parseType(std::vector<Token> tokens, size_t* i);
    
    template<typename T>
    bool contains(std::vector<T> v, T val) {
        return std::find(v.begin(), v.end(), val) != v.end();
    }
}
#endif // COMMON_H
