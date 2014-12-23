/* OutfitterPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "OutfitterPanel.h"

#include "Color.h"
#include "ConversationPanel.h"
#include "DistanceMap.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "OutfitInfoDisplay.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "ShipInfoDisplay.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

using namespace std;

namespace {
	static const vector<string> CATEGORIES = {
		"Guns",
		"Missiles",
		"Turrets",
		"Systems",
		"Power",
		"Engines",
		"Hand to Hand",
		"Special"
	};
}



OutfitterPanel::OutfitterPanel(PlayerInfo &player)
	: ShopPanel(player, CATEGORIES)
{
	for(const pair<string, Outfit> &it : GameData::Outfits())
		catalog[it.second.Category()].insert(it.first);
}



int OutfitterPanel::TileSize() const
{
	return OUTFIT_SIZE;
}



int OutfitterPanel::DrawPlayerShipInfo(const Point &point) const
{
	ShipInfoDisplay info(*playerShip);
	info.DrawAttributes(point);
	
	return info.AttributesHeight();
}



bool OutfitterPanel::DrawItem(const string &name, const Point &point) const
{
	const Outfit *outfit = GameData::Outfits().Get(name);
	if(!planet->Outfitter().Has(outfit)
			&& (!playerShip || !playerShip->OutfitCount(outfit))
			&& !available[outfit] && !player.Cargo().Get(outfit))
		return false;
	
	bool isSelected = (outfit == selectedOutfit);
	bool isOwned = playerShip && playerShip->OutfitCount(outfit);
	DrawOutfit(*outfit, point, isSelected, isOwned);
	
	zones.emplace_back(point.X(), point.Y(), SHIP_SIZE / 2, SHIP_SIZE / 2, outfit);
	
	// Check if this outfit is a "license".
	bool isLicense = IsLicense(name);
	int mapSize = outfit->Get("map");
	
	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	if(playerShip || isLicense || mapSize)
	{
		int count = playerShip->OutfitCount(outfit);
		if(isLicense)
			count = player.GetCondition(LicenseName(name));
		if(mapSize)
			count = HasMapped(mapSize);
		if(count)
			font.Draw("installed: " + to_string(count),
				point + Point(-OUTFIT_SIZE / 2 + 20, OUTFIT_SIZE / 2 - 38),
				bright);
	}
	if(player.Cargo().Get(outfit))
	{
		string count = "in cargo: " + to_string(player.Cargo().Get(outfit));
		Point pos = point + Point(
			OUTFIT_SIZE / 2 - 20 - font.Width(count),
			OUTFIT_SIZE / 2 - 24);
		font.Draw(count, pos, bright);
	}
	
	return true;
}



int OutfitterPanel::DividerOffset() const
{
	return 80;
}



int OutfitterPanel::DetailWidth() const
{
	return 3 * OutfitInfoDisplay::PanelWidth();
}



int OutfitterPanel::DrawDetails(const Point &center) const
{
	OutfitInfoDisplay info(*selectedOutfit);
	Point offset(info.PanelWidth(), 0.);
	
	info.DrawDescription(center - offset * 1.5);
	info.DrawRequirements(center - offset * .5);
	info.DrawAttributes(center + offset * .5);
	
	return info.MaximumHeight() + 40;
}



bool OutfitterPanel::CanBuy() const
{
	if(!planet || !selectedOutfit || !playerShip)
		return false;
	
	if(!playerShip->Attributes().CanAdd(*selectedOutfit, 1))
		return false;
	
	// If you have this in your cargo hold, installing it is free.
	if(player.Cargo().Get(selectedOutfit))
		return true;
	
	if(!(planet->Outfitter().Has(selectedOutfit) || available[selectedOutfit]))
		return false;
	
	int mapSize = selectedOutfit->Get("map");
	if(mapSize > 0 && HasMapped(mapSize))
		return false;
	
	if(HasLicense(selectedOutfit->Name()))
		return false;
	
	return selectedOutfit->Cost() < player.Accounts().Credits();
}



void OutfitterPanel::Buy()
{
	// Special case: maps.
	int mapSize = selectedOutfit->Get("map");
	if(mapSize > 0)
	{
		if(!HasMapped(mapSize))
		{
			DistanceMap distance(player.GetSystem(), mapSize);
			for(const auto &it : distance.Distances())
				if(!player.HasVisited(it.first))
					player.Visit(it.first);
			player.Accounts().AddCredits(-selectedOutfit->Cost());
		}
		return;
	}
	
	// Special case: licenses.
	if(IsLicense(selectedOutfit->Name()))
	{
		int &entry = player.Conditions()[LicenseName(selectedOutfit->Name())];
		if(entry <= 0)
		{
			entry = true;
			player.Accounts().AddCredits(-selectedOutfit->Cost());
		}
		return;
	}
	
	if(player.Cargo().Get(selectedOutfit))
		player.Cargo().Transfer(selectedOutfit, 1);
	else
	{
		player.Accounts().AddCredits(-selectedOutfit->Cost());
		--available[selectedOutfit];
	}
	playerShip->AddOutfit(selectedOutfit, 1);
}



bool OutfitterPanel::CanSell() const
{
	if(!planet || !selectedOutfit)
		return false;
	
	if(player.Cargo().Get(selectedOutfit))
		return true;
	
	if(!(playerShip && playerShip->OutfitCount(selectedOutfit)))
		return false;
	
	return playerShip->Attributes().CanAdd(*selectedOutfit, -1);
}



void OutfitterPanel::Sell()
{
	if(player.Cargo().Get(selectedOutfit))
		player.Cargo().Transfer(selectedOutfit, 1);
	else
		playerShip->AddOutfit(selectedOutfit, -1);
	
	player.Accounts().AddCredits(selectedOutfit->Cost());
	++available[selectedOutfit];
}



bool OutfitterPanel::FlightCheck()
{
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		// Skip any ships that are "absent" for whatever reason.
		if(ship->GetSystem() != player.GetSystem())
			continue;
		
		playerShip = &*ship;
		
		const Outfit &attributes = player.GetShip()->Attributes();
		double energy = attributes.Get("energy generation") + attributes.Get("energy capacity");
		if(!attributes.Get("thrust"))
		{
			GetUI()->Push(new ConversationPanel(player,
				*GameData::Conversations().Get("flight check: no thrusters")));
			return false;
		}
		if(attributes.Get("thrusting energy") > energy)
		{
			GetUI()->Push(new ConversationPanel(player,
				*GameData::Conversations().Get("flight check: no thruster energy")));
			return false;
		}
		if(!attributes.Get("turn"))
		{
			GetUI()->Push(new ConversationPanel(player,
				*GameData::Conversations().Get("flight check: no steering")));
			return false;
		}
		if(attributes.Get("turning energy") > energy)
		{
			GetUI()->Push(new ConversationPanel(player,
				*GameData::Conversations().Get("flight check: no steering energy")));
			return false;
		}
		if(attributes.Get("heat generation") * 10. > player.GetShip()->Mass())
		{
			GetUI()->Push(new ConversationPanel(player,
				*GameData::Conversations().Get("flight check: overheating")));
			return false;
		}
	}
	return true;
}



int OutfitterPanel::Modifier() const
{
	SDL_Keymod mod = SDL_GetModState();
	
	int modifier = 1;
	if(mod & KMOD_CTRL)
		modifier *= 20;
	if(mod & KMOD_SHIFT)
		modifier *= 5;
	
	return modifier;
}



	
void OutfitterPanel::DrawOutfit(const Outfit &outfit, const Point &center, bool isSelected, bool isOwned)
{
	const Sprite *thumbnail = outfit.Thumbnail();
	const Sprite *back = SpriteSet::Get(
		isSelected ? "ui/outfitter selected" : "ui/outfitter unselected");
	SpriteShader::Draw(back, center);
	SpriteShader::Draw(thumbnail, center);
	
	// Draw the outfit name.
	const string &name = outfit.Name();
	const Font &font = FontSet::Get(14);
	Point offset(-.5f * font.Width(name), -.5f * OUTFIT_SIZE + 10.f);
	font.Draw(name, center + offset, Color((isSelected | isOwned) ? .8 : .5, 0.));
}



bool OutfitterPanel::HasMapped(int mapSize) const
{
	DistanceMap distance(player.GetSystem(), mapSize);
	for(const auto &it : distance.Distances())
		if(!player.HasVisited(it.first))
			return false;
	
	return true;
}



bool OutfitterPanel::IsLicense(const string &name) const
{
	static const string &LICENSE = " License";
	if(name.length() < LICENSE.length())
		return false;
	if(name.compare(name.length() - LICENSE.length(), LICENSE.length(), LICENSE))
		return false;
	
	return true;
}



bool OutfitterPanel::HasLicense(const string &name) const
{
	return (IsLicense(name) && player.GetCondition(LicenseName(name)) > 0);
}



string OutfitterPanel::LicenseName(const string &name) const
{
	static const string &LICENSE = " License";
	return "license: " + name.substr(0, name.length() - LICENSE.length());
}
