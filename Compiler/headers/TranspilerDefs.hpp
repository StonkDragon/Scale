#define transpilerError(msg, _at)          \
    FPResult err;                          \
    err.success = false;                   \
    err.line = body.at(_at).getLine();     \
    err.column = body.at(_at).getColumn(); \
    err.in = body.at(_at).getFile();       \
    err.value = body.at(_at).getValue();   \
    err.type = body.at(_at).getType();     \
    err.message = msg

#define transpilerErrorTok(msg, tok) \
    FPResult err;                    \
    err.success = false;             \
    err.line = (tok).getLine();      \
    err.column = (tok).getColumn();  \
    err.in = (tok).getFile();        \
    err.value = (tok).getValue();    \
    err.type = (tok).getType();      \
    err.message = msg
