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
#include "OutfitInfoDisplay.h"

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

#include <algorithm>
#include <cmath>
#include <set>

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
	

	if(selected and !compare)
	{
		OutfitInfoDisplay infoDisplay(*selected);
		
		Color back(.125, 1.);
		Point size(infoDisplay.PanelWidth(), infoDisplay.AttributesHeight());
		Point topLeft(Screen::Right() - size.X(), Screen::Top());
		FillShader::Fill(topLeft + .5 * size, size, back);
		
		const Sprite *left = SpriteSet::Get("ui/left edge");
		const Sprite *bottom = SpriteSet::Get("ui/bottom edge");
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
		infoDisplay.DrawAttributes(topLeft + Point(0., 10.), false);
	}
	else if (selected)
	{
		OutfitInfoDisplay infoDisplay(*selected, *compare);
		
		Color back(.125, 1.);
		Point size(infoDisplay.PanelWidth(), infoDisplay.AttributesHeight());
		Point topLeft(Screen::Right() - size.X(), Screen::Top());
		
		Point fillCenter(topLeft + 0.5 * size);
		fillCenter = fillCenter + Point(-0.5 * size. X(), 0.);
		
		FillShader::Fill(fillCenter, Point(2 * size.X(), size.Y()), back);
		
		const Sprite *left = SpriteSet::Get("ui/left edge");
		const Sprite *bottom = SpriteSet::Get("ui/bottom edge");
		const Sprite *bottom2 = SpriteSet::Get("ui/bottom edge");

		Point leftPos = topLeft + Point(
										-.5 * left->Width(),
										size.Y() - .5 * left->Height());
		SpriteShader::Draw(left, leftPos + Point(-size.X(), 0));
		
		// The top left corner of the bottom sprite should be 10 x units right
		// of the bottom left corner of the left edge sprite.
		Point bottomPos = leftPos + Point(
										  10. + .5 * (bottom->Width() - left->Width()),
										  .5 * (left->Height() + bottom->Height()));
		SpriteShader::Draw(bottom, bottomPos);
		
		
		SpriteShader::Draw(bottom2, bottomPos + Point(-size.X(), 0));
		infoDisplay.DrawAttributes(topLeft + Point(0., 10.), true);
	}
}



// Only override the ones you need; the default action is to return false.
bool MapOutfitterPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(command.Has(Command::MAP) || key == 'd' || key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
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
	else if(key == 'c')
	{
		if(!compare)
			compare = selected;
		else if (compare!=selected)
			compare = selected;
		else
			compare = nullptr;
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
		
		if(SDL_GetModState() & KMOD_SHIFT)
		{
			const Outfit *click = nullptr;
			
			for(const ClickZone<const Outfit *> & zone : zones)
				if(zone.Contains(point))
				{
					click = zone.Value();
				}
			
			if(compare && click)
				compare = click;
			else if(compare == click && click)
				compare = nullptr;
			else if(click)
				selected = click;
				
			return true;
		}
		else
		{
			selected = nullptr;
			
			for(const ClickZone<const Outfit *> &zone : zones)
				if(zone.Contains(point))
					selected = zone.Value();
			
			return true;
		}
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
	if(!system)
		return 0.;
	
	double value = -.5;
	for(const StellarObject &object : system->Objects())
		if(object.GetPlanet() && !object.GetPlanet()->Outfitter().empty())
			value = 0.;

	if(!selected)
		return value;
	
	for(const StellarObject &object : system->Objects())
		if(object.GetPlanet() && object.GetPlanet()->Outfitter().Has(selected))
			return 1.;
	
	return value;
}



void MapOutfitterPanel::Init()
{
	catalog.clear();
	set<const Outfit *> seen;
	for(const auto &it : GameData::Planets())
		if(player.HasVisited(it.second.GetSystem()))
			for(const Outfit *outfit : it.second.Outfitter())
				if(seen.find(outfit) == seen.end())
				{
					catalog[outfit->Category()].push_back(outfit);
					seen.insert(outfit);
				}
	
	for(auto &it : catalog)
		sort(it.second.begin(), it.second.end(),
			[](const Outfit *a, const Outfit *b) {return a->Name() < b->Name();});
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
	Color dimTextColor = *GameData::Colors().Get("dim");
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
		
		for(const Outfit *outfit : it->second)
		{
			if(corner.Y() < Screen::Bottom() && corner.Y() + ICON_HEIGHT >= Screen::Top())
			{
				const string &name = outfit->Name();
				if(outfit == selected)
					FillShader::Fill(corner + .5 * blockSize, blockSize, selectionColor);
				
				const Sprite *sprite = outfit->Thumbnail();
				if(sprite)
				{
					double scale = min(.5, ICON_HEIGHT / sprite->Height());
					SpriteShader::Draw(sprite, corner + iconOffset, scale);
				}
				
				bool isForSale = true;
				if(selectedSystem)
				{
					isForSale = false;
					for(const StellarObject &object : selectedSystem->Objects())
						if(object.GetPlanet() && object.GetPlanet()->Outfitter().Has(outfit))
							isForSale = true;
				}
				
				const Color &color = (isForSale ? textColor : dimTextColor);
				font.Draw(name, corner + nameOffset, color);
				
				string price = Format::Number(outfit->Cost()) + " credits";
				font.Draw(price, corner + priceOffset, color);
				
				double space = -outfit->Get("outfit space");
				string size = Format::Number(space);
				size += (abs(space) == 1. ? " ton" : " tons");
				if(space && -outfit->Get("weapon capacity") == space)
					size += " of weapon space";
				else if(space && -outfit->Get("engine capacity") == space)
					size += " of engine space";
				else
					size += " of outfit space";
				font.Draw(size, corner + sizeOffset, color);
			}
			zones.emplace_back(corner + .5 * blockSize, blockSize, outfit);
			corner += Point(0., ICON_HEIGHT);
		}
	}
	maxScroll = corner.Y() - scroll - .5 * Screen::Height();
}
