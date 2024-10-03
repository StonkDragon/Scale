
#include <Common.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>
#include <Functions.hpp>

namespace sclc {
    handler(IntegerLiteral) {
        noUnused;
        std::string stringValue = body[i].value;

        // only parse to verify bounds and make very large integer literals defined behavior
        size_t suf_idx = -1;
        union {
            long long s;
            unsigned long long u;
        } val;
        try {
            if (strstarts(stringValue, "0x")) {
                stringValue = stringValue.substr(2);
                val.u = std::stoull(stringValue, &suf_idx, 16);
            } else if (strstarts(stringValue, "0b")) {
                stringValue = stringValue.substr(2);
                val.u = std::stoull(stringValue, &suf_idx, 2);
            } else if (strstarts(stringValue, "0c")) {
                stringValue = stringValue.substr(2);
                val.u = std::stoull(stringValue, &suf_idx, 8);
            } else {
                val.s = std::stoll(stringValue, &suf_idx, 10);
            }
        } catch (const std::invalid_argument& e) {
            transpilerError("Invalid integer literal: '" + stringValue + "'", i);
            errors.push_back(err);
            return;
        } catch (const std::out_of_range& e) {
            transpilerError("Integer literal out of range: '" + stringValue + "' " + e.what(), i);
            errors.push_back(err);
            return;
        }

        std::string suffix = "";
        if (suf_idx != (size_t) -1) {
            suffix = stringValue.substr(suf_idx);
        }

        if (suffix == "u") typeStack.push_back("uint");
        else if (suffix == "i8") typeStack.push_back("int8");
        else if (suffix == "i16") typeStack.push_back("int16");
        else if (suffix == "i32") typeStack.push_back("int32");
        else if (suffix == "i64") typeStack.push_back("int64");
        else if (suffix == "u8") typeStack.push_back("uint8");
        else if (suffix == "u16") typeStack.push_back("uint16");
        else if (suffix == "u32") typeStack.push_back("uint32");
        else if (suffix == "u64") typeStack.push_back("uint64");
        else typeStack.push_back("int");

        append("scale_push(%s, 0x%llx & SCALE_%s_MASK);\n", sclTypeToCType(result, typeStackTop).c_str(), val.u, typeStackTop.c_str());
    }
} // namespace sclc

