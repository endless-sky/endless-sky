/* LandedOfferPanel.cpp
Copyright (c) 2023 by an anonymous author

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "LandedOfferPanel.h"

#include "PlayerInfo.h"
#include "Port.h"
#include "Ship.h"
#include "UI.h"

using namespace std;



LandedOfferPanel::LandedOfferPanel(PlayerInfo &player, Mission::Location location, shared_ptr<Panel> otherPanel):
	player(player), location(location), otherPanel(otherPanel)
{
}



bool LandedOfferPanel::TimeToLeaveOrDie() const
{
	// If the player is dead, we must pass control up to the PlanetPanel to handle that.
	if(player.IsDead())
		return true;
	// If the conversation wants to launch, then PlanetPanel needs to launch the player.
	const Ship *flagship = player.Flagship();
	return flagship && flagship->CanBeFlagship() && player.ShouldLaunch();
}



void LandedOfferPanel::Step()
{
	// Offer missions. In each Step, any number of non-UI missions
	// will be offered, but only one UI mission. This ensures the
	// player cannot accidentally depart or switch screens until
	// all missions have been processed.
	//
	// The otherPanel is to handle a special case in the PlanetPanel.
	// If the player starts a new game, exits the shipyard without buying
	// anything, clicks to the bank, then returns to the shipyard and buys a
	// ship, this will make sure they are shown an intro mission.
	while(GetUI()->WillBeTop(this) || (otherPanel && GetUI()->WillBeTop(otherPanel.get())))
	{
		if(TimeToLeaveOrDie())
			return;
		// Find a mission to offer here, if there is one.
		Mission *mission = player.MissionToOffer(location);
		// It is possible for non-landing missions to cause a landing mission's
		// "to accept" to be true, when it was false at the landing screen.
		// This ensures those landing missions will be offered at the spaceport.
		// For the shops, it makes more sense to wait until the player returns
		// to the landing area.
		if(location == Mission::SPACEPORT && !mission && GetPort() &&
				GetPort()->HasService(Port::ServicesType::OffersMissions))
			mission = player.MissionToOffer(Mission::LANDING);

		// Offer a mission if we have one.
		if(mission)
			mission->Do(Mission::OFFER, player, GetUI());
		// Otherwise, show "blocked mission" dialogs.
		else if(player.HandleBlockedMissions(location, GetUI()))
			return;
		// If a landing mission could have been offered but was blocked, warn about that now.
		else if(location == Mission::SPACEPORT && player.HandleBlockedMissions(Mission::LANDING, GetUI()))
			return;
		// Nothing could be offered, so we're done.
		else
			return;
	}
}



const Port *LandedOfferPanel::GetPort() const
{
	return nullptr;
}
