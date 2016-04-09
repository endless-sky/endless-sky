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

#include "Format.h"
#include "GameData.h"
#include "Outfit.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Sprite.h"
#include "StellarObject.h"
#include "System.h"

#include <algorithm>
#include <cmath>
#include <set>

using namespace std;



MapOutfitterPanel::MapOutfitterPanel(PlayerInfo &player)
	: MapSalesPanel(player, true)
{
	Init();
}



MapOutfitterPanel::MapOutfitterPanel(const MapPanel &panel)
	: MapSalesPanel(panel, true)
{
	Init();
}



const Sprite *MapOutfitterPanel::SelectedSprite() const
{
	return selected ? selected->Thumbnail() : nullptr;
}



const Sprite *MapOutfitterPanel::CompareSprite() const
{
	return compare ? compare->Thumbnail() : nullptr;
}



const ItemInfoDisplay &MapOutfitterPanel::SelectedInfo() const
{
	return selectedInfo;
}



const ItemInfoDisplay &MapOutfitterPanel::CompareInfo() const
{
	return compareInfo;
}



void MapOutfitterPanel::Select(int index)
{
	if(index < 0 || index >= static_cast<int>(list.size()))
		selected = nullptr;
	else
	{
		selected = list[index];
		selectedInfo.Update(*selected);
	}
}



void MapOutfitterPanel::Compare(int index)
{
	if(index < 0 || index >= static_cast<int>(list.size()))
		compare = nullptr;
	else
	{
		compare = list[index];
		compareInfo.Update(*compare);
	}
}



bool MapOutfitterPanel::HasAny(const Planet *planet) const
{
	return !planet->Outfitter().empty();
}



bool MapOutfitterPanel::HasThis(const Planet *planet) const
{
	return planet->Outfitter().Has(selected);
}



int MapOutfitterPanel::FindItem(const std::string &text) const
{
	int bestIndex = 9999;
	int bestItem = -1;
	for(unsigned i = 0; i < list.size(); ++i)
	{
		int index = Search(list[i]->Name(), text);
		if(index >= 0 && index < bestIndex)
		{
			bestIndex = index;
			bestItem = i;
			if(!index)
				return i;
		}
	}
	return bestItem;
}



void MapOutfitterPanel::DrawItems() const
{
	list.clear();
	Point corner = Screen::TopLeft() + Point(0, scroll);
	for(const string &category : categories)
	{
		auto it = catalog.find(category);
		if(it == catalog.end())
			continue;
		
		// Draw the header. If this category is collapsed, skip drawing the items.
		if(DrawHeader(corner, category))
			continue;
		
		for(const Outfit *outfit : it->second)
		{
			string price = Format::Number(outfit->Cost()) + " credits";
			
			double space = -outfit->Get("outfit space");
			string info = Format::Number(space);
			info += (abs(space) == 1. ? " ton" : " tons");
			if(space && -outfit->Get("weapon capacity") == space)
				info += " of weapon space";
			else if(space && -outfit->Get("engine capacity") == space)
				info += " of engine space";
			else
				info += " of outfit space";
			
			bool isForSale = true;
			if(selectedSystem)
			{
				isForSale = false;
				for(const StellarObject &object : selectedSystem->Objects())
					if(object.GetPlanet() && object.GetPlanet()->Outfitter().Has(outfit))
					{
						isForSale = true;
						break;
					}
			}
			
			Draw(corner, outfit->Thumbnail(), isForSale, outfit == selected,
				outfit->Name(), price, info);
			list.push_back(outfit);
		}
	}
	maxScroll = corner.Y() - scroll - .5 * Screen::Height();
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
