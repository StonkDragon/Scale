#define transpilerError(msg, at)         \
    FPResult err;                        \
    err.success = false;                 \
    err.message = msg;                   \
    err.column = body[(at)].getColumn(); \
    err.line = body[(at)].getLine();     \
    err.in = body[(at)].getFile();       \
    err.value = body[(at)].getValue();   \
    err.type = body[(at)].getType()

#define debugPrintPush()         \
    if (Main.options.debugBuild) \
        append("fprintf(stderr, \"Pushed: %%lld\\n\", stack.data[stack.ptr - 1].i);\n");

#define push_var()                                                                                                                                                                                    \
    if (isOperator(body[i]))                                                                                                                                                                          \
    {                                                                                                                                                                                                 \
        FPResult operatorsHandled = handleOperator(fp, body[i], scopeDepth);                                                                                                                          \
        if (!operatorsHandled.success)                                                                                                                                                                \
        {                                                                                                                                                                                             \
            errors.push_back(operatorsHandled);                                                                                                                                                       \
        }                                                                                                                                                                                             \
    }                                                                                                                                                                                                 \
    else if (body[i].getType() == tok_identifier)                                                                                                                                                     \
    {                                                                                                                                                                                                 \
        if (body[i].getValue() == "self" && !function->isMethod)                                                                                                                                      \
        {                                                                                                                                                                                             \
            transpilerError("Can't use 'self' outside a member function.", i);                                                                                                                        \
            errors.push_back(err);                                                                                                                                                                    \
            continue;                                                                                                                                                                                 \
        }                                                                                                                                                                                             \
        if (body[i].getValue() == "drop")                                                                                                                                                             \
        {                                                                                                                                                                                             \
            append("stack.ptr--;\n");                                                                                                                                                                 \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "dup")                                                                                                                                                         \
        {                                                                                                                                                                                             \
            append("stack.data[stack.ptr++].v = stack.data[stack.ptr - 1].v;\n");                                                                                                                     \
            debugPrintPush();                                                                                                                                                                         \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "swap")                                                                                                                                                        \
        {                                                                                                                                                                                             \
            append("{\n");                                                                                                                                                                            \
            scopeDepth++;                                                                                                                                                                             \
            append("scl_value b = stack.data[--stack.ptr].v;\n");                                                                                                                                     \
            append("scl_value a = stack.data[--stack.ptr].v;\n");                                                                                                                                     \
            append("stack.data[stack.ptr++].v = b;\n");                                                                                                                                               \
            debugPrintPush();                                                                                                                                                                         \
            append("stack.data[stack.ptr++].v = a;\n");                                                                                                                                               \
            debugPrintPush();                                                                                                                                                                         \
            scopeDepth--;                                                                                                                                                                             \
            append("}\n");                                                                                                                                                                            \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "over")                                                                                                                                                        \
        {                                                                                                                                                                                             \
            append("{\n");                                                                                                                                                                            \
            scopeDepth++;                                                                                                                                                                             \
            append("scl_value c = stack.data[--stack.ptr].v;\n");                                                                                                                                     \
            append("scl_value b = stack.data[--stack.ptr].v;\n");                                                                                                                                     \
            append("scl_value a = stack.data[--stack.ptr].v;\n");                                                                                                                                     \
            append("stack.data[stack.ptr++].v = c;\n");                                                                                                                                               \
            debugPrintPush();                                                                                                                                                                         \
            append("stack.data[stack.ptr++].v = b;\n");                                                                                                                                               \
            debugPrintPush();                                                                                                                                                                         \
            append("stack.data[stack.ptr++].v = a;\n");                                                                                                                                               \
            debugPrintPush();                                                                                                                                                                         \
            scopeDepth--;                                                                                                                                                                             \
            append("}\n");                                                                                                                                                                            \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "sdup2")                                                                                                                                                       \
        {                                                                                                                                                                                             \
            append("{\n");                                                                                                                                                                            \
            scopeDepth++;                                                                                                                                                                             \
            append("scl_value b = stack.data[--stack.ptr].v;\n");                                                                                                                                     \
            append("scl_value a = stack.data[--stack.ptr].v;\n");                                                                                                                                     \
            append("stack.data[stack.ptr++].v = a;\n");                                                                                                                                               \
            debugPrintPush();                                                                                                                                                                         \
            append("stack.data[stack.ptr++].v = b;\n");                                                                                                                                               \
            debugPrintPush();                                                                                                                                                                         \
            append("stack.data[stack.ptr++].v = a;\n");                                                                                                                                               \
            debugPrintPush();                                                                                                                                                                         \
            scopeDepth--;                                                                                                                                                                             \
            append("}\n");                                                                                                                                                                            \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "swap2")                                                                                                                                                       \
        {                                                                                                                                                                                             \
            append("{\n");                                                                                                                                                                            \
            scopeDepth++;                                                                                                                                                                             \
            append("scl_value c = stack.data[--stack.ptr].v;\n");                                                                                                                                     \
            append("scl_value b = stack.data[--stack.ptr].v;\n");                                                                                                                                     \
            append("scl_value a = stack.data[--stack.ptr].v;\n");                                                                                                                                     \
            append("stack.data[stack.ptr++].v = b;\n");                                                                                                                                               \
            debugPrintPush();                                                                                                                                                                         \
            append("stack.data[stack.ptr++].v = a;\n");                                                                                                                                               \
            debugPrintPush();                                                                                                                                                                         \
            append("stack.data[stack.ptr++].v = c;\n");                                                                                                                                               \
            debugPrintPush();                                                                                                                                                                         \
            scopeDepth--;                                                                                                                                                                             \
            append("}\n");                                                                                                                                                                            \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "clearstack")                                                                                                                                                  \
        {                                                                                                                                                                                             \
            append("stack.ptr = 0;\n");                                                                                                                                                               \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "&&")                                                                                                                                                          \
        {                                                                                                                                                                                             \
            append("stack.ptr -= 2;\n");                                                                                                                                                              \
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i && stack.data[stack.ptr + 1].i;\n");                                                                                          \
            debugPrintPush();                                                                                                                                                                         \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "!")                                                                                                                                                           \
        {                                                                                                                                                                                             \
            append("stack.data[stack.ptr - 1].i = !stack.data[stack.ptr - 1].i;\n");                                                                                                                  \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "||")                                                                                                                                                          \
        {                                                                                                                                                                                             \
            append("stack.ptr -= 2;\n");                                                                                                                                                              \
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i || stack.data[stack.ptr + 1].i;\n");                                                                                          \
            debugPrintPush();                                                                                                                                                                         \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "<")                                                                                                                                                           \
        {                                                                                                                                                                                             \
            append("stack.ptr -= 2;\n");                                                                                                                                                              \
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i < stack.data[stack.ptr + 1].i;\n");                                                                                           \
            debugPrintPush();                                                                                                                                                                         \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == ">")                                                                                                                                                           \
        {                                                                                                                                                                                             \
            append("stack.ptr -= 2;\n");                                                                                                                                                              \
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i > stack.data[stack.ptr + 1].i;\n");                                                                                           \
            debugPrintPush();                                                                                                                                                                         \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "==")                                                                                                                                                          \
        {                                                                                                                                                                                             \
            append("stack.ptr -= 2;\n");                                                                                                                                                              \
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i == stack.data[stack.ptr + 1].i;\n");                                                                                          \
            debugPrintPush();                                                                                                                                                                         \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "<=")                                                                                                                                                          \
        {                                                                                                                                                                                             \
            append("stack.ptr -= 2;\n");                                                                                                                                                              \
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i <= stack.data[stack.ptr + 1].i;\n");                                                                                          \
            debugPrintPush();                                                                                                                                                                         \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == ">=")                                                                                                                                                          \
        {                                                                                                                                                                                             \
            append("stack.ptr -= 2;\n");                                                                                                                                                              \
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i >= stack.data[stack.ptr + 1].i;\n");                                                                                          \
            debugPrintPush();                                                                                                                                                                         \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "!=")                                                                                                                                                          \
        {                                                                                                                                                                                             \
            append("stack.ptr -= 2;\n");                                                                                                                                                              \
            append("stack.data[stack.ptr++].i = stack.data[stack.ptr].i != stack.data[stack.ptr + 1].i;\n");                                                                                          \
            debugPrintPush();                                                                                                                                                                         \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "++")                                                                                                                                                          \
        {                                                                                                                                                                                             \
            append("stack.data[stack.ptr - 1].i++;\n");                                                                                                                                               \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "--")                                                                                                                                                          \
        {                                                                                                                                                                                             \
            append("stack.data[stack.ptr - 1].i--;\n");                                                                                                                                               \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "exit")                                                                                                                                                        \
        {                                                                                                                                                                                             \
            append("exit(stack.data[--stack.ptr].i);\n");                                                                                                                                             \
        }                                                                                                                                                                                             \
        else if (body[i].getValue() == "abort")                                                                                                                                                       \
        {                                                                                                                                                                                             \
            append("abort();\n");                                                                                                                                                                     \
        }                                                                                                                                                                                             \
        else if (hasFunction(result, body[i]))                                                                                                                                                        \
        {                                                                                                                                                                                             \
            Function *f = getFunctionByName(result, body[i].getValue());                                                                                                                              \
            if (f->isMethod)                                                                                                                                                                          \
            {                                                                                                                                                                                         \
                transpilerError("'" + f->getName() + "' is a method, not a function.", i);                                                                                                            \
                errors.push_back(err);                                                                                                                                                                \
                continue;                                                                                                                                                                             \
            }                                                                                                                                                                                         \
            append("stack.ptr -= %zu;\n", f->getArgs().size());                                                                                                                                       \
            if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                                        \
            {                                                                                                                                                                                         \
                if (f->getReturnType() == "float")                                                                                                                                                    \
                {                                                                                                                                                                                     \
                    append("stack.data[stack.ptr++].f = Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());                                                                    \
                    debugPrintPush();                                                                                                                                                                 \
                }                                                                                                                                                                                     \
                else                                                                                                                                                                                  \
                {                                                                                                                                                                                     \
                    append("stack.data[stack.ptr++].v = (scl_value) Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());                                                        \
                    debugPrintPush();                                                                                                                                                                 \
                }                                                                                                                                                                                     \
            }                                                                                                                                                                                         \
            else                                                                                                                                                                                      \
            {                                                                                                                                                                                         \
                append("Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());                                                                                                    \
            }                                                                                                                                                                                         \
        }                                                                                                                                                                                             \
        else if (hasContainer(result, body[i]))                                                                                                                                                       \
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
                debugPrintPush();                                                                                                                                                                     \
            }                                                                                                                                                                                         \
            else                                                                                                                                                                                      \
            {                                                                                                                                                                                         \
                append("stack.data[stack.ptr++].v = (scl_value) Container_%s.%s;\n", containerName.c_str(), memberName.c_str());                                                                      \
                debugPrintPush();                                                                                                                                                                     \
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
                    debugPrintPush();                                                                                                                                                                 \
                    if (hasMethod(result, Token(tok_identifier, "init", 0, "", 0), struct_))                                                                                                          \
                    {                                                                                                                                                                                 \
                        append("{\n");                                                                                                                                                                \
                        scopeDepth++;                                                                                                                                                                 \
                        Method *f = getMethodByName(result, "init", struct_);                                                                                                                         \
                        append("struct Struct_%s* tmp = (struct Struct_%s*) stack.data[--stack.ptr].v;\n", ((Method *)(f))->getMemberType().c_str(), ((Method *)(f))->getMemberType().c_str());       \
                        append("stack.data[stack.ptr++].v = tmp;\n");                                                                                                                                 \
                        debugPrintPush();                                                                                                                                                             \
                        append("stack.ptr -= %zu;\n", f->getArgs().size());                                                                                                                           \
                        if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                            \
                        {                                                                                                                                                                             \
                            if (f->getReturnType() == "float")                                                                                                                                        \
                            {                                                                                                                                                                         \
                                append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());             \
                                debugPrintPush();                                                                                                                                                     \
                            }                                                                                                                                                                         \
                            else                                                                                                                                                                      \
                            {                                                                                                                                                                         \
                                append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str()); \
                                debugPrintPush();                                                                                                                                                     \
                            }                                                                                                                                                                         \
                        }                                                                                                                                                                             \
                        else                                                                                                                                                                          \
                        {                                                                                                                                                                             \
                            append("Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());                                             \
                        }                                                                                                                                                                             \
                        append("stack.data[stack.ptr++].v = tmp;\n");                                                                                                                                 \
                        debugPrintPush();                                                                                                                                                             \
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
                            debugPrintPush();                                                                                                                                                         \
                        }                                                                                                                                                                             \
                        else                                                                                                                                                                          \
                        {                                                                                                                                                                             \
                            append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());     \
                            debugPrintPush();                                                                                                                                                         \
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
                    debugPrintPush();                                                                                                                                                                 \
                    if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                                \
                    {                                                                                                                                                                                 \
                        if (f->getReturnType() == "float")                                                                                                                                            \
                        {                                                                                                                                                                             \
                            append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());                 \
                            debugPrintPush();                                                                                                                                                         \
                        }                                                                                                                                                                             \
                        else                                                                                                                                                                          \
                        {                                                                                                                                                                             \
                            append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());     \
                            debugPrintPush();                                                                                                                                                         \
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
                        debugPrintPush();                                                                                                                                                             \
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
                        debugPrintPush();                                                                                                                                                             \
                    }                                                                                                                                                                                 \
                    else                                                                                                                                                                              \
                    {                                                                                                                                                                                 \
                        append("stack.data[stack.ptr++].v = (scl_value) Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());                                                                \
                        debugPrintPush();                                                                                                                                                             \
                    }                                                                                                                                                                                 \
                }                                                                                                                                                                                     \
            }                                                                                                                                                                                         \
            else                                                                                                                                                                                      \
            {                                                                                                                                                                                         \
                if (v.getType() == "float")                                                                                                                                                           \
                {                                                                                                                                                                                     \
                    append("stack.data[stack.ptr++].f = Var_%s;\n", loadFrom.c_str());                                                                                                                \
                    debugPrintPush();                                                                                                                                                                 \
                }                                                                                                                                                                                     \
                else                                                                                                                                                                                  \
                {                                                                                                                                                                                     \
                    append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());                                                                                                    \
                    debugPrintPush();                                                                                                                                                                 \
                }                                                                                                                                                                                     \
            }                                                                                                                                                                                         \
        }                                                                                                                                                                                             \
        else                                                                                                                                                                                          \
        {                                                                                                                                                                                             \
            transpilerError("Unknown identifier: '" + body[i].getValue() + "'", i);                                                                                                                   \
            errors.push_back(err);                                                                                                                                                                    \
        }                                                                                                                                                                                             \
    }                                                                                                                                                                                                 \
    else if (body[i].getType() == tok_number)                                                                                                                                                         \
    {                                                                                                                                                                                                 \
        FPResult res = handleNumber(fp, body[i], scopeDepth);                                                                                                                                         \
        if (!res.success)                                                                                                                                                                             \
        {                                                                                                                                                                                             \
            errors.push_back(res);                                                                                                                                                                    \
            continue;                                                                                                                                                                                 \
        }                                                                                                                                                                                             \
    }                                                                                                                                                                                                 \
    else if (body[i].getType() == tok_addr_of)                                                                                                                                                        \
    {                                                                                                                                                                                                 \
        append("stack.data[stack.ptr - 1].v = (*(scl_value*) stack.data[stack.ptr - 1].v);\n");                                                                                                       \
    }                                                                                                                                                                                                 \
    else                                                                                                                                                                                              \
    {                                                                                                                                                                                                 \
        transpilerError("This operation may not be used in for-loops", i);                                                                                                                            \
        errors.push_back(err);                                                                                                                                                                        \
        while (body[i].getType() != tok_do && body[i].getType() != tok_to && body[i].getType() != tok_step)                                                                                           \
            ITER_INC;                                                                                                                                                                                 \
    }

#define push_var_with_type()                                                                                                                                                                      \
    if (body[i].getType() == tok_identifier)                                                                                                                                                      \
    {                                                                                                                                                                                             \
        if (body[i].getValue() == "self" && !function->isMethod)                                                                                                                                  \
        {                                                                                                                                                                                         \
            transpilerError("Can't use 'self' outside a member function.", i);                                                                                                                    \
            errors.push_back(err);                                                                                                                                                                \
            break;                                                                                                                                                                             \
        }                                                                                                                                                                                         \
        if (hasFunction(result, body[i]))                                                                                                                                                         \
        {                                                                                                                                                                                         \
            Function *f = getFunctionByName(result, body[i].getValue());                                                                                                                          \
            if (f->isMethod)                                                                                                                                                                      \
            {                                                                                                                                                                                     \
                transpilerError("'" + f->getName() + "' is a method, not a function.", i);                                                                                                        \
                errors.push_back(err);                                                                                                                                                            \
                break;                                                                                                                                                                         \
            }                                                                                                                                                                                     \
            append("stack.ptr -= %zu;\n", f->getArgs().size());                                                                                                                                   \
            if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                                    \
            {                                                                                                                                                                                     \
                if (f->getReturnType() == "float")                                                                                                                                                \
                {                                                                                                                                                                                 \
                    append("stack.data[stack.ptr++].f = Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());                                                                \
                    debugPrintPush();                                                                                                                                                             \
                }                                                                                                                                                                                 \
                else                                                                                                                                                                              \
                {                                                                                                                                                                                 \
                    append("stack.data[stack.ptr++].v = (scl_value) Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());                                                    \
                    debugPrintPush();                                                                                                                                                             \
                }                                                                                                                                                                                 \
                type = f->getReturnType();                                                                                                                                                        \
            }                                                                                                                                                                                     \
            else                                                                                                                                                                                  \
            {                                                                                                                                                                                     \
                transpilerError("Function '" + f->getName() + "' does not return a value!", i);                                                                                                   \
                errors.push_back(err);                                                                                                                                                            \
                break;                                                                                                                                                                         \
            }                                                                                                                                                                                     \
        }                                                                                                                                                                                         \
        else if (hasContainer(result, body[i]))                                                                                                                                                   \
        {                                                                                                                                                                                         \
            std::string containerName = body[i].getValue();                                                                                                                                       \
            ITER_INC;                                                                                                                                                                             \
            if (body[i].getType() != tok_dot)                                                                                                                                                     \
            {                                                                                                                                                                                     \
                transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i);                                                                            \
                errors.push_back(err);                                                                                                                                                            \
                break;                                                                                                                                                                         \
            }                                                                                                                                                                                     \
            ITER_INC;                                                                                                                                                                             \
            std::string memberName = body[i].getValue();                                                                                                                                          \
            Container container = getContainerByName(result, containerName);                                                                                                                      \
            if (!container.hasMember(memberName))                                                                                                                                                 \
            {                                                                                                                                                                                     \
                transpilerError("Unknown container member: '" + memberName + "'", i);                                                                                                             \
                errors.push_back(err);                                                                                                                                                            \
                break;                                                                                                                                                                         \
            }                                                                                                                                                                                     \
            if (container.getMemberType(memberName) == "float")                                                                                                                                   \
            {                                                                                                                                                                                     \
                append("stack.data[stack.ptr++].f = Container_%s.%s;\n", containerName.c_str(), memberName.c_str());                                                                              \
                debugPrintPush();                                                                                                                                                                 \
            }                                                                                                                                                                                     \
            else                                                                                                                                                                                  \
            {                                                                                                                                                                                     \
                append("stack.data[stack.ptr++].v = (scl_value) Container_%s.%s;\n", containerName.c_str(), memberName.c_str());                                                                  \
                debugPrintPush();                                                                                                                                                                 \
            }                                                                                                                                                                                     \
            type = container.getMemberType(memberName);                                                                                                                                           \
        }                                                                                                                                                                                         \
        else if (getStructByName(result, body[i].getValue()) != Struct(""))                                                                                                                       \
        {                                                                                                                                                                                         \
            if (body[i + 1].getType() == tok_column)                                                                                                                                              \
            {                                                                                                                                                                                     \
                ITER_INC;                                                                                                                                                                         \
                ITER_INC;                                                                                                                                                                         \
                if (body[i].getType() == tok_new)                                                                                                                                                 \
                {                                                                                                                                                                                 \
                    transpilerError("Cannot instanciate in foreach loop!", i);                                                                                                                    \
                    errors.push_back(err);                                                                                                                                                        \
                    break;                                                                                                                                                                     \
                }                                                                                                                                                                                 \
                else                                                                                                                                                                              \
                {                                                                                                                                                                                 \
                    if (!hasMethod(result, body[i], body[i - 2].getValue()))                                                                                                                      \
                    {                                                                                                                                                                             \
                        transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + body[i - 2].getValue() + "'", i);                                                               \
                        errors.push_back(err);                                                                                                                                                    \
                        break;                                                                                                                                                                 \
                    }                                                                                                                                                                             \
                    Method *f = getMethodByName(result, body[i].getValue(), body[i - 2].getValue());                                                                                              \
                    append("stack.ptr -= %zu;\n", f->getArgs().size());                                                                                                                           \
                    if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                            \
                    {                                                                                                                                                                             \
                        if (f->getReturnType() == "float")                                                                                                                                        \
                        {                                                                                                                                                                         \
                            append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());             \
                            debugPrintPush();                                                                                                                                                     \
                        }                                                                                                                                                                         \
                        else                                                                                                                                                                      \
                        {                                                                                                                                                                         \
                            append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str()); \
                            debugPrintPush();                                                                                                                                                     \
                        }                                                                                                                                                                         \
                        type = f->getReturnType();                                                                                                                                                \
                    }                                                                                                                                                                             \
                    else                                                                                                                                                                          \
                    {                                                                                                                                                                             \
                        transpilerError("Method '" + f->getMemberType() + ":" + f->getName() + "' does not return a value!", i);                                                                  \
                        errors.push_back(err);                                                                                                                                                    \
                        break;                                                                                                                                                                 \
                    }                                                                                                                                                                             \
                }                                                                                                                                                                                 \
            }                                                                                                                                                                                     \
            else                                                                                                                                                                                  \
            {                                                                                                                                                                                     \
                ITER_INC;                                                                                                                                                                         \
                ITER_INC;                                                                                                                                                                         \
                Struct s = getStructByName(result, body[i - 2].getValue());                                                                                                                       \
                if (!s.hasMember(body[i].getValue()))                                                                                                                                             \
                {                                                                                                                                                                                 \
                    transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);                                                                          \
                    errors.push_back(err);                                                                                                                                                        \
                    break;                                                                                                                                                                     \
                }                                                                                                                                                                                 \
                Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8];                                                                                                           \
                if (mem.getType() == "float")                                                                                                                                                     \
                    append("stack.data[stack.ptr - 1].f = ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", s.getName().c_str(), body[i].getValue().c_str());                            \
                else                                                                                                                                                                              \
                    append("stack.data[stack.ptr - 1].v = (scl_value) ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", s.getName().c_str(), body[i].getValue().c_str());                \
                type = mem.getType();                                                                                                                                                             \
            }                                                                                                                                                                                     \
        }                                                                                                                                                                                         \
        else if (hasVar(body[i]))                                                                                                                                                                 \
        {                                                                                                                                                                                         \
            std::string loadFrom = body[i].getValue();                                                                                                                                            \
            Variable v = getVar(body[i]);                                                                                                                                                         \
            if (getStructByName(result, v.getType()) != Struct(""))                                                                                                                               \
            {                                                                                                                                                                                     \
                if (body[i + 1].getType() == tok_column)                                                                                                                                          \
                {                                                                                                                                                                                 \
                    ITER_INC;                                                                                                                                                                     \
                    ITER_INC;                                                                                                                                                                     \
                    if (!hasMethod(result, body[i], v.getType()))                                                                                                                                 \
                    {                                                                                                                                                                             \
                        transpilerError("Unknown method '" + body[i].getValue() + "' on type '" + v.getType() + "'", i);                                                                          \
                        errors.push_back(err);                                                                                                                                                    \
                        break;                                                                                                                                                                 \
                    }                                                                                                                                                                             \
                    Method *f = getMethodByName(result, body[i].getValue(), v.getType());                                                                                                         \
                    append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());                                                                                                \
                    append("stack.ptr -= %zu;\n", f->getArgs().size());                                                                                                                           \
                    debugPrintPush();                                                                                                                                                             \
                    if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                            \
                    {                                                                                                                                                                             \
                        if (f->getReturnType() == "float")                                                                                                                                        \
                        {                                                                                                                                                                         \
                            append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());             \
                            debugPrintPush();                                                                                                                                                     \
                        }                                                                                                                                                                         \
                        else                                                                                                                                                                      \
                        {                                                                                                                                                                         \
                            append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str()); \
                            debugPrintPush();                                                                                                                                                     \
                        }                                                                                                                                                                         \
                        type = f->getReturnType();                                                                                                                                                \
                    }                                                                                                                                                                             \
                    else                                                                                                                                                                          \
                    {                                                                                                                                                                             \
                        transpilerError("Method '" + f->getMemberType() + ":" + f->getName() + "' does not return a value!", i);                                                                  \
                        errors.push_back(err);                                                                                                                                                    \
                        break;                                                                                                                                                                 \
                    }                                                                                                                                                                             \
                }                                                                                                                                                                                 \
                else                                                                                                                                                                              \
                {                                                                                                                                                                                 \
                    if (body[i + 1].getType() != tok_dot)                                                                                                                                         \
                    {                                                                                                                                                                             \
                        append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());                                                                                            \
                        debugPrintPush();                                                                                                                                                         \
                        type = getVar(Token(tok_identifier, loadFrom, 0, "", 0)).getType();\
                        break;                                                                                                                                                                 \
                    }                                                                                                                                                                             \
                    ITER_INC;                                                                                                                                                                     \
                    ITER_INC;                                                                                                                                                                     \
                    Struct s = getStructByName(result, v.getType());                                                                                                                              \
                    if (!s.hasMember(body[i].getValue()))                                                                                                                                         \
                    {                                                                                                                                                                             \
                        transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i);                                                                      \
                        errors.push_back(err);                                                                                                                                                    \
                        break;                                                                                                                                                                 \
                    }                                                                                                                                                                             \
                    Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8];                                                                                                       \
                    if (mem.getType() == "float")                                                                                                                                                 \
                    {                                                                                                                                                                             \
                        append("stack.data[stack.ptr++].f = Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());                                                                        \
                        debugPrintPush();                                                                                                                                                         \
                    }                                                                                                                                                                             \
                    else                                                                                                                                                                          \
                    {                                                                                                                                                                             \
                        append("stack.data[stack.ptr++].v = (scl_value) Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());                                                            \
                        debugPrintPush();                                                                                                                                                         \
                    }                                                                                                                                                                             \
                    type = mem.getType();                                                                                                                                                         \
                }                                                                                                                                                                                 \
            }                                                                                                                                                                                     \
            else                                                                                                                                                                                  \
            {                                                                                                                                                                                     \
                if (v.getType() == "float")                                                                                                                                                       \
                {                                                                                                                                                                                 \
                    append("stack.data[stack.ptr++].f = Var_%s;\n", loadFrom.c_str());                                                                                                            \
                    debugPrintPush();                                                                                                                                                             \
                }                                                                                                                                                                                 \
                else                                                                                                                                                                              \
                {                                                                                                                                                                                 \
                    append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());                                                                                                \
                    debugPrintPush();                                                                                                                                                             \
                }                                                                                                                                                                                 \
                type = v.getType();                                                                                                                                                               \
            }                                                                                                                                                                                     \
        }                                                                                                                                                                                         \
        else                                                                                                                                                                                      \
        {                                                                                                                                                                                         \
            transpilerError("Unknown identifier: '" + body[i].getValue() + "'", i);                                                                                                               \
            errors.push_back(err);                                                                                                                                                                \
        }                                                                                                                                                                                         \
    }                                                                                                                                                                                             \
    else                                                                                                                                                                                          \
    {                                                                                                                                                                                             \
        transpilerError("This operation may not be used in foreach-loops", i);                                                                                                                    \
        errors.push_back(err);                                                                                                                                                                    \
        while (body[i].getType() != tok_do)                                                                                       \
            ITER_INC;                                                                                                                                                                             \
    }

#define push_result()                                                                                                                                                                             \
    if (body[i].getType() == tok_identifier && hasFunction(result, body[i]))                                                                                                                      \
    {                                                                                                                                                                                             \
        Function *f = getFunctionByName(result, body[i].getValue());                                                                                                                              \
        if (f->isMethod)                                                                                                                                                                          \
        {                                                                                                                                                                                         \
            transpilerError("'" + f->getName() + "' is a method, not a function.", i);                                                                                                            \
            errors.push_back(err);                                                                                                                                                                \
            continue;                                                                                                                                                                             \
        }                                                                                                                                                                                         \
        append("stack.ptr -= %zu;\n", f->getArgs().size());                                                                                                                                       \
        if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                                        \
        {                                                                                                                                                                                         \
            if (f->getReturnType() == "float")                                                                                                                                                    \
            {                                                                                                                                                                                     \
                append("stack.data[stack.ptr++].f = Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());                                                                    \
                debugPrintPush();                                                                                                                                                                 \
            }                                                                                                                                                                                     \
            else                                                                                                                                                                                  \
            {                                                                                                                                                                                     \
                append("stack.data[stack.ptr++].v = (scl_value) Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());                                                        \
                debugPrintPush();                                                                                                                                                                 \
            }                                                                                                                                                                                     \
        }                                                                                                                                                                                         \
        else                                                                                                                                                                                      \
        {                                                                                                                                                                                         \
            append("Function_%s(%s);\n", f->getName().c_str(), sclGenArgs(result, f).c_str());                                                                                                    \
        }                                                                                                                                                                                         \
    }                                                                                                                                                                                             \
    else if (body[i].getType() == tok_identifier && hasContainer(result, body[i]))                                                                                                                \
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
            debugPrintPush();                                                                                                                                                                     \
        }                                                                                                                                                                                         \
        else                                                                                                                                                                                      \
        {                                                                                                                                                                                         \
            append("stack.data[stack.ptr++].v = (scl_value) Container_%s.%s;\n", containerName.c_str(), memberName.c_str());                                                                      \
            debugPrintPush();                                                                                                                                                                     \
        }                                                                                                                                                                                         \
    }                                                                                                                                                                                             \
    else if (body[i].getType() == tok_identifier && getStructByName(result, body[i].getValue()) != Struct(""))                                                                                    \
    {                                                                                                                                                                                             \
        if (body[i + 1].getType() == tok_column)                                                                                                                                                  \
        {                                                                                                                                                                                         \
            ITER_INC;                                                                                                                                                                             \
            ITER_INC;                                                                                                                                                                             \
            if (body[i].getType() == tok_new)                                                                                                                                                     \
            {                                                                                                                                                                                     \
                std::string struct_ = body[i - 2].getValue();                                                                                                                                     \
                append("stack.data[stack.ptr++].v = scl_alloc_struct(sizeof(struct Struct_%s), \"%s\");\n", struct_.c_str(), struct_.c_str());                                                    \
                debugPrintPush();                                                                                                                                                                 \
                if (hasMethod(result, Token(tok_identifier, "init", 0, "", 0), struct_))                                                                                                          \
                {                                                                                                                                                                                 \
                    append("{\n");                                                                                                                                                                \
                    scopeDepth++;                                                                                                                                                                 \
                    Method *f = getMethodByName(result, "init", struct_);                                                                                                                         \
                    append("struct Struct_%s* tmp = (struct Struct_%s*) stack.data[--stack.ptr].v;\n", ((Method *)(f))->getMemberType().c_str(), ((Method *)(f))->getMemberType().c_str());       \
                    append("stack.data[stack.ptr++].v = tmp;\n");                                                                                                                                 \
                    debugPrintPush();                                                                                                                                                             \
                    append("stack.ptr -= %zu;\n", f->getArgs().size());                                                                                                                           \
                    if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                            \
                    {                                                                                                                                                                             \
                        if (f->getReturnType() == "float")                                                                                                                                        \
                        {                                                                                                                                                                         \
                            append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());             \
                            debugPrintPush();                                                                                                                                                     \
                        }                                                                                                                                                                         \
                        else                                                                                                                                                                      \
                        {                                                                                                                                                                         \
                            append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str()); \
                            debugPrintPush();                                                                                                                                                     \
                        }                                                                                                                                                                         \
                    }                                                                                                                                                                             \
                    else                                                                                                                                                                          \
                    {                                                                                                                                                                             \
                        append("Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());                                             \
                    }                                                                                                                                                                             \
                    append("stack.data[stack.ptr++].v = tmp;\n");                                                                                                                                 \
                    debugPrintPush();                                                                                                                                                             \
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
                        debugPrintPush();                                                                                                                                                         \
                    }                                                                                                                                                                             \
                    else                                                                                                                                                                          \
                    {                                                                                                                                                                             \
                        append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());     \
                        debugPrintPush();                                                                                                                                                         \
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
                debugPrintPush();                                                                                                                                                                 \
                if (f->getReturnType().size() > 0 && f->getReturnType() != "none")                                                                                                                \
                {                                                                                                                                                                                 \
                    if (f->getReturnType() == "float")                                                                                                                                            \
                    {                                                                                                                                                                             \
                        append("stack.data[stack.ptr++].f = Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());                 \
                        debugPrintPush();                                                                                                                                                         \
                    }                                                                                                                                                                             \
                    else                                                                                                                                                                          \
                    {                                                                                                                                                                             \
                        append("stack.data[stack.ptr++].v = (scl_value) Method_%s_%s(%s);\n", ((Method *)(f))->getMemberType().c_str(), f->getName().c_str(), sclGenArgs(result, f).c_str());     \
                        debugPrintPush();                                                                                                                                                         \
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
                    debugPrintPush();                                                                                                                                                             \
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
                    debugPrintPush();                                                                                                                                                             \
                }                                                                                                                                                                                 \
                else                                                                                                                                                                              \
                {                                                                                                                                                                                 \
                    append("stack.data[stack.ptr++].v = (scl_value) Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str());                                                                \
                    debugPrintPush();                                                                                                                                                             \
                }                                                                                                                                                                                 \
            }                                                                                                                                                                                     \
        }                                                                                                                                                                                         \
        else                                                                                                                                                                                      \
        {                                                                                                                                                                                         \
            if (v.getType() == "float")                                                                                                                                                           \
            {                                                                                                                                                                                     \
                append("stack.data[stack.ptr++].f = Var_%s;\n", loadFrom.c_str());                                                                                                                \
                debugPrintPush();                                                                                                                                                                 \
            }                                                                                                                                                                                     \
            else                                                                                                                                                                                  \
            {                                                                                                                                                                                     \
                append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str());                                                                                                    \
                debugPrintPush();                                                                                                                                                                 \
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
    }                                                                                                                                                                                             \
    else if (body[i].getType() == tok_number_float)                                                                                                                                               \
    {                                                                                                                                                                                             \
        FPResult res = handleDouble(fp, body[i], scopeDepth);                                                                                                                                     \
        if (!res.success)                                                                                                                                                                         \
        {                                                                                                                                                                                         \
            errors.push_back(res);                                                                                                                                                                \
            continue;                                                                                                                                                                             \
        }                                                                                                                                                                                         \
    }                                                                                                                                                                                             \
    else if (body[i].getType() == tok_string_literal)                                                                                                                                             \
    {                                                                                                                                                                                             \
        append("stack.data[stack.ptr++].v = \"%s\";\n", body[i].getValue().c_str());                                                                                                              \
        debugPrintPush();                                                                                                                                                                         \
    }
