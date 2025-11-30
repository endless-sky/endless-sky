/* PlayerInfoPanel.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "PlayerInfoPanel.h"

#include "text/Alignment.h"
#include "audio/Audio.h"
#include "Command.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "InfoPanelState.h"
#include "Information.h"
#include "Interface.h"
#include "text/Layout.h"
#include "LogbookPanel.h"
#include "MissionPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "Ship.h"
#include "ShipInfoPanel.h"
#include "System.h"
#include "text/Table.h"
#include "text/Truncate.h"
#include "UI.h"

#include <algorithm>
#include <cmath>
#include <utility>

using namespace std;

namespace {
	// Number of lines per page of the fleet listing.
	const int LINES_PER_PAGE = 26;

	// Draw a list of (string, value) pairs.
	void DrawList(vector<pair<int64_t, string>> &list, const Table &table, const string &title,
		int64_t titleValue, int maxCount = 0, bool drawValues = true)
	{
		if(list.empty())
			return;

		int otherCount = list.size() - maxCount;

		if(otherCount > 0 && maxCount > 0)
		{
			list[maxCount - 1].second = "(" + to_string(otherCount + 1) + " others)";
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
		table.Draw(titleValue, dim);
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

	bool CompareName(const shared_ptr<Ship> &lhs, const shared_ptr<Ship> &rhs)
	{
		return lhs->GivenName() < rhs->GivenName();
	}

	bool CompareModelName(const shared_ptr<Ship> &lhs, const shared_ptr<Ship> &rhs)
	{
		return lhs->DisplayModelName() < rhs->DisplayModelName();
	}

	bool CompareSystem(const shared_ptr<Ship> &lhs, const shared_ptr<Ship> &rhs)
	{
		// Ships (drones) with no system are sorted to the end.
		if(lhs->GetSystem() == nullptr)
			return false;
		else if(rhs->GetSystem() == nullptr)
			return true;
		return lhs->GetSystem()->DisplayName() < rhs->GetSystem()->DisplayName();
	}

	bool CompareShields(const shared_ptr<Ship> &lhs, const shared_ptr<Ship> &rhs)
	{
		return lhs->Shields() < rhs->Shields();
	}

	bool CompareHull(const shared_ptr<Ship> &lhs, const shared_ptr<Ship> &rhs)
	{
		return lhs->Hull() < rhs->Hull();
	}

	bool CompareFuel(const shared_ptr<Ship> &lhs, const shared_ptr<Ship> &rhs)
	{
		return lhs->Attributes().Get("fuel capacity") * lhs->Fuel() <
			rhs->Attributes().Get("fuel capacity") * rhs->Fuel();
	}

	bool CompareRequiredCrew(const shared_ptr<Ship> &lhs, const shared_ptr<Ship> &rhs)
	{
		// Parked ships are sorted to the end.
		if(lhs->IsParked())
			return false;
		else if(rhs->IsParked())
			return true;
		return lhs->RequiredCrew() < rhs->RequiredCrew();
	}

	// A helper function for reversing the arguments of the given function F.
	template<InfoPanelState::ShipComparator &F>
	bool ReverseCompare(const shared_ptr<Ship> &lhs, const shared_ptr<Ship> &rhs)
	{
		return F(rhs, lhs);
	}

	// Reverses the argument order of the given comparator function.
	InfoPanelState::ShipComparator &GetReverseCompareFrom(InfoPanelState::ShipComparator &f)
	{
		if(f == &CompareName)
			return ReverseCompare<CompareName>;
		else if(f == &CompareModelName)
			return ReverseCompare<CompareModelName>;
		else if(f == &CompareSystem)
			return ReverseCompare<CompareSystem>;
		else if(f == &CompareShields)
			return ReverseCompare<CompareShields>;
		else if(f == &CompareHull)
			return ReverseCompare<CompareHull>;
		else if(f == &CompareFuel)
			return ReverseCompare<CompareFuel>;
		return ReverseCompare<CompareRequiredCrew>;
	}
}

// Table columns and their starting x positions, end x positions, alignment and sort comparator.
const PlayerInfoPanel::SortableColumn PlayerInfoPanel::columns[7] = {
	SortableColumn("ship", 0, 217, {217, Truncate::MIDDLE}, CompareName),
	SortableColumn("model", 220, 347, {127, Truncate::BACK}, CompareModelName),
	SortableColumn("system", 350, 487, {137, Truncate::BACK}, CompareSystem),
	SortableColumn("shields", 550, 493, {57, Alignment::RIGHT, Truncate::BACK}, CompareShields),
	SortableColumn("hull", 610, 553, {57, Alignment::RIGHT, Truncate::BACK}, CompareHull),
	SortableColumn("fuel", 670, 613, {57, Alignment::RIGHT, Truncate::BACK}, CompareFuel),
	SortableColumn("crew", 730, 673, {57, Alignment::RIGHT, Truncate::BACK}, CompareRequiredCrew)
};



PlayerInfoPanel::PlayerInfoPanel(PlayerInfo &player)
	: PlayerInfoPanel(player, InfoPanelState(player))
{
}

PlayerInfoPanel::PlayerInfoPanel(PlayerInfo &player, InfoPanelState panelState)
	: player(player), panelState(panelState)
{
	Audio::Pause();
	SetInterruptible(false);
}



PlayerInfoPanel::~PlayerInfoPanel()
{
	Audio::Resume();
}



void PlayerInfoPanel::Step()
{
	if(GetUI()->IsTop(this) && !checkedHelp)
	{
		if(DoHelp("player info"))
		{
			// Nothing to do here, just don't want to execute the other branch.
		}
		// If the player has acquired a second ship for the first time, explain to
		// them how to reorder and sort the ships in their fleet.
		else if(panelState.Ships().size() > 1)
			if(!DoHelp("multiple ships"))
				DoHelp("fleet management");
		checkedHelp = true;
	}
}



void PlayerInfoPanel::Draw()
{
	// Dim everything behind this panel.
	DrawBackdrop();

	// Fill in the information for how this interface should be drawn.
	Information interfaceInfo;
	interfaceInfo.SetCondition("player tab");
	if(panelState.CanEdit() && !panelState.Ships().empty())
	{
		bool allParked = true;
		bool allParkedSystem = true;
		bool hasOtherShips = false;
		const Ship *flagship = player.Flagship();
		const System *flagshipSystem = flagship ? flagship->GetSystem() : player.GetSystem();
		for(const auto &it : panelState.Ships())
			if(!it->IsDisabled() && it.get() != flagship)
			{
				allParked &= it->IsParked();
				hasOtherShips = true;
				if(it->GetSystem() == flagshipSystem)
					allParkedSystem &= it->IsParked();
			}
		if(hasOtherShips)
		{
			interfaceInfo.SetCondition(allParked ? "show unpark all" : "show park all");
			interfaceInfo.SetCondition(allParkedSystem ? "show unpark system" : "show park system");
		}

		// If ships are selected, decide whether the park or unpark button
		// should be shown.
		if(!panelState.AllSelected().empty())
		{
			bool parkable = false;
			allParked = true;
			for(int i : panelState.AllSelected())
			{
				const Ship &ship = *panelState.Ships()[i];
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

		// If ship order has changed by choosing a sort comparison,
		// show the save order button. Any manual sort by the player
		// is applied immediately and doesn't need this button.
		if(panelState.CanEdit() && panelState.CurrentSort())
			interfaceInfo.SetCondition("show save order");
	}

	interfaceInfo.SetCondition("three buttons");
	if(player.HasLogs())
		interfaceInfo.SetCondition("enable logbook");

	// Draw the interface.
	const Interface *infoPanelUi = GameData::Interfaces().Get("info panel");
	infoPanelUi->Draw(interfaceInfo, this);

	// Draw the player and fleet info sections.
	menuZones.clear();

	DrawPlayer(infoPanelUi->GetBox("player"));
	DrawFleet(infoPanelUi->GetBox("fleet"));
}



bool PlayerInfoPanel::AllowsFastForward() const noexcept
{
	return true;
}



bool PlayerInfoPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	bool control = (mod & (KMOD_CTRL | KMOD_GUI));
	bool shift = (mod & KMOD_SHIFT);
	if(key == 'd' || key == SDLK_ESCAPE || (key == 'w' && control)
			|| key == 'i' || command.Has(Command::INFO))
	{
		GetUI()->Pop(this);
	}
	else if(command.Has(Command::HELP))
	{
		if(panelState.Ships().size() > 1)
		{
			DoHelp("fleet management", true);
			DoHelp("multiple ships", true);
		}
		DoHelp("player info", true);
	}
	else if(key == 's' || key == SDLK_RETURN || key == SDLK_KP_ENTER || (control && key == SDLK_TAB))
	{
		if(!panelState.Ships().empty())
		{
			GetUI()->Pop(this);
			GetUI()->Push(new ShipInfoPanel(player, std::move(panelState)));
		}
	}
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
	{
		int direction = (key == SDLK_PAGEDOWN) - (key == SDLK_PAGEUP);
		Scroll((LINES_PER_PAGE - 2) * direction);
	}
	else if(key == SDLK_HOME)
		Scroll(-static_cast<int>(player.Ships().size()));
	else if(key == SDLK_END)
		Scroll(player.Ships().size());
	else if(key == SDLK_UP || key == SDLK_DOWN)
	{
		if(panelState.AllSelected().empty())
		{
			// If no ship was selected, moving up or down selects the first or last ship.
			if(isNewPress)
			{
				if(key == SDLK_UP)
					panelState.SetSelectedIndex(panelState.Ships().size() - 1);
				else
					panelState.SetSelectedIndex(0);
			}
		}
		// Holding both Ctrl & Shift keys and using the arrows moves the
		// selected ship group up or down one row.
		else if(panelState.CanEdit() && !panelState.AllSelected().empty() && control && shift)
		{
			// Move based on the position of the first selected ship. An upward
			// movement is a shift of one, while a downward move shifts 1 and
			// then 1 for each ship in the contiguous selection.
			size_t toIndex = *panelState.AllSelected().begin();
			if(key == SDLK_UP && toIndex > 0)
				--toIndex;
			else if(key == SDLK_DOWN)
			{
				int next = ++toIndex;
				for(const auto sel : panelState.AllSelected())
				{
					if(sel != next)
						break;

					++toIndex;
					++next;
				}
			}

			// Clamp the destination index to the end of the ships list.
			size_t moved = panelState.AllSelected().size();
			toIndex = min(panelState.Ships().size() - moved, toIndex);

			if(panelState.ReorderShipsTo(toIndex))
				ScrollAbsolute(panelState.SelectedIndex() - 12);
			return true;
		}
		else
		{
			// Move the selection up or down one space.
			int selectedIndex = panelState.SelectedIndex() + (key == SDLK_DOWN) - (key == SDLK_UP);
			bool isValidIndex = static_cast<unsigned>(selectedIndex) < panelState.Ships().size();
			if(selectedIndex < 0)
			{
				if(isNewPress)
					panelState.DeselectAll();
			}
			else if(shift)
			{
				if(panelState.AllSelected().contains(selectedIndex))
					panelState.Deselect(panelState.SelectedIndex());
				if(isValidIndex)
					panelState.SetSelectedIndex(selectedIndex);
			}
			else if(control)
			{
				// If ctrl is down, select current ship.
				if(isValidIndex)
					panelState.SetSelectedIndex(selectedIndex);
			}
			else if(isValidIndex)
				panelState.SelectOnly(selectedIndex);
			else if(isNewPress)
				panelState.DeselectAll();
		}

		// Update the scroll.
		int selected = panelState.SelectedIndex();
		if(selected >= 0)
		{
			if(selected < panelState.Scroll() + LINES_PER_PAGE && selected >= panelState.Scroll())
			{
				// If the selected ship is on screen, do not scroll.
			}
			else if(selected == panelState.Scroll() + LINES_PER_PAGE)
				Scroll(1);
			else if(selected == panelState.Scroll() - 1)
				Scroll(-1);
			else if(key == SDLK_UP)
				ScrollAbsolute(selected - LINES_PER_PAGE + 1);
			else
				ScrollAbsolute(selected);
		}
	}
	else if(panelState.CanEdit() && (key == 'k' || (key == 'p' && shift)) && !panelState.AllSelected().empty())
	{
		// Toggle the parked status for all selected ships.
		bool allParked = true;
		const Ship *flagship = player.Flagship();
		for(int i : panelState.AllSelected())
		{
			const Ship &ship = *panelState.Ships()[i];
			if(!ship.IsDisabled() && &ship != flagship)
				allParked &= ship.IsParked();
		}

		for(int i : panelState.AllSelected())
		{
			const Ship &ship = *panelState.Ships()[i];
			if(!ship.IsDisabled() && &ship != flagship)
				player.ParkShip(&ship, !allParked);
		}
	}
	else if(panelState.CanEdit() && (key == 'a') && !panelState.Ships().empty())
	{
		// Toggle the parked status for all ships except the flagship.
		bool allParked = true;
		const Ship *flagship = player.Flagship();
		for(const auto &it : panelState.Ships())
			if(!it->IsDisabled() && it.get() != flagship)
				allParked &= it->IsParked();

		for(const auto &it : panelState.Ships())
			if(!it->IsDisabled() && (allParked || it.get() != flagship))
				player.ParkShip(it.get(), !allParked);
	}
	else if(panelState.CanEdit() && (key == 'c') && !panelState.Ships().empty())
	{
		// Toggle the parked status for all ships in system except the flagship.
		bool allParked = true;
		const Ship *flagship = player.Flagship();
		const System *flagshipSystem = flagship ? flagship->GetSystem() : player.GetSystem();
		for(const auto &it : panelState.Ships())
			if(!it->IsDisabled() && it.get() != flagship && it->GetSystem() == flagshipSystem)
				allParked &= it->IsParked();

		for(const auto &it : panelState.Ships())
			if(!it->IsDisabled() && (allParked || it.get() != flagship) && it->GetSystem() == flagshipSystem)
				player.ParkShip(it.get(), !allParked);
	}
	// If "Save order" button is pressed.
	else if(panelState.CanEdit() && panelState.CurrentSort() && key == 'v')
	{
		player.SetShipOrder(panelState.Ships());
		panelState.SetCurrentSort(nullptr);
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
			for(int i : panelState.AllSelected())
				selected.insert(panelState.Ships()[i].get());
			player.SetEscortGroup(group, &selected);
		}
		else
		{
			// Convert ship pointers into indices in the ship list.
			set<int> added;
			for(Ship *ship : player.GetEscortGroup(group))
				for(size_t i = 0; i < panelState.Ships().size(); ++i)
					if(panelState.Ships()[i].get() == ship)
						added.insert(i);

			// If the shift key is not down, replace the current set of selected
			// ships with the group with the given index.
			if(!shift)
				panelState.SetSelected(added);
			else if(!added.empty())
			{
				// If every single ship in this group is already selected, shift
				// plus the group number means to deselect all those ships.
				bool allWereSelected = true;
				for(int i : added)
					allWereSelected &= panelState.Deselect(i);

				if(!allWereSelected)
				{
					for(int i : added)
						panelState.Select(i);
					panelState.SetSelectedIndex(*added.begin());
				}
			}
			ScrollAbsolute(panelState.SelectedIndex());
		}
	}
	else
		return false;

	return true;
}



bool PlayerInfoPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;

	// Sort the ships if the click was on one of the column headers.
	Point mouse = Point(x, y);
	for(auto &zone : menuZones)
		if(zone.Contains(mouse))
		{
			SortShips(*zone.Value());
			return true;
		}

	// Do nothing if the click was not on one of the ships in the fleet list.
	if(hoverIndex < 0)
		return true;

	bool shift = (SDL_GetModState() & KMOD_SHIFT);
	bool control = (SDL_GetModState() & (KMOD_CTRL | KMOD_GUI));
	if(panelState.CanEdit() && (shift || control || clicks < 2))
	{
		// If the control+click was on an already selected ship, deselect it.
		if(control && panelState.AllSelected().contains(hoverIndex))
			panelState.Deselect(hoverIndex);
		else
		{
			if(control)
				panelState.SetSelectedIndex(hoverIndex);
			else if(shift)
			{
				// Select all the ships between the previous selection and this one.
				int start = max(0, min(panelState.SelectedIndex(), hoverIndex));
				int end = max(panelState.SelectedIndex(), hoverIndex);
				panelState.SelectMany(start, end + 1);
				panelState.SetSelectedIndex(hoverIndex);
			}
			else if(panelState.AllSelected().contains(hoverIndex))
			{
				// If the click is on an already selected line, start dragging
				// but do not change the selection.
			}
			else
				panelState.SelectOnly(hoverIndex);
		}
	}
	else
	{
		const bool sameIndex = panelState.SelectedIndex() == hoverIndex;
		panelState.SelectOnly(hoverIndex);
		// If not landed, clicking a ship name takes you straight to its info.
		if(!panelState.CanEdit() || sameIndex)
		{
			GetUI()->Pop(this);
			GetUI()->Push(new ShipInfoPanel(player, std::move(panelState)));
		}
	}

	return true;
}



bool PlayerInfoPanel::Drag(double dx, double dy)
{
	isDragging = true;
	return Hover(hoverPoint + Point(dx, dy));
}



bool PlayerInfoPanel::Release(int /* x */, int /* y */, MouseButton button)
{
	if(button != MouseButton::LEFT)
		return false;
	if(!isDragging)
		return true;
	isDragging = false;

	// Do nothing if the block of ships has not been dragged to a valid new
	// location in the list, or if it's not possible to reorder the list.
	if(!panelState.CanEdit() || hoverIndex < 0 || hoverIndex == panelState.SelectedIndex())
		return true;

	panelState.ReorderShipsTo(hoverIndex);

	return true;
}



void PlayerInfoPanel::DrawPlayer(const Rectangle &bounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < 250.)
		return;

	// Colors to draw with.
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	const int columnWidth = 230;
	table.AddColumn(0, {columnWidth, Alignment::LEFT});
	table.AddColumn(columnWidth, {columnWidth, Alignment::RIGHT});
	table.SetUnderline(0, columnWidth);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	table.DrawTruncatedPair("player:", dim, player.FirstName() + " " + player.LastName(),
		bright, Truncate::MIDDLE, true);
	table.DrawTruncatedPair("net worth:", dim, Format::CreditString(player.Accounts().NetWorth()),
		bright, Truncate::MIDDLE, true);
	table.DrawTruncatedPair("time played:", dim, Format::PlayTime(player.GetPlayTime()),
		bright, Truncate::MIDDLE, true);

	// Determine the player's combat rating.
	int combatExperience = player.Conditions().Get("combat rating");
	int combatLevel = log(max<int64_t>(1, combatExperience));
	string combatRating = GameData::Rating("combat", combatLevel);
	if(!combatRating.empty())
	{
		table.DrawGap(10);
		table.DrawUnderline(dim);
		table.Draw("combat rating:", bright);
		table.Advance();
		table.DrawGap(5);

		table.DrawTruncatedPair("rank:", dim,
			to_string(combatLevel) + " - " + combatRating,
			dim, Truncate::MIDDLE, false);
		table.DrawTruncatedPair("experience:", dim,
			Format::Number(combatExperience), dim, Truncate::MIDDLE, false);
		bool maxRank = (combatRating == GameData::Rating("combat", combatLevel + 1));
		table.DrawTruncatedPair("    for next rank:", dim,
				maxRank ? "MAX" : Format::Number(ceil(exp(combatLevel + 1))),
				dim, Truncate::MIDDLE, false);
	}

	// Display the factors affecting piracy targeting the player.
	auto factors = player.RaidFleetFactors();
	double attractionLevel = max(0., log2(max(factors.first, 0.)));
	double deterrenceLevel = max(0., log2(max(factors.second, 0.)));
	string attractionRating = GameData::Rating("cargo attractiveness", attractionLevel);
	string deterrenceRating = GameData::Rating("armament deterrence", deterrenceLevel);
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
		table.DrawTruncatedPair("cargo: " + attractionRating, dim,
			"(+" + Format::Decimal(attractionLevel, 1) + ")", dim, Truncate::MIDDLE, false);
		table.DrawTruncatedPair("fleet: " + deterrenceRating, dim,
			"(-" + Format::Decimal(deterrenceLevel, 1) + ")", dim, Truncate::MIDDLE, false);
	}
	// Other special information:
	vector<pair<int64_t, string>> salary;
	for(const auto &it : player.Accounts().SalariesIncome())
		salary.emplace_back(it.second, it.first);
	sort(salary.begin(), salary.end(), std::greater<>());
	DrawList(salary, table, "salary:", player.Accounts().SalariesIncomeTotal(), 4);

	vector<pair<int64_t, string>> tribute;
	for(const auto &it : player.GetTribute())
		tribute.emplace_back(it.second, it.first->TrueName());
	sort(tribute.begin(), tribute.end(), std::greater<>());
	DrawList(tribute, table, "tribute:", player.GetTributeTotal(), 4);

	int maxRows = static_cast<int>(250. - 30. - table.GetPoint().Y()) / 20;
	vector<pair<int64_t, string>> licenses;
	for(const auto &it : player.Licenses())
		licenses.emplace_back(1, it);
	DrawList(licenses, table, "licenses:", licenses.size(), maxRows, false);
}



void PlayerInfoPanel::DrawFleet(const Rectangle &bounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < 750.)
		return;

	// Colors to draw with.
	const Color &back = *GameData::Colors().Get("faint");
	const Color &selectedBack = *GameData::Colors().Get("dimmer");
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &elsewhere = *GameData::Colors().Get("dim");
	const Color &dead = *GameData::Colors().Get("dead");
	const Color &flagship = *GameData::Colors().Get("flagship");
	const Color &disabled = *GameData::Colors().Get("disabled");

	// Table attributes.
	Table table;
	for(const auto &col : columns)
		table.AddColumn(col.offset, col.layout);

	table.SetUnderline(0, 730);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));
	table.DrawUnderline(dim);

	// Header row.
	const Point tablePoint = table.GetPoint();
	for(const auto &column : columns)
	{
		Rectangle zone = Rectangle(
			tablePoint + Point((column.offset + column.endX) / 2, table.GetRowSize().Y() / 2),
			Point(column.layout.width, table.GetRowSize().Y())
		);

		// Highlight the column header if it is under the mouse
		// or ships are sorted according to that column.
		const Color &columnHeaderColor = ((!isDragging && zone.Contains(hoverPoint))
			|| panelState.CurrentSort() == column.shipSort)
				? bright : dim;

		table.Draw(column.name, columnHeaderColor);

		menuZones.emplace_back(zone, column.shipSort);
	}

	table.DrawGap(5);

	// Loop through all the player's ships.
	int index = panelState.Scroll();
	hoverIndex = -1;
	for(auto sit = panelState.Ships().begin() + panelState.Scroll(); sit < panelState.Ships().end(); ++sit)
	{
		// Bail out if we've used out the whole drawing area.
		if(!bounds.Contains(table.GetRowBounds()))
			break;

		// Check if this row is selected.
		if(panelState.SelectedIndex() == index)
			table.DrawHighlight(selectedBack);
		else if(panelState.AllSelected().contains(index))
			table.DrawHighlight(back);

		// Find out if the mouse is hovering over the ship
		Rectangle shipZone = Rectangle(table.GetCenterPoint(), table.GetRowSize());
		bool isHovered = (hoverIndex == -1) && shipZone.Contains(hoverPoint);
		if(isHovered)
			hoverIndex = index;

		const Ship &ship = **sit;
		bool isElsewhere = (ship.GetSystem() != player.GetSystem());
		isElsewhere |= ((ship.CanBeCarried() || ship.GetPlanet() != player.GetPlanet()) && player.GetPlanet());
		bool isDead = ship.IsDestroyed();
		bool isDisabled = ship.IsDisabled();
		bool isFlagship = &ship == player.Flagship();

		table.SetColor(
			isDead ? dead
			: isHovered ? bright
			: isFlagship ? flagship
			: isDisabled ? disabled
			: isElsewhere ? elsewhere
			: dim
		);

		// Indent the ship name if it is a fighter or drone.
		table.Draw(ship.CanBeCarried() ? "    " + ship.GivenName() : ship.GivenName());
		table.Draw(ship.DisplayModelName());

		const System *system = ship.GetSystem();
		table.Draw(system ? (player.KnowsName(*system) ? system->DisplayName() : "???") : "");

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
		if(!isFlagship)
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
		for(int i : panelState.AllSelected())
		{
			const string &name = panelState.Ships()[i]->GivenName();
			font.Draw(name, pos + Point(1., 1.), Color(0., 1.));
			font.Draw(name, pos, bright);
			pos.Y() += 20.;
		}
	}
}



// Sorts the player's fleet given a comparator function (based on column).
void PlayerInfoPanel::SortShips(InfoPanelState::ShipComparator *shipComparator)
{
	if(panelState.Ships().empty())
		return;

	// Clicking on a sort column twice reverses the comparison.
	if(panelState.CurrentSort() == shipComparator)
		shipComparator = GetReverseCompareFrom(*shipComparator);

	// Save selected ships to preserve selection after sort.
	multiset<shared_ptr<Ship>, InfoPanelState::ShipComparator *> selectedShips(shipComparator);
	shared_ptr<Ship> lastSelected = panelState.SelectedIndex() == -1
		? nullptr
		: panelState.Ships()[panelState.SelectedIndex()];

	for(int i : panelState.AllSelected())
		selectedShips.insert(panelState.Ships()[i]);
	panelState.DeselectAll();

	// Move flagship to first position
	for(auto &ship : panelState.Ships())
		if(ship.get() == player.Flagship())
		{
			swap(ship, *panelState.Ships().begin());
			break;
		}

	stable_sort(
		panelState.Ships().begin() + 1,
		panelState.Ships().end(),
		shipComparator
	);

	// Ships are now sorted.
	panelState.SetCurrentSort(shipComparator);

	// Load the same selected ships from before the sort.
	if(selectedShips.empty())
		return;
	auto it = selectedShips.begin();
	for(size_t i = 0; i < panelState.Ships().size(); ++i)
		if(panelState.Ships()[i] == *it)
		{
			if(lastSelected == *it)
				panelState.SetSelectedIndex(i);
			else
				panelState.Select(i);

			++it;
			if(it == selectedShips.end())
				break;
		}
}



bool PlayerInfoPanel::Hover(int x, int y)
{
	return Hover(Point(x, y));
}



bool PlayerInfoPanel::Hover(const Point &point)
{
	hoverPoint = point;
	hoverIndex = -1;

	return true;
}



bool PlayerInfoPanel::Scroll(double /* dx */, double dy)
{
	return Scroll(dy * -.1 * Preferences::ScrollSpeed());
}



bool PlayerInfoPanel::ScrollAbsolute(int scroll)
{
	int maxScroll = panelState.Ships().size() - LINES_PER_PAGE;
	int newScroll = max(0, min<int>(maxScroll, scroll));
	if(panelState.Scroll() == newScroll)
		return false;

	panelState.SetScroll(newScroll);

	return true;
}



// Adjust the scroll by the given amount. Return true if it changed.
bool PlayerInfoPanel::Scroll(int distance)
{
	return ScrollAbsolute(panelState.Scroll() + distance);
}



PlayerInfoPanel::SortableColumn::SortableColumn(
	string name,
	double offset,
	double endX,
	Layout layout,
	InfoPanelState::ShipComparator *shipSort
)
	: name(name), offset(offset), endX(endX), layout(layout), shipSort(shipSort)
{
}
