/* FighterHitHelper.h
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

#ifndef FIGHTER_HIT_POLICY_H_
#define FIGHTER_HIT_POLICY_H_

#include "GameData.h"
#include "Gamerules.h"
#include "Ship.h"



class FighterHitHelper
{
public:
	// Checks whether the given ship is a valid target for non-targeted projectiles.
	static inline bool IsValidTarget(const Ship *ship)
	{
		if(!ship->CanBeCarried() || !ship->IsDisabled())
			return true;
		switch(GameData::GetGamerules().FightersHitWhenDisabled())
		{
			case Gamerules::FighterDodgePolicy::ALL: return false;
			case Gamerules::FighterDodgePolicy::NONE: return true;
			case Gamerules::FighterDodgePolicy::ONLY_PLAYER: return !ship->IsYours();
		}
		return false;
	}
};

#endif
