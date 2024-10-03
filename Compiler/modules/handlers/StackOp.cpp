
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>

namespace sclc {
    handler(StackOp) {
        noUnused;
        if (body[i].value == "swap") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            typeStack.push_back(a);
            typeStack.push_back(b);
            append("scale_swap();\n");
        } else if (body[i].value == "dup") {
            std::string a = typeStackTop;
            typeStack.push_back(a);
            append("scale_dup();\n");
        } else if (body[i].value == "drop") {
            append("scale_drop();\n");
            typePop;
        } else if (body[i].value == "over") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(a);
            typeStack.push_back(b);
            typeStack.push_back(c);
            append("scale_over();\n");
        } else if (body[i].value == "sdup2") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            typeStack.push_back(b);
            typeStack.push_back(a);
            typeStack.push_back(b);
            append("scale_sdup2();\n");
        } else if (body[i].value == "swap2") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(b);
            typeStack.push_back(c);
            typeStack.push_back(a);
            append("scale_swap2();\n");
        } else if (body[i].value == "rot") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(b);
            typeStack.push_back(a);
            typeStack.push_back(c);
            append("scale_rot();\n");
        } else if (body[i].value == "unrot") {
            std::string a = typeStackTop; typePop;
            std::string b = typeStackTop; typePop;
            std::string c = typeStackTop; typePop;
            typeStack.push_back(a);
            typeStack.push_back(c);
            typeStack.push_back(b);
            append("scale_unrot();\n");
        }
    }
} // namespace sclc
