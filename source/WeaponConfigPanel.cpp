/*WeaponConfigPanel.cpp
Copyright (c) 2022 by warpcore

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "WeaponConfigPanel.h"

#include "Command.h"
#include "GameData.h"
#include "InfoPanelState.h"
#include "Information.h"
#include "LogbookPanel.h"
#include "Interface.h"
#include "LineShader.h"
#include "MissionPanel.h"
#include "OutlineShader.h"
#include "PlayerInfo.h"
#include "PlayerInfoPanel.h"
#include "Ship.h"
#include "ShipInfoPanel.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "UI.h"
#include "text/alignment.hpp"
#include "text/DisplayText.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "text/Table.h"

#include <algorithm>
#include <map>
#include <queue>
#include <unordered_map>
#include <vector>

using namespace std;

const WeaponConfigPanel::Column WeaponConfigPanel::columns[6] = {
	Column(250, {50, Alignment::RIGHT, Truncate::BACK}), // ange
	Column(260, {50, Alignment::LEFT, Truncate::BACK}), // ammo?
	Column(310, {80, Alignment::LEFT, Truncate::BACK}), // defensive?
	Column(390, {60, Alignment::LEFT, Truncate::BACK}), // autofire mode
	Column(550, {100, Alignment::RIGHT, Truncate::BACK}), // turn rate
	Column(560, {100, Alignment::LEFT, Truncate::BACK}) // opportunistic?
};



WeaponConfigPanel::WeaponConfigPanel(PlayerInfo &player)
	: WeaponConfigPanel(player, InfoPanelState(player))
{
}

WeaponConfigPanel::WeaponConfigPanel(PlayerInfo &player, InfoPanelState panelState)
	: player(player), panelState(panelState)
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
		while(shipIt != player.Ships().end() && shipIt->get() != player.Flagship())
			++shipIt;
	}

	UpdateInfo();
}



void WeaponConfigPanel::Step()
{
	DoHelp("weapon config");
}



void WeaponConfigPanel::Draw()
{
	// Dim everything behind this panel.
	DrawBackdrop();

	// Fill in the information for how this interface should be drawn.
	Information interfaceInfo;
	interfaceInfo.SetCondition("weapon config panel");

	if(player.Ships().size() > 1)
		interfaceInfo.SetCondition("five buttons");
	else
		interfaceInfo.SetCondition("three buttons");
	interfaceInfo.SetCondition("show turn rate bar");
	interfaceInfo.SetBar("turnratethreshold", 0.5);



	// Draw the interface.
	const Interface *weaponConfigPanelUi = GameData::Interfaces().Get("info panel");
	weaponConfigPanelUi->Draw(interfaceInfo, this);

	// Draw all the different information sections.
	ClearZones();
	if(shipIt == player.Ships().end())
		return;

	Rectangle silhouetteBounds = weaponConfigPanelUi->GetBox("silhouette");
	Rectangle weaponsBounds = weaponConfigPanelUi->GetBox("weaponsList");
	//Rectangle tableBounds = weaponConfigPanelUi->GetBox("weaponsConfigTable");

	// Constants for arranging stuff.
	static const double WIDTH = silhouetteBounds.Width();
	static const double LINE_HEIGHT = 20.;
	static const double GUN_TURRET_GAP = 10.;
	static const double LABEL_PAD = 5;
	static const double LABEL_WIDTH = weaponsBounds.Width() - 20.;
	static const double HEADER_PAD = 5.;

	Column weaponColumn(static_cast<int>(LABEL_PAD), {static_cast<int>(weaponsBounds.Width() - LABEL_PAD - 50), Alignment::LEFT, Truncate::BACK}); // name

	// Colors to draw with.
	Color dimmer = *GameData::Colors().Get("dimmer");
	Color dim = *GameData::Colors().Get("dim");
	Color medium = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Font &font = FontSet::Get(14);
	const Ship &ship = **shipIt;

	// Figure out how much to scale the sprite by.
	const Sprite *sprite = ship.GetSprite();
	double scale = 0.;
	if(sprite)
		scale = min(1., min((WIDTH - 10) / sprite->Width(), (WIDTH - 10) / sprite->Height()));
	// Draw the ship, using the black silhouette swizzle.
	SpriteShader::Draw(sprite, silhouetteBounds.Center(), scale, 28);
	OutlineShader::Draw(sprite, silhouetteBounds.Center(), scale * Point(sprite->Width(), sprite->Height()), Color(.5f));

	// Figure out how many weapons of each type there are.
	int count[2] = {0, 0};
	for (const Hardpoint &hardpoint : ship.Weapons())
	{
		++count[hardpoint.IsTurret()];
	}

	// Figure out how tall each part of the weapon listing will be.
	int gunRows = count[0];
	int turretRows = count[1];
	// If there are both guns and turrets, add a gap of GUN_TURRET_GAP pixels.
	double height = LINE_HEIGHT * (gunRows + turretRows) + GUN_TURRET_GAP * (gunRows && turretRows);

	SetControlColumnZones(height, weaponsBounds.Left());

	double gunY = weaponsBounds.Top() + .5 * (weaponsBounds.Height() - height);
	double turretY = gunY + LINE_HEIGHT * gunRows + GUN_TURRET_GAP * (gunRows != 0);

	// Table attributes.
	Table table;
	Table turretTable;
	table.AddColumn(weaponColumn.start, weaponColumn.layout);
	turretTable.AddColumn(weaponColumn.start, weaponColumn.layout);
	for(Column column : columns)
	{
		table.AddColumn(column.start, column.layout);
		turretTable.AddColumn(column.start, column.layout);
	}

	table.SetUnderline(0, 750.);
	turretTable.SetHighlight(0, 750.);

	table.DrawAt(Point(weaponsBounds.Left(), gunY - LINE_HEIGHT - HEADER_PAD));
	turretTable.DrawAt(Point(weaponsBounds.Left(), turretY));

	// Header row.
	table.DrawUnderline(medium);
	table.SetColor(bright);
	table.Draw("name");
	table.Draw("range");
	table.Draw("ammo?");
	table.Draw("offensive");
	//table.Draw("ammo use");
	table.Draw("autofire");
	table.Draw("turn speed");
	table.Draw("targeting mode");
	table.DrawGap(HEADER_PAD);

	int index = 0;

	static const Point LINE_SIZE(LABEL_WIDTH, LINE_HEIGHT);
	Point topFrom;
	Point topTo;
	Color topColor;
	bool hasTop = false;
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		string name = "[empty]";
		if(hardpoint.GetOutfit())
			name = hardpoint.GetOutfit()->Name();

		bool isTurret = hardpoint.IsTurret();

		bool isHover = (index == hoverIndex);
		Color textColor = isHover ? bright : medium;

		Point zoneCenter = (isTurret ? turretTable : table).GetCenterPoint();
		zones.emplace_back(zoneCenter, table.GetRowSize(), index);

		// Determine what color to use for the line.
		float high = (isHover ? .8f : .5f);
		Color color(high, .75f * high, 0.f, 1.f);
		if(isTurret)
		{
			color = Color(0.f, .75f * high, high, 1.f);
			if(isHover)
			{
				turretTable.DrawHighlight(dimmer);
			}
			turretTable.Draw(name, textColor);
			if(!hardpoint.GetOutfit())
			{
				turretTable.Advance(6);
			}
			else if(hardpoint.GetOutfit()->AntiMissile())
			{
				turretTable.Draw(hardpoint.GetOutfit()->Range(), textColor);
				turretTable.Advance(5);
			}
			else
			{
				turretTable.Draw(hardpoint.GetOutfit()->Range(), textColor);
				turretTable.Draw(hardpoint.GetOutfit()->Ammo() ? "Yes" : "No", textColor);
				if(isHover && defensiveZone.Contains(hoverPoint))
					turretTable.DrawHighlightCell(dim);
				turretTable.Draw(hardpoint.IsDefensive() ? "Off" : "On", textColor);
				if(isHover && autofireZone.Contains(hoverPoint))
					turretTable.DrawHighlightCell(dim);
				turretTable.Draw(!hardpoint.HasIndividualAFMode() ? "default" : (!hardpoint.IsAutoFireOn() ? "never" : (!hardpoint.FrugalAutoFire() ? "always" : "frugal")), textColor);
				turretTable.Draw(hardpoint.GetOutfit()->TurretTurn() * 60., textColor);
				if(isHover && opportunisticZone.Contains(hoverPoint))
					turretTable.DrawHighlightCell(dim);
				turretTable.Draw(hardpoint.IsOpportunistic() ? "Opportunistic" : "Focused", textColor);
			}
		}
		else
		{
			if(isHover)
			{
				table.DrawHighlight(dimmer);
			}
			table.Draw(name, textColor);
			if(!hardpoint.GetOutfit())
			{
				table.Advance(6);
			}
			else
			{
				table.Draw(hardpoint.GetOutfit()->Range(), textColor);
				table.Draw(hardpoint.GetOutfit()->Ammo() ? "Yes" : "No", textColor);
				if(isHover && defensiveZone.Contains(hoverPoint))
					table.DrawHighlightCell(dim);
				table.Draw(hardpoint.IsDefensive() ? "Off" : "On", textColor);
				if(isHover && autofireZone.Contains(hoverPoint))
					table.DrawHighlightCell(dim);
				table.Draw(!hardpoint.HasIndividualAFMode() ? "default" : (!hardpoint.IsAutoFireOn() ? "never" : (!hardpoint.FrugalAutoFire() ? "always" : "frugal")), textColor);
				table.Advance();
				if(isHover && opportunisticZone.Contains(hoverPoint))
					table.DrawHighlightCell(dim);
				table.Draw(hardpoint.IsOpportunistic() ? "Opportunistic" : "Focused", textColor);
			}
		}

		// Draw the line.
		Point from(weaponsBounds.Left(), zoneCenter.Y());
		Point to = silhouetteBounds.Center() + (2. * scale) * hardpoint.GetPoint();
		DrawLine(from, to, color);
		if(isHover)
		{
			topFrom = from;
			topTo = to;
			topColor = color;
			hasTop = true;
		}

		++index;
	}

	// Make sure the line for whatever hardpoint we're hovering is always on top.
	if(hasTop)
		DrawLine(topFrom, topTo, topColor);

	// Re-positioning weapons.
	if(draggingIndex >= 0)
	{
		const Outfit *outfit = ship.Weapons()[draggingIndex].GetOutfit();
		string name = outfit ? outfit->Name() : "[empty]";
		Point pos(hoverPoint.X() - .5 * font.Width(name), hoverPoint.Y());
		font.Draw(name, pos + Point(1., 1.), Color(0., 1.));
		font.Draw(name, pos, bright);
	}
}



bool WeaponConfigPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool /* isNewPress */)
{
	bool control = (mod & (KMOD_CTRL | KMOD_GUI));
	bool shift = (mod & KMOD_SHIFT);
	if(key == 'd' || key == SDLK_ESCAPE || (key == 'w' && control))
	{
		GetUI()->Pop(this);
		GetUI()->Push(new ShipInfoPanel(player, std::move(panelState)));
	}
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
		// Set scroll so the currently shown ship will be the first in page.
		panelState.SetScroll(shipIt - panelState.Ships().begin());

		GetUI()->Pop(this);
		GetUI()->Push(new PlayerInfoPanel(player, std::move(panelState)));
	}
	else if(panelState.CanEdit() && (key == 'P' || (key == 'p' && shift)))
	{
		if(shipIt->get() != player.Flagship() || (*shipIt)->IsParked())
			player.ParkShip(shipIt->get(), !(*shipIt)->IsParked());
	}
	else if(command.Has(Command::MAP) || key == 'm')
		GetUI()->Push(new MissionPanel(player));
	else if(key == 'l' && player.HasLogs())
		GetUI()->Push(new LogbookPanel(player));
	else
		return false;

	return true;
}



bool WeaponConfigPanel::Click(int x, int y, int /* clicks */)
{
	if(shipIt == panelState.Ships().end())
		return true;

	draggingIndex = -1;
	Point clickPoint(x, y);
	if(defensiveZone.Contains(clickPoint))
	{
		(**shipIt).GetArmament().ToggleDefensive(hoverIndex);
		return true;
	}
	else if(autofireZone.Contains(clickPoint))
	{
		(**shipIt).GetArmament().CycleAutoFireMode(hoverIndex);
		return true;
	}
	else if(opportunisticZone.Contains(clickPoint))
	{
		(**shipIt).GetArmament().ToggleOpportunistic(hoverIndex);
		return true;
	}
	if(panelState.CanEdit() && hoverIndex >= 0 && (**shipIt).GetSystem() == player.GetSystem() && !(**shipIt).IsDisabled())
		draggingIndex = hoverIndex;

	return true;
}



bool WeaponConfigPanel::Hover(int x, int y)
{
	Point point(x, y);
	//info.Hover(point);
	return Hover(point);
}



bool WeaponConfigPanel::RClick(int x, int y)
{
	return false;
}



bool WeaponConfigPanel::Scroll(double dx, double dy)
{
	return false;
}



bool WeaponConfigPanel::Drag(double dx, double dy)
{
	return Hover(hoverPoint + Point(dx, dy));
}



bool WeaponConfigPanel::Release(int /* x */, int /* y */)
{
	if(draggingIndex >= 0 && hoverIndex >= 0 && hoverIndex != draggingIndex)
		(**shipIt).GetArmament().Swap(hoverIndex, draggingIndex);

	draggingIndex = -1;
	return true;
}



void WeaponConfigPanel::UpdateInfo()
{
	draggingIndex = -1;
	hoverIndex = -1;
	ClearZones();
	if(shipIt == panelState.Ships().end())
		return;

	const Ship &ship = **shipIt;
	if(player.Flagship() && ship.GetSystem() == player.GetSystem() && &ship != player.Flagship())
		player.Flagship()->SetTargetShip(*shipIt);
}



void WeaponConfigPanel::ClearZones()
{
	zones.clear();
}



void WeaponConfigPanel::DrawLine(const Point &from, const Point &to, const Color &color) const
{
	Color black(0.f, 1.f);
	Point mid(to.X(), from.Y());

	LineShader::Draw(from, mid, 3.5f, black);
	LineShader::Draw(mid, to, 3.5f, black);
	LineShader::Draw(from, mid, 1.5f, color);
	LineShader::Draw(mid, to, 1.5f, color);
}



bool WeaponConfigPanel::Hover(const Point &point)
{
	if(shipIt == panelState.Ships().end())
		return true;

	hoverPoint = point;

	hoverIndex = -1;
	const vector<Hardpoint> &weapons = (**shipIt).Weapons();
	bool dragIsTurret = (draggingIndex >= 0 && weapons[draggingIndex].IsTurret());
	for(const auto &zone : zones)
	{
		bool isTurret = weapons[zone.Value()].IsTurret();
		if(zone.Contains(hoverPoint) && (draggingIndex == -1 || isTurret == dragIsTurret))
			hoverIndex = zone.Value();
	}

	return true;
}



void WeaponConfigPanel::SetControlColumnZones(double height, double tableLeft)
{
	double defensiveColumnCenterX = columns[2].GetCenter() + tableLeft;
	double defensiveColumnWidth = columns[2].layout.width;
	defensiveZone = Rectangle(Point(defensiveColumnCenterX, 0), Point(defensiveColumnWidth, height));

	double autofireColumnCenterX = columns[3].GetCenter() + tableLeft;
	double autofireColumnWidth = columns[3].layout.width;
	autofireZone = Rectangle(Point(autofireColumnCenterX, 0), Point(autofireColumnWidth, height));

	double opportunisticColumnCenterX = columns[5].GetCenter() + tableLeft;
	double opportunisticColumnWidth = columns[5].layout.width;
	opportunisticZone = Rectangle(Point(opportunisticColumnCenterX, 0), Point(opportunisticColumnWidth, height));
}



WeaponConfigPanel::Column::Column(double start, Layout layout)
: start(start), layout(layout)
{
}



double WeaponConfigPanel::Column::GetLeft() const
{
	if(layout.align == Alignment::LEFT)
		return start;
	if(layout.align == Alignment::RIGHT)
		return start - layout.width;
	return start - (.5 * layout.width);
}



double WeaponConfigPanel::Column::GetRight() const
{
	if(layout.align == Alignment::LEFT)
		return start + layout.width;
	if(layout.align == Alignment::RIGHT)
		return start;
	return start + (.5 * layout.width);
}



double WeaponConfigPanel::Column::GetCenter() const
{
	if(layout.align == Alignment::LEFT)
		return start + layout.width / 2;
	if(layout.align == Alignment::RIGHT)
		return start - (.5 * layout.width);
	return start;
}





