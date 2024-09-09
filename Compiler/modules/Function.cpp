
#include "../headers/Common.hpp"

using namespace sclc;

Function::Function(std::string name, Token name_token) : Function(name, false, name_token) {}
Function::Function(std::string name, bool isMethod, Token name_token) : namedReturnValue("", "") {
    for (auto&& p : funcNameIdents) {
        if (p.first == name) {
            name = p.second;
            DBG("Function with name '%s' is actually '%s'", p.first.c_str(), name.c_str());
            break;
        }
    }

    this->name_token = name_token;
    this->name = name;
    this->name_without_overload = this->name.substr(0, this->name.find("$$ol"));
    this->isMethod = isMethod;
    this->member_type = "";

    this->has_expect = 0;
    this->has_private = 0;
    this->has_construct = 0;
    this->has_destructor = 0;
    this->has_constructor = 0;
    this->has_cdecl = 0;
    this->has_lambda = 0;
    this->has_asm = 0;
    this->has_final = 0;
    this->has_unsafe = 0;
    this->has_operator = 0;
    this->has_getter = 0;
    this->has_setter = 0;
    this->has_foreign = 0;
    this->has_overrides = 0;
    this->has_nonvirtual = 0;
    this->has_async = 0;
    this->has_reified = 0;
    this->has_inline = 0;
    this->has_const = 0;
}
Function::~Function() {}
std::vector<Token>& Function::getBody() {
    return body;
}
void Function::addToken(Token token) {
    body.push_back(token);
}
void Function::addModifier(std::string modifier) {
    modifiers.push_back(modifier);

    if (has_expect == 0 && modifier == "expect") has_expect = modifiers.size();
    else if (has_private == 0 && modifier == "private") has_private = modifiers.size();
    else if (has_construct == 0 && modifier == "construct") has_construct = modifiers.size();
    else if (has_constructor == 0 && modifier == "<constructor>") has_constructor = modifiers.size();
    else if (has_destructor == 0 && modifier == "<destructor>") has_destructor = modifiers.size();
    else if (has_cdecl == 0 && modifier == "cdecl") has_cdecl = modifiers.size();
    else if (has_asm == 0 && modifier == "asm") has_asm = modifiers.size();
    else if (has_lambda == 0 && modifier == "<lambda>") has_lambda = modifiers.size();
    else if (has_final == 0 && modifier == "final") has_final = modifiers.size();
    else if (has_unsafe == 0 && modifier == "unsafe") has_unsafe = modifiers.size();
    else if (has_operator == 0 && modifier == "operator") has_operator = modifiers.size();
    else if (has_getter == 0 && modifier == "@getter") has_getter = modifiers.size();
    else if (has_setter == 0 && modifier == "@setter") has_setter = modifiers.size();
    else if (has_foreign == 0 && modifier == "foreign") has_foreign = modifiers.size();
    else if (has_overrides == 0 && modifier == "overrides") has_overrides = modifiers.size();
    else if (has_nonvirtual == 0 && modifier == "nonvirtual") has_nonvirtual = modifiers.size();
    else if (has_async == 0 && modifier == "async") has_async = modifiers.size();
    else if (has_const == 0 && modifier == "const") has_const = modifiers.size();
    else if (has_inline == 0 && modifier == "inline") has_inline = modifiers.size();
    else if (has_reified == 0 && modifier == "reified") {
        has_reified = modifiers.size();
        has_nonvirtual = modifiers.size();
    }
}
void Function::addArgument(Variable arg) {
    args.push_back(arg);
}
bool Function::operator!=(const Function& other) const {
    return !this->operator==(other);
}
bool hasCompatibleArgs(const std::vector<Variable>& argsA, const std::vector<Variable>& argsB) {
    if (argsA.size() != argsB.size()) return false;
    for (size_t i = 0; i < argsB.size(); i++) {
        if (!typeEquals(argsA[i].type, argsB[i].type)) return false;
    }
    return true;
}
bool Function::operator==(const Function& other) const {
    if (isMethod != other.isMethod) return false;
    if (name != other.name) return false;
    // if (has_reified != other.has_reified) return false;
    // if (hasCompatibleArgs(args, other.args)) return false;
    return member_type == other.member_type;
}
bool Function::operator!=(const Function* other) const {
    return !this->operator==(*other);
}
bool Function::operator==(const Function* other) const {
    if (this == other) return true;
    return this->operator==(*other);
}
bool Function::belongsToType(std::string typeName) {
    return (!this->isMethod && !strstarts(this->name, typeName + "$")) || (this->isMethod && static_cast<Method*>(this)->member_type != typeName);
}
void Function::clearArgs() {
    this->args.clear();
}
bool Function::isCVarArgs() {
    bool hasAdditionalParams = false;
    if (!this->isMethod) {
        hasAdditionalParams = !this->args.empty();
    } else {
        hasAdditionalParams = this->args.size() > 1;
    }
    if (!hasAdditionalParams) return false;

    const Variable& lastArg = this->args[this->args.size() - 1 - this->isMethod];
    return removeTypeModifiers(lastArg.type) == "varargs";
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
Function* Function::clone() {
    Function* f = new Function(this->name, this->isMethod, this->name_token);
    f->isMethod = isMethod;
    f->return_type = return_type;
    f->member_type = member_type;
    for (auto s : modifiers) {
        f->addModifier(s);
    }
    for (Token t : this->getBody()) {
        f->addToken(t);
    }
    for (Variable& v : args) {
        f->addArgument(v);
    }
    f->namedReturnValue = namedReturnValue;
    f->container = container;
    f->overloads.insert(f->overloads.end(), overloads.begin(), overloads.end());
    overloads.push_back(f);
    return f;
}

Method::Method(std::string member_type, std::string name, Token name_token) : Function(name, true, name_token) {
    this->member_type = member_type;
    this->isMethod = true;
    this->force_add = false;
}
Method* Method::cloneAs(std::string memberType) {
    Method* m = new Method(memberType, name, name_token);
    m->isMethod = isMethod;
    m->force_add = force_add;
    m->return_type = return_type;
    for (auto s : modifiers) {
        m->addModifier(s);
    }
    m->return_type = return_type;
    for (Token t : this->getBody()) {
        m->addToken(t);
    }
    for (Variable& v : args) {
        m->addArgument(v);
    }
    m->namedReturnValue = namedReturnValue;
    m->container = container;
    return m;
}
