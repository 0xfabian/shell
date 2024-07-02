#pragma once

#include <unordered_map>

struct Aliases
{
    std::unordered_map<std::string, std::string> data;

    void set(const std::string& name, const std::string& value);
    bool get(const std::string& name, std::string& value);
    bool unset(const std::string& name);

    bool contains(const std::string& name);
};