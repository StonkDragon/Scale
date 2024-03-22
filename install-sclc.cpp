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

template<typename Args>
std::string create_command(std::vector<Args> args) {
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
    bool root = false;
#ifdef _WIN32
    void* adminSid = NULL;

    if (CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, &adminSid)) {
        if (!CheckTokenMembership(NULL, adminSid, &root)) {
            root = false;
        }

        FreeSid(adminSid);
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
    // require_root();

    depends_on("clang --version", "clang is required!");
    depends_on("clang++ --version", "clang++ is required!");
    depends_on("cmake --version", "cmake is required!");
#if !defined(_WIN32)
    depends_on("make --version", "make is required!");
#else
    depends_on("nmake /?", "nmake is required!");
#endif

#define CONCAT(a, b) CONCAT_(a, b)
#define CONCAT_(a, b) a ## b

#include "Configuration.h"

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

    std::string scl_root_dir = std::filesystem::absolute(SCL_ROOT_DIR);

    std::string path = scl_root_dir + ("/Scale/" STR(VERSION));
    std::string binary = "sclc";

    if (is_root()) {
        std::filesystem::remove_all("/opt/Scale/latest");
        std::filesystem::create_directories("/opt/Scale/");
        std::filesystem::create_directory_symlink(std::filesystem::path(path), "/opt/Scale/latest");
        std::filesystem::permissions(
            path,
            std::filesystem::perms::all
        );
        std::filesystem::permissions(
            "/opt/Scale/latest",
            std::filesystem::perms::all
        );
    }

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
    }

    std::filesystem::copy("Scale", path, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

    if (!isDevBuild) {
        exec_command(create_command<std::string>({"git", "clone", "--depth=1", "https://github.com/ivmai/bdwgc.git", "bdwgc", "-b", "release-8_2"}));
        
        auto oldsighandler = signal(SIGINT, [](int sig){
            std::filesystem::current_path(std::filesystem::current_path().parent_path());
            std::filesystem::remove_all("bdwgc");
        });
        std::filesystem::current_path("bdwgc");

        exec_command(create_command<std::string>({"sh", "./autogen.sh"}));
        exec_command(create_command<std::string>({"./configure", "--prefix=" + path + "/Internal"}));
        exec_command(create_command<std::string>({"make"}));
        exec_command(create_command<std::string>({"make", "install"}));

        std::filesystem::current_path(std::filesystem::current_path().parent_path());
        std::filesystem::remove_all("bdwgc");
    }

    auto scale_runtime = create_command<std::string>({
        "clang",
        "-O2",
        "-std=gnu17",
        "-I" + path + "/Internal",
        "-I" + path + "/Internal/include",
        path + "/Internal/scale_runtime.c",
        "-c",
    #if !defined(_WIN32)
        "-fPIC",
    #endif
        "-o",
        path + "/Internal/scale_runtime.o"
    });
    auto cxx_glue = create_command<std::string>({
        "clang++",
        "-O2",
        "-std=gnu++17",
        "-I" + path + "/Internal",
        "-I" + path + "/Internal/include",
        path + "/Internal/scale_cxx.cpp",
        "-c",
    #if !defined(_WIN32)
        "-fPIC",
    #endif
        "-o",
        path + "/Internal/scale_cxx.o"
    });

    auto library = create_command<std::string>({
        "clang++",
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
    #if defined(_WIN32)
        "-static-libstdc++",
    #endif
        path + "/Internal/scale_runtime.o",
        path + "/Internal/scale_cxx.o",
        "-L" + path + "/Internal/lib",
        "-lgc",
        "-o",
        path + "/Internal/" LIB_SCALE_FILENAME
    });

    std::string compile_command = create_command<std::string>({
        "clang++",
        ("-DVERSION=\\\"" STR(VERSION) "\\\""),
        "-DC_VERSION=\\\"gnu17\\\"",
        "-DSCL_ROOT_DIR=\\\"" + scl_root_dir + "\\\"",
        "-std=gnu++17",
        "-Wall",
        "-Wextra",
        "-Werror",
    });

    if (debug) {
        compile_command += "-O0 -g ";
    } else {
        compile_command += "-O2 ";
    }

    auto source_files = listFiles("Compiler", ".cpp");
    auto builders = std::vector<std::thread>(source_files.size());
    
    std::vector<std::string> link_command = {
        compile_command
    };

    for (auto f : source_files) {
        std::string cmd = create_command<std::string>({
            compile_command,
            "-o",
            f.string() + ".o",
            "-c",
            f.string()
        });
        link_command.push_back(f.string() + ".o");

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

#ifdef __linux__
    link_command.push_back("-Wl,--export-dynamic");
#endif

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

    exec_command(create_command<std::string>(link_command));

#ifdef _WIN32
    std::filesystem::path home = std::filesystem::path(std::getenv("UserProfile"));
#else
    std::filesystem::path home = std::filesystem::path(std::getenv("HOME"));
#endif

    std::filesystem::path symlinked_path;
    if (is_root()) {
        symlinked_path = "/usr/local/bin/sclc";
    } else if (std::filesystem::exists(home / "bin")) {
        symlinked_path = home / "bin/sclc";
    }
    std::filesystem::remove(symlinked_path);
    std::filesystem::create_symlink(std::string(path) + "/" + binary, symlinked_path);

    std::string macro_library = create_command<std::string>({
        std::string(path) + "/" + binary,
        "-makelib",
        "-o",
        path + "/Frameworks/Scale.framework/impl/__scale_macros.scl",
        path + "/Frameworks/Scale.framework/include/std/__internal/macro_entry.scale"
    });

    auto scale_stdlib = create_command<std::string>({
        std::string(path) + "/" + binary,
        "-no-link-std",
        "-makelib",
        "-o",
        path + "/Internal/" + SCALE_STDLIB_FILENAME
    });

    auto files = listFiles(path + "/Frameworks/Scale.framework/include", ".scale");
    for (auto&& f : files) {
        if (strcontains(f.string(), "/macros/") || strcontains(f.string(), "/__") || strcontains(f.string(), "/compiler/")) {
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
