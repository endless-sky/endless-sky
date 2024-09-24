#include "DiscordRPC.h"
#include "discord_rpc/discord_rpc.h"
#include <iostream>
#include "PlayerInfo.h"
#include "System.h"

auto start = time(nullptr);

void DiscordRPC::Initialize()
{
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));

    Discord_Initialize("1287535358178758708", &handlers, 1, nullptr);
    std::cout << "Discord RPC Initialized." << std::endl;
}

void DiscordRPC::Update(const PlayerInfo &playerInfo)
{
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

    std::string systemName = "Unknown";

    const System *currentSystem = playerInfo.GetSystem();
    if (currentSystem)
        systemName = currentSystem->Name();

    std::string detailsMessage = "Exploring System: " + systemName;

    discordPresence.details = detailsMessage.c_str();
    discordPresence.largeImageKey = "endless_sky_icon";
    discordPresence.startTimestamp = start;

    Discord_UpdatePresence(&discordPresence);
}
