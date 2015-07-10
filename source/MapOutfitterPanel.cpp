/* MapOutfitterPanel.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MapOutfitterPanel.h"

#include "Color.h"
#include "Command.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "MapDetailPanel.h"
#include "MapShipyardPanel.h"
#include "MissionPanel.h"
#include "Outfit.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

#include <cmath>

using namespace std;

namespace {
	static const vector<string> CATEGORIES = {
		"Guns",
		"Turrets",
		"Secondary Weapons",
		"Ammunition",
		"Systems",
		"Power",
		"Engines",
		"Hand to Hand",
		"Special"
	};
	static const double ICON_HEIGHT = 90.;
	static const double PAD = 8.;
	static const int WIDTH = 270;
}



MapOutfitterPanel::MapOutfitterPanel(PlayerInfo &player)
	: MapPanel(player, -5)
{
	Init();
}



MapOutfitterPanel::MapOutfitterPanel(const MapPanel &panel)
	: MapPanel(panel)
{
	SetCommodity(-5);
	Init();
}



void MapOutfitterPanel::Draw() const
{
	MapPanel::Draw();
	
	DrawPanel();
	DrawItems();
	
	Information info;
	info.SetCondition("is outfitters");
	const Interface *interface = GameData::Interfaces().Get("map buttons");
	interface->Draw(info);
}



// Only override the ones you need; the default action is to return false.
bool MapOutfitterPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(command.Has(Command::MAP) || key == 'd' || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == 's')
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapShipyardPanel(*this));
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



bool MapOutfitterPanel::Click(int x, int y)
{
	const Interface *interface = GameData::Interfaces().Get("map buttons");
	char key = interface->OnClick(Point(x, y));
	if(key)
		return DoKey(key);
	
	if(x < Screen::Left() + WIDTH)
	{
		Point point(x, y);
		
		selected = nullptr;
		for(const ClickZone<const Outfit *> &zone : zones)
			if(zone.Contains(point))
				selected = zone.Value();
		
		return true;
	}
	else
		return MapPanel::Click(x, y);
}



bool MapOutfitterPanel::Hover(int x, int y)
{
	isDragging = (x < Screen::Left() + WIDTH);
	if(isDragging)
		return true;
	
	return MapPanel::Hover(x, y);
}



bool MapOutfitterPanel::Drag(int dx, int dy)
{
	if(isDragging)
		scroll = min(0, max(-maxScroll, scroll + dy));
	else
		return MapPanel::Drag(dx, dy);
	
	return true;
}



bool MapOutfitterPanel::Scroll(int dx, int dy)
{
	if(isDragging)
		scroll = min(0, max(-maxScroll, scroll + 50 * dy));
	else
		return MapPanel::Scroll(dx, dy);
	
	return true;
}



double MapOutfitterPanel::SystemValue(const System *system) const
{
	if(!system || !selected)
		return 0.;
	
	for(const StellarObject &object : system->Objects())
		if(object.GetPlanet() && object.GetPlanet()->Outfitter().Has(selected))
			return 1.;
	
	return 0.;
}



void MapOutfitterPanel::Init()
{
	catalog.clear();
	for(const auto &it : GameData::Planets())
		if(player.HasVisited(it.second.GetSystem()))
			for(const Outfit *outfit : it.second.Outfitter())
				catalog[outfit->Category()].insert(outfit->Name());
}



void MapOutfitterPanel::DrawPanel() const
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



void MapOutfitterPanel::DrawItems() const
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
	Point sizeOffset(ICON_HEIGHT, priceOffset.Y() + font.Height() + PAD);
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
				const Outfit *outfit = GameData::Outfits().Get(name);
				if(outfit == selected)
					FillShader::Fill(corner + .5 * blockSize, blockSize, selectionColor);
				
				const Sprite *sprite = outfit->Thumbnail();
				if(sprite)
				{
					double scale = min(.5, ICON_HEIGHT / sprite->Height());
					SpriteShader::Draw(sprite, corner + iconOffset, scale);
				}
				
				zones.emplace_back(corner + .5 * blockSize, blockSize, outfit);
				
				font.Draw(name, corner + nameOffset, textColor);
				
				string price = Format::Number(outfit->Cost()) + " credits";
				font.Draw(price, corner + priceOffset, textColor);
				
				double space = -outfit->Get("outfit space");
				string size = Format::Number(space);
				size += (abs(space) == 1. ? " ton" : " tons");
				if(space && -outfit->Get("weapon capacity") == space)
					size += " of weapon space";
				else if(space && -outfit->Get("engine capacity") == space)
					size += " of engine space";
				else
					size += " of outfit space";
				font.Draw(size, corner + sizeOffset, textColor);
			}
			corner += Point(0., ICON_HEIGHT);
		}
	}
	maxScroll = corner.Y() - scroll - .5 * Screen::Height();
}
