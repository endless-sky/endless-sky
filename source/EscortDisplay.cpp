/* EscortDisplay.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "EscortDisplay.h"

#include "Color.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Government.h"
#include "LineShader.h"
#include "Point.h"
#include "Preferences.h"
#include "OutlineShader.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "System.h"

#include <algorithm>
#include <set>

using namespace std;




void EscortDisplay::Clear()
{
	icons.clear();
}



void EscortDisplay::Add(const Ship &ship, bool isHere, bool fleetIsJumping, bool isSelected)
{
	icons.emplace_back(ship, isHere, fleetIsJumping, isSelected);
}



// The display starts in the lower left corner of the screen and takes up
// all but the top 450 pixels of the screen. It can show additional columns
// up to the center of the screen.
void EscortDisplay::Draw(int visibleMessages) const
{
	MergeStacks();
	stacks.clear();
	zones.clear();
	static const Set<Color> &colors = GameData::Colors();
	
	// Draw escort status.
	const Font &font = FontSet::Get(14);
	Point pos = Point(Screen::Left() + 20., Screen::Bottom());
	const Color &elsewhereColor = *colors.Get("escort elsewhere");
	const Color &cannotJumpColor = *colors.Get("escort blocked");
	const Color &notReadyToJumpColor = *colors.Get("escort not ready");
	const Color &selectedColor = *colors.Get("escort selected");
	const Color &hereColor = *colors.Get("escort present");
	const Color &hostileColor = *colors.Get("escort hostile");
	int hiddenEscorts = 0;
	for(const Icon &escort : icons)
	{
		if(!escort.sprite)
			continue;
		
		if(hiddenEscorts)
		{
			hiddenEscorts += escort.ships.size();
			continue;
		}
		
		pos.Y() -= escort.Height();
		if(pos.Y() <= Screen::Top() + 450.)
		{
			// Show only as many escorts as we have room for on screen.
			if(!Preferences::Has("Show more escorts") || pos.X() > -200)
			{
				hiddenEscorts = escort.ships.size();
				continue;
			}
			pos = Point(pos.X() + 110., Screen::Bottom() - escort.Height() - 20. * visibleMessages);
		}
		
		// Draw the system name for any escort not in the current system.
		if(escort.withSystem)
		{
			font.Draw(escort.system, pos + Point(-10., 12.), elsewhereColor);
		}
		
		Color color;
		if(escort.isHostile)
			color = hostileColor;
		else if(!escort.isHere)
			color = elsewhereColor;
		else if(escort.cannotJump)
			color = cannotJumpColor;
		else if(escort.notReadyToJump)
			color = notReadyToJumpColor;
		else if(escort.isSelected)
			color = selectedColor;
		else
			color = hereColor;
		
		// Figure out what scale should be applied to the ship sprite.
		double scale = min(20. / escort.sprite->Width(), 20. / escort.sprite->Height());
		Point size(escort.sprite->Width() * scale, escort.sprite->Height() * scale);
		OutlineShader::Draw(escort.sprite, pos, size, color);
		zones.push_back(pos);
		stacks.push_back(escort.ships);
		// Draw the number of ships in this stack.
		double width = 70.;
		if(escort.ships.size() > 1)
		{
			string number = to_string(escort.ships.size());
		
			Point numberPos = pos;
			numberPos.X() += 15. + width - font.Width(number);
			numberPos.Y() -= .5 * font.Height();
			font.Draw(number, numberPos, elsewhereColor);
			width -= 20.;
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
		Point from(pos.X() + 15., pos.Y() - 8.5);
		for(int i = 0; i < 5; ++i)
		{
			// If the low and high levels are different, draw a fully opaque bar up
			// to the low limit, and half-transparent up to the high limit.
			if(escort.high[i] > 0.)
			{
				bool isSplit = (escort.low[i] != escort.high[i]);
				const Color &color = (isSplit ? halfColor : fullColor)[i];
				
				Point to = from + Point(width * min(1., escort.high[i]), 0.);
				LineShader::Draw(from, to, 1.5, color);
				
				if(isSplit)
				{
					Point to = from + Point(width * max(0., escort.low[i]), 0.);
					LineShader::Draw(from, to, 1.5, color);
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
	
	// Report the number of escorts not shown.
	if(hiddenEscorts)
	{
		Color dim = *GameData::Colors().Get("medium");
		string hidden = to_string(hiddenEscorts) + " more " + (hiddenEscorts == 1 ? "escort" : "escorts");
		font.Draw(hidden, Point(Screen::Left() + 10., Screen::Top() + 425.), dim);
	}
}



// Check if the given point is a click on an escort icon. If so, return the
// stack of ships represented by the icon. Otherwise, return an empty stack.
const vector<const Ship *> &EscortDisplay::Click(const Point &point) const
{
	for(unsigned i = 0; i < zones.size(); ++i)
		if(point.Distance(zones[i]) < 15.)
			return stacks[i];
	
	static const vector<const Ship *> empty;
	return empty;
}



EscortDisplay::Icon::Icon(const Ship &ship, bool isHere, bool fleetIsJumping, bool isSelected)
	: sprite(ship.GetSprite()),
	isHere(isHere && !ship.IsDisabled()),
	isHostile(ship.GetGovernment() && ship.GetGovernment()->IsEnemy()),
	notReadyToJump(fleetIsJumping && !ship.IsHyperspacing() && !ship.IsReadyToJump()),
	cannotJump(fleetIsJumping && !ship.IsHyperspacing() && !ship.JumpsRemaining()),
	isSelected(isSelected),
	withSystem(false),
	cost(ship.Cost()),
	system((!isHere && ship.GetSystem()) ? ship.GetSystem()->Name() : ""),
	low{ship.Shields(), ship.Hull(), ship.Energy(), ship.Heat(), ship.Fuel()},
	high(low),
	ships(1, &ship)
{
}



// Sorting operator.
// It comes sooner if it's here, then by system name, then if it costs more.
bool EscortDisplay::Icon::operator<(const Icon &other) const
{
	if(isHere != other.isHere)
		return isHere;
	if(system != other.system)
		return system < other.system;
	return (cost > other.cost);
}



int EscortDisplay::Icon::Height() const
{
	return 30 + 15 * withSystem;
}



void EscortDisplay::Icon::Merge(const Icon &other)
{
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



void EscortDisplay::MergeStacks() const
{
	if(icons.empty())
		return;
	
	// Sort by system name, then biggest cost.
	icons.sort();
	
	// Mark icons that show system name.
	string currentSystem;
	for(Icon &icon : icons)
	{
		icon.withSystem = (!icon.isHere && icon.system != currentSystem);
		if(icon.withSystem)
			currentSystem = icon.system;
	}
	
	int maxHeight = Screen::Height() - 450;
	set<Icon *> processed;
	while(true)
	{
		int height = 0;
		for(Icon &icon : icons)
			height += icon.Height();
		
		// Icons fit a single column, stop merging.
		if(height < maxHeight)
			break;
		
		// Start merging backwards, from the end of the list.
		bool merged = false;
		auto stackable = icons.end();
		while(!merged)
		{
			--stackable;
			if(stackable == icons.begin())
				break;
			
			if(processed.count(&(*stackable)))
				continue;
			
			// Merge with other escorts.
			auto stack = stackable;
			while(stack != icons.begin())
			{
				--stack;
				
				// Same system? Icons are ordered by system first so we stop merging
				// when we encounter a different system.
				if(stack->isHere != stackable->isHere || stack->system != stackable->system)
					break;
				
				// Same sprite and attitude?
				if(stack->sprite != stackable->sprite || stack->isHostile != stackable->isHostile)
					continue;
				
				stack->Merge(*stackable);
				icons.erase(stackable);
				stackable = stack;
				merged = true;
			}
			
			processed.insert(&(*stackable));
		}
		
		// Nothing else can be merged.
		if(!merged)
			break;
	}
}
