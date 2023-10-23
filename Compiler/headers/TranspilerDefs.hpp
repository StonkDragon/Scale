#pragma once
#define transpilerError(msg, _at)         \
    FPResult err;                         \
    err.success = false;                  \
    err.location = body.at(_at).location; \
    err.value = body.at(_at).value;       \
    err.type = body.at(_at).type;         \
    err.message = msg

#define transpilerErrorTok(msg, tok) \
    FPResult err;                    \
    err.success = false;             \
    err.location = (tok).location;   \
    err.value = (tok).value;         \
    err.type = (tok).type;           \
    err.message = msg
#define LOAD_PATH(path, type)                                                          \
    if (removeTypeModifiers(type) == "none" || removeTypeModifiers(type) == "nothing") \
    {                                                                                  \
        append("%s;\n", path.c_str());                                                 \
    }                                                                                  \
    else if (type.front() == '*')                                                      \
    {                                                                                  \
        append("{\n");                                                                 \
        scopeDepth++;                                                                  \
        append("%s tmp = %s;\n", sclTypeToCType(result, type).c_str(), path.c_str());  \
        append("(localstack++)->v = _scl_alloc_struct(tmp.$statics);\n");              \
        append("memcpy((localstack - 1)->v, &tmp, tmp.$statics->size);\n");            \
        scopeDepth--;                                                                  \
        append("}\n");                                                                 \
    }                                                                                  \
    else                                                                               \
    {                                                                                  \
        std::string ctype = sclTypeToCType(result, type);                              \
        if (isPrimitiveIntegerType(type)) {                                            \
            append("*(scl_uint*) (localstack) = %s;\n", path.c_str());                 \
        } else {                                                                       \
            append("*(%s*) (localstack) = %s;\n", ctype.c_str(), path.c_str());        \
        }                                                                              \
        append("localstack++;\n");                                                     \
    }                                                                                  \
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
#define safeInc() do { if (++i >= body.size()) { std::cerr << body.back().location.file << ":" << body.back().location.line << ":" << body.back().location.column << ":" << Color::RED << " Unexpected end of file!" << std::endl; std::raise(SIGSEGV); } } while (0)
#define safeIncN(n) do { if ((i + n) >= body.size()) { std::cerr << body.back().location.file << ":" << body.back().location.line << ":" << body.back().location.column << ":" << Color::RED << " Unexpected end of file!" << std::endl; std::raise(SIGSEGV); } i += n; } while (0)
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