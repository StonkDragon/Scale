#define transpilerError(msg, at)         \
    FPResult err;                        \
    err.success = false;                 \
    err.line = body[(at)].getLine();     \
    err.column = body[(at)].getColumn(); \
    err.in = body[(at)].getFile();       \
    err.value = body[(at)].getValue();   \
    err.type = body[(at)].getType();     \
    err.message = msg
