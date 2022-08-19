#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace DragonConfig {
    enum class EntryType {
        String,
        List,
        Compound
    };
    
    struct ConfigEntry {
    private:
        std::string key;
        EntryType type;

    public:
        std::string getKey() const { return key; }
        EntryType getType() const { return type; }
        void setKey(const std::string& key) { this->key = key; }
        void setType(EntryType type) { this->type = type; }
    };

    struct StringEntry : ConfigEntry {
        static const StringEntry Empty;

    private:
        std::string value;

    public:
        StringEntry() {
            this->setType(EntryType::String);
        }
        std::string getValue() {
            return this->value;
        }
        void setValue(std::string value) {
            this->value = value;
        }
        bool isEmpty() {
            return this->value.empty();
        }
        bool operator==(const StringEntry& other) {
            return (this->value == other.value && this->getKey() == other.getKey());
        }
        bool operator!=(const StringEntry& other) {
            return !operator==(other);
        }
        void print(std::ostream& stream, int indent = 0) {
            stream << std::string(indent, ' ') << this->getKey() << ": \"" << this->value << "\";" << std::endl;
        }
    };

    struct ListEntry : ConfigEntry {
        static const ListEntry Empty;

    private:
        std::vector<std::string> value;

    public:
        ListEntry() {
            this->setType(EntryType::List);
        }
        std::string get(unsigned long index) {
            if (index >= this->value.size()) {
                std::cerr << "Index out of bounds" << std::endl;
                exit(1);
            }
            return this->value[index];
        }
        unsigned long size() {
            return this->value.size();
        }
        void add(std::string value) {
            this->value.push_back(value);
        }
        void addAll(std::vector<std::string> values) {
            this->value.insert(this->value.end(), values.begin(), values.end());
        }
        void remove(unsigned long index) {
            if (index >= this->value.size()) {
                std::cerr << "Index out of bounds" << std::endl;
                exit(1);
            }
            this->value.erase(this->value.begin() + index);
        }
        void removeAll(std::vector<unsigned long> indices) {
            for (unsigned long i = 0; i < indices.size(); i++) {
                this->remove(indices[i]);
            }
        }
        void clear() {
            this->value.clear();
        }
        bool isEmpty() {
            return this->value.empty();
        }
        bool operator==(const ListEntry& other) {
            return (this->value == other.value && this->getKey() == other.getKey());
        }
        bool operator!=(const ListEntry& other) {
            return !operator==(other);
        }
        void print(std::ostream& stream, int indent = 0) {
            stream << std::string(indent, ' ') << this->getKey() << ": [" << std::endl;
            indent += 2;
            for (unsigned long i = 0; i < this->value.size(); i++) {
                stream << std::string(indent, ' ') << "\"" << this->value[i] << "\";" << std::endl;
            }
            indent -= 2;
            stream << std::string(indent, ' ') << "];" << std::endl;
        }
    };

    struct CompoundEntry : ConfigEntry {
        static const CompoundEntry Empty;

        std::vector<CompoundEntry> compounds;
        std::vector<ListEntry> lists;
        std::vector<StringEntry> strings;

        CompoundEntry() {
            this->setType(EntryType::Compound);
        }
        bool hasMember(const std::string& key) {
            for (auto& entry : this->strings) {
                if (entry.getKey() == key) {
                    return true;
                }
            }
            for (auto& entry : this->lists) {
                if (entry.getKey() == key) {
                    return true;
                }
            }
            for (auto& entry : this->compounds) {
                if (entry.getKey() == key) {
                    return true;
                }
            }
            return false;
        }
        StringEntry getString(const std::string& key) {
            for (auto& entry : this->strings) {
                if (entry.getKey() == key) {
                    return entry;
                }
            }
            return StringEntry::Empty;
        }
        StringEntry getStringOrDefault(const std::string& key, const std::string& defaultValue) {
            for (auto& entry : this->strings) {
                if (entry.getKey() == key) {
                    return entry;
                }
            }
            StringEntry entry;
            entry.setKey(key);
            entry.setValue(defaultValue);
            this->strings.push_back(entry);
            return entry;
        }
        ListEntry getList(const std::string& key) {
            for (auto& entry : this->lists) {
                if (entry.getKey() == key) {
                    return entry;
                }
            }
            return ListEntry::Empty;
        }
        CompoundEntry getCompound(const std::string& key) {
            for (auto& entry : this->compounds) {
                if (entry.getKey() == key) {
                    return entry;
                }
            }
            return CompoundEntry::Empty;
        }
        void setString(const std::string& key, const std::string& value) {
            for (auto& entry : this->strings) {
                if (entry.getKey() == key) {
                    entry.getValue() = value;
                    return;
                }
            }
            StringEntry newEntry;
            newEntry.setKey(key);
            newEntry.setValue(value);
            this->strings.push_back(newEntry);
        }
        void addString(const std::string& key, const std::string& value) {
            if (this->hasMember(key)) {
                std::cerr << "String with key '" << key << "' already exists" << std::endl;
                exit(1);
            }
            StringEntry newEntry;
            newEntry.setKey(key);
            newEntry.setValue(value);
            this->strings.push_back(newEntry);
        }
        void addList(const std::string& key, const std::vector<std::string>& value) {
            if (this->hasMember(key)) {
                std::cerr << "List with key '" << key << "' already exists!" << std::endl;
                exit(1);
            }
            ListEntry newEntry;
            newEntry.setKey(key);
            newEntry.addAll(value);
            this->lists.push_back(newEntry);
        }
        void addList(const std::string& key, const std::string& value) {
            if (this->hasMember(key)) {
                std::cerr << "List with key '" << key << "' already exists!" << std::endl;
                exit(1);
            }
            ListEntry newEntry;
            newEntry.setKey(key);
            newEntry.add(value);
            this->lists.push_back(newEntry);
        }
        void addList(const ListEntry& value) {
            if (this->hasMember(value.getKey())) {
                std::cerr << "List with key '" << value.getKey() << "' already exists!" << std::endl;
                exit(1);
            }
            this->lists.push_back(value);
        }
        void addCompound(const CompoundEntry& value) {
            if (this->hasMember(value.getKey())) {
                std::cerr << "Compound with key '" << value.getKey() << "' already exists!" << std::endl;
                exit(1);
            }
            this->compounds.push_back(value);
        }
        void removeString(const std::string& key) {
            for (u_long i = 0; i < this->strings.size(); i++) {
                if (this->strings[i].getKey() == key) {
                    this->strings[i] = StringEntry::Empty;
                    return;
                }
            }
        }
        void removeList(const std::string& key) {
            for (u_long i = 0; i < this->lists.size(); i++) {
                if (this->lists[i].getKey() == key) {
                    this->lists[i] = ListEntry::Empty;
                    return;
                }
            }
        }
        void removeCompound(const std::string& key) {
            for (u_long i = 0; i < this->compounds.size(); i++) {
                if (this->compounds[i].getKey() == key) {
                    this->compounds[i] = CompoundEntry::Empty;
                    return;
                }
            }
        }
        void removeAll() {
            this->compounds.clear();
            this->lists.clear();
            this->strings.clear();
        }
        bool isEmpty() {
            return this->compounds.empty() && this->lists.empty() && this->strings.empty();
        }
        bool operator==(const CompoundEntry& other) {
            for (u_long i = 0; i < this->strings.size(); i++) {
                if (this->strings[i] != other.strings[i]) {
                    return false;
                }
            }
            for (u_long i = 0; i < this->lists.size(); i++) {
                if (this->lists[i] != other.lists[i]) {
                    return false;
                }
            }
            for (u_long i = 0; i < this->compounds.size(); i++) {
                if (this->compounds[i] != other.compounds[i]) {
                    return false;
                }
            }
            return true;
        }
        bool operator!=(const CompoundEntry& other) {
            return !operator==(other);
        }
        void print(std::ostream& stream, int indent = 0) {
            if (this->getKey() == ".root") {
                for (u_long i = 0; i < this->strings.size(); i++) {
                    this->strings[i].print(stream, indent);
                }
                for (u_long i = 0; i < this->lists.size(); i++) {
                    this->lists[i].print(stream, indent);
                }
                for (u_long i = 0; i < this->compounds.size(); i++) {
                    this->compounds[i].print(stream, indent);
                }
            } else {
                stream << std::string(indent, ' ') << this->getKey() << ": {" << std::endl;
                indent += 2;
                for (u_long i = 0; i < this->strings.size(); i++) {
                    this->strings[i].print(stream, indent);
                }
                for (u_long i = 0; i < this->lists.size(); i++) {
                    this->lists[i].print(stream, indent);
                }
                for (u_long i = 0; i < this->compounds.size(); i++) {
                    this->compounds[i].print(stream, indent);
                }
                indent -= 2;
                stream << std::string(indent, ' ') << "};" << std::endl;
            }
        }
    };

    const CompoundEntry CompoundEntry::Empty;
    const StringEntry StringEntry::Empty;
    const ListEntry ListEntry::Empty;

    struct ConfigParser {
        CompoundEntry parse(const std::string& configFile) {
            FILE* fp = fopen(configFile.c_str(), "r");
            if (!fp) {
                std::cerr << "[Dragon] " << "Failed to open buildConfig file: " << configFile << std::endl;
                exit(1);
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
            CompoundEntry rootEntry = parseCompound(data, &i);
            rootEntry.setKey(".root");
            return rootEntry;
        }

    private:
        bool isValidIdentifier(char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        }

        CompoundEntry parseCompound(std::string& data, int* i) {
            CompoundEntry compound;
            char c = data.at(++(*i));
            while (c != '}') {
                std::string key = "";
                while (c != ':') {
                    key += c;
                    if (!isValidIdentifier(c)) {
                        std::cerr << "[Dragon] " << "Invalid identifier: " << key << std::endl;
                        exit(1);
                    }
                    c = data.at(++(*i));
                }
                c = data.at(++(*i));
                if (c == '[') {
                    std::vector<std::string> values;
                    c = data.at(++(*i));
                    while (c != ']') {
                        std::string next;
                        if (c != '"') {
                            std::cerr << "[Dragon] " << "Invalid string: " << key << std::endl;
                            exit(1);
                        }
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
                    c = data.at(++(*i));
                    if (c != ';') {
                        std::cerr << "[Dragon] " << "Missing semicolon: " << key << std::endl;
                        exit(1);
                    }
                    ListEntry entry;
                    entry.setKey(key);
                    entry.addAll(values);
                    compound.lists.push_back(entry);
                } else if (c == '{') {
                    CompoundEntry entry = parseCompound(data, i);
                    entry.setKey(key);
                    compound.compounds.push_back(entry);
                } else {
                    std::string value = "";
                    if (c != '"') {
                        std::cerr << "[Dragon] " << "Invalid string: " << key << std::endl;
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
                    StringEntry entry;
                    entry.setKey(key);
                    entry.setValue(value);
                    compound.strings.push_back(entry);
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
    };
}
