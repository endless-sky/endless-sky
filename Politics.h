/* Politics.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef POLITICS_H_
#define POLITICS_H_

#include <map>
#include <set>

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
	
	// Get the attitude of one government toward another. This does not apply to
	// the player's government, which uses "reputation" instead.
	double Attitude(const Government *gov, const Government *other) const;
	// Set the attitude of the given government toward the other government. A
	// positive value means they are allies, i.e. anything that affects your
	// reputation with one affects it with the other. A negative value means
	// that whatever hurts your reputation with one, helps it with the other.
	// The value should be between -1 and 1, controlling how strongly your
	// reputation is affected.
	void SetAttitude(const Government *gov, const Government *other, double value);
	// Commit the given "offense" against the given government (which may not
	// actually consider it to be an offense). This may result in temporary
	// hostilities (if the even type is PROVOKE), or a permanent change to your
	// reputation.
	void Offend(const Government *gov, int eventType, int count = 1);
	// Bribe the given government to be friendly to you for one day.
	void Bribe(const Government *gov);
	
	// Check if the given ship can land on the given planet.
	bool CanLand(const Ship &ship, const Planet *planet) const;
	// Check if the player can land on the given planet.
	bool CanLand(const Planet *planet) const;
	// Bribe a planet to let the player's ships land there.
	void BribePlanet(const Planet *planet);
	
	// Check to see if the player has done anything they should be fined for.
	// Each government can only fine you once per day.
	std::string Fine(PlayerInfo &player, const Government *gov, int scan = 0, double security = 1.);
	
	// Get or set your reputation with the given government.
	double Reputation(const Government *gov) const;
	void AddReputation(const Government *gov, double value);
	void SetReputation(const Government *gov, double value);
	
	// Reset any temporary provocation (typically because a day has passed).
	void ResetProvocation();
	
	
private:
	// attitude[target][other] stores how much an action toward the given target
	// government will affect your reputation with the given other government.
	// The relationships need not be perfectly symmetrical. For example, just
	// because Republic ships will help a merchant under attack does not mean
	// that merchants will come to the aid of Republic ships.
	std::map<const Government *, std::map<const Government *, double>> attitudeToward;
	std::map<const Government *, double> reputationWith;
	std::set<const Government *> provoked;
	std::set<const Government *> bribed;
	std::set<const Planet *> bribedPlanets;
	std::set<const Government *> fined;
};



#endif
