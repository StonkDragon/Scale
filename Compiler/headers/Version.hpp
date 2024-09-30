#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>

namespace sclc
{
    struct Version {
        int major;
        int minor;
        int patch;

        Version(std::string str);
        Version(int major, int minor, int patch);
        Version();
        Version(const Version& other);
        Version(Version&& other);
        Version& operator=(const Version& other);
        Version& operator=(Version&& other);
        ~Version();

        bool operator==(const Version& v) const;
        bool operator>(const Version& v) const;
        bool operator>=(const Version& v) const;
        bool operator<=(const Version& v) const;
        bool operator<(const Version& v) const;
        bool operator!=(const Version& v) const;
        std::string asString() const;
    };
} // namespace sclc
