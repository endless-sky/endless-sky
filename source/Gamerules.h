/* Gamerules.h
Copyright (c) 2022 by Amazinite

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

#include "Setting.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>

class DataNode;
class DataWriter;
class Sprite;
class UI;



// Gamerules contains a list of constants and booleans that define game behavior,
// for example, the spawn rate of person ships or whether universal ramscoops are active.
class Gamerules {
public:
	// Defines which disabled fighters can dodge stray projectiles.
	enum class FighterDodgePolicy : int_fast8_t {
		NONE = 0,
		ONLY_PLAYER,
		ALL,
	};

	// Defines how big the player's active fleet can get.
	// If the player's fleet size is limited, the flagship does not count toward the limit.
	enum class FleetSizeLimitation : int_fast8_t {
		// No limit on how many ships the player can own.
		NONE = 0,
		// The player can only have a set number of ships active at once.
		// Carried ships do not contribute to this limit.
		SHIP_CAP,
		// The player can only command up to this many crew at once.
		// Only the base required crew of ships is taken into account.
		// Crew equivalent is used where present, such as on automata.
		CREW_CAP,
		// The player has an "administration capacity" that limits how many ships
		// they can have active, with bigger ships having a higher administrative cost.
		ADMIN_CAP,
	};

	// Whether permadeath is active and how it should behave.
	enum class PermadeathMode : int_fast8_t {
		// Permadeath is off.
		OFF = 0,
		// Your pilot gets locked when you die. Locked pilots can't have their save files loaded.
		LOCK_ON_DEATH,
		// Your pilot gets completely deleted when you die.
		DELETE_ON_DEATH,
		// Your pilot gets locked the moment you take off from a planet.
		LOCK_ON_TAKEOFF,
		// Your pilot gets completely deleted the moment you take off from a planet.
		DELETE_ON_TAKEOFF,
	};

	// Defines how the value from a captured ship is shared with the crew.
	enum class CaptureProfitSharing : int_fast8_t {
		OFF = 0,
		// Death benefits must be paid to the family of crew members who died when capturing a ship.
		DEAD_CREW,
		// Surviving crew members get a share of the value of captured ships.
		SURVIVING_CREW,
		// Both surviving crew members and the families of dead crew members get a share of captured ship value.
		ALL_CREW,
	};


public:
	// Identifiers for specific gamerules.
	static const std::string LOCK_GAMERULES;
	static const std::string UNIVERSAL_RAMSCOOP;
	static const std::string PERSON_SPAWN_PERIOD;
	static const std::string NO_PERSON_SPAWN_WEIGHT;
	static const std::string NPC_MAX_MINING_TIME;
	static const std::string UNIVERSAL_FRUGAL_THRESHOLD;
	static const std::string DEPRECIATIVE_AGING;
	static const std::string DEPRECIATION_MIN;
	static const std::string DEPRECIATION_CAPTURED;
	static const std::string DEPRECIATION_PLUNDERED;
	static const std::string DEPRECIATION_CARGO_IS_PLUNDER;
	static const std::string DEPRECIATION_DAILY;
	static const std::string DEPRECIATION_GRACE_PERIOD;
	static const std::string DEPRECIATION_MAX_AGE;
	static const std::string FIGHTER_DODGE_POLICY;
	static const std::string SYSTEM_DEPARTURE_MIN;
	static const std::string SYSTEM_ARRIVAL_MIN;
	static const std::string HABITABLE_BASED_ARRIVAL_DISTANCE;
	static const std::string HABITABLE_ARRIVAL_MIN;
	static const std::string HABITABLE_ARRIVAL_MAX;
	static const std::string FLEET_MULTIPLIER;
	static const std::string RAID_FLEET_ROLL_CHANCE;
	static const std::string RAID_ATTRACTION_MULTIPLIER;
	static const std::string RAID_DETERRENCE_MULTIPLIER;
	static const std::string RAID_FLEET_SPAWN_ATTEMPTS;
	static const std::string FLEET_SIZE_LIMITATION;
	static const std::string DEFAULT_MAX_ESCORT_COUNT;
	static const std::string DEFAULT_MAX_ESCORT_CREW;
	static const std::string DEFAULT_ADMIN_CAP;
	static const std::string PERMADEATH_MODE;
	static const std::string SINGLE_SAVE_FILE;
	static const std::string RESTRICTED_SAVE_LOADING;
	static const std::string UNIVERSAL_AMMO_RESTOCKING;
	static const std::string CAPTURE_PROFIT_SHARING;
	static const std::string CAPTURE_CAPTAIN_SHARES;
	static const std::string CAPTURE_DEAD_CREW_SHARES;
	static const std::string CAPTURE_SURVIVING_CREW_SHARES;
	static const std::string CAPTURE_MIN_DEATH_BENEFIT;
	static const std::string CAPTURE_VALUE_BASIS;


public:
	Gamerules();
	// Don't allow copying this class.
	Gamerules(const Gamerules &) = delete;
	Gamerules &operator=(const Gamerules &) = delete;
	Gamerules(Gamerules &&) = delete;
	Gamerules &operator=(Gamerules &&) = default;
	~Gamerules() noexcept = default;

	// Load a gamerules node.
	void Load(const DataNode &node);
	// Save these gamerules by saving the name and any individual values that differ from the original preset.
	// By saving only the difference, newly added gamerules or modified default gamerules will be applied to
	// existing save files, but any customizations that a player made to their gamerules will remain.
	void Save(DataWriter &out, const Gamerules &preset) const;

	// Toggle a rule given its identifier.
	void Toggle(const std::string &rule, UI *ui = nullptr);
	// Given a rule's identifier, return its display name in the UI.
	const std::string &DisplayName(const std::string &rule) const;
	// Given a rule's identifier, return its current display value and whether it appears as on or off.
	std::pair<std::string, bool> DisplayValue(const std::string &rule) const;
	// Replace the name and all the rule values with those of the given gamerules.
	void Replace(const Gamerules &rules);
	// Reset a particular rule to the value used by the preset.
	void Reset(const std::string &rule, const Gamerules &preset);

	const std::string &Name() const;
	const std::string &Description() const;
	const Sprite *Thumbnail() const;

	void SetLockGamerules(bool value);
	void SetMiscValue(const std::string &rule, int value);
	int GetValue(const std::string &rule) const;

	bool LockGamerules() const;
	bool UniversalRamscoopActive() const;
	int PersonSpawnPeriod() const;
	int NoPersonSpawnWeight() const;
	int NPCMaxMiningTime() const;
	double UniversalFrugalThreshold() const;
	bool DepreciationAgingActive() const;
	double DepreciationMin() const;
	double DepreciationPlundered() const;
	double DepreciationCaptured() const;
	double DepreciationDaily() const;
	int DepreciationGracePeriod() const;
	int DepreciationMaxAge() const;
	bool DepreciationCargoIsPlunder() const;
	FighterDodgePolicy FightersHitWhenDisabled() const;
	double SystemDepartureMin() const;
	std::optional<double> SystemArrivalMin() const;
	bool HabitableBasedArrivalDistance() const;
	std::optional<double> HabitableArrivalMin() const;
	std::optional<double> HabitableArrivalMax() const;
	double FleetMultiplier() const;
	double RaidFleetRollChance() const;
	double RaidAttractionMultiplier() const;
	double RaidDeterrenceMultiplier() const;
	int RaidFleetSpawnAttempts() const;
	FleetSizeLimitation GetFleetSizeLimitation() const;
	int GetDefaultMaxEscortCount() const;
	int GetDefaultMaxEscortCrew() const;
	int GetDefaultAdminCap() const;
	PermadeathMode GetPermadeathMode() const;
	bool SingleSaveFile() const;
	bool RestrictedSaveLoading() const;
	CaptureProfitSharing GetCaptureProfitSharing() const;
	double CaptureCaptainShares() const;
	double CaptureDeadCrewShares() const;
	double CaptureSurvivingCrewShares() const;
	int CaptureMinimumDeathBenefit() const;
	std::optional<double> CaptureValueBasis() const;

	bool operator==(const Gamerules &other) const;


private:
	// Sync the display settings with the underlying gamerule values.
	void SyncSettings();


private:
	// Rule values are stored in this Storage class for easy replacement
	// and comparison via the default equality operator.
	class Storage {
	public:
		bool operator==(const Storage &other) const = default;

	public:
		bool lockGamerules = true;
		bool universalRamscoop = true;
		int personSpawnPeriod = 36000;
		int noPersonSpawnWeight = 1000;
		int npcMaxMiningTime = 3600;
		double universalFrugalThreshold = .75;
		bool depreciationAgingActive = true;
		double depreciationMin = .25;
		double depreciationPlundered = .25;
		double depreciationCaptured = .25;
		double depreciationDaily = .997;
		bool depreciationCargoIsPlunder = false;
		int depreciationGracePeriod = 7;
		int depreciationMaxAge = 1000;
		FighterDodgePolicy fighterHitPolicy = FighterDodgePolicy::ALL;
		double systemDepartureMin = 0.;
		std::optional<double> systemArrivalMin;
		bool habitableBasedArrivalDistance = true;
		std::optional<double> habitableArrivalMin = 500.;
		std::optional<double> habitableArrivalMax = 5000.;
		double fleetMultiplier = 1.;
		double raidFleetRollChance = 1.;
		double raidAttractionMultiplier = 1.;
		double raidDeterrenceMultiplier = 1.;
		int raidFleetSpawnAttempts = 10;
		FleetSizeLimitation fleetSizeLimitation = FleetSizeLimitation::NONE;
		// The player can only have up to this many escorts active at once. Carried ships are excluded.
		// This value can be increased for a pilot through gameplay.
		int defaultMaxEscortCount = 6;
		// The player can only have escorts whose total base required crew (+ crew equivalent) is less than this value.
		// This value can be increased for a pilot through gameplay.
		int defaultMaxEscortCrew = 1000;
		// The administrative capacity of the player's fleet. Different ships will have different admin costs.
		// This value can be increased for a pilot through gameplay.
		int defaultAdminCap = 25;
		PermadeathMode permadeathMode = PermadeathMode::OFF;
		bool singleSaveFile = false;
		bool restrictedSaveLoading = false;
		CaptureProfitSharing captureProfitSharing = CaptureProfitSharing::OFF;
		double captureCaptainShares = 2.;
		double captureDeadCrewShares = 1.;
		double captureSurvivingCrewShares = 1.;
		int captureMinimumDeathBenefit = 100000;
		std::optional<double> captureValueBasis;

		// Miscellaneous rules that are only used by the game data and not by the engine.
		std::map<std::string, int> miscRules;
	};


private:
	std::string name;
	std::string description;
	const Sprite *thumbnail = nullptr;
	Storage storage;
	std::map<std::string, Setting> settings;
};
