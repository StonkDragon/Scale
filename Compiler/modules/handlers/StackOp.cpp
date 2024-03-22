#include <gc/gc_allocator.h>

#include "../../headers/Common.hpp"
#include "../../headers/TranspilerDefs.hpp"
#include "../../headers/Types.hpp"

namespace sclc {
    handler(StackOp) {
        noUnused;
        if (body[i].value == "swap") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            typeStack.push_back(a);
            typeStack.push_back(b);
            append("_scl_swap();\n");
        } else if (body[i].value == "dup") {
            std::string a = typeStackTop;
            typeStack.push_back(a);
            append("_scl_dup();\n");
        } else if (body[i].value == "drop") {
            append("_scl_drop();\n");
            typePop;
        } else if (body[i].value == "over") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(a);
            typeStack.push_back(b);
            typeStack.push_back(c);
            append("_scl_over();\n");
        } else if (body[i].value == "sdup2") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            typeStack.push_back(b);
            typeStack.push_back(a);
            typeStack.push_back(b);
            append("_scl_sdup2();\n");
        } else if (body[i].value == "swap2") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(b);
            typeStack.push_back(c);
            typeStack.push_back(a);
            append("_scl_swap2();\n");
        } else if (body[i].value == "rot") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(b);
            typeStack.push_back(a);
            typeStack.push_back(c);
            append("_scl_rot();\n");
        } else if (body[i].value == "unrot") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(a);
            typeStack.push_back(c);
            typeStack.push_back(b);
            append("_scl_unrot();\n");
        }
    }
} // namespace sclc
