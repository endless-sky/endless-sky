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

#include "Command.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "LogbookPanel.h"
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
#include <utility>

using namespace std;

namespace {
	// Number of lines per page of the fleet listing.
	const int LINES_PER_PAGE = 26;
	
	// Find any condition strings that begin with the given prefix, and convert
	// them to strings ending in the given suffix (if any). Return those strings
	// plus the values of the conditions.
	vector<pair<int64_t, string>> Match(const PlayerInfo &player, const string &prefix, const string &suffix)
	{
		vector<pair<int64_t, string>> match;
		
		auto it = player.Conditions().lower_bound(prefix);
		for( ; it != player.Conditions().end(); ++it)
		{
			if(it->first.compare(0, prefix.length(), prefix))
				break;
			if(it->second > 0)
				match.emplace_back(it->second, it->first.substr(prefix.length()) + suffix);
		}
		return match;
	}
	
	// Draw a list of (string, value) pairs.
	void DrawList(vector<pair<int64_t, string>> &list, Table &table, const string &title, int maxCount = 0, bool drawValues = true)
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
		
		const Color &dim = *GameData::Colors().Get("medium");
		table.DrawGap(10);
		table.DrawUnderline(dim);
		table.Draw(title, *GameData::Colors().Get("bright"));
		table.Advance();
		table.DrawGap(5);
		
		for(const auto &it : list)
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
	interfaceInfo.SetCondition("three buttons");
	if(player.HasLogs())
		interfaceInfo.SetCondition("enable logbook");
	
	// Draw the interface.
	const Interface *interface = GameData::Interfaces().Get("info panel");
	interface->Draw(interfaceInfo, this);
	
	// Draw the player and fleet info sections.
	zones.clear();
	DrawPlayer(interface->GetBox("player"));
	DrawFleet(interface->GetBox("fleet"));
}



bool PlayerInfoPanel::AllowFastForward() const
{
	return true;
}



bool PlayerInfoPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	bool control = (mod & (KMOD_CTRL | KMOD_GUI));
	bool shift = (mod & KMOD_SHIFT);
	if(key == 'd' || key == SDLK_ESCAPE || (key == 'w' && control)
			|| key == 'i' || command.Has(Command::INFO))
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
		if(selectedIndex < 0)
		{
			// If no ship was selected, moving up or down selects the first or
			// last ship, and the scroll jumps to the first or last page.
			if(key == SDLK_UP)
			{
				selectedIndex = player.Ships().size() - 1;
				Scroll(player.Ships().size());
			}
			else
			{
				selectedIndex = 0;
				Scroll(-player.Ships().size());
			}
		}
		// Holding both Ctrl & Shift keys and using the arrows moves the
		// selected ship group up or down one row.
		else if(!allSelected.empty() && control && shift)
		{
			// Move based on the position of the first selected ship. An upward
			// movement is a shift of one, while a downward move shifts 1 and
			// then 1 for each ship in the contiguous selection.
			size_t toIndex = *allSelected.begin();
			if(key == SDLK_UP && toIndex > 0)
				--toIndex;
			else if(key == SDLK_DOWN)
			{
				int next = ++toIndex;
				for(set<int>::const_iterator sel = allSelected.begin(); ++sel != allSelected.end(); )
				{
					if(*sel != next)
						break;
					
					++toIndex;
					++next;
				}
			}
			
			// Clamp the destination index to the end of the ships list.
			size_t moved = allSelected.size();
			toIndex = min(player.Ships().size() - moved, toIndex);
			selectedIndex = player.ReorderShips(allSelected, toIndex);
			// If the move accessed invalid indices, no moves are done
			// but the selectedIndex is set to -1.
			if(selectedIndex < 0)
				selectedIndex = *allSelected.begin();
			else
			{
				// Update the selected indices so they still refer
				// to the block of ships that just got moved.
				int lastIndex = selectedIndex + moved;
				allSelected.clear();
				for(int i = selectedIndex; i < lastIndex; ++i)
					allSelected.insert(i);
			}
			// Update the scroll if necessary to keep the selected ship on screen.
			int scrollDirection = ((selectedIndex >= scroll + LINES_PER_PAGE) - (selectedIndex < scroll));
			if(selectedIndex >= 0 && Scroll((LINES_PER_PAGE - 2) * scrollDirection))
				hoverIndex = -1;
			return true;
		}
		else
		{
			// Move the selection up or down one space.
			selectedIndex += (key == SDLK_DOWN) - (key == SDLK_UP);
			// Down arrow when the last ship is selected deselects all.
			if(static_cast<unsigned>(selectedIndex) >= player.Ships().size())
				selectedIndex = -1;
			
			// Update the scroll if necessary to keep the selected ship on screen.
			int scrollDirection = ((selectedIndex >= scroll + LINES_PER_PAGE) - (selectedIndex < scroll));
			if(selectedIndex >= 0 && Scroll((LINES_PER_PAGE - 2) * scrollDirection))
				hoverIndex = -1;
		}
		// Update the selection.
		bool hasMod = (SDL_GetModState() & (KMOD_SHIFT | KMOD_CTRL | KMOD_GUI));
		if(!hasMod)
			allSelected.clear();
		if(selectedIndex >= 0)
			allSelected.insert(selectedIndex);
	}
	else if(canEdit && (key == 'P' || (key == 'p' && shift)) && !allSelected.empty())
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
	else if(canEdit && (key == 'A' || (key == 'a' && shift)) && player.Ships().size() > 1)
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
	else if(command.Has(Command::MAP) || key == 'm')
		GetUI()->Push(new MissionPanel(player));
	else if(key == 'l' && player.HasLogs())
		GetUI()->Push(new LogbookPanel(player));
	else if(key >= '0' && key <= '9')
	{
		int group = key - '0';
		if(control)
		{
			// Convert from indices into ship pointers.
			set<Ship *> selected;
			for(int i : allSelected)
				selected.insert(player.Ships()[i].get());
			player.SetGroup(group, &selected);
		}
		else
		{
			// Convert ship pointers into indices in the ship list.
			set<int> added;
			for(Ship *ship : player.GetGroup(group))
				for(unsigned i = 0; i < player.Ships().size(); ++i)
					if(player.Ships()[i].get() == ship)
						added.insert(i);
			
			// If the shift key is not down, replace the current set of selected
			// ships with the group with the given index.
			if(!shift)
				allSelected = added;
			else
			{
				// If every single ship in this group is already selected, shift
				// plus the group number means to deselect all those ships.
				bool allWereSelected = true;
				for(int i : added)
					allWereSelected &= allSelected.erase(i);
				
				if(!allWereSelected)
					for(int i : added)
						allSelected.insert(i);
			}
			
			// Any ships are selected now, the first one is the selected index.
			selectedIndex = (allSelected.empty() ? -1 : *allSelected.begin());
		}
	}
	else
		return false;
	
	return true;
}



bool PlayerInfoPanel::Click(int /* x */, int /* y */, int clicks)
{
	// Do nothing if the click was not on one of the ships in the fleet list.
	if(hoverIndex < 0)
		return true;
	
	bool shift = (SDL_GetModState() & KMOD_SHIFT);
	bool control = (SDL_GetModState() & (KMOD_CTRL | KMOD_GUI));
	if(canEdit && (shift || control || clicks < 2))
	{
		// Only allow changing your flagship when landed.
		if(control && allSelected.count(hoverIndex))
			allSelected.erase(hoverIndex);
		else
		{
			if(allSelected.count(hoverIndex))
			{
				// If the click is on an already selected line, start dragging
				// but do not change the selection.
			}
			else if(control)
				allSelected.insert(hoverIndex);
			else if(shift)
			{
				// Select all the ships between the previous selection and this one.
				for(int i = max(0, min(selectedIndex, hoverIndex)); i <= max(selectedIndex, hoverIndex); ++i)
					allSelected.insert(i);
			}
			else
			{
				allSelected.clear();
				allSelected.insert(hoverIndex);
			}
			selectedIndex = hoverIndex;
		}
	}
	else
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
	isDragging = true;
	return Hover(hoverPoint + Point(dx, dy));
}



bool PlayerInfoPanel::Release(int x, int y)
{
	if(!isDragging)
		return true;
	isDragging = false;
	
	// Do nothing if the block of ships has not been dragged to a valid new
	// location in the list, or if it's not possible to reorder the list.
	if(!canEdit || hoverIndex < 0 || hoverIndex == selectedIndex)
		return true;
	
	// Try to move all the selected ships to this location.
	selectedIndex = player.ReorderShips(allSelected, hoverIndex);
	if(selectedIndex < 0)
		return true;
	
	// Change the selected indices so they still refer to the block of ships
	// that just got moved.
	int lastIndex = selectedIndex + allSelected.size();
	allSelected.clear();
	for(int i = selectedIndex; i < lastIndex; ++i)
		allSelected.insert(i);
	
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
	table.Draw(Format::Credits(player.Accounts().NetWorth()) + " credits", bright);
	
	// Determine the player's combat rating.
	int combatLevel = log(max<int64_t>(1, player.GetCondition("combat rating")));
	const string &combatRating = GameData::Rating("combat", combatLevel);
	if(!combatRating.empty())
	{
		table.DrawGap(10);
		table.DrawUnderline(dim);
		table.Draw("combat rating:", bright);
		table.Advance();
		table.DrawGap(5);
		
		table.Draw(combatRating, dim);
		table.Draw("(" + to_string(combatLevel) + ")", dim);
	}
	
	// Display the factors affecting piracy targeting the player.
	pair<double, double> factors = player.RaidFleetFactors();
	double attractionLevel = max(0., log2(max(factors.first, 0.)));
	double deterrenceLevel = max(0., log2(max(factors.second, 0.)));
	const string &attractionRating = GameData::Rating("cargo attractiveness", attractionLevel);
	const string &deterrenceRating = GameData::Rating("armament deterrence", deterrenceLevel);
	if(!attractionRating.empty() && !deterrenceRating.empty())
	{
		double attraction = max(0., min(1., .005 * (factors.first - factors.second - 2.)));
		double prob = 1. - pow(1. - attraction, 10.);
		
		table.DrawGap(10);
		table.DrawUnderline(dim);
		table.Draw("piracy threat:", bright);
		table.Draw(to_string(lround(100 * prob)) + "%", dim);
		table.DrawGap(5);
		
		// Format the attraction and deterrence levels with tens places, so it
		// is clear which is higher even if they round to the same level.
		table.Draw("cargo: " + attractionRating, dim);
		table.Draw("(+" + Format::Decimal(attractionLevel, 1) + ")", dim);
		table.DrawGap(5);
		table.Draw("fleet: " + deterrenceRating, dim);
		table.Draw("(-" + Format::Decimal(deterrenceLevel, 1) + ")", dim);
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
	Color dead(.4f, 0.f, 0.f, 0.f);
	
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
	const Font &font = FontSet::Get(14);
	for(auto sit = player.Ships().begin() + scroll; sit < player.Ships().end(); ++sit)
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
		table.Draw(font.TruncateMiddle(ship.CanBeCarried() ? "    " + ship.Name() : ship.Name(), 217));
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
	if(isDragging)
	{
		const Font &font = FontSet::Get(14);
		Point pos(hoverPoint.X(), hoverPoint.Y());
		for(int i : allSelected)
		{
			const string &name = player.Ships()[i]->Name();
			font.Draw(name, pos + Point(1., 1.), Color(0., 1.));
			font.Draw(name, pos, bright);
			pos.Y() += 20.;
		}
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
