/* Politics.cpp
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



namespace {
	// Check if the ship evades being cargo scanned.
	bool EvadesCargoScan(const Ship &ship)
	{
		// Illegal goods can be hidden inside legal goods to avoid detection.
		const int contraband = ship.Cargo().IllegalCargoAmount();
		const int netIllegalCargo = contraband - ship.Attributes().Get("scan concealment");
		if(netIllegalCargo <= 0)
			return true;

		const int legalGoods = ship.Cargo().Used() - contraband;
		const double illegalRatio = legalGoods ? max(1., 2. * netIllegalCargo / legalGoods) : 1.;
		const double scanChance = illegalRatio / (1. + ship.Attributes().Get("scan interference"));
		return Random::Real() > scanChance;
	}

	// Check if the ship evades being outfit scanned.
	bool EvadesOutfitScan(const Ship &ship)
	{
		return ship.Attributes().Get("inscrutable") > 0. ||
				Random::Real() > 1. / (1. + ship.Attributes().Get("scan interference"));
	}
}



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
				// your bribe is canceled out.
				bribed.erase(other);
				provoked.insert(other);
			}
		}
		if(count && abs(weight) >= .05)
		{
			// Weights less than 5% should never cause permanent reputation
			// changes. This is to allow two governments to be hostile or
			// friendly without the player's behavior toward one of them
			// influencing their reputation with the other.
			double penalty = (count * weight) * other->PenaltyFor(eventType, gov);
			if(eventType & ShipEvent::ATROCITY && weight > 0)
				Politics::SetReputation(other, min(0., reputationWith[other]));

			Politics::AddReputation(other, -penalty);
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



// Check to see if the player has done anything they should be fined for on a planet.
string Politics::Fine(PlayerInfo &player, const Government *gov, double security)
{
	Punishment punishment = CalculateFine(player, gov, 0, nullptr);

	// Do nothing if you have already been fined today, or if you evade
	// detection.
	if(!punishment.HasPunishment() || fined.count(gov) || Random::Real() > security)
		return "";

	string reason;
	if(punishment.reason & Punishment::Cargo)
		reason = " for carrying illegal cargo";
	if(punishment.reason & Punishment::Outfit)
	{
		if(!reason.empty())
			reason += " and";
		reason += " for having illegal outfits installed on your ship";
	}
	reason += '.';

	if(!punishment.missionReason.empty())
	{
		reason = ".\n\t";
		reason += punishment.missionReason;
	}

	if(punishment.failedMissions)
		reason += "\n\tYou failed " + Format::Number(punishment.failedMissions)
			+ ((punishment.failedMissions > 1) ? " missions" : " mission")
			+ " after your illegal cargo was discovered.";

	if(punishment.isAtrocity)
	{
		gov->Offend(ShipEvent::ATROCITY);
		reason = "After scanning your ship, the " + gov->GetName()
			+ " captain hails you with a grim expression on his face. He says, "
			"\"I'm afraid we're going to have to put you to death " + reason + " Goodbye.\"";
	}
	else
	{
		// Scale the fine based on how lenient this government is.
		reason = "The " + gov->GetName() + " authorities fine you "
			+ Format::CreditString(punishment.cost) + reason;
		player.Accounts().AddFine(punishment.cost);
		fined.insert(gov);
	}

	return reason;
}



// Fine the player when in space. This can result in multiple fines per day, but only once per ship.
Politics::Punishment Politics::Fine(PlayerInfo &player, const Ship &ship, int scan, const Ship &target)
{
	auto punishment = CalculateFine(player, ship.GetGovernment(), scan, &target);

	// Do nothing if there is nothing to fine for or the target ship has already been fined today.
	if(!punishment.HasPunishment() || fined.count(ship.GetGovernment()))
		return {};

	if(punishment.isAtrocity)
		ship.GetGovernment()->Offend(ShipEvent::ATROCITY);
	fined.insert(ship.GetGovernment());
	return punishment;
}



// Get or set your reputation with the given government.
double Politics::Reputation(const Government *gov) const
{
	auto it = reputationWith.find(gov);
	return (it == reputationWith.end() ? 0. : it->second);
}



void Politics::AddReputation(const Government *gov, double value)
{
	SetReputation(gov, reputationWith[gov] + value);
}



void Politics::SetReputation(const Government *gov, double value)
{
	value = min(value, gov->ReputationMax());
	value = max(value, gov->ReputationMin());
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



Politics::Punishment Politics::CalculateFine(PlayerInfo &player, const Government *gov, int scan, const Ship *target)
{
	Punishment punishment;

	// Check if the government actually fines.
	if(!gov->GetFineFraction())
		return punishment;

	for(const auto &ship : player.Ships())
	{
		if(target && target != &*ship)
			continue;
		if(ship->GetSystem() != player.GetSystem())
			continue;

		if((!scan || (scan & ShipEvent::SCAN_CARGO)) && !EvadesCargoScan(*ship))
			if(auto fine = ship->Cargo().IllegalCargoFine(gov))
			{
				punishment.cost += fine;
				punishment.reason |= Punishment::Cargo;
			}
		if((!scan || (scan & ShipEvent::SCAN_OUTFITS)) && !EvadesOutfitScan(*ship))
		{
			for(const auto &it : ship->Outfits())
				if(it.second)
				{
					int64_t fine = gov->Fines(it.first);
					if(gov->Condemns(it.first))
						punishment.isAtrocity = true;

					if(fine > 0 || punishment.isAtrocity)
					{
						// Negative fines are an alternative way of specifying an atrocity.
						// We don't add them to the total fines that the player needs to pay.
						if(!punishment.isAtrocity)
							punishment.cost += fine;
						punishment.reason |= Punishment::Outfit;
					}
				}
		}
	}

	// Handle any missions that were failed because of punishment.
	// TODO: When scanning a single ship only mission cargo aboard that ship
	// should be failed.
	if(!scan || (scan & ShipEvent::SCAN_CARGO))
	{
		for(const Mission &mission : player.Missions())
		{
			if(mission.IsFailed())
				continue;

			// Append the illegalCargoMessage from each applicable mission, if available
			string illegalCargoMessage = mission.IllegalCargoMessage();
			if(!illegalCargoMessage.empty())
			{
				// Check to see if the message is already present. This is to avoid
				// displaying the same duplicate message to the player.
				if(punishment.missionReason.find(illegalCargoMessage) == string::npos)
				{
					if(!punishment.missionReason.empty())
						punishment.missionReason += "\n\t";
					punishment.missionReason += illegalCargoMessage;
				}
			}
			// Fail any missions with illegal cargo and "Stealth" set
			if(mission.IllegalCargoFine() > 0 && mission.FailIfDiscovered())
			{
				player.FailMission(mission);
				++punishment.failedMissions;
			}
		}
	}

	// Scale the fine based on how lenient this government is.
	punishment.cost = lround(punishment.cost * gov->GetFineFraction());
	return punishment;
}
