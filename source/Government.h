/* Government.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GOVERNMENT_H_
#define GOVERNMENT_H_

#include "Color.h"
#include "LocationFilter.h"

#include <map>
#include <string>
#include <vector>

class Conversation;
class DataNode;
class Fleet;
class Phrase;
class Planet;
class PlayerInfo;
class Ship;
class System;



// Class representing a government. Each ship belongs to some government, and
// attacking that ship will provoke its ally governments and reduce your
// reputation with them, but increase your reputation with that ship's enemies.
// The ships for each government are identified by drawing them with a different
// color "swizzle." Some government's ships can also be easier or harder to
// bribe than others.
class Government {
public:
	// Default constructor.
	Government();
	
	// Load a government's definition from a file.
	void Load(const DataNode &node);
	
	// Get the display name of this government.
	const std::string &GetName() const;
	// Set / Get the name used for this government in the data files.
	void SetName(const std::string &trueName);
	const std::string &GetTrueName() const;
	// Get the color swizzle to use for ships of this government.
	int GetSwizzle() const;
	// Get the color to use for displaying this government on the map.
	const Color &GetColor() const;
	
	// Get the government's initial disposition toward other governments or
	// toward the player.
	double AttitudeToward(const Government *other) const;
	double InitialPlayerReputation() const;
	// Get the amount that your reputation changes for the given offense. The
	// given value should be a combination of one or more ShipEvent values.
	double PenaltyFor(int eventType) const;
	// In order to successfully bribe this government you must pay them this
	// fraction of your fleet's value. (Zero means they cannot be bribed.)
	double GetBribeFraction() const;
	// This government will fine you the given fraction of the maximum fine for
	// carrying illegal cargo or outfits. Zero means they will not fine you.
	double GetFineFraction() const;
	// A government might not exercise the ability to perform scans or fine
	// the player in every system.
	bool CanEnforce(const System *system) const;
	bool CanEnforce(const Planet *planet) const;
	// Get the conversation that will be shown if this government gives a death
	// sentence to the player (for carrying highly illegal cargo).
	const Conversation *DeathSentence() const;
	
	// Get a hail message (which depends on whether this is an enemy government
	// and if the ship is disabled).
	std::string GetHail(bool isDisabled) const;
	// Find out if this government speaks a different language.
	const std::string &Language() const;
	// Pirate raids in this government's systems use this fleet definition. If
	// it is null, there are no pirate raids.
	const Fleet *RaidFleet() const;
	
	// Check if, according to the politics stored by GameData, this government is
	// an enemy of the given government right now.
	bool IsEnemy(const Government *other) const;
	// Check if this government is an enemy of the player.
	bool IsEnemy() const;
	
	// Below are shortcut functions which actually alter the game state in the
	// Politics object, but are provided as member functions here for clearer syntax.
	
	// Check if this is the player government.
	bool IsPlayer() const;
	// Commit the given "offense" against this government (which may not
	// actually consider it to be an offense). This may result in temporary
	// hostilities (if the even type is PROVOKE), or a permanent change to your
	// reputation.
	void Offend(int eventType, int count = 1) const;
	// Bribe this government to be friendly to you for one day.
	void Bribe() const;
	// Check to see if the player has done anything they should be fined for.
	// Each government can only fine you once per day.
	std::string Fine(PlayerInfo &player, int scan = 0, const Ship *target = nullptr, double security = 1.) const;
	
	// Get or set the player's reputation with this government.
	double Reputation() const;
	void AddReputation(double value) const;
	void SetReputation(double value) const;
	
	// Get the government's crew attack/defense values
	double CrewAttack() const;
	double CrewDefense() const;
	
	
private:
	unsigned id;
	std::string name;
	std::string displayName;
	int swizzle = 0;
	Color color;
	
	std::vector<double> attitudeToward;
	double initialPlayerReputation = 0.;
	std::map<int, double> penaltyFor;
	double bribe = 0.;
	double fine = 1.;
	std::vector<LocationFilter> enforcementZones;
	const Conversation *deathSentence = nullptr;
	const Phrase *friendlyHail = nullptr;
	const Phrase *friendlyDisabledHail = nullptr;
	const Phrase *hostileHail = nullptr;
	const Phrase *hostileDisabledHail = nullptr;
	std::string language;
	const Fleet *raidFleet = nullptr;
	double crewAttack = 1.;
	double crewDefense = 2.;
};



#endif
