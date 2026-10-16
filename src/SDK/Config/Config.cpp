#include "Config.h"

#include "SDK/Files/Paths.h"
#include "SDK/Logger/Logger.h"

#include <fstream>
#include <map>

namespace
{
    constexpr const char* ConfigFileName = "ovr_skins.cfg";

    std::map<std::string, std::string> Values;

    void Save()
    {
        std::string FilePath = Paths::Resolve(ConfigFileName);
        std::ofstream File(FilePath, std::ios::trunc);
        if (!File)
        {
            LogE("Failed to write %s", FilePath.c_str());
            return;
        }

        for (const auto& [Key, Value] : Values)
            File << Key << '=' << Value << '\n';
    }
}

bool Config::Init()
{
    Values.clear();

    std::string FilePath = Paths::Resolve(ConfigFileName);
    std::ifstream File(FilePath);
    if (!File)
    {
        Log("Config %s not found, starting empty", FilePath.c_str());
        return true;
    }

    std::string Line;
    while (std::getline(File, Line))
    {
        if (!Line.empty() && Line.back() == '\r')
            Line.pop_back();

        std::size_t Separator = Line.find('=');
        if (Separator == std::string::npos || Separator == 0)
            continue;
        Values[Line.substr(0, Separator)] = Line.substr(Separator + 1);
    }

    Log("Config %s loaded, %zu keys", FilePath.c_str(), Values.size());
    return true;
}

std::string Config::Get(const char* Key)
{
    auto Entry = Values.find(Key);
    return Entry != Values.end() ? Entry->second : std::string();
}

void Config::Set(const char* Key, const std::string& Value)
{
    if (Value.empty())
        Values.erase(Key);
    else
        Values[Key] = Value;
    Save();
}
