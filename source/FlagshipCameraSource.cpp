/* FlagshipCameraSource.cpp
Copyright (c) 2024 by the Endless Sky developers

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FlagshipCameraSource.h"

#include "PlayerInfo.h"
#include "Ship.h"

FlagshipCameraSource::FlagshipCameraSource(PlayerInfo &player)
	: player(player)
{
}

Point FlagshipCameraSource::GetTarget() const
{
	auto flagship = player.FlagshipPtr();
	return flagship ? flagship->Center() : Point();
}

Point FlagshipCameraSource::GetVelocity() const
{
	auto flagship = player.FlagshipPtr();
	return flagship ? flagship->Velocity() : Point();
}

std::shared_ptr<Ship> FlagshipCameraSource::GetShipForHUD() const
{
	return player.FlagshipPtr();
}

void FlagshipCameraSource::Step()
{
	// Flagship camera doesn't need per-frame updates - the ship handles its own movement
}
