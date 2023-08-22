#pragma once

#include "Common.hpp"

namespace sclc {
    void createVariadicCall(Function* f, FILE* fp, TPResult& result, std::vector<FPResult>& errors, std::vector<Token> body, size_t& i);
    std::string generateArgumentsForFunction(TPResult& result, Function *func);
    std::string generateSymbolForFunction(Function* f);
    Method* findMethodLocally(Method* self, TPResult& result);
    Function* findFunctionLocally(Function* self, TPResult& result);
    std::string getFunctionType(TPResult& result, Function* self);
    void methodCall(Method* self, FILE* fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t i, bool ignoreArgs = false, bool doActualPop = true, bool withIntPromotion = false, bool onSuperType = false, bool checkOverloads = true);
    void generateUnsafeCallF(Function* self, FILE* fp, TPResult& result);
    void generateUnsafeCall(Method* self, FILE* fp, TPResult& result);
    bool hasImplementation(TPResult& result, Function* func);
    bool shouldCall(Function* self, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t i);
    bool opFunc(std::string name);
    std::string opToString(std::string op);
    void functionCall(Function* self, FILE* fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t i, bool withIntPromotion = false, bool hasToCallStatic = false, bool checkOverloads = true);
    bool operatorInt(const std::string& op);
    bool operatorFloat(const std::string& op);
    bool operatorBool(const std::string& op);
    std::string sclFunctionNameToFriendlyString(std::string name);
    std::string sclFunctionNameToFriendlyString(Function* f);
    std::string sclGenCastForMethod(TPResult& result, Method* m);
    std::vector<Method*> makeVTable(TPResult& res, std::string name);
} // namespace sclc
