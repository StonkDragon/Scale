#include "../headers/Common.hpp"

namespace sclc {
    FPResult::FPResult() : location("", 0, 0) {
        success = false;
        isNote = false;
        type = tok_eof;
        value = "";
    }
    std::string FPResult::toString() const {
        return "FPResult {success: " +
                std::to_string(success) +
                ", isNote: " +
                std::to_string(isNote) +
                ", message: " +
                message +
                ", value: " +
                value +
                ", type: " +
                std::to_string(type) +
                ", location: " +
                location.toString() +
                "}";
    }
}