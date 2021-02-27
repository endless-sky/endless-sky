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

#include "CoreStartData.h"
#include "text/Format.h"
#include "GameData.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

#include <algorithm>
#include <limits>
#include <set>

using namespace std;



MapShipyardPanel::MapShipyardPanel(PlayerInfo &player)
	: MapSalesPanel(player, false)
{
	Init();
}



MapShipyardPanel::MapShipyardPanel(const MapPanel &panel, bool onlyHere)
	: MapSalesPanel(panel, false)
{
	Init();
	onlyShowSoldHere = onlyHere;
	UpdateCache();
}



const Sprite *MapShipyardPanel::SelectedSprite() const
{
	return selected ? selected->Thumbnail() ? selected->Thumbnail() : selected->GetSprite() : nullptr;
}



const Sprite *MapShipyardPanel::CompareSprite() const
{
	return compare ? compare->Thumbnail() ? compare->Thumbnail() : compare->GetSprite() : nullptr;
}



const ItemInfoDisplay &MapShipyardPanel::SelectedInfo() const
{
	return selectedInfo;
}



const ItemInfoDisplay &MapShipyardPanel::CompareInfo() const
{
	return compareInfo;
}



const string &MapShipyardPanel::KeyLabel(int index) const
{
	static const string LABEL[3] = {
		"Has no shipyard",
		"Has shipyard",
		"Sells this ship"
	};
	return LABEL[index];
}



void MapShipyardPanel::Select(int index)
{
	if(index < 0 || index >= static_cast<int>(list.size()))
		selected = nullptr;
	else
	{
		selected = list[index];
		selectedInfo.Update(*selected, player.StockDepreciation(), player.GetDate().DaysSinceEpoch());
	}
	UpdateCache();
}



void MapShipyardPanel::Compare(int index)
{
	if(index < 0 || index >= static_cast<int>(list.size()))
		compare = nullptr;
	else
	{
		compare = list[index];
		compareInfo.Update(*compare, player.StockDepreciation(), player.GetDate().DaysSinceEpoch());
	}
}



double MapShipyardPanel::SystemValue(const System *system) const
{
	if(!system || !player.HasVisited(*system) || !system->IsInhabited(player.Flagship()))
		return numeric_limits<double>::quiet_NaN();
	
	// Visiting a system is sufficient to know what ports are available on its planets.
	double value = -.5;
	for(const StellarObject &object : system->Objects())
		if(object.GetPlanet())
		{
			const auto &shipyard = object.GetPlanet()->Shipyard();
			if(shipyard.Has(selected))
				return 1.;
			if(!shipyard.empty())
				value = 0.;
		}
	return value;
}



int MapShipyardPanel::FindItem(const string &text) const
{
	int bestIndex = 9999;
	int bestItem = -1;
	for(unsigned i = 0; i < list.size(); ++i)
	{
		int index = Search(list[i]->ModelName(), text);
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



void MapShipyardPanel::DrawItems()
{
	if(GetUI()->IsTop(this) && player.GetPlanet() && player.GetDate() >= player.StartData().GetDate() + 12)
		DoHelp("map advanced shops");
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
		
		for(const Ship *ship : it->second)
		{
			string price = Format::Credits(ship->Cost()) + " credits";
			
			string info = Format::Number(ship->Attributes().Get("shields")) + " shields / ";
			info += Format::Number(ship->Attributes().Get("hull")) + " hull";
			
			bool isForSale = true;
			if(player.HasVisited(*selectedSystem))
			{
				isForSale = false;
				for(const StellarObject &object : selectedSystem->Objects())
					if(object.GetPlanet() && object.GetPlanet()->Shipyard().Has(ship))
					{
						isForSale = true;
						break;
					}
			}
			if(!isForSale && onlyShowSoldHere)
				continue;
			
			const Sprite *sprite = ship->Thumbnail();
			if(!sprite)
				sprite = ship->GetSprite();
			Draw(corner, sprite, isForSale, ship == selected, ship->ModelName(), price, info);
			list.push_back(ship);
		}
	}
	maxScroll = corner.Y() - scroll - .5 * Screen::Height();
}



void MapShipyardPanel::Init()
{
	catalog.clear();
	set<const Ship *> seen;
	for(const auto &it : GameData::Planets())
		if(it.second.IsValid() && player.HasVisited(*it.second.GetSystem()))
			for(const Ship *ship : it.second.Shipyard())
				if(!seen.count(ship))
				{
					catalog[ship->Attributes().Category()].push_back(ship);
					seen.insert(ship);
				}
	
	for(auto &it : catalog)
		sort(it.second.begin(), it.second.end(),
			[](const Ship *a, const Ship *b) { return a->ModelName() < b->ModelName(); });
}
