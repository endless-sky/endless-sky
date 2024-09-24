#pragma once
#include <string>
#include "PlayerInfo.h"

class DiscordRPC
{
public:
    static void Initialize();
    static void Update(const PlayerInfo &playerInfo);
};
