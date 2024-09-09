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
#define LOAD_PATH(path, type)                                                                                                          \
    if (removeTypeModifiers(type) == "none" || removeTypeModifiers(type) == "nothing")                                                 \
    {                                                                                                                                  \
        append("%s;\n", path.c_str());                                                                                                 \
    }                                                                                                                                  \
    else if (type.front() == '@')                                                                                                      \
    {                                                                                                                                  \
        std::string ctype = sclTypeToCType(result, type);                                                                              \
        const Struct& s = getStructByName(result, type);                                                                               \
        append("_scl_push_value(%s, %s, %s);\n", ctype.c_str(), (s != Struct::Null ? "MEM_FLAG_INSTANCE" : "0"), path.c_str());        \
        typeStack.push_back(type.substr(1));                                                                                           \
    }                                                                                                                                  \
    else                                                                                                                               \
    {                                                                                                                                  \
        std::string ctype = sclTypeToCType(result, type);                                                                              \
        if (isPrimitiveIntegerType(type))                                                                                              \
        {                                                                                                                              \
            append("_scl_push(scl_int, %s);\n", path.c_str());                                                                         \
        }                                                                                                                              \
        else                                                                                                                           \
        {                                                                                                                              \
            append("_scl_push(%s, %s);\n", ctype.c_str(), path.c_str());                                                               \
        }                                                                                                                              \
        typeStack.push_back(type);                                                                                                     \
    }

#define wasRepeat() (whatWasIt.size() > 0 && whatWasIt.back() == 1)
#define popRepeat() (whatWasIt.pop_back())
#define pushRepeat() (whatWasIt.push_back(1))
#define wasCatch() (whatWasIt.size() > 0 && whatWasIt.back() == 2)
#define popCatch() (whatWasIt.pop_back())
#define pushCatch() (whatWasIt.push_back(2))
#define wasIterate() (whatWasIt.size() > 0 && whatWasIt.back() == 4)
#define popIterate() (whatWasIt.pop_back())
#define pushIterate() (whatWasIt.push_back(4))
#define wasTry() (whatWasIt.size() > 0 && whatWasIt.back() == 8)
#define popTry() (whatWasIt.pop_back())
#define pushTry() (whatWasIt.push_back(8))
#define wasSwitch() (whatWasIt.size() > 0 && whatWasIt.back() == 16)
#define popSwitch() (whatWasIt.pop_back())
#define pushSwitch() (whatWasIt.push_back(16))
#define wasCase() (whatWasIt.size() > 0 && whatWasIt.back() == 32)
#define popCase() (whatWasIt.pop_back())
#define pushCase() (whatWasIt.push_back(32))
#define wasSwitch2() (whatWasIt.size() > 0 && whatWasIt.back() == 64)
#define popSwitch2() (whatWasIt.pop_back())
#define pushSwitch2() (whatWasIt.push_back(64))
#define wasUsing() (whatWasIt.size() > 0 && whatWasIt.back() == 128)
#define popUsing() (whatWasIt.pop_back())
#define pushUsing() (whatWasIt.push_back(128))
#define wasOther() (whatWasIt.size() > 0 && whatWasIt.back() == 0)
#define popOther() (whatWasIt.pop_back())
#define pushOther() (whatWasIt.push_back(0))

#define varScopePush() var_indices.push_back(vars.size())
#define varScopePop()                                              \
    do                                                             \
    {                                                              \
        vars.erase(vars.begin() + var_indices.back(), vars.end()); \
        var_indices.pop_back();                                    \
    } while (0)
#define typeStackTop (typeStack.size() ? typeStack.back() : "")
#define handler(_tok) extern "C" void handle##_tok(std::vector<Token> &body, Function *function, std::vector<FPResult> &errors, std::vector<FPResult> &warns, std::ostream &fp, TPResult &result, size_t& i)
#define handle(_tok) handle##_tok(body, function, errors, warns, fp, result, i)
#define handlerRef(_tok) (&handle##_tok)
#define handleRef(ref)                                       \
    if (ref)                                                 \
    {                                                        \
        (ref)(body, function, errors, warns, fp, result, i); \
    }                                                        \
    else                                                     \
    {                                                        \
        transpilerError("Unexpected token:", i);             \
        errors.push_back(err);                               \
    }
#define noUnused    \
    (void)body;     \
    (void)function; \
    (void)errors;   \
    (void)warns;    \
    (void)fp;       \
    (void)result;   \
    (void)i
#define debugDump(_var) std::cout << __func__ << ":" << std::to_string(__LINE__) << ": " << #_var << ": " << _var << std::endl
#define typePop                   \
    do                            \
    {                             \
        if (typeStack.size())     \
        {                         \
            typeStack.pop_back(); \
        }                         \
    } while (0)
#define safeInc(...)                                                     \
    do                                                                   \
    {                                                                    \
        if (++i >= body.size())                                          \
        {                                                                \
            transpilerError("Unexpected end of file!", body.size() - 1); \
            errors.push_back(err);                                       \
            return __VA_ARGS__;                                          \
        }                                                                \
    } while (0)

#define safeIncN(n, ...)                                                 \
    do                                                                   \
    {                                                                    \
        if ((i + n) >= body.size())                                      \
        {                                                                \
            transpilerError("Unexpected end of file!", body.size() - 1); \
            errors.push_back(err);                                       \
            return __VA_ARGS__;                                          \
        }                                                                \
        i += n;                                                          \
    } while (0)

#define THIS_INCREMENT_IS_CHECKED ++i;

namespace sclc
{
    extern std::vector<std::string> cflags;
    extern std::vector<std::string> modes;
    extern Function* currentFunction;
    extern Struct currentStruct;
    extern std::unordered_map<std::string, std::vector<Method *>> vtables;
    extern StructTreeNode *structTree;
    extern int scopeDepth;
    extern size_t condCount;
    extern std::vector<short> whatWasIt;
    extern std::vector<std::string> switchTypes;
    extern std::vector<std::string> usingNames;
    extern std::unordered_map<std::string, std::vector<std::string>> usingStructs;
    extern char repeat_depth;
    extern int iterator_count;
    extern int isInUnsafe;
    extern std::vector<std::string> typeStack;
    extern std::string return_type;
    extern int lambdaCount;

    handler(Await);
    handler(Typeof);
    handler(Typeid);
    handler(Nameof);
    handler(Sizeof);
    handler(Try);
    handler(Unsafe);
    handler(Assert);
    handler(Varargs);
    handler(Token);
    handler(Pragma);
    handler(Lambda);
    handler(Catch);
    handler(Using);
    handler(ReturnOnNil);
    handler(ColumnOnInterface);
    handler(Identifier);
    handler(Dot);
    handler(Column);
    handler(As);
    handler(StringLiteral);
    handler(CharStringLiteral);
    handler(CDecl);
    handler(ExternC);
    handler(IntegerLiteral);
    handler(CharLiteral);
    handler(FloatLiteral);
    handler(FalsyType);
    handler(FalsyType);
    handler(TruthyType);
    handler(New);
    handler(Is);
    handler(If);
    handler(Unless);
    handler(Else);
    handler(Elif);
    handler(Elunless);
    handler(While);
    handler(Until);
    handler(Do);
    handler(Repeat);
    handler(For);
    handler(Foreach);
    handler(Fi);
    handler(DoneLike);
    handler(Return);
    handler(AddrRef);
    handler(Store);
    handler(Declare);
    handler(Continue);
    handler(Break);
    handler(Goto);
    handler(Label);
    handler(Case);
    handler(Esac);
    handler(Default);
    handler(Switch);
    handler(CurlyOpen);
    handler(BracketOpen);
    handler(ParenOpen);
    handler(Identifier);
    handler(StackOp);
} // namespace sclc
