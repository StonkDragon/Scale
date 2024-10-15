
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    extern std::vector<std::string> strings;

    handler(Dot) {
        noUnused;
        std::string type = typeStackTop;
        typePop;

        if (hasLayout(result, type)) {
            const Layout& l = getLayout(result, type);
            safeInc();
            std::string member = body[i].value;
            if (!l.hasMember(member)) {
                transpilerError("No layout for '" + member + "' in '" + type + "'", i);
                errors.push_back(err);
                return;
            }
            const Variable& v = l.getMember(member);

            append("scale_top(%s) = scale_top(%s)->%s;\n", sclTypeToCType(result, v.type).c_str(), sclTypeToCType(result, l.name).c_str(), member.c_str());
            typeStack.push_back(l.getMember(member).type);
            return;
        }
        std::string tmp = removeTypeModifiers(type);
        Struct s = getStructByName(result, type);
        if (s == Struct::Null) {
            transpilerError("Cannot infer type of stack top for '.': expected valid Struct, but got '" + type + "'", i);
            errors.push_back(err);
            return;
        }
        Token dot = body[i];
        bool deref = body[i + 1].type == tok_addr_of;
        safeIncN(deref + 1);
        if (!s.hasMember(body[i].value)) {
            if (s.name == "Map" || s.super == "Map") {
                Method* f = getMethodByName(result, "get", s.name);
                if (!f) {
                    transpilerError("Could not find method 'get' on struct '" + s.name + "'", i - 1);
                    errors.push_back(err);
                    return;
                }
                
                append("{\n");
                scopeDepth++;
                append("scale_any tmp = scale_pop(scale_any);\n");
                std::string t = type;
                append("scale_push(scale_str, (scale_str) (scale_mark_static(&static_str_%lu.layout) + sizeof(memory_layout_t)));\n", findOrAdd(strings, body[i].value));
                typeStack.push_back("str");
                append("scale_push(scale_any, tmp);\n");
                typeStack.push_back(t);
                scopeDepth--;
                append("}\n");
                methodCall(f, fp, result, warns, errors, body, i);
                return;
            }
            std::string help = "";
            Method* m;
            if ((m = getMethodByName(result, body[i].value, s.name)) != nullptr) {
                std::string lambdaType = "lambda(";
                for (size_t i = 0; i < m->args.size(); i++) {
                    if (i) lambdaType += ",";
                    lambdaType += m->args[i].type;
                }
                lambdaType += "):" + m->return_type;
                append("scale_push(scale_any, %s);\n", s.name.c_str(), m->outputName().c_str());
                typeStack.push_back(lambdaType);
                return;
            }
            transpilerError("Struct '" + s.name + "' has no member named '" + body[i].value + "'" + help, i);
            errors.push_back(err);
            return;
        }
        Variable mem = s.getMember(body[i].value);
        if (!mem.isAccessible(currentFunction)) {
            if (s.name != function->member_type) {
                transpilerError("'" + body[i].value + "' has private access in Struct '" + s.name + "'", i);
                errors.push_back(err);
                return;
            }
        }
        if (typeCanBeNil(type) && dot.value != "?.") {
            {
                transpilerError("Member access on maybe-nil type '" + type + "'", i);
                errors.push_back(err);
            }
            transpilerError("If you know '" + type + "' can't be nil at this location, add '!!' before this '.'", i);
            err.isNote = true;
            errors.push_back(err);
            return;
        }

        Method* m = attributeAccessor(result, s.name, mem.name);
        if (m) {
            if (dot.value == "?.") {
                if (deref) {
                    append("scale_top(%s) = scale_top(scale_int) ? *(%s(scale_top(%s))) : 0%s;\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->outputName().c_str(), sclTypeToCType(result, m->member_type).c_str(), removeTypeModifiers(m->return_type) == "float" ? ".0" : "");
                    std::string type = m->return_type;
                    if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                        type = type.substr(1, type.size() - 2);
                    } else if (type.front() == '*') {
                        type = type.substr(1);
                    } else {
                        type = "any";
                    }
                    typeStack.push_back(type);
                } else {
                    append("scale_top(%s) = scale_top(scale_int) ? %s(scale_top(%s)) : 0%s;\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->outputName().c_str(), sclTypeToCType(result, m->member_type).c_str(), removeTypeModifiers(m->return_type) == "float" ? ".0" : "");
                    typeStack.push_back(m->return_type);
                }
            } else {
                if (deref) {
                    append("scale_top(%s) = *(%s(scale_top(%s)));\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->outputName().c_str(), sclTypeToCType(result, m->member_type).c_str());
                    std::string type = m->return_type;
                    if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                        type = type.substr(1, type.size() - 2);
                    } else if (type.front() == '*') {
                        type = type.substr(1);
                    } else {
                        type = "any";
                    }
                    typeStack.push_back(type);
                } else {
                    append("scale_top(%s) = %s(scale_top(%s));\n", sclTypeToCType(result, m->return_type).c_str(), m->member_type.c_str(), m->outputName().c_str(), sclTypeToCType(result, m->member_type).c_str());
                    typeStack.push_back(m->return_type);
                }
            }
            return;
        }

        if (dot.value == "?.") {
            append("scale_top(%s) = scale_top(scale_int) ? scale_top(%s)->%s : NULL;\n", sclTypeToCType(result, mem.type).c_str(), sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
        } else {
            append("scale_top(%s) = scale_top(%s)->%s;\n", sclTypeToCType(result, mem.type).c_str(), sclTypeToCType(result, s.name).c_str(), body[i].value.c_str());
        }
        typeStack.push_back(mem.type);
        if (deref) {
            std::string path = dot.value + "@" + body[i].value;
            append("scale_assert_fast(scale_top(scale_int), \"Tried dereferencing nil pointer '%s'!\");", path.c_str());
            append("scale_top(scale_any) = *scale_top(scale_any*);\n");
            std::string type = typeStackTop;
            typePop;
            if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                type = type.substr(1, type.size() - 2);
            } else if (type.front() == '*') {
                type = type.substr(1);
            } else {
                type = "any";
            }
            typeStack.push_back(type);
        }
    }
} // namespace sclc

