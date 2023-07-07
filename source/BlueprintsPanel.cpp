/* BlueprintsPanel.cpp
Copyright (c) 2017 Michael Zahniser
Copyright (c) 2023 by Dave Flowers

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "BlueprintsPanel.h"

#include "text/alignment.hpp"
#include "Command.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "InfoPanelState.h"
#include "Information.h"
#include "Interface.h"
#include "text/layout.hpp"
#include "LogbookPanel.h"
#include "MissionPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "PlayerInfoPanel.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "Ship.h"
#include "ShipInfoPanel.h"
#include "System.h"
#include "text/Table.h"
#include "text/truncate.hpp"
#include "UI.h"

#include <algorithm>
#include <cmath>
#include <utility>

using namespace std;

namespace {
	// Number of lines per page of the fleet listing.
	const int LINES_PER_PAGE = 26;

	bool CompareName(const shared_ptr<Ship> &lhs, const shared_ptr<Ship> &rhs)
	{
		return lhs->Name() < rhs->Name();
	}

	bool CompareModelName(const shared_ptr<Ship> &lhs, const shared_ptr<Ship> &rhs)
	{
		return lhs->ModelName() < rhs->ModelName();
	}

	bool CompareSystem(const shared_ptr<Ship> &lhs, const shared_ptr<Ship> &rhs)
	{
		// Ships (drones) with no system are sorted to the end.
		if(lhs->GetSystem() == nullptr)
			return false;
		else if(rhs->GetSystem() == nullptr)
			return true;
		return lhs->GetSystem()->Name() < rhs->GetSystem()->Name();
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
	template <InfoPanelState::ShipComparator &F>
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
const BlueprintsPanel::SortableColumn BlueprintsPanel::columns[7] = {
	SortableColumn("ship", 0, 217, {217, Truncate::MIDDLE}, CompareName),
	SortableColumn("model", 220, 347, {127, Truncate::BACK}, CompareModelName),
	SortableColumn("system", 350, 487, {137, Truncate::BACK}, CompareSystem),
	SortableColumn("shields", 550, 493, {57, Alignment::RIGHT, Truncate::BACK}, CompareShields),
	SortableColumn("hull", 610, 553, {57, Alignment::RIGHT, Truncate::BACK}, CompareHull),
	SortableColumn("fuel", 670, 613, {57, Alignment::RIGHT, Truncate::BACK}, CompareFuel),
	SortableColumn("crew", 730, 673, {57, Alignment::RIGHT, Truncate::BACK}, CompareRequiredCrew)
};



BlueprintsPanel::BlueprintsPanel(PlayerInfo &player)
	: BlueprintsPanel(player, InfoPanelState(player))
{
}

BlueprintsPanel::BlueprintsPanel(PlayerInfo &player, InfoPanelState panelState)
	: player(player), panelState(panelState)
{
	SetInterruptible(false);
}



void BlueprintsPanel::Draw()
{
	// Dim everything behind this panel.
	DrawBackdrop();

	// Fill in the information for how this interface should be drawn.
	Information interfaceInfo;
	interfaceInfo.SetCondition("blueprints tab");
	if(panelState.CanEdit() && !panelState.Ships().empty())
	{
		// If ship order has changed by choosing a sort comparison,
		// show the save order button. Any manual sort by the player
		// is applied immediately and doesn't need this button.
		if(panelState.CanEdit() && panelState.CurrentSort())
			interfaceInfo.SetCondition("show save order");
	}

	interfaceInfo.SetCondition("seven buttons");
	if(player.HasLogs())
		interfaceInfo.SetCondition("enable logbook");

	// Draw the interface.
	const Interface *infoPanelUi = GameData::Interfaces().Get("info panel");
	infoPanelUi->Draw(interfaceInfo, this);

	// Draw the fleet info section.
	menuZones.clear();

	DrawFleet(infoPanelUi->GetBox("fleet"));
}



bool BlueprintsPanel::AllowsFastForward() const noexcept
{
	return true;
}



bool BlueprintsPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	bool control = (mod & (KMOD_CTRL | KMOD_GUI));
	bool shift = (mod & KMOD_SHIFT);
	if(key == 'd' || key == SDLK_ESCAPE || (key == 'w' && control))
	{
		GetUI()->Pop(this);
	}
	else if(key == 'i' || command.Has(Command::INFO))
	{
		GetUI()->Pop(this);
		GetUI()->Push(new PlayerInfoPanel(player, std::move(panelState)));
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
				if(panelState.AllSelected().count(selectedIndex))
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
			player.SetGroup(group, &selected);
		}
		else
		{
			// Convert ship pointers into indices in the ship list.
			set<int> added;
			for(Ship *ship : player.GetGroup(group))
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



bool BlueprintsPanel::Click(int x, int y, int clicks)
{
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
		if(control && panelState.AllSelected().count(hoverIndex))
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
			else if(panelState.AllSelected().count(hoverIndex))
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
		// If not landed, clicking a ship name takes you straight to its info.
		panelState.SetSelectedIndex(hoverIndex);

		GetUI()->Pop(this);
		GetUI()->Push(new ShipInfoPanel(player, std::move(panelState)));
	}

	return true;
}



bool BlueprintsPanel::Drag(double dx, double dy)
{
	isDragging = true;
	return Hover(hoverPoint + Point(dx, dy));
}



bool BlueprintsPanel::Release(int /* x */, int /* y */)
{
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



void BlueprintsPanel::DrawFleet(const Rectangle &bounds)
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
		else if(panelState.AllSelected().count(index))
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
			const string &name = panelState.Ships()[i]->Name();
			font.Draw(name, pos + Point(1., 1.), Color(0., 1.));
			font.Draw(name, pos, bright);
			pos.Y() += 20.;
		}
	}
}



// Sorts the player's fleet given a comparator function (based on column).
void BlueprintsPanel::SortShips(InfoPanelState::ShipComparator *shipComparator)
{
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

	// Load the same selected ships from before the sort.
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

	// Ships are now sorted.
	panelState.SetCurrentSort(shipComparator);
}



bool BlueprintsPanel::Hover(int x, int y)
{
	return Hover(Point(x, y));
}



bool BlueprintsPanel::Hover(const Point &point)
{
	hoverPoint = point;
	hoverIndex = -1;

	return true;
}



bool BlueprintsPanel::Scroll(double /* dx */, double dy)
{
	return Scroll(dy * -.1 * Preferences::ScrollSpeed());
}



bool BlueprintsPanel::ScrollAbsolute(int scroll)
{
	int maxScroll = panelState.Ships().size() - LINES_PER_PAGE;
	int newScroll = max(0, min<int>(maxScroll, scroll));
	if(panelState.Scroll() == newScroll)
		return false;

	panelState.SetScroll(newScroll);

	return true;
}



// Adjust the scroll by the given amount. Return true if it changed.
bool BlueprintsPanel::Scroll(int distance)
{
	return ScrollAbsolute(panelState.Scroll() + distance);
}



BlueprintsPanel::SortableColumn::SortableColumn(
	string name,
	double offset,
	double endX,
	Layout layout,
	InfoPanelState::ShipComparator *shipSort
)
: name(name), offset(offset), endX(endX), layout(layout), shipSort(shipSort)
{
}
