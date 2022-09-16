#ifndef MAIN_CPP_
#define MAIN_CPP_

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
#endif

#include "Common.hpp"
#include "modules/DragonConfig.hpp"

#ifndef VERSION
#define VERSION "unknown. Did you forget to build with -DVERSION=<version>?"
#endif

#ifndef PREPROCESSOR
#define PREPROCESSOR "cpp"
#endif

#ifndef COMPILER_FEATURES
#define COMPILER_FEATURES ""
#endif

namespace sclc
{
    std::string scaleFolder;

    struct Version {
        int major;
        int minor;
        int patch;

        Version(std::string str) {
            std::string::difference_type n = std::count(str.begin(), str.end(), '.');
            if (n == 1) {
                sscanf(str.c_str(), "%d.%d", &major, &minor);
                patch = 0;
            } else if (n == 2) {
                sscanf(str.c_str(), "%d.%d.%d", &major, &minor, &patch);
            } else if (n == 0) {
                sscanf(str.c_str(), "%d", &major);
                minor = 0;
                patch = 0;
            } else {
                std::cerr << "Unknown version number: " << str << std::endl;
                exit(1);
            }
        }
        ~Version() {}

        inline bool operator==(Version& v) {
            return (major == v.major) && (minor == v.minor) && (patch == v.patch);
        }

        inline bool operator>(Version& v) {
            if (major > v.major) {
                return true;
            } else if (major == v.major) {
                if (minor > v.minor) {
                    return true;
                } else if (minor == v.minor) {
                    if (patch > v.patch) {
                        return true;
                    }
                    return false;
                }
                return false;
            }
            return false;
        }

        inline bool operator>=(Version& v) {
            return ((*this) > v) || ((*this) == v);
        }

        inline bool operator<=(Version& v) {
            return !((*this) > v);
        }

        inline bool operator<(Version& v) {
            return !((*this) >= v);
        }

        inline bool operator!=(Version& v) {
            return !((*this) == v);
        }

        std::string asString() {
            return std::to_string(this->major) + "." + std::to_string(this->minor) + "." + std::to_string(this->patch);
        }
    };

    void usage(std::string programName) {
        std::cout << "Usage: " << programName << " <filename> [args]" << std::endl;
        std::cout << "  --transpile, -t  Transpile only" << std::endl;
        std::cout << "  --help, -h       Show this help" << std::endl;
        std::cout << "  -o <filename>    Specify Output file" << std::endl;
        std::cout << "  -E               Preprocess only" << std::endl;
        std::cout << "  -f <framework>   Use Scale Framework" << std::endl;
        std::cout << "  --no-core        Do not implicitly require Core Framework" << std::endl;
    }

    bool contains(std::vector<std::string>& vec, std::string& item) {
        for (auto& i : vec) {
            if (i == item) {
                return true;
            }
        }
        return false;
    }

    int main(std::vector<std::string> args)
    {
        if (args.size() < 2) {
            usage(args[0]);
            return 1;
        }

        auto start = std::chrono::high_resolution_clock::now();

        bool transpileOnly      = false;
        bool preprocessOnly     = false;
        bool assembleOnly       = false;
        bool noCoreFramework    = false;

        std::string outfile     = "out.scl";
        scaleFolder             = std::string(getenv("HOME")) + "/Scale";
        std::string cmd         = "clang " + std::string(COMPILER_FEATURES) + " -I" + scaleFolder + "/Frameworks -I" + scaleFolder + "/Internal " + scaleFolder + "/Internal/scale_internal.c -std=gnu17 -O2 -DVERSION=\"" + std::string(VERSION) + "\" ";
        std::vector<std::string> files;
        std::vector<std::string> frameworks;

        for (size_t i = 1; i < args.size(); i++) {
            if (strends(std::string(args[i]), ".scale")) {
                files.push_back(args[i]);
            } else {
                if (args[i] == "--transpile" || args[i] == "-t") {
                    transpileOnly = true;
                } else if (args[i] == "--help" || args[i] == "-h") {
                    usage(args[0]);
                    return 0;
                } else if (args[i] == "-E") {
                    preprocessOnly = true;
                } else if (args[i] == "-f") {
                    if (i + 1 < args.size()) {
                        std::string framework = args[i + 1];
                        if (!fileExists(scaleFolder + "/Frameworks/" + framework + ".framework")) {
                            std::cerr << "Framework '" << framework << "' not found" << std::endl;
                            return 1;
                        }
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
                    assembleOnly = true;
                    cmd += "-S ";
                } else if (args[i] == "--no-core") {
                    noCoreFramework = true;
                } else {
                    cmd += args[i] + " ";
                }
            }
        }

        cmd += "-o \"" + outfile + "\" ";

        if (!noCoreFramework)
            frameworks.push_back("Core");

        std::string globalPreproc = std::string(PREPROCESSOR) + " -DVERSION=\"" + std::string(VERSION) + "\" ";
        Version FrameworkMinimumVersion = Version("2.8");

        for (std::string framework : frameworks) {
            DragonConfig::ConfigParser parser;
            DragonConfig::CompoundEntry root = parser.parse(scaleFolder + "/Frameworks/" + framework + ".framework/index.drg").getCompound("framework");
            DragonConfig::ListEntry implementers = root.getList("implementers");
            DragonConfig::ListEntry implHeaders = root.getList("implHeaders");
            DragonConfig::ListEntry depends = root.getList("depends");
            DragonConfig::ListEntry compilerFlags = root.getList("compilerFlags");
            std::string version = root.getString("version").getValue();
            std::string headerDir = root.getString("headerDir").getValue();
            std::string implDir = root.getString("implDir").getValue();
            std::string implHeaderDir = root.getString("implHeaderDir").getValue();

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

            for (size_t i = 0; i < depends.size(); i++) {
                std::string depend = depends.get(i);
                if (!contains(frameworks, depend)) {
                    std::cerr << "Error: Framework '" << framework << "' depends on '" << depend << "' but it is not included" << std::endl;
                    return 1;
                }
            }

            for (size_t i = 0; i < compilerFlags.size(); i++) {
                std::string flag = compilerFlags.get(i);
                cmd += flag + " ";
            }

            Main.frameworks.push_back(framework);

            globalPreproc += " -I" + scaleFolder + "/Frameworks/" + framework + ".framework/" + headerDir;
            unsigned long implementersSize = implementers.size();
            for (unsigned long i = 0; i < implementersSize; i++) {
                std::string implementer = implementers.get(i);
                if (!assembleOnly)
                    cmd += scaleFolder + "/Frameworks/" + framework + ".framework/" + implDir + "/" + implementer + " ";
            }
            for (unsigned long i = 0; i < implHeaders.size(); i++) {
                std::string header = framework + ".framework/" + implHeaderDir + "/" + implHeaders.get(i);
                Main.frameworkNativeHeaders.push_back(header);
            }
        }

        std::cout << "Scale Compiler version " << std::string(VERSION) << std::endl;
        
        std::vector<Token>  tokens;

        for (size_t i = 0; i < files.size(); i++) {
            std::string filename = files[i];
            std::cout << "Compiling " << filename << "..." << std::endl;

            std::string preproc_cmd = globalPreproc + " " + filename + " " + filename + ".scale-preproc";
            int preprocResult = system(preproc_cmd.c_str());

            if (preprocResult != 0) {
                std::cout << "Error: Preprocessor failed." << std::endl;
                return 1;
            }

            if (preprocessOnly) {
                std::cout << "Preprocessed " << filename << std::endl;
                continue;
            }

            Tokenizer tokenizer;
            Main.tokenizer = &tokenizer;
            FPResult result = Main.tokenizer->tokenize(filename + ".scale-preproc");

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
                            std::string l = replaceFirstAfter(line, error.value, Color::BOLDRED + error.value + Color::RESET, error.column);
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

            std::vector<Token> theseTokens = Main.tokenizer->getTokens();

            tokens.insert(tokens.end(), theseTokens.begin(), theseTokens.end());

            remove(std::string(filename + ".scale-preproc").c_str());
        }

        if (preprocessOnly) {
            std::cout << "Preprocessed " << files.size() << " files." << std::endl;
            return 0;
        }

        TokenParser lexer(tokens);
        Main.lexer = &lexer;
        TPResult result = Main.lexer->parse();

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
                        std::string l = replaceFirstAfter(line, error.value, Color::BOLDRED + error.value + Color::RESET, error.column);
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
        
        FunctionParser parser(result);
        Main.parser = &parser;
        char* source = (char*) malloc(sizeof(char) * 50);
        
        srand(time(NULL));
        if (!transpileOnly) sprintf(source, ".scale-%08x.tmp", (unsigned int) rand());
        else sprintf(source, "out");
        cmd += source;
        cmd += ".c ";

        FPResult parseResult = Main.parser->parse(std::string(source));

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
                        std::string l = replaceFirstAfter(line, error.value, Color::BOLDRED + error.value + Color::RESET, error.column);
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
            remove((std::string(source) + ".c").c_str());
            remove((std::string(source) + ".h").c_str());
            return parseResult.errors.size();
        }

        if (transpileOnly) {
            auto end = chrono::high_resolution_clock::now();
            double duration = (double) chrono::duration_cast<chrono::nanoseconds>(end - start).count() / 1000000000.0;
            std::cout << "Transpiled successfully in " << duration << " seconds." << std::endl;
            return 0;
        }

        std::cout << "Compiling with " << cmd << std::endl;

        int ret = system(cmd.c_str());

        if (ret != 0) {
            std::cerr << Color::RED << "Compilation failed with code " << ret << Color::RESET << std::endl;
            remove((std::string(source) + ".c").c_str());
            remove((std::string(source) + ".h").c_str());
            return ret;
        }
        remove((std::string(source) + ".c").c_str());
        remove((std::string(source) + ".h").c_str());
        
        std::cout << Color::GREEN << "Compilation finished." << Color::RESET << std::endl;

        auto end = chrono::high_resolution_clock::now();
        double duration = (double) chrono::duration_cast<chrono::nanoseconds>(end - start).count() / 1000000000.0;
        std::cout << "Took " << duration << " seconds." << std::endl;

        return 0;
    }
}

int main(int argc, char const *argv[])
{
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

#endif
