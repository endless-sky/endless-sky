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
#include "MissionPanel.h"
#include "PlayerInfo.h"
#include "PlayerInfoPanel.h"
#include "Ship.h"
#include "UI.h"

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
	/*if(panelState.CanEdit() && (shipIt != player.Ships().end())
			&& (shipIt->get() != player.Flagship() || (*shipIt)->IsParked()))
	{
		interfaceInfo.SetCondition((*shipIt)->IsParked() ? "show unpark" : "show park");
		interfaceInfo.SetCondition("show disown");
	}
	else if(!panelState.CanEdit())
	{
		interfaceInfo.SetCondition("show dump");
		if(CanDump())
			interfaceInfo.SetCondition("enable dump");
	}*/
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
	DrawWeapons(weaponConfigPanelUi->GetBox("weapons"));
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



void WeaponConfigPanel::DrawWeapons(const Rectangle &bounds) {

}

