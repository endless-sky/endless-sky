/* Government.h
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

#include "Color.h"
#include "ExclusiveItem.h"
#include "LocationFilter.h"
#include "RaidFleet.h"
#include "Swizzle.h"

#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

class Conversation;
class DataNode;
class Fleet;
class Outfit;
class Phrase;
class Planet;
class PlayerInfo;
class Ship;
class System;



// Class representing a government. Each ship belongs to some government, and
// attacking that ship will provoke its allied governments and reduce your
// reputation with them, but increase your reputation with that ship's enemies.
// The ships for each government are identified by drawing them with a different
// color "swizzle." Some government's ships can also be easier or harder to
// bribe than others.
class Government {
public:
	struct Atrocity {
		// False if a global atrocity is ignored by this government.
		bool isAtrocity = true;
		const Conversation *customDeathSentence = nullptr;
	};

	enum class SpecialPenalty : int {
		NONE = 0,
		PROVOKE,
		ATROCITY
	};

	class PenaltyEffect {
	public:
		PenaltyEffect(double reputationChange = 0., SpecialPenalty specialPenalty = SpecialPenalty::NONE)
			: reputationChange(reputationChange), specialPenalty(specialPenalty)
		{}

		double reputationChange;
		SpecialPenalty specialPenalty;
	};


public:
	// Default constructor.
	Government();

	// Load a government's definition from a file.
	void Load(const DataNode &node, const std::set<const System *> *visitedSystems,
		const std::set<const Planet *> *visitedPlanets);

	// Get the display name of this government.
	const std::string &DisplayName() const;
	// Set / Get the true name used for this government in the data files.
	void SetTrueName(const std::string &trueName);
	const std::string &TrueName() const;
	// Get the color swizzle to use for ships of this government.
	const Swizzle *GetSwizzle() const;
	// Get the color to use for displaying this government on the map.
	const Color &GetColor() const;

	// Get the government's initial disposition toward other governments or
	// toward the player.
	double AttitudeToward(const Government *other) const;
	double InitialPlayerReputation() const;
	// Get the amount that your reputation changes for the given offense against the given government.
	// The given value should be a combination of one or more ShipEvent values.
	// Returns 0 if the Government is null.
	PenaltyEffect PenaltyFor(int eventType, const Government *other) const;
	// In order to successfully bribe this government you must pay them this
	// fraction of your fleet's value. (Zero means they cannot be bribed.)
	double GetBribeFraction() const;
	// This government will never accept a bribe if the player's reputation
	// with them is below this value, if it is negative. If the value is 0,
	// bribes are accepted regardless of reputation.
	double GetBribeThreshold() const;
	// This government will fine you the given fraction of the maximum fine for
	// carrying illegal cargo or outfits. Zero means they will not fine you.
	double GetFineFraction() const;
	bool Trusts(const Government *other) const;
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
	// Get a hail message that the government responds with when accepting or rejecting a bribe.
	std::string GetShipBribeAcceptanceHail() const;
	std::string GetShipBribeRejectionHail() const;
	std::string GetPlanetBribeAcceptanceHail() const;
	std::string GetPlanetBribeRejectionHail() const;

	// Find out if this government speaks a different language.
	const std::string &Language() const;
	// Find out if this government should send custom hails even if the player does not know its language.
	bool SendUntranslatedHails() const;
	// Pirate raids in this government's systems use these fleet definitions. If
	// it is empty, there are no pirate raids.
	// The second attribute denotes the minimal and maximal attraction required for the fleet to appear.
	const std::vector<RaidFleet> &RaidFleets() const;

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
	std::pair<const Conversation *, std::string> Fine(PlayerInfo &player, int scan = 0,
		const Ship *target = nullptr, double security = 1.) const;
	// Check to see if the items are condemnable (atrocities) or warrant a fine.
	Atrocity Condemns(const Outfit *outfit) const;
	Atrocity Condemns(const Ship *ship) const;
	bool IgnoresUniversalAtrocities() const;
	// Returns the fine for given item for this government.
	int Fines(const Outfit *outfit) const;
	int Fines(const Ship *ship) const;
	bool IgnoresUniversalIllegals() const;
	// Check if given ship has illegal outfits or cargo.
	bool FinesContents(const Ship *ship) const;

	// Get or set the player's reputation with this government.
	double Reputation() const;
	double ReputationMax() const;
	double ReputationMin() const;
	void AddReputation(double value) const;
	void SetReputation(double value) const;

	// Get the government's crew attack/defense values
	double CrewAttack() const;
	double CrewDefense() const;

	bool IsProvokedOnScan() const;

	// Determine if ships from this government can travel to the given system or planet.
	bool IsRestrictedFrom(const System &system) const;
	bool IsRestrictedFrom(const Planet &planet) const;


private:
	unsigned id;
	std::string trueName;
	std::string displayName;
	const Swizzle *swizzle = Swizzle::None();
	ExclusiveItem<Color> color;

	std::vector<double> attitudeToward;
	double defaultAttitude = 0.;
	std::set<const Government *> trusted;
	std::map<unsigned, std::map<int, PenaltyEffect>> customPenalties;
	double initialPlayerReputation = 0.;
	double reputationMax = std::numeric_limits<double>::max();
	double reputationMin = std::numeric_limits<double>::lowest();
	std::map<int, PenaltyEffect> penaltyFor;
	std::map<const Outfit *, int> illegalOutfits;
	std::map<std::string, int> illegalShips;
	bool ignoreUniversalIllegals = false;
	std::map<const Outfit *, Atrocity> atrocityOutfits;
	std::map<std::string, Atrocity> atrocityShips;
	bool ignoreUniversalAtrocities = false;
	double bribe = 0.;
	double bribeThreshold = 0.;
	double fine = 1.;
	std::vector<LocationFilter> enforcementZones;
	LocationFilter travelRestrictions;
	const Conversation *deathSentence = nullptr;
	const Phrase *friendlyHail = nullptr;
	const Phrase *friendlyDisabledHail = nullptr;
	const Phrase *hostileHail = nullptr;
	const Phrase *hostileDisabledHail = nullptr;
	const Phrase *shipBribeAcceptanceHail = nullptr;
	const Phrase *shipBribeRejectionHail = nullptr;
	const Phrase *planetBribeAcceptanceHail = nullptr;
	const Phrase *planetBribeRejectionHail = nullptr;
	std::string language;
	bool sendUntranslatedHails = false;
	std::vector<RaidFleet> raidFleets;
	double crewAttack = 1.;
	double crewDefense = 2.;
	bool provokedOnScan = false;
	// If a government appears in this set, and the reputation with this government is affected by actions,
	// and events performed against that government, use the penalties that government applies for the
	// action instead of this government's own penalties.
	std::set<unsigned> useForeignPenaltiesFor;
};
