#pragma once

#include <Common.hpp>

namespace sclc {
    void createVariadicCall(Ptr<Function> f, std::ostream& fp, TPResult& result, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i);
    std::string generateArgumentsForFunction(TPResult& result, Function *func);
    std::string generateSymbolForFunction(Ptr<Function> f);
    Ptr<Method> findMethodLocally(Ptr<Method> self, TPResult& result);
    Ptr<Function> findFunctionLocally(Ptr<Function> self, TPResult& result);
    std::string getFunctionType(TPResult& result, Ptr<Function> self);
    void methodCall(Ptr<Method> self, std::ostream& fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i, bool ignoreArgs = false, bool doActualPop = true, bool withIntPromotion = false, bool onSuperType = false, bool checkOverloads = true);
    bool hasImplementation(TPResult& result, Ptr<Function> func);
    bool shouldCall(Ptr<Function> self, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t i);
    std::string opToString(std::string op);
    bool opFunc(std::string name);
    void functionCall(Ptr<Function> self, std::ostream& fp, TPResult& result, std::vector<FPResult>& warns, std::vector<FPResult>& errors, std::vector<Token>& body, size_t& i, bool withIntPromotion = false, bool hasToCallStatic = false, bool checkOverloads = true);
    std::string sclFunctionNameToFriendlyString(std::string name);
    std::string sclFunctionNameToFriendlyString(Ptr<Function> f);
    std::string sclGenCastForMethod(TPResult& result, Ptr<Method> m);
    std::vector<Ptr<Method>> makeVTable(TPResult& res, std::string name);
} // namespace sclc
