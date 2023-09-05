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
#include "text/DisplayText.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Government.h"
#include "LineShader.h"
#include "OutlineShader.h"
#include "Point.h"
#include "PointerShader.h"
#include "Rectangle.h"
#include "Ship.h"
#include "Sprite.h"
#include "System.h"

#include <algorithm>
#include <set>

using namespace std;

namespace {
	//  Horizontal layout of each escort icon:
	// (PAD) ICON (BAR_PAD) BARS (BAR_PAD) (PAD)
	const double PAD = 5.;
	const double ICON_SIZE = 20.;
	const double BAR_PAD = 1.0;
	const double WIDTH = 80.;
	const double BAR_WIDTH = WIDTH - ICON_SIZE - 2. * PAD - 2. * BAR_PAD;
}



void EscortDisplay::Clear()
{
	icons.clear();
}



void EscortDisplay::Add(const Ship &ship, bool isHere, bool fleetIsJumping, bool isSelected)
{
	icons.emplace_back(ship, isHere, fleetIsJumping, isSelected);
}



// Draw as many escort icons as will fit in the given bounding box.
void EscortDisplay::Draw(const Rectangle &bounds) const
{
	// Figure out how much space there is for the icons.
	int maxColumns = max(1., bounds.Width() / WIDTH);
	MergeStacks(maxColumns, bounds.Height());
	icons.sort();
	stacks.clear();
	zones.clear();
	static const Set<Color> &colors = GameData::Colors();

	// Draw escort status.
	const Font &font = FontSet::Get(14);
	// Top left corner of the current escort icon.
	Point corner = Point(bounds.Left(), bounds.Bottom());
	const Color &disabledColor = *colors.Get("escort disabled");
	const Color &elsewhereColor = *colors.Get("escort elsewhere");
	const Color &cannotJumpColor = *colors.Get("escort blocked");
	const Color &notReadyToJumpColor = *colors.Get("escort not ready");
	const Color &selectedColor = *colors.Get("escort selected");
	const Color &hereColor = *colors.Get("escort present");
	const Color &hostileColor = *colors.Get("escort hostile");

	int maxNumberWidth = 0;
	for(const Icon &escort : icons)
	{
		if(escort.ships.size())
		{
			int width = font.Width(std::to_string(escort.ships.size()));
			if(width > maxNumberWidth)
				maxNumberWidth = width;
		}
	}

	for(const Icon &escort : icons)
	{
		if(!escort.sprite)
			continue;

		corner.Y() -= escort.Height();
		// Show only as many escorts as we have room for on screen.
		if(corner.Y() < bounds.Top())
		{
			corner.X() += WIDTH;
			if(corner.X() + WIDTH > bounds.Right())
				break;
			corner.Y() = bounds.Bottom() - escort.Height();
		}
		Point pos = corner + Point(PAD + .5 * ICON_SIZE, .5 * ICON_SIZE);

		// Draw the system name for any escort not in the current system.
		if(!escort.system.empty())
			font.Draw({escort.system, {static_cast<int>(WIDTH - 20.), Alignment::LEFT, Truncate::BACK}},
				pos + Point(-10., 10.), elsewhereColor);

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
			PointerShader::Draw(pos, Point(1., 0.), ICON_SIZE / 2, ICON_SIZE / 2, -ICON_SIZE / 2, selectedColor);

		// Figure out what scale should be applied to the ship sprite.
		float scale = min(ICON_SIZE / escort.sprite->Width(), ICON_SIZE / escort.sprite->Height());
		Point size(escort.sprite->Width() * scale, escort.sprite->Height() * scale);
		OutlineShader::Draw(escort.sprite, pos, size, color);
		zones.push_back(Rectangle::FromCorner(corner, Point(WIDTH, escort.Height())));
		stacks.push_back(escort.ships);
		// Draw the number of ships in this stack.
		double width = BAR_WIDTH;
		if(escort.ships.size() > 1)
		{
			string number = to_string(escort.ships.size());

			Point numberPos = pos;
			numberPos.X() += .5 * ICON_SIZE + BAR_PAD + BAR_WIDTH - font.Width(number);
			numberPos.Y() -= .5 * font.Height();
			font.Draw(number, numberPos, elsewhereColor);
			width -= maxNumberWidth;
		}

		// Draw the status bars.
		static const Color fullColor[5] = {
			colors.Get("shields")->Additive(1.), colors.Get("hull")->Additive(1.),
			colors.Get("energy")->Additive(1.), colors.Get("heat")->Additive(1.), colors.Get("fuel")->Additive(1.)
		};
		static const Color halfColor[5] = {
			fullColor[0].Additive(.5), fullColor[1].Additive(.5),
			fullColor[2].Additive(.5), fullColor[3].Additive(.5), fullColor[4].Additive(.5),
		};
		Point from(pos.X() + .5 * ICON_SIZE + BAR_PAD, pos.Y() - 8.5);
		for(int i = 0; i < 5; ++i)
		{
			// If the low and high levels are different, draw a fully opaque bar up
			// to the low limit, and half-transparent up to the high limit.
			if(escort.high[i] > 0.)
			{
				bool isSplit = (escort.low[i] != escort.high[i]);
				const Color &color = (isSplit ? halfColor : fullColor)[i];

				Point to = from + Point(width * min(1., escort.high[i]), 0.);
				LineShader::Draw(from, to, 1.5f, color);

				if(isSplit)
				{
					Point to = from + Point(width * max(0., escort.low[i]), 0.);
					LineShader::Draw(from, to, 1.5f, color);
				}
			}
			from.Y() += 4.;
			if(i == 1)
			{
				from.X() += 5.;
				width -= 5.;
			}
		}
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



EscortDisplay::Icon::Icon(const Ship &ship, bool isHere, bool fleetIsJumping, bool isSelected)
	: sprite(ship.GetSprite()),
	isDisabled(ship.IsDisabled()),
	isHere(isHere),
	isHostile(ship.GetGovernment() && ship.GetGovernment()->IsEnemy()),
	notReadyToJump(fleetIsJumping && !ship.IsHyperspacing() && !ship.IsReadyToJump(true)),
	cannotJump(fleetIsJumping && !ship.IsHyperspacing() && !ship.JumpsRemaining()),
	isSelected(isSelected),
	cost(ship.Cost()),
	system((!isHere && ship.GetSystem()) ? ship.GetSystem()->Name() : ""),
	low{ship.Shields(), ship.Hull(), ship.Energy(), ship.Heat(), ship.Fuel()},
	high(low),
	ships(1, &ship)
{
}



// Sorting operator. It comes sooner if it costs more.
bool EscortDisplay::Icon::operator<(const Icon &other) const
{
	return (cost > other.cost);
}



int EscortDisplay::Icon::Height() const
{
	return 25 + 15 * !system.empty();
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
	auto RecomputeSize = [&]()
	{
		// Note that when merging items, we can't incrementally decrease the
		// size because we don't know how large the previous columns were. It may
		// be possible to only do the full recompute only when we cross the
		// column boundary.
		height = 0;
		column = 0;
		for(Icon &icon : icons)
		{
			height += icon.Height();
			if(height > maxHeight)
			{
				++column;
				height = icon.Height();
			}
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
