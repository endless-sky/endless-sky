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
#include "LineShader.h"
#include "Point.h"
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



void EscortDisplay::Add(const Ship &ship, bool isHere)
{
	icons.emplace_back(ship, isHere);
}



// The display starts in the lower left corner of the screen and takes up
// all but the top 450 pixels of the screen.
void EscortDisplay::Draw() const
{
	MergeStacks();
	sort(icons.begin(), icons.end());
	
	// Draw escort status.
	static const Font &font = FontSet::Get(14);
	Point pos = Point(Screen::Left() + 20., Screen::Bottom());
	static const Color hereColor(.8, 1.);
	static const Color elsewhereColor(.4, .4, .6, 1.);
	static const Color readyToJumpColor(.2, .8, .2, 1.);
	for(const Icon &escort : icons)
	{
		if(!escort.sprite)
			continue;
		
		pos.Y() -= escort.Height();
		// Show only as many escorts as we have room for on screen.
		if(pos.Y() <= Screen::Top() + 450.)
			break;
		
		// Draw the system name for any escort not in the current system.
		if(!escort.system.empty())
			font.Draw(escort.system, pos + Point(-10., 10.), elsewhereColor);

		Color color;
		if(!escort.isHere)
			color = elsewhereColor;
		else if(escort.isReadyToJump)
			color = readyToJumpColor;
		else
			color = hereColor;
		
		// Figure out what scale should be applied to the ship sprite.
		double scale = min(20. / escort.sprite->Width(), 20. / escort.sprite->Height());
		Point size(escort.sprite->Width() * scale, escort.sprite->Height() * scale);
		OutlineShader::Draw(escort.sprite, pos, size, color);
		// Draw the number of ships in this stack.
		double width = 70.;
		if(escort.stackSize > 1)
		{
			string number = to_string(escort.stackSize);
		
			Point numberPos = pos;
			numberPos.X() += 15. + width - font.Width(number);
			numberPos.Y() -= .5 * font.Height();
			font.Draw(number, numberPos, elsewhereColor);
			width -= 20.;
		}
		
		// Draw the status bars.
		static const Color fullColor[5] = {
			Color(.44, .56, .70, 0), Color(.70, .62, .44, 0),
			Color(.60, .60, .60, 0), Color(.70, .44, .44, 0), Color(.70, .62, .44, 0)
		};
		static const Color halfColor[5] = {
			Color(.22, .28, .35, 0), Color(.35, .31, .22, 0),
			Color(.30, .30, .30, 0), Color(.35, .22, .22, 0), Color(.35, .31, .22, 0)
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
}



EscortDisplay::Icon::Icon(const Ship &ship, bool isHere)
	: sprite(ship.GetSprite().GetSprite()),
	isHere(isHere && !ship.IsDisabled()),
	isReadyToJump(ship.CheckHyperspace()),
	stackSize(1),
	cost(ship.Cost()),
	system((!isHere && ship.GetSystem()) ? ship.GetSystem()->Name() : ""),
	low{ship.Shields(), ship.Hull(), ship.Energy(), ship.Heat(), ship.Fuel()},
	high(low)
{
}



// Sorting operator. It comes sooner if it costs more.
bool EscortDisplay::Icon::operator<(const Icon &other) const
{
	return (cost > other.cost);
}



int EscortDisplay::Icon::Height() const
{
	return 30 + 15 * !system.empty();
}



void EscortDisplay::Icon::Merge(const Icon &other)
{
	isHere &= other.isHere;
	isReadyToJump &= other.isReadyToJump;
	stackSize += other.stackSize;
	if(system.empty() && !other.system.empty())
		system = other.system;
	
	for(unsigned i = 0; i < low.size(); ++i)
	{
		low[i] = min(low[i], other.low[i]);
		high[i] = max(high[i], other.high[i]);
	}
}



void EscortDisplay::MergeStacks() const
{
	if(icons.empty())
		return;
	
	int maxHeight = Screen::Height() - 450;
	set<const Sprite *> unstackable;
	while(true)
	{
		Icon *cheapest = nullptr;
		
		int height = 0;
		for(Icon &icon : icons)
		{
			if(unstackable.find(icon.sprite) == unstackable.end() && (!cheapest || *cheapest < icon))
				cheapest = &icon;
			
			height += icon.Height();
		}
		
		if(height < maxHeight || !cheapest)
			break;
		
		// Merge all other instances of this ship's sprite with this icon.
		vector<Icon>::iterator it = icons.begin();
		while(it != icons.end())
		{
			if(&*it == cheapest || it->sprite != cheapest->sprite || it->isHere != cheapest->isHere)
			{
				++it;
				continue;
			}
			
			cheapest->Merge(*it);
			it = icons.erase(it);	
		}
		unstackable.insert(cheapest->sprite);
	}
}
