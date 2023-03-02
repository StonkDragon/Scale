#include "../headers/Common.hpp"

using namespace sclc;

Function::Function(std::string name, Token nameToken) : Function(name, false, nameToken) {}
Function::Function(std::string name, bool isMethod, Token nameToken) : namedReturnValue("", "") {
    if (name == "+") name = "operator_" + Main.options.operatorRandomData + "_add";
    if (name == "-") name = "operator_" + Main.options.operatorRandomData + "_sub";
    if (name == "*") name = "operator_" + Main.options.operatorRandomData + "_mul";
    if (name == "/") name = "operator_" + Main.options.operatorRandomData + "_div";
    if (name == "%") name = "operator_" + Main.options.operatorRandomData + "_mod";
    if (name == "&") name = "operator_" + Main.options.operatorRandomData + "_logic_and";
    if (name == "|") name = "operator_" + Main.options.operatorRandomData + "_logic_or";
    if (name == "^") name = "operator_" + Main.options.operatorRandomData + "_logic_xor";
    if (name == "~") name = "operator_" + Main.options.operatorRandomData + "_logic_not";
    if (name == "<<") name = "operator_" + Main.options.operatorRandomData + "_logic_lsh";
    if (name == ">>") name = "operator_" + Main.options.operatorRandomData + "_logic_rsh";
    if (name == "**") name = "operator_" + Main.options.operatorRandomData + "_pow";
    if (name == ".") name = "operator_" + Main.options.operatorRandomData + "_dot";
    if (name == "<") name = "operator_" + Main.options.operatorRandomData + "_less";
    if (name == "<=") name = "operator_" + Main.options.operatorRandomData + "_less_equal";
    if (name == ">") name = "operator_" + Main.options.operatorRandomData + "_more";
    if (name == ">=") name = "operator_" + Main.options.operatorRandomData + "_more_equal";
    if (name == "==") name = "operator_" + Main.options.operatorRandomData + "_equal";
    if (name == "!") name = "operator_" + Main.options.operatorRandomData + "_not";
    if (name == "!!") name = "operator_" + Main.options.operatorRandomData + "_assert_not_nil";
    if (name == "!=") name = "operator_" + Main.options.operatorRandomData + "_not_equal";
    if (name == "&&") name = "operator_" + Main.options.operatorRandomData + "_bool_and";
    if (name == "||") name = "operator_" + Main.options.operatorRandomData + "_bool_or";
    if (name == "++") name = "operator_" + Main.options.operatorRandomData + "_inc";
    if (name == "--") name = "operator_" + Main.options.operatorRandomData + "_dec";
    if (name == "@") name = "operator_" + Main.options.operatorRandomData + "_at";
    if (name == "?") name = "operator_" + Main.options.operatorRandomData + "_wildcard";

    this->nameToken = nameToken;
    this->name = name;
    this->isMethod = isMethod;
    this->hasNamedReturnValue = false;
    this->isPrivate = false;
    this->isExternC = false;
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
    return this->nameToken;
}
void Function::setNameToken(Token t) {
    this->nameToken = t;
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
