
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
#include <fstream>

#include <signal.h>
#include <errno.h>

#ifndef _WIN32
#include <unistd.h>
#ifndef LINK_MATH
#define LINK_MATH
#endif
#else
#define strdup _strdup
#include <process.h>
#endif

#include <Common.hpp>
#include <DragonConfig.hpp>

#ifndef VERSION
#define VERSION "unknown. Did you forget to build with -DVERSION=<version>?"
#endif

#ifndef COMPILER
#define COMPILER "clang"
#endif

#ifndef C_VERSION
#define C_VERSION "gnu99"
#endif

#ifndef SCALE_INSTALL_DIR
#define SCALE_INSTALL_DIR "Scale"
#endif

#ifndef DEFAULT_OUTFILE
#ifdef _WIN32
#define DEFAULT_OUTFILE "out.exe"
#else
#define DEFAULT_OUTFILE "out.scl"
#endif
#endif

#ifndef FRAMEWORK_VERSION_REQ
#define FRAMEWORK_VERSION_REQ "24.2.0"
#endif

#ifndef SCALE_ROOT_DIR
#define SCALE_ROOT_DIR DIR_SEP "opt"
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

    void usage(std::string programName) {
        std::cout << "Usage: " << programName << " <filename> [args]" << std::endl;
        std::cout << std::endl;
        std::cout << "Accepted Flags:" << std::endl;
        std::cout << "  -t, --transpile             Transpile only" << std::endl;
        std::cout << "  -h, --help                  Show this help" << std::endl;
        std::cout << "  -v, --version               Show version information" << std::endl;
        std::cout << "  -o <filename>               Specify Output file" << std::endl;
        std::cout << "  -f <framework>              Use Scale Framework" << std::endl;
        std::cout << "  -I <path>                   Add path to include path" << std::endl;
        std::cout << "  -no-main                    Do not check for main Function" << std::endl;
        std::cout << "  -compiler <comp>            Use comp as the compiler instead of " << std::string(COMPILER) << std::endl;
        std::cout << "  -feat <feature>             Enables the specified language feature" << std::endl;
        std::cout << "  -makelib                    Compile as a library (implies -no-main)" << std::endl;
        std::cout << "  -cflags                     Print c compiler flags and exit" << std::endl;
        std::cout << "  -no-error-location          Do not print an overview of the file on error" << std::endl;
        std::cout << "  -verbose-linker             Tells the linker to run in verbose mode for debugging purposes" << std::endl;
        std::cout << "  -stack-size <sz>            Sets the starting stack size. Must be a multiple of 2" << std::endl;
        std::cout << "  -no-scale-std               Do not depend on Scale.framework" << std::endl;
        std::cout << "  -no-link-scale              Do not dynamically link to Scale.framework library" << std::endl;
        std::cout << "  -embedded                   Compile in embedded mode, no dynamic libraries, implies -no-scale-std and -no-link-scale" << std::endl;
        std::cout << "  -verbose-linker             Tells the linker to run in verbose mode for debugging purposes" << std::endl;
        std::cout << "  -create-framework <name>    Create a new framework with the given name" << std::endl;
        std::cout << "  --                          Stop parsing flags" << std::endl;
        std::cout << std::endl;
        std::cout << "  Any other options are passed directly to " << std::string(COMPILER) << " (or compiler specified by -compiler)" << std::endl;
    }

    FPResult findFileInIncludePath(std::string file);

    auto listFiles(const std::filesystem::path& dir, std::string ext) -> std::vector<std::filesystem::path> {
        std::vector<std::filesystem::path> files;
        for (auto child : std::filesystem::directory_iterator(dir)) {
            if (child.path().extension() == ext) {
                files.push_back(child.path());
            } else if (child.is_directory()) {
                auto x = listFiles(child.path(), ext);
                files.insert(files.end(), x.begin(), x.end());
            }
            
        }
        return files;
    }

    bool checkFramework(std::string& framework, std::vector<std::string>& tmpFlags, std::vector<std::string>& frameworks, bool& hasCppFiles, const Version& FrameworkMinimumVersion) {
        for (auto path : Main::options::includePaths) {
            if (std::filesystem::exists(path + DIR_SEP + framework + ".framework" DIR_SEP "index.drg")) {
                DragonConfig::ConfigParser parser;
                Ptr<DragonConfig::CompoundEntry> root = parser.parse(path + DIR_SEP + framework + ".framework" DIR_SEP "index.drg");
                if (root == nullptr) {
                    std::cerr << Color::RED << "Failed to parse index.drg of Framework " << framework << std::endl;
                    return false;
                }
                root = root->getCompound("framework");
                Main::options::indexDrgFiles[framework] = root;
                Ptr<DragonConfig::ListEntry> implementers = root->getList("implementers");
                Ptr<DragonConfig::ListEntry> implHeaders = root->getList("implHeaders");
                Ptr<DragonConfig::ListEntry> depends = root->getList("depends");
                Ptr<DragonConfig::ListEntry> compilerFlags = root->getList("compilerFlags");
                Ptr<DragonConfig::CompoundEntry> build = root->getCompound("build");
                Ptr<DragonConfig::CompoundEntry> config = root->getCompound("config");

                Ptr<DragonConfig::StringEntry> versionTag = root->getString("version");
                std::string version = versionTag->getValue();
                if (versionTag == nullptr) {
                    std::cerr << "Framework " << framework << " does not specify a version! Skipping." << std::endl;
                    return false;
                }
                
                Ptr<DragonConfig::StringEntry> headerDirTag = root->getString("headerDir");
                std::string headerDir = headerDirTag == nullptr ? "" : headerDirTag->getValue();
                Main::options::mapFrameworkIncludeFolders[framework] = headerDir;
                
                Ptr<DragonConfig::StringEntry> implDirTag = root->getString("implDir");
                std::string implDir = implDirTag == nullptr ? "" : implDirTag->getValue();
                
                Ptr<DragonConfig::StringEntry> implHeaderDirTag = root->getString("implHeaderDir");
                std::string implHeaderDir = implHeaderDirTag == nullptr ? "" : implHeaderDirTag->getValue();

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

                auto compileFramework = [](const std::string& framework, const std::string& path, const std::string& includePath, Ptr<DragonConfig::CompoundEntry> build) {
                    if (build == nullptr) {
                        return;
                    }

                    auto curPath = std::filesystem::current_path();
                    std::filesystem::current_path(path + DIR_SEP + framework + ".framework");

                    Ptr<DragonConfig::ListEntry> excludedFiles = build->getList("exclude");
                    Ptr<DragonConfig::ListEntry> includedFiles = build->getList("files");
                    Ptr<DragonConfig::ListEntry> arguments = build->getList("arguments");
                    auto libraryName = std::filesystem::absolute(build->getStringOrDefault("outfile", path + DIR_SEP + framework + ".framework" DIR_SEP + LIB_PREF + framework + LIB_SUFF)->getValue());

                    std::string cmd =
                        "sclc -makelib -o " +
                        libraryName.string() +
                        " -I " +
                        std::filesystem::absolute(path + DIR_SEP + framework + ".framework").string() +
                        " -I " +
                        std::filesystem::absolute(path + DIR_SEP + framework + ".framework" DIR_SEP + includePath).string();

                    for (size_t i = 0; arguments != nullptr && i < arguments->size(); i++) {
                        cmd += " " + arguments->getString(i)->getValue();
                    }

                    auto files = listFiles(std::filesystem::current_path() / includePath, ".scale");
                    for (auto file : files) {
                        for (size_t i = 0; excludedFiles != nullptr && i < excludedFiles->size(); i++) {
                            std::string x = excludedFiles->getString(i)->getValue();
                            if (file.string().find(x) != std::string::npos) {
                                goto after;
                            }
                        }
                        cmd += " " + std::filesystem::absolute(file).string();
                        after: (void)0;
                    }
                    for (size_t i = 0; includedFiles != nullptr && i < includedFiles->size(); i++) {
                        cmd += " " + std::filesystem::absolute(includedFiles->getString(i)->getValue()).string();
                    }

                    std::cout << Color::GREEN << "Compiling framework '" << framework << "' (" << libraryName << ")" << Color::RESET << std::endl;
                    Main::frameworkPaths.push_back(path + DIR_SEP + framework + ".framework");

                    setenv("SCALE_COMPILE_FRAMEWORK", framework.c_str(), 1);
                    int compile_command = system(cmd.c_str());
                    setenv("SCALE_COMPILE_FRAMEWORK", "", 1);

                    if (compile_command) {
                        std::cout << Color::RED << "Compilation of framework '" + framework + "' failed with error code " << compile_command << Color::RESET << std::endl;
                        std::exit(compile_command);
                    }
                    
                    std::filesystem::current_path(curPath);
                };

                auto s = getenv("SCALE_COMPILE_FRAMEWORK");
                auto libFile = path + DIR_SEP + framework + ".framework" DIR_SEP + build->getStringOrDefault("outfile", LIB_PREF + framework + LIB_SUFF)->getValue();
                if (build != nullptr && !std::filesystem::exists(std::filesystem::absolute(libFile))) {
                    if (s == nullptr || s != framework) {
                        compileFramework(framework, path, headerDir, build);
                    }
                }
                if (std::filesystem::exists(path + DIR_SEP + framework + ".framework" DIR_SEP + LIB_PREF + framework + LIB_SUFF)) {
                    tmpFlags.push_back("-L" + path + DIR_SEP + framework + ".framework");
                    #if defined(__APPLE__)
                        tmpFlags.push_back("-Wl,-rpath," + path + DIR_SEP + framework + ".framework");
                    #elif !defined(_WIN32)
                        tmpFlags.push_back("-Wl,-R");
                        tmpFlags.push_back("-Wl," + path + DIR_SEP + framework + ".framework");
                    #endif
                    tmpFlags.push_back("-l" + framework);
                    Main::frameworkPaths.push_back(path + DIR_SEP + framework + ".framework");
                }

                if (headerDir.size() > 0) {
                    Main::options::includePaths.push_back(path + DIR_SEP + framework + ".framework" DIR_SEP + headerDir);
                    Main::options::mapIncludePathsToFrameworks[framework] = path + DIR_SEP + framework + ".framework" DIR_SEP + headerDir;
                }
                unsigned long implementersSize = implementers == nullptr ? 0 : implementers->size();
                for (unsigned long i = 0; i < implementersSize; i++) {
                    std::string implementer = implementers->getString(i)->getValue();
                    if (!Main::options::assembleOnly) {
                        bool isCppFile = strends(implementer, ".cpp") || strends(implementer, ".c++");
                        if (!hasCppFiles && isCppFile) {
                            hasCppFiles = true;
                        }
                        nonScaleFiles.push_back(path + DIR_SEP + framework + ".framework" DIR_SEP + implDir + DIR_SEP + implementer);
                    }
                }
                for (unsigned long i = 0; implHeaders != nullptr && i < implHeaders->size() && implHeaderDir.size() > 0; i++) {
                    std::string header = framework + ".framework" DIR_SEP + implHeaderDir + DIR_SEP + implHeaders->getString(i)->getValue();
                    Main::frameworkNativeHeaders.push_back(header);
                }

                if (config) {
                    for (auto x : config->entries) {
                        if (Main::config.hasMember(x->getKey())) {
                            std::cerr << "Config entry with key '" << x->getKey() << "' already exists!" << std::endl;
                        } else {
                            Main::config.entries.push_back(x);
                        }
                    }
                }

                return true;
            }
        }
        return false;
    }

    void logWarns(std::vector<FPResult>& warns);
    void logErrors(std::vector<FPResult>& errors);

    int makeFramework(std::string name) {
        std::filesystem::create_directories(name + ".framework");
        std::filesystem::create_directories(name + ".framework" DIR_SEP "include");
        std::filesystem::create_directories(name + ".framework" DIR_SEP "impl");
        std::fstream indexFile((name + ".framework" DIR_SEP "index.drg").c_str(), std::fstream::out);

        DragonConfig::CompoundEntry framework = DragonConfig::CompoundEntry();
        framework.setKey("framework");

        framework.addString("version", "24.2.0");
        framework.addString("headerDir", "include");
        framework.addString("implDir", "impl");
        framework.addString("implHeaderDir", "impl");
        
        auto implementers = DragonConfig::ListEntry();
        implementers.setKey("implementers");
        framework.addList(&implementers);

        auto implHeaders = DragonConfig::ListEntry();
        implHeaders.setKey("implHeaders");
        framework.addList(&implHeaders);

        auto modules = DragonConfig::CompoundEntry();
        modules.setKey("modules");
        framework.addCompound(&modules);

        framework.print(indexFile);

        indexFile.close();
        return 0;
    }

    std::string limitPathTo(const std::filesystem::path& path, size_t limit) {
        std::string out = path.string();
        if (out.size() > limit) {
            out = out.substr(out.size() - limit);
            out = "..." + out;
        }
        return out;
    };

    void logWarns(std::vector<FPResult>& warns) {
        for (FPResult error : warns) {
            if (error.type >= tok_MAX) continue;
            if (error.location.line == 0) {
                std::cout << Color::BOLDRED << "Fatal Error: " << error.location.file << ": " << error.message << Color::RESET << std::endl;
                continue;
            }
            FILE* f = fopen(error.location.file.c_str(), "r");
            if (!f) {
                std::cout << Color::BOLDRED << "Fatal Error: Could not open file " << error.location.file << ": " << strerror(errno) << Color::RESET << std::endl;
                continue;
            }
            char line[1024] = {0};
            int i = 1;
            if (f) fseek(f, 0, SEEK_SET);
            std::string colString;
            if (error.isNote) {
                colString = Color::BOLDGRAY;
            } else {
                colString = Color::BOLDMAGENTA;
            }

            std::string path = limitPathTo(std::filesystem::absolute(error.location.file), 48);
            std::cerr <<
                Color::BOLD <<
                path <<
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
            while (fgets(line, 511, f) != NULL) {
                if (i == error.location.line) {
                    std::cerr << line << Color::RESET;
                    std::cerr << std::string(error.location.column - 1, ' ') << Color::BOLDGREEN << "^" << Color::RESET << std::endl;
                }
                i++;
            }
            fclose(f);
            logErrors(error.errors);
            logWarns(error.warns);
        }
    }

    void logErrors(std::vector<FPResult>& errors) {
        size_t errorCount = 0;
        for (FPResult error : errors) {
            if (errorCount >= Main::options::errorLimit) {
                std::cout << Color::BOLDRED << "Too many errors (" << errors.size() << "), aborting" << Color::RESET << std::endl;
                break;
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
            FILE* f = fopen(error.location.file.c_str(), "r");
            if (!f) {
                std::cout << Color::BOLDRED << "Fatal Error: Could not open file " << error.location.file << ": " << strerror(errno) << Color::RESET << std::endl;
                continue;
            }
            char line[1024] = {0};
            int i = 1;
            if (f) fseek(f, 0, SEEK_SET);
            std::string path = limitPathTo(std::filesystem::absolute(error.location.file), 48);
            std::cerr <<
                Color::BOLD <<
                path <<
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
            while (fgets(line, 511, f) != NULL) {
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
            errorCount++;
            logErrors(error.errors);
            logWarns(error.warns);
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
        scaleFolder             = std::string(SCALE_ROOT_DIR) + DIR_SEP + std::string(SCALE_INSTALL_DIR) + DIR_SEP + std::string(VERSION);
        scaleLatestFolder       = std::string(SCALE_ROOT_DIR) + DIR_SEP + std::string(SCALE_INSTALL_DIR) + DIR_SEP "latest";

        std::vector<std::string> frameworks;
        std::vector<std::string> tmpFlags;
        std::string optimizer   = "O2";
        bool hasCppFiles        = false;
        bool hasFilesFromArgs   = false;
        bool outFileSpecified   = false;

        Main::options::includePaths.push_back(scaleFolder + DIR_SEP "Frameworks");
        Main::options::includePaths.push_back(".");

        if (args.size() < 2) {
            usage(args[0]);
            return 1;
        }

        srand(time(NULL));
        tmpFlags.reserve(args.size());

        #ifdef _WIN32
        tmpFlags.push_back("-D_CRT_SECURE_NO_WARNINGS");
        #endif
        tmpFlags.push_back("-fvisibility=default");

        Main::version = Version(std::string(VERSION));
        Main::options::errorLimit = 20;

        DBG("Looking for project config");

        Main::config = DragonConfig::CompoundEntry();

        if (std::filesystem::exists("scale.drg")) {
            Ptr<DragonConfig::CompoundEntry> scaleConfig = DragonConfig::ConfigParser().parse("scale.drg");
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

                if (scaleConfig->hasMember("arguments"))
                    for (size_t i = 0; i < scaleConfig->getList("arguments")->size(); i++)
                        args.push_back(scaleConfig->getList("arguments")->getString(i)->getValue());

                if (scaleConfig->hasMember("compilerFlags"))
                    for (size_t i = 0; i < scaleConfig->getList("compilerFlags")->size(); i++)
                        tmpFlags.push_back(scaleConfig->getList("compilerFlags")->getString(i)->getValue());

                if (scaleConfig->hasMember("includeFiles"))
                    for (size_t i = 0; i < scaleConfig->getList("includeFiles")->size(); i++)
                        Main::frameworkNativeHeaders.push_back(scaleConfig->getList("includeFiles")->getString(i)->getValue());
                
                if (scaleConfig->hasMember("featureFlags"))
                    for (size_t i = 0; i < scaleConfig->getList("featureFlags")->size(); i++)
                        Main::options::features.push_back(scaleConfig->getList("featureFlags")->getString(i)->getValue());
                
                if (scaleConfig->hasMember("includePaths"))
                    for (size_t i = 0; i < scaleConfig->getList("includePaths")->size(); i++)
                        Main::options::includePaths.push_back(scaleConfig->getList("includePaths")->getString(i)->getValue());
                
                if (scaleConfig->hasMember("files"))
                    for (size_t i = 0; i < scaleConfig->getList("files")->size(); i++) {
                        if (!std::filesystem::exists(args[i])) {
                            continue;
                        }
                        std::string file = std::filesystem::absolute(args[i]).string();
                        if (!contains(Main::options::files, file)) {
                            Main::options::files.push_back(file);
                        }
                    }

                Ptr<DragonConfig::CompoundEntry> config = scaleConfig->getCompound("config");
                if (config) {
                    for (auto x : config->entries) {
                        if (Main::config.hasMember(x->getKey())) {
                            std::cerr << "Config entry with key '" << x->getKey() << "' already exists!" << std::endl;
                        } else {
                            Main::config.entries.push_back(x);
                        }
                    }
                }
                
                Main::options::indexDrgFiles["/local"] = scaleConfig;
            }
        }

        Main::options::stackSize = 128;

        DBG("Parsing cl args");

        for (size_t i = 1; i < args.size(); i++) {
            if (!hasCppFiles && (strends(args[i], ".cpp") || strends(args[i], ".c++"))) {
                hasCppFiles = true;
            }
            if (strends(args[i], ".c") || strends(args[i], ".cpp") || strends(args[i], ".c++")) {
                nonScaleFiles.push_back(args[i]);
            } else if (strends(std::string(args[i]), ".scale")) {
                if (!std::filesystem::exists(args[i])) {
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
                #ifndef _WIN32
                tmpFlags.push_back("-fPIC");
                #endif
                #endif
                Main::options::noMain = true;
                tmpFlags.push_back("-DSCALE_COMPILER_NO_MAIN");
                if (!outFileSpecified)
                #if defined(__APPLE__)
                    Main::options::outfile = outfile = "libout.dylib";
                #elif defined(_WIN32)
                    Main::options::outfile = outfile = "out.dll";
                #else
                    Main::options::outfile = outfile = "libout.so";
                #endif
            } else if (args[i] == "-no-scale-std") {
                Main::options::noScaleFramework = true;
            } else if (args[i] == "-no-link-std") {
                Main::options::noLinkScale = true;
            } else if (args[i] == "-embedded") {
                Main::options::noScaleFramework = true;
                Main::options::noLinkScale = true;
                Main::options::embedded = true;
                tmpFlags.push_back("-DSCALE_EMBEDDED");
            } else if (args[i] == "-verbose-linker") {
                tmpFlags.push_back("-v");
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
                    Main::options::stackSize = std::stoull(args[i + 1]);
                    i++;
                } else {
                    std::cerr << "Error: -stack-size requires an argument" << std::endl;
                    return 1;
                }
            } else if (args[i] == "--") {
                break;
            } else if (args[i] == "-I") {
                if (i + 1 < args.size()) {
                    Main::options::includePaths.push_back(args[i + 1]);
                    i++;
                } else {
                    std::cerr << "Error: -I requires an argument" << std::endl;
                    return 1;
                }
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
            } else {
                if (args[i].size() > 2 && args[i][0] == '-' && args[i][1] == 'I') {
                    Main::options::includePaths.push_back(std::string(args[i].c_str() + 2));
                } else if (args[i].size() > 2 && args[i][0] == '-' && args[i][1] == 'O') {
                    optimizer = std::string(args[i].c_str() + 1);
                }
                tmpFlags.push_back(args[i]);
            }
        }
        
        if (!hasFilesFromArgs) {
            Main::options::noMain = true;
            if (!outFileSpecified)
            #ifdef _WIN32
                Main::options::outfile = outfile = "a.exe";
            #else
                Main::options::outfile = outfile = "a.out";
            #endif
        }

        cflags.reserve(tmpFlags.size() + 10);
        Main::options::optimizer = optimizer;

        if (!Main::options::printCflags)
            cflags.push_back(compiler);

        DBG("Preparing compile command");

        cflags.push_back("-I" + scaleFolder + DIR_SEP "Internal");
        cflags.push_back("-I" + scaleFolder + DIR_SEP "Internal" DIR_SEP "include");
        cflags.push_back("-I" + scaleFolder + DIR_SEP "Frameworks");
        cflags.push_back("-I.");
        cflags.push_back("-L" + scaleFolder);
        cflags.push_back("-L" + scaleFolder + DIR_SEP "Internal" DIR_SEP "lib");
    #ifdef __APPLE__
        cflags.push_back("-Wl,-rpath," + scaleFolder);
        cflags.push_back("-Wl,-rpath," + scaleFolder + DIR_SEP "Internal" DIR_SEP "lib");
    #endif
        cflags.push_back("-" + optimizer);
        cflags.push_back("-DVERSION=\"" + std::string(VERSION) + "\"");
        if (strends(compiler, "++")) {
            cflags.push_back("-std=gnu++17");
        } else {
            cflags.push_back("-std=" + std::string(C_VERSION));
        }
        cflags.push_back("-fno-stack-protector");
#if !defined(_WIN32)
        cflags.push_back("-fPIC");
#else
        cflags.push_back("-Wl,-export-all-symbols");
        cflags.push_back("-fuse-ld=lld");
        cflags.push_back("-Wl,-lldmingw");
        cflags.push_back("-lKernel32");
        cflags.push_back("-lUser32");
        cflags.push_back("-lucrt");
#endif
        
        if (!Main::options::noScaleFramework) {
            if (std::find(frameworks.begin(), frameworks.end(), "Scale") == frameworks.end()) {
                frameworks.push_back("Scale");
            }
        }

        DBG("Checking frameworks");

        for (std::string& framework : frameworks) {
            checkFramework(framework, tmpFlags, frameworks, hasCppFiles, Version(std::string(FRAMEWORK_VERSION_REQ)));
        }

        #ifdef _WIN32
        #define OS_IDENT "win"
        #define OS_NAME "Windows"
        #elif defined(__APPLE__)
        #define OS_IDENT "mac"
        #define OS_NAME "macOS"
        #else
        #define OS_IDENT "linux"
        #define OS_NAME "Linux"
        #endif

        #if __SIZEOF_POINTER__ < 8
        #if defined(__wasm__)
        #define ARCH "WASM32"
        #elif defined(__arm__)
        #define ARCH "aarch32"
        #elif defined(__i386__)
        #define ARCH "x86"
        #else
        #define ARCH "unknown 32-bit"
        #endif
        #else
        #if defined(__wasm__)
        #define ARCH  "WASM64"
        #elif defined(__aarch64__)
        #define ARCH  "aarch64"
        #elif defined(__x86_64__)
        #define ARCH  "x86_64"
        #else
        #define ARCH  "unknown 64-bit"
        #endif
        #endif

        Main::config.setString("os", OS_IDENT);
        Main::config.setString("os-name", OS_NAME);
        Main::config.setString("arch", ARCH);
        Main::config.setString("version", VERSION);

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

        std::vector<Token>  tokens;

        DBG("Adding include paths for all files");

        for (size_t i = 0; i < Main::options::files.size() && !Main::options::printCflags; i++) {
            using path = std::filesystem::path;
            path s = Main::options::files[i];
            auto parent = s.parent_path();
            if (std::find(Main::options::includePaths.begin(), Main::options::includePaths.end(), parent.string()) == Main::options::includePaths.end()) {
                Main::options::includePaths.push_back(parent.string());
            }
        }

        DBG("Tokenizing %zu files", Main::options::files.size());

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

            Tokenizer tokenizer;
            FPResult result = tokenizer.tokenize(filename);

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

            tokenizer.removeInvalidTokens();

            FPResult importResult = tokenizer.tryImports();
            logErrors(importResult.errors);
            logWarns(importResult.warns);
            if (!importResult.errors.empty()) {
                exit(1);
            }

            std::vector<Token> theseTokens = tokenizer.getTokens();
            
            tokens.insert(tokens.end(), theseTokens.begin(), theseTokens.end());
        }

        if (Main::options::preprocessOnly) {
            std::cout << "Preprocessed " << Main::options::files.size() << " files." << std::endl;
            return 0;
        }

        DBG("Parsing syntax tree");

        TPResult syntaxResult;
        if (!Main::options::printCflags) {
            SyntaxTree lexer(tokens);
            lexer.parse(syntaxResult);

            logWarns(syntaxResult.warns);
            logErrors(syntaxResult.errors);
            size_t numErrs = sclc::count_if(syntaxResult.errors, [](const FPResult& err) -> bool {
                return !err.isNote;
            });
            numWarns += sclc::count_if(syntaxResult.warns, [](const FPResult& err) -> bool {
                return !err.isNote;
            });

            if (numErrs) {
                if (numWarns) {
                    std::cout << numWarns << " warning" << (numWarns == 1 ? "" : "s") << " and ";
                }
                std::cout << numErrs << " error" << (numErrs == 1 ? "" : "s") << " generated." << std::endl;
                return numErrs;
            }
        }

        DBG("Preparing parser");
        Parser parser(syntaxResult);

        if (hasCppFiles) {
            // link to c++ library if needed
            cflags.push_back("-lc++");
        }

        for (std::string& s : tmpFlags) {
            if (!Main::options::noLinkScale && strstarts(s, scaleFolder + DIR_SEP "Frameworks" DIR_SEP "Scale.framework")) {
                continue;
            }
            cflags.push_back(s);
        }

        std::string outputFileName = outfile + ".c";
        std::string outputHeaderFileName = outfile + ".h";

        DBG("Parsing");
        
        FPResult parseResult;
        parser.parse(
            parseResult,
            outputFileName,
            outputHeaderFileName
        );
        cflags.push_back(outputFileName);
        
        logWarns(parseResult.warns);
        logErrors(parseResult.errors);
        auto numErrs = sclc::count_if(parseResult.errors, [](const FPResult& err) {
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

        DBG("Finishing compile command");

        for (std::string& file : nonScaleFiles) {
            if (!Main::options::noLinkScale && strstarts(file, scaleFolder + DIR_SEP "Frameworks" DIR_SEP "Scale.framework")) {
                continue;
            }
            cflags.push_back("\"" + file + "\"");
        }

        if (!Main::options::printCflags && !Main::options::dontSpecifyOutFile) {
            cflags.push_back("-o");
            cflags.push_back("\"" + outfile + "\"");
        }
        
#if !defined(__APPLE__) && !defined(_WIN32)
        cflags.push_back("-Wl,-R");
        cflags.push_back("-Wl," + scaleFolder + DIR_SEP "Internal" DIR_SEP "lib");
        cflags.push_back("-Wl,-R");
        cflags.push_back("-Wl," + scaleFolder);
#endif
#if defined(__APPLE__)
        cflags.push_back("-Wl,-w");
#endif
        if (!Main::options::embedded) {
        #ifdef LINK_MATH
            cflags.push_back("-lm");
        #endif
            cflags.push_back("-lScaleRuntime");
            // cflags.push_back("-lgc");
        }

        std::string cmd = "";
        for (std::string& s : cflags) {
            cmd += s + " ";
        }

        if (std::filesystem::exists(Main::options::outfile)) {
            DBG("Removing %s", Main::options::outfile.c_str());
            std::filesystem::remove(Main::options::outfile);
        }

        DBG("Compiling with %s", cmd.c_str());

        if (Main::options::printCflags) {
            std::cout << cmd << std::endl;
            return 0;
        } else {
            int compile_command = system(cmd.c_str());

            if (compile_command) {
                std::cout << Color::RED << "Compilation failed with error code " << compile_command << Color::RESET << std::endl;
                return compile_command;
            }
        }

        DBG("Cleaning up files");

        std::filesystem::remove(outputFileName.c_str());
        std::filesystem::remove(outputHeaderFileName.c_str());

        return 0;
    }
}

int main(int argc, char const *argv[]) {
    signal(SIGINT, sclc::signalHandler);
    signal(SIGILL, sclc::signalHandler);
    signal(SIGFPE, sclc::signalHandler);
    signal(SIGSEGV, sclc::signalHandler);
    signal(SIGTERM, sclc::signalHandler);
#ifdef SIGBREAK
    signal(SIGBREAK, sclc::signalHandler);
#endif
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
