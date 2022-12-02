#define transpilerError(msg, at) \
    FPResult err; \
    err.success = false; \
    err.message = msg; \
    err.column = body[(at)].getColumn(); \
    err.line = body[(at)].getLine(); \
    err.in = body[(at)].getFile(); \
    err.value = body[(at)].getValue(); \
    err.type =  body[(at)].getType()

#define push_var() \
if (hasContainer(result, body[i])) { \
std::string containerName = body[i].getValue(); \
ITER_INC; \
if (body[i].getType() != tok_dot) { \
transpilerError("Expected '.' to access container contents, but got '" + body[i].getValue() + "'", i); \
errors.push_back(err); \
continue; \
} \
ITER_INC; \
std::string memberName = body[i].getValue(); \
Container container = getContainerByName(result, containerName); \
if (!container.hasMember(memberName)) { \
transpilerError("Unknown container member: '" + memberName + "'", i); \
errors.push_back(err); \
continue; \
} \
if (container.getMemberType(memberName) == "float") { \
transpilerError("Floats are not supported in for loops!", i); \
errors.push_back(err); \
continue; \
} else \
append("stack.data[stack.ptr++].v = (scl_value) Container_%s.%s;\n", containerName.c_str(), memberName.c_str()); \
} else if (getStructByName(result, body[i].getValue()) != Struct("")) { \
if (body[i + 1].getType() == tok_column) { \
 \
} else { \
ITER_INC; \
ITER_INC; \
Struct s = getStructByName(result, body[i - 2].getValue()); \
if (!s.hasMember(body[i].getValue())) { \
transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i); \
errors.push_back(err); \
continue; \
} \
Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8]; \
if (mem.getType() == "float") { \
transpilerError("Floats are not supported in for loops!", i); \
errors.push_back(err); \
continue; \
} else \
append("stack.data[stack.ptr - 1].v = (scl_value) ((struct Struct_%s*) stack.data[stack.ptr - 1].v)->%s;\n", s.getName().c_str(), body[i].getValue().c_str()); \
} \
} else if (hasVar(body[i])) { \
std::string loadFrom = body[i].getValue(); \
Variable v = getVar(body[i]); \
if (getStructByName(result, v.getType()) != Struct("")) { \
if (body[i + 1].getType() == tok_column) { \
 \
} else { \
if (body[i + 1].getType() != tok_dot) { \
append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str()); \
continue; \
} \
ITER_INC; \
ITER_INC; \
Struct s = getStructByName(result, v.getType()); \
if (!s.hasMember(body[i].getValue())) { \
transpilerError("Struct '" + s.getName() + "' has no member named '" + body[i].getValue() + "'", i); \
errors.push_back(err); \
continue; \
} \
Variable mem = s.getMembers()[s.indexOfMember(body[i].getValue()) / 8]; \
if (mem.getType() == "float") { \
transpilerError("Floats are not supported in for loops!", i); \
errors.push_back(err); \
continue; \
} else \
append("stack.data[stack.ptr++].v = (scl_value) Var_%s->%s;\n", loadFrom.c_str(), body[i].getValue().c_str()); \
} \
} else { \
if (v.getType() == "float") { \
transpilerError("Floats are not supported in for loops!", i); \
errors.push_back(err); \
continue; \
} else \
append("stack.data[stack.ptr++].v = (scl_value) Var_%s;\n", loadFrom.c_str()); \
} \
} else if (body[i].getType() == tok_number) { \
FPResult res = handleNumber(fp, body[i], scopeDepth); \
if (!res.success) { \
errors.push_back(res); \
continue; \
} \
}