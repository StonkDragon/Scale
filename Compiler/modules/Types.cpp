
#include <array>

#include <Common.hpp>
#include <TokenType.hpp>
#include <TranspilerDefs.hpp>
#include <Types.hpp>

namespace sclc {
    extern std::vector<std::string> typeStack;
    extern Struct currentStruct;

    const std::array<std::string, 2> removableTypeModifiers{"const ", "readonly "};

    std::string notNilTypeOf(std::string t) {
        if (t == "?") return t;
        while (t.back() == '?') t.pop_back();
        return t;
    }

    std::string typePointedTo(std::string type) {
        if (type.front() == '[' && type.back() == ']') {
            return type.substr(1, type.size() - 2);
        }
        if (type.front() == '*') {
            return type.substr(1);
        }
        return "any";
    }

    std::string primitiveToReference(std::string type) {
        type = removeTypeModifiers(type);
        type = notNilTypeOf(type);
        if (!isPrimitiveType(type)) {
            return type;
        }
        if (isPrimitiveIntegerType(type, false) || type == "bool") {
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

    bool isPrimitiveIntegerType(std::string s, bool rem) {
        if (rem) s = removeTypeModifiers(s);
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

    bool binaryCompatible(Function* a, Function* b) {
        if (!typeEquals(a->return_type, b->return_type)) return false;
        if (a->args.size() != b->args.size()) return false;
        for (size_t i = 0; i < a->args.size(); i++) {
            if (!typeEquals(a->args[i].type, b->args[i].type)) return false;
        }
        return true;
    }

    std::string retemplate(std::string type);

    std::string argVectorToString(std::vector<Variable>& args) {
        std::string arg = "";
        for (size_t i = 0; i < args.size(); i++) {
            if (i) 
                arg += ", ";
            const Variable& v = args[i];
            if (v.type.front() == '@') {
                arg += "@";
            }
            arg += retemplate(removeTypeModifiers(v.type));
            if (v.type.back() == '?') {
                arg += "?";
            }
        }
        return arg;
    }

    std::string argVectorToString(std::vector<std::string>& args) {
        std::string arg = "";
        for (size_t i = 0; i < args.size(); i++) {
            if (i) 
                arg += ", ";
            if (args[i].front() == '@') {
                arg += "@";
            }
            arg += retemplate(removeTypeModifiers(args[i]));
            if (args[i].back() == '?') {
                arg += "?";
            }
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
            if (isDigit(args.front())) {
                return std::stoi(args);
            } else {
                size_t count = 1;
                size_t depth = 0;
                for (char c : args) {
                    if (c == '(') {
                        depth++;
                    } else if (c == ')') {
                        depth--;
                    } else if (c == ',' && depth == 0) {
                        count++;
                    }
                }
                return count;
            }
        }
        return -1;
    }

    std::vector<Variable> lambdaArgs(const std::string& lambda) {
        if (strstarts(lambda, "lambda(")) {
            std::string argsString = lambda.substr(lambda.find_first_of("(") + 1, lambda.find_last_of("):") - lambda.find_first_of("(") - 1);
            std::vector<Variable> args;
            if (isDigit(argsString.front())) {
                size_t count = std::stoi(argsString);
                std::cout << "Old lambda detected with " << count << " args" << std::endl;
                for (size_t i = 0; i < count; i++) {
                    args.push_back(Variable{"", "any"});
                }
            } else {
                // Parse all arguments from the lambda
                // The lambda is in the format 'lambda(arg1,arg2,arg3):returntype'
                std::string a = lambda.substr(lambda.find_first_of("(") + 1, lambda.find_last_of("):") - lambda.find_first_of("(") - 1);
                // a = arg1,arg2,arg3
                // split by commas
                // each arg may be of lambda type, so ignore commas in parentheses
                size_t depth = 0;
                size_t start = 0;
                for (size_t i = 0; i < a.size(); i++) {
                    if (a[i] == '(') {
                        depth++;
                    } else if (a[i] == ')') {
                        depth--;
                    } else if (a[i] == ',' && depth == 0) {
                        args.push_back(Variable{"", a.substr(start, i - start)});
                        start = i + 1;
                    }
                }
                std::string arg = a.substr(start, a.size() - start - 1);
                if (!arg.empty()) {
                    args.push_back(Variable{"", arg});
                }
            }
            return args;
        }
        return {};
    }

    bool lambdasEqual(const std::string& a, const std::string& b) {
        std::string returnTypeA = lambdaReturnType(a);
        std::string returnTypeB = lambdaReturnType(b);

        auto argsA = lambdaArgs(a);
        auto argsB = lambdaArgs(b);

        if (argsA.size() != argsB.size()) {
            return false;
        }

        for (size_t i = 0; i < argsA.size(); i++) {
            if (!typeEquals(argsA[i].type, argsB[i].type)) {
                return false;
            }
        }

        return typeEquals(returnTypeA, returnTypeB);
    }

    bool typeEquals(const std::string& a, const std::string& b) {
        if (a.empty() || b.empty()) {
            return false;
        }
        if (b == "any") {
            return true;
        }
        if (a == "?" || b == "?") {
            return true;
        }

        std::string a2 = removeTypeModifiers(a);
        std::string b2 = removeTypeModifiers(b);
        if (a2 == b2) {
            return true;
        } else if (isPrimitiveIntegerType(a2, false) && isPrimitiveIntegerType(b2, false)) {
            return intBitWidth(a2) == intBitWidth(b2) && a2.front() == b2.front();
        }
        if (a2.front() == '[') {
            std::string aType = a2.substr(1, a2.size() - 2);
            if (b2.front() == '[') {
                std::string bType = b2.substr(1, b2.size() - 2);
                return typeEquals(aType, bType);
            }
        } else if (a2.front() == '*') {
            std::string aType = a2.substr(1);
            if (b2.front() == '*') {
                std::string bType = b2.substr(1);
                return typeEquals(aType, bType);
            }
        } else if (strstarts(a2, "lambda(") && strstarts(b2, "lambda(")) {
            return lambdasEqual(a2, b2);
        }
        return false;
    }

    bool typeIsUnsigned(std::string s) {
        s = removeTypeModifiers(s);
        return isPrimitiveIntegerType(s, false) && s.front() == 'u';
    }

    bool typeIsSigned(std::string s) {
        s = removeTypeModifiers(s);
        return isPrimitiveIntegerType(s, false) && s.front() == 'i';
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
        if (allowIntPromotion && arg == "bool") arg = "int";
        if (allowIntPromotion && stack == "bool") stack = "int";
        if (typeEquals(stack, arg) && stackTypeIsNilable == argIsNilable) {
            return true;
        }

        if (isPrimitiveIntegerType(arg) && isPrimitiveIntegerType(stack)) {
            return allowIntPromotion || (typeIsSigned(arg) == typeIsSigned(stack) && intBitWidth(arg) == intBitWidth(stack));
        }

        if (arg == "any" || (argIsNilable && arg != "float" && arg != "float32" && (stack == "any" || stack == "[any]"))) {
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
        if (args.empty()) {
            return true;
        }
        if (typeStack.size() < args.size()) {
            return false;
        }

        if (args.back().type == "varargs") {
            return true;
        }

        size_t startIndex = typeStack.size() - args.size();

        for (ssize_t i = args.size() - 1; i >= 0; i--) {
            if (!typesCompatible(result, typeStack[startIndex + i], args[i].type, allowIntPromotion)) {
                return false;
            }
        }
        return true;
    }

    bool checkConstable(TPResult& result, std::vector<Variable>& args) {
        (void) result;
        if (args.empty()) {
            return true;
        }
        if (typeStack.size() < args.size()) {
            return false;
        }

        if (args.back().type == "varargs") {
            return true;
        }

        size_t startIndex = typeStack.size() - args.size();

        for (ssize_t i = args.size() - 1; i >= 0; i--) {
            bool stackIsConst = typeIsConst(typeStack[startIndex + i]);
            bool argIsConst = typeIsConst(args[i].type);
            if (stackIsConst && !argIsConst) {
                return false;
            }
        }
        return true;
    }

    std::string stackSliceToString(size_t amount) {
        if (typeStack.size() < amount) amount = typeStack.size();

        if (amount == 0)
            return "";

        std::string arg = "";
        for (size_t i = 0; i < amount; i++) {
            std::string stackType = typeStack[typeStack.size() - amount + i];
            
            if (i) {
                arg += ", ";
            }

            arg += retemplate(stackType);
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

    bool hasTypealias(TPResult& r, std::string t) {
        t = removeTypeModifiers(t);
        return r.typealiases.find(t) != r.typealiases.end();
    }

    std::string getTypealias(TPResult& r, std::string t) {
        t = removeTypeModifiers(t);
        return r.typealiases[t].first;
    }

    bool typealiasCanBeNil(TPResult& r, std::string t) {
        t = removeTypeModifiers(t);
        return r.typealiases[t].second;
    }

    std::string removeTypeModifiers(std::string t) {
        if (t.size() && t.front() == '@') {
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
        static std::unordered_map<std::string, std::string> cache = {
            std::pair("any", "scale_any"),
            std::pair("none", "void"),
            std::pair("nothing", "scale_no_return void"),
            std::pair("int", "scale_int"),
            std::pair("int8", "scale_int8"),
            std::pair("[int8]", "scale_int8*"),
            std::pair("[str]", "scale_str*"),
            std::pair("SclObject", "scale_SclObject"),
            std::pair("str", "scale_str"),
            std::pair("int16", "scale_int16"),
            std::pair("int32", "scale_int32"),
            std::pair("int64", "scale_int64"),
            std::pair("uint", "scale_uint"),
            std::pair("uint8", "scale_uint8"),
            std::pair("uint16", "scale_uint16"),
            std::pair("uint32", "scale_uint32"),
            std::pair("uint64", "scale_uint64"),
            std::pair("float", "scale_float"),
            std::pair("bool", "scale_bool"),
            std::pair("varargs", "..."),
            std::pair("?", "scale_any"),
        };

        std::string key = t;

        auto it = cache.find(t);
        if (it != cache.end()) return it->second;

        bool valueType = t.front() == '@';
        t = removeTypeModifiers(t);

        if (strstarts(t, "async<")) cache[key] = ("scale_any");
        else if (strstarts(t, "lambda(")) cache[key] = ("scale_lambda");
        else if (t.size() > 2 && t.front() == '[') {
            cache[key] = (sclTypeToCType(result, t.substr(1, t.size() - 2)) + "*");
        } else if (!t.empty() && t.front() == '*') {
            cache[key] = (sclTypeToCType(result, t.substr(1)) + "*");
        } else if (!(getStructByName(result, t) == Struct::Null)) {
            if (valueType) {
                cache[key] = ("struct Struct_" + t);
            } else {
                cache[key] = ("scale_" + t);
            }
        } else if (getInterfaceByName(result, t)) {
            if (Main::options::noScaleFramework) {
                cache[key] = ("scale_any");
            } else if (valueType) {
                cache[key] = ("struct Struct_SclObject");
            } else {
                cache[key] = ("scale_SclObject");
            }
        } else if (hasTypealias(result, t)) {
            cache[key] = ("ta_" + t);
        } else if (hasEnum(result, t)) {
            cache[key] = ("scale_int");
        } else if (hasLayout(result, t)) {
            if (valueType) {
                cache[key] = ("struct Layout_" + t);
            } else {
                cache[key] = ("scale_" + t);
            }
        } else {
            cache[key] = ("scale_any");
        }

        return cache[key];
    }

    std::string sclIntTypeToConvert(std::string type) {
        if (type == "int") return "";
        if (type == "uint") return "_F10int$toUIntlL";
        if (type == "int8") return "_F10int$toInt8lb";
        if (type == "int16") return "_F11int$toInt16ls";
        if (type == "int32") return "_F11int$toInt32li";
        if (type == "int64") return "";
        if (type == "uint8") return "_F11int$toUInt8lB";
        if (type == "uint16") return "_F12int$toUInt16lS";
        if (type == "uint32") return "_F12int$toUInt32lI";
        if (type == "uint64") return "";
        return "never_happens";
    }


    std::string rtTypeToSclType(std::string rtType) {
        if (rtType.empty()) return "";
        if (rtType == "a") return "any";
        if (rtType == "i") return "int";
        if (rtType == "g") return "bool";
        if (rtType == "f") return "float";
        if (rtType == "f32") return "float32";
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
            return "@" + rtTypeToSclType(rtType.substr(1));
        }
        if (rtType.front() == 'r') {
            return "*" + rtTypeToSclType(rtType.substr(1));
        }
        return "<" + rtType + ">";
    }

    std::string typeToRTSig(std::string type) {
        if (type.size() && type.front() == '@') {
            return "P" + typeToRTSig(type.substr(1));
        }
        type = removeTypeModifiers(type);
        if (type == "any") return "a;";
        if (type == "int") return "i;";
        if (type == "bool") return "g;";
        if (type == "float") return "f;";
        if (type == "float32") return "f32;";
        if (type == "str") return "s;";
        if (type == "none") return "V;";
        if (type == "[int8]") return "cs;";
        if (type == "[any]") return "p;";
        if (type == "lambda" || strstarts(type, "lambda(")) return "F;";
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            return "[" + typeToRTSig(type.substr(1, type.size() - 2));
        }
        if (!type.empty() && type.front() == '*') {
            return "r" + typeToRTSig(type.substr(1));
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
        if (type == "?") return "?;";
        return "L" + type + ";";
    }

    std::string typeToSymbol(std::string type) {
        static std::unordered_map<std::string, std::string> cache = {
            {"any", "a"},
            {"bool", "g"},
            {"int", "l"},
            {"int8", "b"},
            {"int16", "s"},
            {"int32", "i"},
            {"int64", "l"},
            {"uint", "L"},
            {"uint8", "B"},
            {"uint16", "S"},
            {"uint32", "I"},
            {"uint64", "L"},
            {"float", "d"},
            {"float32", "f"},
            {"str", "E"},
            {"none", "v"},
            {"[int8]", "c"},
            {"[any]", "p"},
            {"?", "Q"},
            {"lambda", "F"},
        };

        type = removeTypeModifiers(type);
        auto it = cache.find(type);
        if (it != cache.end()) return it->second;

        if (type.size() && type.front() == '@') {
            cache[type] = "P" + typeToSymbol(type.substr(1));
        } else if (strstarts(type, "lambda(")) {
            cache[type] = "F";
        } else if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            cache[type] =  "A" + typeToSymbol(type.substr(1, type.size() - 2));
        } else if (!type.empty() && type.front() == '*') {
            cache[type] =  "r" + typeToSymbol(type.substr(1));
        } else {
            cache[type] = std::to_string(type.length()) + type;
        }

        return cache[type];
    }

    std::string argsToRTSignature(Function* f) {
        static std::unordered_map<Function*, std::string> cache;

        auto it = cache.find(f);
        if (it != cache.end()) return it->second;

        std::string args = "(";
        for (const Variable& v : f->args) {
            if (f->isMethod && v.name == "self") {
                continue;
            }
            args += typeToRTSig(v.type);
        }
        args += ")" + typeToRTSig(f->return_type);
        return cache[f] = args;
    }

    std::string typeToRTSigIdent(std::string type) {
        if (type.front() == '@') {
            return "P" + typeToRTSigIdent(type.substr(1));
        }
        type = removeTypeModifiers(type);
        if (type == "any") return "a$";
        if (type == "int") return "i$";
        if (type == "bool") return "g$";
        if (type == "float") return "f$";
        if (type == "float32") return "f32$";
        if (type == "str") return "s$";
        if (type == "none") return "V$";
        if (type == "[int8]") return "cs$";
        if (type == "[any]") return "p$";
        if (strstarts(type, "async<")) return "A" + typeToRTSigIdent(type.substr(6, type.size() - 7));
        if (type == "lambda" || strstarts(type, "lambda(")) return "F$";
        if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
            return "A" + typeToRTSigIdent(type.substr(1, type.size() - 2));
        }
        if (!type.empty() && type.front() == '*') {
            return "r" + typeToRTSigIdent(type.substr(1));
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
        if (type == "?") return "Q$";
        return "L" + type + "$";
    }

    std::string argsToRTSignatureIdent(Function* f) {
        static std::unordered_map<Function*, std::string> cache;
        auto it = cache.find(f);
        if (it != cache.end()) return it->second;

        std::string args;
        for (const Variable& v : f->args) {
            std::string type = removeTypeModifiers(v.type);
            if (v.name == "self" && f->isMethod) {
                continue;
            }
            args += typeToRTSigIdent(type);
        }
        args += typeToRTSigIdent(f->return_type);
        return cache[f] = args;
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
