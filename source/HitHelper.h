/* HitHelper.h
Copyright (c) 2024 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "GameData.h"
#include "Gamerules.h"
#include "Ship.h"
#include "Weapon.h"



class HitHelper
{
public:
	// Check whether a ship can be validly hit by a projectile fired from the given shooter.
	static bool IsValid(const Ship &ship, const Ship *shooter, const Weapon *weapon)
	{
		const Gamerules &rules = GameData::GetGamerules();
		bool isDisabled = ship.IsDisabled();
		// Certain weapons can't hit disabled ships unless targeting them.
		if(isDisabled && weapon && !weapon->CanCollideDisabled())
			return false;
		// Gamerules can also provide blanket protection to some or all ships.
		// Crossfire immunity might only apply to disabled ships.
		if(!isDisabled && rules.CrossfireCreatingDisabledOnly())
			return true;
		// Check whether this ship is capable of receiving crossfire.
		switch(rules.CrossfireReceivingImmunity(ship.CanBeCarried()))
		{
			case Gamerules::CrossfireImmunity::NONE:
				break;
			case Gamerules::CrossfireImmunity::PLAYER_SHIPS:
				if(ship.IsYours())
					return false;
				break;
			case Gamerules::CrossfireImmunity::ALL_SHIPS:
				return false;
		}
		// If this hit doesn't have a known shooter, then it is valid.
		if(!shooter)
			return true;
		// Otherwise, check if the firing ship can create crossfire.
		switch(rules.CrossfireCreatingImmunity(shooter->CanBeCarried()))
		{
			case Gamerules::CrossfireImmunity::NONE:
				return true;
			case Gamerules::CrossfireImmunity::PLAYER_SHIPS:
				return !shooter->IsYours();
			case Gamerules::CrossfireImmunity::ALL_SHIPS:
				return false;
		}
		return false;
	}
};
