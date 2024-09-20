/* EscortDisplay.cpp
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

#include "EscortDisplay.h"

#include "Color.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "Point.h"
#include "Rectangle.h"
#include "Ship.h"
#include "image/Sprite.h"
#include "System.h"

#include <algorithm>
#include <set>

using namespace std;



void EscortDisplay::Clear()
{
	icons.clear();

	element = GameData::Interfaces().Get("escort element");
	basicHeight = element->GetValue("basic height");
	systemLabelHeight = element->GetValue("system label height");
}



void EscortDisplay::Add(const Ship &ship, bool isHere, bool systemNameKnown, bool fleetIsJumping, bool isSelected)
{
	icons.emplace_back(ship, isHere, systemNameKnown, fleetIsJumping, isSelected, basicHeight, systemLabelHeight);
}



// Draw as many escort icons as will fit in the given bounding box.
void EscortDisplay::Draw(const Rectangle &bounds) const
{
	const int width = element->GetValue("width");

	// Figure out how much space there is for the icons.
	int maxColumns = max(1., bounds.Width() / width);
	MergeStacks(maxColumns, bounds.Height());
	icons.sort();
	stacks.clear();
	zones.clear();
	static const Set<Color> &colors = GameData::Colors();

	// Top left corner of the current escort icon.
	Point corner = Point(bounds.Left(), bounds.Bottom());

	const Color &disabledColor = *colors.Get("escort disabled");
	const Color &elsewhereColor = *colors.Get("escort elsewhere");
	const Color &cannotJumpColor = *colors.Get("escort blocked");
	const Color &notReadyToJumpColor = *colors.Get("escort not ready");
	const Color &hereColor = *colors.Get("escort present");
	const Color &hostileColor = *colors.Get("escort hostile");

	// using bounding rect instead of icon zone
	//	const Point iconZoneOffset = element->GetBox("icon click zone").Center();
	const Point iconZoneSize = { element->GetValue("width"), element->GetValue("basic height") };
	bool isFirstIcon = true;
	for(const Icon &escort : icons)
	{
		if(!escort.sprite)
			continue;

		Information info;

		corner.Y() -= escort.Height();
		// Show only as many escorts as we have room for on screen.
		if(corner.Y() <= bounds.Top() && !isFirstIcon)
		{
			corner.X() += width;
			// if(corner.X() + width > bounds.Right())
			// 	break;
			corner.Y() = bounds.Bottom() - escort.Height();
		}

		// Draw the system name for any escort not in the current system.
		if(!escort.system.empty())
		{
			info.SetCondition("other system");
			info.SetString("system", escort.system);
		}

		Color color;
		if(escort.isDisabled)
			color = disabledColor;
		else if(escort.isHostile)
			color = hostileColor;
		else if(!escort.isHere)
			color = elsewhereColor;
		else if(escort.cannotJump)
			color = cannotJumpColor;
		else if(escort.notReadyToJump)
			color = notReadyToJumpColor;
		else
			color = hereColor;

		// Draw the selection pointer
		if(escort.isSelected)
			info.SetCondition("selected");

		// Figure out what scale should be applied to the ship sprite.
		info.SetSprite("icon", escort.sprite);
		info.SetOutlineColor(color);
		zones.emplace_back(corner, iconZoneSize);
		stacks.push_back(escort.ships);
		// Draw the number of ships in this stack.
		if(escort.ships.size() > 1)
		{
			info.SetCondition("multiple");
			info.SetString("count", to_string(escort.ships.size()));
		}

		// Draw the status bars.
		for(int i = 0; i < 5; ++i)
		{
			static const string levels[5][2] = {
				{"shields high", "shields low"},
				{"hull high", "hull low"},
				{"energy high", "energy low"},
				{"heat high", "heat low"},
				{"fuel high", "fuel low"}
			};
			info.SetBar(levels[i][0], escort.high[i]);
			info.SetBar(levels[i][1], escort.low[i]);
		}

		const Point dimensions(width, escort.Height());
		const Point center(corner + dimensions / 2.);

		info.SetRegion(Rectangle(center, dimensions));

		element->Draw(info);

		isFirstIcon = false;
	}
}



// Check if the given point is a click on an escort icon. If so, return the
// stack of ships represented by the icon. Otherwise, return an empty stack.
const vector<const Ship *> &EscortDisplay::Click(const Point &point) const
{
	for(unsigned i = 0; i < zones.size(); ++i)
		if(zones[i].Contains(point))
			return stacks[i];

	static const vector<const Ship *> empty;
	return empty;
}



EscortDisplay::Icon::Icon(const Ship &ship, bool isHere, bool systemNameKnown, bool fleetIsJumping, bool isSelected,
		int basicHeight, int systemLabelHeight)
	: sprite(ship.GetSprite()),
	isDisabled(ship.IsDisabled()),
	isHere(isHere),
	isHostile(ship.GetGovernment() && ship.GetGovernment()->IsEnemy()),
	notReadyToJump(fleetIsJumping && !ship.IsHyperspacing() && !ship.IsReadyToJump(true)),
	cannotJump(fleetIsJumping && !ship.IsHyperspacing() && !ship.JumpsRemaining()),
	isSelected(isSelected),
	cost(ship.Cost()),
	system((!isHere && ship.GetSystem()) ? (systemNameKnown ? ship.GetSystem()->Name() : "???") : ""),
	low{ship.Shields(), ship.Hull(), ship.Energy(), min(ship.Heat(), 1.), ship.Fuel()},
	high(low),
	ships(1, &ship)
{
	height = basicHeight;
	if(!system.empty())
		height += systemLabelHeight;
}



// Sorting operator. It comes sooner if it costs more.
bool EscortDisplay::Icon::operator<(const Icon &other) const
{
	return (cost > other.cost);
}



int EscortDisplay::Icon::Height() const
{
	return height;
}



void EscortDisplay::Icon::Merge(const Icon &other)
{
	isDisabled &= other.isDisabled;
	isHere &= other.isHere;
	isHostile |= other.isHostile;
	notReadyToJump |= other.notReadyToJump;
	cannotJump |= other.cannotJump;
	isSelected |= other.isSelected;
	if(system.empty() && !other.system.empty())
		system = other.system;

	for(unsigned i = 0; i < low.size(); ++i)
	{
		low[i] = min(low[i], other.low[i]);
		high[i] = max(high[i], other.high[i]);
	}
	ships.insert(ships.end(), other.ships.begin(), other.ships.end());
}



void EscortDisplay::MergeStacks(int columns, int maxHeight) const
{
	if(icons.empty())
		return;
	// Goals:
	// 	* Place all ships in a group, even if there is room to display them
	// 	  separately.
	//		* Display all ships, even if we have to group ships together that do
	//		  not share an icon.
	//		* If you have to collapse unrelated ships into a single icon, prefer
	//		  ships not in the system.

	int height = 0;
	int column = 0;
	int iconsInLastColumn = 0;
	auto RecomputeSize = [&]()
	{
		// Note that when merging items, we can't incrementally decrease the
		// size because we don't know how large the previous columns were. It may
		// be possible to only do the full recompute only when we cross the
		// column boundary.
		height = 0;
		column = 0;
		iconsInLastColumn = 0;
		for(Icon &icon : icons)
		{
			height += icon.Height();
			if(height >= maxHeight && iconsInLastColumn > 0)
			{
				iconsInLastColumn = 0;
				++column;
				height = icon.Height();
			}
			++iconsInLastColumn;
		}
	};

	auto TooBig = [&]()
	{
		return column > columns || (column == columns && height > 0);
	};

	// Collapse all ships into groups by icon, location, and hostility
	for(auto it = icons.begin(); it != icons.end(); ++it)
	{
		auto it2 = it;
		for(++it2; it2 != icons.end();)
		{
			if(it->sprite == it2->sprite &&
			   it->isHostile == it2->isHostile &&
			   it->isHere == it2->isHere)
			{
				it->Merge(*it2);
				it2 = icons.erase(it2);
			}
			else
				++it2;
		}
	}

	RecomputeSize();

	if(TooBig())
	{
		// There isn't enough room. Merge out-of-system ships even if they
		// don't have matching icons.
		for(auto it = icons.begin(); it != icons.end() && TooBig(); ++it)
		{
			if(!it->isHere)
			{
				auto it2 = it;
				for(++it2; it2 != icons.end() && TooBig();)
				{
					if(!it2->isHere &&
						it->sprite == it2->sprite &&
						it->isHostile == it2->isHostile)
					{
						it->Merge(*it2);
						it2 = icons.erase(it2);

						RecomputeSize();
					}
					else
						++it2;
				}
			}
		}
	}

	if(TooBig())
	{
		// Still too big. Start collapsing all icons from lowest to highest
		// priority, regardless of status or location
		icons.sort();
		auto it = icons.end();
		--it;
		while(TooBig() && it != icons.begin())
		{
			--it;
			it->Merge(icons.back());
			icons.pop_back();
			
			RecomputeSize();
		}
	}
}
