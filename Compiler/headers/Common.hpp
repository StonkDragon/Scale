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

#define _SCL_PREPROC_NARG(...) \
         _SCL_PREPROC_NARG_(__VA_ARGS__,_SCL_PREPROC_RSEQ_N())
#define _SCL_PREPROC_NARG_(...) \
         _SCL_PREPROC_ARG_N(__VA_ARGS__)
#define _SCL_PREPROC_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define _SCL_PREPROC_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

#define _SCL_PREPROC_REPEAT_N(n, what) _SCL_PREPROC_REPEAT_N_(n, what)
#define _SCL_PREPROC_REPEAT_NI(n, what) _SCL_PREPROC_REPEAT_N_I(n, what)
#define _SCL_PREPROC_REPEAT_N_(n, what) _SCL_PREPROC_REPEAT_ ## n(what, n)
#define _SCL_PREPROC_REPEAT_N_I(n, what) _SCL_PREPROC_REPEAT_ ## n ## I(what, n)
#define LIST_INDEX(x, _index) x[_index - 1]
#define _SCL_PREPROC_REPEAT_0(what, _index)
#define _SCL_PREPROC_REPEAT_0I(what, _index)
#define _SCL_PREPROC_REPEAT_1(what, _index) what
#define _SCL_PREPROC_REPEAT_1I(what, _index) LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_2(what, _index) _SCL_PREPROC_REPEAT_1(what, 1), what
#define _SCL_PREPROC_REPEAT_2I(what, _index) _SCL_PREPROC_REPEAT_1I(what, 1), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_3(what, _index) _SCL_PREPROC_REPEAT_2(what, 2), what
#define _SCL_PREPROC_REPEAT_3I(what, _index) _SCL_PREPROC_REPEAT_2I(what, 2), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_4(what, _index) _SCL_PREPROC_REPEAT_3(what, 3), what
#define _SCL_PREPROC_REPEAT_4I(what, _index) _SCL_PREPROC_REPEAT_3I(what, 3), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_5(what, _index) _SCL_PREPROC_REPEAT_4(what, 4), what
#define _SCL_PREPROC_REPEAT_5I(what, _index) _SCL_PREPROC_REPEAT_4I(what, 4), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_6(what, _index) _SCL_PREPROC_REPEAT_5(what, 5), what
#define _SCL_PREPROC_REPEAT_6I(what, _index) _SCL_PREPROC_REPEAT_5I(what, 5), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_7(what, _index) _SCL_PREPROC_REPEAT_6(what, 6), what
#define _SCL_PREPROC_REPEAT_7I(what, _index) _SCL_PREPROC_REPEAT_6I(what, 6), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_8(what, _index) _SCL_PREPROC_REPEAT_7(what, 7), what
#define _SCL_PREPROC_REPEAT_8I(what, _index) _SCL_PREPROC_REPEAT_7I(what, 7), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_9(what, _index) _SCL_PREPROC_REPEAT_8(what, 8), what
#define _SCL_PREPROC_REPEAT_9I(what, _index) _SCL_PREPROC_REPEAT_8I(what, 8), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_10(what, _index) _SCL_PREPROC_REPEAT_9(what, 9), what
#define _SCL_PREPROC_REPEAT_10I(what, _index) _SCL_PREPROC_REPEAT_9I(what, 9), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_11(what, _index) _SCL_PREPROC_REPEAT_10(what, 10), what
#define _SCL_PREPROC_REPEAT_11I(what, _index) _SCL_PREPROC_REPEAT_10I(what, 10), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_12(what, _index) _SCL_PREPROC_REPEAT_11(what, 11), what
#define _SCL_PREPROC_REPEAT_12I(what, _index) _SCL_PREPROC_REPEAT_11I(what, 11), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_13(what, _index) _SCL_PREPROC_REPEAT_12(what, 12), what
#define _SCL_PREPROC_REPEAT_13I(what, _index) _SCL_PREPROC_REPEAT_12I(what, 12), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_14(what, _index) _SCL_PREPROC_REPEAT_13(what, 13), what
#define _SCL_PREPROC_REPEAT_14I(what, _index) _SCL_PREPROC_REPEAT_13I(what, 13), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_15(what, _index) _SCL_PREPROC_REPEAT_14(what, 14), what
#define _SCL_PREPROC_REPEAT_15I(what, _index) _SCL_PREPROC_REPEAT_14I(what, 14), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_16(what, _index) _SCL_PREPROC_REPEAT_15(what, 15), what
#define _SCL_PREPROC_REPEAT_16I(what, _index) _SCL_PREPROC_REPEAT_15I(what, 15), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_17(what, _index) _SCL_PREPROC_REPEAT_16(what, 16), what
#define _SCL_PREPROC_REPEAT_17I(what, _index) _SCL_PREPROC_REPEAT_16I(what, 16), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_18(what, _index) _SCL_PREPROC_REPEAT_17(what, 17), what
#define _SCL_PREPROC_REPEAT_18I(what, _index) _SCL_PREPROC_REPEAT_17I(what, 17), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_19(what, _index) _SCL_PREPROC_REPEAT_18(what, 18), what
#define _SCL_PREPROC_REPEAT_19I(what, _index) _SCL_PREPROC_REPEAT_18I(what, 18), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_20(what, _index) _SCL_PREPROC_REPEAT_19(what, 19), what
#define _SCL_PREPROC_REPEAT_20I(what, _index) _SCL_PREPROC_REPEAT_19I(what, 19), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_21(what, _index) _SCL_PREPROC_REPEAT_20(what, 20), what
#define _SCL_PREPROC_REPEAT_21I(what, _index) _SCL_PREPROC_REPEAT_20I(what, 20), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_22(what, _index) _SCL_PREPROC_REPEAT_21(what, 21), what
#define _SCL_PREPROC_REPEAT_22I(what, _index) _SCL_PREPROC_REPEAT_21I(what, 21), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_23(what, _index) _SCL_PREPROC_REPEAT_22(what, 22), what
#define _SCL_PREPROC_REPEAT_23I(what, _index) _SCL_PREPROC_REPEAT_22I(what, 22), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_24(what, _index) _SCL_PREPROC_REPEAT_23(what, 23), what
#define _SCL_PREPROC_REPEAT_24I(what, _index) _SCL_PREPROC_REPEAT_23I(what, 23), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_25(what, _index) _SCL_PREPROC_REPEAT_24(what, 24), what
#define _SCL_PREPROC_REPEAT_25I(what, _index) _SCL_PREPROC_REPEAT_24I(what, 24), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_26(what, _index) _SCL_PREPROC_REPEAT_25(what, 25), what
#define _SCL_PREPROC_REPEAT_26I(what, _index) _SCL_PREPROC_REPEAT_25I(what, 25), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_27(what, _index) _SCL_PREPROC_REPEAT_26(what, 26), what
#define _SCL_PREPROC_REPEAT_27I(what, _index) _SCL_PREPROC_REPEAT_26I(what, 26), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_28(what, _index) _SCL_PREPROC_REPEAT_27(what, 27), what
#define _SCL_PREPROC_REPEAT_28I(what, _index) _SCL_PREPROC_REPEAT_27I(what, 27), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_29(what, _index) _SCL_PREPROC_REPEAT_28(what, 28), what
#define _SCL_PREPROC_REPEAT_29I(what, _index) _SCL_PREPROC_REPEAT_28I(what, 28), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_30(what, _index) _SCL_PREPROC_REPEAT_29(what, 29), what
#define _SCL_PREPROC_REPEAT_30I(what, _index) _SCL_PREPROC_REPEAT_29I(what, 29), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_31(what, _index) _SCL_PREPROC_REPEAT_30(what, 30), what
#define _SCL_PREPROC_REPEAT_31I(what, _index) _SCL_PREPROC_REPEAT_30I(what, 30), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_32(what, _index) _SCL_PREPROC_REPEAT_31(what, 31), what
#define _SCL_PREPROC_REPEAT_32I(what, _index) _SCL_PREPROC_REPEAT_31I(what, 31), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_33(what, _index) _SCL_PREPROC_REPEAT_32(what, 32), what
#define _SCL_PREPROC_REPEAT_33I(what, _index) _SCL_PREPROC_REPEAT_32I(what, 32), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_34(what, _index) _SCL_PREPROC_REPEAT_33(what, 33), what
#define _SCL_PREPROC_REPEAT_34I(what, _index) _SCL_PREPROC_REPEAT_33I(what, 33), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_35(what, _index) _SCL_PREPROC_REPEAT_34(what, 34), what
#define _SCL_PREPROC_REPEAT_35I(what, _index) _SCL_PREPROC_REPEAT_34I(what, 34), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_36(what, _index) _SCL_PREPROC_REPEAT_35(what, 35), what
#define _SCL_PREPROC_REPEAT_36I(what, _index) _SCL_PREPROC_REPEAT_35I(what, 35), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_37(what, _index) _SCL_PREPROC_REPEAT_36(what, 36), what
#define _SCL_PREPROC_REPEAT_37I(what, _index) _SCL_PREPROC_REPEAT_36I(what, 36), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_38(what, _index) _SCL_PREPROC_REPEAT_37(what, 37), what
#define _SCL_PREPROC_REPEAT_38I(what, _index) _SCL_PREPROC_REPEAT_37I(what, 37), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_39(what, _index) _SCL_PREPROC_REPEAT_38(what, 38), what
#define _SCL_PREPROC_REPEAT_39I(what, _index) _SCL_PREPROC_REPEAT_38I(what, 38), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_40(what, _index) _SCL_PREPROC_REPEAT_39(what, 39), what
#define _SCL_PREPROC_REPEAT_40I(what, _index) _SCL_PREPROC_REPEAT_39I(what, 39), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_41(what, _index) _SCL_PREPROC_REPEAT_40(what, 40), what
#define _SCL_PREPROC_REPEAT_41I(what, _index) _SCL_PREPROC_REPEAT_40I(what, 40), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_42(what, _index) _SCL_PREPROC_REPEAT_41(what, 41), what
#define _SCL_PREPROC_REPEAT_42I(what, _index) _SCL_PREPROC_REPEAT_41I(what, 41), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_43(what, _index) _SCL_PREPROC_REPEAT_42(what, 42), what
#define _SCL_PREPROC_REPEAT_43I(what, _index) _SCL_PREPROC_REPEAT_42I(what, 42), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_44(what, _index) _SCL_PREPROC_REPEAT_43(what, 43), what
#define _SCL_PREPROC_REPEAT_44I(what, _index) _SCL_PREPROC_REPEAT_43I(what, 43), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_45(what, _index) _SCL_PREPROC_REPEAT_44(what, 44), what
#define _SCL_PREPROC_REPEAT_45I(what, _index) _SCL_PREPROC_REPEAT_44I(what, 44), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_46(what, _index) _SCL_PREPROC_REPEAT_45(what, 45), what
#define _SCL_PREPROC_REPEAT_46I(what, _index) _SCL_PREPROC_REPEAT_45I(what, 45), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_47(what, _index) _SCL_PREPROC_REPEAT_46(what, 46), what
#define _SCL_PREPROC_REPEAT_47I(what, _index) _SCL_PREPROC_REPEAT_46I(what, 46), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_48(what, _index) _SCL_PREPROC_REPEAT_47(what, 47), what
#define _SCL_PREPROC_REPEAT_48I(what, _index) _SCL_PREPROC_REPEAT_47I(what, 47), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_49(what, _index) _SCL_PREPROC_REPEAT_48(what, 48), what
#define _SCL_PREPROC_REPEAT_49I(what, _index) _SCL_PREPROC_REPEAT_48I(what, 48), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_50(what, _index) _SCL_PREPROC_REPEAT_49(what, 49), what
#define _SCL_PREPROC_REPEAT_50I(what, _index) _SCL_PREPROC_REPEAT_49I(what, 49), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_51(what, _index) _SCL_PREPROC_REPEAT_50(what, 50), what
#define _SCL_PREPROC_REPEAT_51I(what, _index) _SCL_PREPROC_REPEAT_50I(what, 50), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_52(what, _index) _SCL_PREPROC_REPEAT_51(what, 51), what
#define _SCL_PREPROC_REPEAT_52I(what, _index) _SCL_PREPROC_REPEAT_51I(what, 51), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_53(what, _index) _SCL_PREPROC_REPEAT_52(what, 52), what
#define _SCL_PREPROC_REPEAT_53I(what, _index) _SCL_PREPROC_REPEAT_52I(what, 52), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_54(what, _index) _SCL_PREPROC_REPEAT_53(what, 53), what
#define _SCL_PREPROC_REPEAT_54I(what, _index) _SCL_PREPROC_REPEAT_53I(what, 53), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_55(what, _index) _SCL_PREPROC_REPEAT_54(what, 54), what
#define _SCL_PREPROC_REPEAT_55I(what, _index) _SCL_PREPROC_REPEAT_54I(what, 54), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_56(what, _index) _SCL_PREPROC_REPEAT_55(what, 55), what
#define _SCL_PREPROC_REPEAT_56I(what, _index) _SCL_PREPROC_REPEAT_55I(what, 55), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_57(what, _index) _SCL_PREPROC_REPEAT_56(what, 56), what
#define _SCL_PREPROC_REPEAT_57I(what, _index) _SCL_PREPROC_REPEAT_56I(what, 56), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_58(what, _index) _SCL_PREPROC_REPEAT_57(what, 57), what
#define _SCL_PREPROC_REPEAT_58I(what, _index) _SCL_PREPROC_REPEAT_57I(what, 57), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_59(what, _index) _SCL_PREPROC_REPEAT_58(what, 58), what
#define _SCL_PREPROC_REPEAT_59I(what, _index) _SCL_PREPROC_REPEAT_58I(what, 58), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_60(what, _index) _SCL_PREPROC_REPEAT_59(what, 59), what
#define _SCL_PREPROC_REPEAT_60I(what, _index) _SCL_PREPROC_REPEAT_59I(what, 59), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_61(what, _index) _SCL_PREPROC_REPEAT_60(what, 60), what
#define _SCL_PREPROC_REPEAT_61I(what, _index) _SCL_PREPROC_REPEAT_60I(what, 60), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_62(what, _index) _SCL_PREPROC_REPEAT_61(what, 61), what
#define _SCL_PREPROC_REPEAT_62I(what, _index) _SCL_PREPROC_REPEAT_61I(what, 61), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_63(what, _index) _SCL_PREPROC_REPEAT_62(what, 62), what
#define _SCL_PREPROC_REPEAT_63I(what, _index) _SCL_PREPROC_REPEAT_62I(what, 62), LIST_INDEX(what, _index)
#define _SCL_PREPROC_REPEAT_64(what, _index) _SCL_PREPROC_REPEAT_63(what, 63), what
#define _SCL_PREPROC_REPEAT_64I(what, _index) _SCL_PREPROC_REPEAT_63I(what, 63), LIST_INDEX(what, _index)

#endif // COMMON_H
