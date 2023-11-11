#include <assert.h>

#include "../headers/Common.hpp"

using namespace sclc;

Function::Function(std::string name, Token name_token) : Function(name, false, name_token) {}
Function::Function(std::string name, bool isMethod, Token name_token) : namedReturnValue("", "") {
    if (name == "+") name = "operator$add";
    else if (name == "-") name = "operator$sub";
    else if (name == "*") name = "operator$mul";
    else if (name == "/") name = "operator$div";
    else if (name == "%") name = "operator$mod";
    else if (name == "&") name = "operator$logic_and";
    else if (name == "|") name = "operator$logic_or";
    else if (name == "^") name = "operator$logic_xor";
    else if (name == "~") name = "operator$logic_not";
    else if (name == "<<") name = "operator$logic_lsh";
    else if (name == "<<<") name = "operator$logic_rol";
    else if (name == ">>") name = "operator$logic_rsh";
    else if (name == ">>>") name = "operator$logic_ror";
    else if (name == "**") name = "operator$pow";
    else if (name == ".") name = "operator$dot";
    else if (name == "<") name = "operator$less";
    else if (name == "<=") name = "operator$less_equal";
    else if (name == ">") name = "operator$more";
    else if (name == ">=") name = "operator$more_equal";
    else if (name == "==") name = "operator$equal";
    else if (name == "!") name = "operator$not";
    else if (name == "!!") name = "operator$assert_not_nil";
    else if (name == "!=") name = "operator$not_equal";
    else if (name == "&&") name = "operator$bool_and";
    else if (name == "||") name = "operator$bool_or";
    else if (name == "++") name = "operator$inc";
    else if (name == "--") name = "operator$dec";
    else if (name == "@") name = "operator$at";
    else if (name == "=>") name = "operator$store";
    else if (name == "=>[]") name = "operator$set";
    else if (name == "[]") name = "operator$get";
    else if (name == "?") name = "operator$wildcard";
    else if (name == "?:") name = "operator$elvis";

    this->name_token = name_token;
    this->name = name;
    this->isMethod = isMethod;
    this->member_type = "";

    this->has_expect = 0;
    this->has_export = 0;
    this->has_private = 0;
    this->has_construct = 0;
    this->has_final = 0;
    this->has_constructor = 0;
    this->has_cdecl = 0;
    this->has_intrinsic = 0;
    this->has_lambda = 0;
    this->has_asm = 0;
    this->has_sealed = 0;
    this->has_unsafe = 0;
    this->has_operator = 0;
    this->has_restrict = 0;
    this->has_getter = 0;
    this->has_setter = 0;
    this->has_foreign = 0;
    this->has_overrides = 0;
    this->has_binary_inherited = 0;
}
std::string Function::finalName() {
    if (
        !isMethod && 
        (
            isInitFunction(this) ||
            isDestroyFunction(this) ||
            this->has_private
        )
    ) {
        if (this->has_foreign) {
            return name;
        }
        return name +
               "$" +
               std::to_string(id(name.c_str())) +
               "$" +
               std::to_string(id(name_token.location.file.c_str())) +
               "$" +
               std::to_string(name_token.location.line);
    }
    return name;
}
std::vector<Token>& Function::getBody() {
    return body;
}
void Function::addToken(Token token) {
    body.push_back(token);
}
void Function::addModifier(std::string modifier) {
    modifiers.push_back(modifier);

    if (has_expect == 0 && modifier == "expect") has_expect = modifiers.size();
    else if (has_export == 0 && modifier == "export") has_export = modifiers.size();
    else if (has_private == 0 && modifier == "private") has_private = modifiers.size();
    else if (has_construct == 0 && modifier == "construct") has_construct = modifiers.size();
    else if (has_constructor == 0 && modifier == "<constructor>") has_constructor = modifiers.size();
    else if (has_final == 0 && modifier == "final") has_final = modifiers.size();
    else if (has_final == 0 && modifier == "<destructor>") has_final = modifiers.size();
    else if (has_cdecl == 0 && modifier == "cdecl") has_cdecl = modifiers.size();
    else if (has_intrinsic == 0 && modifier == "intrinsic") has_intrinsic = modifiers.size();
    else if (has_asm == 0 && modifier == "asm") has_asm = modifiers.size();
    else if (has_lambda == 0 && modifier == "<lambda>") has_lambda = modifiers.size();
    else if (has_sealed == 0 && modifier == "sealed") has_sealed = modifiers.size();
    else if (has_unsafe == 0 && modifier == "unsafe") has_unsafe = modifiers.size();
    else if (has_operator == 0 && modifier == "operator") has_operator = modifiers.size();
    else if (has_restrict == 0 && modifier == "restrict") has_restrict = modifiers.size();
    else if (has_getter == 0 && modifier == "@getter") has_getter = modifiers.size();
    else if (has_setter == 0 && modifier == "@setter") has_setter = modifiers.size();
    else if (has_foreign == 0 && modifier == "foreign") has_foreign = modifiers.size();
    else if (has_overrides == 0 && modifier == "overrides") has_overrides = modifiers.size();
    else if (has_binary_inherited == 0 && modifier == "<binary-inherited>") has_binary_inherited = modifiers.size();
}
void Function::addArgument(Variable arg) {
    args.push_back(arg);
}
bool Function::operator!=(const Function& other) const {
    return !(*this == other);
}
bool Function::operator==(const Function& other) const {
    if (other.isMethod && !this->isMethod) return false;
    if (!other.isMethod && this->isMethod) return false;
    if (other.isMethod && this->isMethod) {
        Method* thisM = (Method*) this;
        Function* f = (Function*) &other;
        Method* otherM = (Method*) f;
        return name == other.name && thisM->member_type == otherM->member_type;
    }
    return name == other.name;
}
bool Function::operator!=(const Function* other) const {
    return !(*this == other);
}
bool Function::operator==(const Function* other) const {
    if (this == other) return true;
    if (other->isMethod && !this->isMethod) return false;
    if (!other->isMethod && this->isMethod) return false;
    if (other->isMethod && this->isMethod) {
        Method* thisM = (Method*) this;
        Method* otherM = (Method*) other;
        return name == other->name && thisM->member_type == otherM->member_type;
    }
    return name == other->name;
}
bool Function::belongsToType(std::string typeName) {
    return (!this->isMethod && !strstarts(this->name, typeName + "$")) || (this->isMethod && static_cast<Method*>(this)->member_type != typeName);
}
void Function::clearArgs() {
    this->args = std::vector<Variable>();
}
bool Function::isCVarArgs() {
    return this->args.size() >= (1 + ((size_t) this->isMethod)) && this->args[this->args.size() - (1 + ((size_t) this->isMethod))].type == "varargs";
}
Variable& Function::varArgsParam() {
    if (this->isCVarArgs()) {
        return this->args[this->args.size() - (1 + ((size_t) this->isMethod))];
    }
    throw std::runtime_error(std::string(__func__) + " called on non-varargs function");
}
const std::string& Function::getModifier(size_t index) {
    if (index <= 0 || index > this->modifiers.size()) {
        throw std::runtime_error(std::string(__func__) + " called with invalid index: " + std::to_string(index) + " (size: " + std::to_string(this->modifiers.size()) + ")");
    }
    return this->modifiers.at(index - 1);
}
