#pragma once

#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#define transpilerError(msg, _at)          \
    FPResult err;                          \
    err.success = false;                   \
    err.line = body.at(_at).line;     \
    err.column = body.at(_at).column; \
    err.in = body.at(_at).file;       \
    err.value = body.at(_at).value;   \
    err.type = body.at(_at).type;     \
    err.message = msg

#define transpilerErrorTok(msg, tok) \
    FPResult err;                    \
    err.success = false;             \
    err.line = (tok).line;      \
    err.column = (tok).column;  \
    err.in = (tok).file;        \
    err.value = (tok).value;    \
    err.type = (tok).type;      \
    err.message = msg
#define LOAD_PATH(path, type)                                                                           \
    if (removeTypeModifiers(type) == "none" || removeTypeModifiers(type) == "nothing")                  \
    {                                                                                                   \
        append("%s;\n", path.c_str());                                                                  \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
        append("if (sizeof(%s) < 8) { (_stack.sp)->i = 0; }\n", sclTypeToCType(result, type).c_str());  \
        append("*(%s*) (_stack.sp++) = %s;\n", sclTypeToCType(result, type).c_str(), path.c_str());     \
    }                                                                                                   \
    typeStack.push(type);

#define wasRepeat()     (whatWasIt.size() > 0 && whatWasIt.back() == 1)
#define popRepeat()     (whatWasIt.pop_back())
#define pushRepeat()    (whatWasIt.push_back(1))
#define wasCatch()      (whatWasIt.size() > 0 && whatWasIt.back() == 2)
#define popCatch()      (whatWasIt.pop_back())
#define pushCatch()     (whatWasIt.push_back(2))
#define wasIterate()    (whatWasIt.size() > 0 && whatWasIt.back() == 4)
#define popIterate()    (whatWasIt.pop_back())
#define pushIterate()   (whatWasIt.push_back(4))
#define wasTry()        (whatWasIt.size() > 0 && whatWasIt.back() == 8)
#define popTry()        (whatWasIt.pop_back())
#define pushTry()       (whatWasIt.push_back(8))
#define wasSwitch()     (whatWasIt.size() > 0 && whatWasIt.back() == 16)
#define popSwitch()     (whatWasIt.pop_back())
#define pushSwitch()    (whatWasIt.push_back(16))
#define wasCase()       (whatWasIt.size() > 0 && whatWasIt.back() == 32)
#define popCase()       (whatWasIt.pop_back())
#define pushCase()      (whatWasIt.push_back(32))
#define wasOther()      (whatWasIt.size() > 0 && whatWasIt.back() == 0)
#define popOther()      (whatWasIt.pop_back())
#define pushOther()     (whatWasIt.push_back(0))

#define varScopePush()                          \
    do                                          \
    {                                           \
        var_indices.push_back(vars.size());     \
    } while (0)
#define varScopePop()                           \
    do                                          \
    {                                           \
        vars.erase(vars.begin() + var_indices.back(), vars.end()); \
        var_indices.pop_back();                 \
    } while (0)
#define varScopeTop() vars
#define typeStackTop (typeStack.size() ? typeStack.top() : "")
#define handler(_tok) extern "C" void handle ## _tok (std::vector<Token>& body, Function* function, std::vector<FPResult>& errors, std::vector<FPResult>& warns, FILE* fp, TPResult& result)
#define handle(_tok) handle ## _tok (body, function, errors, warns, fp, result)
#define handlerRef(_tok) handle ## _tok
#define handleRef(ref) (ref)(body, function, errors, warns, fp, result)
#define noUnused \
        (void) body; \
        (void) function; \
        (void) errors; \
        (void) warns; \
        (void) fp; \
        (void) result
#define debugDump(_var) std::cout << __func__ << ":" << std::to_string(__LINE__) << ": " << #_var << ": " << _var << std::endl
#define typePop do { if (typeStack.size()) { typeStack.pop(); } } while (0)
#define safeInc(...) \
    do { \
        if (++i >= body.size()) { \
            __VA_OPT__(safeIncMsg("End of file: " __VA_ARGS__);) \
            std::cerr << body.back().file << ":" << body.back().line << ":" << body.back().column << ":" << Color::RED << " Unexpected end of file!" << std::endl; \
            std::raise(SIGSEGV); \
        } \
    } while (0)

#define incrementAndExpectWithNote(_condition, _err, _note, ...) \
    do { \
        safeInc(_err __VA_OPT__(,) __VA_ARGS__); \
        i--; \
        if (!(_condition)) { \
            { \
                transpilerError(_err, i); \
                errors.push_back(err); \
            } \
            transpilerError(_note, i); \
            err.isNote = true; \
            errors.push_back(err); \
            return __VA_ARGS__; \
        } \
        i++; \
    } while (0)

#define incrementAndExpect(_condition, _err, ...) \
    do { \
        safeInc(_err __VA_OPT__(,) __VA_ARGS__); \
        i--; \
        if (!(_condition)) { \
            transpilerError(_err, i); \
            errors.push_back(err); \
            return __VA_ARGS__; \
        } \
        i++; \
    } while (0)

#define expect(_condition, _err, ...) expectWithPos(_condition, _err, i, __VA_ARGS__)

#define expectWithPos(_condition, _err, _pos, ...) \
    do { \
        if (!(_condition)) { \
            transpilerError(_err, _pos); \
            errors.push_back(err); \
            return __VA_ARGS__; \
        } \
    } while (0)

#define warnIf(_condition, _err) \
    do { \
        if (_condition) { \
            transpilerError(_err, i); \
            warns.push_back(err); \
        } \
    } while (0)

#define safeIncMsg(_why, ...) i--; transpilerError(_why, i); i++; errors.push_back(err); return __VA_ARGS__
#define safeIncN(n) do { if ((i + n) >= body.size()) { std::cerr << body.back().file << ":" << body.back().line << ":" << body.back().column << ":" << Color::RED << " Unexpected end of file!" << std::endl; std::raise(SIGSEGV); } i += n; } while (0)
#define THIS_INCREMENT_IS_CHECKED ++i;

namespace sclc {
    handler(Identifier);
    handler(New);
    handler(Repeat);
    handler(For);
    handler(Foreach);
    handler(AddrRef);
    handler(Store);
    handler(Declare);
    handler(CurlyOpen);
    handler(BracketOpen);
    handler(ParenOpen);
    handler(StringLiteral);
    handler(CDecl);
    handler(IntegerLiteral);
    handler(FloatLiteral);
    handler(FalsyType);
    handler(TruthyType);
    handler(Is);
    handler(If);
    handler(Fi);
    handler(Unless);
    handler(Else);
    handler(Elif);
    handler(Elunless);
    handler(While);
    handler(Do);
    handler(DoneLike);
    handler(Return);
    handler(Goto);
    handler(Label);
    handler(Case);
    handler(Esac);
    handler(Default);
    handler(Switch);
    handler(AddrOf);
    handler(Break);
    handler(Continue);
    handler(As);
    handler(Dot);
    handler(Column);
    handler(Token);
    handler(Lambda);
} // namespace sclc
