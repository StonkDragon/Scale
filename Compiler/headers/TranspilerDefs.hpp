#define transpilerError(msg, at)         \
    FPResult err;                        \
    err.success = false;                 \
    err.message = msg;                   \
    err.column = body[(at)].getColumn(); \
    err.line = body[(at)].getLine();     \
    err.in = body[(at)].getFile();       \
    err.value = body[(at)].getValue();   \
    err.type = body[(at)].getType()

#define debugPrintPush() \
    if (Main.options.debugBuild) append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");

#define push_var()                                                                                                                                                                                \
    if (hasContainer(result, body[i]))                                                                                                                                                            \
    {                                                                                                                                                                                             \
        std::string containerName = body[i].getValue();                                                                                                                                           \
        ITER_INC;                                                                                                                                                                                 \
        if (body[i].getType() != tok_dot)                                                                                                                                                         \
        {                                                                                                                                                                                         \
            transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);                                                                                \
            errors.push_back(err);                                                                                                                                                                \
            continue;                                                                                                                                                                             \
        }                                                                                                                                                                                         \
        ITER_INC;                                                                                                                                                                                 \
        std::string memberName = body[i].getValue();                                                                                                                                              \
        Container container = getContainerByName(result, containerName);                                                                                                                          \
        if (!container.hasMember(memberName))                                                                                                                                                     \
        {                                                                                                                                                                                         \
            transpilerError("Unknown container member: '" + memberName + "'", i);                                                                                                                 \
            errors.push_back(err);                                                                                                                                                                \
            continue;                                                                                                                                                                             \
        }                                                                                                                                                                                         \
        if (container.getMemberType(memberName) == "float")                                                                                                                                       \
        {                                                                                                                                                                                         \
            append("stack.data[stack.ptr++].f = Container_%s.%s;\n", containerName.c_str(), memberName.c_str());                                                                                  \
            if (Main.options.debugBuild)                                                                                                                                                          \
                append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                                  \
        }                                                                                                                                                                                         \
        else                                                                                                                                                                                      \
        {                                                                                                                                                                                         \
            append("stack.data[stack.ptr++].v = (scl_value) Container_%s.%s;\n", containerName.c_str(), memberName.c_str());                                                                      \
            if (Main.options.debugBuild)                                                                                                                                                          \
                append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                                  \
        }                                                                                                                                                                                         \
    }                                                                                                                                                                                             \
    else if (getStructByName(result, body[i].getValue()) != Struct(""))                                                                                                                           \
    {                                                                                                                                                                                             \
        if (body[i + 1].getType() == tok_column)                                                                                                                                                  \
        {                                                                                                                                                                                         \
            ITER_INC;                                                                                                                                                                             \
            ITER_INC;                                                                                                                                                                             \
            if (body[i].getType() == tok_new)                                                                                                                                                     \
            {                                                                                                                                                                                     \
                std::string struct_ = body[i - 2].getValue();                                                                                                                                     \
                append("stack.data[stack.ptr++].v = scl_alloc_struct(sizeof(struct Struct_%s), \"%s\");\n", struct_.c_str(), struct_.c_str());                                                    \
                if (Main.options.debugBuild)                                                                                                                                                      \
                    append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                              \
                if (hasMethod(result, Token(tok_identifier, "init", 0, "", 0), struct_))                                                                                                          \
                {                                                                                                                                                                                 \
                    append("{\n");                                                                                                                                                                \
                    scopeDepth++;                                                                                                                                                                 \
                    Method *f = getMethodByName(result, "init", struct_);                                                                                                                         \
                    append("struct Struct_%s* tmp = (struct Struct_%s*) stack.data[--stack.ptr].v;\n", ((Method *)(f))->getMemberType().c_str(), ((Method *)(f))->getMemberType().c_str());       \
                    append("stack.data[stack.ptr++].v = tmp;\n");                                                                                                                                 \
                    if (Main.options.debugBuild)                                                                                                                                                  \
                        append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                          \
                    append("stack.ptr -= %zu;\n", f->getArgs().size());                                                                                                                           \
                    if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                            \
                    {                                                                                                                                                                             \
                        if (f->getReturnType() == "float")                                                                                                                                        \
                        {                                                                                                                                                                         \
                            append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());             \
                            if (Main.options.debugBuild)                                                                                                                                          \
                                append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                  \
                        }                                                                                                                                                                         \
                        else                                                                                                                                                                      \
                        {                                                                                                                                                                         \
                            append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str()); \
                            if (Main.options.debugBuild)                                                                                                                                          \
                                append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                  \
                        }                                                                                                                                                                         \
                    }                                                                                                                                                                             \
                    else                                                                                                                                                                          \
                    {                                                                                                                                                                             \
                        append("Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());                                             \
                    }                                                                                                                                                                             \
                    append("stack.data[stack.ptr++].v = tmp;\n");                                                                                                                                 \
                    if (Main.options.debugBuild)                                                                                                                                                  \
                        append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                          \
                    scopeDepth--;                                                                                                                                                                 \
                    append("}\n");                                                                                                                                                                \
                }                                                                                                                                                                                 \
            }                                                                                                                                                                                     \
            else                                                                                                                                                                                  \
            {                                                                                                                                                                                     \
                if (!hasMethod(result, body[i], body[i - 2].getValue()))                                                                                                                          \
                {                                                                                                                                                                                 \
                    transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + body[i - 2].getValue() + "'", i);                                                                   \
                    errors.push_back(err);                                                                                                                                                        \
                    continue;                                                                                                                                                                     \
                }                                                                                                                                                                                 \
                Method *f = getMethodByName(result, body[i].getValue(), body[i - 2].getValue());                                                                                                  \
                append("stack.ptr -= %zu;\n", f->getArgs().size());                                                                                                                               \
                if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                                \
                {                                                                                                                                                                                 \
                    if (f->getReturnType() == "float")                                                                                                                                            \
                    {                                                                                                                                                                             \
                        append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());                 \
                        if (Main.options.debugBuild)                                                                                                                                              \
                            append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                      \
                    }                                                                                                                                                                             \
                    else                                                                                                                                                                          \
                    {                                                                                                                                                                             \
                        append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());     \
                        if (Main.options.debugBuild)                                                                                                                                              \
                            append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                      \
                    }                                                                                                                                                                             \
                }                                                                                                                                                                                 \
                else                                                                                                                                                                              \
                {                                                                                                                                                                                 \
                    append("Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());                                                 \
                }                                                                                                                                                                                 \
            }                                                                                                                                                                                     \
        }                                                                                                                                                                                         \
        else                                                                                                                                                                                      \
        {                                                                                                                                                                                         \
            ITER_INC;                                                                                                                                                                             \
            ITER_INC;                                                                                                                                                                             \
            Struct s = getStructByName(result, body[i - 2].getValue());                                                                                                                           \
            if (!s.hasMember(body[i].getValue()))                                                                                                                                                 \
            {                                                                                                                                                                                     \
                transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);                                                                              \
                errors.push_back(err);                                                                                                                                                            \
                continue;                                                                                                                                                                         \
            }                                                                                                                                                                                     \
            Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8];                                                                                                               \
            if (mem.getType() == "float")                                                                                                                                                         \
                append("stack.data[stack.ptr - 1].f = ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", s.getName().c_str(), body[i].getValue().c_str());                                \
            else                                                                                                                                                                                  \
                append("stack.data[stack.ptr - 1].v = (scl_value) ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", s.getName().c_str(), body[i].getValue().c_str());                    \
        }                                                                                                                                                                                         \
    }                                                                                                                                                                                             \
    else if (hasVar(body[i]))                                                                                                                                                                     \
    {                                                                                                                                                                                             \
        std::string loadFrom = body[i].getValue();                                                                                                                                                \
        Variable v = getVar(body[i]);                                                                                                                                                             \
        if (getStructByName(result, v.getType()) != Struct(""))                                                                                                                                   \
        {                                                                                                                                                                                         \
            if (body[i + 1].getType() == tok_column)                                                                                                                                              \
            {                                                                                                                                                                                     \
                ITER_INC;                                                                                                                                                                         \
                ITER_INC;                                                                                                                                                                         \
                if (!hasMethod(result, body[i], v.getType()))                                                                                                                                     \
                {                                                                                                                                                                                 \
                    transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + v.getType() + "'", i);                                                                              \
                    errors.push_back(err);                                                                                                                                                        \
                    continue;                                                                                                                                                                     \
                }                                                                                                                                                                                 \
                Method *f = getMethodByName(result, body[i].getValue(), v.getType());                                                                                                             \
                append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());                                                                                                    \
                append("stack.ptr -= %zu;\n", f->getArgs().size());                                                                                                                               \
                if (Main.options.debugBuild)                                                                                                                                                      \
                    append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                              \
                if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                                \
                {                                                                                                                                                                                 \
                    if (f->getReturnType() == "float")                                                                                                                                            \
                    {                                                                                                                                                                             \
                        append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());                 \
                        if (Main.options.debugBuild)                                                                                                                                              \
                            append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                      \
                    }                                                                                                                                                                             \
                    else                                                                                                                                                                          \
                    {                                                                                                                                                                             \
                        append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());     \
                        if (Main.options.debugBuild)                                                                                                                                              \
                            append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                      \
                    }                                                                                                                                                                             \
                }                                                                                                                                                                                 \
                else                                                                                                                                                                              \
                {                                                                                                                                                                                 \
                    append("Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());                                                 \
                }                                                                                                                                                                                 \
            }                                                                                                                                                                                     \
            else                                                                                                                                                                                  \
            {                                                                                                                                                                                     \
                if (body[i + 1].getType() != tok_dot)                                                                                                                                             \
                {                                                                                                                                                                                 \
                    append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());                                                                                                \
                    if (Main.options.debugBuild)                                                                                                                                                  \
                        append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                          \
                    continue;                                                                                                                                                                     \
                }                                                                                                                                                                                 \
                ITER_INC;                                                                                                                                                                         \
                ITER_INC;                                                                                                                                                                         \
                Struct s = getStructByName(result, v.getType());                                                                                                                                  \
                if (!s.hasMember(body[i].getValue()))                                                                                                                                             \
                {                                                                                                                                                                                 \
                    transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);                                                                          \
                    errors.push_back(err);                                                                                                                                                        \
                    continue;                                                                                                                                                                     \
                }                                                                                                                                                                                 \
                Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8];                                                                                                           \
                if (mem.getType() == "float")                                                                                                                                                     \
                {                                                                                                                                                                                 \
                    append("stack.data[stack.ptr++].f = Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());                                                                            \
                    if (Main.options.debugBuild)                                                                                                                                                  \
                        append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                          \
                }                                                                                                                                                                                 \
                else                                                                                                                                                                              \
                {                                                                                                                                                                                 \
                    append("stack.data[stack.ptr++].v = (scl_value) Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());                                                                \
                    if (Main.options.debugBuild)                                                                                                                                                  \
                        append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                          \
                }                                                                                                                                                                                 \
            }                                                                                                                                                                                     \
        }                                                                                                                                                                                         \
        else                                                                                                                                                                                      \
        {                                                                                                                                                                                         \
            if (v.getType() == "float")                                                                                                                                                           \
            {                                                                                                                                                                                     \
                append("stack.data[stack.ptr++].f = Var_%s;\n", loadFrom.c_str());                                                                                                                \
                if (Main.options.debugBuild)                                                                                                                                                      \
                    append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                              \
            }                                                                                                                                                                                     \
            else                                                                                                                                                                                  \
            {                                                                                                                                                                                     \
                append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());                                                                                                    \
                if (Main.options.debugBuild)                                                                                                                                                      \
                    append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");                                                                                              \
            }                                                                                                                                                                                     \
        }                                                                                                                                                                                         \
    }                                                                                                                                                                                             \
    else if (body[i].getType() == tok_number)                                                                                                                                                     \
    {                                                                                                                                                                                             \
        FPResult res = handleNumber(fp, body[i], scopeDepth);                                                                                                                                     \
        if (!res.success)                                                                                                                                                                         \
        {                                                                                                                                                                                         \
            errors.push_back(res);                                                                                                                                                                \
            continue;                                                                                                                                                                             \
        }                                                                                                                                                                                         \
    }
