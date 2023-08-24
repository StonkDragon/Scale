#define NOBUILD_IMPLEMENTATION
extern "C" {
    #include "nobuild.h"
}

#include <iostream>
#include <filesystem>
#include <cstdlib>

#ifdef _WIN32
#define DEPEND(_cmd, _err) depend_on_system_command(_cmd " > nul 2>&1", _err)
#else
#define DEPEND(_cmd, _err) depend_on_system_command(_cmd " > /dev/null 2>&1", _err)
#endif
void depend_on_system_command(const char* command, const char* error) {
    int result = std::system(command);
    if (result != 0) {
        std::cerr << error << std::endl;
        std::exit(1);
    }
}

#ifdef _WIN32
#define SOFT_DEPEND(_cmd) std::system(_cmd " > nul 2>&1")
#else
#define SOFT_DEPEND(_cmd) std::system(_cmd " > /dev/null 2>&1")
#endif

int main(int argc, char const *argv[]) {
    DEPEND("clang --version", "clang is required!");
    DEPEND("clang++ --version", "clang++ is required!");
    DEPEND("cmake --version", "cmake is required!");
#if !defined(_WIN32)
    DEPEND("make --version", "make is required!");
#else
    DEPEND("nmake /?", "nmake is required!");
#endif

    if (!SOFT_DEPEND("dragon help")) {
        CMD("git", "clone", "https://github.com/StonkDragon/Dragon");
        std::filesystem::path rootPath = std::filesystem::current_path();
        std::filesystem::current_path("Dragon");
        CMD("clang", "-o", "nobuild", "nobuild.c");
        CMD("./nobuild");
        CMD("build/dragon", "build");
        std::filesystem::current_path(rootPath);
        RM("Dragon");
    }

    CMD("dragon", "build", "-conf", "install");

    return 0;
}
