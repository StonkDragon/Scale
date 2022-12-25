#define transpilerError(msg, at)         \
    FPResult err;                        \
    err.success = false;                 \
    err.message = msg;                   \
    err.line = body[(at)].getLine();     \
    err.in = body[(at)].getFile();       \
    err.value = body[(at)].getValue();   \
    err.type = body[(at)].getType()

#define debugPrintPush()         \
    if (Main.options.debugBuild) \
        append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");
