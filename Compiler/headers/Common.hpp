#ifndef COMMON_H
#define COMMON_H

#include <gc/gc_allocator.h>

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <csignal>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <chrono>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <gc.h>
#include <gc_cpp.h>

#define TOKEN(x, y, line, file) if (value == x) return Token(y, value, line, file, begin)

// optimize this :
#define append(...) do { for (int j = 0; j < scopeDepth; j++) { fp << "  "; } fp << sclc::format(__VA_ARGS__); } while (0)
#define append2(...) fp << sclc::format(__VA_ARGS__)

#ifdef DEBUG
#define DBG(...) std::cout << sclc::format(__VA_ARGS__) << std::endl
#else
#define DBG(...)
#endif

#if __has_builtin(__builtin_expect)
#define UNLIKELY(X) __builtin_expect(!!(X), 0)
#define LIKELY(X) __builtin_expect(!!(X), 1)
#else
#define UNLIKELY(X) (!!(X))
#define LIKELY(X) (!!(X))
#endif

#undef INT_MAX
#undef INT_MIN
#undef LONG_MAX
#undef LONG_MIN

#define INT_MAX 0x7FFFFFFF
#define INT_MIN 0x80000000
#define LONG_MAX 0x7FFFFFFFFFFFFFFFll
#define LONG_MIN 0x8000000000000000ll

#define LINE_LENGTH 48

#ifdef _WIN32
#define DIR_SEP "\\"
#else
#define DIR_SEP "/"
#endif

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
#include "Interface.hpp"
#include "Struct.hpp"
#include "DragonConfig.hpp"
#include "Enum.hpp"

namespace sclc {
    typedef unsigned long ID_t;

    template<typename... Args>
    std::string format(const std::string& str, Args... args) {
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wformat-security"
        size_t len = std::snprintf(nullptr, 0, str.c_str(), args...) + 1;
        if (len == 0) throw std::runtime_error("Format error: " + str);
        char* data = new char[len];
        std::snprintf(data, len, str.c_str(), args...);
        return std::string(data, data + len - 1);
        #pragma clang diagnostic pop
    }

    class SyntaxTree
    {
    private:
        std::vector<Token> tokens;
    public:
        SyntaxTree(std::vector<Token>& tokens);
        ~SyntaxTree() {}
        TPResult parse();
    };

    class Parser
    {
        TPResult& result;

    public:
        Parser(TPResult& result);
        ~Parser() {}
        FPResult parse(std::string func_file, std::string rt_file, std::string header_file);
        TPResult& getResult();
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
        Tokenizer();
        ~Tokenizer();
        FPResult tokenize(std::string source);
        std::vector<Token> getTokens();
        Token nextToken();
        void printTokens();
        void removeInvalidTokens();
        FPResult tryImports();
    };
    
    class Transpiler {
    public:
        TPResult& result;
        std::vector<FPResult>& errors;
        std::vector<FPResult>& warns;
        std::ostream& fp;

        Transpiler(TPResult& result, std::vector<FPResult>& errors, std::vector<FPResult>& warns, std::ostream& fp);
        ~Transpiler();

        void writeHeader();
        void writeFunctionHeaders();
        void writeGlobals();
        void writeContainers();
        void writeStructs();
        void writeFunctions(const std::string& header_file);
        void filePreamble(const std::string& header_file);
        void filePostamble();
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
            static bool printCflags;
            static bool noLinkScale;
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
    extern const std::unordered_map<std::string, std::string> funcNameIdents;

    long long parseNumber(std::string str);
    Result<double, FPResult> parseDouble(const Token& tok);
    void signalHandler(int signum);
    bool strends(const std::string& str, const std::string& suffix);
    bool strstarts(const std::string& str, const std::string& prefix);
    bool strcontains(const std::string& str, const std::string& substr);
    bool pathstarts(std::filesystem::path str, std::string prefix);
    bool pathcontains(std::filesystem::path str, std::string substr);
    int isCharacter(char c);
    int isDigit(char c);
    int isSpace(char c);
    int isPrint(char c);
    int isBracket(char c);
    int isHexDigit(char c);
    int isOctDigit(char c);
    int isBinDigit(char c);
    int isOperator(char c);
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
    bool hasGlobal(TPResult& result, std::string name);
    FPResult parseType(std::vector<Token>& tokens, size_t* i, const std::map<std::string, Token>& typeReplacements = std::map<std::string, Token>());
    bool sclIsProhibitedInit(std::string s);
    bool typeCanBeNil(std::string s, bool doRemoveMods = true);
    bool typeIsConst(std::string s);
    bool typeIsReadonly(std::string s);
    bool isPrimitiveType(std::string s, bool rem = true);
    bool featureEnabled(std::string feat);
    bool isInitFunction(Function* f);
    bool isDestroyFunction(Function* f);
    std::string sclFunctionNameToFriendlyString(Function* f);
    std::string sclFunctionNameToFriendlyString(std::string name);
    bool isPrimitiveIntegerType(std::string type, bool rem = true);
    std::string argsToRTSignature(Function* f);
    bool typeEquals(const std::string& a, const std::string& b);
    std::vector<Method*> makeVTable(TPResult& res, std::string name);
    std::string argsToRTSignatureIdent(Function* f);
    void makePath(TPResult& result, Variable v, bool topLevelDeref, std::vector<Token>& body, size_t& i, std::vector<FPResult>& errors, bool doesWriteAfter, Function* function, std::vector<FPResult>& warns, std::ostream& fp, std::function<void(std::string, std::string)> onComplete);
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
    std::string retemplate(std::string type);
    bool isAllowed1ByteChar(char c);
    bool checkUTF8(const std::string& str);
    void checkShadow(std::string name, Token& body, Function* function, TPResult& result, std::vector<FPResult>& warns);

    template<typename T>
    static inline bool contains(std::vector<T> v, T val) {
        if (v.empty()) return false;
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
    static inline void addIfAbsent(std::vector<T>& vec, T val) {
        if (vec.empty() || !contains<T>(vec, val))
            vec.push_back(val);
    }

    template<typename T>
    static inline std::vector<T> joinVecs(std::vector<T> a, std::vector<T> b) {
        std::vector<T> ret;
        ret.reserve(a.size() + b.size());
        for (T& t : a) {
            ret.push_back(t);
        }
        for (T& t : b) {
            ret.push_back(t);
        }
        return ret;
    }

    template<typename T>
    static inline size_t indexInVec(std::vector<T>& vec, T elem) {
        for (size_t i = 0; i < vec.size(); i++) {
            if (vec[i] == elem) {
                return i;
            }
        }
        return -1;
    }

    static inline constexpr ID_t ror(const ID_t value, ID_t shift) {
        return (value >> shift) | (value << ((sizeof(ID_t) << 3) - shift));
    }

    struct StructTreeNode {
        Struct s;
        std::vector<StructTreeNode*> children;

        StructTreeNode(Struct s);
        ~StructTreeNode();
        void addChild(StructTreeNode* child);
        std::string toString(int indent = 0);
        void forEach(std::function<void(StructTreeNode*)> f);
        static StructTreeNode* directSubstructsOf(StructTreeNode* root, TPResult& result, std::string name);
        static StructTreeNode* fromArrayOfStructs(TPResult& result);
    };

    static constexpr ID_t id(const char data[]) {
        if (data[0] == 0) return 0;
        ID_t h = 3323198485UL;
        for (size_t i = 0; data[i]; i++) {
            h ^= data[i];
            h *= 0x5BD1E995;
            h ^= h >> 15;
        }
        return h;
    }
}

#endif // COMMON_H
