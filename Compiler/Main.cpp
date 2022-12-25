#if __SIZEOF_POINTER__ < 8
#error "Scale is not supported on this platform"
#endif

#include <iostream>
#include <string>
#include <chrono>
#include <cstdio>
#include <cstdlib>

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
#ifdef __APPLE__
#define COMPILER "gcc-12"
#else
#define COMPILER "gcc"
#endif
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
#define FRAMEWORK_VERSION_REQ "3.3"
#endif

#ifdef _WIN32
#define HOME getenv("USERPROFILE")
#else
#define HOME getenv("HOME")
#endif

namespace sclc
{
    std::string scaleFolder;

    void usage(std::string programName) {
        std::cout << "Usage: " << programName << " <filename> [args]" << std::endl;
        std::cout << std::endl;
        std::cout << "Accepted Flags:" << std::endl;
        std::cout << "  -t, --transpile  Transpile only" << std::endl;
        std::cout << "  -h, --help       Show this help" << std::endl;
        std::cout << "  -o <filename>    Specify Output file" << std::endl;
        std::cout << "  -f <framework>   Use Scale Framework" << std::endl;
        std::cout << "  --no-core        Do not implicitly require Scale Framework" << std::endl;
        std::cout << "  --no-main        Do not check for main Function" << std::endl;
        std::cout << "  -v, --version    Show version information" << std::endl;
        std::cout << "  --comp <comp>    Use comp as the compiler instead of gcc" << std::endl;
        std::cout << "  -run             Run the compiled program" << std::endl;
        std::cout << "  -cflags          Print c compiler flags and exit" << std::endl;
        std::cout << "  -debug           Run in debug mode" << std::endl;
        std::cout << std::endl;
        std::cout << "  Any other options are passed directly to " << std::string(COMPILER) << " (or compiler specified by --comp)" << std::endl;
    }

    bool contains(std::vector<std::string>& vec, std::string& item) {
        for (auto& i : vec) {
            if (i == item) {
                return true;
            }
        }
        return false;
    }

    std::string findFileInIncludePath(std::string file);

    int main(std::vector<std::string> args) {
        if (args.size() < 2) {
            usage(args[0]);
            return 1;
        }

        auto start = std::chrono::high_resolution_clock::now();

        std::string outfile     = std::string(DEFAULT_OUTFILE);
        std::string compiler    = std::string(COMPILER);
        scaleFolder             = std::string(HOME) + "/" + std::string(SCALE_INSTALL_DIR);
        std::vector<std::string> frameworks;
        std::vector<std::string> tmpFlags;
        std::string optimizer   = "O2";

        DragonConfig::CompoundEntry* scaleConfig = DragonConfig::ConfigParser().parse("scale.drg");
        if (scaleConfig) {
            if (scaleConfig->hasMember("outfile"))
                outfile = scaleConfig->getString("outfile")->getValue();

            if (scaleConfig->hasMember("compiler"))
                compiler = scaleConfig->getString("compiler")->getValue();

            if (scaleConfig->hasMember("optimizer"))
                optimizer = scaleConfig->getString("optimizer")->getValue();

            if (scaleConfig->hasMember("frameworks"))
                for (size_t i = 0; i < scaleConfig->getList("frameworks")->size(); i++)
                    frameworks.push_back(scaleConfig->getList("frameworks")->get(i));

            if (scaleConfig->hasMember("compilerFlags"))
                for (size_t i = 0; i < scaleConfig->getList("compilerFlags")->size(); i++)
                    tmpFlags.push_back(scaleConfig->getList("compilerFlags")->get(i));
        }

        for (size_t i = 1; i < args.size(); i++) {
            if (strends(std::string(args[i]), ".scale")) {
                if (!fileExists(args[i])) {
                    continue;
                }
                Main.options.files.push_back(args[i]);
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
                        if (!fileExists(scaleFolder + "/Frameworks/" + framework + ".framework") && !fileExists("./" + framework + ".framework")) {
                            std::cerr << "Framework '" << framework << "' not found" << std::endl;
                            return 1;
                        }
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
                        outfile = args[i + 1];
                        i++;
                    } else {
                        std::cerr << "Error: -o requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "-S") {
                    Main.options.assembleOnly = true;
                    Main.options.dontSpecifyOutFile = true;
                    tmpFlags.push_back("-S");
                } else if (args[i] == "--no-core") {
                    Main.options.noScaleFramework = true;
                } else if (args[i] == "--no-main") {
                    Main.options.noMain = true;
                } else if (args[i] == "-v" || args[i] == "--version") {
                    std::cout << "Scale Compiler version " << std::string(VERSION) << std::endl;
                    system((compiler + " -v").c_str());
                    return 0;
                } else if (args[i] == "--comp") {
                    if (i + 1 < args.size()) {
                        compiler = args[i + 1];
                        i++;
                    } else {
                        std::cerr << "Error: --comp requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "-run") {
                    Main.options.doRun = true;
                } else if (args[i] == "-debug") {
                    Main.options.debugBuild = true;
                } else if (args[i] == "-cflags") {
                    Main.options.printCflags = true;
                } else {
                    if (args[i] == "-c")
                        Main.options.dontSpecifyOutFile = true;
                    if (args[i][0] == '-' && args[i][1] == 'O')
                        optimizer = std::string(args[i].c_str() + 1);
                    if (args[i][0] == '-' && args[i][1] == 'I')
                        Main.options.includePaths.push_back(std::string(args[i].c_str() + 2));
                    if (args[i] == "-Werror")
                        Main.options.Werror = true;
                    tmpFlags.push_back(args[i]);
                }
            }
        }

        std::vector<std::string> cflags;
        Main.options.optimizer = optimizer;

        if (!Main.options.printCflags)
            cflags.push_back(compiler);
        cflags.push_back("-I" + scaleFolder + "/Frameworks");
        cflags.push_back("-I" + scaleFolder + "/Internal");
        cflags.push_back(scaleFolder + "/Internal/scale_internal.c");
        cflags.push_back("-std=" + std::string(C_VERSION));
        cflags.push_back("-" + optimizer);
        cflags.push_back("-DVERSION=\"" + std::string(VERSION) + "\"");

        if (Main.options.files.size() == 0) {
            std::cerr << Color::RED << "No translation units specified." << std::endl;
            return 1;
        }

        if (!Main.options.printCflags && !Main.options.dontSpecifyOutFile) {
            cflags.push_back("-o");
            cflags.push_back("\"" + outfile + "\"");
        }

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

        for (std::string framework : frameworks) {
            if (fileExists(scaleFolder + "/Frameworks/" + framework + ".framework/index.drg")) {
                DragonConfig::ConfigParser parser;
                DragonConfig::CompoundEntry* root = parser.parse(scaleFolder + "/Frameworks/" + framework + ".framework/index.drg");
                if (root == nullptr) {
                    std::cerr << Color::RED << "Failed to parse index.drg of Framework " << framework << std::endl;
                    return 1;
                }
                root = root->getCompound("framework");
                DragonConfig::ListEntry* implementers = root->getList("implementers");
                DragonConfig::ListEntry* implHeaders = root->getList("implHeaders");
                DragonConfig::ListEntry* depends = root->getList("depends");
                DragonConfig::ListEntry* compilerFlags = root->getList("compilerFlags");

                DragonConfig::StringEntry* versionTag = root->getString("version");
                std::string version = versionTag->getValue();
                if (versionTag == nullptr) {
                    std::cerr << "Framework " << framework << " does not specify a version! Skipping." << std::endl;
                    continue;
                }
                
                DragonConfig::StringEntry* headerDirTag = root->getString("headerDir");
                std::string headerDir = headerDirTag == nullptr ? "" : headerDirTag->getValue();
                
                DragonConfig::StringEntry* implDirTag = root->getString("implDir");
                std::string implDir = implDirTag == nullptr ? "" : implDirTag->getValue();
                
                DragonConfig::StringEntry* implHeaderDirTag = root->getString("implHeaderDir");
                std::string implHeaderDir = implHeaderDirTag == nullptr ? "" : implHeaderDirTag->getValue();

                Version ver = Version(version);
                Version compilerVersion = Version(VERSION);
                if (ver > compilerVersion) {
                    std::cerr << "Error: Framework '" << framework << "' requires Scale v" << version << " but you are using " << VERSION << std::endl;
                    return 1;
                }
                if (ver < FrameworkMinimumVersion) {
                    fprintf(stderr, "Error: Framework '%s' is too outdated (%s). Please update it to at least version %s\n", framework.c_str(), ver.asString().c_str(), FrameworkMinimumVersion.asString().c_str());
                    return 1;
                }

                for (size_t i = 0; depends != nullptr && i < depends->size(); i++) {
                    std::string depend = depends->get(i);
                    if (!contains(frameworks, depend)) {
                        std::cerr << "Error: Framework '" << framework << "' depends on '" << depend << "' but it is not included" << std::endl;
                        return 1;
                    }
                }

                for (size_t i = 0; compilerFlags != nullptr && i < compilerFlags->size(); i++) {
                    std::string flag = compilerFlags->get(i);
                    cflags.push_back(flag);
                }

                Main.frameworks.push_back(framework);

                if (headerDir.size() > 0) {
                    Main.options.includePaths.push_back(scaleFolder + "/Frameworks/" + framework + ".framework/" + headerDir);
                    Main.options.mapIncludePathsToFrameworks[framework] = scaleFolder + "/Frameworks/" + framework + ".framework/" + headerDir;
                }
                unsigned long implementersSize = implementers == nullptr ? 0 : implementers->size();
                for (unsigned long i = 0; i < implementersSize; i++) {
                    std::string implementer = implementers->get(i);
                    if (!Main.options.assembleOnly) {
                        cflags.push_back(scaleFolder + "/Frameworks/" + framework + ".framework/" + implDir + "/" + implementer);
                    }
                }
                for (unsigned long i = 0; implHeaders != nullptr && i < implHeaders->size() && implHeaderDir.size() > 0; i++) {
                    std::string header = framework + ".framework/" + implHeaderDir + "/" + implHeaders->get(i);
                    Main.frameworkNativeHeaders.push_back(header);
                }
            } else if (fileExists("./" + framework + ".framework/index.drg")) {
                DragonConfig::ConfigParser parser;
                DragonConfig::CompoundEntry* root = parser.parse("./" + framework + ".framework/index.drg");
                if (root == nullptr) {
                    std::cerr << Color::RED << "Failed to parse index.drg of Framework " << framework << std::endl;
                    return 1;
                }
                root = root->getCompound("framework");
                DragonConfig::ListEntry* implementers = root->getList("implementers");
                DragonConfig::ListEntry* implHeaders = root->getList("implHeaders");
                DragonConfig::ListEntry* depends = root->getList("depends");
                DragonConfig::ListEntry* compilerFlags = root->getList("compilerFlags");

                DragonConfig::StringEntry* versionTag = root->getString("version");
                std::string version = versionTag->getValue();
                if (versionTag == nullptr) {
                    std::cerr << "Framework " << framework << " does not specify a version! Skipping." << std::endl;
                    continue;
                }
                
                DragonConfig::StringEntry* headerDirTag = root->getString("headerDir");
                std::string headerDir = headerDirTag == nullptr ? "" : headerDirTag->getValue();
                
                DragonConfig::StringEntry* implDirTag = root->getString("implDir");
                std::string implDir = implDirTag == nullptr ? "" : implDirTag->getValue();
                
                DragonConfig::StringEntry* implHeaderDirTag = root->getString("implHeaderDir");
                std::string implHeaderDir = implHeaderDirTag == nullptr ? "" : implHeaderDirTag->getValue();

                Version ver = Version(version);
                Version compilerVersion = Version(VERSION);
                if (ver > compilerVersion) {
                    std::cerr << "Error: Framework '" << framework << "' requires Scale v" << version << " but you are using " << VERSION << std::endl;
                    return 1;
                }
                if (ver < FrameworkMinimumVersion) {
                    fprintf(stderr, "Error: Framework '%s' is too outdated (%s). Please update it to at least version %s\n", framework.c_str(), ver.asString().c_str(), FrameworkMinimumVersion.asString().c_str());
                    return 1;
                }

                for (size_t i = 0; depends != nullptr && i < depends->size(); i++) {
                    std::string depend = depends->get(i);
                    if (!contains(frameworks, depend)) {
                        std::cerr << "Error: Framework '" << framework << "' depends on '" << depend << "' but it is not included" << std::endl;
                        return 1;
                    }
                }

                for (size_t i = 0; compilerFlags != nullptr && i < compilerFlags->size(); i++) {
                    std::string flag = compilerFlags->get(i);
                    cflags.push_back(flag);
                }

                Main.frameworks.push_back(framework);

                if (headerDir.size() > 0) {
                    Main.options.includePaths.push_back("./" + framework + ".framework/" + headerDir);
                    Main.options.mapIncludePathsToFrameworks[framework] = "./" + framework + ".framework/" + headerDir;
                }
                unsigned long implementersSize = implementers == nullptr ? 0 : implementers->size();
                for (unsigned long i = 0; i < implementersSize && implDir.size() > 0; i++) {
                    std::string implementer = implementers->get(i);
                    if (!Main.options.assembleOnly) {
                        cflags.push_back("./" + framework + ".framework/" + implDir + "/" + implementer);
                    }
                }
                for (unsigned long i = 0; implHeaders != nullptr && i < implHeaders->size() && implHeaderDir.size() > 0; i++) {
                    std::string header = framework + ".framework/" + implHeaderDir + "/" + implHeaders->get(i);
                    Main.frameworkNativeHeaders.push_back(header);
                }
            }
        }
        Main.options.includePaths.push_back("./");
        if (!Main.options.noScaleFramework) {
            std::string file = "core.scale";
            std::string framework = "Scale";
            std::string fullFile = Main.options.mapIncludePathsToFrameworks[framework] + "/" + file;
            if (std::find(Main.options.files.begin(), Main.options.files.end(), fullFile) == Main.options.files.end()) {
                Main.options.files.push_back(fullFile);
            }
        }

        if (!Main.options.doRun) std::cout << "Scale Compiler version " << std::string(VERSION) << std::endl;
        
        std::vector<Token>  tokens;

        for (size_t i = 0; i < Main.options.files.size() && !Main.options.printCflags; i++) {
            std::string filename = Main.options.files[i];
            if (!Main.options.doRun) {
                std::cout << "Compiling ";
                if (strncmp(filename.c_str(), (scaleFolder + "/Frameworks/").c_str(), (scaleFolder + "/Frameworks/").size()) == 0) {
                    std::cout << std::string(filename.c_str() + scaleFolder.size() + 12);
                } else {
                    std::cout << filename;
                }
                std::cout << "..." << std::endl;
            }

            Tokenizer tokenizer;
            Main.tokenizer = &tokenizer;
            FPResult result = Main.tokenizer->tokenize(filename);

            if (result.warns.size() > 0) {
                for (FPResult error : result.warns) {
                    if (error.line == 0) {
                        std::cout << Color::BOLDRED << "Fatal Error: " << error.message << std::endl;
                        continue;
                    }
                    FILE* f = fopen(std::string(error.in).c_str(), "r");
                    char* line = (char*) malloc(sizeof(char) * 500);
                    int i = 1;
                    fseek(f, 0, SEEK_SET);
                    std::cerr << Color::BOLDMAGENTA << "Warning: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                    i = 1;
                    while (fgets(line, 500, f) != NULL) {
                        if (i == error.line) {
                            std::cerr << Color::BOLDMAGENTA << "> " << Color::RESET;
                            std::string l;
                            if (error.type == tok_string_literal) {
                                l = replaceFirstAfter(line, "\"" + error.value + "\"", Color::BOLDMAGENTA + "\"" + error.value + "\"" + Color::RESET, error.column);
                            } else if (error.type == tok_char_literal) {
                                char* c = new char[4];
                                snprintf(c, 4, "%c", (char) strtol(error.value.c_str(), NULL, 0));
                                l = replaceFirstAfter(line, std::string("'") + c + "'", Color::BOLDMAGENTA + std::string("'") + c + "'" + Color::RESET, error.column);
                            } else {
                                l = replaceFirstAfter(line, error.value, Color::BOLDMAGENTA + error.value + Color::RESET, error.column);
                            }
                            if (l.at(l.size() - 1) != '\n') {
                                l += '\n';
                            }
                            std::cerr << l;
                        } else if (i == error.line - 1 || i == error.line - 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        } else if (i == error.line + 1 || i == error.line + 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        }
                        i++;
                    }
                    fclose(f);
                    std::cerr << std::endl;
                    free(line);
                }
            }
            if (result.errors.size() > 0) {
                for (FPResult error : result.errors) {
                    if (error.line == 0) {
                        std::cout << Color::BOLDRED << "Fatal Error: " << error.message << std::endl;
                        continue;
                    }
                    FILE* f = fopen(std::string(error.in).c_str(), "r");
                    char* line = (char*) malloc(sizeof(char) * 500);
                    int i = 1;
                    fseek(f, 0, SEEK_SET);
                    std::cerr << Color::BOLDRED << "Error: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                    i = 1;
                    while (fgets(line, 500, f) != NULL) {
                        if (i == error.line) {
                            std::cerr << Color::BOLDRED << "> " << Color::RESET;
                            std::string l;
                            if (error.type == tok_string_literal) {
                                l = replaceFirstAfter(line, "\"" + error.value + "\"", Color::BOLDRED + "\"" + error.value + "\"" + Color::RESET, error.column);
                            } else if (error.type == tok_char_literal) {
                                char* c = new char[4];
                                snprintf(c, 4, "%c", (char) strtol(error.value.c_str(), NULL, 0));
                                l = replaceFirstAfter(line, std::string("'") + c + "'", Color::BOLDRED + std::string("'") + c + "'" + Color::RESET, error.column);
                            } else {
                                l = replaceFirstAfter(line, error.value, Color::BOLDRED + error.value + Color::RESET, error.column);
                            }
                            if (l.at(l.size() - 1) != '\n') {
                                l += '\n';
                            }
                            std::cerr << l;
                        } else if (i == error.line - 1 || i == error.line - 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        } else if (i == error.line + 1 || i == error.line + 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        }
                        i++;
                    }
                    fclose(f);
                    std::cerr << std::endl;
                    free(line);
                }
                remove(std::string(filename + ".scale-preproc").c_str());
                return result.errors.size();
            }

            FPResult importResult = Main.tokenizer->tryImports();
            if (!importResult.success) {
                if (importResult.line == 0) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << importResult.message << std::endl;
                    continue;
                }
                FILE* f = fopen(std::string(importResult.in).c_str(), "r");
                char* line = (char*) malloc(sizeof(char) * 500);
                int i = 1;
                fseek(f, 0, SEEK_SET);
                std::cerr << Color::BOLDRED << "Error: " << Color::RESET << importResult.in << ":" << importResult.line << ":" << importResult.column << ": " << importResult.message << std::endl;
                i = 1;
                while (fgets(line, 500, f) != NULL) {
                    if (i == importResult.line) {
                        std::cerr << Color::BOLDRED << "> " << Color::RESET;
                        std::string l;
                        if (importResult.type == tok_string_literal) {
                            l = replaceFirstAfter(line, "\"" + importResult.value + "\"", Color::BOLDRED + "\"" + importResult.value + "\"" + Color::RESET, importResult.column);
                        } else if (importResult.type == tok_char_literal) {
                            char* c = new char[4];
                            snprintf(c, 4, "%c", (char) strtol(importResult.value.c_str(), NULL, 0));
                            l = replaceFirstAfter(line, std::string("'") + c + "'", Color::BOLDRED + "'" + c + "'" + Color::RESET, importResult.column);
                        } else {
                            l = replaceFirstAfter(line, importResult.value, Color::BOLDRED + importResult.value + Color::RESET, importResult.column);
                        }
                        if (l.at(l.size() - 1) != '\n') {
                            l += '\n';
                        }
                        std::cerr << l;
                    } else if (i == importResult.line - 1 || i == importResult.line - 2) {
                        if (strlen(line) > 0)
                            std::cerr << "  " << line;
                    } else if (i == importResult.line + 1 || i == importResult.line + 2) {
                        if (strlen(line) > 0)
                            std::cerr << "  " << line;
                    }
                    i++;
                }
                fclose(f);
                std::cerr << std::endl;
                free(line);
                remove(std::string(filename + ".scale-preproc").c_str());
                return 1;
            }

            std::vector<Token> theseTokens = Main.tokenizer->getTokens();
            
            tokens.insert(tokens.end(), theseTokens.begin(), theseTokens.end());

            remove(std::string(filename + ".c").c_str());
        }

        if (Main.options.preprocessOnly) {
            std::cout << "Preprocessed " << Main.options.files.size() << " files." << std::endl;
            return 0;
        }


        TPResult result;
        if (!Main.options.printCflags) {
            TokenParser lexer(tokens);
            Main.lexer = &lexer;
            result = Main.lexer->parse();
        }

        if (!Main.options.printCflags && result.warns.size() > 0) {
            for (FPResult error : result.warns) {
                if (error.line == 0) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.message << std::endl;
                    continue;
                }
                FILE* f = fopen(std::string(error.in).c_str(), "r");
                char* line = (char*) malloc(sizeof(char) * 500);
                int i = 1;
                fseek(f, 0, SEEK_SET);
                std::cerr << Color::BOLDMAGENTA << "Error: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                i = 1;
                while (fgets(line, 500, f) != NULL) {
                    if (i == error.line) {
                        std::cerr << Color::BOLDMAGENTA << "> " << Color::RESET;
                        std::string l;
                        if (error.type == tok_string_literal) {
                            l = replaceFirstAfter(line, "\"" + error.value + "\"", Color::BOLDMAGENTA + "\"" + error.value + "\"" + Color::RESET, error.column);
                        } else if (error.type == tok_char_literal) {
                            char* c = new char[4];
                            snprintf(c, 4, "%c", (char) strtol(error.value.c_str(), NULL, 0));
                            l = replaceFirstAfter(line, std::string("'") + c + "'", Color::BOLDMAGENTA + std::string("'") + c + "'" + Color::RESET, error.column);
                        } else {
                            l = replaceFirstAfter(line, error.value, Color::BOLDMAGENTA + error.value + Color::RESET, error.column);
                        }
                        if (l.at(l.size() - 1) != '\n') {
                            l += '\n';
                        }
                        std::cerr << l;
                    } else if (i == error.line - 1 || i == error.line - 2) {
                        if (strlen(line) > 0)
                            std::cerr << "  " << line;
                    } else if (i == error.line + 1 || i == error.line + 2) {
                        if (strlen(line) > 0)
                            std::cerr << "  " << line;
                    }
                    i++;
                }
                fclose(f);
                std::cerr << std::endl;
                free(line);
            }
        }
        if (!Main.options.printCflags && result.errors.size() > 0) {
            for (FPResult error : result.errors) {
                if (error.line == 0) {
                    std::cout << Color::BOLDRED << "Fatal Error: " << error.message << std::endl;
                    continue;
                }
                FILE* f = fopen(std::string(error.in).c_str(), "r");
                char* line = (char*) malloc(sizeof(char) * 500);
                int i = 1;
                fseek(f, 0, SEEK_SET);
                std::cerr << Color::BOLDRED << "Error: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                i = 1;
                while (fgets(line, 500, f) != NULL) {
                    if (i == error.line) {
                        std::cerr << Color::BOLDRED << "> " << Color::RESET;
                        std::string l;
                        if (error.type == tok_string_literal) {
                            l = replaceFirstAfter(line, "\"" + error.value + "\"", Color::BOLDRED + "\"" + error.value + "\"" + Color::RESET, error.column);
                        } else if (error.type == tok_char_literal) {
                            char* c = new char[4];
                            snprintf(c, 4, "%c", (char) strtol(error.value.c_str(), NULL, 0));
                            l = replaceFirstAfter(line, std::string("'") + c + "'", Color::BOLDRED + std::string("'") + c + "'" + Color::RESET, error.column);
                        } else {
                            l = replaceFirstAfter(line, error.value, Color::BOLDRED + error.value + Color::RESET, error.column);
                        }
                        if (l.at(l.size() - 1) != '\n') {
                            l += '\n';
                        }
                        std::cerr << l;
                    } else if (i == error.line - 1 || i == error.line - 2) {
                        if (strlen(line) > 0)
                            std::cerr << "  " << line;
                    } else if (i == error.line + 1 || i == error.line + 2) {
                        if (strlen(line) > 0)
                            std::cerr << "  " << line;
                    }
                    i++;
                }
                fclose(f);
                std::cerr << std::endl;
                free(line);
            }
            return result.errors.size();
        }

        std::string source = "out.c";
        if (!Main.options.printCflags) {
            FunctionParser parser(result);
            Main.parser = &parser;
            
            srand(time(NULL));

            FPResult parseResult = Main.parser->parse(source);
            if (parseResult.warns.size() > 0) {
                for (FPResult error : parseResult.warns) {
                    if (error.line == 0) {
                        std::cout << Color::BOLDRED << "Fatal Error: " << error.message << std::endl;
                        continue;
                    }
                    FILE* f = fopen(std::string(error.in).c_str(), "r");
                    char* line = (char*) malloc(sizeof(char) * 500);
                    int i = 1;
                    fseek(f, 0, SEEK_SET);
                    std::cerr << Color::BOLDMAGENTA << "Warning: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                    i = 1;
                    while (fgets(line, 500, f) != NULL) {
                        if (i == error.line) {
                            std::cerr << Color::BOLDMAGENTA << "> " << Color::RESET;
                            std::string l;
                            if (error.type == tok_string_literal) {
                                l = replaceFirstAfter(line, "\"" + error.value + "\"", Color::BOLDMAGENTA + "\"" + error.value + "\"" + Color::RESET, error.column);
                            } else if (error.type == tok_char_literal) {
                                char* c = new char[4];
                                snprintf(c, 4, "%c", (char) strtol(error.value.c_str(), NULL, 0));
                                l = replaceFirstAfter(line, std::string("'") + c + "'", Color::BOLDMAGENTA + std::string("'") + c + "'" + Color::RESET, error.column);
                            } else {
                                l = replaceFirstAfter(line, error.value, Color::BOLDMAGENTA + error.value + Color::RESET, error.column);
                            }
                            if (l.at(l.size() - 1) != '\n') {
                                l += '\n';
                            }
                            std::cerr << l;
                        } else if (i == error.line - 1 || i == error.line - 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        } else if (i == error.line + 1 || i == error.line + 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        }
                        i++;
                    }
                    fclose(f);
                    std::cerr << std::endl;
                    free(line);
                }
            }
            if (parseResult.errors.size() > 0) {
                for (FPResult error : parseResult.errors) {
                    if (error.line == 0) {
                        std::cout << Color::BOLDRED << "Fatal Error: " << error.message << std::endl;
                        continue;
                    }
                    FILE* f = fopen(std::string(error.in).c_str(), "r");
                    char* line = (char*) malloc(sizeof(char) * 500);
                    int i = 1;
                    fseek(f, 0, SEEK_SET);
                    std::cerr << Color::BOLDRED << "Error: " << Color::RESET << error.in << ":" << error.line << ":" << error.column << ": " << error.message << std::endl;
                    i = 1;
                    while (fgets(line, 500, f) != NULL) {
                        if (i == error.line) {
                            std::cerr << Color::BOLDRED << "> " << Color::RESET;
                            std::string l;
                            if (error.type == tok_string_literal) {
                                l = replaceFirstAfter(line, "\"" + error.value + "\"", Color::BOLDRED + "\"" + error.value + "\"" + Color::RESET, error.column);
                            } else if (error.type == tok_char_literal) {
                                char* c = new char[4];
                                snprintf(c, 4, "%c", (char) strtol(error.value.c_str(), NULL, 0));
                                l = replaceFirstAfter(line, std::string("'") + c + "'", Color::BOLDRED + std::string("'") + c + "'" + Color::RESET, error.column);
                            } else {
                                l = replaceFirstAfter(line, error.value, Color::BOLDRED + error.value + Color::RESET, error.column);
                            }
                            if (l.at(l.size() - 1) != '\n') {
                                l += '\n';
                            }
                            std::cerr << l;
                        } else if (i == error.line - 1 || i == error.line - 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        } else if (i == error.line + 1 || i == error.line + 2) {
                            if (strlen(line) > 0)
                                std::cerr << "  " << line;
                        }
                        i++;
                    }
                    fclose(f);
                    std::cerr << std::endl;
                    free(line);
                }
                remove((source + ".c").c_str());
                remove((source + ".h").c_str());
                return parseResult.errors.size();
            }
        }

        if (Main.options.transpileOnly) {
            auto end = chrono::high_resolution_clock::now();
            double duration = (double) chrono::duration_cast<chrono::nanoseconds>(end - start).count() / 1000000000.0;
            std::cout << "Transpiled successfully in " << duration << " seconds." << std::endl;
            return 0;
        }

        for (std::string s : tmpFlags) {
            cflags.push_back(s);
        }
        
        cflags.push_back(source);

#ifdef LINK_MATH
        cflags.push_back("-lm");
#endif

        std::string cmd = "";
        for (std::string s : cflags) {
            cmd += s + " ";
        }

        if (Main.options.printCflags) {
            std::cout << cmd << std::endl;
            return 0;
        }

        int ret = system(cmd.c_str());

        if (ret != 0) {
            std::cerr << Color::RED << "Compilation failed with code " << ret << Color::RESET << std::endl;
            remove(source.c_str());
            return ret;
        }
        if (!Main.options.transpileOnly) {
            remove(source.c_str());
        }
        
        if (!Main.options.doRun) std::cout << Color::GREEN << "Compilation finished." << Color::RESET << std::endl;

        auto end = chrono::high_resolution_clock::now();
        double duration = (double) chrono::duration_cast<chrono::nanoseconds>(end - start).count() / 1000000000.0;
        if (!Main.options.doRun) std::cout << "Took " << duration << " seconds." << std::endl;

        if (Main.options.doRun) {
            const char** argv = new const char*[1];
            argv[0] = (const char*) outfile.c_str();
        #ifdef _WIN32
            execv(argv[0], (const char* const*) argv);
        #else
            execv(argv[0], (char* const*) argv);
        #endif
        }

        return 0;
    }
}

int main(int argc, char const *argv[]) {
    signal(SIGSEGV, sclc::signalHandler);
    signal(SIGABRT, sclc::signalHandler);
    signal(SIGILL, sclc::signalHandler);
    signal(SIGFPE, sclc::signalHandler);
    signal(SIGINT, sclc::signalHandler);
    signal(SIGTERM, sclc::signalHandler);
#ifdef SIGQUIT
    signal(SIGQUIT, sclc::signalHandler);
#endif

    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        args.push_back(argv[i]);
    }
    return sclc::main(args);
}
