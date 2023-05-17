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
#define COMPILER "gcc"
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
#define FRAMEWORK_VERSION_REQ "23.5"
#endif

#ifndef SCL_ROOT_DIR
#ifdef _WIN32
#define SCL_ROOT_DIR getenv("USERPROFILE")
#else
#define SCL_ROOT_DIR getenv("HOME")
#endif
#endif

namespace sclc
{
    std::string scaleFolder;

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
        hash h = hash1((char*) std::string(VERSION).c_str());
        snprintf(s, 256, "%x%x%x%x", h, h, h, h);
        return std::string(s);
    }

    FPResult findFileInIncludePath(std::string file);

    std::vector<std::string> split(const std::string& str, const std::string& delimiter);

    template <typename A,typename B,typename C>
    class triple {
    public:
        A first;
        B second;
        C third;

        triple(A a, B b, C c) {
            first = a;
            second = b;
            third = c;
        }
    };
    using Triple = triple<std::string, std::string, std::string>;
    using TripleList = std::vector<Triple>;

    auto parseSclDoc(std::string file) {
        std::map<std::string, TripleList> docs;
        FILE* fp = fopen(file.c_str(), "rb");
        if (fp) fseek(fp, 0, SEEK_END);
        long sz = ftell(fp);
        if (fp) fseek(fp, 0, SEEK_SET);
        char* data = new char[sz];
        fread(data, 1, sz, fp);
        fclose(fp);

        std::vector<std::string> lines = split(std::string(data), "\n");
        std::string current = "";
        std::string nextFileName = "";
        for (size_t i = 0; i < lines.size(); i++) {
            std::string line = lines[i];
            try {
                line = replaceAll(line, "`", "");
            } catch (std::out_of_range& e) {}
            if (strstarts(line, "@")) {
                try {
                    nextFileName = line.substr(1);
                } catch (std::out_of_range& e) {}
            } else if (strstarts(line, "##") && !strstarts(line, "###")) {
                try {
                    current = line.substr(3);
                    TripleList key_value;
                    docs[current] = key_value;
                } catch (std::out_of_range& e) {}
            } else if (strstarts(line, "###")) {
                std::string key = line.substr(4);
                docs[current].push_back(triple(key, std::string(""), nextFileName));
                line = lines[++i];
                std::string linePrefix = "";
                while (i < lines.size() && !strstarts(line, "##") && !strstarts(line, "@")) {
                    while (strstarts(line, "<div")) {
                        line = lines[++i];
                    }
                    if (line == "<pre>" || strstarts(line, "<pre>")) {
                        linePrefix = Color::CYAN + "  ";
                    } else if (line == "</pre>" || strstarts(line, "</pre>")) {
                        linePrefix = "";
                    } else {
                        docs[current].back().second += "  " + linePrefix + line + "\n";
                    }
                    line = lines[++i];
                }
                std::string prev = docs[current].back().second;
                std::string now = replaceAll(prev, "\n  \n", "\n");
                now = replaceAll(now, "<<RESET>>", Color::GREEN);
                now = replaceAll(now, "<<BLACK>>", Color::BLACK);
                now = replaceAll(now, "<<RED>>", Color::RED);
                now = replaceAll(now, "<<GREEN>>", Color::GREEN);
                now = replaceAll(now, "<<YELLOW>>", Color::YELLOW);
                now = replaceAll(now, "<<BLUE>>", Color::BLUE);
                now = replaceAll(now, "<<MAGENTA>>", Color::MAGENTA);
                now = replaceAll(now, "<<CYAN>>", Color::CYAN);
                now = replaceAll(now, "<<WHITE>>", Color::WHITE);
                now = replaceAll(now, "<<BOLDBLACK>>", Color::BOLDBLACK);
                now = replaceAll(now, "<<BOLDRED>>", Color::BOLDRED);
                now = replaceAll(now, "<<BOLDGREEN>>", Color::BOLDGREEN);
                now = replaceAll(now, "<<BOLDYELLOW>>", Color::BOLDYELLOW);
                now = replaceAll(now, "<<BOLDBLUE>>", Color::BOLDBLUE);
                now = replaceAll(now, "<<BOLDMAGENTA>>", Color::BOLDMAGENTA);
                now = replaceAll(now, "<<BOLDCYAN>>", Color::BOLDCYAN);
                now = replaceAll(now, "<<BOLDWHITE>>", Color::BOLDWHITE);

                docs[current].back().second = now;
                i--;
            }
        }
        return docs;
    }

    auto docHandler(std::vector<std::string> args) {
        std::map<std::string, TripleList> docs;
        
        struct {
            std::vector<std::string> find;
            std::vector<std::string> find_category;
            bool help;
            bool info;
            bool categories;
        } DocOps = {
            std::vector<std::string>(),
            std::vector<std::string>(),
            false,
            Main.options.printDocFor.size() == 0,
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
            } else if (arg == "info") {
                DocOps.info = true;
            } else if (arg == "categories") {
                DocOps.categories = true;
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
            std::cout << "  info                     Display Scale language documentation." << std::endl;
            std::cout << "  help                     Display this help." << std::endl;
            std::cout << "  categories               Display categories." << std::endl;
            std::cout << "" << std::endl;
            return 0;
        }

        if (DocOps.categories) {
            if (Main.options.printDocFor.size()) {
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
                std::cout << "Documentation for " << Main.options.printDocFor << std::endl;
                std::cout << "Categories: " << std::endl;

                std::string current = "";
                for (std::pair<std::string, TripleList> section : docs) {
                    current = section.first;
                    std::cout << "  " << Color::BOLDBLUE << current << Color::RESET << std::endl;
                }
                return 0;
            } else {
                docs = parseSclDoc(scaleFolder + "/Internal/Docs.scldoc");
                std::cout << "Scale Documentation" << std::endl;
                std::cout << "Categories: " << std::endl;

                std::string current;
                for (std::pair<std::string, TripleList> section : docs) {
                    current = section.first;
                    std::cout << "  " << Color::BOLDBLUE << current << Color::RESET << std::endl;
                }
                return 0;
            }
        }

        if (DocOps.info) {
            std::cout << "Scale Documentation" << std::endl;
            docs = parseSclDoc(scaleFolder + "/Internal/Docs.scldoc");

            std::string current;
            if (DocOps.find.size() == 0) {
                for (std::pair<std::string, TripleList> section : docs) {
                    current = section.first;
                    std::cout << Color::BOLDBLUE << current << ":" << Color::RESET << std::endl;
                    for (Triple kv : section.second) {
                        if (kv.third.size())
                            std::cout << Color::BLUE << kv.first << Color::CYAN << "\nModule: " << kv.third << "\n" << Color::RESET << Color::GREEN << kv.second << std::endl;
                        else
                            std::cout << Color::BLUE << kv.first << "\n" << Color::RESET << Color::GREEN << kv.second << std::endl;
                    }
                }
            } else {
                for (std::string f : DocOps.find) {
                    bool found = false;
                    std::string sec = "";
                    bool hasSection = false;
                    std::cout << Color::RESET << "Searching for '" + f + "'" << std::endl;
                    for (std::pair<std::string, TripleList> section : docs) {
                        current = section.first;
                        for (Triple kv : section.second) {
                            std::string matchCurrent = current;
                            std::string matchKey = kv.first;
                            std::string matchDescription = kv.second;
                            if (std::regex_search(matchCurrent.begin(), matchCurrent.end(), std::regex(f, std::regex_constants::icase)) || std::regex_search(matchKey.begin(),matchKey.end(), std::regex(f, std::regex_constants::icase))) {
                                found = true;
                                sec = current;
                                hasSection = true;
                            }
                        }
                        if (!found) {
                            for (Triple kv : section.second) {
                                std::string matchCurrent = current;
                                std::string matchKey = kv.first;
                                std::string matchDescription = kv.second;
                                if (std::regex_search(matchDescription.begin(), matchDescription.end(), std::regex(f, std::regex_constants::icase))) {
                                    found = true;
                                    sec = current;
                                }
                            }
                        }
                    }
                    for (std::pair<std::string, TripleList> section : docs) {
                        current = section.first;
                        if (found && current == sec)
                            std::cout << Color::BOLDBLUE << current << ":" << Color::RESET << std::endl;
                        for (Triple kv : section.second) {
                            std::string matchCurrent = current;
                            std::string matchKey = kv.first;
                            std::string matchDescription = kv.second;
                            if (std::regex_search(matchCurrent.begin(), matchCurrent.end(), std::regex(f, std::regex_constants::icase)) || std::regex_search(matchKey.begin(),matchKey.end(), std::regex(f, std::regex_constants::icase))) {
                                if (kv.third.size())
                                    std::cout << Color::BLUE << kv.first << Color::CYAN << "\nModule: " << kv.third << "\n" << Color::RESET << Color::GREEN << kv.second << std::endl;
                                else
                                    std::cout << Color::BLUE << kv.first << "\n" << Color::RESET << Color::GREEN << kv.second << std::endl;
                            } else if (!hasSection && std::regex_search(matchDescription.begin(), matchDescription.end(), std::regex(f, std::regex_constants::icase))) {
                                std::string s = kv.second;
                                std::smatch matches;
                                std::regex_search(s, matches, std::regex(f, std::regex_constants::icase));
                                if (kv.third.size())
                                    std::cout << Color::BLUE << kv.first << Color::CYAN << "\nModule: " << kv.third << "\n" << Color::RESET << Color::GREEN << s << std::endl;
                                else
                                    std::cout << Color::BLUE << kv.first << "\n" << Color::RESET << Color::GREEN << s << std::endl;
                            }
                        }
                    }
                    if (!found) {
                        std::cout << Color::RED << "Could not find '" << f << "'" << std::endl;
                    }
                }
            }
            return 0;
        }

        if (Main.options.printDocFor.size()) {
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
            std::cout << "Documentation for " << Main.options.printDocFor << std::endl;

            std::string current = "";
            if (DocOps.find.size() == 0) {
                if (DocOps.find_category.size() == 0) {
                    for (std::pair<std::string, TripleList> section : docs) {
                        current = section.first;
                        std::cout << Color::BOLDBLUE << current << ":" << Color::RESET << std::endl;
                        for (Triple kv : section.second) {
                            if (kv.third.size())
                                std::cout << Color::BLUE << kv.first << Color::CYAN << "\nModule: " << kv.third << "\n" << Color::RESET << Color::GREEN << kv.second << std::endl;
                            else
                                std::cout << Color::BLUE << kv.first << "\n" << Color::RESET << Color::GREEN << kv.second << std::endl;
                        }
                    }
                } else {
                    for (std::string f : DocOps.find_category) {
                        bool found = false;
                        std::string sec = "";
                        std::cout << Color::RESET << "Searching for '" + f + "'" << std::endl;
                        for (std::pair<std::string, TripleList> section : docs) {
                            current = section.first;
                            std::string matchCurrent = current;
                            if (std::regex_search(matchCurrent.begin(), matchCurrent.end(), std::regex(f, std::regex_constants::icase))) {
                                found = true;
                                sec = current;
                            }
                        }
                        for (std::pair<std::string, TripleList> section : docs) {
                            current = section.first;
                            if (found && current == sec) {
                                std::cout << Color::BOLDBLUE << current << ":" << Color::RESET << std::endl;
                                for (Triple kv : section.second) {
                                    if (kv.third.size())
                                        std::cout << Color::BLUE << kv.first << Color::CYAN << "\nModule: " << kv.third << "\n" << Color::RESET << Color::GREEN << kv.second << std::endl;
                                    else
                                        std::cout << Color::BLUE << kv.first << "\n" << Color::RESET << Color::GREEN << kv.second << std::endl;
                                }
                                break;
                            }
                        }
                        if (!found) {
                            std::cout << Color::RED << "Could not find category '" << f << "'" << std::endl;
                        }
                    }
                }
            } else {
                for (std::string f : DocOps.find) {
                    bool found = false;
                    std::string sec = "";
                    bool hasSection = false;
                    std::cout << Color::RESET << "Searching for '" + f + "'" << std::endl;
                    for (std::pair<std::string, TripleList> section : docs) {
                        current = section.first;
                        for (Triple kv : section.second) {
                            std::string matchCurrent = current;
                            std::string matchKey = kv.first;
                            std::string matchDescription = kv.second;
                            if (std::regex_search(matchCurrent.begin(), matchCurrent.end(), std::regex(f, std::regex_constants::icase)) || std::regex_search(matchKey.begin(),matchKey.end(), std::regex(f, std::regex_constants::icase))) {
                                found = true;
                                sec = current;
                                hasSection = true;
                            }
                        }
                        if (!found) {
                            for (Triple kv : section.second) {
                                std::string matchCurrent = current;
                                std::string matchKey = kv.first;
                                std::string matchDescription = kv.second;
                                if (std::regex_search(matchDescription.begin(), matchDescription.end(), std::regex(f, std::regex_constants::icase))) {
                                    found = true;
                                    sec = current;
                                }
                            }
                        }
                    }
                    for (std::pair<std::string, TripleList> section : docs) {
                        current = section.first;
                        if (found && current == sec)
                            std::cout << Color::BOLDBLUE << current << ":" << Color::RESET << std::endl;
                        for (Triple kv : section.second) {
                            std::string matchCurrent = current;
                            std::string matchKey = kv.first;
                            std::string matchDescription = kv.second;
                            if (std::regex_search(matchCurrent.begin(), matchCurrent.end(), std::regex(f, std::regex_constants::icase)) || std::regex_search(matchKey.begin(),matchKey.end(), std::regex(f, std::regex_constants::icase))) {
                                if (kv.third.size())
                                        std::cout << Color::BLUE << kv.first << Color::CYAN << "\nModule: " << kv.third << "\n" << Color::RESET << Color::GREEN << kv.second << std::endl;
                                    else
                                        std::cout << Color::BLUE << kv.first << "\n" << Color::RESET << Color::GREEN << kv.second << std::endl;
                            } else if (!hasSection && std::regex_search(matchDescription.begin(), matchDescription.end(), std::regex(f, std::regex_constants::icase))) {
                                std::string s = kv.second;
                                std::smatch matches;
                                std::regex_search(s, matches, std::regex(f, std::regex_constants::icase));
                                std::cout << Color::BLUE << kv.first << "\n" << Color::RESET << Color::GREEN << s << std::endl;
                                if (kv.third.size())
                                    std::cout << Color::BLUE << kv.first << Color::CYAN << "\nModule: " << kv.third << "\n" << Color::RESET << Color::GREEN << s << std::endl;
                                else
                                    std::cout << Color::BLUE << kv.first << "\n" << Color::RESET << Color::GREEN << s << std::endl;
                            }
                        }
                    }
                    if (!found) {
                        std::cout << Color::RED << "Could not find '" << f << "'" << std::endl;
                    }
                }
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

        framework->addString("version", "23.5");
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

    int main(std::vector<std::string> args) {
        if (args.size() < 2) {
            usage(args[0]);
            return 1;
        }

        using clock = std::chrono::high_resolution_clock;

        auto start = clock::now();

        std::string outfile     = std::string(DEFAULT_OUTFILE);
        std::string compiler    = std::string(COMPILER);
        scaleFolder             = std::string(SCL_ROOT_DIR) + "/" + std::string(SCALE_INSTALL_DIR) + "/" + std::string(VERSION);
        std::vector<std::string> frameworks;
        std::vector<std::string> tmpFlags;
        std::string optimizer   = "O2";
        bool hasCppFiles        = false;
        srand(time(NULL));
        Main.options.operatorRandomData = gen_random();

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
            if (strends(std::string(args[i]), ".scale")) {
                if (!fileExists(args[i])) {
                    continue;
                }
                if (std::find(Main.options.files.begin(), Main.options.files.end(), args[i]) == Main.options.files.end())
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
                } else if (args[i] == "--no-main" || args[i] == "-no-main") {
                    Main.options.noMain = true;
                    tmpFlags.push_back("-DSCL_COMPILER_NO_MAIN");
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
                } else if (args[i] == "-cflags") {
                    Main.options.printCflags = true;
                } else if (args[i] == "--dump-parsed-data") {
                    Main.options.dumpInfo = true;
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
                } else if (args[i] == "-doc-for") {
                    if (i + 1 < args.size()) {
                        Main.options.printDocFor = args[i + 1];
                        i++;
                        Main.options.docPrinterArgsStart = i;
                    } else {
                        std::cerr << "Error: -doc-for requires an argument" << std::endl;
                        return 1;
                    }
                } else if (args[i] == "-doc") {
                    Main.options.printDocs = true;
                    Main.options.docPrinterArgsStart = i;
                    break;
                } else if (args[i] == "-no-error-location") {
                    Main.options.noErrorLocation = true;
                } else {
                    if (args[i] == "-c")
                        Main.options.dontSpecifyOutFile = true;
                    if (args[i].size() >= 2 && args[i][0] == '-' && args[i][1] == 'O')
                        optimizer = std::string(args[i].c_str() + 1);
                    if (args[i].size() >= 2 && args[i][0] == '-' && args[i][1] == 'I')
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

        if (Main.options.files.size() != 0) {
            cflags.push_back("-I" + scaleFolder + "/Internal");
            cflags.push_back("-I" + scaleFolder + "/Frameworks");
            cflags.push_back("-I.");
            cflags.push_back(scaleFolder + "/Internal/scale_runtime.c");
            cflags.push_back("-" + optimizer);
            cflags.push_back("-DVERSION=\"" + std::string(VERSION) + "\"");
            cflags.push_back("-DSCL_DEFAULT_STACK_FRAME_COUNT=" + std::to_string(Main.options.stackSize));
        }

        std::string source;
        if (Main.options.files.size() == 0 && Main.options.printDocFor.size() == 0 && !Main.options.printDocs) {
            goto actAsCCompiler;
        }
        
        {
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
                    Main.options.mapFrameworkConfigs[framework] = root;
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
                    Main.options.mapFrameworkIncludeFolders[framework] = headerDir;
                    
                    DragonConfig::StringEntry* implDirTag = root->getString("implDir");
                    std::string implDir = implDirTag == nullptr ? "" : implDirTag->getValue();
                    
                    DragonConfig::StringEntry* implHeaderDirTag = root->getString("implHeaderDir");
                    std::string implHeaderDir = implHeaderDirTag == nullptr ? "" : implHeaderDirTag->getValue();

                    DragonConfig::StringEntry* docfileTag = root->getString("docfile");
                    Main.options.mapFrameworkDocfiles[framework] = docfileTag == nullptr ? "" : scaleFolder + "/Frameworks/" + framework + ".framework/" + docfileTag->getValue();

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
                        std::string depend = depends->getString(i)->getValue();
                        if (!contains(frameworks, depend)) {
                            std::cerr << "Error: Framework '" << framework << "' depends on '" << depend << "' but it is not included" << std::endl;
                            return 1;
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
                        Main.options.includePaths.push_back(scaleFolder + "/Frameworks/" + framework + ".framework/" + headerDir);
                        Main.options.mapIncludePathsToFrameworks[framework] = scaleFolder + "/Frameworks/" + framework + ".framework/" + headerDir;
                    }
                    unsigned long implementersSize = implementers == nullptr ? 0 : implementers->size();
                    for (unsigned long i = 0; i < implementersSize; i++) {
                        std::string implementer = implementers->getString(i)->getValue();
                        if (!Main.options.assembleOnly) {
                            if (!hasCppFiles && (strends(implementer, ".cpp") || strends(implementer, ".c++"))) {
                                hasCppFiles = true;
                            }
                            tmpFlags.push_back(scaleFolder + "/Frameworks/" + framework + ".framework/" + implDir + "/" + implementer);
                        }
                    }
                    for (unsigned long i = 0; implHeaders != nullptr && i < implHeaders->size() && implHeaderDir.size() > 0; i++) {
                        std::string header = framework + ".framework/" + implHeaderDir + "/" + implHeaders->getString(i)->getValue();
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
                    Main.options.mapFrameworkConfigs[framework] = root;
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
                    Main.options.mapFrameworkIncludeFolders[framework] = headerDir;
                    
                    DragonConfig::StringEntry* implDirTag = root->getString("implDir");
                    std::string implDir = implDirTag == nullptr ? "" : implDirTag->getValue();
                    
                    DragonConfig::StringEntry* implHeaderDirTag = root->getString("implHeaderDir");
                    std::string implHeaderDir = implHeaderDirTag == nullptr ? "" : implHeaderDirTag->getValue();

                    DragonConfig::StringEntry* docfileTag = root->getString("docfile");
                    Main.options.mapFrameworkDocfiles[framework] = docfileTag == nullptr ? "" : "./" + framework + ".framework/" + docfileTag->getValue();
                    
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
                        std::string depend = depends->getString(i)->getValue();
                        if (!contains(frameworks, depend)) {
                            std::cerr << "Error: Framework '" << framework << "' depends on '" << depend << "' but it is not included" << std::endl;
                            return 1;
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
                        Main.options.includePaths.push_back("./" + framework + ".framework/" + headerDir);
                        Main.options.mapIncludePathsToFrameworks[framework] = "./" + framework + ".framework/" + headerDir;
                    }
                    unsigned long implementersSize = implementers == nullptr ? 0 : implementers->size();
                    for (unsigned long i = 0; i < implementersSize && implDir.size() > 0; i++) {
                        std::string implementer = implementers->getString(i)->getValue();
                        if (!Main.options.assembleOnly) {
                            if (!hasCppFiles && (strends(implementer, ".cpp") || strends(implementer, ".c++"))) {
                                hasCppFiles = true;
                            }
                            tmpFlags.push_back("./" + framework + ".framework/" + implDir + "/" + implementer);
                        }
                    }
                    for (unsigned long i = 0; implHeaders != nullptr && i < implHeaders->size() && implHeaderDir.size() > 0; i++) {
                        std::string header = framework + ".framework/" + implHeaderDir + "/" + implHeaders->getString(i)->getValue();
                        Main.frameworkNativeHeaders.push_back(header);
                    }
                }
            }
            Main.options.includePaths.push_back("./");
            if (!Main.options.noScaleFramework) {
                auto findModule = [&](const std::string moduleName) -> std::optional<std::vector<std::string>> {
                    std::vector<std::string> mods;
                    for (auto config : Main.options.mapFrameworkConfigs) {
                        auto modules = config.second->getCompound("modules");
                        if (modules) {
                            auto list = modules->getList(moduleName);
                            if (list) {
                                for (size_t j = 0; j < list->size(); j++) {
                                    FPResult find = findFileInIncludePath(list->getString(j)->getValue());
                                    if (!find.success) {
                                        return std::nullopt;
                                    }
                                    auto file = find.in;
                                    mods.push_back(file);
                                }
                                break;
                            }
                        }
                    }
                    return std::make_optional(mods);
                };

                auto mod = findModule("std");
                if (mod) {
                    for (auto file : *mod) {
                        if (std::find(Main.options.files.begin(), Main.options.files.end(), file) == Main.options.files.end()) {
                            Main.options.files.push_back(file);
                        }
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
                path s = Main.options.files.at(i);
                if (s.parent_path().string().size())
                    Main.options.includePaths.push_back(s.parent_path().string());
            }

            for (size_t i = 0; i < Main.options.files.size() && !Main.options.printCflags; i++) {
                std::string filename = Main.options.files[i];

                if (!std::filesystem::exists(filename)) {
                    std::cout << Color::BOLDRED << "Fatal Error: File " << filename << " does not exist!" << Color::RESET << std::endl;
                    return 1;
                }
                if (Main.options.debugBuild)
                    std::cout << "Tokenizing file '" << filename << "'" << std::endl;

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

                remove((filename + ".c").c_str());
                remove((filename + ".h").c_str());
                remove((filename + ".typeinfo.h").c_str());
            }

            if (Main.options.preprocessOnly) {
                std::cout << "Preprocessed " << Main.options.files.size() << " files." << std::endl;
                return 0;
            }

            TPResult result;
            if (!Main.options.printCflags) {
                SyntaxTree lexer(tokens);
                Main.lexer = &lexer;
                result = Main.lexer->parse();
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
            if (!Main.options.printCflags) {
                Parser parser(result);
                Main.parser = &parser;

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
                    remove(source.c_str());
                    remove((source.substr(0, source.size() - 2) + ".h").c_str());
                    remove((source.substr(0, source.size() - 2) + ".typeinfo.h").c_str());
                    std::cout << "Failed!" << std::endl;
                    return parseResult.errors.size();
                }
            }

            if (Main.options.transpileOnly) {
                auto end = clock::now();
                double duration = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000000000.0;
                std::cout << "Transpiled successfully in " << duration << " seconds." << std::endl;
                return 0;
            }
        }

        if (!hasCppFiles) {
            // -std= only works when there are no c++ files to build
            cflags.push_back("-std=" + std::string(C_VERSION));
        } else {
            // link to c++ library if needed
            cflags.push_back("-lc++");
        }

        if (!Main.options.printCflags && !Main.options.dontSpecifyOutFile) {
            cflags.push_back("-o");
            cflags.push_back("\"" + outfile + "\"");
        }

        if (Main.options.mainReturnsNone) {
            tmpFlags.push_back("-DSCL_MAIN_RETURN_NONE");
        }
        tmpFlags.push_back("-DSCL_MAIN_ARG_COUNT=" + std::to_string(Main.options.mainArgCount));

    actAsCCompiler:

        for (std::string s : tmpFlags) {
            cflags.push_back(s);
        }
        
        if (source.size())
            cflags.push_back(source);

#ifdef LINK_MATH
        cflags.push_back("-lm");
#endif

        std::string cmd = "";
        for (std::string s : cflags) {
            cmd += s + " ";
        }
        if (Main.options.debugBuild) {
            cmd += "-DSCL_DEBUG";
        }

        if (Main.options.printCflags) {
            std::cout << cmd << std::endl;
            return 0;
        } else if (!Main.options.transpileOnly) {
            int ret = system(cmd.c_str());
            remove(source.c_str());
            remove((source.substr(0, source.size() - 2) + ".h").c_str());
            remove((source.substr(0, source.size() - 2) + ".typeinfo.h").c_str());
            remove("scale_support.h");
            if (ret) {
                std::cerr << Color::RED << "Compilation failed with code " << ret << Color::RESET << std::endl;
                return ret;
            }
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
