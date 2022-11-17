#define transpilerError(msg, at) \
    FPResult err; \
    err.success = false; \
    err.message = msg; \
    err.column = body[(at)].getColumn(); \
    err.line = body[(at)].getLine(); \
    err.in = body[(at)].getFile(); \
    err.value = body[(at)].getValue(); \
    err.type =  body[(at)].getType(); \
    errors.push_back(err)