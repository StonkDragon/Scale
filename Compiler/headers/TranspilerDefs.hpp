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
