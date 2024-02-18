#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <thread>
#include <shared_mutex>

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

void require_root() {
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
    if (!root) {
        std::cerr << "This file must be run as root." << std::endl;
        exit(1);
    }
}

int main(int argc, char const *argv[]) {
    require_root();

    depends_on("clang --version", "clang is required!");
    depends_on("clang++ --version", "clang++ is required!");
    depends_on("cmake --version", "cmake is required!");
#if !defined(_WIN32)
    depends_on("make --version", "make is required!");
#else
    depends_on("nmake /?", "nmake is required!");
#endif

#include "Configuration.h"

    bool isDevBuild = (argv[1] && (std::strcmp(argv[1], "-dev") == 0));

    std::string compile_command = create_command<std::string>({
        "clang++",
        "-DVERSION=\\\"" STR(VERSION) "\\\"",
        "-DC_VERSION=\\\"gnu17\\\"",
        "-DSCL_ROOT_DIR=\\\"" SCL_ROOT_DIR "\\\"",
        "-std=gnu++17",
        "-Wall",
        "-Wextra",
        "-Werror",
    #ifdef DEBUG
        "-g",
        "-O0"
    #else
        "-O2"
    #endif
    });

    std::string path = SCL_ROOT_DIR "/Scale/" STR(VERSION);

    if (!isDevBuild) {
        std::filesystem::remove_all(path);
    } else {
        std::filesystem::remove_all(path + "/Frameworks");
        std::filesystem::remove_all(path + "/Internal/" LIB_SCALE_FILENAME);
    }
    std::filesystem::create_directories(path);
    std::filesystem::permissions(
        path,
        std::filesystem::perms::all
    );

    std::filesystem::copy("Scale", path, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

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

        if (last_write > last_write_obj) {
            std::thread t(
                [cmd]() { exec_command(cmd); }
            );
            builders.push_back(std::move(t));
        }
    }

#ifdef __linux__
    link_command.push_back("-Wl,--export-dynamic");
#endif

    std::string binary = "sclc";

    link_command.push_back("-o");
    link_command.push_back(binary);

    for (auto&& x : builders) {
        if (x.joinable())
            x.join();
    }

    exec_command(create_command<std::string>(link_command));

    std::filesystem::copy(binary, std::string(path) + "/" + binary, std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove("/usr/local/bin/sclc");
    std::filesystem::remove("/usr/local/bin/scaledoc");
    std::filesystem::create_symlink(std::string(path) + "/" + binary, "/usr/local/bin/sclc");
    std::filesystem::create_symlink(std::string(path) + "/" + binary, "/usr/local/bin/scaledoc");
    
    std::filesystem::remove_all("/opt/Scale/latest");
    std::filesystem::create_directory_symlink(std::filesystem::path(path), "/opt/Scale/latest");

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
        exec_command(create_command<std::string>({"sudo", "make", "install"}));

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
        STR(VERSION),
        "-compatibility_version",
        STR(VERSION),
        "-undefined",
        "dynamic_lookup",
    #else
    #ifdef __linux__
        "-Wl,--undefined",
    #endif
        "-shared",
    #endif
    #if defined(_WIN32)
        "-static-libstdc++",
    #else
        "-fPIC",
    #endif
        path + "/Internal/scale_runtime.o",
        path + "/Internal/scale_cxx.o",
        "-L" + path + "/Internal/lib",
        "-lgc",
        "-o",
        path + "/Internal/" LIB_SCALE_FILENAME
    });

    std::filesystem::remove(path + "/Internal/" LIB_SCALE_FILENAME);

    exec_command(scale_runtime);
    exec_command(cxx_glue);
    exec_command(library);

    std::string macro_library = create_command<std::string>({
        "sclc",
        "-makelib",
        "-o",
        path + "/Frameworks/Scale.framework/impl/__scale_macros.scl",
        path + "/Frameworks/Scale.framework/include/std/__internal/macro_entry.scale"
    });

    exec_command(macro_library);

    return 0;
}
