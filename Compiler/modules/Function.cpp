#include "../headers/Common.hpp"

using namespace sclc;

Function::Function(std::string name, Token nameToken) : Function(name, false, nameToken) {}
Function::Function(std::string name, bool isMethod, Token nameToken) : namedReturnValue("", "") {
    if (name == "+") name = "operator$add";
    if (name == "-") name = "operator$sub";
    if (name == "*") name = "operator$mul";
    if (name == "/") name = "operator$div";
    if (name == "%") name = "operator$mod";
    if (name == "&") name = "operator$logic_and";
    if (name == "|") name = "operator$logic_or";
    if (name == "^") name = "operator$logic_xor";
    if (name == "~") name = "operator$logic_not";
    if (name == "<<") name = "operator$logic_lsh";
    if (name == ">>") name = "operator$logic_rsh";
    if (name == "**") name = "operator$pow";
    if (name == ".") name = "operator$dot";
    if (name == "<") name = "operator$less";
    if (name == "<=") name = "operator$less_equal";
    if (name == ">") name = "operator$more";
    if (name == ">=") name = "operator$more_equal";
    if (name == "==") name = "operator$equal";
    if (name == "!") name = "operator$not";
    if (name == "!!") name = "operator$assert_not_nil";
    if (name == "!=") name = "operator$not_equal";
    if (name == "&&") name = "operator$bool_and";
    if (name == "||") name = "operator$bool_or";
    if (name == "++") name = "operator$inc";
    if (name == "--") name = "operator$dec";
    if (name == "@") name = "operator$at";
    if (name == "=>[]") name = "operator$set";
    if (name == "[]") name = "operator$get";
    if (name == "?") name = "operator$wildcard";

    this->nameToken = nameToken;
    this->name = name;
    this->isMethod = isMethod;
    this->hasNamedReturnValue = false;
    this->isPrivate = false;
    this->isExternC = false;
    this->member_type = "";

    this->has_expect = 0;
    this->has_export = 0;
    this->has_private = 0;
    this->has_construct = 0;
    this->has_final = 0;
    this->has_cdecl = 0;
    this->has_constructor = 0;
    this->has_intrinsic = 0;
    this->has_lambda = 0;
    this->has_asm = 0;
    this->has_sealed = 0;
    this->has_unsafe = 0;
    this->has_operator = 0;
}
std::string Function::getName() {
    return name;
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
        if (this->isExternC || this->has_expect) {
            return name;
        }
        return name +
               "$" +
               std::to_string(id((char*) name.c_str())) +
               "$" +
               std::to_string(id((char*) nameToken.getFile().c_str())) +
               "$" +
               std::to_string(nameToken.getLine());
    }
    return name;
}
std::vector<Token>& Function::getBody() {
    return body;
}
std::vector<Token>& Function::getBodyRef() {
    return body;
}
void Function::addToken(Token token) {
    body.push_back(token);
}
void Function::addModifier(std::string modifier) {
    modifiers.push_back(modifier);

    if (!has_expect && modifier == "expect") {
        has_expect = modifiers.size();
        isExternC = true;
    }
    else if (!has_export && modifier == "export") has_export = modifiers.size();
    else if (!has_private && modifier == "private") {
        has_private = modifiers.size();
        isPrivate = true;
    }
    else if (!has_construct && modifier == "construct") has_construct = modifiers.size();
    else if (!has_constructor && modifier == "<constructor>") has_constructor = modifiers.size();
    else if (!has_final && modifier == "final") has_final = modifiers.size();
    else if (!has_final && modifier == "<destructor>") has_final = modifiers.size();
    else if (!has_cdecl && modifier == "cdecl") has_cdecl = modifiers.size();
    else if (!has_intrinsic && modifier == "intrinsic") has_intrinsic = modifiers.size();
    else if (!has_asm && modifier == "asm") has_asm = modifiers.size();
    else if (!has_lambda && modifier == "<lambda>") has_lambda = modifiers.size();
    else if (!has_sealed && modifier == "sealed") has_sealed = modifiers.size();
    else if (!has_unsafe && modifier == "unsafe") has_unsafe = modifiers.size();
    else if (!has_operator && modifier == "operator") has_operator = modifiers.size();
    else if (!has_restrict && modifier == "restrict") has_restrict = modifiers.size();
    else if (!has_getter && modifier == "@getter") has_getter = modifiers.size();
    else if (!has_setter && modifier == "@setter") has_setter = modifiers.size();
}
std::vector<std::string>& Function::getModifiers() {
    return modifiers;
}
void Function::addArgument(Variable arg) {
    args.push_back(arg);
}
std::vector<Variable>& Function::getArgs() {
    return args;
}
std::string Function::getFile() {
    return file;
}
void Function::setFile(std::string file) {
    this->file = file;
}
void Function::setName(std::string name) {
    this->name = name;
}
std::string Function::getReturnType() {
    return return_type;
}
void Function::setReturnType(std::string type) {
    return_type = type;
}
Token Function::getNameToken() {
    return this->nameToken;
}
void Function::setNameToken(Token t) {
    this->nameToken = t;
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
        return name == other.name && thisM->getMemberType() == otherM->getMemberType();
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
        return name == other->name && thisM->getMemberType() == otherM->getMemberType();
    }
    return name == other->name;
}
Variable& Function::getNamedReturnValue() {
    if (this->hasNamedReturnValue)
        return this->namedReturnValue;
    
    return Variable::emptyVar();
}
void Function::setNamedReturnValue(Variable v) {
    this->namedReturnValue = v;
    this->hasNamedReturnValue = true;
}
bool Function::belongsToType(std::string typeName) {
    return (!this->isMethod && !strstarts(this->getName(), typeName + "$")) || (this->isMethod && static_cast<Method*>(this)->getMemberType() != typeName);
}
void Function::clearArgs() {
    this->args = std::vector<Variable>();
}
bool Function::isCVarArgs() {
    return this->args.size() >= (1 + ((size_t) this->isMethod)) && this->args[this->args.size() - (1 + ((size_t) this->isMethod))].getType() == "varargs";
}
Variable& Function::varArgsParam() {
    if (this->isCVarArgs()) {
        return this->args[this->args.size() - (1 + ((size_t) this->isMethod))];
    }
    throw std::runtime_error("Function::varArgsParam() called on non-varargs function");
}
std::string& Function::getModifier(size_t index) {
    return this->modifiers[index - 1];
}
