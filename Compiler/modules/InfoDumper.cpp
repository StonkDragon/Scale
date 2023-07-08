#include <filesystem>

#include "../headers/Common.hpp"

namespace sclc {

    #define print(s) std::cout << s << std::endl

    template<typename T>
    std::vector<T> joinVecs(std::vector<T> a, std::vector<T> b) {
        std::vector<T> ret;
        for (T& t : a) {
            ret.push_back(t);
        }
        for (T& t : b) {
            ret.push_back(t);
        }
        return ret;
    }

    void binaryHeader(TPResult result) {
        remove(Main.options.outfile.c_str());
        FILE* f = fopen(Main.options.outfile.c_str(), "ab");
        if (!f) {
            std::cerr << "Failed to open file for writing: '" << Main.options.outfile << "'" << std::endl;
            return;
        }

        char magic[] = {1, 'S', 'C', 'L'};
        fwrite(magic, sizeof(char), 4, f);

        uint32_t version = Main.version->major << 16 | Main.version->minor << 8 | Main.version->patch;
        fwrite(&version, sizeof(uint32_t), 1, f);

        uint32_t numContainers = 0;
        for (Container c : result.containers) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(c.name_token->file).string())) {
                numContainers++;
            }
        }
        fwrite(&numContainers, sizeof(uint32_t), 1, f);

        uint32_t numStructs = 0;
        for (Struct s : result.structs) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(s.name_token.file).string())) {
                numStructs++;
            }
        }
        fwrite(&numStructs, sizeof(uint32_t), 1, f);

        uint32_t numEnums = 0;
        for (Enum e : result.enums) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(e.name_token->file).string())) {
                numEnums++;
            }
        }
        fwrite(&numEnums, sizeof(uint32_t), 1, f);

        uint32_t numFunctions = 0;
        for (Function* f : joinVecs(result.functions, result.extern_functions)) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(f->nameToken.file).string())) {
                numFunctions++;
            }
        }
        fwrite(&numFunctions, sizeof(uint32_t), 1, f);

        uint32_t numGlobals = 0;
        for (Variable v : result.globals) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(v.name_token->file).string())) {
                numGlobals++;
            }
        }
        fwrite(&numGlobals, sizeof(uint32_t), 1, f);

        uint32_t numExternGlobals = 0;
        for (Variable v : result.extern_globals) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(v.name_token->file).string())) {
                numExternGlobals++;
            }
        }
        fwrite(&numExternGlobals, sizeof(uint32_t), 1, f);

        uint32_t numInterfaces = 0;
        for (Interface* i : result.interfaces) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(i->name_token->file).string())) {
                numInterfaces++;
            }
        }
        fwrite(&numInterfaces, sizeof(uint32_t), 1, f);

        uint32_t numLayouts = 0;
        for (Layout l : result.layouts) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(l.getNameToken().file).string())) {
                numLayouts++;
            }
        }
        fwrite(&numLayouts, sizeof(uint32_t), 1, f);

        uint32_t numTypealiases = result.typealiases.size();
        fwrite(&numTypealiases, sizeof(uint32_t), 1, f);

        for (Container c : result.containers) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(c.name_token->file).string())) {
                continue;
            }

            uint32_t nameLength = c.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(c.getName().c_str(), sizeof(char), nameLength, f);
            uint32_t numMembers = c.getMembers().size();
            fwrite(&numMembers, sizeof(uint32_t), 1, f);
            for (Variable v : c.getMembers()) {
                uint32_t memberNameLength = v.getName().length();
                fwrite(&memberNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.getName().c_str(), sizeof(char), memberNameLength, f);

                uint32_t memberTypeLength = v.getType().length();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.getType().c_str(), sizeof(char), memberTypeLength, f);
            }
        }
        for (Struct s : result.structs) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(s.name_token.file).string())) {
                continue;
            }

            uint32_t nameLength = s.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(s.getName().c_str(), sizeof(char), nameLength, f);

            fwrite(&(s.flags), sizeof(int), 1, f);

            uint32_t numMembers = s.getMembers().size();
            fwrite(&numMembers, sizeof(uint32_t), 1, f);

            for (Variable v : s.getMembers()) {
                uint32_t memberNameLength = v.getName().length();
                fwrite(&memberNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.getName().c_str(), sizeof(char), memberNameLength, f);

                uint32_t memberTypeLength = v.getType().length();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.getType().c_str(), sizeof(char), memberTypeLength, f);
            }
        }
        for (Enum e : result.enums) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(e.name_token->file).string())) {
                continue;
            }

            uint32_t nameLength = e.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(e.getName().c_str(), sizeof(char), nameLength, f);

            uint32_t numMembers = e.getMembers().size();
            fwrite(&numMembers, sizeof(uint32_t), 1, f);
            for (uint32_t i = 0; i < numMembers; i++) {
                uint32_t memberNameLength = e.getMembers()[i].length();
                fwrite(&memberNameLength, sizeof(uint32_t), 1, f);
                fwrite(e.getMembers()[i].c_str(), sizeof(char), memberNameLength, f);
            }
        }
        for (Function* func : joinVecs(result.functions, result.extern_functions)) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(func->nameToken.file).string())) {
                continue;
            }

            uint8_t isMethod = func->isMethod;
            fwrite(&isMethod, sizeof(uint8_t), 1, f);
            
            uint32_t nameLength = func->getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(func->getName().c_str(), sizeof(char), nameLength, f);

            if (isMethod) {
                Method* m = static_cast<Method*>(func);
                uint32_t memberTypeLength = m->getMemberType().length();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->getMemberType().c_str(), sizeof(char), memberTypeLength, f);
            }

            uint32_t returnTypeLength = func->getReturnType().length();
            fwrite(&returnTypeLength, sizeof(uint32_t), 1, f);
            fwrite(func->getReturnType().c_str(), sizeof(char), returnTypeLength, f);

            auto mods = func->getModifiers();
            uint32_t numMods = mods.size();

            if (isMethod && ((Method*) func)->addAnyway()) {
                numMods++;
            }

            fwrite(&numMods, sizeof(uint32_t), 1, f);

            if (isMethod && ((Method*) func)->addAnyway()) {
                uint32_t modLength = 9;
                fwrite(&modLength, sizeof(uint32_t), 1, f);
                fwrite("<virtual>", sizeof(char), modLength, f);
            }

            for (std::string mod : mods) {
                uint32_t modLength = mod.length();
                fwrite(&modLength, sizeof(uint32_t), 1, f);
                fwrite(mod.c_str(), sizeof(char), modLength, f);
            }

            uint32_t numArgs = func->getArgs().size();
            fwrite(&numArgs, sizeof(uint32_t), 1, f);

            for (Variable v : func->getArgs()) {
                uint32_t argTypeLength = v.getType().length();
                fwrite(&argTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.getType().c_str(), sizeof(char), argTypeLength, f);
            }
        }
        for (Variable v : result.globals) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(v.name_token->file).string())) {
                continue;
            }

            uint32_t nameLength = v.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(v.getName().c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = v.getType().length();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(v.getType().c_str(), sizeof(char), typeLength, f);
        }
        for (Variable v : result.extern_globals) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(v.name_token->file).string())) {
                continue;
            }

            uint32_t nameLength = v.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(v.getName().c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = v.getType().length();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(v.getType().c_str(), sizeof(char), typeLength, f);
        }
        for (Interface* i : result.interfaces) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(i->name_token->file).string())) {
                continue;
            }

            uint32_t nameLength = i->getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(i->getName().c_str(), sizeof(char), nameLength, f);

            uint32_t numMethods = i->getImplements().size();
            fwrite(&numMethods, sizeof(uint32_t), 1, f);

            for (Function* m : i->getImplements()) {
                uint32_t methodNameLength = m->getName().length();
                fwrite(&methodNameLength, sizeof(uint32_t), 1, f);
                fwrite(m->getName().c_str(), sizeof(char), methodNameLength, f);

                uint32_t memberTypeLength = m->getMemberType().length();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->getMemberType().c_str(), sizeof(char), memberTypeLength, f);

                uint32_t returnTypeLength = m->getReturnType().length();
                fwrite(&returnTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->getReturnType().c_str(), sizeof(char), returnTypeLength, f);

                auto mods = m->getModifiers();
                uint32_t numMods = mods.size();
                fwrite(&numMods, sizeof(uint32_t), 1, f);
                for (std::string mod : mods) {
                    uint32_t modLength = mod.length();
                    fwrite(&modLength, sizeof(uint32_t), 1, f);
                    fwrite(mod.c_str(), sizeof(char), modLength, f);
                }

                uint32_t numArgs = m->getArgs().size();
                fwrite(&numArgs, sizeof(uint32_t), 1, f);

                for (Variable v : m->getArgs()) {
                    uint32_t argTypeLength = v.getType().length();
                    fwrite(&argTypeLength, sizeof(uint32_t), 1, f);
                    fwrite(v.getType().c_str(), sizeof(char), argTypeLength, f);
                }
            }
        }
        for (Layout l : result.layouts) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(l.getNameToken().file).string())) {
                continue;
            }

            uint32_t nameLength = l.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(l.getName().c_str(), sizeof(char), nameLength, f);

            uint32_t numFields = l.getMembers().size();
            fwrite(&numFields, sizeof(uint32_t), 1, f);

            for (Variable v : l.getMembers()) {
                uint32_t fieldNameLength = v.getName().length();
                fwrite(&fieldNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.getName().c_str(), sizeof(char), fieldNameLength, f);

                uint32_t fieldTypeLength = v.getType().length();
                fwrite(&fieldTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.getType().c_str(), sizeof(char), fieldTypeLength, f);
            }
        }
        for (auto ta : result.typealiases) {
            uint32_t nameLength = ta.first.length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(ta.first.c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = ta.second.length();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(ta.second.c_str(), sizeof(char), typeLength, f);
        }

        fclose(f);
    }

    void InfoDumper::dump(TPResult result) {
        if (Main.options.binaryHeader) {
            binaryHeader(result);
            return;
        }

        remove(Main.options.outfile.c_str());
        FILE* f = fopen(Main.options.outfile.c_str(), "ab");
        if (!f) {
            std::cerr << "Failed to open file for writing: " << Main.options.outfile << std::endl;
            return;
        }

        char magic[] = {'\0', 'S', 'C', 'L'};
        fwrite(magic, sizeof(char), 4, f);

        uint32_t version = Main.version->major << 16 | Main.version->minor << 8 | Main.version->patch;
        fwrite(&version, sizeof(uint32_t), 1, f);

        uint32_t numContainers = 0;
        for (Container c : result.containers) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(c.name_token->file).string())) {
                numContainers++;
            }
        }
        fwrite(&numContainers, sizeof(uint32_t), 1, f);

        uint32_t numStructs = 0;
        for (Struct s : result.structs) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(s.name_token.file).string())) {
                numStructs++;
            }
        }
        fwrite(&numStructs, sizeof(uint32_t), 1, f);

        uint32_t numEnums = 0;
        for (Enum e : result.enums) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(e.name_token->file).string())) {
                numEnums++;
            }
        }
        fwrite(&numEnums, sizeof(uint32_t), 1, f);

        uint32_t numFunctions = 0;
        for (Function* f : joinVecs(result.functions, result.extern_functions)) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(f->nameToken.file).string())) {
                numFunctions++;
            }
        }
        fwrite(&numFunctions, sizeof(uint32_t), 1, f);

        uint32_t numGlobals = 0;
        for (Variable v : result.globals) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(v.name_token->file).string())) {
                numGlobals++;
            }
        }
        fwrite(&numGlobals, sizeof(uint32_t), 1, f);

        uint32_t numExternGlobals = 0;
        for (Variable v : result.extern_globals) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(v.name_token->file).string())) {
                numExternGlobals++;
            }
        }
        fwrite(&numExternGlobals, sizeof(uint32_t), 1, f);

        uint32_t numInterfaces = 0;
        for (Interface* i : result.interfaces) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(i->name_token->file).string())) {
                numInterfaces++;
            }
        }
        fwrite(&numInterfaces, sizeof(uint32_t), 1, f);

        uint32_t numLayouts = 0;
        for (Layout l : result.layouts) {
            if (contains(Main.options.filesFromCommandLine, std::filesystem::absolute(l.getNameToken().file).string())) {
                numLayouts++;
            }
        }
        fwrite(&numLayouts, sizeof(uint32_t), 1, f);

        uint32_t numTypealiases = result.typealiases.size();
        fwrite(&numTypealiases, sizeof(uint32_t), 1, f);

        for (Container c : result.containers) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(c.name_token->file).string())) {
                continue;
            }

            uint32_t nameLength = c.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(c.getName().c_str(), sizeof(char), nameLength, f);
            uint32_t numMembers = c.getMembers().size();
            fwrite(&numMembers, sizeof(uint32_t), 1, f);
            for (Variable v : c.getMembers()) {
                uint32_t memberNameLength = v.getName().length();
                fwrite(&memberNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.getName().c_str(), sizeof(char), memberNameLength, f);

                uint32_t memberTypeLength = v.getType().length();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.getType().c_str(), sizeof(char), memberTypeLength, f);
            }
        }
        for (Struct s : result.structs) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(s.name_token.file).string())) {
                continue;
            }

            uint32_t nameLength = s.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(s.getName().c_str(), sizeof(char), nameLength, f);

            fwrite(&(s.flags), sizeof(int), 1, f);

            uint32_t numMembers = s.getMembers().size();
            fwrite(&numMembers, sizeof(uint32_t), 1, f);

            for (Variable v : s.getMembers()) {
                uint32_t memberNameLength = v.getName().length();
                fwrite(&memberNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.getName().c_str(), sizeof(char), memberNameLength, f);

                uint32_t memberTypeLength = v.getType().length();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.getType().c_str(), sizeof(char), memberTypeLength, f);
            }
        }
        for (Enum e : result.enums) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(e.name_token->file).string())) {
                continue;
            }

            uint32_t nameLength = e.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(e.getName().c_str(), sizeof(char), nameLength, f);

            uint32_t numMembers = e.getMembers().size();
            fwrite(&numMembers, sizeof(uint32_t), 1, f);
            for (uint32_t i = 0; i < numMembers; i++) {
                uint32_t memberNameLength = e.getMembers()[i].length();
                fwrite(&memberNameLength, sizeof(uint32_t), 1, f);
                fwrite(e.getMembers()[i].c_str(), sizeof(char), memberNameLength, f);
            }
        }
        for (Function* func : joinVecs(result.functions, result.extern_functions)) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(func->nameToken.file).string())) {
                continue;
            }

            if (contains<std::string>(func->getModifiers(), "intrinsic") && func->getBody().size() == 0) {
                continue;
            }

            uint8_t isMethod = func->isMethod;
            fwrite(&isMethod, sizeof(uint8_t), 1, f);
            
            uint32_t nameLength = func->getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(func->getName().c_str(), sizeof(char), nameLength, f);

            if (isMethod) {
                Method* m = static_cast<Method*>(func);
                uint32_t memberTypeLength = m->getMemberType().length();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->getMemberType().c_str(), sizeof(char), memberTypeLength, f);
            }

            uint32_t returnTypeLength = func->getReturnType().length();
            fwrite(&returnTypeLength, sizeof(uint32_t), 1, f);
            fwrite(func->getReturnType().c_str(), sizeof(char), returnTypeLength, f);

            auto mods = func->getModifiers();
            uint32_t numMods = mods.size();

            if (isMethod && ((Method*) func)->addAnyway()) {
                numMods++;
            }

            fwrite(&numMods, sizeof(uint32_t), 1, f);

            if (isMethod && ((Method*) func)->addAnyway()) {
                uint32_t modLength = 9;
                fwrite(&modLength, sizeof(uint32_t), 1, f);
                fwrite("<virtual>", sizeof(char), modLength, f);
            }

            for (std::string mod : mods) {
                uint32_t modLength = mod.length();
                fwrite(&modLength, sizeof(uint32_t), 1, f);
                fwrite(mod.c_str(), sizeof(char), modLength, f);
            }

            uint32_t numArgs = func->getArgs().size();
            fwrite(&numArgs, sizeof(uint32_t), 1, f);

            for (Variable v : func->getArgs()) {
                uint32_t argNameLength = v.getName().length();
                fwrite(&argNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.getName().c_str(), sizeof(char), argNameLength, f);

                uint32_t argTypeLength = v.getType().length();
                fwrite(&argTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.getType().c_str(), sizeof(char), argTypeLength, f);
            }

            uint32_t bodySize = func->getBody().size();
            fwrite(&bodySize, sizeof(uint32_t), 1, f);
            for (uint32_t i = 0; i < bodySize; i++) {
                auto tok = func->getBody()[i];
                fwrite(&(tok.type), sizeof(TokenType), 1, f);
                uint32_t strLength = tok.value.length();
                fwrite(&strLength, sizeof(uint32_t), 1, f);
                fwrite(tok.value.c_str(), sizeof(char), strLength, f);
            }
        }
        for (Variable v : result.globals) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(v.name_token->file).string())) {
                continue;
            }

            uint32_t nameLength = v.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(v.getName().c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = v.getType().length();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(v.getType().c_str(), sizeof(char), typeLength, f);
        }
        for (Variable v : result.extern_globals) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(v.name_token->file).string())) {
                continue;
            }

            uint32_t nameLength = v.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(v.getName().c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = v.getType().length();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(v.getType().c_str(), sizeof(char), typeLength, f);
        }
        for (Interface* i : result.interfaces) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(i->name_token->file).string())) {
                continue;
            }

            uint32_t nameLength = i->getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(i->getName().c_str(), sizeof(char), nameLength, f);

            uint32_t numMethods = i->getImplements().size();
            fwrite(&numMethods, sizeof(uint32_t), 1, f);

            for (Function* m : i->getImplements()) {
                uint32_t methodNameLength = m->getName().length();
                fwrite(&methodNameLength, sizeof(uint32_t), 1, f);
                fwrite(m->getName().c_str(), sizeof(char), methodNameLength, f);

                uint32_t memberTypeLength = m->getMemberType().length();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->getMemberType().c_str(), sizeof(char), memberTypeLength, f);

                uint32_t returnTypeLength = m->getReturnType().length();
                fwrite(&returnTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->getReturnType().c_str(), sizeof(char), returnTypeLength, f);

                auto mods = m->getModifiers();
                uint32_t numMods = mods.size();
                fwrite(&numMods, sizeof(uint32_t), 1, f);
                for (std::string mod : mods) {
                    uint32_t modLength = mod.length();
                    fwrite(&modLength, sizeof(uint32_t), 1, f);
                    fwrite(mod.c_str(), sizeof(char), modLength, f);
                }

                uint32_t numArgs = m->getArgs().size();
                fwrite(&numArgs, sizeof(uint32_t), 1, f);

                for (Variable v : m->getArgs()) {
                    uint32_t argNameLength = v.getName().length();
                    fwrite(&argNameLength, sizeof(uint32_t), 1, f);
                    fwrite(v.getName().c_str(), sizeof(char), argNameLength, f);

                    uint32_t argTypeLength = v.getType().length();
                    fwrite(&argTypeLength, sizeof(uint32_t), 1, f);
                    fwrite(v.getType().c_str(), sizeof(char), argTypeLength, f);
                }
            }
        }
        for (Layout l : result.layouts) {
            if (!contains(Main.options.filesFromCommandLine, std::filesystem::absolute(l.getNameToken().file).string())) {
                continue;
            }

            uint32_t nameLength = l.getName().length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(l.getName().c_str(), sizeof(char), nameLength, f);

            uint32_t numFields = l.getMembers().size();
            fwrite(&numFields, sizeof(uint32_t), 1, f);

            for (Variable v : l.getMembers()) {
                uint32_t fieldNameLength = v.getName().length();
                fwrite(&fieldNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.getName().c_str(), sizeof(char), fieldNameLength, f);

                uint32_t fieldTypeLength = v.getType().length();
                fwrite(&fieldTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.getType().c_str(), sizeof(char), fieldTypeLength, f);
            }
        }
        for (auto ta : result.typealiases) {
            uint32_t nameLength = ta.first.length();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(ta.first.c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = ta.second.length();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(ta.second.c_str(), sizeof(char), typeLength, f);
        }

        fclose(f);
    }
} // namespace sclc
