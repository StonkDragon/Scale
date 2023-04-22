#define transpilerError(msg, at)         \
    FPResult err;                        \
    err.success = false;                 \
    err.line = body[(at)].getLine();     \
    err.column = body[(at)].getColumn(); \
    err.in = body[(at)].getFile();       \
    err.value = body[(at)].getValue();   \
    err.type = body[(at)].getType();     \
    std::cout << "TRanspiler error in function " << __func__ << " in line " << __LINE__ << std::endl; \
    err.message = msg

#define resetFile()                                                                          \
    do                                                                                       \
    {                                                                                        \
        if (Main.options.debugBuild)                                                         \
        {                                                                                    \
            std::string file = body[i].getFile();                                            \
            if (strstarts(file, scaleFolder))                                                \
            {                                                                                \
                file = file.substr(scaleFolder.size() + std::string("/Frameworks/").size()); \
            }                                                                                \
            else                                                                             \
            {                                                                                \
                file = std::filesystem::path(file).relative_path();                          \
            }                                                                                \
            append("current_file[callstk.ptr - 1] = \"%s\";\n", file.c_str());               \
            append("current_line[callstk.ptr - 1] = %d;\n", body[i].getLine());              \
            append("current_col[callstk.ptr - 1] = %d;\n", body[i].getColumn());             \
            currentFile = file;                                                              \
            currentLine = body[i].getLine();                                                 \
            currentCol = body[i].getColumn();                                                \
        }                                                                                    \
    } while (0)
