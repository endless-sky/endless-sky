/* SpaceportPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SpaceportPanel.h"

#include "Color.h"
#include "FontSet.h"
#include "GameData.h"
#include "Interface.h"
#include "News.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "UI.h"

using namespace std;



SpaceportPanel::SpaceportPanel(PlayerInfo &player)
	: player(player)
{
	SetTrapAllEvents(false);
	
	text.SetFont(FontSet::Get(14));
	text.SetAlignment(WrappedText::JUSTIFIED);
	text.SetWrapWidth(480);
	text.Wrap(player.GetPlanet()->SpaceportDescription());
	
	newsMessage.SetFont(FontSet::Get(14));
}



void SpaceportPanel::UpdateNews()
{
	const News *news = GameData::PickNews(player.GetPlanet());
	if(!news)
		return;
	
	hasNews = true;
	// Randomly pick which portrait is to be shown.
	auto portrait = news->Portrait();
	
	// Ensure we only display one name for a given portrait.
	const auto it = displayedProfessions.find(portrait);
	auto name = string{};
	if(it == displayedProfessions.end())
	{
		name = news->Name();
		displayedProfessions.emplace(portrait, name);
	}
	else
		name = it->second;
	
	// Clear any previous information
	hasPortrait = false;
	newsInfo.SetString("name portrait", "");
	newsInfo.SetString("name", "");
	newsInfo.SetSprite("portrait", nullptr);
	// Query the news interface to find out the wrap width.
	// TODO: Allow Interface to handle wrapped text directly.
	const Interface *interface = GameData::Interfaces().Get("news");
	// Cache the randomly picked results until the next update is requested.
	if(portrait)
	{
		hasPortrait = true;
		newsInfo.SetString("name portrait", name + ':');
		newsInfo.SetSprite("portrait", portrait);
		newsMessage.SetWrapWidth(interface->GetBox("message portrait").Width());
	}
	else
	{
		newsInfo.SetString("name", name + ':');
		newsMessage.SetWrapWidth(interface->GetBox("message").Width());
	}
	
	newsMessage.Wrap('"' + news->Message() + '"');
}



void SpaceportPanel::Step()
{
	if(GetUI()->IsTop(this))
	{
		Mission *mission = player.MissionToOffer(Mission::SPACEPORT);
		// Special case: if the player somehow got to the spaceport before all
		// landing missions were offered, they can still be offered here:
		if(!mission)
			mission = player.MissionToOffer(Mission::LANDING);
		if(mission)
			mission->Do(Mission::OFFER, player, GetUI());
		else
			player.HandleBlockedMissions(Mission::SPACEPORT, GetUI());
	}
}



void SpaceportPanel::Draw()
{
	if(player.IsDead())
		return;
	
	text.Draw(Point(-300., 80.), *GameData::Colors().Get("bright"));
	
	if(hasNews)
	{
		const Interface *interface = GameData::Interfaces().Get("news");
		interface->Draw(newsInfo);
		// Depending on if the news has a portrait, the interface message that
		// gets filled in changes.
		string message = "message";
		if(hasPortrait)
			message += " portrait";
		newsMessage.Draw(interface->GetBox(message).TopLeft(), *GameData::Colors().Get("medium"));
	}
}
