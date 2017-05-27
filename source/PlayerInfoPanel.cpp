/* PlayerInfoPanel.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "PlayerInfoPanel.h"

#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "MissionPanel.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "Ship.h"
#include "ShipInfoPanel.h"
#include "System.h"
#include "Table.h"
#include "UI.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	// Number of lines per page of the fleet listing.
	const int LINES_PER_PAGE = 26;
	
	// Find any condition strings that begin with the given prefix, and convert
	// them to strings ending in the given suffix (if any). Return those strings
	// plus the values of the conditions.
	vector<pair<int, string>> Match(const PlayerInfo &player, const string &prefix, const string &suffix)
	{
		vector<pair<int, string>> match;
		
		auto it = player.Conditions().lower_bound(prefix);
		for( ; it != player.Conditions().end(); ++it)
		{
			if(it->first.compare(0, prefix.length(), prefix))
				break;
			if(it->second > 0)
				match.push_back(pair<int, string>(it->second, it->first.substr(prefix.length()) + suffix));
		}
		return match;
	}
	
	// Draw a list of (string, value) pairs.
	void DrawList(vector<pair<int, string>> &list, Table &table, const string &title, int maxCount = 0, bool drawValues = true)
	{
		if(list.empty())
			return;
		
		int otherCount = list.size() - maxCount;
		
		if(otherCount > 0 && maxCount > 0)
		{
			list[maxCount - 1].second = "(" + to_string(otherCount + 1) + " Others)";
			while(otherCount--)
			{
				list[maxCount - 1].first += list.back().first;
				list.pop_back();
			}
		}
		
		Color dim = *GameData::Colors().Get("medium");
		Color bright = *GameData::Colors().Get("bright");
		table.DrawGap(10);
		table.DrawUnderline(dim);
		table.Draw(title, bright);
		table.Advance();
		table.DrawGap(5);
		
		for(const pair<int, string> &it : list)
		{
			table.Draw(it.second, dim);
			if(drawValues)
				table.Draw(it.first);
			else
				table.Advance();
		}
	}
}



PlayerInfoPanel::PlayerInfoPanel(PlayerInfo &player)
	: player(player), canEdit(player.GetPlanet())
{
	SetInterruptible(false);
}



void PlayerInfoPanel::Step()
{
	// If the player has acquired a second ship for the first time, explain to
	// them how to reorder the ships in their fleet.
	if(player.Ships().size() > 1)
		DoHelp("multiple ships");
}



void PlayerInfoPanel::Draw()
{
	// Dim everything behind this panel.
	DrawBackdrop();
	
	// Fill in the information for how this interface should be drawn.
	Information interfaceInfo;
	interfaceInfo.SetCondition("player tab");
	if(canEdit && player.Ships().size() > 1)
	{
		bool allParked = true;
		bool hasOtherShips = false;
		const Ship *flagship = player.Flagship();
		for(const auto &it : player.Ships())
			if(!it->IsDisabled() && it.get() != flagship)
			{
				allParked &= it->IsParked();
				hasOtherShips = true;
			}
		if(hasOtherShips)
			interfaceInfo.SetCondition(allParked ? "show unpark all" : "show park all");
		
		// If ships are selected, decide whether the park or unpark button
		// should be shown.
		if(!allSelected.empty())
		{
			bool parkable = false;
			allParked = true;
			for(int i : allSelected)
			{
				const Ship &ship = *player.Ships()[i];
				if(!ship.IsDisabled() && &ship != flagship)
				{
					allParked &= ship.IsParked();
					parkable = true;
				}
			}
			if(parkable)
			{
				interfaceInfo.SetCondition("can park");
				interfaceInfo.SetCondition(allParked ? "show unpark" : "show park");
			}
		}
	}
	interfaceInfo.SetCondition("two buttons");
	
	// Draw the interface.
	const Interface *interface = GameData::Interfaces().Get("info panel");
	interface->Draw(interfaceInfo, this);
	
	// Draw the player and fleet info sections.
	zones.clear();
	DrawPlayer(interface->GetBox("player"));
	DrawFleet(interface->GetBox("fleet"));
}



bool PlayerInfoPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(key == 'd' || key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == 's' || key == SDLK_RETURN || key == SDLK_KP_ENTER)
	{
		if(!player.Ships().empty())
		{
			GetUI()->Pop(this);
			GetUI()->Push(new ShipInfoPanel(player, selectedIndex));
		}
	}
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
	{
		int direction = (key == SDLK_PAGEDOWN) - (key == SDLK_PAGEUP);
		if(Scroll((LINES_PER_PAGE - 2) * direction))
			hoverIndex = -1;
	}
	else if(key == SDLK_UP || key == SDLK_DOWN)
	{
		// Use the arrow keys to select a different ship.
		int direction = (key == SDLK_DOWN) - (key == SDLK_UP);
		// Special case: up when nothing is selected selects the last ship.
		if(key == SDLK_UP && selectedIndex < 0)
			selectedIndex = player.Ships().size();
		// Move the selection up or down one space.
		selectedIndex += direction;
		// Down arrow when the last ship is selected deselects all.
		if(static_cast<unsigned>(selectedIndex) >= player.Ships().size())
			selectedIndex = -1;
		// Update the selection.
		bool hasMod = (SDL_GetModState() & (KMOD_SHIFT | KMOD_CTRL | KMOD_GUI));
		if(!hasMod)
			allSelected.clear();
		if(selectedIndex >= 0)
			allSelected.insert(selectedIndex);
		
		// Update the scroll if necessary to keep the selected ship on screen.
		int scrollDirection = ((selectedIndex >= scroll + LINES_PER_PAGE) - (selectedIndex < scroll));
		if(Scroll((LINES_PER_PAGE - 2) * scrollDirection))
			hoverIndex = -1;
	}
	else if(canEdit && key == 'P' && !allSelected.empty())
	{
		// Toggle the parked status for all selected ships.
		bool allParked = true;
		const Ship *flagship = player.Flagship();
		for(int i : allSelected)
		{
			const Ship &ship = *player.Ships()[i];
			if(!ship.IsDisabled() && &ship != flagship)
				allParked &= ship.IsParked();
		}
		
		for(int i : allSelected)
		{
			const Ship &ship = *player.Ships()[i];
			if(!ship.IsDisabled() && &ship != flagship)
				player.ParkShip(&ship, !allParked);
		}
	}
	else if(canEdit && key == 'A' && player.Ships().size() > 1)
	{
		// Toggle the parked status for all ships except the flagship.
		bool allParked = true;
		const Ship *flagship = player.Flagship();
		for(const auto &it : player.Ships())
			if(!it->IsDisabled() && it.get() != flagship)
				allParked &= it->IsParked();
		
		for(const auto &it : player.Ships())
			if(!it->IsDisabled() && (allParked || it.get() != flagship))
				player.ParkShip(it.get(), !allParked);
	}
	else if(command.Has(Command::INFO | Command::MAP) || key == 'm')
		GetUI()->Push(new MissionPanel(player));
	else
		return false;
	
	return true;
}



bool PlayerInfoPanel::Click(int x, int y, int clicks)
{
	draggingIndex = -1;
	bool shift = (SDL_GetModState() & KMOD_SHIFT);
	bool control = (SDL_GetModState() & (KMOD_CTRL | KMOD_GUI));
	if(canEdit && (shift || control || clicks < 2))
	{
		// Only allow changing your flagship when landed.
		if(hoverIndex >= 0)
		{
			if(shift)
			{
				// Select all the ships between the previous selection and this one.
				for(int i = max(0, min(selectedIndex, hoverIndex)); i < max(selectedIndex, hoverIndex); ++i)
					allSelected.insert(i);
			}
			else if(!control)
			{
				allSelected.clear();
				draggingIndex = hoverIndex;
			}
			
			if(control && allSelected.count(hoverIndex))
				allSelected.erase(hoverIndex);
			else
			{
				allSelected.insert(hoverIndex);
				selectedIndex = hoverIndex;
			}
		}
	}
	else if(hoverIndex >= 0)
	{
		// If not landed, clicking a ship name takes you straight to its info.
		GetUI()->Pop(this);
		GetUI()->Push(new ShipInfoPanel(player, hoverIndex));
	}
	
	return true;
}



bool PlayerInfoPanel::Hover(int x, int y)
{
	return Hover(Point(x, y));
}



bool PlayerInfoPanel::Drag(double dx, double dy)
{
	return Hover(hoverPoint + Point(dx, dy));
}



bool PlayerInfoPanel::Release(int x, int y)
{
	// Drag and drop can be used to reorder the player's ships.
	// TODO: insert *every* selected ship at this index.
	if(canEdit && draggingIndex >= 0 && hoverIndex >= 0 && hoverIndex != draggingIndex)
	{
		player.ReorderShip(draggingIndex, hoverIndex);
	
		// The ship we just dragged should remain selected.
		selectedIndex = hoverIndex;
		allSelected.clear();
		if(selectedIndex >= 0)
			allSelected.insert(selectedIndex);
	}
	draggingIndex = -1;
	
	return true;
}



bool PlayerInfoPanel::Scroll(double dx, double dy)
{
	return Scroll(dy * -.1 * Preferences::ScrollSpeed());
}



void PlayerInfoPanel::DrawPlayer(const Rectangle &bounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < 250.)
		return;
	
	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	
	// Table attributes.
	Table table;
	table.AddColumn(0, Table::LEFT);
	table.AddColumn(230, Table::RIGHT);
	table.SetUnderline(0, 230);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));
	
	// Header row.
	table.Draw("player:", dim);
	table.Draw(player.FirstName() + " " + player.LastName(), bright);
	table.Draw("net worth:", dim);
	table.Draw(Format::Number(player.Accounts().NetWorth()) + " credits", bright);
	
	// Determine the player's combat rating.
	static const vector<string> &RATINGS = GameData::CombatRatings();
	if(!RATINGS.empty())
	{
		table.DrawGap(10);
		table.DrawUnderline(dim);
		table.Draw("combat rating:", bright);
		table.Advance();
		table.DrawGap(5);
		
		int ratingLevel = min<int>(RATINGS.size() - 1, log(max(1, player.GetCondition("combat rating"))));
		table.Draw(RATINGS[ratingLevel], dim);
		table.Draw("(" + to_string(ratingLevel) + ")", dim);
	}
	
	// Other special information:
	auto salary = Match(player, "salary: ", "");
	sort(salary.begin(), salary.end());
	DrawList(salary, table, "salary:", 4);
	
	auto tribute = Match(player, "tribute: ", "");
	sort(tribute.begin(), tribute.end());
	DrawList(tribute, table, "tribute:", 4);
	
	int maxRows = static_cast<int>(250. - 30. - table.GetPoint().Y()) / 20;
	auto licenses = Match(player, "license: ", " License");
	DrawList(licenses, table, "licenses:", maxRows, false);
}



void PlayerInfoPanel::DrawFleet(const Rectangle &bounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < 750.)
		return;
	
	// Colors to draw with.
	Color back = *GameData::Colors().Get("faint");
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	Color elsewhere = *GameData::Colors().Get("dim");
	Color dead(.4, 0., 0., 0.);
	
	// Table attributes.
	Table table;
	table.AddColumn(0, Table::LEFT);
	table.AddColumn(220, Table::LEFT);
	table.AddColumn(350, Table::LEFT);
	table.AddColumn(550, Table::RIGHT);
	table.AddColumn(610, Table::RIGHT);
	table.AddColumn(670, Table::RIGHT);
	table.AddColumn(730, Table::RIGHT);
	table.SetUnderline(0, 730);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));
	
	// Header row.
	table.DrawUnderline(dim);
	table.SetColor(bright);
	table.Draw("ship");
	table.Draw("model");
	table.Draw("system");
	table.Draw("shields");
	table.Draw("hull");
	table.Draw("fuel");
	table.Draw("crew");
	table.DrawGap(5);
	
	// Loop through all the player's ships.
	int index = scroll;
	auto sit = player.Ships().begin() + scroll;
	for( ; sit < player.Ships().end(); ++sit)
	{
		// Bail out if we've used out the whole drawing area.
		if(!bounds.Contains(table.GetRowBounds()))
			break;
		
		// Check if this row is selected.
		if(allSelected.count(index))
			table.DrawHighlight(back);
		
		const Ship &ship = **sit;
		bool isElsewhere = (ship.GetSystem() != player.GetSystem());
		isElsewhere |= (ship.CanBeCarried() && player.GetPlanet());
		bool isDead = ship.IsDestroyed() || ship.IsDisabled();
		bool isHovered = (index == hoverIndex);
		table.SetColor(isDead ? dead : isElsewhere ? elsewhere : isHovered ? bright : dim);
		
		// Store this row's position, to handle hovering.
		zones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), index);
		
		// Indent the ship name if it is a fighter or drone.
		table.Draw(ship.CanBeCarried() ? "    " + ship.Name() : ship.Name());
		table.Draw(ship.ModelName());
		
		const System *system = ship.GetSystem();
		table.Draw(system ? system->Name() : "");
		
		string shields = to_string(static_cast<int>(100. * max(0., ship.Shields()))) + "%";
		table.Draw(shields);
		
		string hull = to_string(static_cast<int>(100. * max(0., ship.Hull()))) + "%";
		table.Draw(hull);
		
		string fuel = to_string(static_cast<int>(
			ship.Attributes().Get("fuel capacity") * ship.Fuel()));
		table.Draw(fuel);
		
		// If this isn't the flagship, we'll remember how many crew it has, but
		// only the minimum number of crew need to be paid for.
		int crewCount = ship.Crew();
		if(&ship != player.Flagship())
			crewCount = min(crewCount, ship.RequiredCrew());
		string crew = (ship.IsParked() ? "Parked" : to_string(crewCount));
		table.Draw(crew);
		
		++index;
	}
	
	// Re-ordering ships in your fleet.
	if(draggingIndex >= 0)
	{
		const Font &font = FontSet::Get(14);
		const string &name = player.Ships()[draggingIndex]->Name();
		Point pos(hoverPoint.X() - .5 * font.Width(name), hoverPoint.Y());
		font.Draw(name, pos + Point(1., 1.), Color(0., 1.));
		font.Draw(name, pos, bright);
	}
}



bool PlayerInfoPanel::Hover(const Point &point)
{
	hoverPoint = point;
	hoverIndex = -1;
	for(const auto &zone : zones)
		if(zone.Contains(hoverPoint))
			hoverIndex = zone.Value();
	
	return true;
}



// Adjust the scroll by the given amount. Return true if it changed.
bool PlayerInfoPanel::Scroll(int distance)
{
	int newScroll = max(0, min<int>(player.Ships().size() - LINES_PER_PAGE, scroll + distance));
	if(scroll == newScroll)
		return false;
	
	scroll = newScroll;
	return true;
}
