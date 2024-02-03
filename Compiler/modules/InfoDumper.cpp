#include <filesystem>

#include "../headers/Common.hpp"

namespace sclc {

    #define print(s) std::cout << s << std::endl

    void binaryHeader(TPResult result) {
        remove(Main::options::outfile.c_str());
        FILE* f = fopen(Main::options::outfile.c_str(), "ab");
        if (!f) {
            std::cerr << "Failed to open file for writing: '" << Main::options::outfile << "'" << std::endl;
            return;
        }

        char magic[] = {1, 'S', 'C', 'L'};
        fwrite(magic, sizeof(char), 4, f);

        uint32_t version = Main::version->major << 16 | Main::version->minor << 8 | Main::version->patch;
        fwrite(&version, sizeof(uint32_t), 1, f);

        uint32_t numStructs = 0;
        for (Struct& s : result.structs) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(s.name_token.location.file).string())) {
                numStructs++;
            }
        }
        fwrite(&numStructs, sizeof(uint32_t), 1, f);

        uint32_t numEnums = 0;
        for (Enum e : result.enums) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(e.name_token->location.file).string())) {
                numEnums++;
            }
        }
        fwrite(&numEnums, sizeof(uint32_t), 1, f);

        uint32_t numFunctions = 0;
        for (Function* f : result.functions) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(f->name_token.location.file).string())) {
                numFunctions++;
            }
        }
        fwrite(&numFunctions, sizeof(uint32_t), 1, f);

        uint32_t numGlobals = 0;
        for (Variable& v : result.globals) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(v.name_token->location.file).string())) {
                numGlobals++;
            }
        }
        fwrite(&numGlobals, sizeof(uint32_t), 1, f);

        uint32_t numInterfaces = 0;
        for (Interface* i : result.interfaces) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(i->name_token->location.file).string())) {
                numInterfaces++;
            }
        }
        fwrite(&numInterfaces, sizeof(uint32_t), 1, f);

        uint32_t numLayouts = 0;
        for (Layout& l : result.layouts) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(l.name_token.location.file).string())) {
                numLayouts++;
            }
        }
        fwrite(&numLayouts, sizeof(uint32_t), 1, f);

        uint32_t numTypealiases = result.typealiases.size();
        fwrite(&numTypealiases, sizeof(uint32_t), 1, f);

        for (Struct& s : result.structs) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(s.name_token.location.file).string())) {
                continue;
            }

            uint32_t nameLength = s.name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(s.name.c_str(), sizeof(char), nameLength, f);

            fwrite(&(s.flags), sizeof(int), 1, f);

            uint32_t numMembers = s.members.size();
            fwrite(&numMembers, sizeof(uint32_t), 1, f);

            for (Variable& v : s.members) {
                uint32_t memberNameLength = v.name.size();
                fwrite(&memberNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.name.c_str(), sizeof(char), memberNameLength, f);

                uint32_t memberTypeLength = v.type.size();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.type.c_str(), sizeof(char), memberTypeLength, f);
            }
        }
        for (Enum e : result.enums) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(e.name_token->location.file).string())) {
                continue;
            }

            uint32_t nameLength = e.name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(e.name.c_str(), sizeof(char), nameLength, f);

            uint32_t numMembers = e.members.size();
            fwrite(&numMembers, sizeof(uint32_t), 1, f);
            for (auto&& member : e.members) {
                uint32_t memberNameLength = member.first.size();
                fwrite(&memberNameLength, sizeof(uint32_t), 1, f);
                fwrite(member.first.c_str(), sizeof(char), memberNameLength, f);

                fwrite(&(member.second), sizeof(long), 1, f);
            }
        }
        for (Function* func : result.functions) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(func->name_token.location.file).string())) {
                continue;
            }

            uint8_t isMethod = func->isMethod;
            fwrite(&isMethod, sizeof(uint8_t), 1, f);
            
            uint32_t nameLength = func->name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(func->name.c_str(), sizeof(char), nameLength, f);

            if (isMethod) {
                Method* m = static_cast<Method*>(func);
                uint32_t memberTypeLength = m->member_type.size();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->member_type.c_str(), sizeof(char), memberTypeLength, f);
            }

            uint32_t returnTypeLength = func->return_type.size();
            fwrite(&returnTypeLength, sizeof(uint32_t), 1, f);
            fwrite(func->return_type.c_str(), sizeof(char), returnTypeLength, f);

            auto mods = func->modifiers;
            uint32_t numMods = mods.size();

            if (isMethod && ((Method*) func)->force_add) {
                numMods++;
            }

            fwrite(&numMods, sizeof(uint32_t), 1, f);

            if (isMethod && ((Method*) func)->force_add) {
                uint32_t modLength = 9;
                fwrite(&modLength, sizeof(uint32_t), 1, f);
                fwrite("<virtual>", sizeof(char), modLength, f);
            }

            for (std::string& mod : mods) {
                uint32_t modLength = mod.size();
                fwrite(&modLength, sizeof(uint32_t), 1, f);
                fwrite(mod.c_str(), sizeof(char), modLength, f);
            }

            uint32_t numArgs = func->args.size();
            fwrite(&numArgs, sizeof(uint32_t), 1, f);

            for (Variable& v : func->args) {
                uint32_t argTypeLength = v.type.size();
                fwrite(&argTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.type.c_str(), sizeof(char), argTypeLength, f);
            }
        }
        for (Variable& v : result.globals) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(v.name_token->location.file).string()) || v.isExtern) {
                continue;
            }

            uint32_t nameLength = v.name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(v.name.c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = v.type.size();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(v.type.c_str(), sizeof(char), typeLength, f);
        }
        for (Variable& v : result.globals) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(v.name_token->location.file).string()) || !v.isExtern) {
                continue;
            }

            uint32_t nameLength = v.name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(v.name.c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = v.type.size();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(v.type.c_str(), sizeof(char), typeLength, f);
        }
        for (Interface* i : result.interfaces) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(i->name_token->location.file).string())) {
                continue;
            }

            uint32_t nameLength = i->name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(i->name.c_str(), sizeof(char), nameLength, f);

            uint32_t numMethods = i->toImplement.size();
            fwrite(&numMethods, sizeof(uint32_t), 1, f);

            for (Function* m : i->toImplement) {
                uint32_t methodNameLength = m->name.size();
                fwrite(&methodNameLength, sizeof(uint32_t), 1, f);
                fwrite(m->name.c_str(), sizeof(char), methodNameLength, f);

                uint32_t memberTypeLength = m->member_type.size();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->member_type.c_str(), sizeof(char), memberTypeLength, f);

                uint32_t returnTypeLength = m->return_type.size();
                fwrite(&returnTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->return_type.c_str(), sizeof(char), returnTypeLength, f);

                auto mods = m->modifiers;
                uint32_t numMods = mods.size();
                fwrite(&numMods, sizeof(uint32_t), 1, f);
                for (std::string& mod : mods) {
                    uint32_t modLength = mod.size();
                    fwrite(&modLength, sizeof(uint32_t), 1, f);
                    fwrite(mod.c_str(), sizeof(char), modLength, f);
                }

                uint32_t numArgs = m->args.size();
                fwrite(&numArgs, sizeof(uint32_t), 1, f);

                for (Variable& v : m->args) {
                    uint32_t argTypeLength = v.type.size();
                    fwrite(&argTypeLength, sizeof(uint32_t), 1, f);
                    fwrite(v.type.c_str(), sizeof(char), argTypeLength, f);
                }
            }
        }
        for (Layout& l : result.layouts) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(l.name_token.location.file).string())) {
                continue;
            }

            uint32_t nameLength = l.name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(l.name.c_str(), sizeof(char), nameLength, f);

            uint32_t numFields = l.members.size();
            fwrite(&numFields, sizeof(uint32_t), 1, f);

            for (Variable& v : l.members) {
                uint32_t fieldNameLength = v.name.size();
                fwrite(&fieldNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.name.c_str(), sizeof(char), fieldNameLength, f);

                uint32_t fieldTypeLength = v.type.size();
                fwrite(&fieldTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.type.c_str(), sizeof(char), fieldTypeLength, f);
            }
        }
        for (auto ta : result.typealiases) {
            uint32_t nameLength = ta.first.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(ta.first.c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = ta.second.size();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(ta.second.c_str(), sizeof(char), typeLength, f);
        }

        fclose(f);
    }

    void InfoDumper::dump(TPResult& result) {
        if (Main::options::binaryHeader) {
            binaryHeader(result);
            return;
        }

        remove(Main::options::outfile.c_str());
        FILE* f = fopen(Main::options::outfile.c_str(), "ab");
        if (!f) {
            std::cerr << "Failed to open file for writing: " << Main::options::outfile << std::endl;
            return;
        }

        char magic[] = {'\0', 'S', 'C', 'L'};
        fwrite(magic, sizeof(char), 4, f);

        uint32_t version = Main::version->major << 16 | Main::version->minor << 8 | Main::version->patch;
        fwrite(&version, sizeof(uint32_t), 1, f);

        uint32_t numStructs = 0;
        for (Struct& s : result.structs) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(s.name_token.location.file).string())) {
                numStructs++;
            }
        }
        fwrite(&numStructs, sizeof(uint32_t), 1, f);

        uint32_t numEnums = 0;
        for (Enum e : result.enums) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(e.name_token->location.file).string())) {
                numEnums++;
            }
        }
        fwrite(&numEnums, sizeof(uint32_t), 1, f);

        uint32_t numFunctions = 0;
        for (Function* f : result.functions) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(f->name_token.location.file).string())) {
                numFunctions++;
            }
        }
        fwrite(&numFunctions, sizeof(uint32_t), 1, f);

        uint32_t numGlobals = 0;
        for (Variable& v : result.globals) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(v.name_token->location.file).string())) {
                numGlobals++;
            }
        }
        fwrite(&numGlobals, sizeof(uint32_t), 1, f);

        uint32_t numInterfaces = 0;
        for (Interface* i : result.interfaces) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(i->name_token->location.file).string())) {
                numInterfaces++;
            }
        }
        fwrite(&numInterfaces, sizeof(uint32_t), 1, f);

        uint32_t numLayouts = 0;
        for (Layout& l : result.layouts) {
            if (contains(Main::options::filesFromCommandLine, std::filesystem::absolute(l.name_token.location.file).string())) {
                numLayouts++;
            }
        }
        fwrite(&numLayouts, sizeof(uint32_t), 1, f);

        uint32_t numTypealiases = result.typealiases.size();
        fwrite(&numTypealiases, sizeof(uint32_t), 1, f);

        for (Struct& s : result.structs) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(s.name_token.location.file).string())) {
                continue;
            }

            uint32_t nameLength = s.name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(s.name.c_str(), sizeof(char), nameLength, f);

            fwrite(&(s.flags), sizeof(int), 1, f);

            uint32_t numMembers = s.members.size();
            fwrite(&numMembers, sizeof(uint32_t), 1, f);

            for (Variable& v : s.members) {
                uint32_t memberNameLength = v.name.size();
                fwrite(&memberNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.name.c_str(), sizeof(char), memberNameLength, f);

                uint32_t memberTypeLength = v.type.size();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.type.c_str(), sizeof(char), memberTypeLength, f);
            }
        }
        for (Enum e : result.enums) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(e.name_token->location.file).string())) {
                continue;
            }

            uint32_t nameLength = e.name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(e.name.c_str(), sizeof(char), nameLength, f);

            uint32_t numMembers = e.members.size();
            fwrite(&numMembers, sizeof(uint32_t), 1, f);
            for (auto&& member : e.members) {
                uint32_t memberNameLength = member.first.size();
                fwrite(&memberNameLength, sizeof(uint32_t), 1, f);
                fwrite(member.first.c_str(), sizeof(char), memberNameLength, f);

                fwrite(&(member.second), sizeof(long), 1, f);
            }
        }
        for (Function* func : result.functions) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(func->name_token.location.file).string())) {
                continue;
            }

            if (func->has_intrinsic && func->getBody().empty()) {
                continue;
            }

            uint8_t isMethod = func->isMethod;
            fwrite(&isMethod, sizeof(uint8_t), 1, f);
            
            uint32_t nameLength = func->name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(func->name.c_str(), sizeof(char), nameLength, f);

            if (isMethod) {
                Method* m = static_cast<Method*>(func);
                uint32_t memberTypeLength = m->member_type.size();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->member_type.c_str(), sizeof(char), memberTypeLength, f);
            }

            uint32_t returnTypeLength = func->return_type.size();
            fwrite(&returnTypeLength, sizeof(uint32_t), 1, f);
            fwrite(func->return_type.c_str(), sizeof(char), returnTypeLength, f);

            auto mods = func->modifiers;
            uint32_t numMods = mods.size();

            if (isMethod && ((Method*) func)->force_add) {
                numMods++;
            }

            fwrite(&numMods, sizeof(uint32_t), 1, f);

            if (isMethod && ((Method*) func)->force_add) {
                uint32_t modLength = 9;
                fwrite(&modLength, sizeof(uint32_t), 1, f);
                fwrite("<virtual>", sizeof(char), modLength, f);
            }

            for (std::string& mod : mods) {
                uint32_t modLength = mod.size();
                fwrite(&modLength, sizeof(uint32_t), 1, f);
                fwrite(mod.c_str(), sizeof(char), modLength, f);
            }

            uint32_t numArgs = func->args.size();
            fwrite(&numArgs, sizeof(uint32_t), 1, f);

            for (Variable& v : func->args) {
                uint32_t argNameLength = v.name.size();
                fwrite(&argNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.name.c_str(), sizeof(char), argNameLength, f);

                uint32_t argTypeLength = v.type.size();
                fwrite(&argTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.type.c_str(), sizeof(char), argTypeLength, f);
            }

            uint32_t bodySize = func->getBody().size();
            fwrite(&bodySize, sizeof(uint32_t), 1, f);
            for (uint32_t i = 0; i < bodySize; i++) {
                auto tok = func->getBody()[i];
                fwrite(&(tok.type), sizeof(TokenType), 1, f);
                uint32_t strLength = tok.value.size();
                fwrite(&strLength, sizeof(uint32_t), 1, f);
                fwrite(tok.value.c_str(), sizeof(char), strLength, f);
            }
        }
        for (Variable& v : result.globals) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(v.name_token->location.file).string()) || v.isExtern) {
                continue;
            }

            uint32_t nameLength = v.name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(v.name.c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = v.type.size();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(v.type.c_str(), sizeof(char), typeLength, f);
        }
        for (Variable& v : result.globals) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(v.name_token->location.file).string()) || !v.isExtern) {
                continue;
            }

            uint32_t nameLength = v.name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(v.name.c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = v.type.size();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(v.type.c_str(), sizeof(char), typeLength, f);
        }
        for (Interface* i : result.interfaces) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(i->name_token->location.file).string())) {
                continue;
            }

            uint32_t nameLength = i->name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(i->name.c_str(), sizeof(char), nameLength, f);

            uint32_t numMethods = i->toImplement.size();
            fwrite(&numMethods, sizeof(uint32_t), 1, f);

            for (Function* m : i->toImplement) {
                uint32_t methodNameLength = m->name.size();
                fwrite(&methodNameLength, sizeof(uint32_t), 1, f);
                fwrite(m->name.c_str(), sizeof(char), methodNameLength, f);

                uint32_t memberTypeLength = m->member_type.size();
                fwrite(&memberTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->member_type.c_str(), sizeof(char), memberTypeLength, f);

                uint32_t returnTypeLength = m->return_type.size();
                fwrite(&returnTypeLength, sizeof(uint32_t), 1, f);
                fwrite(m->return_type.c_str(), sizeof(char), returnTypeLength, f);

                auto mods = m->modifiers;
                uint32_t numMods = mods.size();
                fwrite(&numMods, sizeof(uint32_t), 1, f);
                for (std::string& mod : mods) {
                    uint32_t modLength = mod.size();
                    fwrite(&modLength, sizeof(uint32_t), 1, f);
                    fwrite(mod.c_str(), sizeof(char), modLength, f);
                }

                uint32_t numArgs = m->args.size();
                fwrite(&numArgs, sizeof(uint32_t), 1, f);

                for (Variable& v : m->args) {
                    uint32_t argNameLength = v.name.size();
                    fwrite(&argNameLength, sizeof(uint32_t), 1, f);
                    fwrite(v.name.c_str(), sizeof(char), argNameLength, f);

                    uint32_t argTypeLength = v.type.size();
                    fwrite(&argTypeLength, sizeof(uint32_t), 1, f);
                    fwrite(v.type.c_str(), sizeof(char), argTypeLength, f);
                }
            }
        }
        for (Layout& l : result.layouts) {
            if (!contains(Main::options::filesFromCommandLine, std::filesystem::absolute(l.name_token.location.file).string())) {
                continue;
            }

            uint32_t nameLength = l.name.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(l.name.c_str(), sizeof(char), nameLength, f);

            uint32_t numFields = l.members.size();
            fwrite(&numFields, sizeof(uint32_t), 1, f);

            for (Variable& v : l.members) {
                uint32_t fieldNameLength = v.name.size();
                fwrite(&fieldNameLength, sizeof(uint32_t), 1, f);
                fwrite(v.name.c_str(), sizeof(char), fieldNameLength, f);

                uint32_t fieldTypeLength = v.type.size();
                fwrite(&fieldTypeLength, sizeof(uint32_t), 1, f);
                fwrite(v.type.c_str(), sizeof(char), fieldTypeLength, f);
            }
        }
        for (auto ta : result.typealiases) {
            uint32_t nameLength = ta.first.size();
            fwrite(&nameLength, sizeof(uint32_t), 1, f);
            fwrite(ta.first.c_str(), sizeof(char), nameLength, f);

            uint32_t typeLength = ta.second.size();
            fwrite(&typeLength, sizeof(uint32_t), 1, f);
            fwrite(ta.second.c_str(), sizeof(char), typeLength, f);
        }

        fclose(f);
    }
} // namespace sclc
