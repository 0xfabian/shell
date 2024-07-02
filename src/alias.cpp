#include <alias.h>

void Aliases::set(const std::string& name, const std::string& value)
{
    data[name] = value;
}

bool Aliases::get(const std::string& name, std::string& value)
{
    if (contains(name))
    {
        value = data[name];
        return true;
    }

    return false;
}

bool Aliases::unset(const std::string& name)
{
    if (!contains(name))
        return false;

    data.erase(name);

    return true;
}

bool Aliases::contains(const std::string& name)
{
    return data.find(name) != data.end();
}