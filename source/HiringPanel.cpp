/* HiringPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "HiringPanel.h"

#include "FillShader.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "UI.h"

using namespace std;



HiringPanel::HiringPanel(PlayerInfo &player)
	: player(player), maxHire(0), maxFire(0)
{
	SetTrapAllEvents(false);
}



void HiringPanel::Step()
{
	DoHelp("hiring");
}



void HiringPanel::Draw()
{
	if(!player.Flagship())
		return;
	const Ship &flagship = *player.Flagship();
	
	// Draw a line in the same place as the trading and bank panels.
	FillShader::Fill(Point(-60., 95.), Point(480., 1.), *GameData::Colors().Get("medium"));
	
	const Interface *interface = GameData::Interfaces().Get("hiring");
	Information info;
	
	int flagshipBunks = flagship.Attributes().Get("bunks");
	int flagshipRequired = flagship.RequiredCrew();
	int flagshipExtra = flagship.Crew() - flagshipRequired;
	int flagshipUnused = flagshipBunks - flagship.Crew();
	info.SetString("flagship bunks", to_string(static_cast<int>(flagshipBunks)));
	info.SetString("flagship required", to_string(static_cast<int>(flagshipRequired)));
	info.SetString("flagship extra", to_string(static_cast<int>(flagshipExtra)));
	info.SetString("flagship unused", to_string(static_cast<int>(flagshipUnused)));
	
	// Sum up the statistics for all your ships.
	int fleetBunks = 0;
	int fleetRequired = 0;
	for(const shared_ptr<Ship> &ship : player.Ships())
		if(!ship->IsParked())
		{
			fleetBunks += static_cast<int>(ship->Attributes().Get("bunks"));
			fleetRequired += ship->RequiredCrew();
		}
	int passengers = player.Cargo().Passengers();
	int fleetUnused = fleetBunks - fleetRequired - flagshipExtra;
	info.SetString("fleet bunks", to_string(static_cast<int>(fleetBunks)));
	info.SetString("fleet required", to_string(static_cast<int>(fleetRequired)));
	info.SetString("fleet unused", to_string(static_cast<int>(fleetUnused)));
	info.SetString("passengers", to_string(static_cast<int>(passengers)));
	
	static const int DAILY_SALARY = 100;
	int salary = DAILY_SALARY * (fleetRequired - 1);
	int extraSalary = DAILY_SALARY * flagshipExtra;
	info.SetString("salary required", to_string(static_cast<int>(salary)));
	info.SetString("salary extra", to_string(static_cast<int>(extraSalary)));
	
	int modifier = Modifier();
	if(modifier > 1)
		info.SetString("modifier", "x " + to_string(modifier));
	
	maxFire = max(flagshipExtra, 0);
	maxHire = max(min(flagshipUnused, fleetUnused - passengers), 0);
	
	if(maxHire)
		info.SetCondition("can hire");
	if(maxFire)
		info.SetCondition("can fire");
	
	interface->Draw(info, this);
}



bool HiringPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(!player.Flagship())
		return false;
	
	if(key == 'h' || key == '=' || key == SDLK_RETURN || key == SDLK_SPACE)
	{
		player.Flagship()->AddCrew(min(maxHire, Modifier()));
		player.UpdateCargoCapacities();
	}
	else if(key == 'f' || key == '-' || key == SDLK_BACKSPACE || key == SDLK_DELETE)
	{
		player.Flagship()->AddCrew(-min(maxFire, Modifier()));
		player.UpdateCargoCapacities();
	}
	
	return false;
}
