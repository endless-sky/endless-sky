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
		"Engines"
	};
}



OutfitterPanel::OutfitterPanel(const GameData &data, PlayerInfo &player)
	: ShopPanel(data, player, CATEGORIES)
{
	for(const pair<string, Outfit> &it : data.Outfits())
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
	const Outfit *outfit = data.Outfits().Get(name);
	if(!planet->Outfitter().Has(outfit) && !playerShip->OutfitCount(outfit) && !available[outfit])
		return false;
	
	bool isSelected = (outfit == selectedOutfit);
	bool isOwned = playerShip && playerShip->OutfitCount(outfit);
	DrawOutfit(*outfit, point, isSelected, isOwned);
	
	zones.emplace_back(point.X(), point.Y(), SHIP_SIZE / 2, SHIP_SIZE / 2, outfit);
	
	if(playerShip)
	{
		int count = playerShip->OutfitCount(outfit);
		if(count)
			FontSet::Get(14).Draw(to_string(count),
				point + Point(-OUTFIT_SIZE / 2 + 20, OUTFIT_SIZE / 2 - 40),
				Color(.8, 0.));
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
	return planet && playerShip && selectedOutfit
		&& (planet->Outfitter().Has(selectedOutfit) || available[selectedOutfit])
		&& selectedOutfit->Cost() < player.Accounts().Credits()
		&& playerShip->Attributes().CanAdd(*selectedOutfit, 1);
}



void OutfitterPanel::Buy()
{
	player.Accounts().AddCredits(-selectedOutfit->Cost());
	playerShip->AddOutfit(selectedOutfit, 1);
	--available[selectedOutfit];
}



bool OutfitterPanel::CanSell() const
{
	return planet && playerShip && selectedOutfit
		&& playerShip->OutfitCount(selectedOutfit)
		&& playerShip->Attributes().CanAdd(*selectedOutfit, -1);
}



void OutfitterPanel::Sell()
{
	player.Accounts().AddCredits(selectedOutfit->Cost());
	playerShip->AddOutfit(selectedOutfit, -1);
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
				*data.Conversations().Get("flight check: no thrusters")));
			return false;
		}
		if(attributes.Get("thrusting energy") > energy)
		{
			GetUI()->Push(new ConversationPanel(player,
				*data.Conversations().Get("flight check: no thruster energy")));
			return false;
		}
		if(!attributes.Get("turn"))
		{
			GetUI()->Push(new ConversationPanel(player,
				*data.Conversations().Get("flight check: no steering")));
			return false;
		}
		if(attributes.Get("turning energy") > energy)
		{
			GetUI()->Push(new ConversationPanel(player,
				*data.Conversations().Get("flight check: no steering energy")));
			return false;
		}
		if(attributes.Get("heat generation") * 10. > player.GetShip()->Mass())
		{
			GetUI()->Push(new ConversationPanel(player,
				*data.Conversations().Get("flight check: overheating")));
			return false;
		}
	}
	return true;
}



int OutfitterPanel::Modifier() const
{
	SDLMod mod = SDL_GetModState();
	
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
