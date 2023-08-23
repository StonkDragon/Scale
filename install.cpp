#define NOBUILD_IMPLEMENTATION
extern "C" {
    #include "nobuild.h"
}

#include <iostream>
#include <filesystem>
#include <cstdlib>

#ifdef _WIN32
#define DEPEND(_cmd) depend_on_system_command(_cmd " > nul 2>&1")
#else
#define DEPEND(_cmd) depend_on_system_command(_cmd " > /dev/null 2>&1")
#endif
void depend_on_system_command(const char* command) {
    int result = std::system(command);
    if (result != 0) {
        std::cerr << "Command '" << command << "' failed with exit code " << result << std::endl;
        std::exit(1);
    }
}

#ifdef _WIN32
#define SOFT_DEPEND(_cmd) std::system(_cmd " > nul 2>&1")
#else
#define SOFT_DEPEND(_cmd) std::system(_cmd " > /dev/null 2>&1")
#endif

int main(int argc, char const *argv[]) {
    DEPEND("clang --version");
    DEPEND("clang++ --version");
    DEPEND("cmake --version");
#if !defined(_WIN32)
    DEPEND("make --version");
#else
    DEPEND("nmake /?");
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
