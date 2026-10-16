#pragma once

#include <cstdint>

class CNetMessage;

namespace EmblemEffect
{
    void Update();
    void OnProjectile(CNetMessage* Message, bool Located);
}
