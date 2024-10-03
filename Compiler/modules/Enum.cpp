
#include <Enum.hpp>

namespace sclc {
    const size_t Enum::npos = -1;
    Enum::Enum(std::string name) {
        this->name = name;
        isExtern = false;
    }
    void Enum::addMember(std::string member, long value, std::string type) {
        members[member] = value;
        member_types[member] = type;
        nextValue = value + 1;
    }
    size_t Enum::indexOf(std::string name) {
        return members.find(name) != members.end() ? members[name] : npos;
    }
    bool Enum::hasMember(std::string name) {
        return members.find(name) != members.end();
    }
} // namespace sclc
