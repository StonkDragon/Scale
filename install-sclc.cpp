#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <thread>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>

#if defined(_WIN32)
#include <windows.h>
#endif

#define TO_STRING(x) TO_STRING_(x)
#define TO_STRING_(x) #x

#define CONCAT(a, b) CONCAT_(a, b)
#define CONCAT_(a, b) a ## b

#include "Configuration.h"

#if defined(__APPLE__)
#define LIB_PREF "lib"
#define LIB_SUFF ".dylib"
#define EXE_SUFF ""
#elif defined(__linux__)
#define LIB_PREF "lib"
#define LIB_SUFF ".so"
#define EXE_SUFF ""
#elif defined(_WIN32)
#define LIB_PREF ""
#define LIB_SUFF ".dll"
#define EXE_SUFF ".exe"
#endif

#define LIB_SCALE_FILENAME    LIB_PREF "ScaleRuntime" LIB_SUFF

#if defined(_WIN32)
#define DIR_SEP "\\"
#else
#define DIR_SEP "/"
#endif

namespace fs = std::filesystem;

class Color {
public:
    static std::string RESET;
    static std::string BLACK;
    static std::string RED;
    static std::string GREEN;
    static std::string YELLOW;
    static std::string BLUE;
    static std::string MAGENTA;
    static std::string CYAN;
    static std::string WHITE;
    static std::string GRAY;
    static std::string BOLDBLACK;
    static std::string BOLDRED;
    static std::string BOLDGREEN;
    static std::string BOLDYELLOW;
    static std::string BOLDBLUE;
    static std::string BOLDMAGENTA;
    static std::string BOLDCYAN;
    static std::string BOLDWHITE;
    static std::string BOLDGRAY;
    static std::string UNDERLINE;
    static std::string BOLD;
    static std::string REVERSE;
    static std::string HIDDEN;
    static std::string ITALIC;
    static std::string STRIKETHROUGH;
};

#ifdef _WIN32
#define COLOR(NAME, WHAT) std::string Color::NAME = ""
#else
#define COLOR(NAME, WHAT) std::string Color::NAME = WHAT
#endif

COLOR(RESET, "\033[0m");
COLOR(BLACK, "\033[30m");
COLOR(RED, "\033[31m");
COLOR(GREEN, "\033[32m");
COLOR(YELLOW, "\033[33m");
COLOR(BLUE, "\033[34m");
COLOR(MAGENTA, "\033[35m");
COLOR(CYAN, "\033[36m");
COLOR(WHITE, "\033[37m");
COLOR(GRAY, "\033[30m");
COLOR(BOLDBLACK, "\033[1m\033[30m");
COLOR(BOLDRED, "\033[1m\033[31m");
COLOR(BOLDGREEN, "\033[1m\033[32m");
COLOR(BOLDYELLOW, "\033[1m\033[33m");
COLOR(BOLDBLUE, "\033[1m\033[34m");
COLOR(BOLDMAGENTA, "\033[1m\033[35m");
COLOR(BOLDCYAN, "\033[1m\033[36m");
COLOR(BOLDWHITE, "\033[1m\033[37m");
COLOR(BOLDGRAY, "\033[1m\033[30m");
COLOR(UNDERLINE, "\033[4m");
COLOR(BOLD, "\033[1m");
COLOR(REVERSE, "\033[7m");
COLOR(HIDDEN, "\033[8m");
COLOR(ITALIC, "\033[3m");
COLOR(STRIKETHROUGH, "\033[9m");

#define ERASE_LINE "\033[2K\r"

void depends_on(std::string command, const char* error) {
#if defined(_WIN32)
    int result = std::system((command + " > nul 2>&1").c_str());
#else
    int result = std::system((command + " > /dev/null 2>&1").c_str());
#endif
    if (result != 0) {
        std::cerr << error << std::endl;
        exit(1);
    }
}

std::string create_command(std::vector<std::string> args) {
    std::string out = "";
    for (auto a : args) {
        out += a + " ";
    }
    return out;
}

std::vector<fs::path> listFiles(const fs::path& dir, std::string ext) {
    std::vector<fs::path> files;
    for (auto child : fs::directory_iterator(dir)) {
        if (child.path().extension() == ext) {
            files.push_back(child.path());
        } else if (child.is_directory()) {
            auto x = listFiles(child.path(), ext);
            files.insert(files.end(), x.begin(), x.end());
        }
        
    }
    return files;
}

void exec_command(std::string cmd) {
    // std::cout << cmd << std::endl;
    int x = std::system(cmd.c_str());
    if (x) std::exit(x);
}

std::thread async(std::function<void()> f) {
    return std::thread(f);
}

std::thread exec_command_async(std::string cmd) {
    return async([cmd]() {
        exec_command(cmd);
    });
}

#define await(t) ({ std::cout << ERASE_LINE << Color::YELLOW << "Waiting on " #t << Color::RESET; std::cout.flush(); if (t.joinable()) t.join(); })

void go_rebuild_yourself(int argc, char const *argv[]) {
    const char* source_file = __FILE__;
    const char* binary_file = argv[0];
    if (fs::last_write_time(source_file) > fs::last_write_time(binary_file)) {
        auto cmd = create_command({
        CXX, "-o", binary_file, source_file, "-std=" CXX_VERSION,
    #if defined(_WIN32)
        "-lAdvapi32"
    #endif
        });
        exec_command(cmd);
        std::vector<std::string> command(argv, argv + argc);
        command.push_back("-noinc");
        exec_command(create_command(command));
        std::exit(0);
    }
}

int real_main(int argc, char const *argv[]) {
    go_rebuild_yourself(argc, argv);

    depends_on(CC " --version", CC " is required!");
    depends_on(CXX " --version", CXX " is required!");

    bool fullRebuild = false;
    bool debug = false;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-noinc") == 0) {
            fullRebuild = true;
        } else if (std::strcmp(argv[i], "-debug") == 0) {
            debug = true;
        }
    }

#if defined(_WIN32)
    char* user_home_dir = std::getenv("APPDATA");
#else
    char* user_home_dir = std::getenv("HOME");
#endif
    if (user_home_dir == nullptr) {
        std::cerr << "could not get home directory!" << std::endl;
    }

    std::string install_root_dir = user_home_dir;

#if defined(__linux__)
    install_root_dir += "/.local";
#endif

    const std::string path = install_root_dir + DIR_SEP "Scale" DIR_SEP + TO_STRING(VERSION);
    const std::string binary = "sclc" EXE_SUFF;

    std::cout << Color::GREEN << "Installing Scale to " << path << Color::RESET << std::endl;

    fs::remove_all(path);
    fs::create_directories(path);
    fs::create_directories(path + DIR_SEP "Internal" DIR_SEP "include");
    fs::create_directories(path + DIR_SEP "Internal" DIR_SEP "lib");

    fs::copy("Scale", path, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    if (fs::exists("bdwgc")) {
        fs::remove_all("bdwgc");
    }

    exec_command(create_command({"git", "clone", "--depth=1", "https://github.com/ivmai/bdwgc.git", "bdwgc"}));
    #if defined(_WIN32)
    exec_command(create_command({"git", "clone", "--depth=1", "https://github.com/ivmai/libatomic_ops.git", "libatomic_ops"}));
    #endif

    fs::copy("bdwgc" DIR_SEP "include", path + DIR_SEP "Internal" DIR_SEP "include", fs::copy_options::overwrite_existing | fs::copy_options::recursive);
    
    auto gc_lib_cmd = async([path, debug]() {
        exec_command(create_command({
            CC,
            "-c",
            "-I",
            "bdwgc" DIR_SEP "include",
            debug ? "-O0 -g -DDEBUG" : "-O2",
        #if !defined(_WIN32)
            "-fPIC",
        #else
            "-D_CRT_SECURE_NO_WARNINGS",
        #endif
            "-DGC_DLL",
            "-DGC_THREADS",
            "-DALL_INTERIOR_POINTERS",
            "-DNO_EXECUTE_PERMISSION",
            "-DPARALLEL_MARK",
            "-DGC_BUILTIN_ATOMIC",
            "-DDONT_USE_USER32_DLL",
            "-fno-strict-aliasing",
            "-march=native",
            "-Wall",
            "bdwgc" DIR_SEP "extra" DIR_SEP "gc.c",
            "-o",
            path + DIR_SEP "Internal" DIR_SEP "gc.o"
        }));
        fs::remove_all("bdwgc");
    });

    #if defined(_WIN32)
    auto escape_backslashes = [](std::string s) -> std::string {
        size_t size = 0;
        for (size_t i = 0; i < s.size(); i++) {
            size += 1 + (s[i] == '\\');
        }
        std::string out;
        out.reserve(size + 1);
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == '\\') {
                out.push_back('\\');
            }
            out.push_back(s[i]);
        }
        return out;
    };

    install_root_dir = escape_backslashes(install_root_dir);
    #endif

    std::string compile_command = create_command({
        CXX,
        "-fvisibility=default",
        ("-DVERSION=\\\"" TO_STRING(VERSION) "\\\""),
        ("-DC_VERSION=" TO_STRING(TO_STRING(C_VERSION))),
        "-DSCALE_ROOT_DIR=\\\"" + install_root_dir + "\\\"",
        ("-std=" CXX_VERSION),
        "-Wall",
        "-Wextra",
        "-Werror",
        "-ICompiler" DIR_SEP "headers",
    #if defined(_WIN32)
        "-D_CRT_SECURE_NO_WARNINGS",
    #endif
        debug ? "-O0 -g -DDEBUG" : "-O2",
    });
    
    auto source_files = listFiles("Compiler", ".cpp");
    auto builders = std::unordered_map<std::string, std::thread>(source_files.size());
    
    std::vector<std::string> link_command = {
        compile_command,
    #if defined(_WIN32)
        "-Wl,-export-all-symbols",
        "-fuse-ld=lld",
        "-Wl,-lldmingw",
    #elif defined(__APPLE__)
        "-Wl,-export_dynamic",
    #elif defined(__linux__)
        "-rdynamic",
    #endif
    };

    for (auto f : source_files) {
        std::string cmd = create_command({
            compile_command,
            "-o",
            f.string() + ".o",
            "-c",
            f.string()
        });
        link_command.push_back(f.string() + ".o");
        
        #if defined(min)
        #undef min
        #endif

        if (fullRebuild || !fs::exists(f.string() + ".o") || fs::last_write_time(f) > fs::last_write_time(f.string() + ".o")) {
            builders[f] = exec_command_async(cmd);
        }
    }

    link_command.push_back("-o");
    link_command.push_back(std::string(path) + DIR_SEP + binary);

    fs::remove(path + DIR_SEP "Internal" DIR_SEP "lib" DIR_SEP "" LIB_SCALE_FILENAME);
    
    auto runtime_cmd = exec_command_async(create_command({ // Compile runtime
        CC,
        "-fvisibility=default",
        ("-std=" C_VERSION),
        "-I" + path + DIR_SEP "Internal",
        "-I" + path + DIR_SEP "Internal" DIR_SEP "include",
        path + DIR_SEP "Internal" DIR_SEP "scale_runtime.c",
        "-c",
    #if !defined(_WIN32)
        "-fPIC",
    #else
        "-D_CRT_SECURE_NO_WARNINGS",
    #endif
        "-o",
        path + DIR_SEP "Internal" DIR_SEP "scale_runtime.o",
        debug ? "-O0 -g -DDEBUG" : "-O2",
    }));
    auto cxx_runtime_cmd = exec_command_async(create_command({ // Compile C++ runtime
        CXX,
        "-fvisibility=default",
        ("-std=" CXX_VERSION),
        "-I" + path + DIR_SEP "Internal",
        "-I" + path + DIR_SEP "Internal" DIR_SEP "include",
        path + DIR_SEP "Internal" DIR_SEP "scale_cxx.cpp",
        "-c",
    #if !defined(_WIN32)
        "-fPIC",
    #else
        "-D_CRT_SECURE_NO_WARNINGS",
    #endif
        "-o",
        path + DIR_SEP "Internal" DIR_SEP "scale_cxx.o",
        debug ? "-O0 -g -DDEBUG" : "-O2",
    }));
    await(runtime_cmd);
    await(cxx_runtime_cmd);
    await(gc_lib_cmd);
    exec_command(create_command({ // Link runtime
        CXX,
        "-fvisibility=default",
    #if defined(_WIN32)
        "-Wl,-export-all-symbols",
        "-fuse-ld=lld",
        "-Wl,-lldmingw",
        "-D_CRT_SECURE_NO_WARNINGS",
    #else
        "-fPIC",
        "-shared",
    #endif
    #if defined(__APPLE__)
        "-dynamiclib",
        "-current_version",
        (TO_STRING(VERSION)),
        "-compatibility_version",
        (TO_STRING(VERSION)),
        "-undefined",
        "dynamic_lookup",
    #elif defined(__linux__)
        "-Wl,--undefined,dynamic_lookup",
    #endif
        path + DIR_SEP "Internal" DIR_SEP "scale_runtime.o",
        path + DIR_SEP "Internal" DIR_SEP "scale_cxx.o",
        path + DIR_SEP "Internal" DIR_SEP "gc.o",
        "-o",
        path + DIR_SEP "Internal" DIR_SEP "lib" DIR_SEP "" LIB_SCALE_FILENAME,
        debug ? "-O0 -g -DDEBUG" : "-O2"
    }));

    fs::remove(path + DIR_SEP "Internal" DIR_SEP "scale_runtime.o");
    fs::remove(path + DIR_SEP "Internal" DIR_SEP "scale_cxx.o");
    fs::remove(path + DIR_SEP "Internal" DIR_SEP "gc.o");

    fs::path symlinked_path =
    #if defined(__linux__)
    install_root_dir + DIR_SEP "bin" DIR_SEP + binary;
    #elif defined(_WIN32)
    install_root_dir
    #else
    install_root_dir + DIR_SEP "bin" DIR_SEP + binary;
    #endif
    symlinked_path = symlinked_path.make_preferred();

    std::cout << ERASE_LINE << Color::BLUE << "Waiting for compilation to finish..." << Color::RESET << std::endl;
    size_t n = 0;
    for (auto&& p : builders) {
        if (p.second.joinable()) {
            p.second.join();
            n++;
            std::cout << ERASE_LINE << Color::GREEN << "[" << n << "/" << builders.size() << "] " << Color::RESET << p.first;
            std::cout.flush();
        }
    }
    exec_command(create_command(link_command));

    if (!fs::exists(symlinked_path.parent_path())) {
        fs::create_directories(symlinked_path.parent_path());
    } else {
        fs::remove(symlinked_path);
    }

#if defined(_WIN32)
    if (CreateSymbolicLinkA(symlinked_path.string().c_str(), (path + DIR_SEP + binary).c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) == 0) {
        unsigned int err = GetLastError();
        if (err == ERROR_PRIVILEGE_NOT_HELD) {
            std::cout << Color::RED << "[INFO]: Unprivileged users cannot create symlinks in windows. Failed to create link from " << fs::path(path + DIR_SEP + binary).make_preferred() << " to " << symlinked_path << Color::RESET << std::endl;
        } else {
            std::cerr << Color::RED << "Error creating symlink: " << GetLastError() << Color::RESET << std::endl;
            std::exit(1);
        }
    };
#else
    try {
        fs::create_symlink(path + DIR_SEP + binary, symlinked_path);
    } catch (fs::filesystem_error& err) {
        std::cerr << Color::RED << "Error creating symlink: " << err.what() << Color::RESET << std::endl;
        std::exit(1);
    }
#endif

    std::cout << ERASE_LINE << Color::GREEN << "Scale was successfully installed to " << path << Color::RESET << std::endl;

    #if defined(_WIN32)
    std::cout << "IMPORTANT!!!!!" << std::endl;
    std::cout << "For Scale to work properly, you need to modify the PATH environment variable like this:" << std::endl;
    std::cout << "1. Right-click the start button" << std::endl;
    std::cout << "2. Click 'System'" << std::endl;
    std::cout << "3. On the right side of the window click 'Advanced system settings'" << std::endl;
    std::cout << "4. Go to the tab 'Advanced'" << std::endl;
    std::cout << "5. At the bottom of the window click 'Environment variables...'" << std::endl;
    std::cout << "6. Select the row 'Path' and click 'Edit...'" << std::endl;
    std::cout << "7. Click 'New' and enter '" << path << "'" << std::endl;
    std::cout << "8. Click 'New' and enter '" << path << DIR_SEP "Internal'" << std::endl;
    #endif

    return 0;
}

int main(int argc, char const *argv[]) {
    try {
        return real_main(argc, argv);
    } catch (fs::filesystem_error& fs) {
        std::cerr << Color::RED << "Unexpected filesystem error: " << fs.what() << Color::RESET << std::endl;
    }
}
