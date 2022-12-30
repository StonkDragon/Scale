#include "../headers/DragonConfig.hpp"

using namespace std;
using namespace DragonConfig;

std::string ConfigEntry::getKey() const { return key; }
EntryType ConfigEntry::getType() const { return type; }
void ConfigEntry::setKey(const std::string& key) { this->key = key; }
void ConfigEntry::setType(EntryType type) { this->type = type; }
void ConfigEntry::print(std::ostream& out, int indent) { (void) out; (void) indent; }

StringEntry::StringEntry() {
    this->setType(EntryType::String);
}
std::string StringEntry::getValue() {
    return this->value;
}
void StringEntry::setValue(std::string value) {
    this->value = value;
}
bool StringEntry::isEmpty() {
    return this->value.empty();
}
bool StringEntry::operator==(const StringEntry& other) {
    return (this->value == other.value && this->getKey() == other.getKey());
}
bool StringEntry::operator!=(const StringEntry& other) {
    return !operator==(other);
}
void StringEntry::print(std::ostream& stream, int indent) {
    stream << std::string(indent, ' ') << this->getKey() << ": \"" << this->value << "\";" << std::endl;
}

ListEntry::ListEntry() {
    this->setType(EntryType::List);
    this->value = std::vector<std::string>();
}
std::string ListEntry::get(unsigned long index) {
    if (index >= this->value.size()) {
        std::cerr << "Index out of bounds" << std::endl;
        exit(1);
    }
    return this->value[index];
}
unsigned long ListEntry::size() {
    return this->value.size();
}
void ListEntry::add(std::string value) {
    this->value.push_back(value);
}
void ListEntry::addAll(std::vector<std::string> values) {
    this->value.insert(this->value.end(), values.begin(), values.end());
}
void ListEntry::remove(unsigned long index) {
    if (index >= this->value.size()) {
        std::cerr << "Index out of bounds" << std::endl;
        exit(1);
    }
    this->value.erase(this->value.begin() + index);
}
void ListEntry::removeAll(std::vector<unsigned long> indices) {
    for (unsigned long i = 0; i < indices.size(); i++) {
        this->remove(indices[i]);
    }
}
void ListEntry::clear() {
    this->value.clear();
}
bool ListEntry::isEmpty() {
    return this->value.empty();
}
bool ListEntry::operator==(const ListEntry& other) {
    return (this->value == other.value && this->getKey() == other.getKey());
}
bool ListEntry::operator!=(const ListEntry& other) {
    return !operator==(other);
}
void ListEntry::print(std::ostream& stream, int indent) {
    stream << std::string(indent, ' ') << this->getKey() << ": [" << std::endl;
    indent += 2;
    for (unsigned long i = 0; i < this->value.size(); i++) {
        stream << std::string(indent, ' ') << "\"" << this->value[i] << "\";" << std::endl;
    }
    indent -= 2;
    stream << std::string(indent, ' ') << "];" << std::endl;
}

CompoundEntry::CompoundEntry() {
    this->setType(EntryType::Compound);
}
bool CompoundEntry::hasMember(const std::string& key) {
    for (auto entry : this->entries) {
        if (entry->getKey() == key) {
            return true;
        }
    }
    return false;
}
StringEntry* CompoundEntry::getString(const std::string& key) {
    for (auto& entry : this->entries) {
        if (entry->getKey() == key) {
            return reinterpret_cast<StringEntry*>(entry);
        }
    }
    return nullptr;
}
StringEntry* CompoundEntry::getStringOrDefault(const std::string& key, const std::string& defaultValue) {
    for (auto entry : this->entries) {
        if (entry->getKey() == key) {
            return reinterpret_cast<StringEntry*>(entry);
        }
    }
    StringEntry* entry = new StringEntry();
    entry->setKey(key);
    entry->setValue(defaultValue);
    this->entries.push_back(entry);
    return entry;
}
ListEntry* CompoundEntry::getList(const std::string& key) {
    for (auto entry : this->entries) {
        if (entry->getKey() == key) {
            return reinterpret_cast<ListEntry*>(entry);
        }
    }
    return nullptr;
}
CompoundEntry* CompoundEntry::getCompound(const std::string& key) {
    for (auto entry : this->entries) {
        if (entry->getKey() == key) {
            return reinterpret_cast<CompoundEntry*>(entry);
        }
    }
    return nullptr;
}
void CompoundEntry::setString(const std::string& key, const std::string& value) {
    for (auto& entry : this->entries) {
        if (entry->getKey() == key) {
            reinterpret_cast<StringEntry*>(entry)->getValue() = value;
            return;
        }
    }
    StringEntry* newEntry = new StringEntry();
    newEntry->setKey(key);
    newEntry->setValue(value);
    this->entries.push_back(newEntry);
}
void CompoundEntry::addString(const std::string& key, const std::string& value) {
    if (this->hasMember(key)) {
        std::cerr << "std::string with key '" << key << "' already exists" << std::endl;
        exit(1);
    }
    StringEntry* newEntry = new StringEntry();
    newEntry->setKey(key);
    newEntry->setValue(value);
    this->entries.push_back(newEntry);
}
void CompoundEntry::addList(const std::string& key, const std::vector<std::string>& value) {
    if (this->hasMember(key)) {
        std::cerr << "List with key '" << key << "' already exists!" << std::endl;
        exit(1);
    }
    ListEntry* newEntry = new ListEntry();
    newEntry->setKey(key);
    newEntry->addAll(value);
    this->entries.push_back(newEntry);
}
void CompoundEntry::addList(const std::string& key, std::string value) {
    if (this->hasMember(key)) {
        std::cerr << "List with key '" << key << "' already exists!" << std::endl;
        exit(1);
    }
    ListEntry* newEntry = new ListEntry();
    newEntry->setKey(key);
    newEntry->add(value);
    this->entries.push_back(newEntry);
}
void CompoundEntry::addList(ListEntry* value) {
    if (this->hasMember(value->getKey())) {
        std::cerr << "List with key '" << value->getKey() << "' already exists!" << std::endl;
        exit(1);
    }
    this->entries.push_back(reinterpret_cast<ConfigEntry*>(value));
}
void CompoundEntry::addCompound(CompoundEntry* value) {
    if (this->hasMember(value->getKey())) {
        std::cerr << "Compound with key '" << value->getKey() << "' already exists!" << std::endl;
        exit(1);
    }
    this->entries.push_back(reinterpret_cast<ConfigEntry*>(value));
}
void CompoundEntry::remove(const std::string& key) {
    for (u_long i = 0; i < this->entries.size(); i++) {
        if (this->entries[i]->getKey() == key) {
            this->entries[i] = nullptr;
            return;
        }
    }
}
void CompoundEntry::removeAll() {
    this->entries.clear();
}
bool CompoundEntry::isEmpty() {
    return this->entries.empty();
}
void CompoundEntry::print(std::ostream& stream, int indent) {
    if (this->getKey() == ".root") {
        for (u_long i = 0; i < this->entries.size(); i++) {
            this->entries[i]->print(stream, indent);
        }
    } else {
        stream << std::string(indent, ' ') << this->getKey() << ": {" << std::endl;
        indent += 2;
        for (u_long i = 0; i < this->entries.size(); i++) {
            this->entries[i]->print(stream, indent);
        }
        indent -= 2;
        stream << std::string(indent, ' ') << "};" << std::endl;
    }
}

CompoundEntry* ConfigParser::parse(const std::string& configFile) {
    FILE* fp = fopen(configFile.c_str(), "r");
    if (!fp) {
        return nullptr;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);
    std::string config(buf);
    delete[] buf;

    int configSize = config.size();
    bool inStr = false;
    std::string data;
    int z = 0;
    for (int *i = &z; (*i) < configSize; ++(*i)) {
        if (!inStr && config.at((*i)) == ' ') continue;
        if (config.at((*i)) == '"' && config.at((*i) - 1) != '\\') {
            inStr = !inStr;
            char c = config.at((*i));
            data += c;
            continue;
        }
        if (config.at((*i)) == '\n') {
            inStr = false;
            continue;
        }
        char c = config.at((*i));
        data += c;
    }
    std::string str;

    data = ".root:{" + data + "};";
    std::string key = ".root";
    int i = 6;
    CompoundEntry* rootEntry = parseCompound(data, &i);
    rootEntry->setKey(".root");
    return rootEntry;
}

bool ConfigParser::isValidIdentifier(char c) {
    return c != ':';
    // return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
}

CompoundEntry* ConfigParser::parseCompound(std::string& data, int* i) {
    CompoundEntry* compound = new CompoundEntry();
    char c = data.at(++(*i));
    while (c != '}') {
        std::string key = "";
        for (; isValidIdentifier(c); c = data.at(++(*i))) {
            key += c;
        }
        c = data.at(++(*i));
        if (c == '[') {
            std::vector<std::string> values;
            c = data.at(++(*i));
            while (c != ']') {
                std::string next;
                if (c == '"') {
                    char prev = c;
                    c = data.at(++(*i));
                    while (true) {
                        if (c == '"' && prev != '\\') break;
                        prev = c;
                        next += c;
                        c = data.at(++(*i));
                    }
                    c = data.at(++(*i));
                    if (c != ';') {
                        std::cerr << "[Dragon] " << "Missing semicolon: " << next << std::endl;
                        exit(1);
                    }
                    values.push_back(next);
                    c = data.at(++(*i));
                }
            }
            c = data.at(++(*i));
            if (c != ';') {
                std::cerr << "[Dragon] " << "Missing semicolon: " << key << std::endl;
                exit(1);
            }
            ListEntry* entry = new ListEntry();
            entry->setKey(key);
            entry->addAll(values);
            compound->entries.push_back(entry);
        } else if (c == '{') {
            CompoundEntry* entry = parseCompound(data, i);
            entry->setKey(key);
            compound->entries.push_back(entry);
        } else {
            std::string value = "";
            if (c != '"') {
                std::cerr << "[Dragon] " << "Invalid std::string: " << key << std::endl;
                exit(1);
            }
            c = data.at(++(*i));
            while (c != '"') {
                value += c;
                c = data.at(++(*i));
            }
            c = data.at(++(*i));
            if (c != ';') {
                std::cerr << "[Dragon] " << "Invalid value: " << value << std::endl;
                exit(1);
            }
            StringEntry* entry = new StringEntry();
            entry->setKey(key);
            entry->setValue(value);
            compound->entries.push_back(entry);
        }
        c = data.at(++(*i));
    }
    c = data.at(++(*i));
    if (c != ';') {
        std::cerr << "[Dragon] " << "Invalid compound entry!" << std::endl;
        exit(1);
    }
    return compound;
}
