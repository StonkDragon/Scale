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
    std::cout << cmd << std::endl;
    int x = std::system(cmd.c_str());
    if (x) std::exit(x);
}

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

    bool isDevBuild = false;
    bool fullRebuild = false;
    bool debug = false;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-dev") == 0) {
            isDevBuild = true;
        } else if (std::strcmp(argv[i], "-noinc") == 0) {
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

    std::string path = install_root_dir + DIR_SEP "Scale" DIR_SEP + TO_STRING(VERSION);
    std::string binary = "sclc" EXE_SUFF;

    std::cout << "Installing Scale to " << path << std::endl;

    if (!isDevBuild) {
        fs::remove_all(path);
    } else {
        fs::remove_all(path + DIR_SEP "Frameworks");
        fs::remove_all(path + DIR_SEP "Internal" DIR_SEP LIB_SCALE_FILENAME);
    }
    
    fs::create_directories(path);

    fs::copy("Scale", path, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    if (fs::exists("bdwgc")) {
        fs::remove_all("bdwgc");
    }
    exec_command(create_command({"git", "clone", "--depth=1", "https://github.com/ivmai/bdwgc.git", "bdwgc"}));
    
    fs::current_path("bdwgc");
    
    #if defined(_WIN32)
    exec_command(create_command({"git", "clone", "--depth=1", "https://github.com/ivmai/libatomic_ops.git", "libatomic_ops"}));
    #endif

    exec_command(create_command({
        CC,
        "-c",
        "-I",
        "include",
        "-O3",
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
        "extra" DIR_SEP "gc.c",
        "-o",
        path + DIR_SEP "Internal" DIR_SEP "gc.o"
    }));
    fs::create_directories(path + DIR_SEP "Internal" DIR_SEP "include");
    fs::create_directories(path + DIR_SEP "Internal" DIR_SEP "lib");
    fs::copy("include", path + DIR_SEP "Internal" DIR_SEP "include", fs::copy_options::overwrite_existing | fs::copy_options::recursive);

    fs::current_path(fs::current_path().parent_path());
    fs::remove_all("bdwgc");

    auto scale_runtime = create_command({
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
        path + DIR_SEP "Internal" DIR_SEP "scale_runtime.o"
    });
    auto cxx_glue = create_command({
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
        path + DIR_SEP "Internal" DIR_SEP "scale_cxx.o"
    });

    auto library = create_command({
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
        path + DIR_SEP "Internal" DIR_SEP "lib" DIR_SEP "" LIB_SCALE_FILENAME
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
        "-I" + path + DIR_SEP "Internal" DIR_SEP "include",
    #if defined(_WIN32)
        "-D_CRT_SECURE_NO_WARNINGS",
    #endif
    });

    if (debug) {
        compile_command += "-O0 -g -DDEBUG ";
        cxx_glue += "-O0 -g -DDEBUG ";
        scale_runtime += "-O0 -g -DDEBUG ";
        library += "-O0 -g -DDEBUG ";
    } else {
        compile_command += "-O2 ";
        cxx_glue += "-O2 ";
        scale_runtime += "-O2 ";
        library += "-O2 ";
    }

    auto source_files = listFiles("Compiler", ".cpp");
    auto builders = std::vector<std::thread>(source_files.size());
    
    std::vector<std::string> link_command = {
        compile_command,
    #if defined(_WIN32)
        "-Wl,-export-all-symbols",
        "-lUser32",
        "-lDbgHelp",
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

        fs::file_time_type last_write = fs::file_time_type::min();
        fs::file_time_type last_write_obj = fs::file_time_type::min();
        
        if (fullRebuild || !fs::exists(f.string() + ".o") || fs::last_write_time(f) > fs::last_write_time(f.string() + ".o")) {
            builders.push_back(std::thread(
                [cmd]() { exec_command(cmd); }
            ));
        }
    }

    link_command.push_back("-o");
    link_command.push_back(std::string(path) + DIR_SEP + binary);

    for (auto&& x : builders) {
        if (x.joinable())
            x.join();
    }

    fs::remove(path + DIR_SEP "Internal" DIR_SEP "lib" DIR_SEP "" LIB_SCALE_FILENAME);
    
    exec_command(scale_runtime);
    exec_command(cxx_glue);
    exec_command(library);

    exec_command(create_command(link_command));

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

    if (!fs::exists(symlinked_path.parent_path())) {
        fs::create_directories(symlinked_path.parent_path());
    } else {
        fs::remove(symlinked_path);
    }

#if defined(_WIN32)
    if (CreateSymbolicLinkA(symlinked_path.string().c_str(), (path + DIR_SEP + binary).c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) == 0) {
        unsigned int err = GetLastError();
        if (err == ERROR_PRIVILEGE_NOT_HELD) {
            std::cout << "[INFO]: Unprivileged users cannot create symlinks in windows. Failed to create link from " << fs::path(path + DIR_SEP + binary).make_preferred() << " to " << symlinked_path << std::endl;
        } else {
            std::cerr << "Error creating symlink: " << GetLastError() << std::endl;
            std::exit(1);
        }
    };
#else
    try {
        fs::create_symlink(path + DIR_SEP + binary, symlinked_path);
    } catch (fs::filesystem_error& err) {
        std::cerr << "Error creating symlink: " << err.what() << std::endl;
        std::exit(1);
    }
#endif

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
        std::cerr << "Unexpected filesystem error: " << fs.what() << std::endl;
    }
}
