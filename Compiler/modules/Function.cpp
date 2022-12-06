#include "../headers/Common.hpp"

using namespace sclc;

Function::Function(std::string name, Token& nameToken) {
    this->nameToken = &nameToken;
    this->name = name;
    this->isMethod = false;
}
Function::Function(std::string name, bool isMethod, Token& nameToken) {
    this->nameToken = &nameToken;
    this->name = name;
    this->isMethod = isMethod;
}
std::string Function::getName() {
    return name;
}
std::vector<Token> Function::getBody() {
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
    return *(this->nameToken);
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
