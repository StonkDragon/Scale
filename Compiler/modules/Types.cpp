#include <array>

#include "../headers/Common.hpp"
#include "../headers/TokenType.hpp"
#include "../headers/TranspilerDefs.hpp"
#include "../headers/Types.hpp"

namespace sclc {
    extern std::stack<std::string> typeStack;
    extern Struct currentStruct;

    const std::array<std::string, 3> removableTypeModifiers = {"mut ", "const ", "readonly "};

    std::string notNilTypeOf(std::string t) {
        while (t.size() && t != "?" && t.back() == '?') t = t.substr(0, t.size() - 1);
        return t;
    }

    std::string typePointedTo(std::string type) {
        if (type.size() >= 2 && type.front() == '[' && type.back() == ']') {
            return type.substr(1, type.size() - 2);
        }
        return "any";
    }

    std::string primitiveToReference(std::string type) {
        type = removeTypeModifiers(type);
        type = notNilTypeOf(type);
        if (!isPrimitiveType(type)) {
            return type;
        }
        if (isPrimitiveIntegerType(type) || type == "bool") {
            return "Int";
        }
        if (type == "float") {
            return "Float";
        }
        return "Any";
    }

    bool structExtends(TPResult& result, Struct& s, std::string name) {
        if (s.name == name) return true;

        Struct super = getStructByName(result, s.super);
        Struct oldSuper = s;
        while (super.name.size()) {
            if (super.name == name)
                return true;
            oldSuper = super;
            super = getStructByName(result, super.super);
        }
        return false;
    }

    std::string selfTypeToRealType(std::string selfType, std::string realType) {
        bool selfTypeIsNilable = typeCanBeNil(selfType);
        bool selfTypeIsValueType = selfType.front() == '*';
        selfType = removeTypeModifiers(selfType);
        if (selfType == "self") {
            std::string type = "";
            if (selfTypeIsValueType) {
                type += "*";
            }
            type += realType;
            if (selfTypeIsNilable) {
                type += "?";
            }
            return type;
        } else if (selfType.front() == '[' && selfType.back() == ']') {
            std::string type = "";
            if (selfTypeIsValueType) {
                type += "*";
            }
            type += "[" + selfTypeToRealType(selfType.substr(1, selfType.size() - 2), realType) + "]";
            if (selfTypeIsNilable) {
                type += "?";
            }
            return type;
        } else {
            return selfType;
        }
    }

    bool isSelfType(std::string type) {
        type = removeTypeModifiers(type);
        if (type == "self") {
            return true;
        } else if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            return isSelfType(type.substr(1, type.size() - 2));
        }
        return false;
    }

    bool isPrimitiveIntegerType(std::string s) {
        s = removeTypeModifiers(s);
        return s == "int" ||
               s == "int64" ||
               s == "int32" ||
               s == "int16" ||
               s == "int8" ||
               s == "uint" ||
               s == "uint64" ||
               s == "uint32" ||
               s == "uint16" ||
               s == "uint8";
    }

    bool argsAreIdentical(std::vector<Variable>& methodArgs, std::vector<Variable>& functionArgs) {
        if ((methodArgs.size() - 1) != functionArgs.size()) return false;
        for (size_t i = 0; i < methodArgs.size(); i++) {
            if (methodArgs[i].name == "self" || functionArgs[i].name == "self") continue;
            if (!typeEquals(methodArgs[i].type, functionArgs[i].type)) return false;
        }
        return true;
    }

    std::string argVectorToString(std::vector<Variable>& args) {
        std::string arg = "";
        for (size_t i = 0; i < args.size(); i++) {
            if (i) 
                arg += ", ";
            const Variable& v = args[i];
            arg += removeTypeModifiers(v.type);
        }
        return arg;
    }

    std::string argVectorToString(std::vector<std::string>& args) {
        std::string arg = "";
        for (size_t i = 0; i < args.size(); i++) {
            if (i) 
                arg += ", ";
            arg += removeTypeModifiers(args[i]);
        }
        return arg;
    }

    bool canBeCastTo(TPResult& r, const Struct& one, const Struct& other) {
        if (one == other || (other.name == "SclObject" && !Main::options::noScaleFramework)) {
            return true;
        }
        if ((one.isStatic() && !other.isStatic()) || (!one.isStatic() && other.isStatic())) {
            return false;
        }
        Struct oneParent = getStructByName(r, one.super);
        bool extend = false;
        while (!extend) {
            if (oneParent == other) {
                extend = true;
            } else if ((oneParent.name == "SclObject" && !Main::options::noScaleFramework) || oneParent.name.empty()) {
                return false;
            }
            oneParent = getStructByName(r, oneParent.super);
        }
        return extend;
    }

    std::string lambdaReturnType(const std::string& lambda) {
        if (strstarts(lambda, "lambda("))
            return lambda.substr(lambda.find_last_of("):") + 1);
        return "";
    }

    size_t lambdaArgCount(const std::string& lambda) {
        if (strstarts(lambda, "lambda(")) {
            std::string args = lambda.substr(lambda.find_first_of("(") + 1, lambda.find_last_of("):") - lambda.find_first_of("(") - 1);
            return std::stoi(args);
        }
        return -1;
    }

    bool lambdasEqual(const std::string& a, const std::string& b) {
        std::string returnTypeA = lambdaReturnType(a);
        std::string returnTypeB = lambdaReturnType(b);

        size_t argAmountA = lambdaArgCount(a);
        size_t argAmountB = lambdaArgCount(b);

        return argAmountA == argAmountB && typeEquals(returnTypeA, returnTypeB);
    }

    bool typeEquals(const std::string& a, const std::string& b) {
        if (b == "any" || b == "[any]") {
            return true;
        }
        if (removeTypeModifiers(a) == removeTypeModifiers(b)) {
            return true;
        } else if (a.size() > 2 && a.front() == '[') {
            if (b.size() > 2 && b.front() == '[') {
                return typeEquals(a.substr(1, a.size() - 1), b.substr(1, b.size() - 1));
            }
        } else if (strstarts(a, "lambda(") && strstarts(b, "lambda(")) {
            return lambdasEqual(a, b);
        } else if (a == "?" || b == "?") {
            return true;
        }
        return false;
    }

    bool typeIsUnsigned(std::string s) {
        s = removeTypeModifiers(s);
        return isPrimitiveIntegerType(s) && s.front() == 'u';
    }

    bool typeIsSigned(std::string s) {
        s = removeTypeModifiers(s);
        return isPrimitiveIntegerType(s) && s.front() == 'i';
    }

    size_t intBitWidth(std::string s) {
        std::string type = removeTypeModifiers(s);
        if (type == "int" || type == "uint") return 64;
        if (type == "int8" || type == "uint8") return 8;
        if (type == "int16" || type == "uint16") return 16;
        if (type == "int32" || type == "uint32") return 32;
        if (type == "int64" || type == "uint64") return 64;
        return 0;
    }

    bool typesCompatible(TPResult& result, std::string stack, std::string arg, bool allowIntPromotion) {
        if (strstarts(arg, "ctype$")) {
            return true;
        }
        if (arg == stack) {
            return true;
        }
        bool stackTypeIsNilable = typeCanBeNil(stack);
        bool argIsNilable = typeCanBeNil(arg);
        stack = removeTypeModifiers(stack);
        arg = removeTypeModifiers(arg);
        if (arg == "bool") arg = "int";
        if (stack == "bool") stack = "int";
        if (typeEquals(stack, arg) && stackTypeIsNilable == argIsNilable) {
            return true;
        }

        if (hasEnum(result, stack)) stack = "int";
        if (hasEnum(result, arg)) arg = "int";

        if (isPrimitiveIntegerType(arg) && isPrimitiveIntegerType(stack)) {
            return allowIntPromotion || (typeIsSigned(arg) == typeIsSigned(stack) && intBitWidth(arg) == intBitWidth(stack));
        }

        if (arg == "any" || arg == "[any]" || (argIsNilable && arg != "float" && (stack == "any" || stack == "[any]"))) {
            return true;
        }

        Interface* interface = getInterfaceByName(result, arg);
        if (stackTypeIsNilable && !argIsNilable) {
            return false;
        } else if (interface) {
            const Struct& givenType = getStructByName(result, stack);
            if (givenType == Struct::Null) {
                return false;
            } else if (!givenType.implements(interface->name)) {
                return false;
            }
        } else if (!typeEquals(stack, arg)) {
            const Struct& givenType = getStructByName(result, stack);
            const Struct& requiredType = getStructByName(result, arg);
            if (givenType == Struct::Null) {
                return false;
            } else if (requiredType == Struct::Null) {
                return false;
            } else if (!canBeCastTo(result, givenType, requiredType)) {
                return false;
            }
        }
        return true;
    }

    bool checkStackType(TPResult& result, std::vector<Variable>& args, bool allowIntPromotion) {
        if (args.size() == 0) {
            return true;
        }
        if (typeStack.size() < args.size()) {
            return false;
        }

        if (args.back().type == "varargs") {
            return true;
        }

        auto end = &typeStack.top() + 1;
        auto begin = end - args.size();
        std::vector<std::string> stack(begin, end);

        for (ssize_t i = args.size() - 1; i >= 0; i--) {
            bool tmp = typesCompatible(result, stack[i], args[i].type, allowIntPromotion);
            if (tmp == false) {
                return false;
            }
        }
        return true;
    }

    std::string stackSliceToString(size_t amount) {
        if (typeStack.size() < amount) amount = typeStack.size();

        if (amount == 0)
            return "";
        auto end = &typeStack.top() + 1;
        auto begin = end - amount;
        std::vector<std::string> tmp(begin, end);

        std::string arg = "";
        for (size_t i = 0; i < amount; i++) {
            std::string stackType = tmp[i];
            
            if (i) {
                arg += ", ";
            }

            arg += stackType;
        }
        return arg;
    }

    bool argsContainsIntType(std::vector<Variable>& args) {
        for (auto&& arg : args) {
            if (isPrimitiveIntegerType(arg.type)) {
                return true;
            }
        }
        return false;
    }

    std::map<std::string, std::string> getTemplates(TPResult& result, Function* func) {
        if (!func->isMethod) {
            return std::map<std::string, std::string>();
        }
        Struct s = getStructByName(result, func->member_type);
        return s.templates;
    }


    bool hasTypealias(TPResult& r, std::string t) {
        t = removeTypeModifiers(t);
        return r.typealiases.find(t) != r.typealiases.end();
    }

    std::string getTypealias(TPResult& r, std::string t) {
        t = removeTypeModifiers(t);
        return r.typealiases[t];
    }

    std::string removeTypeModifiers(std::string t) {
        if (t.size() && t.front() == '*') {
            t.erase(0, 1);
        }
        for (const std::string& modifier : removableTypeModifiers) {
            while (t.compare(0, modifier.size(), modifier) == 0) {
                t.erase(0, modifier.size());
            }
        }
        if (t.size() && t != "?" && t.back() == '?') {
            t.pop_back();
        }
        return t;
    }

    std::string sclTypeToCType(TPResult& result, std::string t) {
        if (t == "?") {
            return "scl_any";
        }
        bool valueType = t.front() == '*';
        t = removeTypeModifiers(t);

        if (strstarts(t, "async<")) return "scl_any";
        if (strstarts(t, "lambda(")) return "_scl_lambda";
        if (t == "any") return "scl_any";
        if (t == "none") return "void";
        if (t == "nothing") return "_scl_no_return void";
        if (t == "int") return "scl_int";
        if (t == "uint") return "scl_uint";
        if (t == "float") return "scl_float";
        if (t == "bool") return "scl_bool";
        if (t == "varargs") return "...";
        if (!(getStructByName(result, t) == Struct::Null)) {
            if (valueType) {
                return "struct Struct_" + t;
            }
            return "scl_" + t;
        }
        if (t.size() > 2 && t.front() == '[') {
            return sclTypeToCType(result, t.substr(1, t.size() - 2)) + "*";
        }
        if (getInterfaceByName(result, t)) {
            if (Main::options::noScaleFramework) {
                return "scl_any";
            }
            if (valueType) {
                return "struct Struct_SclObject";
            }
            return "scl_SclObject";
        }
        if (hasTypealias(result, t)) {
            return "ta_" + t;
        }
        if (hasEnum(result, t)) {
            return "scl_int";
        }
        if (hasLayout(result, t)) {
            if (valueType) {
                return "struct Layout_" + t;
            }
            return "scl_" + t;
        }

        return "scl_any";
    }

    std::string sclIntTypeToConvert(std::string type) {
        if (type == "int") return "";
        if (type == "uint") return "fn_int$toUInt";
        if (type == "int8") return "fn_int$toInt8";
        if (type == "int16") return "fn_int$toInt16";
        if (type == "int32") return "fn_int$toInt32";
        if (type == "int64") return "";
        if (type == "uint8") return "fn_int$toUInt8";
        if (type == "uint16") return "fn_int$toUInt16";
        if (type == "uint32") return "fn_int$toUInt32";
        if (type == "uint64") return "";
        return "never_happens";
    }


    std::string rtTypeToSclType(std::string rtType) {
        if (rtType.size() == 0) return "";
        if (rtType == "a") return "any";
        if (rtType == "i") return "int";
        if (rtType == "f") return "float";
        if (rtType == "s") return "str";
        if (rtType == "V") return "none";
        if (rtType == "cs") return "[int8]";
        if (rtType == "p") return "[any]";
        if (rtType == "F") return "lambda";
        if (rtType.front() == '[') {
            return "[" + rtTypeToSclType(rtType.substr(1)) + "]";
        }
        if (rtType == "u") return "uint";
        if (rtType == "i8") return "int8";
        if (rtType == "u8") return "uint8";
        if (rtType == "i16") return "int16";
        if (rtType == "u16") return "uint16";
        if (rtType == "i32") return "int32";
        if (rtType == "u32") return "uint32";
        if (rtType == "i64") return "int64";
        if (rtType == "u64") return "uint64";
        if (rtType.front() == 'L') {
            return rtType.substr(1, rtType.size() - 1);
        }
        if (rtType.front() == 'P') {
            return "*" + rtTypeToSclType(rtType.substr(1));
        }
        return "<" + rtType + ">";
    }

    std::string typeToRTSig(std::string type) {
        if (type.size() && type.front() == '*') {
            type = removeTypeModifiers(type.substr(1, type.size() - 1));
            return "P" + typeToRTSig(type);
        }
        type = removeTypeModifiers(type);
        if (type == "any") return "a;";
        if (type == "int" || type == "bool") return "i;";
        if (type == "float") return "f;";
        if (type == "str") return "s;";
        if (type == "none") return "V;";
        if (type == "[int8]") return "cs;";
        if (type == "[any]") return "p;";
        if (type == "lambda" || strstarts(type, "lambda(")) return "F;";
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            return "[" + typeToRTSig(type.substr(1, type.size() - 2));
        }
        if (type == "uint") return "u;";
        if (type == "int8") return "i8;";
        if (type == "int16") return "i16;";
        if (type == "int32") return "i32;";
        if (type == "int64") return "i64;";
        if (type == "uint8") return "u8;";
        if (type == "uint16") return "u16;";
        if (type == "uint32") return "u32;";
        if (type == "uint64") return "u64;";
        if (currentStruct.templates.find(type) != currentStruct.templates.end()) {
            return typeToRTSig(currentStruct.templates[type]);
        }
        return "L" + type + ";";
    }

    std::string argsToRTSignature(Function* f) {
        std::string args = "(";
        for (const Variable& v : f->args) {
            if (v.name == "self" && f->isMethod) {
                continue;
            }
            args += typeToRTSig(v.type);
        }
        args += ")" + typeToRTSig(f->return_type);
        return args;
    }

    std::string typeToRTSigIdent(std::string type) {
        type = removeTypeModifiers(type);
        if (type == "any") return "a$";
        if (type == "int" || type == "bool") return "i$";
        if (type == "float") return "f$";
        if (type == "str") return "s$";
        if (type == "none") return "V$";
        if (type == "[int8]") return "cs$";
        if (type == "[any]") return "p$";
        if (strstarts(type, "async<")) return "A" + typeToRTSigIdent(type.substr(6, type.size() - 7));
        if (type == "lambda" || strstarts(type, "lambda(")) return "F$";
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            return "A" + typeToRTSigIdent(type.substr(1, type.size() - 2));
        }
        if (type == "uint") return "u$";
        if (type == "int8") return "i8$";
        if (type == "int16") return "i16$";
        if (type == "int32") return "i32$";
        if (type == "int64") return "i64$";
        if (type == "uint8") return "u8$";
        if (type == "uint16") return "u16$";
        if (type == "uint32") return "u32$";
        if (type == "uint64") return "u64$";
        if (currentStruct.templates.find(type) != currentStruct.templates.end()) {
            return typeToRTSigIdent(currentStruct.templates[type]);
        }
        return "L" + type + "$";
    }

    std::string argsToRTSignatureIdent(Function* f) {
        std::string args = "$$";
        for (const Variable& v : f->args) {
            std::string type = removeTypeModifiers(v.type);
            if (v.name == "self" && f->isMethod) {
                continue;
            }
            args += typeToRTSigIdent(type);
        }
        args += "$$";
        args += typeToRTSigIdent(f->return_type);
        return args;
    }

    bool argVecEquals(std::vector<Variable>& a, std::vector<Variable>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (size_t i = 0; i < a.size(); i++) {
            if (a[i].name == "self" || b[i].name == "self") {
                continue;
            }
            if (a[i].type != b[i].type) {
                return false;
            }
        }
        return true;
    }

} // namespace sclc
