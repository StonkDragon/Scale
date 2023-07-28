#include <iostream>
#include <string>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <filesystem>
#include <map>
#include <fstream>
#include <optional>

#include <signal.h>
#include <errno.h>

#ifndef _WIN32
#include <unistd.h>
#ifndef LINK_MATH
#define LINK_MATH
#endif
#else
#include <process.h>
#define execv _execv
#endif

#include "headers/Common.hpp"
#include "headers/DragonConfig.hpp"

#ifndef VERSION
#define VERSION "unknown. Did you forget to build with -DVERSION=<version>?"
#endif

#ifndef COMPILER
#define COMPILER "clang"
#endif

#ifndef C_VERSION
#define C_VERSION "gnu2x"
#endif

#ifndef SCALE_INSTALL_DIR
#define SCALE_INSTALL_DIR "Scale"
#endif

#ifndef DEFAULT_OUTFILE
#define DEFAULT_OUTFILE "out.scl"
#endif

#ifndef FRAMEWORK_VERSION_REQ
#define FRAMEWORK_VERSION_REQ "23.8"
#endif

#ifndef SCL_ROOT_DIR
#ifdef _WIN32
#define SCL_ROOT_DIR getenv("USERPROFILE")
#else
#define SCL_ROOT_DIR getenv("HOME")
#endif
#endif

#if defined(__APPLE__)
#define LIB_SCALE_FILENAME "libScaleRuntime.dylib"
#define LIB_SCALE_FILENAME_NO_EXT "libScaleRuntime"
#define LIB_SCALE_EXT ".dylib"
#elif defined(__linux__)
#define LIB_SCALE_FILENAME "libScaleRuntime.so"
#define LIB_SCALE_FILENAME_NO_EXT "libScaleRuntime"
#define LIB_SCALE_EXT ".so"
#elif defined(_WIN32)
// defined, but not used
#define LIB_SCALE_FILENAME "ScaleRuntime.dll"
#define LIB_SCALE_FILENAME_NO_EXT "ScaleRuntime"
#define LIB_SCALE_EXT ".dll"
#endif

#define TO_STRING2(x) #x
#define TO_STRING(x) TO_STRING2(x)

namespace sclc
{
    std::string scaleFolder;
    std::string scaleLatestFolder;
    std::vector<std::string> nonScaleFiles;
    std::vector<std::string> cflags;

    void usage(std::string programName) {
        std::cout << "Usage: " << programName << " <filename> [args]" << std::endl;
        std::cout << std::endl;
        std::cout << "Accepted Flags:" << std::endl;
        std::cout << "  -t, --transpile      Transpile only" << std::endl;
        std::cout << "  -h, --help           Show this help" << std::endl;
        std::cout << "  -v, --version        Show version information" << std::endl;
        std::cout << "  -o <filename>        Specify Output file" << std::endl;
        std::cout << "  -f <framework>       Use Scale Framework" << std::endl;
        std::cout << "  -no-main             Do not check for main Function" << std::endl;
        std::cout << "  -compiler <comp>     Use comp as the compiler instead of " << std::string(COMPILER) << std::endl;
        std::cout << "  -feat <feature>      Enables the specified language feature" << std::endl;
        std::cout << "  -run                 Run the compiled program" << std::endl;
        std::cout << "  -makelib             Compile as a library (implies -no-main)" << std::endl;
        std::cout << "  -create-module       Create a Scale module" << std::endl;
        std::cout << "  -cflags              Print c compiler flags and exit" << std::endl;
        std::cout << "  -debug               Run in debug mode" << std::endl;
        std::cout << "  -no-error-location   Do not print an overview of the file on error" << std::endl;
        std::cout << "  -no-minify           Add extra stack trace information" << std::endl;
        std::cout << "  -doc                 Print documentation" << std::endl;
        std::cout << "  -doc-for <framework> Print documentation for Framework" << std::endl;
        std::cout << "  -stack-size <sz>     Sets the starting stack size. Must be a multiple of 2" << std::endl;
        std::cout << std::endl;
        std::cout << "  Any other options are passed directly to " << std::string(COMPILER) << " (or compiler specified by -compiler)" << std::endl;
    }

    bool contains(std::vector<std::string>& vec, std::string& item) {
        for (auto& i : vec) {
            if (i == item) {
                return true;
            }
        }
        return false;
    }

    std::string gen_random() {
        char* s = (char*) malloc(256);
        ID_t h = id((char*) std::string(VERSION).c_str());
        snprintf(s, 256, "%x%x%x%x", h, h, h, h);
        return std::string(s);
    }

    FPResult findFileInIncludePath(std::string file);

    std::vector<std::string> split(const std::string& str, const std::string& delimiter);

    struct DocumentationEntry {
        std::string name;
        std::string description;
        std::string module;
        std::string file;
    };

    using DocumentationEntries = std::vector<DocumentationEntry>;

    struct Documentation {
        std::map<std::string, DocumentationEntries> entries;

        auto begin() {
            return entries.begin();
        }
        auto end() {
            return entries.end();
        }
    };

    std::vector<Token> parseString(std::string s) {
        Tokenizer tk;

        tk.source = strdup(s.c_str());
        tk.current = 0;

        Token token = tk.nextToken();
        while (token.getType() != tok_eof) {
            tk.tokens.push_back(token);
            if (tk.additional) {
                tk.tokens.push_back(tk.additionalToken);
                tk.additional = false;
            }
            token = tk.nextToken();
        }

        return tk.tokens;
    }

    bool checkFramework(std::string& framework, std::vector<std::string>& tmpFlags, std::vector<std::string>& frameworks, bool& hasCppFiles, Version& FrameworkMinimumVersion) {
        for (auto path : Main.options.includePaths) {
            if (fileExists(path + "/" + framework + ".framework/index.drg")) {
                DragonConfig::ConfigParser parser;
                DragonConfig::CompoundEntry* root = parser.parse(path + "/" + framework + ".framework/index.drg");
                if (root == nullptr) {
                    std::cerr << Color::RED << "Failed to parse index.drg of Framework " << framework << std::endl;
                    return false;
                }
                root = root->getCompound("framework");
                Main.options.mapFrameworkConfigs[framework] = root;
                DragonConfig::ListEntry* implementers = root->getList("implementers");
                DragonConfig::ListEntry* implHeaders = root->getList("implHeaders");
                DragonConfig::ListEntry* depends = root->getList("depends");
                DragonConfig::ListEntry* compilerFlags = root->getList("compilerFlags");

                DragonConfig::StringEntry* versionTag = root->getString("version");
                std::string version = versionTag->getValue();
                if (versionTag == nullptr) {
                    std::cerr << "Framework " << framework << " does not specify a version! Skipping." << std::endl;
                    return false;
                }
                
                DragonConfig::StringEntry* headerDirTag = root->getString("headerDir");
                std::string headerDir = headerDirTag == nullptr ? "" : headerDirTag->getValue();
                Main.options.mapFrameworkIncludeFolders[framework] = headerDir;
                
                DragonConfig::StringEntry* implDirTag = root->getString("implDir");
                std::string implDir = implDirTag == nullptr ? "" : implDirTag->getValue();
                
                DragonConfig::StringEntry* implHeaderDirTag = root->getString("implHeaderDir");
                std::string implHeaderDir = implHeaderDirTag == nullptr ? "" : implHeaderDirTag->getValue();

                DragonConfig::StringEntry* docfileTag = root->getString("docfile");
                Main.options.mapFrameworkDocfiles[framework] = docfileTag == nullptr ? "" : path + "/" + framework + ".framework/" + docfileTag->getValue();

                Version ver = Version(version);
                Version compilerVersion = Version(VERSION);
                if (ver > compilerVersion) {
                    std::cerr << "Error: Framework '" << framework << "' requires Scale v" << version << " but you are using " << VERSION << std::endl;
                    return false;
                }
                if (ver < FrameworkMinimumVersion) {
                    fprintf(stderr, "Error: Framework '%s' is too outdated (%s). Please update it to at least version %s\n", framework.c_str(), ver.asString().c_str(), FrameworkMinimumVersion.asString().c_str());
                    return false;
                }

                for (size_t i = 0; depends != nullptr && i < depends->size(); i++) {
                    std::string depend = depends->getString(i)->getValue();
                    if (!contains(frameworks, depend)) {
                        std::cerr << "Error: Framework '" << framework << "' depends on '" << depend << "' but it is not included" << std::endl;
                        return false;
                    }
                }

                for (size_t i = 0; compilerFlags != nullptr && i < compilerFlags->size(); i++) {
                    std::string flag = compilerFlags->getString(i)->getValue();
                    if (!hasCppFiles && (strends(flag, ".cpp") || strends(flag, ".c++"))) {
                        hasCppFiles = true;
                    }
                    tmpFlags.push_back(flag);
                }

                Main.frameworks.push_back(framework);

                if (headerDir.size() > 0) {
                    Main.options.includePaths.push_back(path + "/" + framework + ".framework/" + headerDir);
                    Main.options.mapIncludePathsToFrameworks[framework] = path + "/" + framework + ".framework/" + headerDir;
                }
                unsigned long implementersSize = implementers == nullptr ? 0 : implementers->size();
                for (unsigned long i = 0; i < implementersSize; i++) {
                    std::string implementer = implementers->getString(i)->getValue();
                    if (!Main.options.assembleOnly) {
                        if (!hasCppFiles && (strends(implementer, ".cpp") || strends(implementer, ".c++"))) {
                            hasCppFiles = true;
                        }
                        nonScaleFiles.push_back(path + "/" + framework + ".framework/" + implDir + "/" + implementer);
                    }
                }
                for (unsigned long i = 0; implHeaders != nullptr && i < implHeaders->size() && implHeaderDir.size() > 0; i++) {
                    std::string header = framework + ".framework/" + implHeaderDir + "/" + implHeaders->getString(i)->getValue();
                    Main.frameworkNativeHeaders.push_back(header);
                }
                return true;
            }
        }
        return false;
    }

    std::string trimLeft(std::string s) {
        size_t i = 0;
        while (i < s.size() && isspace(s[i])) {
            i++;
        }
        return s.substr(i);
    }

    std::string compileLine(std::string line) {
        std::string out = "";
        out.reserve(line.size());
        bool inCode = false;
        for (size_t i = 0; i < line.size(); i++) {
            if (line[i] == '`') {
                inCode = !inCode;
            } else {
                if (inCode) {
                    std::string theCode = line.substr(i, line.find("`", i) - i);
                    i += theCode.size();
                    auto tokens = parseString(theCode);
                    theCode = "";
                    for (size_t i = 0; i < tokens.size(); i++) {
                        if (tokens[i].type == tok_eof) continue;
                        auto t = tokens[i];
                        size_t spacesBetween = 0;
                        if (((ssize_t) i) - 1 >= 0 && tokens[i - 1].type != tok_eof) {
                            spacesBetween = t.column - tokens[i - 1].column - tokens[i - 1].value.size();
                        }
                        theCode += std::string(spacesBetween, ' ');
                        theCode += t.formatted();
                    }
                    inCode = false;
                    out += Color::RESET + theCode + Color::GREEN;
                } else {
                    out += line[i];
                }
            }
        }
        return out;
    }

    Documentation parseSclDoc(std::string file) {
        FILE* docFile = fopen(file.c_str(), "rb");
        
        if (docFile) fseek(docFile, 0, SEEK_END);
        long sz = ftell(docFile);
        if (docFile) fseek(docFile, 0, SEEK_SET);

        char* data = new char[sz];

        fread(data, 1, sz, docFile);
        fclose(docFile);

        Documentation docs;

        std::vector<std::string> lines = split(std::string(data), "\n");
        std::string current = "";
        std::string currentModule = "";
        std::string implementedAt = "";
        std::string parentPath = std::filesystem::path(file).parent_path().string();
        for (size_t i = 0; i < lines.size(); i++) {
            std::string line = lines[i];
            if (strstarts(line, "@") && !strstarts(line, "@@")) {
                currentModule = trimLeft(line.substr(1));
            } else if (strstarts(line, "@@")) {
                implementedAt = trimLeft(line.substr(2));
            } else if (strstarts(line, "##") && !strstarts(line, "###")) {
                current = line.substr(3);
                docs.entries[current] = DocumentationEntries();
            } else if (strstarts(line, "###")) {
                std::string key = line.substr(4);
                DocumentationEntry e;
                e.name = key;
                e.module = currentModule;
                implementedAt = replaceAll(implementedAt, R"(\{scaleFolder\})", scaleFolder);
                implementedAt = replaceAll(implementedAt, R"(\{framework\})", Main.options.printDocFor);
                implementedAt = replaceAll(implementedAt, R"(\{module\})", currentModule);
                implementedAt = replaceAll(implementedAt, R"(\{frameworkPath\})", parentPath);
                e.file = implementedAt;
                
                line = lines[++i];
                auto isNextLine = [](std::string line) -> bool {
                    return strstarts(line, "##") || strstarts(line, "@");
                };
                while (i < lines.size()) {
                    line = lines[i++];
                    if (isNextLine(line)) {
                        i -= 2;
                        break;
                    }
                    e.description += "  " + compileLine(line) + "\n";
                }
                docs.entries[current].push_back(e);
            }
        }
        return docs;
    }

    auto docHandler(std::vector<std::string> args) {
        Documentation docs;
        std::vector<std::string> tmpFlags;
        std::vector<std::string> frameworks = {Main.options.printDocFor};
        bool hasCppFiles;
        Version FrameworkMinimumVersion(FRAMEWORK_VERSION_REQ);
        if (!checkFramework(Main.options.printDocFor, tmpFlags, frameworks, hasCppFiles, FrameworkMinimumVersion)) {
            return 1;
        }
        
        struct {
            std::vector<std::string> find;
            std::vector<std::string> find_category;
            bool help;
            bool categories;
        } DocOps = {
            std::vector<std::string>(),
            std::vector<std::string>(),
            false,
            false
        };

        for (size_t i = Main.options.docPrinterArgsStart + 1; i < args.size(); i++) {
            std::string arg = args[i];
            if (arg == "find") {
                if (i + 1 < args.size()) {
                    DocOps.find.push_back(args[i + 1]);
                    i++;
                } else {
                    std::cerr << "Error: find requires an argument" << std::endl;
                    return 1;
                }
            } else if (arg == "find-category") {
                if (i + 1 < args.size()) {
                    DocOps.find_category.push_back(args[i + 1]);
                    i++;
                } else {
                    std::cerr << "Error: find requires an argument" << std::endl;
                    return 1;
                }
            } else if (arg == "help" || arg == "-h" || arg == "--help") {
                DocOps.help = true;
            } else if (arg == "categories") {
                DocOps.categories = true;
            } else if (arg == "for") {
                if (i + 1 < args.size()) {
                    Main.options.printDocFor = args[i + 1];
                    i++;
                } else {
                    std::cerr << "Error: for requires an argument" << std::endl;
                    return 1;
                }
            } else {
                std::cout << "Unknown argument: " << arg << std::endl;
                DocOps.help = true;
            }
        }

        if (DocOps.help) {
            std::cout << "Scale Doc-Viewer help:" << std::endl;
            std::cout << "" << std::endl;
            std::cout << "  find <regex>             Find <regex> in documentation." << std::endl;
            std::cout << "  find-category <category> Find <category> in documentation." << std::endl;
            std::cout << "  help                     Display this help." << std::endl;
            std::cout << "  categories               Display categories." << std::endl;
            std::cout << "" << std::endl;
            return 0;
        }
        std::string file = Main.options.mapFrameworkDocfiles[Main.options.printDocFor];
        std::string docFileFormat = Main.options.mapFrameworkConfigs[Main.options.printDocFor]->getStringOrDefault("docfile-format", "markdown")->getValue();

        if (file.size() == 0 || !fileExists(file)) {
            std::cerr << Color::RED << "Framework '" + Main.options.printDocFor + "' has no docfile!" << Color::RESET << std::endl;
            return 1;
        }

        std::string includeFolder = Main.options.mapFrameworkIncludeFolders[Main.options.printDocFor];
        Main.options.docsIncludeFolder = includeFolder;

        if (docFileFormat == "markdown") {
            std::cerr << "Markdown Docfiles are not supported anymore!" << std::endl;
            return 1;
        } else if (docFileFormat == "scldoc") {
            docs = parseSclDoc(file);
        } else {
            std::cerr << Color::RED << "Invalid Docfile format: " << docFileFormat << Color::RESET << std::endl;
            return 1;
        }

        if (DocOps.categories) {
            std::cout << "Documentation for " << Main.options.printDocFor << std::endl;
            std::cout << "Categories: " << std::endl;

            std::string current = "";
            for (auto section : docs) {
                current = section.first;
                std::cout << "  " << Color::BOLDBLUE << current << Color::RESET << std::endl;
            }
            return 0;
        }

        std::cout << "Documentation for " << Main.options.printDocFor << std::endl;

        std::string current = "";
        for (auto&& section : docs) {
            current = section.first;

            bool found = true;
            if (DocOps.find_category.size()) {
                found = false;
                for (auto&& cat : DocOps.find_category) {
                    if (std::regex_search(current, std::regex(cat))) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found) continue;

            std::vector<DocumentationEntry> foundIn;

            for (DocumentationEntry e : section.second) {
                if (DocOps.find.size()) {
                    for (auto&& f : DocOps.find) {
                        if (
                            std::regex_search(e.name.begin(), e.name.end(), std::regex(f, std::regex::icase)) ||
                            std::regex_search(e.module.begin(), e.module.end(), std::regex(f, std::regex::icase)) ||
                            std::regex_search(e.description.begin(), e.description.end(), std::regex(f, std::regex::icase))
                        ) {
                            foundIn.push_back(e);
                        }
                    }
                } else {
                    foundIn.push_back(e);
                }
            }

            if (foundIn.size() == 0) continue;
            std::cout << Color::BOLDBLUE << current << ":" << Color::RESET << std::endl;
            for (DocumentationEntry e : foundIn) {
                std::cout << Color::BLUE << e.name << "\n";
                if (e.module.size()) {
                    std::cout << Color::CYAN << "Module: " << e.module;
                    if (e.file.size() == 0) std::cout << "\n";
                }
                if (e.file.size()) {
                    std::cout << " (" << e.file << ")\n";
                }
                std::cout << Color::GREEN << e.description;
            }
        }

        return 0;
    }

    int makeFramework(std::string name) {
        std::filesystem::create_directories(name + ".framework");
        std::filesystem::create_directories(name + ".framework/include");
        std::filesystem::create_directories(name + ".framework/impl");
        std::fstream indexFile((name + ".framework/index.drg").c_str(), std::fstream::out);

        DragonConfig::CompoundEntry* framework = new DragonConfig::CompoundEntry();
        framework->setKey("framework");

        framework->addString("version", "23.8");
        framework->addString("headerDir", "include");
        framework->addString("implDir", "impl");
        framework->addString("implHeaderDir", "impl");
        framework->addString("docfile", "");
        framework->addString("docfile-format", "scldoc");
        
        auto implementers = new DragonConfig::ListEntry();
        implementers->setKey("implementers");
        framework->addList(implementers);

        auto implHeaders = new DragonConfig::ListEntry();
        implHeaders->setKey("implHeaders");
        framework->addList(implHeaders);

        auto modules = new DragonConfig::CompoundEntry();
        modules->setKey("modules");
        framework->addCompound(modules);

        framework->print(indexFile);

        indexFile.close();
        return 0;
    }

    std::string rtTypeToSclType(std::string rtType);

    std::string demangleSymbol(std::string sym);

    std::string demangleLambda(std::string sym) {
        // "lambda[0@lambda[0@main()V;]()V;]()V;"
        std::string subExpr = sym.substr(sym.find("[") + 1, sym.find_last_of("]") - sym.find("[") - 1);
        std::string num = subExpr.substr(0, subExpr.find("@"));
        std::string ofFunction = subExpr.substr(subExpr.find("@") + 1);

        std::string args = sym.substr(("lambda[" + num + "@" + ofFunction + "]").size());
        std::string returnType = args.substr(args.find(")") + 1, args.size() - args.find(")") - 2);
        args = sym.substr(args.find("(") + 1, args.find(")") - args.find("(") - 1);
        std::vector<std::string> argTypes = split(args, ";");
        std::string demangled = "lambda(";
        for (size_t i = 0; i < argTypes.size() - 1; i++) {
            if (i)
                demangled += ", ";
            demangled += ":" + rtTypeToSclType(argTypes[i]);
        }
        demangled += "): " + rtTypeToSclType(returnType);
        demangled += " #" + std::to_string(std::atoi(num.c_str()) + 1);
        demangled += " in " + demangleSymbol(ofFunction);
        return demangled;
    }

    std::string demangleSymbol(std::string sym) {
        std::string varPrefix = TO_STRING(__USER_LABEL_PREFIX__) + std::string("Var_");
        if (strstarts(sym, varPrefix) || strstarts(sym, "Var_")) {
            if (strstarts(sym, varPrefix))
                sym = sym.substr(varPrefix.size());
            else
                sym = sym.substr(4);
            
            sym = replaceAll(sym, "\\$", "::");
            return sym;
        }
        if (strstarts(sym, TO_STRING(__USER_LABEL_PREFIX__))) {
            sym = sym.substr(std::string(TO_STRING(__USER_LABEL_PREFIX__)).size());
        }
        if (strstarts(sym, "lambda[")) {
            return demangleLambda(sym);
        }
        if (sym.find("(") == std::string::npos || sym.find(")") == std::string::npos) {
            return sym;
        }
        std::string name = "";
        if (!strstarts(sym, "(")) {
            name = sym.substr(0, sym.find("("));
        }
        std::string returnType = sym.substr(sym.find(")") + 1, sym.size() - sym.find(")") - 2);
        std::string args = sym.substr(sym.find("(") + 1, sym.find(")") - sym.find("(") - 1);
        std::vector<std::string> argTypes = split(args, ";");
        std::string demangled = name + "(";
        for (size_t i = 0; i < argTypes.size() - 1; i++) {
            if (i)
                demangled += ", ";
            demangled += ":" + rtTypeToSclType(argTypes[i]);
        }
        demangled += "): " + rtTypeToSclType(returnType);
        return demangled;
    }

    int compileRuntimeLib() {
        std::vector<std::string> cmd = {
            std::string("clang"),
            "-O2",
#if defined(__APPLE__)
            "-dynamiclib",
#elif defined(__linux__)
            "-fPIC",
            "-shared",
#endif
            "-undefined",
            "dynamic_lookup",
            "-lgc",
            "-I" + scaleFolder + "/Internal",
            scaleFolder + "/Internal/runtime_vars.c",
            scaleFolder + "/Internal/scale_runtime.c",
            "-o",
            scaleFolder + "/Internal/" + std::string(LIB_SCALE_FILENAME)
        };
        std::string cmdStr = "";
        for (auto& s : cmd) {
            cmdStr += s + " ";
        }

        std::cout << Color::BLUE << "Compiling runtime library..." << std::endl;
        std::cout << Color::CYAN << cmdStr << Color::RESET << std::endl;
        return system(cmdStr.c_str());
    }

    int main(std::vector<std::string> args) {
        using clock = std::chrono::high_resolution_clock;

        auto start = clock::now();

        std::string outfile     = std::string(DEFAULT_OUTFILE);
        std::string compiler    = std::string(COMPILER);
        scaleFolder             = std::string(SCL_ROOT_DIR) + "/" + std::string(SCALE_INSTALL_DIR) + "/" + std::string(VERSION);
        scaleLatestFolder       = std::string(SCL_ROOT_DIR) + "/" + std::string(SCALE_INSTALL_DIR) + "/latest";

        std::vector<std::string> frameworks;
        std::vector<std::string> tmpFlags;
        std::string optimizer   = "O0";
        bool hasCppFiles        = false;
        bool hasFilesFromArgs   = false;
        bool outFileSpecified   = false;

        Main.options.includePaths.push_back(scaleFolder + "/Frameworks");
        Main.options.includePaths.push_back(".");

        if (args[0] == "scaledoc") {
            Main.options.docPrinterArgsStart = 0;
            Main.options.printDocFor = "Scale";
            return docHandler(args);
        }

        if (args.size() < 2) {
            usage(args[0]);
            return 1;
        }

        srand(time(NULL));
        Main.options.operatorRandomData = gen_random();
        tmpFlags.reserve(args.size());

        Main.version = new Version(std::string(VERSION));

        std::string libScaleRuntimeFileName = std::string(LIB_SCALE_FILENAME);
        if (!std::filesystem::exists(std::filesystem::path(scaleFolder) / "Internal" / libScaleRuntimeFileName)) {
            int ret = compileRuntimeLib();
            if (ret) {
                std::cout << Color::RED << "Failed to compile runtime library" << std::endl;
                return ret;
            }
        }

        DragonConfig::CompoundEntry* scaleConfig = DragonConfig::ConfigParser().parse("scale.drg");
        if (scaleConfig) {
            if (scaleConfig->hasMember("outfile"))
                Main.options.outfile = outfile = scaleConfig->getString("outfile")->getValue();

            if (scaleConfig->hasMember("compiler"))
                compiler = scaleConfig->getString("compiler")->getValue();

            if (scaleConfig->hasMember("optimizer"))
                optimizer = scaleConfig->getString("optimizer")->getValue();

            if (scaleConfig->hasMember("frameworks"))
                for (size_t i = 0; i < scaleConfig->getList("frameworks")->size(); i++)
                    frameworks.push_back(scaleConfig->getList("frameworks")->getString(i)->getValue());

            if (scaleConfig->hasMember("compilerFlags"))
                for (size_t i = 0; i < scaleConfig->getList("compilerFlags")->size(); i++)
                    tmpFlags.push_back(scaleConfig->getList("compilerFlags")->getString(i)->getValue());

            if (scaleConfig->hasMember("includeFiles"))
                for (size_t i = 0; i < scaleConfig->getList("includeFiles")->size(); i++)
                    Main.frameworkNativeHeaders.push_back(scaleConfig->getList("includeFiles")->getString(i)->getValue());
            
            if (scaleConfig->hasMember("featureFlags"))
                for (size_t i = 0; i < scaleConfig->getList("featureFlags")->size(); i++)
                    Main.options.features.push_back(scaleConfig->getList("featureFlags")->getString(i)->getValue());
        }
        Main.options.mapFrameworkConfigs["/local"] = scaleConfig;

        Main.options.minify = true;
        Main.options.stackSize = 16;

        for (size_t i = 1; i < args.size(); i++) {
            if (!hasCppFiles && (strends(args[i], ".cpp") || strends(args[i], ".c++"))) {
                hasCppFiles = true;
            }
            if (strends(args[i], ".c") || strends(args[i], ".cpp") || strends(args[i], ".c++")) {
                nonScaleFiles.push_back(args[i]);
            } else if (strends(std::string(args[i]), ".scale") || strends(std::string(args[i]), ".smod")) {
                if (!fileExists(args[i])) {
                    continue;
                }
                std::string file = std::filesystem::absolute(args[i]).string();
                if (!contains(Main.options.files, file)) {
                    Main.options.files.push_back(file);
                    Main.options.filesFromCommandLine.push_back(file);
                    hasFilesFromArgs = true;
                }
            } else {
                if (args[i] == "--transpile" || args[i] == "-t") {
                    Main.options.transpileOnly = true;
                } else if (args[i] == "--help" || args[i] == "-h") {
                    usage(args[0]);
                    return 0;
                } else if (args[i] == "-E") {
                    Main.options.preprocessOnly = true;
                } else if (args[i] == "-f") {
                    if (i + 1 < args.size()) {
                        std::string framework = args[i + 1];
                        bool alreadyIncluded = false;
                        for (auto f : frameworks) {
                            if (f == framework) {
                                alreadyIncluded = true;
                            }
                        }
                        if (!alreadyIncluded)
                            frameworks.push_back(framework);
                        i++;
                    } else {
                        std::cerr << "Error: -f requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "-o") {
                    if (i + 1 < args.size()) {
                        outFileSpecified = true;
                        Main.options.outfile = outfile = args[i + 1];
                        i++;
                    } else {
                        std::cerr << "Error: -o requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "-S") {
                    Main.options.assembleOnly = true;
                    Main.options.dontSpecifyOutFile = true;
                    tmpFlags.push_back("-S");
                } else if (args[i] == "--no-main" || args[i] == "-no-main") {
                    Main.options.noMain = true;
                } else if (args[i] == "-no-minify") {
                    Main.options.minify = false;
                } else if (args[i] == "-v" || args[i] == "--version") {
                    std::cout << "Scale Compiler version " << std::string(VERSION) << std::endl;
                    system((compiler + " -v").c_str());
                    return 0;
                } else if (args[i] == "-feat") {
                    if (i + 1 < args.size()) {
                        Main.options.features.push_back(args[i + 1]);
                        i++;
                    } else {
                        std::cerr << "Error: -feat requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "--comp" || args[i] == "-compiler") {
                    if (i + 1 < args.size()) {
                        compiler = args[i + 1];
                        i++;
                    } else {
                        std::cerr << "Error: " << args[i] << " requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "-run") {
                    Main.options.doRun = true;
                } else if (args[i] == "-debug") {
                    Main.options.debugBuild = true;
                    Main.options.minify = false;
                    tmpFlags.push_back("-DSCL_DEBUG=1");
                } else if (args[i] == "-cflags") {
                    Main.options.printCflags = true;
                } else if (args[i] == "-makelib") {
        #if defined(__APPLE__)
                    tmpFlags.push_back("-dynamiclib");
        #else
                    tmpFlags.push_back("-shared");
                    tmpFlags.push_back("-fPIC");
        #endif
                    tmpFlags.push_back("-undefined");
                    tmpFlags.push_back("dynamic_lookup");
                    Main.options.noMain = true;
                    tmpFlags.push_back("-DSCL_COMPILER_NO_MAIN");
                    if (!outFileSpecified)
                    #if defined(__APPLE__)
                        Main.options.outfile = outfile = "libout.dylib";
                    #else
                        Main.options.outfile = outfile = "libout.so";
                    #endif
                } else if (args[i] == "--dump-parsed-data" || args[i] == "-create-binary-header" || args[i] == "-create-module") {
                    Main.options.dumpInfo = true;
                    Main.options.binaryHeader = (args[i] == "-create-binary-header");
                    if (!outFileSpecified)
                        Main.options.outfile = outfile = "out.smod";
                } else if (args[i] == "-no-scale-std") {
                    Main.options.noScaleFramework = true;
                } else if (args[i] == "-demangle-symbol") {
                    std::string sym;
                    if (i + 1 < args.size()) {
                        std::cout << demangleSymbol(args[i + 1]) << std::endl;
                    } else {
                        while (std::cin) {
                            std::getline(std::cin, sym);
                            if (sym.size())
                                std::cout << demangleSymbol(sym) << std::endl;
                        }
                    }
                    return 0;
                } else if (args[i] == "-create-framework") {
                    if (i + 1 < args.size()) {
                        std::string name = args[i + 1];
                        i++;
                        return makeFramework(name);
                    } else {
                        std::cerr << "Error: " << args[i] << " requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "-stack-size") {
                    if (i + 1 < args.size()) {
                        Main.options.stackSize = std::stoull(args[i + 1]);
                        i++;
                    } else {
                        std::cerr << "Error: -stack-size requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "-clear-cache") {
                    std::string cacheDir = scaleFolder + "/tmp";
                    std::filesystem::remove_all(cacheDir);
                    std::filesystem::create_directory(cacheDir);
                    return 0;
                } else if (args[i] == "-doc-for") {
                    if (i + 1 < args.size()) {
                        Main.options.printDocFor = args[i + 1];
                        i++;
                        Main.options.docPrinterArgsStart = i;
                    } else {
                        std::cerr << "Error: -doc-for requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "-no-error-location") {
                    Main.options.noErrorLocation = true;
                } else {
                    if (args[i].size() >= 2 && args[i][0] == '-' && args[i][1] == 'I') {
                        Main.options.includePaths.push_back(std::string(args[i].c_str() + 2));
                    } else if (args[i] == "-I") {
                        if (i + 1 < args.size()) {
                            Main.options.includePaths.push_back(args[i + 1]);
                            i++;
                        } else {
                            std::cerr << "Error: -I requires an argument" << std::endl;
                            return 1;
                        }
                    } else if (args[i].size() >= 2 && args[i][0] == '-' && args[i][1] == 'O') {
                        optimizer = std::string(args[i].c_str() + 1);
                    } else if (args[i] == "-c") {
                        Main.options.dontSpecifyOutFile = true;
                    } else if (args[i] == "-Werror") {
                        Main.options.Werror = true;
                    }
                    tmpFlags.push_back(args[i]);
                }
            }
        }

        if (!hasFilesFromArgs) {
            Main.options.noMain = true;
            if (!outFileSpecified)
                Main.options.outfile = outfile = "a.out";
        }

        cflags.reserve(tmpFlags.size() + 10);
        Main.options.optimizer = optimizer;

        if (!Main.options.printCflags)
            cflags.push_back(compiler);

        cflags.push_back("-I" + scaleFolder + "/Internal");
        cflags.push_back("-I" + scaleFolder + "/Frameworks");
        cflags.push_back("-I.");
        cflags.push_back("-L" + scaleFolder + "/Internal");
        cflags.push_back("-lScaleRuntime");
        cflags.push_back("-" + optimizer);
        cflags.push_back("-DVERSION=\"" + std::string(VERSION) + "\"");
        
        std::string source;
        bool alreadyIncluded = false;
        for (auto f : frameworks) {
            if (f == "Scale") {
                alreadyIncluded = true;
            }
        }
        if (!Main.options.noScaleFramework && !alreadyIncluded) {
            frameworks.push_back("Scale");
        }

        Version FrameworkMinimumVersion = Version(std::string(FRAMEWORK_VERSION_REQ));

        for (std::string& framework : frameworks) {
            checkFramework(framework, tmpFlags, frameworks, hasCppFiles, FrameworkMinimumVersion);
        }

        if (!Main.options.noScaleFramework) {
            auto findModule = [&](const std::string moduleName) -> std::vector<std::string> {
                std::vector<std::string> mods;
                for (auto config : Main.options.mapFrameworkConfigs) {
                    auto modules = config.second->getCompound("modules");
                    if (modules) {
                        auto list = modules->getList(moduleName);
                        if (list) {
                            for (size_t j = 0; j < list->size(); j++) {
                                FPResult find = findFileInIncludePath(list->getString(j)->getValue());
                                if (!find.success) {
                                    return std::vector<std::string>();
                                }
                                auto file = find.in;
                                mods.push_back(file);
                            }
                            break;
                        }
                    }
                }
                return mods;
            };

            auto mod = findModule("std");
            for (auto file : mod) {
                file = std::filesystem::absolute(file).string();
                if (!contains(Main.options.files, file)) {
                    Main.options.files.push_back(file);
                }
            }
        }

        if (!Main.options.doRun && !Main.options.dumpInfo) std::cout << "Scale Compiler version " << std::string(VERSION) << std::endl;

        if (Main.options.printDocs ||  Main.options.printDocFor.size() != 0) {
            if (Main.options.printDocs || (Main.options.printDocFor.size() && contains(frameworks, Main.options.printDocFor))) {
                return docHandler(args);
            }
            std::cerr << Color::RED << "Framework '" + Main.options.printDocFor + "' not found!" << Color::RESET << std::endl;
            return 1;
        }
        
        std::vector<Token>  tokens;

        for (size_t i = 0; i < Main.options.files.size() && !Main.options.printCflags; i++) {
            using path = std::filesystem::path;
            path s = Main.options.files[i];
            if (s.parent_path().string().size())
                Main.options.includePaths.push_back(s.parent_path().string());
        }

        for (size_t i = 0; i < Main.options.files.size() && !Main.options.printCflags; i++) {
            std::string filename = Main.options.files[i];

            if (!std::filesystem::exists(filename)) {
                std::cout << Color::BOLDRED << "Fatal Error: File " << filename << " does not exist!" << Color::RESET << std::endl;
                return 1;
            }

            if (!strends(filename, ".scale")) {
                continue;
            }

            Tokenizer tokenizer;
            Main.tokenizer = &tokenizer;
            FPResult result = Main.tokenizer->tokenize(filename);

            if (result.warns.size() > 0) {
                for (FPResult error : result.warns) {
                    if (error.type > tok_char_literal) continue;
                    if (error.line == 0) {
                        std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                        continue;
                    }
                    FILE* f = fopen(std::string(error.in).c_str(), "r");
                    if (!f) {
                        std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                        continue;
                    }
                    char* line = (char*) malloc(sizeof(char) * 500);
                    int i = 1;
                    if (f) fseek(f, 0, SEEK_SET);
                    std::string colString = Color::BOLDMAGENTA;
                    if (Main.options.noErrorLocation) colString = "";
                    std::cerr << colString << "Warning: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                    i = 1;
                    if (Main.options.noErrorLocation) {
                        std::cout << "Token: " << error.value << std::endl;
                    } else {
                        while (fgets(line, 500, f) != NULL) {
                            if (i == error.line) {
                                std::cerr << colString << "> " << Color::RESET << replaceFirstAfter(line, error.value, Color::BOLDMAGENTA + error.value + Color::RESET, error.column) << Color::RESET;
                            } else if (i == error.line - 1 || i == error.line - 2) {
                                if (strlen(line) > 0)
                                    std::cerr << "  " << line;
                            } else if (i == error.line + 1 || i == error.line + 2) {
                                if (strlen(line) > 0)
                                    std::cerr << "  " << line;
                            }
                            i++;
                        }
                    }
                    fclose(f);
                    if (!Main.options.noErrorLocation) std::cerr << std::endl;
                    free(line);
                }
            }
            if (result.errors.size() > 0) {
                for (FPResult error : result.errors) {
                    if (error.type > tok_char_literal) continue;
                    std::string colorStr;
                    if (error.isNote) {
                        colorStr = Color::BOLDCYAN;
                    } else {
                        colorStr = Color::BOLDRED;
                    }
                    if (error.line == 0) {
                        std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                        continue;
                    }
                    FILE* f = fopen(std::string(error.in).c_str(), "r");
                    if (!f) {
                        std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                        continue;
                    }
                    char* line = (char*) malloc(sizeof(char) * 500);
                    int i = 1;
                    if (f) fseek(f, 0, SEEK_SET);
                    if (Main.options.noErrorLocation) colorStr = "";
                    std::cerr << colorStr << (error.isNote ? "Note" : "Error") << ": " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                    i = 1;
                    if (Main.options.noErrorLocation) {
                        std::cout << "Token: " << error.value << std::endl;
                    } else {
                        while (fgets(line, 500, f) != NULL) {
                            if (i == error.line) {
                                std::cerr << colorStr << "> " << Color::RESET << replaceFirstAfter(line, error.value, colorStr + error.value + Color::RESET, error.column) << Color::RESET;
                            } else if (i == error.line - 1 || i == error.line - 2) {
                                if (strlen(line) > 0)
                                    std::cerr << "  " << line;
                            } else if (i == error.line + 1 || i == error.line + 2) {
                                if (strlen(line) > 0)
                                    std::cerr << "  " << line;
                            }
                            i++;
                        }
                    }
                    fclose(f);
                    if (!Main.options.noErrorLocation) std::cerr << std::endl;
                    free(line);
                }
                std::cout << "Failed!" << std::endl;
                return result.errors.size();
            }

            FPResult importResult = Main.tokenizer->tryImports();
            if (!importResult.success) {
                std::cerr << Color::BOLDRED << "Include Error: " << importResult.in << ":" << importResult.line << ":" << importResult.column << ": " << importResult.message << std::endl;
                return 1;
            }

            std::vector<Token> theseTokens = Main.tokenizer->getTokens();
            
            tokens.insert(tokens.end(), theseTokens.begin(), theseTokens.end());
        }

        if (Main.options.preprocessOnly) {
            std::cout << "Preprocessed " << Main.options.files.size() << " files." << std::endl;
            return 0;
        }

        TPResult result;
        if (!Main.options.printCflags) {
            SyntaxTree lexer(tokens);
            Main.lexer = &lexer;
            std::vector<std::string> binaryHeaders;
            for (std::string& header : Main.options.files) {
                if (strends(header, ".smod")) {
                    binaryHeaders.push_back(header);
                }
            }
            result = Main.lexer->parse(binaryHeaders);
        }

        if (!Main.options.printCflags && result.warns.size() > 0) {
            for (FPResult error : result.warns) {
                if (error.type > tok_char_literal) continue;
                if (error.line == 0) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                    continue;
                }
                FILE* f = fopen(std::string(error.in).c_str(), "r");
                if (!f) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                    continue;
                }
                char* line = (char*) malloc(sizeof(char) * 500);
                int i = 1;
                if (f) fseek(f, 0, SEEK_SET);
                std::string colString = Color::BOLDMAGENTA;
                if (Main.options.noErrorLocation) colString = "";
                std::cerr << colString << "Warning: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                i = 1;
                if (Main.options.noErrorLocation) {
                    std::cout << "Token: " << error.value << std::endl;
                } else {
                    while (fgets(line, 500, f) != NULL) {
                        if (i == error.line) {
                            std::cerr << colString << "> " << Color::RESET << replaceFirstAfter(line, error.value, Color::BOLDMAGENTA + error.value + Color::RESET, error.column) << Color::RESET;
                        } else if (i == error.line - 1 || i == error.line - 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        } else if (i == error.line + 1 || i == error.line + 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        }
                        i++;
                    }
                }
                fclose(f);
                if (!Main.options.noErrorLocation) std::cerr << std::endl;
                free(line);
            }
        }
        if (!Main.options.printCflags && result.errors.size() > 0) {
            for (FPResult error : result.errors) {
                if (error.type > tok_char_literal) continue;
                std::string colorStr;
                if (error.isNote) {
                    colorStr = Color::BOLDCYAN;
                } else {
                    colorStr = Color::BOLDRED;
                }
                if (error.line == 0) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                    continue;
                }
                FILE* f = fopen(std::string(error.in).c_str(), "r");
                if (!f) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                    continue;
                }
                char* line = (char*) malloc(sizeof(char) * 500);
                int i = 1;
                if (f) fseek(f, 0, SEEK_SET);
                
                
                std::cerr << colorStr << (error.isNote ? "Note" : "Error") << ": " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                i = 1;
                if (Main.options.noErrorLocation) {
                    std::cout << "Token: " << error.value << std::endl;
                } else {
                    while (fgets(line, 500, f) != NULL) {
                        if (i == error.line) {
                            std::cerr << colorStr << "> " << Color::RESET << replaceFirstAfter(line, error.value, colorStr + error.value + Color::RESET, error.column) << Color::RESET;
                        } else if (i == error.line - 1 || i == error.line - 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        } else if (i == error.line + 1 || i == error.line + 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        }
                        i++;
                    }
                }
                fclose(f);
                if (!Main.options.noErrorLocation) std::cerr << std::endl;
                free(line);
            }
            std::cout << "Failed!" << std::endl;
            return result.errors.size();
        }

        if (Main.options.dumpInfo) {
            InfoDumper::dump(result);
            return 0;
        }

        source = "out.c";
        Parser parser(result);
        Main.parser = &parser;

        if (!hasCppFiles) {
            // -std= only works when there are no c++ files to build
            cflags.push_back("-std=" + std::string(C_VERSION));
        } else {
            // link to c++ library if needed
            cflags.push_back("-lc++");
        }

        for (std::string& s : tmpFlags) {
            cflags.push_back(s);
        }

        FPResult parseResult = Main.parser->parse(source);
        if (parseResult.warns.size() > 0) {
            for (FPResult error : parseResult.warns) {
                if (error.type > tok_char_literal) continue;
                if (error.line == 0) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                    continue;
                }
                FILE* f = fopen(std::string(error.in).c_str(), "r");
                if (!f) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                    continue;
                }
                char* line = (char*) malloc(sizeof(char) * 500);
                int i = 1;
                if (f) fseek(f, 0, SEEK_SET);
                std::string colString = Color::BOLDMAGENTA;
                if (Main.options.noErrorLocation) colString = "";
                std::cerr << colString << "Warning: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                i = 1;
                if (Main.options.noErrorLocation) {
                    std::cout << "Token: " << error.value << std::endl;
                } else {
                    while (fgets(line, 500, f) != NULL) {
                        if (i == error.line) {
                            std::cerr << colString << "> " << Color::RESET << replaceFirstAfter(line, error.value, Color::BOLDMAGENTA + error.value + Color::RESET, error.column) << Color::RESET;
                        } else if (i == error.line - 1 || i == error.line - 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        } else if (i == error.line + 1 || i == error.line + 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        }
                        i++;
                    }
                }
                fclose(f);
                if (!Main.options.noErrorLocation) std::cerr << std::endl;
                free(line);
            }
        }
        if (parseResult.errors.size() > 0) {
            for (FPResult error : parseResult.errors) {
                if (error.type > tok_char_literal) continue;
                std::string colorStr;
                ssize_t addAtCol = -1;
                std::string strToAdd = "";
                if (error.isNote) {
                    colorStr = Color::BOLDCYAN;
                    size_t i = error.message.find('\\');
                    if (i != std::string::npos) {
                        std::string cmd = error.message.substr(i + 1);
                        auto data = split(cmd, ";");
                        error.message = error.message.substr(0, i);
                        if (data[0] == "insertText") {
                            strToAdd = Color::BOLDGREEN + data[1] + Color::RESET;
                            auto pos = split(data[2], ":");
                            addAtCol = std::atoi(pos[1].c_str()) - 1;
                        }
                    }
                } else {
                    colorStr = Color::BOLDRED;
                }
                if (error.line == 0) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                    continue;
                }
                FILE* f = fopen(std::string(error.in).c_str(), "r");
                if (!f) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.in << ": " << error.message << Color::RESET << std::endl;
                    continue;
                }
                char* line = (char*) malloc(sizeof(char) * 500);
                int i = 1;
                if (f) fseek(f, 0, SEEK_SET);
                if (Main.options.noErrorLocation) colorStr = "";
                std::cerr << colorStr << (error.isNote ? "Note" : "Error") << ": " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                i = 1;
                if (Main.options.noErrorLocation) {
                    std::cout << "Token: " << error.value << std::endl;
                } else {
                    while (fgets(line, 500, f) != NULL) {
                        if (i == error.line) {
                            if (strToAdd.size())
                                std::cerr << colorStr << "> " << Color::RESET << std::string(line).insert(addAtCol, strToAdd) << Color::RESET;
                            else
                                std::cerr << colorStr << "> " << Color::RESET << replaceFirstAfter(line, error.value, colorStr + error.value + Color::RESET, error.column) << Color::RESET;
                        } else if (i == error.line - 1 || i == error.line - 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        } else if (i == error.line + 1 || i == error.line + 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        }
                        i++;
                    }
                }
                fclose(f);
                if (!Main.options.noErrorLocation) std::cerr << std::endl;
                free(line);
            }
            std::cout << "Failed!" << std::endl;
            return parseResult.errors.size();
        }

        if (Main.options.transpileOnly) {
            auto end = clock::now();
            double duration = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000000000.0;
            std::cout << "Transpiled successfully in " << duration << " seconds." << std::endl;
            return 0;
        }

        std::vector<int> pids;

        for (std::string& s : Main.options.flags) {
            if (std::filesystem::exists(s + ".o") && file_modified_time(s + ".o") > file_modified_time(s)) continue;

            auto tmpFlags = cflags;

            tmpFlags.push_back(s);
            tmpFlags.push_back("-c");

            tmpFlags.push_back("-o");
            tmpFlags.push_back(s + ".o");

            char** args = (char**) malloc(sizeof(char*) * tmpFlags.size() + 1);
            int index = 0;
            for (size_t i = 0; i < tmpFlags.size(); i++) {
                if (strstarts(tmpFlags[i], "-l")) continue;
                if (strstarts(tmpFlags[i], "-L")) continue;

                args[index++] = (char*) malloc(sizeof(char) * tmpFlags[i].size() + 1);
                strcpy(args[index - 1], tmpFlags[i].c_str());
            }
            args[index] = NULL;

            int pid = fork();
            if (pid == 0) {
                int ret = execvp(args[0], (char**) args);
                if (ret == -1) {
                    std::cerr << "Failed to execute command: " << args[0] << std::endl;
                    return ret;
                }
            } else if (pid < 0) {
                std::cerr << "Failed to fork process" << std::endl;
                return 1;
            }
            pids.push_back(pid);
        }

        for (std::string& file : nonScaleFiles) {
            cflags.push_back("\"" + file + "\"");
        }

        for (std::string& s : Main.options.flags) {
            cflags.push_back("\"" + s + ".o\"");
        }

        if (!Main.options.printCflags && !Main.options.dontSpecifyOutFile) {
            cflags.push_back("-o");
            cflags.push_back("\"" + outfile + "\"");
        }
        
#ifdef LINK_MATH
        cflags.push_back("-lm");
#endif

        std::string cmd = "";
        for (std::string& s : cflags) {
            cmd += s + " ";
        }
        if (Main.options.debugBuild) {
            cmd += "-DSCL_DEBUG ";
        }

        for (int pid : pids) {
            int status;
            waitpid(pid, &status, 0);
            if (status != 0) {
                std::cerr << Color::RED << "Failed to compile!" << Color::RESET << std::endl;
                return 1;
            }
        }

        int errors = 0;
        if (Main.options.printCflags) {
            std::cout << cmd << std::endl;
            return 0;
        } else if (!Main.options.transpileOnly) {
            FILE* compile_command = popen((cmd + " 2>&1").c_str(), "r");

            if (!compile_command) {
                std::cerr << Color::RED << "Failed to compile!" << Color::RESET << std::endl;
                return 1;
            }

            char* cline = (char*) malloc(sizeof(char) * 1024);
            bool doCheckForSymbols = false;
            std::map<std::string, std::vector<std::string>> undefinedSymbols;
            std::string currentUndefinedSymbol = "";
            while (fgets(cline, 1024, compile_command) != NULL) {
                std::string line = cline;
                if (strstarts(line, "out.c") || strstarts(line, "out.h") || strstarts(line, "out.typeinfo.h")) {
                    size_t space = line.find(" ");
                    std::string file = line.substr(0, space);
                    line = line.substr(space + 1);
                    std::string col = Color::RED;
                    if (strstarts(line, "note")) {
                        continue;
                    } else if (strstarts(line, "warning")) {
                        col = Color::BLUE;
                    }
                    if (strstarts(line, "error: definition with same mangled name '")) {
                        std::string sym = line.substr(42);
                        line = "Error:" + Color::RESET + " Multiple functions with the same symbol '" + sym.substr(0, sym.find("'")) + "' defined.\n";
                        col = Color::BOLDRED;
                    }
                    std::cerr << file << " " << col << line << Color::RESET;
                } else {
                    if (strcontains(line, "symbol(s) not found for architecture") || strstarts(line, "clang: error: linker command failed with exit code 1 (use -v to see invocation)")) {
                        continue;
                    }
                    if (strstarts(line, "clang: warning")) {
                        continue;
                    }
                    if (strstarts(line, "Undefined symbols for architecture ")) {
                        doCheckForSymbols = true;
                    } else if (doCheckForSymbols && strstarts(line, "  \"")) {
                        std::string sym = line.substr(line.find("\"") + 1);
                        sym = sym.substr(0, sym.find("\""));
                        currentUndefinedSymbol = sym;
                        undefinedSymbols[sym] = std::vector<std::string>();
                    } else if (doCheckForSymbols && !strstarts(line, "  \"")) {
                        undefinedSymbols[currentUndefinedSymbol].push_back(line.find_first_not_of(" ") == std::string::npos ? "" : line.substr(line.find_first_not_of(" ")));
                    } else if (strstarts(line, "ld: library not found for -l")) {
                        std::string lib = line.substr(28);
                        lib = replaceAll(lib, "\n", "");
                        std::cerr << Color::RED << "Error: " << Color::RESET << "Library '" << lib << "' not found." << std::endl;
                    } else {
                        std::cerr << Color::BOLDRED << line << Color::RESET;
                    }
                }
                errors++;
            }

            if (doCheckForSymbols) {
                std::cerr << Color::BOLDRED << "Undefined symbols: " << Color::RESET << std::endl;
                for (auto sym : undefinedSymbols) {
                    std::cerr << Color::RED << "  Symbol '" << sym.first << "' referenced from:" << std::endl;
                    for (auto line : sym.second) {
                        if (line.size() > 0)
                            std::cerr << "    " << Color::CYAN << line;
                    }
                }
                std::cerr << Color::RESET << std::endl;
            }

            free(cline);

            if (errors) {
                std::cout << Color::RED << "Compilation failed!" << std::endl;
                return errors;
            }

            remove(source.c_str());
            remove((source.substr(0, source.size() - 2) + ".h").c_str());
        }

        if (!Main.options.doRun) std::cout << Color::GREEN << "Compilation finished." << Color::RESET << std::endl;

        auto end = clock::now();
        double duration = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000000000.0;
        if (!Main.options.doRun) std::cout << "Took " << duration << " seconds." << std::endl;

        if (Main.options.doRun) {
            const char** argv = new const char*[1];
            argv[0] = (const char*) outfile.c_str();
            #ifdef _WIN32
                return execv(argv[0], (const char* const*) argv);
            #else
                return execv(argv[0], (char* const*) argv);
            #endif
        }

        return 0;
    }
}

int main(int argc, char const *argv[]) {
    signal(SIGSEGV, sclc::signalHandler);
    signal(SIGABRT, sclc::signalHandler);
    signal(SIGILL, sclc::signalHandler);
    signal(SIGINT, sclc::signalHandler);
    signal(SIGTERM, sclc::signalHandler);

    if (!isatty(fileno(stdout))) {
        sclc::Color::RESET = "";
        sclc::Color::BLACK = "";
        sclc::Color::RED = "";
        sclc::Color::GREEN = "";
        sclc::Color::YELLOW = "";
        sclc::Color::BLUE = "";
        sclc::Color::MAGENTA = "";
        sclc::Color::CYAN = "";
        sclc::Color::WHITE = "";
        sclc::Color::BOLDBLACK = "";
        sclc::Color::BOLDRED = "";
        sclc::Color::BOLDGREEN = "";
        sclc::Color::BOLDYELLOW = "";
        sclc::Color::BOLDBLUE = "";
        sclc::Color::BOLDMAGENTA = "";
        sclc::Color::BOLDCYAN = "";
        sclc::Color::BOLDWHITE = "";
    }

    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        args.push_back(argv[i]);
    }
    return sclc::main(args);
}
