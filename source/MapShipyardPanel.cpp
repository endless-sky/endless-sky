/* MapShipyardPanel.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "MapShipyardPanel.h"

#include "comparators/BySeriesAndIndex.h"
#include "CategoryList.h"
#include "CoreStartData.h"
#include "text/Format.h"
#include "GameData.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Ship.h"
#include "image/Sprite.h"
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



const Swizzle *MapShipyardPanel::SelectedSpriteSwizzle() const
{
	return selected->CustomSwizzle();
}



const Swizzle *MapShipyardPanel::CompareSpriteSwizzle() const
{
	return compare->CustomSwizzle();
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
	static const string LABEL[4] = {
		"Has no shipyard",
		"Has shipyard",
		"Sells this ship",
		"Ship parked here"
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
		selectedInfo.Update(*selected, player);
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
		compareInfo.Update(*compare, player);
	}
}



double MapShipyardPanel::SystemValue(const System *system) const
{
	if(!system || !player.CanView(*system))
		return numeric_limits<double>::quiet_NaN();

	// If there is a shipyard with parked ships, the order of precedence is
	// a selected parked ship, the shipyard, parked ships.

	const auto &systemShips = parkedShips.find(system);
	if(systemShips != parkedShips.end() && systemShips->second.find(selected) != systemShips->second.end())
		return .5;
	else if(system->IsInhabited(player.Flagship()))
	{
		// Visiting a system is sufficient to know what ports are available on its planets.
		double value = -1.;
		for(const StellarObject &object : system->Objects())
			if(object.HasSprite() && object.HasValidPlanet())
			{
				const auto &shipyard = object.GetPlanet()->ShipyardStock();
				if(shipyard.Has(selected))
					return 1.;
				if(!shipyard.empty())
					value = 0.;
			}
		return value;
	}
	else if(systemShips != parkedShips.end() && !selected)
		return .5;
	else
		return numeric_limits<double>::quiet_NaN();
}



int MapShipyardPanel::FindItem(const string &text) const
{
	int bestIndex = 9999;
	int bestItem = -1;
	for(unsigned i = 0; i < list.size(); ++i)
	{
		int index = Format::Search(list[i]->DisplayModelName(), text);
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
	for(const auto &cat : categories)
	{
		const string &category = cat.Name();
		auto it = catalog.find(category);
		if(it == catalog.end())
			continue;

		// Draw the header. If this category is collapsed, skip drawing the items.
		if(DrawHeader(corner, category))
			continue;

		for(const string &name : it->second)
		{
			const Ship *ship = GameData::Ships().Get(name);
			string price = Format::CreditString(ship->Cost());

			string info = Format::Number(ship->MaxShields()) + " shields / ";
			info += Format::Number(ship->MaxHull()) + " hull";

			bool isForSale = true;
			unsigned parkedInSystem = 0;
			if(player.CanView(*selectedSystem))
			{
				isForSale = false;
				for(const StellarObject &object : selectedSystem->Objects())
					if(object.HasSprite() && object.HasValidPlanet()
							&& object.GetPlanet()->ShipyardStock().Has(ship))
					{
						isForSale = true;
						break;
					}

				const auto parked = parkedShips.find(selectedSystem);
				if(parked != parkedShips.end())
				{
					const auto shipCount = parked->second.find(ship);
					if(shipCount != parked->second.end())
						parkedInSystem = shipCount->second;
				}
			}
			if(!isForSale && onlyShowSoldHere)
				continue;
			if(!parkedInSystem && onlyShowStorageHere)
				continue;

			const Sprite *sprite = ship->Thumbnail();
			if(!sprite)
				sprite = ship->GetSprite();

			const string parking_details =
				onlyShowSoldHere || parkedInSystem == 0
				? ""
				: parkedInSystem == 1
				? "1 ship parked"
				: Format::Number(parkedInSystem) + " ships parked";
			Draw(corner, sprite, ship->CustomSwizzle(), isForSale, ship == selected,
					ship->DisplayModelName(), ship->VariantMapShopName(), price, info, parking_details);
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
		if(it.second.IsValid() && player.CanView(*it.second.GetSystem()))
			for(const Ship *ship : it.second.ShipyardStock())
				if(!seen.contains(ship))
				{
					catalog[ship->Attributes().Category()].push_back(ship->VariantName());
					seen.insert(ship);
				}

	parkedShips.clear();
	for(const auto &it : player.Ships())
		if(it->IsParked())
		{
			const Ship *model = GameData::Ships().Get(it->TrueModelName());
			++parkedShips[it->GetSystem()][model];
			if(!seen.contains(model))
			{
				catalog[model->Attributes().Category()].push_back(model->TrueModelName());
				seen.insert(model);
			}
		}

	for(auto &it : catalog)
		sort(it.second.begin(), it.second.end(), BySeriesAndIndex<Ship>());
}
