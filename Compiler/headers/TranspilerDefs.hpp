#define transpilerError(msg, at)         \
    FPResult err;                        \
    err.success = false;                 \
    err.line = body[(at)].getLine();     \
    err.column = body[(at)].getColumn(); \
    err.in = body[(at)].getFile();       \
    err.value = body[(at)].getValue();   \
    err.type = body[(at)].getType();     \
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
