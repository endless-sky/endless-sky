/* HiringPanel.cpp
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

#include "HiringPanel.h"

#include "Command.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "PlayerInfo.h"
#include "Screen.h"
#include "Ship.h"
#include "UI.h"

#include <algorithm>

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
	const Ship *flagship = player.Flagship();
	const Interface *hiring = GameData::Interfaces().Get(Screen::Width() < 1280 ? "hiring (small screen)" : "hiring");
	Information info;

	int flagshipBunks = 0;
	int flagshipRequired = 0;
	int flagshipExtra = 0;
	int flagshipUnused = 0;

	if(flagship)
	{
		flagshipBunks = flagship->Attributes().Get("bunks");
		flagshipRequired = flagship->RequiredCrew();
		flagshipExtra = flagship->Crew() - flagshipRequired;
		flagshipUnused = flagshipBunks - flagship->Crew();
	}

	info.SetString("flagship bunks", to_string(flagshipBunks));
	info.SetString("flagship required", to_string(flagshipRequired));
	info.SetString("flagship extra", to_string(flagshipExtra));
	info.SetString("flagship unused", to_string(flagshipUnused));

	// Sum up the statistics for all your ships. You still pay the crew of
	// disabled or out-of-system ships, but any parked ships have no crew costs.
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
	info.SetString("fleet bunks", to_string(fleetBunks));
	info.SetString("fleet required", to_string(fleetRequired));
	info.SetString("fleet unused", to_string(fleetUnused));
	info.SetString("passengers", to_string(passengers));

	static const int DAILY_SALARY = 100;
	int salary = DAILY_SALARY * (fleetRequired - (flagship ? 1 : 0));
	int extraSalary = DAILY_SALARY * flagshipExtra;
	info.SetString("salary required", to_string(salary));
	info.SetString("salary extra", to_string(extraSalary));

	int modifier = Modifier();
	if(modifier > 1)
		info.SetString("modifier", "x " + to_string(modifier));

	maxFire = max(flagshipExtra, 0);
	maxHire = max(min(flagshipUnused, fleetUnused - passengers), 0);

	if(maxHire)
		info.SetCondition("can hire");
	if(maxFire)
		info.SetCondition("can fire");

	hiring->Draw(info, this);
}



bool HiringPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(command.Has(Command::HELP))
	{
		DoHelp("hiring", true);
		return true;
	}

	if(!player.Flagship())
		return false;

	if(key == 'h' || key == SDLK_EQUALS || key == SDLK_KP_PLUS || key == SDLK_PLUS
		|| key == SDLK_RETURN || key == SDLK_SPACE)
	{
		player.Flagship()->AddCrew(min(maxHire, Modifier()));
		player.UpdateCargoCapacities();
	}
	else if(key == 'f' || key == SDLK_MINUS || key == SDLK_KP_MINUS || key == SDLK_BACKSPACE || key == SDLK_DELETE)
	{
		player.Flagship()->AddCrew(-min(maxFire, Modifier()));
		player.UpdateCargoCapacities();
	}
	else
		return false;

	UI::PlaySound(UI::UISound::NORMAL);
	return true;
}
