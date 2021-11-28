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

#include <cstdint>
#include <map>
#include <set>
#include <string>


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
	// This class represents a punishment a government can give to a player.
	// This is currently either a fine or a death sentence for an atrocity.
	class Punishment
	{
	public:
		static constexpr int Cargo = 1;
		static constexpr int Outfit = 2;


	public:
		// The cost of the fine, i.e. how much the player has to pay.
		int64_t cost = 0;
		// Whether this punishment is an atrocity and warrants the death penalty.
		bool isAtrocity = false;
		// For what the type of goods punishment was issued (see above).
		int reason = 0;
		// The amount of missions that failed as a result of this punishment.
		int failedMissions = 0;
		// If any failed mission as a result of this punishment includes any custom
		// message, this string will contain all of them.
		std::string missionReason;


	public:
		// Whether this punishment isn't "empty".
		bool HasPunishment() const { return cost || isAtrocity; }
	};


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
	bool CanUseServices(const Planet *planet) const;
	// Bribe a planet to let the player's ships land there.
	void BribePlanet(const Planet *planet, bool fullAccess);
	void DominatePlanet(const Planet *planet, bool dominate = true);
	bool HasDominated(const Planet *planet) const;
	
	// Check to see if the player has done anything they should be fined for on a planet.
	// Each government can only fine you once per day.
	std::string Fine(PlayerInfo &player, const Government *gov, double security);
	// Fine the player when in space. This can result in multiple fines per day, but only once per ship.
	Punishment Fine(PlayerInfo &player, const Ship &ship, int scan, const Ship &target);
	
	// Get or set your reputation with the given government.
	double Reputation(const Government *gov) const;
	void AddReputation(const Government *gov, double value);
	void SetReputation(const Government *gov, double value);
	
	// Reset any temporary effects (typically because a day has passed).
	void ResetDaily();
	
	
private:
	Punishment CalculateFine(PlayerInfo &player, const Government *gov, int scan, const Ship *target);


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



#endif
