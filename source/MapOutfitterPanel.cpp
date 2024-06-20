/* MapOutfitterPanel.cpp
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

#include "MapOutfitterPanel.h"

#include "comparators/ByName.h"
#include "CoreStartData.h"
#include "text/Format.h"
#include "GameData.h"
#include "Outfit.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Sprite.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

using namespace std;



MapOutfitterPanel::MapOutfitterPanel(PlayerInfo &player)
	: MapSalesPanel(player, true)
{
	Init();
}



MapOutfitterPanel::MapOutfitterPanel(const MapPanel &panel, bool onlyHere)
	: MapSalesPanel(panel, true)
{
	Init();
	onlyShowSoldHere = onlyHere;
	UpdateCache();
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



const string &MapOutfitterPanel::KeyLabel(int index) const
{
	static const string OUTFIT[4] = {
		"Has no outfitter",
		"Has outfitter",
		"Sells this outfit",
		"Outfit in storage"
	};
	static const string MINE[4] = {
		"Has no outfitter",
		"Has outfitter",
		"Mine this here",
		"Mineral in storage"
	};

	if(selected && selected->Get("minable") > 0.)
		return MINE[index];
	return OUTFIT[index];
}



void MapOutfitterPanel::Select(int index)
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



void MapOutfitterPanel::Compare(int index)
{
	if(index < 0 || index >= static_cast<int>(list.size()))
		compare = nullptr;
	else
	{
		compare = list[index];
		compareInfo.Update(*compare, player);
	}
}



double MapOutfitterPanel::SystemValue(const System *system) const
{
	if(!system || !player.CanView(*system))
		return numeric_limits<double>::quiet_NaN();

	auto it = player.Harvested().lower_bound(pair<const System *, const Outfit *>(system, nullptr));
	for( ; it != player.Harvested().end() && it->first == system; ++it)
		if(it->second == selected)
			return 1.;

	if(!system->IsInhabited(player.Flagship()))
		return numeric_limits<double>::quiet_NaN();

	// Visiting a system is sufficient to know what ports are available on its planets.
	double value = -1.;
	const auto &planetStorage = player.PlanetaryStorage();
	for(const StellarObject &object : system->Objects())
		if(object.HasSprite() && object.HasValidPlanet())
		{
			const auto storage = planetStorage.find(object.GetPlanet());
			if(storage != planetStorage.end() && storage->second.Get(selected))
				return .5;
			const auto &outfitter = object.GetPlanet()->Outfitter();
			if(outfitter.Has(selected))
				return 1.;
			if(!outfitter.empty())
				value = 0.;
		}
	return value;
}



int MapOutfitterPanel::FindItem(const string &text) const
{
	int bestIndex = 9999;
	int bestItem = -1;
	for(unsigned i = 0; i < list.size(); ++i)
	{
		int index = Format::Search(list[i]->DisplayName(), text);
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



void MapOutfitterPanel::DrawItems()
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

		for(const Outfit *outfit : it->second)
		{
			string price = Format::CreditString(outfit->Cost());

			string info;
			if(outfit->Get("minable") > 0.)
				info = "(Mined from asteroids)";
			else if(outfit->Get("installable") < 0.)
			{
				double space = outfit->Mass();
				info = Format::CargoString(space, "space");
			}
			else
			{
				double space = -outfit->Get("outfit space");
				info = Format::MassString(space);
				if(space && -outfit->Get("weapon capacity") == space)
					info += " of weapon space";
				else if(space && -outfit->Get("engine capacity") == space)
					info += " of engine space";
				else
					info += " of outfit space";
			}

			bool isForSale = true;
			unsigned storedInSystem = 0;
			if(player.CanView(*selectedSystem))
			{
				isForSale = false;
				const auto &storage = player.PlanetaryStorage();

				for(const StellarObject &object : selectedSystem->Objects())
				{
					if(!object.HasSprite() || !object.HasValidPlanet())
						continue;

					const Planet &planet = *object.GetPlanet();
					if(planet.HasOutfitter())
					{
						const auto pit = storage.find(&planet);
						if(pit != storage.end())
							storedInSystem += pit->second.Get(outfit);
					}
					if(planet.Outfitter().Has(outfit))
					{
						isForSale = true;
						break;
					}
				}
			}
			if(!isForSale && onlyShowSoldHere)
				continue;
			if(!storedInSystem && onlyShowStorageHere)
				continue;

			const string storage_details =
				onlyShowSoldHere || storedInSystem == 0
				? ""
				: storedInSystem == 1
				? "1 unit in storage"
				: Format::Number(storedInSystem) + " units in storage";
			Draw(corner, outfit->Thumbnail(), 0, isForSale, outfit == selected,
				outfit->DisplayName(), price, info, storage_details);
			list.push_back(outfit);
		}
	}
	maxScroll = corner.Y() - scroll - .5 * Screen::Height();
}



void MapOutfitterPanel::Init()
{
	catalog.clear();
	set<const Outfit *> seen;

	// Add all outfits sold by outfitters of planets from viewable systems.
	for(auto &&it : GameData::Planets())
		if(it.second.IsValid() && player.CanView(*it.second.GetSystem()))
			for(const Outfit *outfit : it.second.Outfitter())
				if(!seen.count(outfit))
				{
					catalog[outfit->Category()].push_back(outfit);
					seen.insert(outfit);
				}

	// Add outfits in storage
	for(const auto &it : player.PlanetaryStorage())
		if(it.first->HasOutfitter())
			for(const auto &oit : it.second.Outfits())
				if(!seen.count(oit.first))
				{
					catalog[oit.first->Category()].push_back(oit.first);
					seen.insert(oit.first);
				}

	// Add all known minables.
	for(const auto &it : player.Harvested())
		if(!seen.count(it.second))
		{
			catalog[it.second->Category()].push_back(it.second);
			seen.insert(it.second);
		}

	// Sort the vectors.
	for(auto &it : catalog)
		sort(it.second.begin(), it.second.end(), ByDisplayName<Outfit>());
}
