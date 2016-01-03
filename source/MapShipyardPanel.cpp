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
#include "DotShader.h"
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
#include "ShipInfoDisplay.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

#include <algorithm>
#include <set>

using namespace std;

namespace {
	static const vector<string> CATEGORIES = {
		"Transport",
		"Luxury Transport",
		"Light Freighter",
		"Heavy Freighter",
		"Interceptor",
		"Light Warship",
		"Medium Warship",
		"Heavy Warship",
		"Fighter",
		"Drone"
	};
	static const double ICON_HEIGHT = 90.;
	static const double PAD = 8.;
	static const int WIDTH = 270;
}



MapShipyardPanel::MapShipyardPanel(PlayerInfo &player)
	: MapPanel(player, SHOW_SPECIAL)
{
	Init();
}



MapShipyardPanel::MapShipyardPanel(const MapPanel &panel)
	: MapPanel(panel)
{
	SetCommodity(SHOW_SPECIAL);
	Init();
}



void MapShipyardPanel::Draw() const
{
	MapPanel::Draw();
	
	DrawKey();
	DrawPanel();
	DrawItems();
	
	Information info;
	info.SetCondition("is shipyards");
	if(ZoomIsMax())
		info.SetCondition("max zoom");
	if(ZoomIsMin())
		info.SetCondition("min zoom");
	const Interface *interface = GameData::Interfaces().Get("map buttons");
	interface->Draw(info);
	
	if(selected)
	{
		ShipInfoDisplay infoDisplay(*selected);
		ShipInfoDisplay compareDisplay;
		if(compare)
			compareDisplay.Update(*compare);
		int infoHeight = max(infoDisplay.AttributesHeight(), 120);
		int compareHeight = compare ? max(compareDisplay.AttributesHeight(), 120) : 0;
		
		Color back(.125, 1.);
		Point size(infoDisplay.PanelWidth(), infoHeight + compareHeight);
		Point topLeft(Screen::Right() - size.X(), Screen::Top());
		FillShader::Fill(topLeft + .5 * size, size, back);
		
		const Sprite *left = SpriteSet::Get("ui/left edge");
		const Sprite *bottom = SpriteSet::Get("ui/bottom edge");
		const Sprite *box = SpriteSet::Get("ui/thumb box");
		Point leftPos = topLeft + Point(
			-.5 * left->Width(),
			size.Y() - .5 * left->Height());
		SpriteShader::Draw(left, leftPos);
		// The top left corner of the bottom sprite should be 10 x units right
		// of the bottom left corner of the left edge sprite.
		Point bottomPos = leftPos + Point(
			10. + .5 * (bottom->Width() - left->Width()),
			.5 * (left->Height() + bottom->Height()));
		SpriteShader::Draw(bottom, bottomPos);
		
		Point iconOffset(-.5 * ICON_HEIGHT, .5 * ICON_HEIGHT);
		const Sprite *sprite = selected->GetSprite().GetSprite();
		int swizzle = GameData::PlayerGovernment()->GetSwizzle();
		if(sprite)
		{
			double scale = min(.5, ICON_HEIGHT / sprite->Height());
			SpriteShader::Draw(box, topLeft + iconOffset + Point(-15., 5.));
			SpriteShader::Draw(sprite, topLeft + iconOffset + Point(0., 5.), scale, swizzle);
		}
		infoDisplay.DrawAttributes(topLeft);
		
		if(compare)
		{
			topLeft.Y() += infoHeight;
			
			sprite = compare->GetSprite().GetSprite();
			if(sprite)
			{
				double scale = min(.5, ICON_HEIGHT / sprite->Height());
				SpriteShader::Draw(box, topLeft + iconOffset + Point(-15., 5.));
				SpriteShader::Draw(sprite, topLeft + iconOffset + Point(0., 5.), scale, swizzle);
			}
			
			Color line(.5);
			size.Y() = 1.;
			FillShader::Fill(topLeft + .5 * size - Point(0., 1.), size, line);
			compareDisplay.DrawAttributes(topLeft + Point(0., 10.));
		}
	}
}



// Only override the ones you need; the default action is to return false.
bool MapShipyardPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(command.Has(Command::MAP) || key == 'd' || key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
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
	else if((key == SDLK_DOWN || key == SDLK_UP) && !zones.empty())
	{
		// First, find the currently selected item, if any
		auto it = zones.begin();
		if(!selected)
		{
			if(key == SDLK_DOWN)
				it = --zones.end();
		}
		else
		{
			for( ; it != zones.end() - 1; ++it)
				if(it->Value() == selected)
					break;
		}
		if(key == SDLK_DOWN)
		{
			++it;
			if(it == zones.end())
				it = zones.begin();
		}
		else
		{
			if(it == zones.begin())
				it = zones.end();
			--it;
		}
		double top = (it->Center() - it->Size()).Y();
		double bottom = (it->Center() + it->Size()).Y();
		if(bottom > Screen::Bottom())
			scroll += Screen::Bottom() - bottom;
		if(top < Screen::Top())
			scroll += Screen::Top() - top;
		selected = it->Value();
	}
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
	{
		scroll += (Screen::Height() - 100) * ((key == SDLK_PAGEUP) - (key == SDLK_PAGEDOWN));
		scroll = min(0, max(-maxScroll, scroll));
	}
	else if(key == '+' || key == '=')
		ZoomMap();
	else if(key == '-')
		UnzoomMap();
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
		
		bool isCompare = (SDL_GetModState() & KMOD_SHIFT);
		if(!isCompare)
			selected = nullptr;
		compare = nullptr;
		
		for(const ClickZone<const Ship *> &zone : zones)
			if(zone.Contains(point))
				(isCompare ? compare : selected) = zone.Value();
		
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
	if(!system)
		return 0.;
	
	double value = -.5;
	for(const StellarObject &object : system->Objects())
		if(object.GetPlanet() && !object.GetPlanet()->Shipyard().empty())
			value = 0.;

	if(!selected)
		return value;
	
	for(const StellarObject &object : system->Objects())
		if(object.GetPlanet() && object.GetPlanet()->Shipyard().Has(selected))
			return 1.;
	
	return value;
}



void MapShipyardPanel::Init()
{
	catalog.clear();
	set<const Ship *> seen;
	for(const auto &it : GameData::Planets())
		if(player.HasVisited(it.second.GetSystem()))
			for(const Ship *ship : it.second.Shipyard())
				if(seen.find(ship) == seen.end())
				{
					catalog[ship->Attributes().Category()].push_back(ship);
					seen.insert(ship);
				}
	
	for(auto &it : catalog)
		sort(it.second.begin(), it.second.end(),
			[](const Ship *a, const Ship *b) {return a->ModelName() < b->ModelName();});
}



void MapShipyardPanel::DrawKey() const
{
	const Sprite *back = SpriteSet::Get("ui/sales key");
	SpriteShader::Draw(back, Screen::TopLeft() + Point(WIDTH + 10, 0) + .5 * Point(back->Width(), back->Height()));
	
	Color bright(.6, .6);
	Color dim(.3, .3);
	const Font &font = FontSet::Get(14);
	
	Point pos(Screen::Left() + 50. + WIDTH, Screen::Top() + 12.);
	Point textOff(10., -.5 * font.Height());
	
	static const string LABEL[] = {
		"Has no shipyard",
		"Has shipyard",
		"Sells this ship"
	};
	static const double VALUE[] = {
		-.5,
		0.,
		1.
	};
	
	for(int i = 0; i < 3; ++i)
	{
		bool isSelected = (selectedSystem && VALUE[i] == SystemValue(selectedSystem));
		DotShader::Draw(pos, OUTER, INNER, MapColor(VALUE[i]));
		font.Draw(LABEL[i], pos + textOff, isSelected ? bright : dim);
		pos.Y() += 20.;
	}
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
	Color dimTextColor = *GameData::Colors().Get("dim");
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
		
		for(const Ship *ship : it->second)
		{
			if(corner.Y() < Screen::Bottom() && corner.Y() + ICON_HEIGHT >= Screen::Top())
			{
				const string &name = ship->ModelName();
				if(ship == selected)
					FillShader::Fill(corner + .5 * blockSize, blockSize, selectionColor);
				
				const Sprite *sprite = ship->GetSprite().GetSprite();
				if(sprite)
				{
					int swizzle = GameData::PlayerGovernment()->GetSwizzle();
					double scale = min(.5, ICON_HEIGHT / sprite->Height());
					SpriteShader::Draw(sprite, corner + iconOffset, scale, swizzle);
				}
				
				bool isForSale = true;
				if(selectedSystem)
				{
					isForSale = false;
					for(const StellarObject &object : selectedSystem->Objects())
						if(object.GetPlanet() && object.GetPlanet()->Shipyard().Has(ship))
							isForSale = true;
				}
				
				const Color &color = (isForSale ? textColor : dimTextColor);
				font.Draw(name, corner + nameOffset, color);
				
				string price = Format::Number(ship->Cost()) + " credits";
				font.Draw(price, corner + priceOffset, color);
				
				string shields = Format::Number(ship->Attributes().Get("shields")) + " shields / ";
				shields += Format::Number(ship->Attributes().Get("hull")) + " hull";
				font.Draw(shields, corner + shieldsOffset, color);
			}
			zones.emplace_back(corner + .5 * blockSize, blockSize, ship);
			corner += Point(0., ICON_HEIGHT);
		}
	}
	maxScroll = corner.Y() - scroll - .5 * Screen::Height();
}
