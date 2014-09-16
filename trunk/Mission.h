/* Mission.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MISSION_H_
#define MISSION_H_

#include "ConditionSet.h"
#include "Date.h"
#include "LocationFilter.h"
#include "MissionAction.h"

#include <list>
#include <map>
#include <memory>
#include <string>

class DataNode;
class DataWriter;
class Government;
class Planet;
class PlayerInfo;
class Ship;
class ShipEvent;
class UI;



// This class represents a mission that the player can take on, such as carrying
// cargo from one planet to another or defending or attacking a ship. Many of
// a mission's parameters can be randomized, either so that the same mission
// template can be reused many times, or just so the mission is not always
// exactly the same every time you replay the game.
class Mission {
public:
	// Load a mission, either from the game data or from a saved game.
	void Load(const DataNode &node);
	// Save a mission. It is safe to assume that any mission that is being saved
	// is already "instantiated," so only a subset of the data must be saved.
	void Save(DataWriter &out, const std::string &tag = "mission") const;
	
	// Basic mission information.
	const std::string &Name() const;
	const std::string &Description() const;
	// Check if this mission should be shown in your mission list. If not, the
	// player will not know this mission exists (which is sometimes useful).
	bool IsVisible() const;
	// Check if this mission is a "job" (i.e. something that should not show up
	// automatically in the spaceport, because it will instead be shown in the
	// job listing).
	bool IsJob() const;
	
	// Information about what you are doing.
	const Planet *Destination() const;
	const std::string &Cargo() const;
	int CargoSize() const;
	int CargoIllegality(const Government *government) const;
	int Passengers() const;
	// The mission must be completed by this deadline (if there is a deadline).
	bool HasDeadline() const;
	const Date &Deadline() const;
	
	// Check if it's possible to offer or complete this mission right now. The
	// check for whether you can offer a mission does not take available space
	// into account, so before actually offering a mission you should also check
	// if the player has enough space.
	bool CanOffer(const PlayerInfo &player) const;
	bool HasSpace(const PlayerInfo &player) const;
	bool CanComplete(const PlayerInfo &player) const;
	bool HasFailed() const;
	
	// When the state of this mission changes, it may make changes to the player
	// information or show new UI panels. PlayerInfo::MissionCallback() will be
	// used as the callback for any UI panel that returns a value. If it is not
	// possible for this change to happen, this function returns false.
	enum Trigger {COMPLETE, OFFER, ACCEPT, DECLINE, FAIL};
	bool Do(Trigger trigger, PlayerInfo &player, UI *ui = nullptr);
	
	// Get a list of NPCs associated with this mission. Every time the player
	// takes off from a planet, they should be added to the active ships.
	const std::list<std::shared_ptr<Ship>> &Ships() const;
	// If any event occurs between two ships, check to see if this mission cares
	// about it. This may affect the mission status or display a message.
	void Do(const ShipEvent &event, UI *ui);
	
	// "Instantiate" a mission by replacing randomly selected values and places
	// with a single choice, and then replacing any wildcard text as well.
	Mission Instantiate(const PlayerInfo &player) const;
	
	
private:
	std::string name;
	std::string description;
	
	bool hasFailed = false;
	bool isVisible = true;
	bool isJob = false;
	bool hasDeadline = false;
	bool doDefaultDeadline = false;
	Date deadline;
	int daysToDeadline = 0;
	
	int repeat = 1;
	std::string cargo;
	int cargoSize = 0;
	// Parameters for generating random cargo amounts:
	int cargoLimit = 0;
	double cargoProb = 0.;
	std::map<const Government *, int> cargoIllegality;
	int cargoBaseIllegality = 0;
	int passengers = 0;
	// Parameters for generating random passenger amounts:
	int passengerLimit = 0;
	double passengerProb = 0.;
	
	ConditionSet toOffer;
	ConditionSet toComplete;
	
	const Planet *source = nullptr;
	LocationFilter sourceFilter;
	const Planet *destination = nullptr;
	LocationFilter destinationFilter;
	
	// NPCs:
	std::list<std::shared_ptr<Ship>> ships;
	// TODO: class representing NPC characteristics.
	
	// Actions to perform:
	std::map<Trigger, MissionAction> actions;
};



#endif
