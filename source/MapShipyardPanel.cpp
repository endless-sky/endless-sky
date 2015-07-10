/* MapShipyardPanel.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MapShipyardPanel.h"

#include "Color.h"
#include "Command.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "MapDetailPanel.h"
#include "MapOutfitterPanel.h"
#include "MissionPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

using namespace std;

namespace {
	static const vector<string> CATEGORIES = {
		"Transport",
		"Light Freighter",
		"Heavy Freighter",
		"Interceptor",
		"Light Warship",
		"Heavy Warship",
		"Fighter",
		"Drone"
	};
	static const double ICON_HEIGHT = 90.;
	static const double PAD = 8.;
	static const int WIDTH = 270;
}



MapShipyardPanel::MapShipyardPanel(PlayerInfo &player)
	: MapPanel(player, -5)
{
	Init();
}



MapShipyardPanel::MapShipyardPanel(const MapPanel &panel)
	: MapPanel(panel)
{
	SetCommodity(-5);
	Init();
}



void MapShipyardPanel::Draw() const
{
	MapPanel::Draw();
	
	DrawPanel();
	DrawItems();
	
	Information info;
	info.SetCondition("is shipyards");
	const Interface *interface = GameData::Interfaces().Get("map buttons");
	interface->Draw(info);
}



// Only override the ones you need; the default action is to return false.
bool MapShipyardPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(command.Has(Command::MAP) || key == 'd' || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == 'o')
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapOutfitterPanel(*this));
	}
	else if(key == 'i')
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MissionPanel(*this));
	}
	else if(key == 'p')
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapDetailPanel(*this));
	}
	else
		return false;
	
	return true;
}



bool MapShipyardPanel::Click(int x, int y)
{
	const Interface *interface = GameData::Interfaces().Get("map buttons");
	char key = interface->OnClick(Point(x, y));
	if(key)
		return DoKey(key);
	
	if(x < Screen::Left() + WIDTH)
	{
		Point point(x, y);
		
		selected = nullptr;
		for(const ClickZone<const Ship *> &zone : zones)
			if(zone.Contains(point))
				selected = zone.Value();
		
		return true;
	}
	else
		return MapPanel::Click(x, y);
}



bool MapShipyardPanel::Hover(int x, int y)
{
	isDragging = (x < Screen::Left() + WIDTH);
	if(isDragging)
		return true;
	
	return MapPanel::Hover(x, y);
}



bool MapShipyardPanel::Drag(int dx, int dy)
{
	if(isDragging)
		scroll = max(-maxScroll, min(0, scroll + dy));
	else
		return MapPanel::Drag(dx, dy);
	
	return true;
}



bool MapShipyardPanel::Scroll(int dx, int dy)
{
	if(isDragging)
		scroll = max(-maxScroll, min(0, scroll + 50 * dy));
	else
		return MapPanel::Scroll(dx, dy);
	
	return true;
}



double MapShipyardPanel::SystemValue(const System *system) const
{
	if(!system || !selected)
		return 0.;
	
	for(const StellarObject &object : system->Objects())
		if(object.GetPlanet() && object.GetPlanet()->Shipyard().Has(selected))
			return 1.;
	
	return 0.;
}



void MapShipyardPanel::Init()
{
	catalog.clear();
	for(const auto &it : GameData::Planets())
		if(player.HasVisited(it.second.GetSystem()))
			for(const Ship *ship : it.second.Shipyard())
				catalog[ship->Attributes().Category()].insert(ship->ModelName());
}



void MapShipyardPanel::DrawPanel() const
{
	Color back(0.125, 1.);
	FillShader::Fill(
		Point(Screen::Width() * -.5 + WIDTH * .5, 0.),
		Point(WIDTH, Screen::Height()), 
		back);
	
	const Sprite *edgeSprite = SpriteSet::Get("ui/right edge");
	if(edgeSprite->Height())
	{
		int steps = Screen::Height() / edgeSprite->Height();
		for(int y = -steps; y <= steps; ++y)
		{
			Point pos(
				Screen::Width() * -.5 + WIDTH + .5 * edgeSprite->Width(),
				y * edgeSprite->Height());
			SpriteShader::Draw(edgeSprite, pos);
		}
	}
}



void MapShipyardPanel::DrawItems() const
{
	const Font &bigFont = FontSet::Get(18);
	const Font &font = FontSet::Get(14);
	Color textColor = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	Color selectionColor(0., .3);
	
	Point corner = Screen::TopLeft() + Point(0, scroll);
	double firstY = corner.Y();
	Point iconOffset(.5 * ICON_HEIGHT, .5 * ICON_HEIGHT);
	Point nameOffset(ICON_HEIGHT, .5 * ICON_HEIGHT - PAD - 1.5 * font.Height());
	Point priceOffset(ICON_HEIGHT, nameOffset.Y() + font.Height() + PAD);
	Point shieldsOffset(ICON_HEIGHT, priceOffset.Y() + font.Height() + PAD);
	Point blockSize(WIDTH, ICON_HEIGHT);
	
	zones.clear();
	for(const string &category : CATEGORIES)
	{
		auto it = catalog.find(category);
		if(it == catalog.end())
			continue;
		
		if(corner.Y() != firstY)
			corner.Y() += 50.;
		bigFont.Draw(category, corner + Point(5., 15.), bright);
		corner += Point(0., 40.);
		
		for(const string &name : it->second)
		{
			if(corner.Y() < Screen::Bottom() && corner.Y() + ICON_HEIGHT >= Screen::Top())
			{
				const Ship *ship = GameData::Ships().Get(name);
				if(ship == selected)
					FillShader::Fill(corner + .5 * blockSize, blockSize, selectionColor);
				
				const Sprite *sprite = ship->GetSprite().GetSprite();
				if(sprite)
				{
					int swizzle = GameData::PlayerGovernment()->GetSwizzle();
					double scale = min(.5, ICON_HEIGHT / sprite->Height());
					SpriteShader::Draw(sprite, corner + iconOffset, scale, swizzle);
				}
				
				zones.emplace_back(corner + .5 * blockSize, blockSize, ship);
				
				font.Draw(name, corner + nameOffset, textColor);
				
				string price = Format::Number(ship->Cost()) + " credits";
				font.Draw(price, corner + priceOffset, textColor);
				
				string shields = Format::Number(ship->Attributes().Get("shields")) + " shields / ";
				shields += Format::Number(ship->Attributes().Get("hull")) + " hull";
				font.Draw(shields, corner + shieldsOffset, textColor);
			}
			corner += Point(0., ICON_HEIGHT);
		}
	}
	maxScroll = corner.Y() - scroll - .5 * Screen::Height();
}
