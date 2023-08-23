#include <iostream>
#include <filesystem>

#define NOBUILD_IMPLEMENTATION
extern "C" {
    #include "nobuild.h"
}
#include "Compiler/headers/DragonConfig.hpp"
#include "Compiler/modules/DragonConfig.cpp"

void require_root() {
    if (getuid() != 0) {
        std::cerr << "This file must be run as root." << std::endl;
        exit(1);
    }
}

int main(int argc, char const *argv[]) {
    require_root();

    CMD("git", "clone", "https://github.com/ivmai/bdwgc.git", "bdwgc");
    std::filesystem::path rootPath = std::filesystem::current_path();
    std::filesystem::current_path("bdwgc");
    MKDIRS("out");
    std::filesystem::current_path("out");
    CMD("cmake", "-Dbuild_tests=ON", "..");
    CMD("cmake", "--build", ".");
    CMD("ctest");
#if !defined(_WIN32)
    CMD("sudo", "make", "install");
#else
    CMD("nmake", "-f", "NT_MAKEFILE", "install");
#endif
    std::filesystem::current_path(rootPath);
    RM("bdwgc");

    for (auto versionedFolder : std::filesystem::directory_iterator(ABS_PATH("opt", "Scale"))) {
        if (!versionedFolder.is_directory() && versionedFolder.path().filename() != "latest") {
            continue;
        }
        std::string file = versionedFolder.path().filename().string();
        std::filesystem::remove_all(
            std::filesystem::path("/usr") / "local" / "bin" / ("sclc-" + file)
        );
        std::filesystem::create_symlink(
            versionedFolder.path() / "sclc",
            std::filesystem::path("/usr") / "local" / "bin" / ("sclc-" + file)
        );
    }

    DragonConfig::CompoundEntry* root = DragonConfig::ConfigParser().parse("build.drg");

    std::string version = root->getStringOrDefault("VERSION", "0.0.0")->getValue();

    std::filesystem::remove_all(
        std::filesystem::path("/opt") / "Scale" / "latest"
    );
    std::filesystem::create_directory_symlink(
        std::filesystem::path("/opt") / "Scale" / version,
        std::filesystem::path("/opt") / "Scale" / "latest"
    );

    return 0;
}
