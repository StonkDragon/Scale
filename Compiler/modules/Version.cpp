#include <gc/gc_allocator.h>

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

#include "../headers/Version.hpp"

namespace sclc {
    Version::Version(std::string str) {
        std::string::difference_type n = std::count(str.begin(), str.end(), '.');
        if (n == 1) {
            sscanf(str.c_str(), "%d.%d", &major, &minor);
            patch = 0;
        } else if (n == 2) {
            sscanf(str.c_str(), "%d.%d.%d", &major, &minor, &patch);
        } else if (n == 0) {
            sscanf(str.c_str(), "%d", &major);
            minor = 0;
            patch = 0;
        } else {
            std::cerr << "Unknown version number: " << str << std::endl;
            exit(1);
        }
    }
    Version::Version(int major, int minor, int patch) : major(major), minor(minor), patch(patch) {}
    Version::Version() : Version(0, 0, 0) {}
    Version::Version(const Version& other) {
        this->major = other.major;
        this->minor = other.minor;
        this->patch = other.patch;
    }
    Version::Version(Version&& other) {
        this->major = other.major;
        other.major = 0;
        this->minor = other.minor;
        other.minor = 0;
        this->patch = other.patch;
        other.patch = 0;
    }
    Version& Version::operator=(const Version& other) {
        this->major = other.major;
        this->minor = other.minor;
        this->patch = other.patch;
        return *this;
    }
    Version& Version::operator=(Version&& other) {
        this->major = other.major;
        other.major = 0;
        this->minor = other.minor;
        other.minor = 0;
        this->patch = other.patch;
        other.patch = 0;
        return *this;
    }
    Version::~Version() {}

    bool Version::operator==(Version& v) const {
        return (major == v.major) && (minor == v.minor) && (patch == v.patch);
    }

    bool Version::operator>(Version& v) const {
        if (major > v.major) {
            return true;
        } else if (major == v.major) {
            if (minor > v.minor) {
                return true;
            } else if (minor == v.minor) {
                if (patch > v.patch) {
                    return true;
                }
                return false;
            }
            return false;
        }
        return false;
    }

    bool Version::operator>=(Version& v) const {
        return ((*this) > v) || ((*this) == v);
    }

    bool Version::operator<=(Version& v) const {
        return !((*this) > v);
    }

    bool Version::operator<(Version& v) const {
        return !((*this) >= v);
    }

    bool Version::operator!=(Version& v) const {
        return !((*this) == v);
    }

    std::string Version::asString() {
        return std::to_string(this->major) + "." + std::to_string(this->minor) + "." + std::to_string(this->patch);
    }
} // namespace sclc
