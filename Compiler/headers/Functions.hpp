#pragma once

#include "Common.hpp"

namespace sclc {
    void createVariadicCall(Function* f, std::ostream& fp, TPResult& result, std::vector<FPResult>& errors, std::vector<Token> body, size_t& i);
    std::string generateArgumentsForFunction(TPResult& result, Function *func);
    std::string generateSymbolForFunction(Function* f);
    Method* findMethodLocally(Method* self, TPResult& result);
    Function* findFunctionLocally(Function* self, TPResult& result);
    std::string getFunctionType(TPResult& result, Function* self);
    void methodCall(Method* self, std::ostream& fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i, bool ignoreArgs = false, bool doActualPop = true, bool withIntPromotion = false, bool onSuperType = false, bool checkOverloads = true);
    void generateUnsafeCallF(Function* self, std::ostream& fp, TPResult& result);
    void generateUnsafeCall(Method* self, std::ostream& fp, TPResult& result);
    bool hasImplementation(TPResult& result, Function* func);
    bool shouldCall(Function* self, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t i);
    std::string opToString(std::string op);
    void functionCall(Function* self, std::ostream& fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i, bool withIntPromotion = false, bool hasToCallStatic = false, bool checkOverloads = true);
    std::string sclFunctionNameToFriendlyString(std::string name);
    std::string sclFunctionNameToFriendlyString(Function* f);
    std::string sclGenCastForMethod(TPResult& result, Method* m);
    std::vector<Method*> makeVTable(TPResult& res, std::string name);
} // namespace sclc
