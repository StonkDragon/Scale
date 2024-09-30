
#if !defined(_WIN32) && !defined(__wasm__)
#include <execinfo.h>
#endif
#ifdef _WIN32
#include <Windows.h>
#include <dbghelp.h>
#undef __STRUCT__
#undef interface
#endif

#include <unordered_set>
#include <stack>
#include <sstream>
#if defined(_WIN32)
#include <io.h>
#define write _write
#else
#include <unistd.h>
#endif

#include "headers/Common.hpp"
#include "headers/Types.hpp"
#include "headers/TranspilerDefs.hpp"

namespace sclc
{
    #ifdef _WIN32
    #define COLOR(NAME, WHAT) std::string Color::NAME = ""
    #else
    #define COLOR(NAME, WHAT) std::string Color::NAME = WHAT
    #endif

    COLOR(RESET, "\033[0m");
    COLOR(BLACK, "\033[30m");
    COLOR(RED, "\033[31m");
    COLOR(GREEN, "\033[32m");
    COLOR(YELLOW, "\033[33m");
    COLOR(BLUE, "\033[34m");
    COLOR(MAGENTA, "\033[35m");
    COLOR(CYAN, "\033[36m");
    COLOR(WHITE, "\033[37m");
    COLOR(GRAY, "\033[30m");
    COLOR(BOLDBLACK, "\033[1m\033[30m");
    COLOR(BOLDRED, "\033[1m\033[31m");
    COLOR(BOLDGREEN, "\033[1m\033[32m");
    COLOR(BOLDYELLOW, "\033[1m\033[33m");
    COLOR(BOLDBLUE, "\033[1m\033[34m");
    COLOR(BOLDMAGENTA, "\033[1m\033[35m");
    COLOR(BOLDCYAN, "\033[1m\033[36m");
    COLOR(BOLDWHITE, "\033[1m\033[37m");
    COLOR(BOLDGRAY, "\033[1m\033[30m");
    COLOR(UNDERLINE, "\033[4m");
    COLOR(BOLD, "\033[1m");
    COLOR(REVERSE, "\033[7m");
    COLOR(HIDDEN, "\033[8m");
    COLOR(ITALIC, "\033[3m");
    COLOR(STRIKETHROUGH, "\033[9m");

    Tokenizer* Main::tokenizer = nullptr;
    SyntaxTree* Main::lexer = nullptr;
    Parser* Main::parser = nullptr;
    DragonConfig::CompoundEntry* Main::config = nullptr;
    std::vector<std::string> Main::frameworkNativeHeaders = std::vector<std::string>();
    std::vector<std::string> Main::frameworks = std::vector<std::string>();
    std::vector<std::string> Main::frameworkPaths = std::vector<std::string>();
    Version* Main::version = new Version(0, 0, 0);
    bool Main::options::noMain = false;
    bool Main::options::Werror = false;
    size_t Main::options::stackSize = 0;
    long long Main::writeTablesTime = 0;
    long long Main::tokenHandleTime = 0;
    long long Main::writeHeaderTime = 0;
    bool Main::options::embedded = false;
    long long Main::writeStructsTime = 0;
    long long Main::writeGlobalsTime = 0;
    size_t Main::options::errorLimit = 20;
    bool Main::options::printDocs = false;
    long long Main::writeFunctionsTime = 0;
    long long Main::writeContainersTime = 0;
    bool Main::options::printCflags = false;
    bool Main::options::noLinkScale = false;
    std::string Main::options::outfile = "";
    bool Main::options::binaryHeader = false;
    bool Main::options::assembleOnly = false;
    bool Main::options::transpileOnly = false;
    bool Main::options::preprocessOnly = false;
    std::string Main::options::optimizer = "O0";
    std::string Main::options::printDocFor = "";
    long long Main::writeFunctionHeadersTime = 0;
    bool Main::options::noScaleFramework = false;
    size_t Main::options::docPrinterArgsStart = 0;
    bool Main::options::dontSpecifyOutFile = false;
    std::string Main::options::docsIncludeFolder = "";
    std::vector<std::string> Main::options::files = std::vector<std::string>();
    std::vector<std::string> Main::options::features = std::vector<std::string>();
    std::vector<std::string> Main::options::includePaths = std::vector<std::string>();
    std::vector<std::string> Main::options::filesFromCommandLine = std::vector<std::string>();
    std::unordered_map<std::string, std::string> Main::options::mapFrameworkDocfiles = std::unordered_map<std::string, std::string>();
    std::unordered_map<std::string, std::string> Main::options::mapFrameworkIncludeFolders = std::unordered_map<std::string, std::string>();
    std::unordered_map<std::string, std::string> Main::options::mapIncludePathsToFrameworks = std::unordered_map<std::string, std::string>();
    std::unordered_map<std::string, DragonConfig::CompoundEntry*> Main::options::indexDrgFiles = std::unordered_map<std::string, DragonConfig::CompoundEntry*>();

    std::vector<std::string> keywords({
        "import",
        "construct",
        "final",
        "ref",
        "typeof",
        "nameof",
        "sizeof",
        "typealias",
        "layout",
        "asm",
        "nothing",
        "none",
        "int",
        "float",
        "int32",
        "int16",
        "int8",
        "uint32",
        "uint16",
        "uint8",
        "uint",
        "int64",
        "uint64",
        "any",
        "str",
        "varargs",
        "self",
        "super",
        "bool",
        "final",
        "final",
        "static",
        "private",
        "expect",
        "unsafe",
        "const",
        "readonly",
        "try",
        "catch",
        "throw",
        "open",
        "lambda",
    });

    Struct Struct::Null = Struct("");

    std::vector<Variable> vars;
    std::vector<size_t> var_indices;

    void print_trace(void) {
#ifdef _WIN32
        void* stack[64];
        unsigned short frames;
        SYMBOL_INFO* symbol;
        HANDLE process;

        process = GetCurrentProcess();

        SymInitialize(process, NULL, TRUE);

        frames = CaptureStackBackTrace(0, 64, stack, NULL);
        symbol = (SYMBOL_INFO*) malloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char));
        symbol->MaxNameLen = 255;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

        for (unsigned int i = 0; i < frames; i++) {
            SymFromAddr(process, (DWORD64) (stack[i]), 0, symbol);

            printf("%i: %s - 0x%0llX\n", frames - i - 1, symbol->Name, symbol->Address);
        }

        free(symbol);
#endif
#if !defined(_WIN32) && !defined(__wasm__)
        void* array[64];
        int size = backtrace(array, 64);
        backtrace_symbols_fd(array, size, 2);
#endif
    }

    size_t static_itos(int i, char buf[4]) {
        size_t j = 0;
        size_t k = 0;
        int sign = 0;
        char tmp[4];
        if (i < 0) {
            sign = 1;
            i = -i;
        }
        do {
            tmp[j++] = i % 10 + '0';
            i /= 10;
        } while (i);
        if (sign) {
            tmp[j++] = '-';
        }
        while (j > 0) {
            buf[k++] = tmp[--j];
        }
        buf[k] = 0;
        return k;
    }

    #define WRITESIGSTRING(_sig) if (signum == _sig) write(2, #_sig, sizeof(#_sig) - 1)

    jmp_buf global_jmp_buf;

    void throw_up(int sig) {
        longjmp(global_jmp_buf, sig);
    }

    void signalHandler(int signum) {
        write(2, "\n", 1);
        write(2, "Signal ", 7);
        WRITESIGSTRING(SIGABRT);
        WRITESIGSTRING(SIGSEGV);
        write(2, " received.\n", 11);
        print_trace();
        
        throw_up(signum);
    }

    bool strends(const std::string& str, const std::string& suffix) {
        if (str.size() < suffix.size()) return false;
        size_t start = str.size() - suffix.size();
        for (size_t i = 0; i < suffix.size(); i++) {
            if (str[start + i] != suffix[i]) {
                return false;
            }
        }
        return true;
    }

    bool strstarts(const std::string& str, const std::string& prefix) {
        if (str.size() < prefix.size()) return false;
        for (size_t i = 0; i < prefix.size(); i++) {
            if (str[i] != prefix[i]) {
                return false;
            }
        }
        return true;
    }

    bool pathstarts(std::filesystem::path str, std::string prefix) {
        return strstarts(str.string(), prefix);
    }

    bool strcontains(const std::string& str, const std::string& substr) {
        return str.find(substr) != std::string::npos;
    }

    bool pathcontains(std::filesystem::path str, std::string substr) {
        return strcontains(str.string(), substr);
    }

    int isCharacter(char c) {
        return (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '_' || c < 0;
    }

    int isDigit(char c) {
        return c >= '0' &&
            c <= '9';
    }

    int isSpace(char c) {
        return c == ' ' ||
            c == '\t' ||
            c == '\n' ||
            c == '\r';
    }

    int isPrint(char c) {
        return (c >= ' ');
    }

    int isBracket(char c) {
        return c == '(' ||
            c == ')' ||
            c == '{' ||
            c == '}' ||
            c == '[' ||
            c == ']';
    }

    int isHexDigit(char c) {
        return (c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F') ||
            c == 'x' ||
            c == 'X';
    }

    int isOctDigit(char c) {
        return (c >= '0' && c <= '7') ||
            c == 'o' ||
            c == 'O';
    }

    int isBinDigit(char c) {
        return c == '0' ||
            c == '1' ||
            c == 'b' ||
            c == 'B';
    }

    int isOperator(char c) {
        return c == '?' ||
            c == '@' ||
            c == '(' ||
            c == ')' ||
            c == ',' ||
            c == '+' ||
            c == '-' ||
            c == '*' ||
            c == '/' ||
            c == '%' ||
            c == '&' ||
            c == '|' ||
            c == '^' ||
            c == '~' ||
            c == '<' ||
            c == '>' ||
            c == ':' ||
            c == '=' ||
            c == '!';
    }

    void addIfAbsent(std::vector<Function*>& vec, Function* str) {
        for (size_t i = 0; i < vec.size(); i++) {
            if (str->isMethod && vec[i]->isMethod) {
                if (vec[i]->name == str->name && vec[i]->member_type == str->member_type) {
                    return;
                }
            } else if (!str->isMethod && !vec[i]->isMethod) {
                if (vec[i]->operator==(str)) {
                    return;
                }
            }
        }
        vec.push_back(str);
    }

    std::string replaceAll(const std::string& src, const std::string& from, const std::string& to) {
        if (from.empty())
            return src;

        std::string str = src;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return str;
    }

    std::string replaceFirstAfter(const std::string& src, const std::string& from, const std::string& to, int index) {
        try {
            size_t actual = src.find(from, index - 1);
            if (actual == std::string::npos) {
                return src;
            }
            std::string pre = src.substr(0, actual);
            std::string post = src.substr(actual + from.length());
            return pre + to + post;
        } catch (std::out_of_range const&) {
            return src;
        }
    }

    int lastIndexOf(char* src, char c) {
        int i = strlen(src) - 1;
        while (i >= 0 && src[i] != c) {
            i--;
        }
        return i;
    }

    bool hasVar(const std::string& name) {
        for (auto&& v : vars) {
            if (v.name == name) {
                return true;
            }
        }
        return false;
    }

    Variable getVar(const std::string& name) {
        for (auto&& v : vars) {
            if (v.name == name) {
                return v;
            }
        }
        return Variable("", "");
    }

    long long parseNumber(std::string str) {
        if (str == "true") return 1;
        if (str == "false" || str == "nil") return 0;
        
        long long value;
        if (str.substr(0, 2) == "0x") {
            value = std::stoll(str.substr(2), nullptr, 16);
        } else if (str.substr(0, 2) == "0b") {
            value = std::stoll(str.substr(2), nullptr, 2);
        } else if (str.substr(0, 2) == "0o") {
            value = std::stoll(str.substr(2), nullptr, 8);
        } else {
            value = std::stoll(str);
        }
        return value;
    }

    Result<double, FPResult> parseDouble(const Token& tok) {
        double num;
        try
        {
            num = std::stold(tok.value);
        }
        catch(const std::exception& e)
        {
            FPResult r;
            r.message = "Number out of range: " + tok.value;
            r.location = tok.location;
            r.type = tok.type;
            r.value = tok.value;
            r.success = false;
            return Result<double, FPResult>(r);
        }
        return num;
    }

    void logWarns(std::vector<FPResult>& warns);

    FPResult parseType(std::vector<Token>& body, size_t& i) {
        FPResult r;
        r.success = true;
        r.location = body[i].location;
        r.type = body[i].type;
        r.message = "";
        r.value = "";
        std::string type_mods = "";
        bool isConst = false;
        bool isReadonly = false;

        if ((body[i].type == tok_identifier && body[i].value == "*") || body[i].type == tok_addr_of) {
            if (body[i].type != tok_addr_of) {
                FPResult r2;
                r2.success = true;
                r2.location = body[i].location;
                r2.value = body[i].value;
                r2.type = body[i].type;
                r2.message = "Using '*' to denote a value-type is deprecated. Use '@' instead";
                r.warns.push_back(r2);
                logWarns(r.warns);
            }
            type_mods += "@";
            i++;
        }

        while (body[i].value == "const"|| body[i].value == "readonly") {
            if (!isConst && body[i].value == "const") {
                type_mods += "const ";
                isConst = true;
            } else if (!isReadonly && body[i].value == "readonly") {
                type_mods += "readonly ";
                isReadonly = true;
            }
            i++;
        }
        if (body[i].type == tok_lambda) {
            r.value = type_mods + "lambda";
            if (body[i + 1].type == tok_paren_open) {
                i++;
                r.value += "(";
                int count = 0;
                if (body[i].type == tok_paren_open) {
                    i++;
                    while (body[i].type != tok_paren_close) {
                        FPResult tmp = parseType(body, i);
                        if (!tmp.success) return tmp;
                        i++;
                        if (body[i].type == tok_comma) {
                            i++;
                        }
                        count++;
                    }
                    i++;
                }
                r.value += std::to_string(count) + ")";
                if (body[i].type == tok_column) {
                    i++;
                    FPResult tmp = parseType(body, i);
                    if (!tmp.success) return tmp;
                    r.value += ":" + tmp.value;
                } else {
                    i--;
                    r.value += ":none";
                }
            }
        } else if (body[i].type == tok_varargs) {
            r.value = type_mods + body[i].value;
        } else if (body[i].type == tok_identifier) {
            r.value = type_mods + body[i].value;
            if (body[i].value == "async") {
                i++;
                if (r.type != tok_identifier || body[i].value != "<") {
                    r.success = false;
                    r.message = "Expected '<', but got '" + body[i].value + "'";
                    return r;
                }
                i++;
                FPResult tmp = parseType(body, i);
                if (!tmp.success) return tmp;
                r.value += "<" + tmp.value + ">";
                i++;
                return r;
            } else if (i + 1 < body.size() && body[i + 1].type == tok_double_column) {
                i++;
                i++;
                FPResult tmp = parseType(body, i);
                if (!tmp.success) return tmp;
                r.value += "$" + tmp.value;
            }
        } else if (body[i].type == tok_question_mark || body[i].value == "?") {
            r.value = type_mods + "?";
        } else if (body[i].type == tok_bracket_open) {
            std::string type = "[";
            std::string size = "";
            i++;
            r = parseType(body, i);
            if (!r.success) {
                return r;
            }
            type += r.value;
            if (size.size()) {
                type += ";" + size;
            }
            i++;
            if (body[i].type == tok_bracket_close) {
                type += "]";
                r.value = type_mods + type;
            } else {
                r.message = "Expected ']', but got '" + body[i].value + "'";
                r.value = type_mods + body[i].value;
                r.location = body[i].location;
                r.type = body[i].type;
                r.success = false;
            }
        } else {
            r.success = false;
            r.location = body[i].location;
            r.value = body[i].value;
            r.type = body[i].type;
            r.message = "Unexpected token: '" + body[i].toString() + "'";
        }
        if (i + 1 < body.size() && body[i + 1].value == "?") {
            i++;
            r.value += "?";
        }
        return r;
    }

    const std::unordered_map<std::string, std::string> funcNameIdents = {
        std::pair("+", "operator$add"),
        std::pair("-", "operator$sub"),
        std::pair("*", "operator$mul"),
        std::pair("/", "operator$div"),
        std::pair("%", "operator$mod"),
        std::pair("&", "operator$logic_and"),
        std::pair("|", "operator$logic_or"),
        std::pair("^", "operator$logic_xor"),
        std::pair("~", "operator$logic_not"),
        std::pair("<<", "operator$logic_lsh"),
        std::pair("<<<", "operator$logic_rol"),
        std::pair(">>", "operator$logic_rsh"),
        std::pair(">>>", "operator$logic_ror"),
        std::pair("**", "operator$pow"),
        std::pair(".", "operator$dot"),
        std::pair("<", "operator$less"),
        std::pair("<=", "operator$less_equal"),
        std::pair(">", "operator$more"),
        std::pair(">=", "operator$more_equal"),
        std::pair("==", "operator$equal"),
        std::pair("!", "operator$not"),
        std::pair("!!", "operator$assert_not_nil"),
        std::pair("!=", "operator$not_equal"),
        std::pair("&&", "operator$bool_and"),
        std::pair("||", "operator$bool_or"),
        std::pair("++", "operator$inc"),
        std::pair("--", "operator$dec"),
        std::pair("@", "operator$at"),
        std::pair("=>", "operator$store"),
        std::pair("=>[]", "operator$set"),
        std::pair("[]", "operator$get"),
        std::pair("?", "operator$wildcard"),
        std::pair("?:", "operator$elvis"),
        std::pair("/.", "operator$lcm"),
        std::pair("*.", "operator$gcd"),
        std::pair(">.", "operator$max"),
        std::pair("<.", "operator$min"),
    };

    Function* getFunctionByName0(TPResult& result, const std::string& name) {
        static std::unordered_map<std::string, Function*> cache;
        
        auto it = cache.find(name);
        if (it != cache.end()) return it->second;

        for (Function* func : result.functions) {
            if (func->isMethod) continue;
            if ((func->name_without_overload == name || func->name == name)) {
                return cache[name] = func;
            }
        }
        return cache[name] = nullptr;
    }
    Function* getFunctionByName(TPResult& result, const std::string& name2) {
        try {
            const std::string& name = funcNameIdents.at(name2);
            return getFunctionByName0(result, name);
        } catch (std::out_of_range& _) {
            return getFunctionByName0(result, name2);
        }
    }

    bool hasEnum(TPResult& result, const std::string& name) {
        for (const Enum& e : result.enums) {
            if (e.isExtern) continue;
            if (e.name == name) {
                return true;
            }
        }
        for (const Enum& e : result.enums) {
            if (e.name == name) {
                return true;
            }
        }
        return false;
    }

    Enum getEnumByName(TPResult& result, const std::string& name) {
        for (const Enum& e : result.enums) {
            if (e.name == name) {
                return e;
            }
        }
        return Enum("");
    }
    
    Interface* getInterfaceByName(TPResult& result, const std::string& name) {
        for (Interface* i : result.interfaces) {
            if (i->name == name) {
                return i;
            }
        }
        return nullptr;
    }

    Method* getMethodByName0(TPResult& result, const std::string& name, const std::string& type) {
        if (type == "") {
            return nullptr;
        }

        for (Function* func : result.functions) {
            if (!func->isMethod) continue;
            if ((func->name_without_overload == name || func->name == name) && func->member_type == type) {
                return (Method*) func;
            }
        }
        if (getInterfaceByName(result, type)) {
            return nullptr;
        }
        return getMethodByName0(result, name, getStructByName(result, type).super);
    }

    Method* getMethodByName(TPResult& result, const std::string& name2, const std::string& type) {
        try {
            const std::string& name = funcNameIdents.at(name2);
            return getMethodByName0(result, name, removeTypeModifiers(type));
        } catch (std::out_of_range& _) {
            return getMethodByName0(result, name2.substr(0, name2.find("$$ol")), removeTypeModifiers(type));
        }
    }
    Method* getMethodByNameOnThisType0(TPResult& result, const std::string& name, const std::string& type) {
        if (type == "") {
            return nullptr;
        }

        for (Function* func : result.functions) {
            if (!func->isMethod) continue;
            if ((func->name_without_overload == name || func->name == name) && func->member_type == type) {
                return (Method*) func;
            }
        }
        return nullptr;
    }
    Method* getMethodByNameOnThisType(TPResult& result, const std::string& name2, const std::string& type) {
        try {
            const std::string& name = funcNameIdents.at(name2);
            return getMethodByNameOnThisType0(result, name, removeTypeModifiers(type));
        } catch (std::out_of_range& _) {
            return getMethodByNameOnThisType0(result, name2.substr(0, name2.find("$$ol")), removeTypeModifiers(type));
        }
    }

    std::vector<Method*> methodsOnType(TPResult& res, std::string type) {
        type = removeTypeModifiers(type);

        std::vector<Method*> methods;
        methods.reserve(res.functions.size());

        for (Function* func : res.functions) {
            if (!func->isMethod) continue;
            if (((Method*) func)->member_type == type) {
                methods.push_back((Method*) func);
            }
        }
        return methods;
    }

    std::string removeTypeModifiers(std::string t);

    Struct& getStructByName(TPResult& result, const std::string& name) {
        const std::string& typeName = removeTypeModifiers(name);
        for (auto it = result.structs.begin(); it != result.structs.end(); it++) {
            if (it->isExtern()) continue;
            if (it->name == typeName) {
                return *it;
            }
        }
        for (auto it = result.structs.begin(); it != result.structs.end(); it++) {
            if (it->name == typeName) {
                return *it;
            }
        }
        return Struct::Null;
    }

    Layout EmptyLayout = Layout("");

    Layout& getLayout(TPResult& result, const std::string& name) {
        const std::string& typeName = removeTypeModifiers(name);
        for (Layout& layout : result.layouts) {
            if (layout.isExtern) continue;
            if (layout.name == typeName) {
                return layout;
            }
        }
        for (Layout& layout : result.layouts) {
            if (layout.name == typeName) {
                return layout;
            }
        }
        return EmptyLayout;
    }

    bool hasLayout(TPResult& result, const std::string& name) {
        return !(getLayout(result, name).name.empty());
    }

    bool hasStruct(TPResult& result, const std::string& name) {
        return !(getStructByName(result, name).name.empty());
    }

    bool hasFunction(TPResult& result, const std::string& name) {
        return getFunctionByName(result, name) != nullptr;
    }

    bool hasMethod(TPResult& result, const std::string& name, const std::string& type) {
        return getMethodByName(result, name, type) != nullptr;
    }

    bool hasGlobal(TPResult& result, std::string name) {
        for (Variable& v : result.globals) {
            if (v.name == name) {
                return true;
            }
        }
        return false;
    }

    bool isInitFunction(Function* f) {
        if (f->isMethod) {
            Method* m = static_cast<Method*>(f);
            return m->has_constructor || strstarts(m->name, "init");
        } else {
            return f->has_construct;
        }
    }

    bool isDestroyFunction(Function* f) {
        return f->has_destructor;
    }

    bool memberOfStruct(const Variable* self, Function* f) {
        return f->member_type == self->internalMutableFrom || strstarts(self->name, f->member_type + "$");
    }

    bool typeCanBeNil(std::string s, bool rem) {
        return isPrimitiveType(s, rem) || (s.size() > 1 && s.back() == '?');
    }

    bool typeIsReadonly(std::string s) {
        if (s.empty()) return false;
        if (s.size() && s.front() == '@') s.erase(0, 1);
        while (strstarts(s, "const ")) {
            s.erase(0, 6);
        }
        return strstarts(s, "readonly ");
    }

    bool typeIsConst(std::string s) {
        if (s.empty()) return false;
        if (s.size() && s.front() == '@') s.erase(0, 1);
        while (strstarts(s, "readonly ")) {
            s.erase(0, 9);
        }
        return strstarts(s, "const ");
    }

    bool isPrimitiveType(std::string s, bool rem) {
        if (rem) s = removeTypeModifiers(s);
        return isPrimitiveIntegerType(s, rem) || s == "float" || s == "float32" || s == "any" || s == "bool";
    }

    bool featureEnabled(std::string feat) {
        return contains<std::string>(Main::options::features, feat);
    }

    std::string& operator*(std::string& toRepeat, const size_t count) {
        if (count == 0) {
            toRepeat.clear();
            return toRepeat;
        }
        if (count == 1) {
            return toRepeat;
        }
        const size_t oldSize = toRepeat.size();
        const size_t newSize = oldSize * count;
        toRepeat.resize(newSize);
        for (size_t i = 1; i < count; i++) {
            memcpy(&toRepeat[0] + (i * oldSize), &toRepeat[0], oldSize);
        }
        return toRepeat;
    }

    extern Function* currentFunction;
    extern Struct currentStruct;
    extern std::vector<std::string> typeStack;
    extern int scopeDepth;

    handler(Token);

    std::string makeIndex(TPResult& result, std::vector<Token>& body, size_t& i, std::vector<FPResult>& errors, std::vector<FPResult>& warns, Function* function) {
        std::ostringstream fp;
        append2("({\n");
        scopeDepth++;
        while (i < body.size() && body[i].type != tok_bracket_close) {
            handle(Token);
            safeInc("");
        }
        append("scale_pop(%s);\n", sclTypeToCType(result, typeStackTop).c_str());
        scopeDepth--;
        append("})");
        fp.flush();
        return fp.str();
    }

    void makePath(TPResult& result, Variable v, bool topLevelDeref, std::vector<Token>& body, size_t& i, std::vector<FPResult>& errors, bool doesWriteAfter, Function* function, std::vector<FPResult>& warns, std::ostream& fp, std::function<void(std::string, std::string)> onComplete) {
        (void) fp;
        std::string path = "Var_" + v.name;
        std::string currentType;
        currentType = v.type;
        if (topLevelDeref) {
            path = "(*" + path + ")";
            currentType = typePointedTo(currentType);
        }

        if (doesWriteAfter && !v.isWritableFrom(currentFunction)) {
            transpilerError("Variable '" + v.name + "' is not mutable", i);
            errors.push_back(err);
        }

        Struct s = Struct::Null;
        Layout l = EmptyLayout;
        while (i + 1 < body.size() && (body[i + 1].type == tok_dot || body[i + 1].type == tok_bracket_open)) {
            safeInc();
            if (body[i].type == tok_dot) {
                safeInc();
                bool deref = body[i].type == tok_addr_of;
                if (deref) {
                    safeInc();
                }
                s = getStructByName(result, currentType);
                std::string containingType = currentType;
                bool useLayout = false;
                if (s == Struct::Null) {
                    l = getLayout(result, currentType);
                    if (l == EmptyLayout) {
                        transpilerError("No Struct or Layout named '" + currentType + "' exists", i);
                        errors.push_back(err);
                        return;
                    }
                    useLayout = true;
                }
                bool valueType = currentType.front() == '@';

                if (useLayout) {
                    if (l.hasMember(body[i].value)) {
                        v = l.getMember(body[i].value);
                    } else {
                        transpilerError("Layout '" + currentType + "' has no member named '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                } else {
                    if (s.hasMember(body[i].value)) {
                        v = s.getMember(body[i].value);
                    } else {
                        transpilerError("Struct '" + currentType + "' has no member named '" + body[i].value + "'", i);
                        errors.push_back(err);
                        return;
                    }
                }


                currentType = v.type;
                if (deref) {
                    currentType = typePointedTo(currentType);
                }

                if (!v.isAccessible(currentFunction)) {
                    transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                    errors.push_back(err);
                }

                if (doesWriteAfter && !v.isWritableFrom(currentFunction)) {
                    transpilerError("Variable '" + v.name + "' is not mutable", i);
                    errors.push_back(err);
                }

                Method* f = nullptr;
                bool mutate = false;
                if (!doesWriteAfter || (i + 1 < body.size() && (body[i + 1].type == tok_dot || body[i + 1].type == tok_bracket_open))) {
                    f = attributeAccessor(result, containingType, v.name);
                } else {
                    mutate = true;
                    f = attributeMutator(result, containingType, v.name);
                }
                if (f) {
                    if (containingType.front() == '@' && f->args.back().type.front() != '@') {
                        path = "&" + path;
                    } else if (containingType.front() != '@' && f->args.back().type.front() == '@') {
                        path = "*" + path;
                    }
                    if (mutate) {
                        path = "mt_" + f->member_type + "$" + f->name + "(" + path + ", ";
                        std::vector<Function*> funcs;
                        for (auto&& f : result.functions) {
                            if (f->name_without_overload == v.type + "$operator$store" || f->name_without_overload == v.type + "$=>") {
                                funcs.push_back(f);
                            }
                        }

                        bool funcFound = false;
                        for (Function* f : funcs) {
                            if (
                                f->isMethod ||
                                f->args.size() != 1 ||
                                f->return_type != v.type ||
                                !typesCompatible(result, typeStackTop, f->args[0].type, true)
                            ) {
                                continue;
                            }
                            if (currentType.front() == '@' && f->return_type.front() != '@') {
                                path += "*";
                            }
                            if (f->args[0].type.front() == '@') {
                                path += "fn_" + f->name + "(*tmp)";
                            } else {
                                path += "fn_" + f->name + "(tmp)";
                            }
                            path += "fn_" + f->name + "(tmp)";
                            funcFound = true;
                        }
                        if (currentType.front() == '@') {
                            path += "*";
                        }
                        if (!funcFound) {
                            path += "tmp";
                        }
                        path += ")";
                        onComplete(path, currentType);
                        return;
                    } else {
                        path = "mt_" + f->member_type + "$" + f->name + "(" + path + ")";
                    }
                } else {
                    if (valueType) {
                        path = path + "." + body[i].value;
                    } else {
                        path = path + "->" + body[i].value;
                    }
                }
                if (deref) {
                    path = "(*" + path + ")";
                }
            } else if (body[i].type == tok_bracket_open) {
                size_t start = i;
                safeInc();

                if (body[i].type == tok_bracket_close) {
                    transpilerError("Expected expression, but got ']'", i);
                    errors.push_back(err);
                    return;
                }

                std::string index = makeIndex(result, body, i, errors, warns, function);

                if (i >= body.size()) {
                    transpilerError("Expected ']', but got end of function", i);
                    errors.push_back(err);
                    return;
                }
                
                bool getElem = !doesWriteAfter || (i + 1 < body.size() && (body[i + 1].type == tok_bracket_open || body[i + 1].type == tok_dot));
                bool primitive = false;
                std::string arrayType = removeTypeModifiers(currentType);
                std::string nonRemovedArray = currentType;
                std::string indexingType = "";

                Method* m = nullptr;
                if (arrayType.size() > 2 && arrayType.front() == '[' && arrayType.back() == ']') {
                    indexingType = "int";
                    primitive = true;
                } else if (getElem) {
                    m = getMethodByName(result, "[]", arrayType);
                    if (m == nullptr) {
                        transpilerError("Type '" + arrayType + "' does not overload operator '[]'", start);
                        errors.push_back(err);
                        return;
                    }
                    indexingType = m->args[0].type;
                } else {
                    m = getMethodByName(result, "=>[]", arrayType);
                    if (m == nullptr) {
                        transpilerError("Type '" + arrayType + "' does not overload operator '=>[]'", start);
                        errors.push_back(err);
                        return;
                    }
                    indexingType = m->args[0].type;
                }
                std::string indexType = removeTypeModifiers(typeStackTop);
                typePop;
                if (!typeEquals(indexType, removeTypeModifiers(indexingType))) {
                    transpilerError("'" + arrayType + "' cannot be indexed with '" + indexType + "'", start);
                    errors.push_back(err);
                    return;
                }

                if (primitive) {
                    currentType = arrayType.substr(1, arrayType.size() - 2);

                    if (doesWriteAfter && typeIsConst(currentType)) {
                        transpilerError("Variable '" + v.name + "' is an array of '" + currentType + "' which is not mutable", start);
                        errors.push_back(err);
                    }

                    if (getElem) {
                        path = "scale_checked_index(" + path + ", " + index + ")";
                    } else {
                        path = "scale_checked_write(" + path + ", " + index + ", ";
                        std::vector<Function*> funcs;
                        for (auto&& f : result.functions) {
                            if (f->name_without_overload == v.type + "$operator$store" || f->name_without_overload == v.type + "$=>") {
                                funcs.push_back(f);
                            }
                        }

                        bool funcFound = false;
                        for (Function* f : funcs) {
                            if (
                                f->isMethod ||
                                f->args.size() != 1 ||
                                f->return_type != v.type ||
                                !typesCompatible(result, typeStackTop, f->args[0].type, true)
                            ) {
                                continue;
                            }
                            if (currentType.front() == '@' && f->return_type.front() != '@') {
                                path += "*";
                            }
                            if (f->args[0].type.front() == '@') {
                                path += "fn_" + f->name + "(*tmp)";
                            } else {
                                path += "fn_" + f->name + "(tmp)";
                            }
                            funcFound = true;
                        }
                        if (!funcFound) {
                            if (currentType.front() == '@') {
                                path += "*";
                            }
                            path += "tmp";
                        }
                        path += ")";
                        onComplete(path, currentType);
                        return;
                    }
                } else {
                    if (currentType.front() == '@' && m->args.back().type.front() != '@') {
                        path = "&" + path;
                    } else if (currentType.front() != '@' && m->args.back().type.front() == '@') {
                        path = "*" + path;
                    }

                    if (getElem) {
                        path = "mt_" + arrayType + "$" + m->name + "(" + path + ", " + index + ")";
                        currentType = m->return_type;
                    } else {
                        path = "mt_" + arrayType + "$" + m->name + "(" + path + ", " + index + ", ";
                        std::vector<Function*> funcs;
                        for (auto&& f : result.functions) {
                            if (f->name_without_overload == v.type + "$operator$store" || f->name_without_overload == v.type + "$=>") {
                                funcs.push_back(f);
                            }
                        }
                        currentType = m->args[1].type;

                        bool funcFound = false;
                        for (Function* f : funcs) {
                            if (
                                f->isMethod ||
                                f->args.size() != 1 ||
                                f->return_type != v.type ||
                                !typesCompatible(result, typeStackTop, f->args[0].type, true)
                            ) {
                                continue;
                            }
                            if (currentType.front() == '@' && f->return_type.front() != '@') {
                                path += "*";
                            }
                            if (f->args[0].type.front() == '@') {
                                path += "fn_" + f->name + "(*tmp)";
                            } else {
                                path += "fn_" + f->name + "(tmp)";
                            }
                            funcFound = true;
                        }
                        if (!funcFound) {
                            if (currentType.front() == '@') {
                                path += "*";
                            }
                            path += "tmp";
                        }
                        path += ")";
                        onComplete(path, currentType);
                        return;
                    }
                }

            }
        }

        if (doesWriteAfter) {
            std::vector<Function*> funcs;
            for (auto&& f : result.functions) {
                if (f->name_without_overload == v.type + "$operator$store" || f->name_without_overload == v.type + "$=>") {
                    funcs.push_back(f);
                }
            }

            bool funcFound = false;
            for (Function* f : funcs) {
                if (
                    f->isMethod ||
                    f->args.size() != 1 ||
                    f->return_type != v.type ||
                    !typesCompatible(result, typeStackTop, f->args[0].type, true)
                ) {
                    continue;
                }
                path += " = ";
                if (currentType.front() == '@' && f->return_type.front() != '@') {
                    path += "*";
                }
                if (f->args[0].type.front() == '@') {
                    path += "fn_" + f->name + "(*tmp)";
                } else {
                    path += "fn_" + f->name + "(tmp)";
                }
                funcFound = true;
            }
            if (!funcFound) {
                path = "scale_putlocal(" + path + ", ";
                // path += " = ";
                if (currentType.front() == '@') {
                    path += "*";
                }
                path += "tmp)";
            }
        }
        
        onComplete(path, currentType);
    }

    std::pair<std::string, std::string> findNth(std::unordered_map<std::string, std::string> val, size_t n) {
        size_t i = 0;
        for (auto&& member : val) {
            if (i == n) {
                return member;
            }
            i++;
        }
        return std::pair<std::string, std::string>("", "");
    }

    std::vector<std::string> vecWithout(std::vector<std::string> vec, std::string elem) {
        std::vector<std::string> newVec;
        for (auto&& member : vec) {
            if (member != elem) {
                newVec.push_back(member);
            }
        }
        return newVec;
    }

    char unquoteChar(const std::string& str) {
        char c = str[0];
        if (c == '\\') {
            c = str[1];
            switch (c) {
                case 'n':
                    return '\n';
                    break;
                case 'r':
                    return '\r';
                    break;
                case 't':
                    return '\t';
                    break;
                case '0':
                    return '\0';
                    break;
                case '\\':
                    return '\\';
                    break;
                case '\'':
                    return '\'';
                    break;
                case '\"':
                    return '\"';
                    break;
                default:
                    std::cerr << "Unknown escape sequence: \\" << c << std::endl;
                    return ' ';
            }
        } else {
            return c;
        }
    }

    std::string unquote(const std::string& str) {
        std::string ret;
        ret.reserve(str.size());
        for (auto it = str.begin(); it != str.end(); ++it) {
            if (*it == '\\') {
                ++it;
                switch (*it) {
                    case 'n':
                        ret.push_back('\n');
                        break;
                    case 'r':
                        ret.push_back('\r');
                        break;
                    case 't':
                        ret.push_back('\t');
                        break;
                    case '0':
                        ret.push_back('\0');
                        break;
                    case '\\':
                        ret.push_back('\\');
                        break;
                    case '\'':
                        ret.push_back('\'');
                        break;
                    case '\"':
                        ret.push_back('\"');
                        break;
                    default:
                        std::cerr << "Unknown escape sequence: \\" << *it << std::endl;
                        return "";
                }
            } else {
                ret.push_back(*it);
            }
        }
        return ret;
    }

    std::string capitalize(std::string s) {
        if (s.empty()) return s;
        s[0] = std::toupper(s[0]);
        return s;
    }

    std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
        std::vector<std::string> body;
        size_t start = 0;
        size_t end = 0;
        while ((end = str.find(delimiter, start)) != std::string::npos)
        {
            body.push_back(str.substr(start, end - start));
            start = end + delimiter.size();
        }
        body.push_back(str.substr(start));
        return body;
    }

    std::string ltrim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start);
    }

    std::string scaleArgs(std::vector<Variable> args) {
        std::string result = "";
        for (size_t i = 0; i < args.size(); i++) {
            if (i) {
                result += ", ";
            }
            result += args[i].name + ": " + args[i].type;
        }
        return result;
    }

    bool structImplements(TPResult& result, Struct s, std::string interface) {
        do {
            if (s.implements(interface)) {
                return true;
            }
            s = getStructByName(result, s.super);
        } while (s.name.size());
        return false;
    }

    std::string replace(std::string s, std::string a, std::string b) {
        size_t pos = 0;
        while ((pos = s.find(a, pos)) != std::string::npos) {
            s.replace(pos, a.size(), b);
            pos += b.size();
        }
        return s;
    }

    Method* attributeAccessor(TPResult& result, std::string struct_, std::string member) {
        Method* f = getMethodByName(result, "get" + capitalize(member), struct_);
        return f && f->has_getter ? f : nullptr;
    }

    Method* attributeMutator(TPResult& result, std::string struct_, std::string member) {
        Method* f = getMethodByName(result, "set" + capitalize(member), struct_);
        return f && f->has_setter ? f : nullptr;
    }

    bool isAllowed1ByteChar(char c) {
        return (c < 0x7f && c >= ' ') || c == '\n' || c == '\t' || c == '\r';
    }

    bool checkUTF8(const std::string& str) {
        for (size_t i = 0; i < str.size(); i++) {
            if ((str[i] & 0x80) == 0x00) {
                if (!isAllowed1ByteChar(str[i])) {
                    return false;
                }
            } else if ((str[i] & 0xe0) == 0xc0) {
                if (i + 1 >= str.size()) {
                    return false;
                }
                if ((str[i + 1] & 0xc0) != 0x80) {
                    return false;
                }
                i++;
            } else if ((str[i] & 0xf0) == 0xe0) {
                if (i + 2 >= str.size()) {
                    return false;
                }
                if ((str[i + 1] & 0xc0) != 0x80) {
                    return false;
                }
                if ((str[i + 2] & 0xc0) != 0x80) {
                    return false;
                }
                i += 2;
            } else if ((str[i] & 0xf8) == 0xf0) {
                if (i + 3 >= str.size()) {
                    return false;
                }
                if ((str[i + 1] & 0xc0) != 0x80) {
                    return false;
                }
                if ((str[i + 2] & 0xc0) != 0x80) {
                    return false;
                }
                if ((str[i + 3] & 0xc0) != 0x80) {
                    return false;
                }
                i += 3;
            } else {
                return false;
            }
        }
        return true;
    }

    void checkShadow(std::string name, Token& tok, Function* function, TPResult& result, std::vector<FPResult>& warns) {
        if (hasFunction(result, name)) {
            transpilerErrorTok("Variable '" + name + "' shadowed by function '" + name + "'", tok);
            warns.push_back(err);
        }
        if (getStructByName(result, name) != Struct::Null) {
            transpilerErrorTok("Variable '" + name + "' shadowed by struct '" + name + "'", tok);
            warns.push_back(err);
        }
        if (!function->member_type.empty()) {
            if (hasFunction(result, function->member_type + "$" + name)) {
                transpilerErrorTok("Variable '" + name + "' shadowed by function '" + function->member_type + "::" + name + "'", tok);
                warns.push_back(err);
            }
        }
    }
} // namespace sclc

struct alloc_data {
    size_t bytes_alloced;
    size_t bytes_freed;
    size_t new_calls;
    size_t delete_calls;

    alloc_data() {
        bytes_alloced = 0;
        bytes_freed = 0;
        new_calls = 0;
        delete_calls = 0;
    }

    ~alloc_data() {
        DBG_NOALLOC("Allocated %zu bytes (%zu new calls)\n", bytes_alloced, new_calls);
        DBG_NOALLOC("Freed %zu bytes (%zu delete calls)\n", bytes_freed, delete_calls);
        DBG_NOALLOC("Leaked %zu bytes\n", bytes_alloced - bytes_freed);
    }
};

static alloc_data adata;

void* operator new(size_t x) {
	x = ((x + 7) >> 3) << 3;
	if (x < 32) x = 32;

    adata.bytes_alloced += x;
    adata.new_calls++;

    size_t* a = (size_t*) malloc(x + sizeof(size_t));
    if (a == nullptr) {
        throw std::bad_alloc();
    }

    *a = x;
    return (void*) ((ptrdiff_t) a + sizeof(size_t));
}
void operator delete(void* x) noexcept {
    adata.delete_calls++;
    size_t* a = (size_t*) ((ptrdiff_t) x - sizeof(size_t));
    adata.bytes_freed += *a;

    // free(a);
}

void* operator new(size_t x, std::nothrow_t&) noexcept
    { return operator new(x); }
void* operator new[](size_t x)
    { return operator new(x); }
void* operator new[](size_t x, std::nothrow_t&) noexcept
    { return operator new(x); }

void operator delete(void* x, std::nothrow_t&) noexcept
    { operator delete(x); }
void operator delete[](void* x) noexcept
    { operator delete(x); }
void operator delete[](void* x, std::nothrow_t&) noexcept
    { operator delete(x); }

