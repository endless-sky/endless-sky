/*WeaponConfigPanel.cpp
Copyright (c) 2022 by warpcore

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
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

namespace {
	//constexpr double WIDTH = 250.;
	//constexpr int COLUMN_WIDTH = static_cast<int>(WIDTH) - 20;
}

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

	// Draw the interface.
	const Interface *weaponConfigPanelUi = GameData::Interfaces().Get("info panel");
	weaponConfigPanelUi->Draw(interfaceInfo, this);

	// Draw all the different information sections.
	ClearZones();
	if(shipIt == player.Ships().end())
		return;
	DrawWeapons(weaponConfigPanelUi->GetBox("silhouette"), weaponConfigPanelUi->GetBox("weaponsList"));
}



bool WeaponConfigPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool /* isNewPress */)
{
	bool control = (mod & (KMOD_CTRL | KMOD_GUI));
	bool shift = (mod & KMOD_SHIFT);
	if(key == 'd' || key == SDLK_ESCAPE || (key == 'w' && control))
		GetUI()->Pop(this);
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



void WeaponConfigPanel::DrawWeapons(const Rectangle &silhouetteBounds, const Rectangle &weaponsBounds)
{
	// Constants for arranging stuff.
	static const double WIDTH = silhouetteBounds.Width();
	static const double LINE_HEIGHT = 20.;
	static const double GUN_TURRET_GAP = 10.;
	static const double LABEL_PAD = 5.;
	static const double LABEL_WIDTH = weaponsBounds.Width() - 20.;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
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

	double gunY = weaponsBounds.Top() + .5 * (weaponsBounds.Height() - height);
	double turretY = gunY + LINE_HEIGHT * gunRows + GUN_TURRET_GAP * (gunRows != 0);
	double nextY[2] = {gunY + LINE_HEIGHT * (gunRows - count[0]), turretY + LINE_HEIGHT * (turretRows - count[1])};

	// Table attributes.
	Table table;
	Table turretTable;
	table.AddColumn(50, {50, Alignment::RIGHT, Truncate::BACK}); // Range
	turretTable.AddColumn(50, {50, Alignment::RIGHT, Truncate::BACK});
	table.AddColumn(60, {50, Alignment::LEFT, Truncate::BACK}); // Ammo?
	turretTable.AddColumn(60, {50, Alignment::LEFT, Truncate::BACK});
	table.AddColumn(120, {100, Alignment::LEFT, Truncate::BACK}); // Defensive?
	turretTable.AddColumn(120, {100, Alignment::LEFT, Truncate::BACK});
	table.AddColumn(320, {100, Alignment::RIGHT, Truncate::BACK}); // Turn rate
	turretTable.AddColumn(320, {100, Alignment::RIGHT, Truncate::BACK});
	table.AddColumn(330, {100, Alignment::LEFT, Truncate::BACK}); // Opportunistic?
	turretTable.AddColumn(330, {100, Alignment::LEFT, Truncate::BACK});
	table.SetUnderline(weaponsBounds.Right() + 10., 480);



	table.DrawAt(Point(10. + weaponsBounds.Right(), gunY - LINE_HEIGHT));
	turretTable.DrawAt(Point(10. + weaponsBounds.Right(), turretY));

	// Header row.
	table.DrawUnderline(dim);
	table.SetColor(bright);
	table.Draw("range");
	table.Draw("ammo?");
	table.Draw("defensive");
	//table.Draw("ammo use");
	table.Draw("turn speed");
	table.Draw("opportunistic");

	//queue<int> turretIndices;

	int index = 0;

	static const double TEXT_OFF = .5 * (LINE_HEIGHT - font.Height());
	static const Point LINE_SIZE(LABEL_WIDTH, LINE_HEIGHT);
	Point topFrom;
	Point topTo;
	Color topColor;
	bool hasTop = false;
	auto layout = Layout(static_cast<int>(LABEL_WIDTH), Truncate::BACK);
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		string name = "[empty]";
		if(hardpoint.GetOutfit())
			name = hardpoint.GetOutfit()->Name();

		bool isTurret = hardpoint.IsTurret();

		double &y = nextY[isTurret];
		double x = weaponsBounds.Left() + LABEL_PAD;
		bool isHover = (index == hoverIndex);
		layout.align = Alignment::LEFT;
		Color textColor = isHover ? bright : dim;
		font.Draw({name, layout}, Point(x, y + TEXT_OFF), textColor);
		Point zoneCenter(weaponsBounds.Center().X(), y + .5 * LINE_HEIGHT);
		zones.emplace_back(zoneCenter, LINE_SIZE, index);

		DrawLine(Point(x, y), Point(x + 750, y), bright);

		// Determine what color to use for the line.
		float high = (isHover ? .8f : .5f);
		Color color(high, .75f * high, 0.f, 1.f);
		if(isTurret)
		{
			color = Color(0.f, .75f * high, high, 1.f);
			//turretIndices.push(index);
			//int value = turretIndices.front();
			//turretIndices.pop();
			//const Hardpoint &hardpoint = ship.Weapons().at(value);
			if(!hardpoint.GetOutfit())
			{
				turretTable.Advance(5);
			}
			else if(hardpoint.GetOutfit()->AntiMissile())
			{
				turretTable.Draw(hardpoint.GetOutfit()->Range(), textColor);
				turretTable.Advance(4);
			}
			else
			{
				turretTable.Draw(hardpoint.GetOutfit()->Range(), textColor);
				turretTable.Draw(hardpoint.GetOutfit()->Ammo() ? "Yes" : "No", textColor);
				turretTable.Draw(hardpoint.IsDefensive() ? "On" : "Off", textColor);
				turretTable.Draw(hardpoint.GetOutfit()->TurretTurn(), textColor);
				turretTable.Draw(hardpoint.IsOpportunistic() ? "On" : "Off", textColor);
			}
		}
		else
		{
			if(!hardpoint.GetOutfit())
			{
				table.Advance(5);
				continue;
			}
			table.Draw(hardpoint.GetOutfit()->Range(), textColor);
			table.Draw(hardpoint.GetOutfit()->Ammo() ? "Yes" : "No", textColor);
			table.Draw(hardpoint.IsDefensive() ? "On" : "Off", textColor);
			table.Advance();
			table.Draw(hardpoint.IsOpportunistic() ? "On" : "Off", textColor);
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

		y += LINE_HEIGHT;
		++index;
	}
	// If there are turret and gun hardpoints, draw a gap before starting the turrets.
	//if(count[0] && count[1])
		//table.DrawGap(GUN_TURRET_GAP);

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



