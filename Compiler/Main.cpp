#include <gc/gc_allocator.h>

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
#define C_VERSION "gnu17"
#endif

#ifndef SCALE_INSTALL_DIR
#define SCALE_INSTALL_DIR "Scale"
#endif

#ifndef DEFAULT_OUTFILE
#define DEFAULT_OUTFILE "out.scl"
#endif

#ifndef FRAMEWORK_VERSION_REQ
#define FRAMEWORK_VERSION_REQ "24.2.0"
#endif

#ifndef SCL_ROOT_DIR
#define SCL_ROOT_DIR "/opt"
#endif

#define CONCAT(a, b) CONCAT_(a, b)
#define CONCAT_(a, b) a ## b

#if defined(__APPLE__)
#define LIB_PREF "lib"
#define LIB_SUFF ".dylib"
#elif defined(__linux__)
#define LIB_PREF "lib"
#define LIB_SUFF ".so"
#elif defined(_WIN32)
#define LIB_PREF ""
#define LIB_SUFF ".dll"
#endif

#define LIB_SCALE_FILENAME CONCAT(CONCAT(LIB_PREF, "ScaleRuntime"), LIB_SUFF)

#define TO_STRING2(x) #x
#define TO_STRING(x) TO_STRING2(x)

namespace sclc
{
    std::string scaleFolder;
    std::string scaleLatestFolder;
    std::vector<std::string> nonScaleFiles;
    std::vector<std::string> cflags;
    std::vector<std::regex> disabledDiagnostics;

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
        std::cout << "  -makelib             Compile as a library (implies -no-main)" << std::endl;
        std::cout << "  -create-module       Create a Scale module" << std::endl;
        std::cout << "  -cflags              Print c compiler flags and exit" << std::endl;
        std::cout << "  -no-error-location   Do not print an overview of the file on error" << std::endl;
        std::cout << "  -nowarn <regex>      Do not print any diagnostics where the message matches the given regular expression" << std::endl;
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
        while (token.type != tok_eof) {
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
        for (auto path : Main::options::includePaths) {
            if (fileExists(path + "/" + framework + ".framework/index.drg")) {
                DragonConfig::ConfigParser parser;
                DragonConfig::CompoundEntry* root = parser.parse(path + "/" + framework + ".framework/index.drg");
                if (root == nullptr) {
                    std::cerr << Color::RED << "Failed to parse index.drg of Framework " << framework << std::endl;
                    return false;
                }
                root = root->getCompound("framework");
                Main::options::indexDrgFiles[framework] = root;
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
                Main::options::mapFrameworkIncludeFolders[framework] = headerDir;
                
                DragonConfig::StringEntry* implDirTag = root->getString("implDir");
                std::string implDir = implDirTag == nullptr ? "" : implDirTag->getValue();
                
                DragonConfig::StringEntry* implHeaderDirTag = root->getString("implHeaderDir");
                std::string implHeaderDir = implHeaderDirTag == nullptr ? "" : implHeaderDirTag->getValue();

                DragonConfig::StringEntry* docfileTag = root->getString("docfile");
                Main::options::mapFrameworkDocfiles[framework] = docfileTag == nullptr ? "" : path + "/" + framework + ".framework/" + docfileTag->getValue();

                Version ver = Version(version);
                Version compilerVersion = Version(VERSION);
                if (ver > compilerVersion) {
                    std::cerr << "Error: Framework '" << framework << "' requires Scale v" << version << " but you are using " << VERSION << std::endl;
                    return false;
                }
                if (ver < FrameworkMinimumVersion) {
                    std::cerr << "Error: Framework '" << framework << "' is too outdated (" << ver.asString() << "). Please update it to at least version " << FrameworkMinimumVersion.asString() << std::endl;
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
                    bool isCppFile = strends(flag, ".cpp") || strends(flag, ".c++");
                    if (!hasCppFiles && isCppFile) {
                        hasCppFiles = true;
                    }
                    tmpFlags.push_back(flag);
                }

                Main::frameworks.push_back(framework);

                if (headerDir.size() > 0) {
                    Main::options::includePaths.push_back(path + "/" + framework + ".framework/" + headerDir);
                    Main::options::mapIncludePathsToFrameworks[framework] = path + "/" + framework + ".framework/" + headerDir;
                }
                unsigned long implementersSize = implementers == nullptr ? 0 : implementers->size();
                for (unsigned long i = 0; i < implementersSize; i++) {
                    std::string implementer = implementers->getString(i)->getValue();
                    if (!Main::options::assembleOnly) {
                        bool isCppFile = strends(implementer, ".cpp") || strends(implementer, ".c++");
                        if (!hasCppFiles && isCppFile) {
                            hasCppFiles = true;
                        }
                        nonScaleFiles.push_back(path + "/" + framework + ".framework/" + implDir + "/" + implementer);
                    }
                }
                for (unsigned long i = 0; implHeaders != nullptr && i < implHeaders->size() && implHeaderDir.size() > 0; i++) {
                    std::string header = framework + ".framework/" + implHeaderDir + "/" + implHeaders->getString(i)->getValue();
                    Main::frameworkNativeHeaders.push_back(header);
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
                            spacesBetween = t.location.column - tokens[i - 1].location.column - tokens[i - 1].value.size();
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

        for (size_t i = 0; i < lines.size(); i++) {
            std::string line = lines[i];
            if (strstarts(line, "%include")) {
                std::string includeFile = trimLeft(line.substr(8));
                std::string includePath = std::filesystem::path(file).parent_path().string() + "/" + includeFile;
                FILE* include = fopen(includePath.c_str(), "rb");
                if (!include) {
                    std::cerr << Color::RED << "Failed to open include file " << includePath << Color::RESET << std::endl;
                    exit(1);
                }
                fseek(include, 0, SEEK_END);
                long sz = ftell(include);
                fseek(include, 0, SEEK_SET);

                char* data = new char[sz];

                fread(data, 1, sz, include);
                fclose(include);

                std::string includeData = std::string(data);
                std::vector<std::string> includeLines = split(includeData, "\n");
                lines.erase(lines.begin() + i);
                for (size_t i = 0; i < includeLines.size(); i++) {
                    lines.insert(lines.begin() + i, includeLines[i]);
                }
            }
        }

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
                implementedAt = replaceAll(implementedAt, R"(\{framework\})", Main::options::printDocFor);
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
        std::vector<std::string> frameworks = {Main::options::printDocFor};
        bool hasCppFiles;
        Version FrameworkMinimumVersion(FRAMEWORK_VERSION_REQ);
        if (!checkFramework(Main::options::printDocFor, tmpFlags, frameworks, hasCppFiles, FrameworkMinimumVersion)) {
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

        for (size_t i = Main::options::docPrinterArgsStart + 1; i < args.size(); i++) {
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
                    Main::options::printDocFor = args[i + 1];
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
        std::string file = Main::options::mapFrameworkDocfiles[Main::options::printDocFor];
        std::string docFileFormat = Main::options::indexDrgFiles[Main::options::printDocFor]->getStringOrDefault("docfile-format", "markdown")->getValue();

        if (file.empty() || !fileExists(file)) {
            std::cerr << Color::RED << "Framework '" + Main::options::printDocFor + "' has no docfile!" << Color::RESET << std::endl;
            return 1;
        }

        std::string includeFolder = Main::options::mapFrameworkIncludeFolders[Main::options::printDocFor];
        Main::options::docsIncludeFolder = includeFolder;

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
            std::cout << "Documentation for " << Main::options::printDocFor << std::endl;
            std::cout << "Categories: " << std::endl;

            std::string current = "";
            for (auto section : docs) {
                current = section.first;
                std::cout << "  " << Color::BOLDBLUE << current << Color::RESET << std::endl;
            }
            return 0;
        }

        std::cout << "Documentation for " << Main::options::printDocFor << std::endl;

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

            if (foundIn.empty()) continue;
            std::cout << Color::BOLDBLUE << current << ":" << Color::RESET << std::endl;
            for (DocumentationEntry e : foundIn) {
                std::cout << Color::BLUE << e.name << "\n";
                if (e.module.size()) {
                    std::cout << Color::CYAN << "Module: " << e.module;
                    if (e.file.empty()) std::cout << "\n";
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

        framework->addString("version", "24.2.0");
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

    void logWarns(std::vector<FPResult>& warns);
    void logErrors(std::vector<FPResult>& errors);

    bool diagDisabled(std::string& diag) {
        for (auto&& r : disabledDiagnostics) {
            if (std::regex_search(diag, r)) {
                return true;
            }
        }
        return false;
    }

    void logWarns(std::vector<FPResult>& warns) {
        for (FPResult error : warns) {
            if (error.type >= tok_MAX) continue;
            if (diagDisabled(error.message)) {
                logWarns(error.warns);
                logErrors(error.errors);
                continue;
            }
            if (error.location.line == 0) {
                std::cout << Color::BOLDRED << "Fatal Error: " << error.location.file << ": " << error.message << Color::RESET << std::endl;
                continue;
            }
            FILE* f = fopen(std::string(error.location.file).c_str(), "r");
            if (!f) {
                std::cout << Color::BOLDRED << "Fatal Error: Could not open file " << error.location.file << ": " << std::strerror(errno) << Color::RESET << std::endl;
                continue;
            }
            char* line = (char*) GC_malloc(sizeof(char) * 500);
            int i = 1;
            if (f) fseek(f, 0, SEEK_SET);
            std::string colString;
            if (error.isNote) {
                colString = Color::BOLDGRAY;
            } else {
                colString = Color::BOLDMAGENTA;
            }
            std::string fileRelativeToCurrent = std::filesystem::relative(error.location.file, std::filesystem::current_path()).string();
            std::cerr <<
                Color::BOLD <<
                fileRelativeToCurrent <<
                ":" << error.location.line <<
                ":" << error.location.column <<
                ": " << colString <<
                (error.isNote ? "note" : "warning") <<
                ": " << Color::RESET <<
                (!error.isNote ? Color::BOLD : "") <<
                error.message <<
                Color::RESET <<
                std::endl;
            i = 1;
            while (fgets(line, 500, f) != NULL) {
                if (i == error.location.line) {
                    std::cerr << line << Color::RESET;
                    std::cerr << std::string(error.location.column - 1, ' ') << Color::BOLDGREEN << "^" << Color::RESET << std::endl;
                }
                i++;
            }
            fclose(f);
            GC_free(line);
            logWarns(error.warns);
            logErrors(error.errors);
        }
    }

    void logErrors(std::vector<FPResult>& errors) {
        size_t errorCount = 0;
        for (FPResult error : errors) {
            if (errorCount >= Main::options::errorLimit) {
                std::cout << Color::BOLDRED << "Too many errors (" << errors.size() << "), aborting" << Color::RESET << std::endl;
                break;
            }
            if (diagDisabled(error.message)) {
                logWarns(error.warns);
                logErrors(error.errors);
                continue;
            }
            if (error.type >= tok_MAX) continue;
            std::string colorStr;
            ssize_t addAtCol = -1;
            std::string strToAdd = "";
            if (error.isNote) {
                colorStr = Color::BOLDGRAY;
            } else {
                colorStr = Color::BOLDRED;
            }
            if (error.location.line == 0) {
                std::cout << Color::BOLDRED << "Fatal Error: " << error.location.file << ": " << error.message << Color::RESET << std::endl;
                continue;
            }
            FILE* f = fopen(std::string(error.location.file).c_str(), "r");
            if (!f) {
                std::cout << Color::BOLDRED << "Fatal Error: Could not open file " << error.location.file << ": " << std::strerror(errno) << Color::RESET << std::endl;
                continue;
            }
            char* line = (char*) malloc(sizeof(char) * 500);
            int i = 1;
            if (f) fseek(f, 0, SEEK_SET);
            std::string fileRelativeToCurrent = std::filesystem::relative(error.location.file, std::filesystem::current_path()).string();
            std::cerr <<
                Color::BOLD <<
                fileRelativeToCurrent <<
                ":" << error.location.line <<
                ":" << error.location.column <<
                ": " << colorStr <<
                (error.isNote ? "note" : "error") <<
                ": " << Color::RESET <<
                (!error.isNote ? Color::BOLD : "") <<
                error.message <<
                Color::RESET <<
                std::endl;
            i = 1;
            while (fgets(line, 500, f) != NULL) {
                if (i == error.location.line) {
                    if (strToAdd.size()) {
                        std::cerr << std::string(line).insert(addAtCol, strToAdd) << Color::RESET;
                    } else {
                        std::cerr << line << Color::RESET;
                    }
                    std::cerr << std::string(error.location.column - 1, ' ') << Color::BOLDGREEN << "^" << Color::RESET << std::endl;
                }
                i++;
            }
            fclose(f);
            free(line);
            errorCount++;
            logWarns(error.warns);
            logErrors(error.errors);
        }
    }

    template<typename T, typename Func>
    size_t count_if(std::vector<T> vec, Func func) {
        size_t count = 0;
        for (const T& t : vec) {
            if (func(t)) count++;
        }
        return count;
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
        std::string optimizer   = "O2";
        bool hasCppFiles        = false;
        bool hasFilesFromArgs   = false;
        bool outFileSpecified   = false;

        Main::options::includePaths.push_back(scaleFolder + "/Frameworks");
        Main::options::includePaths.push_back(".");

        if (args[0] == "scaledoc") {
            Main::options::docPrinterArgsStart = 0;
            Main::options::printDocFor = "Scale";
            return docHandler(args);
        }

        if (args.size() < 2) {
            usage(args[0]);
            return 1;
        }

        srand(time(NULL));
        tmpFlags.reserve(args.size());

        Main::version = new Version(std::string(VERSION));
        Main::options::errorLimit = 20;

        DragonConfig::CompoundEntry* scaleConfig = DragonConfig::ConfigParser().parse("scale.drg");
        if (scaleConfig) {
            if (scaleConfig->hasMember("outfile"))
                Main::options::outfile = outfile = scaleConfig->getString("outfile")->getValue();

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
                    Main::frameworkNativeHeaders.push_back(scaleConfig->getList("includeFiles")->getString(i)->getValue());
            
            if (scaleConfig->hasMember("featureFlags"))
                for (size_t i = 0; i < scaleConfig->getList("featureFlags")->size(); i++)
                    Main::options::features.push_back(scaleConfig->getList("featureFlags")->getString(i)->getValue());
            
            Main::options::indexDrgFiles["/local"] = scaleConfig;
        }

        Main::options::stackSize = 128;

        for (size_t i = 1; i < args.size(); i++) {
            if (!hasCppFiles && (strends(args[i], ".cpp") || strends(args[i], ".c++"))) {
                hasCppFiles = true;
            }
            if (strends(args[i], ".c") || strends(args[i], ".cpp") || strends(args[i], ".c++")) {
                nonScaleFiles.push_back(args[i]);
            } else if (strends(std::string(args[i]), ".scale")) {
                if (!fileExists(args[i])) {
                    continue;
                }
                std::string file = std::filesystem::absolute(args[i]).string();
                if (!contains(Main::options::files, file)) {
                    Main::options::files.push_back(file);
                    Main::options::filesFromCommandLine.push_back(file);
                    hasFilesFromArgs = true;
                }
            } else if (args[i] == "--transpile" || args[i] == "-t") {
                Main::options::transpileOnly = true;
            } else if (args[i] == "--help" || args[i] == "-h") {
                usage(args[0]);
                return 0;
            } else if (args[i] == "-E") {
                Main::options::preprocessOnly = true;
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
                    Main::options::outfile = outfile = args[i + 1];
                    i++;
                } else {
                    std::cerr << "Error: -o requires an argument" << std::endl;
                    return 1;
                }
            } else if (args[i] == "-S") {
                Main::options::assembleOnly = true;
                Main::options::dontSpecifyOutFile = true;
                tmpFlags.push_back("-S");
            } else if (args[i] == "--no-main" || args[i] == "-no-main") {
                Main::options::noMain = true;
            } else if (args[i] == "-v" || args[i] == "--version") {
                std::cout << "Scale Compiler version " << std::string(VERSION) << std::endl;
                system((compiler + " -v").c_str());
                return 0;
            } else if (args[i] == "-feat") {
                if (i + 1 < args.size()) {
                    Main::options::features.push_back(args[i + 1]);
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
            } else if (args[i] == "-cflags") {
                Main::options::printCflags = true;
                Main::options::noMain = true;
            } else if (args[i] == "-makelib") {
                #if defined(__APPLE__)
                tmpFlags.push_back("-dynamiclib");
                tmpFlags.push_back("-undefined dynamic_lookup");
                #else
                #if defined(__linux__)
                tmpFlags.push_back("-Wl,--undefined,dynamic_lookup");
                #endif
                tmpFlags.push_back("-shared");
                tmpFlags.push_back("-fPIC");
                #endif
                Main::options::noMain = true;
                tmpFlags.push_back("-DSCL_COMPILER_NO_MAIN");
                if (!outFileSpecified)
                #if defined(__APPLE__)
                    Main::options::outfile = outfile = "libout.dylib";
                #else
                    Main::options::outfile = outfile = "libout.so";
                #endif
            } else if (args[i] == "-no-scale-std") {
                Main::options::noScaleFramework = true;
            } else if (args[i] == "-no-link-std") {
                Main::options::noLinkScale = true;
            } else if (args[i] == "-create-framework") {
                if (i + 1 < args.size()) {
                    std::string name = args[i + 1];
                    i++;
                    return makeFramework(name);
                } else {
                    std::cerr << "Error: " << args[i] << " requires an argument" << std::endl;
                    return 1;
                }
            } else if (args[i] == "-nowarn") {
                if (i + 1 < args.size()) {
                    std::string regex = args[i + 1];
                    i++;
                    disabledDiagnostics.push_back(std::regex(regex));
                } else {
                    std::cerr << "Error: " << args[i] << " requires an argument" << std::endl;
                    return 1;
                }
            } else if (args[i] == "-stack-size") {
                if (i + 1 < args.size()) {
                    Main::options::stackSize = std::stoull(args[i + 1]);
                    i++;
                } else {
                    std::cerr << "Error: -stack-size requires an argument" << std::endl;
                    return 1;
                }
            } else if (args[i] == "-doc-for") {
                if (i + 1 < args.size()) {
                    Main::options::printDocFor = args[i + 1];
                    i++;
                    Main::options::docPrinterArgsStart = i;
                } else {
                    std::cerr << "Error: -doc-for requires an argument" << std::endl;
                    return 1;
                }
            } else if (args[i] == "--") {
                break;
            } else {
                if (args[i].size() >= 2 && args[i][0] == '-' && args[i][1] == 'I') {
                    Main::options::includePaths.push_back(std::string(args[i].c_str() + 2));
                } else if (args[i] == "-I") {
                    if (i + 1 < args.size()) {
                        Main::options::includePaths.push_back(args[i + 1]);
                        i++;
                    } else {
                        std::cerr << "Error: -I requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i].size() >= 2 && args[i][0] == '-' && args[i][1] == 'O') {
                    optimizer = std::string(args[i].c_str() + 1);
                } else if (args[i] == "-c") {
                    Main::options::dontSpecifyOutFile = true;
                } else if (args[i] == "-Werror") {
                    Main::options::Werror = true;
                } else if (strstarts(args[i], "-ferror-limit=")) {
                    if (args[i].size() == 15) {
                        std::cerr << "Error: -ferror-limit requires an argument" << std::endl;
                        return 1;
                    }
                    Main::options::errorLimit = std::atol(args[i].substr(15).c_str());
                }
                tmpFlags.push_back(args[i]);
            }
        }
        
        if (!hasFilesFromArgs) {
            Main::options::noMain = true;
            if (!outFileSpecified)
                Main::options::outfile = outfile = "a.out";
        }

        cflags.reserve(tmpFlags.size() + 10);
        Main::options::optimizer = optimizer;

        if (!Main::options::printCflags)
            cflags.push_back(compiler);

        cflags.push_back("-I" + scaleFolder + "/Internal");
        cflags.push_back("-I" + scaleFolder + "/Internal/include");
        cflags.push_back("-I" + scaleFolder + "/Frameworks");
        cflags.push_back("-I.");
        cflags.push_back("-L" + scaleFolder);
        cflags.push_back("-L" + scaleFolder + "/Internal");
        cflags.push_back("-L" + scaleFolder + "/Internal/lib");
        cflags.push_back("-" + optimizer);
        cflags.push_back("-DVERSION=\"" + std::string(VERSION) + "\"");
        cflags.push_back("-std=" + std::string(C_VERSION));
        cflags.push_back("-ftrapv");
        cflags.push_back("-fno-inline");
#if !defined(_WIN32)
        cflags.push_back("-fPIC");
#endif
        
        if (Main::options::noScaleFramework) goto skipScaleFramework;
        for (auto f : frameworks) {
            if (f == "Scale") {
                goto skipScaleFramework;
            }
        }
        frameworks.push_back("Scale");
    skipScaleFramework:

        Version FrameworkMinimumVersion = Version(std::string(FRAMEWORK_VERSION_REQ));

        for (std::string& framework : frameworks) {
            checkFramework(framework, tmpFlags, frameworks, hasCppFiles, FrameworkMinimumVersion);
        }

        if (!Main::options::noScaleFramework) {
            auto findModule = [&](const std::string moduleName) -> std::vector<std::string> {
                std::vector<std::string> mods;
                for (auto config : Main::options::indexDrgFiles) {
                    auto modules = config.second->getCompound("modules");
                    if (modules) {
                        auto list = modules->getList(moduleName);
                        if (list) {
                            for (size_t j = 0; j < list->size(); j++) {
                                FPResult find = findFileInIncludePath(list->getString(j)->getValue());
                                if (!find.success) {
                                    return std::vector<std::string>();
                                }
                                auto file = find.location.file;
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
                if (!contains(Main::options::files, file)) {
                    Main::options::files.push_back(file);
                }
            }
        }

        if (Main::options::printDocs ||  Main::options::printDocFor.size() != 0) {
            if (Main::options::printDocs || (Main::options::printDocFor.size() && contains(frameworks, Main::options::printDocFor))) {
                return docHandler(args);
            }
            std::cerr << Color::RED << "Framework '" + Main::options::printDocFor + "' not found!" << Color::RESET << std::endl;
            return 1;
        }
        
        std::vector<Token>  tokens;

        for (size_t i = 0; i < Main::options::files.size() && !Main::options::printCflags; i++) {
            using path = std::filesystem::path;
            path s = Main::options::files[i];
            if (s.parent_path().string().size())
                Main::options::includePaths.push_back(s.parent_path().string());
        }

        size_t numWarns = 0;
        for (size_t i = 0; i < Main::options::files.size() && !Main::options::printCflags; i++) {
            std::string filename = Main::options::files[i];

            if (!std::filesystem::exists(filename)) {
                std::cout << Color::BOLDRED << "Fatal Error: File " << filename << " does not exist!" << Color::RESET << std::endl;
                return 1;
            }

            if (!strends(filename, ".scale")) {
                continue;
            }

            if (Main::tokenizer) {
                delete Main::tokenizer;
            }
            Main::tokenizer = new Tokenizer();
            FPResult result = Main::tokenizer->tokenize(filename);

            logWarns(result.warns);
            logErrors(result.errors);
            size_t numErrs = sclc::count_if(result.errors, [](const FPResult& err) -> bool {
                return !err.isNote;
            });
            numWarns += sclc::count_if(result.warns, [](const FPResult& err) -> bool {
                return !err.isNote;
            });

            if (numErrs) {
                if (numWarns) {
                    std::cout << numWarns << " warning" << (numWarns == 1 ? "" : "s") << " and ";
                }
                std::cout << numErrs << " error" << (numErrs == 1 ? "" : "s") << " generated." << std::endl;
                return numErrs;
            }

            Main::tokenizer->removeInvalidTokens();

            FPResult importResult = Main::tokenizer->tryImports();
            if (!importResult.success) {
                std::cerr << Color::BOLDRED << "Include Error: " << importResult.location.file << ":" << importResult.location.line << ":" << importResult.location.column << ": " << importResult.message << std::endl;
                return 1;
            }

            std::vector<Token> theseTokens = Main::tokenizer->getTokens();
            
            tokens.insert(tokens.end(), theseTokens.begin(), theseTokens.end());
        }

        if (Main::options::preprocessOnly) {
            std::cout << "Preprocessed " << Main::options::files.size() << " files." << std::endl;
            return 0;
        }

        TPResult result;
        if (!Main::options::printCflags) {
            Main::lexer = new SyntaxTree(tokens);
            result = Main::lexer->parse();
        }

        logWarns(result.warns);
        logErrors(result.errors);
        size_t numErrs = sclc::count_if(result.errors, [](const FPResult& err) {
            return !err.isNote;
        });
        numWarns += sclc::count_if(result.warns, [](const FPResult& err) {
            return !err.isNote;
        });

        if (numErrs) {
            if (numWarns) {
                std::cout << numWarns << " warning" << (numWarns == 1 ? "" : "s") << " and ";
            }
            std::cout << numErrs << " error" << (numErrs == 1 ? "" : "s") << " generated." << std::endl;
            return numErrs;
        }

        Main::parser = new Parser(result);

        if (hasCppFiles) {
            // link to c++ library if needed
            cflags.push_back("-lc++");
        }

        for (std::string& s : tmpFlags) {
            if (!Main::options::noLinkScale && strstarts(s, scaleFolder + "/Frameworks/Scale.framework")) {
                continue;
            }
            cflags.push_back(s);
        }

        std::string code_file = "scl_code.c";
        std::string types_file = "scl_types.c";
        std::string headers_file = "scl_headers.h";
        
        FPResult parseResult = Main::parser->parse(
            code_file,
            types_file,
            headers_file
        );
        cflags.push_back(code_file);
        cflags.push_back(types_file);
        
        logWarns(parseResult.warns);
        logErrors(parseResult.errors);
        numErrs = sclc::count_if(parseResult.errors, [](const FPResult& err) {
            return !err.isNote;
        });
        numWarns += sclc::count_if(parseResult.warns, [](const FPResult& err) {
            return !err.isNote;
        });

        if (numWarns) {
            std::cout << numWarns << " warning" << (numWarns == 1 ? "" : "s");
            if (numErrs) {
                std::cout << " and ";
            }
        }
        if (numErrs) {
            std::cout << numErrs << " error" << (numErrs == 1 ? "" : "s");
        }
        if (numWarns || numErrs) {
            std::cout << " generated." << std::endl;
        }
        if (numErrs) return numErrs;

        if (Main::options::transpileOnly) {
            auto end = clock::now();
            double duration = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000000000.0;
            std::cout << "Transpiled successfully in " << duration << " seconds." << std::endl;
            return 0;
        }

        for (std::string& file : nonScaleFiles) {
            if (!Main::options::noLinkScale && strstarts(file, scaleFolder + "/Frameworks/Scale.framework")) {
                continue;
            }
            cflags.push_back("\"" + file + "\"");
        }

        if (!Main::options::printCflags && !Main::options::dontSpecifyOutFile) {
            cflags.push_back("-o");
            cflags.push_back("\"" + outfile + "\"");
        }
        
#ifdef LINK_MATH
        cflags.push_back("-lm");
#endif
#ifndef __APPLE__
        cflags.push_back("-Wl,-R");
        cflags.push_back("-Wl," + scaleFolder + "/Internal");
        cflags.push_back("-Wl,-R");
        cflags.push_back("-Wl," + scaleFolder + "/Internal/lib");
#endif
        cflags.push_back("-lScaleRuntime");
        cflags.push_back("-lgc");
        if (!Main::options::noLinkScale) {
            cflags.push_back("-lScale");
        }

        std::string cmd = "";
        for (std::string& s : cflags) {
            cmd += s + " ";
        }

        if (Main::options::printCflags) {
            std::cout << cmd << std::endl;
            return 0;
        } else if (!Main::options::transpileOnly) {
            int compile_command = system(cmd.c_str());

            if (compile_command) {
                std::cout << Color::RED << "Compilation failed with error code " << compile_command << Color::RESET << std::endl;
                return compile_command;
            }
        }

        std::remove(headers_file.c_str());
        std::remove(code_file.c_str());
        std::remove(types_file.c_str());
        std::remove("scale_interop.h");

        return 0;
    }
}

int main(int argc, char const *argv[]) {
    GC_INIT();

    signal(SIGSEGV, sclc::signalHandler);
    signal(SIGABRT, sclc::signalHandler);

#ifndef _WIN32
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
        sclc::Color::BOLD = "";
        sclc::Color::UNDERLINE = "";
        sclc::Color::BOLDGRAY = "";
        sclc::Color::GRAY = "";
    }
#endif

    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        args.push_back(argv[i]);
    }
    std::ios_base::sync_with_stdio(false);
    std::cout.tie(nullptr);
    std::cin.tie(nullptr);
    return sclc::main(args);
}
