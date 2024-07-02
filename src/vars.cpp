#include <vars.h>

void Vars::set(const std::string& name, const std::string& value)
{
    data[name] = value;

    if (is_exported(name))
        setenv(name.c_str(), data[name].c_str(), 1);
}

bool Vars::get(const std::string& name, std::string& value)
{
    if (contains(name))
    {
        value = data[name];
        return true;
    }

    return false;
}

bool Vars::unset(const std::string& name)
{
    if (!contains(name))
        return false;

    if (is_exported(name))
    {
        exported.erase(name);
        unsetenv(name.c_str());
    }

    data.erase(name);

    return true;
}

bool Vars::export_(const std::string& name)
{
    if (!contains(name))
        return false;

    exported.insert(name);
    setenv(name.c_str(), data[name].c_str(), 1);

    return true;
}

bool Vars::contains(const std::string& name)
{
    return data.find(name) != data.end();
}

bool Vars::is_exported(const std::string& name)
{
    return exported.find(name) != exported.end();
}