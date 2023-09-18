#pragma once
#include "Common.hpp"

namespace sclc {
    std::string notNilTypeOf(std::string t);
    std::string typePointedTo(std::string type);
    std::string primitiveToReference(std::string type);
    bool structExtends(TPResult& result, Struct& s, std::string name);
    bool isPrimitiveIntegerType(std::string s);
    bool argsAreIdentical(std::vector<Variable>& methodArgs, std::vector<Variable>& functionArgs);
    std::string argVectorToString(std::vector<Variable>& args);
    bool canBeCastTo(TPResult& r, const Struct& one, const Struct& other);
    std::string lambdaReturnType(const std::string& lambda);
    size_t lambdaArgCount(const std::string& lambda);
    bool lambdasEqual(const std::string& a, const std::string& b);
    bool typeEquals(const std::string& a, const std::string& b);
    bool typeIsUnsigned(std::string s);
    bool typeIsSigned(std::string s);
    size_t intBitWidth(std::string s);
    bool typesCompatible(TPResult& result, std::string stack, std::string arg, bool allowIntPromotion);
    bool checkStackType(TPResult& result, std::vector<Variable>& args, bool allowIntPromotion = false);
    std::string stackSliceToString(size_t amount);
    bool argsContainsIntType(std::vector<Variable>& args);
    std::map<std::string, std::string> getTemplates(TPResult& result, Function* func);
    bool hasTypealias(TPResult& r, std::string t);
    std::string getTypealias(TPResult& r, std::string t);
    std::string removeTypeModifiers(std::string t);
    std::string sclTypeToCType(TPResult& result, std::string t);
    std::string sclIntTypeToConvert(std::string type);
    std::string rtTypeToSclType(std::string rtType);
    std::string typeToRTSig(std::string type);
    std::string argsToRTSignature(Function* f);
    std::string typeToRTSigIdent(std::string type);
    std::string argsToRTSignatureIdent(Function* f);
    bool argVecEquals(std::vector<Variable>& a, std::vector<Variable>& b);
    std::string selfTypeToRealType(std::string selfType, std::string realType);
    bool isSelfType(std::string type);
} // namespace sclc
