/* Politics.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Politics.h"

#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"

#include <algorithm>
#include <cmath>

using namespace std;



// Reset to the initial political state defined in the game data.
void Politics::Reset()
{
	reputationWith.clear();
	dominatedPlanets.clear();
	ResetDaily();
	
	for(const auto &it : GameData::Governments())
		reputationWith[&it.second] = it.second.InitialPlayerReputation();
	
	// Disable fines for today (because the game was just loaded, so any fines
	// were already checked for when you first landed).
	for(const auto &it : GameData::Governments())
		fined.insert(&it.second);
}



bool Politics::IsEnemy(const Government *first, const Government *second) const
{
	if(first == second)
		return false;
	
	// Just for simplicity, if one of the governments is the player, make sure
	// it is the first one.
	if(second->IsPlayer())
		swap(first, second);
	if(first->IsPlayer())
	{
		if(bribed.count(second))
			return false;
		if(provoked.count(second))
			return true;
		
		auto it = reputationWith.find(second);
		return (it != reputationWith.end() && it->second < 0.);
	}
	
	// Neither government is the player, so the question of enemies depends only
	// on the attitude matrix.
	return (first->AttitudeToward(second) < 0. || second->AttitudeToward(first) < 0.);
}



// Commit the given "offense" against the given government (which may not
// actually consider it to be an offense). This may result in temporary
// hostilities (if the even type is PROVOKE), or a permanent change to your
// reputation.
void Politics::Offend(const Government *gov, int eventType, int count)
{
	if(gov->IsPlayer())
		return;
	
	for(const auto &it : GameData::Governments())
	{
		const Government *other = &it.second;
		double weight = other->AttitudeToward(gov);
		
		// You can provoke a government even by attacking an empty ship, such as
		// a drone (count = 0, because count = crew).
		if(eventType & ShipEvent::PROVOKE)
		{
			if(weight > 0.)
			{
				// If you bribe a government but then attack it, the effect of
				// your bribe is cancelled out.
				bribed.erase(other);
				provoked.insert(other);
			}
		}
		else if(count && abs(weight) >= .05)
		{
			// Weights less than 5% should never cause permanent reputation
			// changes. This is to allow two governments to be hostile or
			// friendly without the player's behavior toward one of them
			// influencing their reputation with the other.
			double penalty = (count * weight) * other->PenaltyFor(eventType);
			if(eventType & ShipEvent::ATROCITY)
				reputationWith[other] = min(0., reputationWith[other]);
			
			reputationWith[other] -= penalty;
		}
	}
}



// Bribe the given government to be friendly to you for one day.
void Politics::Bribe(const Government *gov)
{
	bribed.insert(gov);
	provoked.erase(gov);
	fined.insert(gov);
}



// Check if the given ship can land on the given planet.
bool Politics::CanLand(const Ship &ship, const Planet *planet) const
{
	if(!planet || !planet->GetSystem())
		return false;
	if(!planet->IsInhabited())
		return true;
	
	const Government *gov = ship.GetGovernment();
	if(!gov->IsPlayer())
		return !IsEnemy(gov, planet->GetGovernment());
	
	return CanLand(planet);
}



bool Politics::CanLand(const Planet *planet) const
{
	if(!planet || !planet->GetSystem())
		return false;
	if(!planet->IsInhabited())
		return true;
	if(dominatedPlanets.count(planet))
		return true;
	if(bribedPlanets.count(planet))
		return true;
	if(provoked.count(planet->GetGovernment()))
		return false;
	
	return Reputation(planet->GetGovernment()) >= planet->RequiredReputation();
}



bool Politics::CanUseServices(const Planet *planet) const
{
	if(!planet || !planet->GetSystem())
		return false;
	if(dominatedPlanets.count(planet))
		return true;
	
	auto it = bribedPlanets.find(planet);
	if(it != bribedPlanets.end())
		return it->second;
	
	return Reputation(planet->GetGovernment()) >= planet->RequiredReputation();
}



// Bribe a planet to let the player's ships land there.
void Politics::BribePlanet(const Planet *planet, bool fullAccess)
{
	bribedPlanets[planet] = fullAccess;
}



void Politics::DominatePlanet(const Planet *planet, bool dominate)
{
	if(dominate)
		dominatedPlanets.insert(planet);
	else
		dominatedPlanets.erase(planet);
}



bool Politics::HasDominated(const Planet *planet) const
{
	return dominatedPlanets.count(planet);
}



// Check to see if the player has done anything they should be fined for.
string Politics::Fine(PlayerInfo &player, const Government *gov, int scan, const Ship *target, double security)
{
	// Do nothing if you have already been fined today, or if you evade
	// detection.
	if(fined.count(gov) || Random::Real() > security || !gov->GetFineFraction())
		return "";
	
	string reason;
	int64_t maxFine = 0;
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		// Check if the ship evades being scanned due to interference plating.
		if(Random::Real() > 1. / (1. + ship->Attributes().Get("scan interference")))
			continue;
		if(target && target != &*ship)
			continue;
		if(ship->GetSystem() != player.GetSystem())
			continue;
		
		int failedMissions = 0;
		
		if(!scan || (scan & ShipEvent::SCAN_CARGO))
		{
			int64_t fine = ship->Cargo().IllegalCargoFine();
			if((fine > maxFine && maxFine >= 0) || fine < 0)
			{
				maxFine = fine;
				reason = " for carrying illegal cargo.";

				for(const Mission &mission : player.Missions())
				{
					if(mission.IsFailed())
						continue;
					
					// Append the illegalCargoMessage from each applicable mission, if available
					string illegalCargoMessage = mission.IllegalCargoMessage();
					if(!illegalCargoMessage.empty())
					{
						reason = ".\n\t";
						reason.append(illegalCargoMessage);
					}
					// Fail any missions with illegal cargo and "Stealth" set
					if(mission.IllegalCargoFine() > 0 && mission.FailIfDiscovered())
					{
						player.FailMission(mission);
						++failedMissions;
					}
				}
			}
		}
		if(!scan || (scan & ShipEvent::SCAN_OUTFITS))
		{
			for(const auto &it : ship->Outfits())
				if(it.second)
				{
					int64_t fine = it.first->Get("illegal");
					if(it.first->Get("atrocity") > 0.)
						fine = -1;
					if((fine > maxFine && maxFine >= 0) || fine < 0)
					{
						maxFine = fine;
						reason = " for having illegal outfits installed on your ship.";
					}
				}
		}
		if(failedMissions && maxFine > 0)
		{
			reason += "\n\tYou failed " + Format::Number(failedMissions) + ((failedMissions > 1) ? " missions" : " mission") 
				+ " after your illegal cargo was discovered.";
		}
	}
	
	if(maxFine < 0)
	{
		gov->Offend(ShipEvent::ATROCITY);
		if(!scan)
			reason = "atrocity";
		else
			reason = "After scanning your ship, the " + gov->GetName()
				+ " captain hails you with a grim expression on his face. He says, "
				"\"I'm afraid we're going to have to put you to death " + reason + " Goodbye.\"";
	}
	else if(maxFine > 0)
	{
		// Scale the fine based on how lenient this government is.
		maxFine = lround(maxFine * gov->GetFineFraction());
		reason = "The " + gov->GetName() + " authorities fine you "
			+ Format::Credits(maxFine) + " credits" + reason;
		player.Accounts().AddFine(maxFine);
		fined.insert(gov);
	}
	return reason;
}



// Get or set your reputation with the given government.
double Politics::Reputation(const Government *gov) const
{
	auto it = reputationWith.find(gov);
	return (it == reputationWith.end() ? 0. : it->second);
}



void Politics::AddReputation(const Government *gov, double value)
{
	reputationWith[gov] += value;
}



void Politics::SetReputation(const Government *gov, double value)
{
	reputationWith[gov] = value;
}



// Reset any temporary provocation (typically because a day has passed).
void Politics::ResetDaily()
{
	provoked.clear();
	bribed.clear();
	bribedPlanets.clear();
	fined.clear();
}
