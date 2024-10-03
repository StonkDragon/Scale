#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include <Function.hpp>
#include <Interface.hpp>
#include <Variable.hpp>
#include <Struct.hpp>
#include <Enum.hpp>

namespace sclc
{
    struct FPResult {
        bool success;
        bool isNote;
        std::string message;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::string value;
        SourceLocation location;
        TokenType type;

        FPResult();
        std::string toString() const;
    };

    class TPResult {
    public:
        std::vector<Function*> functions;
        std::vector<Interface*> interfaces;
        std::vector<Variable> globals;
        std::vector<FPResult> errors;
        std::vector<FPResult> warns;
        std::vector<Struct> structs;
        std::vector<Layout> layouts;
        std::vector<Enum> enums;
        std::unordered_map<std::string, std::pair<std::string, bool>> typealiases;

        ~TPResult();
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
