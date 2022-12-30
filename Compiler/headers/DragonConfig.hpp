#pragma once

#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
typedef unsigned long u_long;
#endif

using namespace std;

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
        virtual std::string getKey() const;
        virtual EntryType getType() const;
        virtual void setKey(const std::string& key);
        virtual void setType(EntryType type);
        virtual void print(std::ostream& out, int indent = 0);
    };

    struct StringEntry : public ConfigEntry {
    private:
        std::string value;

    public:
        StringEntry();
        std::string getValue();
        void setValue(std::string value);
        bool isEmpty();
        bool operator==(const StringEntry& other);
        bool operator!=(const StringEntry& other);
        void print(std::ostream& stream, int indent = 0);
    };

    struct ListEntry : public ConfigEntry {
    private:
        std::vector<std::string> value;

    public:
        ListEntry();
        std::string get(unsigned long index);
        unsigned long size();
        void add(std::string value);
        void addAll(std::vector<std::string> values);
        void remove(unsigned long index);
        void removeAll(std::vector<unsigned long> indices);
        void clear();
        bool isEmpty();
        bool operator==(const ListEntry& other);
        bool operator!=(const ListEntry& other);
        void print(std::ostream& stream, int indent = 0);
    };

    struct CompoundEntry : public ConfigEntry {
        std::vector<ConfigEntry*> entries;

        CompoundEntry();
        bool hasMember(const std::string& key);
        StringEntry* getString(const std::string& key);
        StringEntry* getStringOrDefault(const std::string& key, const std::string& defaultValue);
        ListEntry* getList(const std::string& key);
        CompoundEntry* getCompound(const std::string& key);
        void setString(const std::string& key, const std::string& value);
        void addString(const std::string& key, const std::string& value);
        void addList(const std::string& key, const std::vector<std::string>& value);
        void addList(const std::string& key, std::string value);
        void addList(ListEntry* value);
        void addCompound(CompoundEntry* value);
        void remove(const std::string& key);
        void removeAll();
        bool isEmpty();
        void print(std::ostream& stream, int indent = 0);
    };

    struct ConfigParser {
        CompoundEntry* parse(const std::string& configFile);

    private:
        bool isValidIdentifier(char c);

        CompoundEntry* parseCompound(std::string& data, int* i);
    };
}
