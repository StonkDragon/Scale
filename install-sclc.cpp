#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <thread>
#include <shared_mutex>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#endif

void depends_on(std::string command, const char* error) {
#ifdef _WIN32
    int result = std::system((command + " > nul 2>&1").c_str());
#else
    int result = std::system((command + " > /dev/null 2>&1").c_str());
#endif
    if (result != 0) {
        std::cerr << error << std::endl;
        std::exit(1);
    }
}

#define STR_(x) #x
#define STR(x) STR_(x)

std::string create_command(std::vector<std::string> args) {
    std::string out = "";
    for (auto a : args) {
        out += a + " ";
    }
    return out;
}

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

void exec_command(std::string cmd) {
    std::cout << cmd << std::endl;
    int x = std::system(cmd.c_str());
    if (x) std::exit(x);
}

bool is_root() {
    int root = false;
#ifdef _WIN32
    void* adminSid = NULL;
    DWORD unused;

    if (CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, &adminSid, &unused)) {
        if (!CheckTokenMembership(NULL, adminSid, &root)) {
            root = false;
        }
    }
#else
    root = (getuid() == 0);
#endif
    return root;
}

void require_root() {
    if (!is_root()) {
        std::cerr << "This file must be run as root." << std::endl;
        exit(1);
    }
}

bool strcontains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

int main(int argc, char const *argv[]) {
    depends_on("clang --version", "clang is required!");
    depends_on("clang++ --version", "clang++ is required!");
// #if !defined(_WIN32)
//     depends_on("make --version", "make is required!");
// #else
//     depends_on("nmake /?", "nmake is required!");
// #endif

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

#define LIB_SCALE_FILENAME    LIB_PREF LIB_SCALE_NAME LIB_SUFF
#define SCALE_STDLIB_FILENAME LIB_PREF SCALE_STDLIB_NAME LIB_SUFF

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

#ifdef _WIN32
    char* user_home_dir = std::getenv("UserProfile");
#else
    char* user_home_dir = std::getenv("HOME");
#endif
    if (user_home_dir == nullptr) {
        std::cerr << "could not get home directory!" << std::endl;
    }
    std::string home = user_home_dir;

    #ifdef _WIN32
    #define ABS_ROOT_DIR "C:/"
    #else
    #define ABS_ROOT_DIR "/opt"
    #endif

    std::string scl_root_dir = is_root() ? ABS_ROOT_DIR : home;

    std::string path = scl_root_dir + "/Scale/" + STR(VERSION);
    std::string binary = "sclc" EXE_SUFF;

    std::cout << "Installing Scale to " << path << std::endl;

    if (!isDevBuild) {
        std::filesystem::remove_all(path);
    } else {
        std::filesystem::remove_all(path + "/Frameworks");
        std::filesystem::remove_all(path + "/Internal/" LIB_SCALE_FILENAME);
    }
    std::filesystem::create_directories(path);
    if (is_root()) {
        std::filesystem::permissions(
            path,
            std::filesystem::perms::all
        );
        std::filesystem::remove_all(scl_root_dir + "/Scale/latest");
        std::filesystem::create_directory_symlink(std::filesystem::path(path), scl_root_dir + "/Scale/latest");
        std::filesystem::permissions(
            scl_root_dir + "/Scale/latest",
            std::filesystem::perms::all
        );
    }

    std::filesystem::copy("Scale", path, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

    if (!isDevBuild) {
        if (std::filesystem::exists("bdwgc")) {
            std::filesystem::remove_all("bdwgc");
        }
        exec_command(create_command({"git", "clone", "--depth=1", "https://github.com/ivmai/bdwgc.git", "bdwgc"}));
        
        auto oldsighandler = signal(SIGINT, [](int sig){
            std::filesystem::current_path(std::filesystem::current_path().parent_path());
            std::filesystem::remove_all("bdwgc");
        });
        std::filesystem::current_path("bdwgc");
        exec_command(create_command({
            "clang",
            "-fvisibility=default",
        #ifdef _WIN32
            "-Wl,-export-all-symbols",
            "-fuse-ld=lld",
            "-Wl,-lldmingw",
            "-Wno-unused-command-line-argument",
        #endif
            "-Iinclude",
            #ifdef _WIN32
            "-D_CRT_SECURE_NO_WARNINGS",
            "-DGC_WIN32_THREADS",
            #else
            "-fPIC",
            #endif
            "-DGC_THREADS",
            "-DGC_BUILTIN_ATOMIC",
            "-DGC_DLL",
            #ifdef _WIN32
            "-DDONT_USE_USER32_DLL",
            #endif
            "-c",
            "-o",
            path + "/Internal/gc.o",
            "extra/gc.c"
        }));
        exec_command(create_command({
            "clang++",
            "-fvisibility=default",
        #ifdef _WIN32
            "-Wl,-export-all-symbols",
            "-fuse-ld=lld",
            "-Wl,-lldmingw",
            "-Wno-unused-command-line-argument",
        #endif
            "-Iinclude",
            #ifdef _WIN32
            "-D_CRT_SECURE_NO_WARNINGS",
            "-DGC_WIN32_THREADS",
            #else
            "-fPIC",
            #endif
            "-DGC_THREADS",
            "-DGC_BUILTIN_ATOMIC",
            "-DGC_DLL",
            #ifdef _WIN32
            "-DDONT_USE_USER32_DLL",
            #endif
            "-c",
            "-o",
            path + "/Internal/gc_cpp.o",
            "gc_cpp.cpp"
        }));
        exec_command(create_command({
            "clang++",
            "-fvisibility=default",
        #ifdef _WIN32
            "-Wl,-export-all-symbols",
            "-fuse-ld=lld",
            "-Wl,-lldmingw",
            "-Wno-unused-command-line-argument",
        #endif
            "-Iinclude",
            #ifdef _WIN32
            "-D_CRT_SECURE_NO_WARNINGS",
            "-DGC_WIN32_THREADS",
            #else
            "-fPIC",
            #endif
            "-DGC_THREADS",
            "-DGC_BUILTIN_ATOMIC",
            "-DGC_DLL",
            #ifdef _WIN32
            "-DDONT_USE_USER32_DLL",
            #endif
            "-c",
            "-o",
            path + "/Internal/gc_badalc.o",
            "gc_badalc.cpp"
        }));

        std::filesystem::create_directories(path + "/Internal/include");
        std::filesystem::copy("include", path + "/Internal/include", std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);

        std::filesystem::current_path(std::filesystem::current_path().parent_path());
        std::filesystem::remove_all("bdwgc");
    }

    auto scale_runtime = create_command({
        "clang",
        "-fvisibility=default",
    #ifdef _WIN32
        "-Wl,-export-all-symbols",
        "-fuse-ld=lld",
    #endif
        "-O2",
        ("-std=" C_VERSION),
        "-I" + path + "/Internal",
        "-I" + path + "/Internal/include",
        path + "/Internal/scale_runtime.c",
        "-c",
    #if !defined(_WIN32)
        "-fPIC",
    #else
        "-D_CRT_SECURE_NO_WARNINGS",
    #endif
        "-o",
        path + "/Internal/scale_runtime.o"
    });
    auto cxx_glue = create_command({
        "clang++",
        "-fvisibility=default",
    #ifdef _WIN32
        "-Wl,-export-all-symbols",
        "-fuse-ld=lld",
    #endif
        "-O2",
        ("-std=" CXX_VERSION),
        "-I" + path + "/Internal",
        "-I" + path + "/Internal/include",
        path + "/Internal/scale_cxx.cpp",
        "-c",
    #if !defined(_WIN32)
        "-fPIC",
    #else
        "-D_CRT_SECURE_NO_WARNINGS",
    #endif
        "-o",
        path + "/Internal/scale_cxx.o"
    });

    auto library = create_command({
        "clang++",
        "-fvisibility=default",
    #ifdef _WIN32
        "-Wl,-export-all-symbols",
        "-fuse-ld=lld",
    #endif
        "-O2",
    #if defined(__APPLE__)
        "-dynamiclib",
        "-current_version",
        (STR(VERSION)),
        "-compatibility_version",
        (STR(VERSION)),
        "-undefined",
        "dynamic_lookup",
    #else
    #ifdef __linux__
        "-Wl,--undefined,dynamic_lookup",
    #endif
        "-shared",
    #endif
        path + "/Internal/scale_runtime.o",
        path + "/Internal/scale_cxx.o",
        path + "/Internal/gc.o",
        path + "/Internal/gc_cpp.o",
        path + "/Internal/gc_badalc.o",
        "-o",
        path + "/Internal/" LIB_SCALE_FILENAME
    });

    #ifdef _WIN32
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

    scl_root_dir = escape_backslashes(scl_root_dir);
    #endif

    std::string compile_command = create_command({
        "clang++",
        "-fvisibility=default",
    #ifdef _WIN32
        "-Wl,-export-all-symbols",
        "-fuse-ld=lld",
    #endif
        ("-DVERSION=\\\"" STR(VERSION) "\\\""),
        ("-DC_VERSION=" STR(STR(C_VERSION))),
        "-DSCL_ROOT_DIR=\\\"" + scl_root_dir + "\\\"",
        ("-std=" CXX_VERSION),
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I" + path + "/Internal/include",
    #ifdef _WIN32
        "-D_CRT_SECURE_NO_WARNINGS",
    #endif
        "-DGC_NOT_DLL",
        "-Wno-inline-new-delete",
    });

    if (debug) {
        compile_command += "-O0 -g ";
    } else {
        compile_command += "-O2 ";
    }

    auto source_files = listFiles("Compiler", ".cpp");
    auto builders = std::vector<std::thread>(source_files.size());
    
    std::vector<std::string> link_command = {
        compile_command,
#ifdef __linux__
        "-Wl,-export-all-symbols",
#endif
#ifdef _WIN32
        "-lUser32",
        "-lDbgHelp",
#endif
        path + "/Internal/gc.o",
        path + "/Internal/gc_cpp.o",
        path + "/Internal/gc_badalc.o",
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
        
        #ifdef min
        #undef min
        #endif

        std::filesystem::file_time_type last_write = std::filesystem::file_time_type::min();
        std::filesystem::file_time_type last_write_obj = std::filesystem::file_time_type::min();
        
        try {
            last_write = std::filesystem::last_write_time(f);
            last_write_obj = std::filesystem::last_write_time(f.string() + ".o");
        } catch (std::filesystem::filesystem_error& _) {}

        if (last_write > last_write_obj || fullRebuild) {
            std::thread t(
                [cmd]() { exec_command(cmd); }
            );
            builders.push_back(std::move(t));
        }
    }

    link_command.push_back("-o");
    link_command.push_back(std::string(path) + "/" + binary);

    for (auto&& x : builders) {
        if (x.joinable())
            x.join();
    }

    std::filesystem::remove(path + "/Internal/" LIB_SCALE_FILENAME);
    
    exec_command(scale_runtime);
    exec_command(cxx_glue);
    exec_command(library);

    exec_command(create_command(link_command));

    std::filesystem::path symlinked_path;
    if (is_root()) {
        #ifdef _WIN32
        symlinked_path = home + "/bin/" + binary;
        #else
        symlinked_path = std::filesystem::path("/usr/local/bin") / binary;
        #endif
    } else {
        symlinked_path = home + "/bin/" + binary;
    }
    symlinked_path = symlinked_path.make_preferred();

    if (!std::filesystem::exists(symlinked_path.parent_path())) {
        std::filesystem::create_directories(symlinked_path.parent_path());
    } else {
        std::filesystem::remove(symlinked_path);
    }

#ifdef _WIN32
    if (CreateSymbolicLinkA(symlinked_path.string().c_str(), (path + "/" + binary).c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) == 0) {
        unsigned int err = GetLastError();
        if (err == ERROR_PRIVILEGE_NOT_HELD) {
            std::cout << "[INFO]: Unprivileged users cannot create symlinks in windows. Failed to create link from " << std::filesystem::path(path + "/" + binary).make_preferred() << " to " << symlinked_path << std::endl;
        } else {
            std::cerr << "Error creating symlink: " << GetLastError() << std::endl;
            std::exit(1);
        }
    };
#else
    try {
        std::filesystem::create_symlink(path + "/" + binary, symlinked_path);
    } catch (std::filesystem::filesystem_error& err) {
        std::cerr << "Error creating symlink: " << err.what() << std::endl;
        std::exit(1);
    }
#endif

    std::string macro_library = create_command({
        (std::filesystem::path(path) / binary).string(),
        "-makelib",
        "-o",
        path + "/Frameworks/Scale.framework/impl/__scale_macros.scl",
        path + "/Frameworks/Scale.framework/include/std/__internal/macro_entry.scale"
    });

    auto scale_stdlib = create_command({
        (std::filesystem::path(path) / binary).string(),
        "-no-link-std",
        "-makelib",
        "-o",
        path + "/Internal/" + SCALE_STDLIB_FILENAME
    });

    #ifdef _WIN32
    #define DIR_SEP "\\"
    #else
    #define DIR_SEP "/"
    #endif

    auto files = listFiles(path + "/Frameworks/Scale.framework/include", ".scale");
    for (auto&& f : files) {
        if (strcontains(f.string(), DIR_SEP "macros" DIR_SEP) || strcontains(f.string(), DIR_SEP "__") || strcontains(f.string(), DIR_SEP "compiler" DIR_SEP)) {
            continue;
        }
        scale_stdlib += f.string() + " ";
    }

    exec_command(scale_stdlib);
    exec_command(macro_library);

    if (!is_root()) {
        std::cout << "--------------" << std::endl;
        std::cout << "'sclc' was symlinked to " << symlinked_path << std::endl;
        std::cout << "--------------" << std::endl;
    }

    return 0;
}
