#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#define TOKEN(x, y, line, file) if (value == x) return Token(y, value, line, file, begin)
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

#define ssize_t signed long long

#include "Color.hpp"
#include "Version.hpp"
#include "TokenType.hpp"
#include "Token.hpp"
#include "Results.hpp"
#include "Function.hpp"
#include "Variable.hpp"
#include "Container.hpp"
#include "Interface.hpp"
#include "Struct.hpp"
#include "Prototype.hpp"
#include "DragonConfig.hpp"

#define namingConvention(_str, _named, _rgx, _default, _force)                                                                                                              \
    ((_force || std::string(_named.getValue()).size() > 12) && !std::regex_match(_named.getValue(), _default##_regex) && std::regex_match(_named.getValue(), _rgx##_regex)) \
    {                                                                                                                                                                       \
        FPResult r;                                                                                                                                                         \
        r.message = std::string(_str) + " in Scale use " + std::string(#_default) + ", but this is " + #_rgx;                                                               \
        r.in = _named.getFile();                                                                                                                                            \
        r.line = _named.getLine();                                                                                                                                          \
        r.column = _named.getColumn();                                                                                                                                      \
        r.type = _named.getType();                                                                                                                                          \
        r.value = _named.getValue();                                                                                                                                        \
        r.success = false;                                                                                                                                                  \
        warns.push_back(r);                                                                                                                                                 \
    }

namespace sclc {
    extern std::regex flatcase_regex;
    extern std::regex UPPERCASE_regex;
    extern std::regex camelCase_regex;
    extern std::regex PascalCase_regex;
    extern std::regex IPascalCase_regex;
    extern std::regex snake_case_regex;
    extern std::regex SCREAMING_SNAKE_CASE_regex;

    typedef unsigned long long hash;

    class SyntaxTree
    {
    private:
        std::vector<Token> tokens;
        std::vector<Function*> functions;
        std::vector<Function*> extern_functions;
        std::vector<Variable> extern_globals;
    public:
        SyntaxTree(std::vector<Token> tokens) {
            this->tokens = tokens;
        }
        ~SyntaxTree() {}
        TPResult parse();
        static bool isOperator(Token token);
        static bool isType(Token token);
        static bool canAssign(Token token);
    };

    class Parser
    {
        TPResult result;

    public:
        Parser(TPResult result) {
            this->result = result;
        }
        ~Parser() {}
        FPResult parse(std::string filename);
        TPResult getResult();
    };

    class InfoDumper {
    public:
        static void dump(TPResult result);
    };

    class Tokenizer
    {
        std::vector<Token> tokens;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        char* source;
        size_t current;

        int line = 1;
        int column = 1;
        int begin = 1;
        std::string filename;
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
        static void writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<Variable>& globals, TPResult result, std::string filename);
    };

    struct _Main {
        Tokenizer* tokenizer;
        SyntaxTree* lexer;
        Parser* parser;
        std::vector<std::string> frameworkNativeHeaders;
        std::vector<std::string> frameworks;
        struct options {
            bool doRun;
            bool noMain;
            bool Werror;
            bool dumpInfo;
            bool printDocs;
            bool debugBuild;
            bool printCflags;
            bool assembleOnly;
            bool transpileOnly;
            bool preprocessOnly;
            bool noScaleFramework;
            std::string optimizer;
            bool dontSpecifyOutFile;
            std::string printDocFor;
            size_t docPrinterArgsStart;
            std::string docsIncludeFolder;
            std::string operatorRandomData;
            std::vector<std::string> files;
            std::vector<std::string> includePaths;
            std::unordered_map<std::string, std::string> mapFrameworkDocfiles;
            std::unordered_map<std::string, std::string> mapFrameworkIncludeFolders;
            std::unordered_map<std::string, std::string> mapIncludePathsToFrameworks;
            std::unordered_map<std::string, DragonConfig::CompoundEntry*> mapFrameworkConfigs;
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
    bool strstarts(const std::string& str, const std::string& prefix);
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
    FPResult handleOperator(TPResult result, FILE* fp, Token token, int scopeDepth);
    FPResult handleNumber(FILE* fp, Token token, int scopeDepth);
    FPResult handleDouble(FILE* fp, Token token, int scopeDepth);
    Function* getFunctionByName(TPResult result, std::string name);
    Interface* getInterfaceByName(TPResult result, std::string name);
    Method* getMethodByName(TPResult result, std::string name, std::string type);
    Container getContainerByName(TPResult result, std::string name);
    Struct getStructByName(TPResult result, std::string name);
    bool hasFunction(TPResult result, Token name);
    std::vector<std::string> supersToVector(TPResult r, Struct s);
    std::string supersToHashedCList(TPResult r, Struct s);
    std::string supersToCList(TPResult r, Struct s);
    std::vector<Method*> methodsOnType(TPResult res, std::string type);
    bool hasMethod(TPResult result, Token name, std::string type);
    bool hasMethod(TPResult result, std::string name, std::string type);
    bool hasContainer(TPResult result, Token name);
    bool hasGlobal(TPResult result, std::string name);
    FPResult parseType(std::vector<Token> tokens, size_t* i);
    std::string sclConvertToStructType(std::string type);
    bool sclIsProhibitedInit(std::string s);
    bool typeCanBeNil(std::string s);
    bool isPrimitiveType(std::string s);
    
    template<typename T>
    bool contains(std::vector<T> v, T val) {
        return std::find(v.begin(), v.end(), val) != v.end();
    }
}
#endif // COMMON_H
