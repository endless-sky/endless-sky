/* ShipInfoPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ShipInfoPanel.h"

#include "text/alignment.hpp"
#include "CategoryList.h"
#include "CategoryTypes.h"
#include "Command.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "HardpointInfoPanel.h"
#include "Information.h"
#include "Interface.h"
#include "LineShader.h"
#include "LogbookPanel.h"
#include "Messages.h"
#include "MissionPanel.h"
#include "OutlineShader.h"
#include "PlayerInfo.h"
#include "PlayerInfoPanel.h"
#include "Rectangle.h"
#include "Ship.h"
#include "ShipNameDialog.h"
#include "image/Sprite.h"
#include "SpriteShader.h"
#include "text/Table.h"
#include "text/truncate.hpp"
#include "UI.h"

#include <algorithm>

using namespace std;

namespace {
	constexpr double WIDTH = 250.;
	constexpr int COLUMN_WIDTH = static_cast<int>(WIDTH) - 20;
}

ShipInfoPanel::ShipInfoPanel(PlayerInfo &player)
	: ShipInfoPanel(player, InfoPanelState(player))
{
}

ShipInfoPanel::ShipInfoPanel(PlayerInfo &player, InfoPanelState state)
	: player(player), panelState(std::move(state))
{
	shipIt = this->panelState.Ships().begin();
	SetInterruptible(false);

	// If a valid ship index was given, show that ship.
	if(static_cast<unsigned>(panelState.SelectedIndex()) < player.Ships().size())
		shipIt += panelState.SelectedIndex();
	else if(player.Flagship())
	{
		// Find the player's flagship. It may not be first in the list, if the
		// first item in the list cannot be a flagship.
		while(shipIt != this->panelState.Ships().end() && shipIt->get() != player.Flagship())
			++shipIt;
	}

	UpdateInfo();
}



void ShipInfoPanel::Step()
{
	DoHelp("ship info");
}



void ShipInfoPanel::Draw()
{
	// Dim everything behind this panel.
	DrawBackdrop();

	// Fill in the information for how this interface should be drawn.
	info.ClearConditions();
	info.SetCondition("ship tab");
	if(panelState.CanEdit() && shipIt != panelState.Ships().end()
			&& (shipIt->get() != player.Flagship() || (*shipIt)->IsParked()))
	{
		if(!(*shipIt)->IsDisabled())
			info.SetCondition("can park");
		info.SetCondition((*shipIt)->IsParked() ? "show unpark" : "show park");
		info.SetCondition("show disown");
	}
	else if(!panelState.CanEdit())
	{
		info.SetCondition("show dump");
		if(CanDump())
			info.SetCondition("enable dump");
	}
	if(player.Ships().size() > 1)
		info.SetCondition("five buttons");
	else
		info.SetCondition("three buttons");
	if(player.HasLogs())
		info.SetCondition("enable logbook");

	// Draw the interface.
	const Interface *infoPanelUi = GameData::Interfaces().Get("info panel");
	infoPanelUi->Draw(info, this);
	# int infoPanelLine = 0; Removed by the 20241125conflictupdate not sure if it is upstream or Delta's.

	// Draw all the different information sections.
	ClearZones();
	if(shipIt == panelState.Ships().end())
		return;
	Rectangle cargoBounds = infoPanelUi->GetBox("cargo");
	// Draws "name: " and the ship name.
	info.DrawShipName(**shipIt, infoPanelUi->GetBox("stats"), infoPanelLine);
	// Draws "model: " and the ship model name.
	info.DrawShipModelStats(**shipIt, infoPanelUi->GetBox("stats"), infoPanelLine);
	infoPanelLine++; // This makes a one-text-line gap in the display of text.
	// Displays the shield and hull.
	info.DrawShipHealthStats(**shipIt, infoPanelUi->GetBox("stats"), infoPanelLine);
	infoPanelLine++; // This makes a one-text-line gap in the display of text.
	// Displays mass, cargo, bunks, and fuel
	info.DrawShipCarryingCapacities(**shipIt, infoPanelUi->GetBox("stats"), infoPanelLine);
	infoPanelLine++; // This makes a one-text-line gap in the display of text.
	// Displays "outfit space free: " and outfit space
	info.DrawShipOutfitStat(**shipIt, infoPanelUi->GetBox("stats"), infoPanelLine);
	infoPanelLine++; // This makes a one-text-line gap in the display of text.
	// Displays max speed, thrust, reverse, lateral, and turn
	info.DrawShipManeuverStats(**shipIt, infoPanelUi->GetBox("stats"), infoPanelLine);
	// infoPanelLine++; // This makes a one-text-line gap in the display of text.
	info.DrawShipEnergyHeatStats(**shipIt, infoPanelUi->GetBox("stats"), infoPanelLine);
	// DrawShipStats(infoPanelUi->GetBox("stats")); // This is the old method that drew all the stats
	DrawOutfits(infoPanelUi->GetBox("outfits"), cargoBounds);
	DrawSprite(infoPanelUi->GetBox("ship info sprite"));
	DrawThumbnail(infoPanelUi->GetBox("ship info thumbnail"));
	DrawCargo(cargoBounds);

	// If the player hovers their mouse over a ship attribute, show its tooltip.
	infoDisplay.DrawTooltips();
}



bool ShipInfoPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool /* isNewPress */)
{
	bool control = (mod & (KMOD_CTRL | KMOD_GUI));
	bool shift = (mod & KMOD_SHIFT);
	if(key == 'd' || key == SDLK_ESCAPE || (key == 'w' && control))
		GetUI()->Pop(this);
	else if(command.Has(Command::HELP))
		DoHelp("ship info", true);
	else if(!player.Ships().empty() && ((key == 'p' && !shift) || key == SDLK_LEFT || key == SDLK_UP))
	{
		if(shipIt == panelState.Ships().begin())
			shipIt = panelState.Ships().end();
		--shipIt;
		UpdateInfo();
	}
	else if(!panelState.Ships().empty() && (key == 'n' || key == SDLK_RIGHT || key == SDLK_DOWN))
	{
		++shipIt;
		if(shipIt == panelState.Ships().end())
			shipIt = panelState.Ships().begin();
		UpdateInfo();
	}
	else if(key == 'i' || command.Has(Command::INFO) || (control && key == SDLK_TAB))
	{
		GetUI()->Pop(this);
		GetUI()->Push(new PlayerInfoPanel(player, std::move(panelState)));
	}
	else if(key == 'h')
	{
		if(!player.Ships().empty())
		{
			GetUI()->Pop(this);
			GetUI()->Push(new HardpointInfoPanel(player, std::move(panelState)));
		}
	}
	else if(key == 'R' || (key == 'r' && shift))
		GetUI()->Push(new ShipNameDialog(this, &ShipInfoPanel::Rename, "Change this ship's name?", (*shipIt)->Name()));
	else if(panelState.CanEdit() && (key == 'P' || (key == 'p' && shift) || key == 'k'))
	{
		if(shipIt->get() != player.Flagship() || (*shipIt)->IsParked())
			player.ParkShip(shipIt->get(), !(*shipIt)->IsParked());
	}
	else if(panelState.CanEdit() && key == 'D')
	{
		if(shipIt->get() != player.Flagship())
		{
			map<const Outfit*, int> uniqueOutfits;
			auto AddToUniques = [&uniqueOutfits] (const std::map<const Outfit *, int> &outfits)
			{
				for(const auto &it : outfits)
					if(it.first->Attributes().Get("unique"))
						uniqueOutfits[it.first] += it.second;
			};
			AddToUniques(shipIt->get()->Outfits());
			AddToUniques(shipIt->get()->Cargo().Outfits());

			string message = "Are you sure you want to disown \""
				+ shipIt->get()->Name()
				+ "\"? Disowning a ship rather than selling it means you will not get any money for it.";
			if(!uniqueOutfits.empty())
			{
				const int uniquesSize = uniqueOutfits.size();
				const int detailedOutfitSize = (uniquesSize > 20 ? 19 : uniquesSize);
				message += "\nThe following unique items carried by the ship will be lost:";
				auto it = uniqueOutfits.begin();
				for(int i = 0; i < detailedOutfitSize; ++i)
				{
					message += "\n" + to_string(it->second) + " "
						+ (it->second == 1 ? it->first->DisplayName() : it->first->PluralName());
					++it;
				}
				if(it != uniqueOutfits.end())
				{
					int otherUniquesCount = 0;
					for( ; it != uniqueOutfits.end(); ++it)
						otherUniquesCount += it->second;
					message += "\nand " + to_string(otherUniquesCount) + " other unique outfits";
				}
			}

			GetUI()->Push(new Dialog(this, &ShipInfoPanel::Disown, message));
		}
	}
	else if(key == 'c' && CanDump())
	{
		int commodities = (*shipIt)->Cargo().CommoditiesSize();
		int amount = (*shipIt)->Cargo().Get(selectedCommodity);
		int plunderAmount = (*shipIt)->Cargo().Get(selectedPlunder);
		if(amount)
		{
			GetUI()->Push(new Dialog(this, &ShipInfoPanel::DumpCommodities,
				"How many tons of " + Format::LowerCase(selectedCommodity)
					+ " do you want to jettison?", amount));
		}
		else if(plunderAmount > 0 && selectedPlunder->Get("installable") < 0.)
		{
			GetUI()->Push(new Dialog(this, &ShipInfoPanel::DumpPlunder,
				"How many tons of " + Format::LowerCase(selectedPlunder->DisplayName())
					+ " do you want to jettison?", plunderAmount));
		}
		else if(plunderAmount == 1)
		{
			GetUI()->Push(new Dialog(this, &ShipInfoPanel::Dump,
				"Are you sure you want to jettison a " + selectedPlunder->DisplayName() + "?"));
		}
		else if(plunderAmount > 1)
		{
			GetUI()->Push(new Dialog(this, &ShipInfoPanel::DumpPlunder,
				"How many " + selectedPlunder->PluralName() + " do you want to jettison?",
				plunderAmount));
		}
		else if(commodities)
		{
			GetUI()->Push(new Dialog(this, &ShipInfoPanel::Dump,
				"Are you sure you want to jettison all of this ship's regular cargo?"));
		}
		else
		{
			GetUI()->Push(new Dialog(this, &ShipInfoPanel::Dump,
				"Are you sure you want to jettison all of this ship's cargo?"));
		}
	}
	else if(command.Has(Command::MAP) || key == 'm')
		GetUI()->Push(new MissionPanel(player));
	else if(key == 'l' && player.HasLogs())
		GetUI()->Push(new LogbookPanel(player));
	else
		return false;

	return true;
}



bool ShipInfoPanel::Click(int x, int y, int /* clicks */)
{
	if(shipIt == panelState.Ships().end())
		return true;

	draggingIndex = -1;
	if(panelState.CanEdit() && hoverIndex >= 0 && (**shipIt).GetSystem() == player.GetSystem() && !(**shipIt).IsDisabled())
		draggingIndex = hoverIndex;

	selectedCommodity.clear();
	selectedPlunder = nullptr;
	Point point(x, y);
	for(const auto &zone : commodityZones)
		if(zone.Contains(point))
			selectedCommodity = zone.Value();
	for(const auto &zone : plunderZones)
		if(zone.Contains(point))
			selectedPlunder = zone.Value();

	return true;
}



bool ShipInfoPanel::Hover(int x, int y)
{
	Point point(x, y);
	infoDisplay.Hover(point);
	return Hover(point);
}



bool ShipInfoPanel::Drag(double dx, double dy)
{
	return Hover(hoverPoint + Point(dx, dy));
}



bool ShipInfoPanel::Release(int /* x */, int /* y */)
{
	if(draggingIndex >= 0 && hoverIndex >= 0 && hoverIndex != draggingIndex)
		(**shipIt).GetArmament().Swap(hoverIndex, draggingIndex);

	draggingIndex = -1;
	return true;
}



void ShipInfoPanel::UpdateInfo()
{
	draggingIndex = -1;
	hoverIndex = -1;
	ClearZones();
	if(shipIt == panelState.Ships().end())
		return;

	const Ship &ship = **shipIt;
	infoDisplay.Update(ship, player);
	if(player.Flagship() && ship.GetSystem() == player.GetSystem() && &ship != player.Flagship())
		player.Flagship()->SetTargetShip(*shipIt);

	outfits.clear();
	for(const auto &it : ship.Outfits())
		outfits[it.first->Category()].push_back(it.first);

	panelState.SelectOnly(shipIt - panelState.Ships().begin());
}



void ShipInfoPanel::ClearZones()
{
	zones.clear();
	commodityZones.clear();
	plunderZones.clear();
}



void ShipInfoPanel::DrawShipStats(const Rectangle &bounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship &ship = **shipIt;

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, {COLUMN_WIDTH, Alignment::LEFT});
	table.AddColumn(COLUMN_WIDTH, {COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE});
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	table.DrawTruncatedPair("ship:", dim, ship.Name(), bright, Truncate::MIDDLE, true);

	infoDisplay.DrawAttributes(table.GetRowBounds().TopLeft() - Point(10., 10.));
}



void ShipInfoPanel::DrawOutfits(const Rectangle &bounds, Rectangle &cargoBounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship &ship = **shipIt;

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, {COLUMN_WIDTH, Alignment::LEFT});
	table.AddColumn(COLUMN_WIDTH, {COLUMN_WIDTH, Alignment::RIGHT});
	table.SetUnderline(0, COLUMN_WIDTH);
	Point start = bounds.TopLeft() + Point(10., 8.);
	table.DrawAt(start);

	// Draw the outfits in the same order used in the outfitter.
	for(const auto &cat : GameData::GetCategory(CategoryType::OUTFIT))
	{
		const string &category = cat.Name();
		auto it = outfits.find(category);
		if(it == outfits.end())
			continue;

		// Skip to the next column if there is no space for this category label
		// plus at least one outfit.
		if(table.GetRowBounds().Bottom() + 40. > bounds.Bottom())
		{
			start += Point(WIDTH, 0.);
			if(start.X() + COLUMN_WIDTH > bounds.Right())
				break;
			table.DrawAt(start);
		}

		// Draw the category label.
		table.Draw(category, bright);
		table.Advance();
		for(const Outfit *outfit : it->second)
		{
			// Check if we've gone below the bottom of the bounds.
			if(table.GetRowBounds().Bottom() > bounds.Bottom())
			{
				start += Point(WIDTH, 0.);
				if(start.X() + COLUMN_WIDTH > bounds.Right())
					break;
				table.DrawAt(start);
				table.Draw(category, bright);
				table.Advance();
			}

			// Draw the outfit name and count.
			table.DrawTruncatedPair(outfit->DisplayName(), dim,
				to_string(ship.OutfitCount(outfit)), bright, Truncate::BACK, false);
		}
		// Add an extra gap in between categories.
		table.DrawGap(10.);
	}

	// Check if this information spilled over into the cargo column.
	if(table.GetPoint().X() >= cargoBounds.Left())
	{
		double startY = table.GetRowBounds().Top() - 8.;
		cargoBounds = Rectangle::WithCorners(
			Point(cargoBounds.Left(), startY),
			Point(cargoBounds.Right(), max(startY, cargoBounds.Bottom())));
	}
}



void ShipInfoPanel::DrawSprite(const Rectangle &bounds)
{
	const Ship &ship = **shipIt;

	// Figure out how much to scale the sprite by.
	const Sprite *sprite = ship.GetSprite();
	double scale = 0.;
	if(sprite)
		scale = min(1., min((WIDTH - 10) / sprite->Width(), (WIDTH - 10) / sprite->Height()));

	// Figure out the left- and right-most hardpoints on the ship. If they are
	// too far apart, the scale may need to be reduced.
	// Also figure out how many weapons of each type are on each side.
	double maxX = 0.;
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		// Multiply hardpoint X by 2 to convert to sprite pixels.
		maxX = max(maxX, fabs(2. * hardpoint.GetPoint().X()));
	}
	// If necessary, shrink the sprite to keep the hardpoints inside the labels.
	static const double LABEL_DX = 95.;
	static const double LABEL_PAD = 5.;
	if(maxX > (LABEL_DX - LABEL_PAD))
		scale = min(scale, (LABEL_DX - LABEL_PAD) / (2. * maxX));

	// Draw the ship, using the default swizzle 0
	if(sprite)
	{
		SpriteShader::Draw(sprite, bounds.Center(), scale, ship.GetSwizzle());
	}
}



void ShipInfoPanel::DrawThumbnail(const Rectangle & bounds)
{
	const Ship & ship = **shipIt;

	// Figure out how much to scale the sprite by.
	const Sprite * sprite = ship.Thumbnail();
	double scale = 0.;
	if(sprite)
		scale = min(1., min((WIDTH - 10) / sprite->Width(), (WIDTH - 10) / sprite->Height()));

	// Figure out the left- and right-most hardpoints on the ship. If they are
	// too far apart, the scale may need to be reduced.
	// Also figure out how many weapons of each type are on each side.
	double maxX = 0.;
	for(const Hardpoint & hardpoint : ship.Weapons())
	{
		// Multiply hardpoint X by 2 to convert to sprite pixels.
		maxX = max(maxX, fabs(2. * hardpoint.GetPoint().X()));
	}
	// If necessary, shrink the sprite to keep the hardpoints inside the labels.
	static const double LABEL_DX = 95.;
	static const double LABEL_PAD = 5.;
	if(maxX > (LABEL_DX - LABEL_PAD))
		scale = min(scale, (LABEL_DX - LABEL_PAD) / (2. * maxX));

	// Draw the ship, using the default swizzle 0
	if(sprite)
	{
		SpriteShader::Draw(sprite, bounds.Center(), scale, ship.GetSwizzle());
	}
}



void ShipInfoPanel::DrawCargo(const Rectangle &bounds)
{
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	Color backColor = *GameData::Colors().Get("faint");
	const Ship &ship = **shipIt;

	// Cargo list.
	const CargoHold &cargo = (player.Cargo().Used() ? player.Cargo() : ship.Cargo());
	Table table;
	table.AddColumn(0, {COLUMN_WIDTH, Alignment::LEFT});
	table.AddColumn(COLUMN_WIDTH, {COLUMN_WIDTH, Alignment::RIGHT});
	table.SetUnderline(-5, COLUMN_WIDTH + 5);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	double endY = bounds.Bottom() - 30. * (cargo.Passengers() != 0);
	bool hasSpace = (table.GetRowBounds().Bottom() < endY);
	if((cargo.CommoditiesSize() || cargo.HasOutfits() || cargo.MissionCargoSize()) && hasSpace)
	{
		table.Draw("Cargo", bright);
		table.Advance();
		hasSpace = (table.GetRowBounds().Bottom() < endY);
	}
	if(cargo.CommoditiesSize() && hasSpace)
	{
		for(const auto &it : cargo.Commodities())
		{
			if(!it.second)
				continue;

			commodityZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), it.first);
			if(it.first == selectedCommodity)
				table.DrawHighlight(backColor);

			table.Draw(it.first, dim);
			table.Draw(to_string(it.second), bright);

			// Truncate the list if there is not enough space.
			if(table.GetRowBounds().Bottom() >= endY)
			{
				hasSpace = false;
				break;
			}
		}
		table.DrawGap(10.);
	}
	if(cargo.HasOutfits() && hasSpace)
	{
		for(const auto &it : cargo.Outfits())
		{
			if(!it.second)
				continue;

			plunderZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), it.first);
			if(it.first == selectedPlunder)
				table.DrawHighlight(backColor);

			// For outfits, show how many of them you have and their total mass.
			bool isSingular = (it.second == 1 || it.first->Get("installable") < 0.);
			string name = (isSingular ? it.first->DisplayName() : it.first->PluralName());
			if(!isSingular)
				name += " (" + to_string(it.second) + "x)";
			table.Draw(name, dim);

			double mass = it.first->Mass() * it.second;
			table.Draw(Format::Number(mass), bright);

			// Truncate the list if there is not enough space.
			if(table.GetRowBounds().Bottom() >= endY)
			{
				hasSpace = false;
				break;
			}
		}
		table.DrawGap(10.);
	}
	if(cargo.HasMissionCargo() && hasSpace)
	{
		for(const auto &it : cargo.MissionCargo())
		{
			// Capitalize the name of the cargo.
			table.Draw(Format::Capitalize(it.first->Cargo()), dim);
			table.Draw(to_string(it.second), bright);

			// Truncate the list if there is not enough space.
			if(table.GetRowBounds().Bottom() >= endY)
				break;
		}
		table.DrawGap(10.);
	}
	if(cargo.Passengers() && endY >= bounds.Top())
	{
		table.DrawAt(Point(bounds.Left(), endY) + Point(10., 8.));
		table.Draw("passengers:", dim);
		table.Draw(to_string(cargo.Passengers()), bright);
	}
}



void ShipInfoPanel::DrawLine(const Point &from, const Point &to, const Color &color) const
{
	Color black(0.f, 1.f);
	Point mid(to.X(), from.Y());

	LineShader::Draw(from, mid, 2.f, black);
	LineShader::Draw(mid, to, 2.f, black);
	LineShader::Draw(from, mid, 1.f, color);
	LineShader::Draw(mid, to, 1.f, color);
}



bool ShipInfoPanel::Hover(const Point &point)
{
	if(shipIt == panelState.Ships().end())
		return true;

	hoverPoint = point;

	hoverIndex = -1;
	const vector<Hardpoint> &weapons = (**shipIt).Weapons();
	bool dragIsTurret = (draggingIndex >= 0 && weapons[draggingIndex].IsTurret());
	bool dragIsPylon = (draggingIndex >= 0 && weapons[draggingIndex].IsPylon());
	for(const auto &zone : zones)
	{
		bool isTurret = weapons[zone.Value()].IsTurret();
		bool isPylon = weapons[zone.Value()].IsPylon();
		if(zone.Contains(hoverPoint) && (draggingIndex == -1 || isTurret == dragIsTurret || isPylon == dragIsPylon))
			hoverIndex = zone.Value();
	}

	return true;
}



void ShipInfoPanel::Rename(const string &name)
{
	if(shipIt != panelState.Ships().end() && !name.empty())
	{
		player.RenameShip(shipIt->get(), name);
		UpdateInfo();
	}
}



bool ShipInfoPanel::CanDump() const
{
	if(panelState.CanEdit() || shipIt == panelState.Ships().end())
		return false;

	CargoHold &cargo = (*shipIt)->Cargo();
	return (selectedPlunder && cargo.Get(selectedPlunder) > 0) || cargo.CommoditiesSize() || cargo.OutfitsSize();
}



void ShipInfoPanel::Dump()
{
	if(!CanDump())
		return;

	CargoHold &cargo = (*shipIt)->Cargo();
	int commodities = (*shipIt)->Cargo().CommoditiesSize();
	int amount = cargo.Get(selectedCommodity);
	int plunderAmount = cargo.Get(selectedPlunder);
	int64_t loss = 0;
	if(amount)
	{
		int64_t basis = player.GetBasis(selectedCommodity, amount);
		loss += basis;
		player.AdjustBasis(selectedCommodity, -basis);
		(*shipIt)->Jettison(selectedCommodity, amount);
	}
	else if(plunderAmount > 0)
	{
		loss += plunderAmount * selectedPlunder->Cost();
		(*shipIt)->Jettison(selectedPlunder, plunderAmount);
	}
	else if(commodities)
	{
		for(const auto &it : cargo.Commodities())
		{
			int64_t basis = player.GetBasis(it.first, it.second);
			loss += basis;
			player.AdjustBasis(it.first, -basis);
			(*shipIt)->Jettison(it.first, it.second);
		}
	}
	else
	{
		for(const auto &it : cargo.Outfits())
		{
			loss += it.first->Cost() * max(0, it.second);
			(*shipIt)->Jettison(it.first, it.second);
		}
	}
	selectedCommodity.clear();
	selectedPlunder = nullptr;

	infoDisplay.Update(**shipIt, player);
	if(loss)
		Messages::Add("You jettisoned " + Format::CreditString(loss) + " worth of cargo."
			, Messages::Importance::High);
}



void ShipInfoPanel::DumpPlunder(int count)
{
	int64_t loss = 0;
	count = min(count, (*shipIt)->Cargo().Get(selectedPlunder));
	if(count > 0)
	{
		loss += count * selectedPlunder->Cost();
		(*shipIt)->Jettison(selectedPlunder, count);
		infoDisplay.Update(**shipIt, player);

		if(loss)
			Messages::Add("You jettisoned " + Format::CreditString(loss) + " worth of cargo."
				, Messages::Importance::High);
	}
}



void ShipInfoPanel::DumpCommodities(int count)
{
	int64_t loss = 0;
	count = min(count, (*shipIt)->Cargo().Get(selectedCommodity));
	if(count > 0)
	{
		int64_t basis = player.GetBasis(selectedCommodity, count);
		loss += basis;
		player.AdjustBasis(selectedCommodity, -basis);
		(*shipIt)->Jettison(selectedCommodity, count);
		infoDisplay.Update(**shipIt, player);

		if(loss)
			Messages::Add("You jettisoned " + Format::CreditString(loss) + " worth of cargo."
				, Messages::Importance::High);
	}
}



void ShipInfoPanel::Disown()
{
	// Make sure a ship really is selected.
	if(shipIt == panelState.Ships().end() || shipIt->get() == player.Flagship())
		return;

	const auto ship = shipIt;
	if(shipIt != panelState.Ships().begin())
		--shipIt;

	player.DisownShip(ship->get());
	panelState.Disown(ship);
	UpdateInfo();
}
