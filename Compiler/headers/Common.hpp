#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <csignal>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <sys/stat.h>
#include <chrono>
#include <dlfcn.h>

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

#define ssize_t signed long

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
    typedef unsigned long ID_t;

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
        void removeInvalidTokens();
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

    struct Main {
        static Tokenizer* tokenizer;
        static SyntaxTree* lexer;
        static Parser* parser;
        static std::vector<std::string> frameworkNativeHeaders;
        static std::vector<std::string> frameworks;
        static Version* version;
        static long long tokenHandleTime;
        static long long writeHeaderTime;
        static long long writeContainersTime;
        static long long writeStructsTime;
        static long long writeGlobalsTime;
        static long long writeFunctionHeadersTime;
        static long long writeTablesTime;
        static long long writeFunctionsTime;
        struct options {
            static bool noMain;
            static bool Werror;
            static bool dumpInfo;
            static bool printDocs;
            static bool debugBuild;
            static bool printCflags;
            static size_t stackSize;
            static size_t errorLimit;
            static bool binaryHeader;
            static bool assembleOnly;
            static bool transpileOnly;
            static std::string outfile;
            static bool preprocessOnly;
            static bool noScaleFramework;
            static std::string optimizer;
            static bool dontSpecifyOutFile;
            static std::string printDocFor;
            static size_t docPrinterArgsStart;
            static std::string docsIncludeFolder;
            static std::vector<std::string> files;
            static std::vector<std::string> features;
            static std::vector<std::string> includePaths;
            static std::vector<std::string> filesFromCommandLine;
            static std::unordered_map<std::string, std::string> mapFrameworkDocfiles;
            static std::unordered_map<std::string, std::string> mapFrameworkIncludeFolders;
            static std::unordered_map<std::string, std::string> mapIncludePathsToFrameworks;
            static std::unordered_map<std::string, DragonConfig::CompoundEntry*> indexDrgFiles;
        };
    };

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
    Struct& getStructByName(TPResult& result, const std::string& name);
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
    bool typeIsReadonly(std::string s);
    bool isPrimitiveType(std::string s);
    bool featureEnabled(std::string feat);
    bool isInitFunction(Function* f);
    bool isDestroyFunction(Function* f);
    std::string sclFunctionNameToFriendlyString(Function* f);
    std::string sclFunctionNameToFriendlyString(std::string name);
    bool isPrimitiveIntegerType(std::string type);
    std::string argsToRTSignature(Function* f);
    bool typeEquals(const std::string& a, const std::string& b);
    std::vector<Method*> makeVTable(TPResult& res, std::string name);
    std::string argsToRTSignatureIdent(Function* f);
    std::string makePath(TPResult& result, Variable v, bool topLevelDeref, std::vector<Token>& body, size_t& i, std::vector<FPResult>& errors, std::string prefix, std::string* currentType, bool doesWriteAfter = true);
    std::pair<std::string, std::string> findNth(std::map<std::string, std::string> val, size_t n);
    std::vector<std::string> vecWithout(std::vector<std::string> vec, std::string elem);
    std::string unquote(const std::string& str);
    std::string capitalize(std::string s);
    std::vector<std::string> split(const std::string& str, const std::string& delimiter);
    std::string ltrim(const std::string& s);
    std::string scaleArgs(std::vector<Variable> args);
    bool structImplements(TPResult& result, Struct s, std::string interface);
    std::string replace(std::string s, std::string a, std::string b);
    Method* attributeAccessor(TPResult& result, std::string struct_, std::string member);
    Method* attributeMutator(TPResult& result, std::string struct_, std::string member);

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

    template<typename T>
    std::vector<T> operator&(const std::vector<T>& a, std::function<bool(T)> b) {
        std::vector<T> result;
        for (auto&& t : a) {
            if (b(t)) {
                result.push_back(t);
            }
        }
        return result;
    }

    template<typename T>
    void addIfAbsent(std::vector<T>& vec, T val) {
        if (vec.size() == 0 || !contains<T>(vec, val))
            vec.push_back(val);
    }

    template<typename T>
    std::vector<T> joinVecs(std::vector<T> a, std::vector<T> b) {
        std::vector<T> ret;
        for (T& t : a) {
            ret.push_back(t);
        }
        for (T& t : b) {
            ret.push_back(t);
        }
        return ret;
    }

    template<typename T>
    size_t indexInVec(std::vector<T>& vec, T elem) {
        for (size_t i = 0; i < vec.size(); i++) {
            if (vec[i] == elem) {
                return i;
            }
        }
        return -1;
    }

    inline ID_t ror(const ID_t value, ID_t shift) {
        return (value >> shift) | (value << ((sizeof(ID_t) << 3) - shift));
    }

    struct StructTreeNode {
        Struct s;
        std::vector<StructTreeNode*> children;

        StructTreeNode(Struct s) : s(s), children() {}

        ~StructTreeNode() {
            for (StructTreeNode* child : children) {
                delete child;
            }
        }

        void addChild(StructTreeNode* child) {
            children.push_back(child);
        }

        std::string toString(int indent = 0) {
            std::string result = "";
            for (int i = 0; i < indent; i++) {
                result += "  ";
            }
            result += s.name + "\n";
            for (StructTreeNode* child : children) {
                result += child->toString(indent + 1);
            }
            return result;
        }

        void forEach(std::function<void(StructTreeNode*)> f) {
            f(this);
            for (StructTreeNode* child : children) {
                child->forEach(f);
            }
        }

        static StructTreeNode* directSubstructsOf(StructTreeNode* root, TPResult& result, std::string name) {
            StructTreeNode* node = new StructTreeNode(getStructByName(result, name));
            for (Struct& s : result.structs) {
                if (s.super == name) {
                    node->addChild(directSubstructsOf(root, result, s.name));
                }
            }
            return node;
        }

        static StructTreeNode* fromArrayOfStructs(TPResult& result) {
            StructTreeNode* root = new StructTreeNode(getStructByName(result, "SclObject"));
            return directSubstructsOf(root, result, "SclObject");
        }
    };

    ID_t id(const char* data);
}
#endif // COMMON_H
