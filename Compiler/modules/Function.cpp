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
            contains<std::string>(this->getModifiers(), "private")
        )
    ) {
        if (this->isExternC || contains<std::string>(this->getModifiers(), "extern")) {
            return name;
        }
        return name +
               "$" +
               std::to_string(hash1((char*) name.c_str())) +
               "$" +
               std::to_string(hash1((char*) nameToken.getFile().c_str())) +
               "$" +
               std::to_string(nameToken.getLine());
    }
    return name;
}
std::vector<Token> Function::getBody() {
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
}
std::vector<std::string> Function::getModifiers() {
    return modifiers;
}
void Function::addArgument(Variable arg) {
    args.push_back(arg);
}
std::vector<Variable> Function::getArgs() {
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
Variable Function::getNamedReturnValue() {
    if (this->hasNamedReturnValue)
        return this->namedReturnValue;
    
    return Variable("", "");
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
    return this->args.size() >= (1 + ((size_t) this->isMethod)) && this->args.at(this->args.size() - (1 + ((size_t) this->isMethod))).getType() == "varargs";
}
Variable& Function::varArgsParam() {
    if (this->isCVarArgs()) {
        return this->args.at(this->args.size() - (1 + ((size_t) this->isMethod)));
    }
    throw std::runtime_error("Function::varArgsParam() called on non-varargs function");
}
