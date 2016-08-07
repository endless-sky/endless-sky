/* MapSalesPanel.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MapSalesPanel.h"

#include "Command.h"
#include "Dialog.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "ItemInfoDisplay.h"
#include "MapDetailPanel.h"
#include "MapOutfitterPanel.h"
#include "MapShipyardPanel.h"
#include "MissionPanel.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "RingShader.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

#include <algorithm>

using namespace std;

const double MapSalesPanel::ICON_HEIGHT = 90.;
const double MapSalesPanel::PAD = 8.;
const int MapSalesPanel::WIDTH = 270;

using namespace std;



MapSalesPanel::MapSalesPanel(PlayerInfo &player, bool isOutfitters)
	: MapPanel(player, SHOW_SPECIAL),
	categories(isOutfitters ? Outfit::CATEGORIES : Ship::CATEGORIES),
	isOutfitters(isOutfitters)
{
	if(!isOutfitters)
		swizzle = GameData::PlayerGovernment()->GetSwizzle();
}



MapSalesPanel::MapSalesPanel(const MapPanel &panel, bool isOutfitters)
	: MapPanel(panel),
	categories(isOutfitters ? Outfit::CATEGORIES : Ship::CATEGORIES),
	isOutfitters(isOutfitters)
{
	SetCommodity(SHOW_SPECIAL);
	if(!isOutfitters)
		swizzle = GameData::PlayerGovernment()->GetSwizzle();
}



void MapSalesPanel::Draw()
{
	MapPanel::Draw();
	
	zones.clear();
	categoryZones.clear();
	hidPrevious = true;
	
	DrawKey();
	DrawPanel();
	DrawItems();
	DrawButtons();
	DrawInfo();
}



bool MapSalesPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(command.Has(Command::MAP) || key == 'd' || key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == 's' && isOutfitters)
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapShipyardPanel(*this));
	}
	else if(key == 'o' && !isOutfitters)
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
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
	{
		scroll += static_cast<double>((Screen::Height() - 100) * ((key == SDLK_PAGEUP) - (key == SDLK_PAGEDOWN)));
		scroll = min(0., max(-maxScroll, scroll));
	}
	else if((key == SDLK_DOWN || key == SDLK_UP) && !zones.empty())
	{
		selected += (key == SDLK_DOWN) - (key == SDLK_UP);
		if(selected < 0)
			selected = zones.size() - 1;
		else if(selected > static_cast<int>(zones.size() - 1))
			selected = 0;
		
		Compare(compare = -1);
		Select(selected);
		ScrollTo(selected);
	}
	else if(key == 'f')
		GetUI()->Push(new Dialog(
			this, &MapSalesPanel::DoFind, "Search for:"));
	else if(key == '+' || key == '=')
		ZoomMap();
	else if(key == '-')
		UnzoomMap();
	else
		return false;
	
	return true;
}



bool MapSalesPanel::Click(int x, int y)
{
	if(x < Screen::Left() + WIDTH)
	{
		Point point(x, y);
		
		bool isCompare = (SDL_GetModState() & KMOD_SHIFT);
		
		for(const ClickZone<int> &zone : zones)
			if(zone.Contains(point))
			{
				if(isCompare)
				{
					if(zone.Value() != selected)
						Compare(compare = zone.Value());
				}
				else
				{
					Select(selected = zone.Value());
					Compare(compare = -1);
				}
				break;
			}
		
		for(const ClickZone<string> &zone : categoryZones)
			if(zone.Contains(point))
			{
				bool set = !hideCategory[zone.Value()];
				if(!isCompare)
					hideCategory[zone.Value()] = set;
				else
					for(const string &category : categories)
						hideCategory[category] = set;
				
				break;
			}
		
		return true;
	}
	else
		return MapPanel::Click(x, y);
}



bool MapSalesPanel::Hover(int x, int y)
{
	isDragging = (x < Screen::Left() + WIDTH);
	if(isDragging)
		return true;
	
	return MapPanel::Hover(x, y);
}



bool MapSalesPanel::Drag(double dx, double dy)
{
	if(isDragging)
		scroll = min(0., max(-maxScroll, scroll + dy));
	else
		return MapPanel::Drag(dx, dy);
	
	return true;
}



bool MapSalesPanel::Scroll(double dx, double dy)
{
	if(isDragging)
		scroll = min(0., max(-maxScroll, scroll + 50 * dy));
	else
		return MapPanel::Scroll(dx, dy);
	
	return true;
}



double MapSalesPanel::SystemValue(const System *system) const
{
	if(!system)
		return 0.;
	
	double value = -.5;
	for(const StellarObject &object : system->Objects())
		if(object.GetPlanet())
		{
			if(HasThis(object.GetPlanet()))
				return 1.;
			if(HasAny(object.GetPlanet()))
				value = 0.;
		}
	return value;
}



void MapSalesPanel::DrawKey() const
{
	const Sprite *back = SpriteSet::Get("ui/sales key");
	SpriteShader::Draw(back, Screen::TopLeft() + Point(WIDTH + 10, 0) + .5 * Point(back->Width(), back->Height()));
	
	Color bright(.6, .6);
	Color dim(.3, .3);
	const Font &font = FontSet::Get(14);
	
	Point pos(Screen::Left() + 50. + WIDTH, Screen::Top() + 12.);
	Point textOff(10., -.5 * font.Height());
	
	static const string LABEL[][2] = {
		{"Has no shipyard", "Has no outfitter"},
		{"Has shipyard", "Has outfitter"},
		{"Sells this ship", "Sells this outfit"}
	};
	static const double VALUE[] = {
		-.5,
		0.,
		1.
	};
	
	double selectedValue = (selectedSystem ? SystemValue(selectedSystem) : -1.);
	for(int i = 0; i < 3; ++i)
	{
		bool isSelected = (VALUE[i] == selectedValue);
		RingShader::Draw(pos, OUTER, INNER, MapColor(VALUE[i]));
		font.Draw(LABEL[i][isOutfitters], pos + textOff, isSelected ? bright : dim);
		pos.Y() += 20.;
	}
}



void MapSalesPanel::DrawPanel() const
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



void MapSalesPanel::DrawButtons()
{
	Information info;
	info.SetCondition(isOutfitters ? "is outfitters" : "is shipyards");
	if(ZoomIsMax())
		info.SetCondition("max zoom");
	if(ZoomIsMin())
		info.SetCondition("min zoom");
	const Interface *interface = GameData::Interfaces().Get("map buttons");
	interface->Draw(info, this);
}



void MapSalesPanel::DrawInfo() const
{
	if(selected >= 0)
	{
		const ItemInfoDisplay &selectedInfo = SelectedInfo();
		const ItemInfoDisplay &compareInfo = CompareInfo();
		int selectedHeight = max(selectedInfo.AttributesHeight(), 120);
		int compareHeight = (compare >= 0) ? max(compareInfo.AttributesHeight(), 120) : 0;
		
		Color back(.125, 1.);
		Point size(selectedInfo.PanelWidth(), selectedHeight + compareHeight);
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
		
		SpriteShader::Draw(box, topLeft + iconOffset + Point(-15., 5.));
		DrawSprite(topLeft + Point(-ICON_HEIGHT, 5.), SelectedSprite());
		
		selectedInfo.DrawAttributes(topLeft);
		
		if(compare >= 0)
		{
			topLeft.Y() += selectedHeight;
			
			SpriteShader::Draw(box, topLeft + iconOffset + Point(-15., 5.));
			DrawSprite(topLeft + Point(-ICON_HEIGHT, 5.), CompareSprite());
			
			Color line(.5);
			size.Y() = 1.;
			FillShader::Fill(topLeft + .5 * size - Point(0., 1.), size, line);
			compareInfo.DrawAttributes(topLeft);
		}
	}
}



bool MapSalesPanel::DrawHeader(Point &corner, const string &category) const
{
	auto hit = hideCategory.find(category);
	bool hide = (hit != hideCategory.end() && hit->second);
	if(!hidPrevious)
		corner.Y() += 50.;
	hidPrevious = hide;
	
	Color textColor = *GameData::Colors().Get(hide ? "dim" : "bright");
	const Font &bigFont = FontSet::Get(18);
	bigFont.Draw(category, corner + Point(5., 15.), textColor);
	categoryZones.emplace_back(corner + Point(WIDTH * .5, 20.), Point(WIDTH, 40.), category);
	corner.Y() += 40.;
	
	return hide;
}



void MapSalesPanel::DrawSprite(const Point &corner, const Sprite *sprite) const
{
	if(sprite)
	{
		Point iconOffset(.5 * ICON_HEIGHT, .5 * ICON_HEIGHT);
		double scale = min(.5, ICON_HEIGHT / sprite->Height());
		SpriteShader::Draw(sprite, corner + iconOffset, scale, swizzle);
	}
}



void MapSalesPanel::Draw(Point &corner, const Sprite *sprite, bool isForSale, bool isSelected,
		const string &name, const string &price, const string &info) const
{
	const Font &font = FontSet::Get(14);
	Color selectionColor(0., .3);
	
	Point nameOffset(ICON_HEIGHT, .5 * ICON_HEIGHT - PAD - 1.5 * font.Height());
	Point priceOffset(ICON_HEIGHT, nameOffset.Y() + font.Height() + PAD);
	Point infoOffset(ICON_HEIGHT, priceOffset.Y() + font.Height() + PAD);
	Point blockSize(WIDTH, ICON_HEIGHT);

	if(corner.Y() < Screen::Bottom() && corner.Y() + ICON_HEIGHT >= Screen::Top())
	{
		if(isSelected)
			FillShader::Fill(corner + .5 * blockSize, blockSize, selectionColor);
		
		DrawSprite(corner, sprite);
		
		Color textColor = *GameData::Colors().Get(isForSale ? "medium" : "dim");
		font.Draw(name, corner + nameOffset, textColor);
		font.Draw(price, corner + priceOffset, textColor);
		font.Draw(info, corner + infoOffset, textColor);
	}
	zones.emplace_back(corner + .5 * blockSize, blockSize, zones.size());
	corner.Y() += ICON_HEIGHT;
}



void MapSalesPanel::DoFind(const string &text)
{
	int index = FindItem(text);
	if(index >= 0 && index < static_cast<int>(zones.size()))
	{
		Compare(compare = -1);
		Select(selected = index);
		ScrollTo(index);
	}
}



void MapSalesPanel::ScrollTo(int index)
{
	if(index < 0 || index >= static_cast<int>(zones.size()))
		return;
	
	const ClickZone<int> &it = zones[selected];
	if(it.Bottom() > Screen::Bottom())
		scroll += Screen::Bottom() - it.Bottom();
	if(it.Top() < Screen::Top())
		scroll += Screen::Top() - it.Top();
}
