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
	if(!first || !second)
		return false;

	if(first == second)
		return false;

	// Just for simplicity, if one of the governments is the player, make sure
	// it is the first one.
	if(second->IsPlayer())
		swap(first, second);
	if(first->IsPlayer())
	{
		if(bribed.contains(second))
			return false;
		if(provoked.contains(second))
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
	if(!gov)
		return;

	if(gov->IsPlayer())
		return;

	for(const auto &it : GameData::Governments())
	{
		const Government *other = &it.second;
		double weight = other->AttitudeToward(gov);
		Government::PenaltyEffect penalty = other->PenaltyFor(eventType, gov);

		// You can provoke a government even by attacking an empty ship, such as
		// a drone (count = 0, because count = crew).
		if(penalty.specialPenalty == Government::SpecialPenalty::PROVOKE)
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
			double reputationChange = (count * weight) * penalty.reputationChange;
			if(penalty.specialPenalty == Government::SpecialPenalty::ATROCITY && weight > 0)
				Politics::SetReputation(other, min(0., reputationWith[other]));

			Politics::AddReputation(other, -reputationChange);
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

	const Government *gov = ship.GetGovernment();
	if(!gov->IsPlayer())
		return !ship.IsRestrictedFrom(*planet) &&
			(!planet->IsInhabited() || !IsEnemy(gov, planet->GetGovernment()));

	return CanLand(planet);
}



bool Politics::CanLand(const Planet *planet) const
{
	if(!planet || !planet->GetSystem())
		return false;
	if(!planet->IsInhabited())
		return true;
	if(HasClearance(planet))
		return true;
	if(provoked.contains(planet->GetGovernment()))
		return false;

	return Reputation(planet->GetGovernment()) >= planet->RequiredReputation();
}



// Check if the player has been granted clearance to land on this planet, either
// through bribes, domination, or mission clearance.
bool Politics::HasClearance(const Planet *planet) const
{
	return dominatedPlanets.contains(planet) || bribedPlanets.contains(planet);
}



bool Politics::CanUseServices(const Planet *planet) const
{
	if(!planet || !planet->GetSystem())
		return false;
	if(dominatedPlanets.contains(planet))
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
	return dominatedPlanets.contains(planet);
}



// Check to see if the player has done anything they should be fined for.
pair<const Conversation *, string> Politics::Fine(PlayerInfo &player,
	const Government *gov, int scan, const Ship *target, double security)
{
	// Do nothing if you have already been fined today, or if you evade
	// detection.
	if(fined.contains(gov) || Random::Real() > security)
		return {};

	const Conversation *deathSentence = nullptr;
	string reason;
	int64_t maxFine = 0;
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		if(target && target != &*ship)
			continue;
		if(ship->GetSystem() != player.GetSystem())
			continue;
		const Planet *planet = player.GetPlanet();
		if(planet && ship->GetPlanet() != planet)
			continue;
		// Skip parked ships. The spaceport authorities are only scanning the ships you just landed with.
		if(ship->IsParked())
			continue;

		int failedMissions = 0;

		// Illegal passengers can only be detected by planetary security.
		if(!scan)
		{
			int64_t fine = ship->Cargo().IllegalPassengersFine(gov);
			if((fine > maxFine && maxFine >= 0) || fine < 0)
			{
				maxFine = fine;
				reason = " for carrying illegal passengers.";

				for(const Mission &mission : player.Missions())
				{
					if(mission.IsFailed())
						continue;

					string fineMessage = mission.FineMessage();
					if(!fineMessage.empty())
					{
						reason = ".\n\t";
						reason.append(fineMessage);
					}
					// Fail any missions with illegal passengers and "stealth" set.
					if(mission.Fine() > 0 && mission.Passengers() && mission.FailIfDiscovered())
					{
						player.FailMission(mission);
						++failedMissions;
					}
				}
			}
		}
		if((!scan || (scan & ShipEvent::SCAN_CARGO)) && !EvadesCargoScan(*ship))
		{
			pair<int, const Conversation *> fine = ship->Cargo().IllegalCargoFine(gov);
			if(fine.second)
				deathSentence = fine.second;
			if((fine.first > maxFine && maxFine >= 0) || fine.first < 0)
			{
				maxFine = fine.first;
				reason = " for carrying illegal cargo.";

				for(const Mission &mission : player.Missions())
				{
					if(mission.IsFailed())
						continue;

					// Append the fineMessage from each applicable mission, if available.
					string fineMessage = mission.FineMessage();
					if(!fineMessage.empty())
					{
						reason = ".\n\t";
						reason.append(fineMessage);
					}
					// Fail any missions with illegal cargo and "stealth" set.
					if(mission.Fine() > 0 && mission.CargoSize() && mission.FailIfDiscovered())
					{
						player.FailMission(mission);
						++failedMissions;
					}
				}
			}
		}
		if((!scan || (scan & ShipEvent::SCAN_OUTFITS)) && !EvadesOutfitScan(*ship))
		{
			for(const auto &it : ship->Outfits())
				if(it.second)
				{
					int fine = gov->Fines(it.first);
					Government::Atrocity atrocity = gov->Condemns(it.first);
					if(atrocity.isAtrocity)
					{
						deathSentence = atrocity.customDeathSentence;
						fine = -1;
					}
					if((fine > maxFine && maxFine >= 0) || fine < 0)
					{
						maxFine = fine;
						reason = " for having illegal outfits installed on your ship.";
					}
				}

			int shipFine = gov->Fines(ship.get());
			Government::Atrocity atrocity = gov->Condemns(ship.get());
			if(atrocity.isAtrocity)
			{
				deathSentence = atrocity.customDeathSentence;
				shipFine = -1;
			}
			if((shipFine > maxFine && maxFine >= 0) || shipFine < 0)
			{
				maxFine = shipFine;
				reason = " for flying an illegal ship.";
			}
		}
		if(failedMissions && maxFine > 0)
		{
			reason += "\n\tYou failed " + Format::Number(failedMissions)
				+ ((failedMissions > 1) ? " missions" : " mission")
				+ " after your illegal cargo was discovered.";
		}
	}

	if(maxFine < 0)
	{
		gov->Offend(ShipEvent::ATROCITY);
		if(!scan)
		{
			reason = "atrocity";
			if(!deathSentence)
				deathSentence = gov->DeathSentence();
		}
		else
			reason = "After scanning your ship, the " + gov->DisplayName()
				+ " captain hails you with a grim expression on his face. He says, "
				"\"I'm afraid we're going to have to put you to death " + reason + " Goodbye.\"";
	}
	else if(maxFine > 0)
	{
		// Scale the fine based on how lenient this government is.
		maxFine = lround(maxFine * gov->GetFineFraction());
		reason = "The " + gov->DisplayName() + " authorities fine you "
			+ Format::CreditString(maxFine) + reason;
		player.Accounts().AddFine(maxFine);
		fined.insert(gov);
	}
	return {deathSentence, reason};
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
