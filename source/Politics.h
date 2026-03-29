/* Politics.h
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

#pragma once

#include <map>
#include <set>
#include <string>

class Conversation;
class Government;
class Planet;
class PlayerInfo;
class Ship;



// This class represents the current state of relationships between governments
// in the game, and in particular the relationship of each government to the
// player. The player has a reputation with each government, which is affected
// by what they do for a government or its allies or enemies.
class Politics {
public:
	// Reset to the initial political state defined in the game data.
	void Reset();

	bool IsEnemy(const Government *first, const Government *second) const;

	// Commit the given "offense" against the given government (which may not
	// actually consider it to be an offense). This may result in temporary
	// hostilities (if the event type is PROVOKE), or a permanent change to your
	// reputation.
	void Offend(const Government *gov, int eventType, int count = 1);
	// Bribe the given government to be friendly to you for one day.
	void Bribe(const Government *gov);

	// Check if the given ship can land on the given planet.
	bool CanLand(const Ship &ship, const Planet *planet) const;
	// Check if the player can land on the given planet.
	bool CanLand(const Planet *planet) const;
	// Check if the player has been granted clearance to land on this planet, either
	// through bribes or domination.
	bool HasClearance(const Planet *planet) const;
	bool CanUseServices(const Planet *planet) const;
	// Bribe a planet to let the player's ships land there.
	void BribePlanet(const Planet *planet, bool fullAccess);
	void DominatePlanet(const Planet *planet, bool dominate = true);
	bool HasDominated(const Planet *planet) const;

	// Check to see if the player has done anything they should be fined for.
	// Each government can only fine you once per day.
	std::pair<const Conversation *, std::string> Fine(PlayerInfo &player,
		const Government *gov, int scan, const Ship *target, double security);

	// Get or set your reputation with the given government.
	double Reputation(const Government *gov) const;
	void AddReputation(const Government *gov, double value);
	void SetReputation(const Government *gov, double value);

	// Reset any temporary effects (typically because a day has passed).
	void ResetDaily();


private:
	// attitude[target][other] stores how much an action toward the given target
	// government will affect your reputation with the given other government.
	// The relationships need not be perfectly symmetrical. For example, just
	// because Republic ships will help a merchant under attack does not mean
	// that merchants will come to the aid of Republic ships.
	std::map<const Government *, double> reputationWith;
	std::set<const Government *> provoked;
	std::set<const Government *> bribed;
	std::map<const Planet *, bool> bribedPlanets;
	std::set<const Planet *> dominatedPlanets;
	std::set<const Government *> fined;
};
