
#include "../headers/Enum.hpp"

namespace sclc {
    const size_t Enum::npos = -1;
    Enum::Enum(std::string name) {
        this->name = name;
    }
    void Enum::addMember(std::string member, long value) {
        members[member] = value;
        nextValue = value + 1;
    }
    size_t Enum::indexOf(std::string name) {
        return members.find(name) != members.end() ? members[name] : npos;
    }
    bool Enum::hasMember(std::string name) {
        return members.find(name) != members.end();
    }
} // namespace sclc
