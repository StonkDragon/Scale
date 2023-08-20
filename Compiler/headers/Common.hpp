#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <sys/stat.h>
#include <chrono>

#define TOKEN(x, y, line, file) if (value == x) return Token(y, value, line, file, begin)

// optimize this :
#define append(...) do { for (int j = 0; j < scopeDepth; j++) { fprintf(fp, "  "); } fprintf(fp, __VA_ARGS__); } while (0)
#define append2(...) do { fprintf(fp, __VA_ARGS__); } while (0)

#undef INT_MAX
#undef INT_MIN
#undef LONG_MAX
#undef LONG_MIN

#define INT_MAX 0x7FFFFFFF
#define INT_MIN 0x80000000
#define LONG_MAX 0x7FFFFFFFFFFFFFFFll
#define LONG_MIN 0x8000000000000000ll

#define LINE_LENGTH 48

#define ssize_t signed long long

namespace sclc {
    typedef std::unordered_map<std::string, std::string> Deprecation;
}

#include "Color.hpp"
#include "Version.hpp"
#include "TokenType.hpp"
#include "Token.hpp"
#include "Function.hpp"
#include "Results.hpp"
#include "Variable.hpp"
#include "Container.hpp"
#include "Interface.hpp"
#include "Struct.hpp"
#include "DragonConfig.hpp"
#include "Enum.hpp"

namespace sclc {
    typedef unsigned int ID_t;

    class SyntaxTree
    {
    private:
        std::vector<Token> tokens;
    public:
        SyntaxTree(std::vector<Token>& tokens) {
            this->tokens = tokens;
        }
        ~SyntaxTree() {}
        TPResult parse(std::vector<std::string>& binaryHeaders);
    };

    class Parser
    {
        TPResult& result;

    public:
        Parser(TPResult& result) : result(result) {}
        ~Parser() {}
        FPResult parse(std::string filename);
        TPResult& getResult();
    };

    class InfoDumper {
    public:
        static void dump(TPResult& result);
    };

    struct Tokenizer
    {
        std::vector<Token> tokens;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        char* source;
        size_t current;
        bool additional;
        Token additionalToken;

        int line = 1;
        int column = 1;
        int begin = 1;
        std::string filename;
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
        static void writeFunctionHeaders(FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns);
        static void writeGlobals(FILE* fp, std::vector<Variable>& globals, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns);
        static void writeContainers(FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns);
        static void writeStructs(FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns);
        static void writeTables(FILE* fp, TPResult& result, std::string filename);
        static void writeFunctions(FILE* fp, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::vector<Variable>& globals, TPResult& result, std::string filename);
    };

    struct _Main {
        Tokenizer* tokenizer;
        SyntaxTree* lexer;
        Parser* parser;
        std::vector<std::string> frameworkNativeHeaders;
        std::vector<std::string> frameworks;
        Version* version;
        long long tokenHandleTime;
        long long writeHeaderTime;
        long long writeContainersTime;
        long long writeStructsTime;
        long long writeGlobalsTime;
        long long writeFunctionHeadersTime;
        long long writeTablesTime;
        long long writeFunctionsTime;
        struct options {
            bool doRun;
            bool noMain;
            bool Werror;
            bool minify;
            bool dumpInfo;
            bool printDocs;
            bool debugBuild;
            bool printCflags;
            size_t stackSize;
            bool binaryHeader;
            bool assembleOnly;
            bool transpileOnly;
            std::string outfile;
            bool preprocessOnly;
            bool noScaleFramework;
            std::string optimizer;
            bool dontSpecifyOutFile;
            std::string printDocFor;
            size_t docPrinterArgsStart;
            std::string docsIncludeFolder;
            std::string operatorRandomData;
            std::vector<std::string> files;
            std::vector<std::string> features;
            std::vector<std::string> includePaths;
            std::vector<std::string> filesFromCommandLine;
            std::unordered_map<std::string, std::string> mapFrameworkDocfiles;
            std::unordered_map<std::string, std::string> mapFrameworkIncludeFolders;
            std::unordered_map<std::string, std::string> mapIncludePathsToFrameworks;
            std::unordered_map<std::string, DragonConfig::CompoundEntry*> mapFrameworkConfigs;
        } options;
    };

    extern _Main Main;
    extern std::string scaleFolder;
    extern std::vector<Variable> vars;
    extern std::vector<size_t> var_indices;

    long long parseNumber(std::string str);
    Result<double, FPResult> parseDouble(const Token& tok);
    void signalHandler(int signum);
    bool strends(const std::string& str, const std::string& suffix);
    bool strstarts(const std::string& str, const std::string& prefix);
    bool strcontains(const std::string& str, const std::string& substr);
    int isCharacter(char c);
    int isDigit(char c);
    int isSpace(char c);
    int isPrint(char c);
    int isBracket(char c);
    int isHexDigit(char c);
    int isOctDigit(char c);
    int isBinDigit(char c);
    int isOperator(char c);
    bool fileExists(const std::string& name);
    void addIfAbsent(std::vector<Function*>& vec, Function* str);
    std::string replaceAll(const std::string& src, const std::string& from, const std::string& to);
    std::string replaceFirstAfter(const std::string& src, const std::string& from, const std::string& to, int index);
    int lastIndexOf(char* src, char c);
    bool hasVar(const std::string& name);
    Variable getVar(const std::string& name);
    Function* getFunctionByName(TPResult& result, const std::string& name);
    Interface* getInterfaceByName(TPResult& result, const std::string& name);
    Method* getMethodByName(TPResult& result, const std::string& name, const std::string& type);
    Method* getMethodByNameOnThisType(TPResult& result, const std::string& name, const std::string& type);
    Method* getMethodByNameWithArgs(TPResult& result, const std::string& name, const std::string& type, bool doCheck = true);
    Method* getMethodWithActualName(TPResult& result, const std::string& name, const std::string& type, bool doCheck = true);
    Function* getFunctionByNameWithArgs(TPResult& result, const std::string& name, bool doCheck = true);
    Container getContainerByName(TPResult& result, const std::string& name);
    const Struct& getStructByName(TPResult& result, const std::string& name);
    Layout getLayout(TPResult& result, const std::string& name);
    bool hasLayout(TPResult& result, const std::string& name);
    bool hasFunction(TPResult& result, const std::string& name);
    bool hasEnum(TPResult& result, const std::string& name);
    Enum getEnumByName(TPResult& result, const std::string& name);
    std::vector<std::string> supersToVector(TPResult& r, Struct& s);
    std::string supersToHashedCList(TPResult& r, Struct& s);
    std::string supersToCList(TPResult& r, Struct& s);
    std::vector<Method*> methodsOnType(TPResult& res, std::string type);
    bool hasMethod(TPResult& result, const std::string& name, const std::string& type);
    bool hasContainer(TPResult& result, std::string name);
    bool hasGlobal(TPResult& result, std::string name);
    FPResult parseType(std::vector<Token>& tokens, size_t* i, const std::map<std::string, std::string>& typeReplacements = std::map<std::string, std::string>());
    bool sclIsProhibitedInit(std::string s);
    bool typeCanBeNil(std::string s);
    bool typeIsConst(std::string s);
    bool typeIsMut(std::string s);
    bool typeIsReadonly(std::string s);
    bool isPrimitiveType(std::string s);
    bool featureEnabled(std::string feat);
    bool isInitFunction(Function* f);
    bool isDestroyFunction(Function* f);
    time_t file_modified_time(const std::string& path);
    
    template<typename T>
    bool contains(std::vector<T> v, T val) {
        if (v.size() == 0) return false;
        if constexpr (std::is_pointer<T>::value) {
            for (T t : v) {
                if (*t == *val) return true;
            }
            return false;
        } else {
            return std::find(v.begin(), v.end(), val) != v.end();
        }
    }

    inline ID_t ror(const ID_t value, ID_t shift) {
        return (value >> shift) | (value << ((sizeof(ID_t) << 3) - shift));
    }

    ID_t id(const char* data);
}
#endif // COMMON_H
