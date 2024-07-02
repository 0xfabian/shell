#pragma once

#include <unordered_map>
#include <unordered_set>

struct Vars
{
    std::unordered_map<std::string, std::string> data;
    std::unordered_set<std::string> exported;

    void set(const std::string& name, const std::string& value);
    bool get(const std::string& name, std::string& value);
    bool unset(const std::string& name);

    bool export_(const std::string& name);

    bool contains(const std::string& name);
    bool is_exported(const std::string& name);
};