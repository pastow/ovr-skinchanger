#pragma once

#include <string>

namespace Config
{
    bool Init();

    std::string Get(const char* Key);
    void Set(const char* Key, const std::string& Value);
}
