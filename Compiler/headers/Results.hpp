#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include "Function.hpp"
#include "Interface.hpp"
#include "Variable.hpp"
#include "Container.hpp"
#include "Struct.hpp"
#include "Enum.hpp"

namespace sclc
{
    class FPResult {
    public:
        bool success;
        bool isNote;
        std::string message;
        std::string in;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::string value;
        int column;
        int line;
        TokenType type;

        FPResult() {
            success = false;
            isNote = false;
            column = 0;
            line = 0;
            type = tok_eof;
            in = "";
            value = "";
        }

        std::string toString() const {
            return "FPResult {success: " +
                    std::to_string(success) +
                    ", isNote: " +
                    std::to_string(isNote) +
                    ", message: " +
                    message +
                    ", in: " +
                    in +
                    ", value: " +
                    value +
                    ", column: " +
                    std::to_string(column) +
                    ", line: " +
                    std::to_string(line) +
                    ", type: " +
                    std::to_string(type) +
                    "}";
        }
    };

    class TPResult {
    public:
        std::vector<Function*> functions;
        std::vector<Interface*> interfaces;
        std::vector<Variable> extern_globals;
        std::vector<Variable> globals;
        std::vector<Container> containers;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::vector<Struct> structs;
        std::vector<Layout> layouts;
        std::vector<Enum> enums;
        std::unordered_map<std::string, std::string> typealiases;
    };

    template<typename R, typename E>
    class Result {
        union {
            R result;
            E error;
        };
        bool success;

    public:
        Result(R result) {
            this->result = result;
            success = true;
        }

        Result(E error) {
            this->error = error;
            success = false;
        }

        ~Result() {
            if (success) {
                result.~R();
            } else {
                error.~E();
            }
        }

        bool isSuccess() const {
            return success;
        }

        bool isError() const {
            return !success;
        }

        R unwrap() const {
            if (success) {
                return result;
            }
            throw std::runtime_error("Result is not a success");
        }

        R unwrapOr(R orValue) const {
            if (success) {
                return result;
            }
            return orValue;
        }

        R unwrapOr(R (*orValue)()) const {
            if (success) {
                return result;
            }
            return orValue();
        }

        R unwrapOr(R (*orValue)(E)) const {
            if (success) {
                return result;
            }
            return orValue(error);
        }

        R unwrapOr(std::function<R()> orValue) const {
            if (success) {
                return result;
            }
            return orValue();
        }

        R unwrapOr(std::function<R(E)> orValue) const {
            if (success) {
                return result;
            }
            return orValue(error);
        }

        E unwrapErr() const {
            if (!success) {
                return error;
            }
            throw std::runtime_error("Result is not an error");
        }
    };
} // namespace sclc
