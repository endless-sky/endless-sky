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
#include "text/Format.h"
#include "GameData.h"
#include "Gamerules.h"
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
	int flagshipOfficers = 0;
	int flagshipRequired = 0;
	int flagshipExtra = 0;
	int flagshipUnused = 0;
	bool flagshipCarried = flagship->CanBeCarried();

	if(flagship)
	{
		flagshipBunks = flagship->Attributes().Get("bunks");
		flagshipOfficers = flagship->RequiredOfficers();
		flagshipRequired = flagship->RequiredCrew() - flagshipOfficers;
		flagshipExtra = flagship->Crew() - flagship->RequiredCrew();
		flagshipUnused = flagshipBunks - flagship->Crew();
	}

	info.SetString("flagship bunks", to_string(flagshipBunks));
	info.SetString("flagship officers", to_string(flagshipOfficers));
	info.SetString("flagship required", to_string(flagshipRequired));
	info.SetString("flagship extra", to_string(flagshipExtra));
	info.SetString("flagship unused", to_string(flagshipUnused));

	// Sum up the statistics for all your ships. You still pay the crew of
	// disabled or out-of-system ships, but any parked ships have no crew costs.
	player.UpdateCrew();

	int fleetOfficers = player.Officers() + (!flagshipCarried ? 1 : 0);
	int fleetBunks = 0;
	int fleetCrew = player.Crew() + (!flagshipCarried ? 0 : 1);
	int passengers = player.Cargo().Passengers();

	for(const shared_ptr<Ship> &ship : player.Ships())
		if(!ship->IsParked())
		{
			fleetBunks += static_cast<int>(ship->Attributes().Get("bunks"));
		}

	int fleetRequired = fleetCrew - flagshipExtra;
	int fleetUnused = fleetBunks - fleetCrew - fleetOfficers;

	info.SetString("fleet bunks", to_string(fleetBunks));
	info.SetString("fleet officers", to_string(fleetOfficers));
	info.SetString("fleet required", to_string(fleetRequired));
	info.SetString("fleet unused", to_string(fleetUnused));
	info.SetString("passengers", to_string(passengers));

	const int baseSalary = GameData::GetGamerules().BaseCrewSalary();
	const int baseOfficerSalary = GameData::GetGamerules().BaseOfficerSalary();
	const int officerSalaryPerCrew = GameData::GetGamerules().OfficerSalaryPerCrew();
	const double officerMultiplier = GameData::GetGamerules().OfficerMultiplier();

	int officerSalary = player.OfficerSalaries();
	int salary = player.CrewSalaries();
	int extraSalary = flagshipExtra * baseSalary;

	info.SetString("salary officers", Format::Credits(officerSalary));
	info.SetString("salary required", Format::Credits(salary));
	info.SetString("salary extra", Format::Credits(extraSalary));

	info.SetString("officer explanation 1", "(Each ship in your fleet requires one officer for every "
		+ Format::Number(GameData::GetGamerules().CrewPerOfficer()) + " crew, rounded up.)");
	info.SetString("officer explanation 2", "(An officer's salary is " + Format::CreditString(baseOfficerSalary)
		+ ", plus " + Format::Credits(officerSalaryPerCrew) + " for each crew member they");
	info.SetString("officer explanation 3", "command, multiplied by " + Format::Number(officerMultiplier * 100)
		+ " percent for each other officer in your fleet.)");

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
