#pragma once

class CGCClient
{
public:
    static bool Init();
    static CGCClient* Get();

    void* SteamGameCoordinator() const;
};
