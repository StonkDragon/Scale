#include <iostream>
#include <filesystem>
#include <cstdlib>

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

    for (auto versionedFolder : std::filesystem::directory_iterator(ABS_PATH("opt", "Scale"))) {
        if (!versionedFolder.is_directory() && versionedFolder.path().filename() != std::string("latest")) {
            continue;
        }
        std::string file = versionedFolder.path().filename().string();
        std::filesystem::remove_all(
            std::filesystem::path("/usr/local/bin/sclc-" + file)
        );
        std::filesystem::create_symlink(
            versionedFolder.path().string() + std::string("/sclc"),
            std::filesystem::path("/usr/local/bin/sclc-" + file)
        );
    }

    DragonConfig::CompoundEntry* root = DragonConfig::ConfigParser().parse("build.drg");

    std::string version = root->getStringOrDefault("VERSION", "0.0.0")->getValue();

    std::filesystem::remove_all(
        std::filesystem::path("/opt/Scale/latest")
    );
    std::filesystem::create_directory_symlink(
        std::filesystem::path("/opt/Scale/" + version),
        std::filesystem::path("/opt/Scale/latest")
    );

    CMD("git", "clone", "--depth=1", "https://github.com/ivmai/bdwgc.git", "bdwgc");
    std::filesystem::path rootPath = std::filesystem::current_path();
    std::filesystem::current_path(std::string("bdwgc"));
    CMD("git", "clone", "--depth=1", "https://github.com/ivmai/libatomic_ops");
#if !defined(_WIN32)
    CMD("autoreconf", "-vif");
    std::string prefix = std::string("--prefix=/opt/Scale/" + version + "/Internal");
    CMD("./configure", prefix.c_str(), "--enable-static", "--disable-shared", "--with-pic");
    CMD("make");
    CMD("sudo", "make", "install");
#else
    ERRO("TODO: Building bdwgc on Windows needs implementation. Please PR if you figure this out!");
    std::exit(1);
#endif
    std::filesystem::current_path(rootPath);
    RM("bdwgc");

    return 0;
}
