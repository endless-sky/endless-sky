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

#include <map>
#include <optional>
#include <string>

class DataNode;
class DataWriter;
class Sprite;



// Gamerules contains a list of constants and booleans that define game behavior,
// for example, the spawnrate of person ships or whether universal ramscoops are active.
class Gamerules {
public:
	// Defines which disabled fighters can dodge stray projectiles.
	enum class FighterDodgePolicy
	{
		NONE = 0,
		ONLY_PLAYER = 1,
		ALL = 2
	};


public:
	Gamerules() = default;

	// Load a gamerules node.
	void Load(const DataNode &node);
	// Save these gamerules by saving the name and any individual values that differ from the original preset.
	// By saving only the difference, newly added gamerules or modified default gamerules will be applied to
	// existing save files, but any customizations that a player made to their gamerules will remain.
	void Save(DataWriter &out, const Gamerules &preset) const;

	// Replace the name and all the rule values with those of the given gamerules.
	void Replace(const Gamerules &rules);
	// Reset a particular value to the value used by the preset.
	void Reset(const std::string &rule, const Gamerules &preset);

	const std::string &Name() const;
	const std::string &Description() const;
	const Sprite *Thumbnail() const;

	void SetLockGamerules(bool value);
	void SetUniversalRamscoopActive(bool value);
	void SetPersonSpawnPeriod(int value);
	void SetNoPersonSpawnWeight(int value);
	void SetNPCMaxMiningTime(int value);
	void SetUniversalFrugalThreshold(double value);
	void SetDepreciationMin(double value);
	void SetDepreciationDaily(double value);
	void SetDepreciationGracePeriod(int value);
	void SetDepreciationMaxAge(int value);
	void SetFighterDodgePolicy(FighterDodgePolicy value);
	void SetSystemDepartureMin(double value);
	void SetSystemArrivalMin(std::optional<double> value);
	void SetFleetMultiplier(double value);
	void SetMiscValue(const std::string &rule, int value);

	int GetValue(const std::string &rule) const;

	bool LockGamerules() const;
	bool UniversalRamscoopActive() const;
	int PersonSpawnPeriod() const;
	int NoPersonSpawnWeight() const;
	int NPCMaxMiningTime() const;
	double UniversalFrugalThreshold() const;
	double DepreciationMin() const;
	double DepreciationDaily() const;
	int DepreciationGracePeriod() const;
	int DepreciationMaxAge() const;
	FighterDodgePolicy FightersHitWhenDisabled() const;
	double SystemDepartureMin() const;
	std::optional<double> SystemArrivalMin() const;
	double FleetMultiplier() const;

	bool operator==(const Gamerules &other) const;


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
		double depreciationMin = .25;
		double depreciationDaily = .997;
		int depreciationGracePeriod = 7;
		int depreciationMaxAge = 1000;
		FighterDodgePolicy fighterHitPolicy = FighterDodgePolicy::ALL;
		double systemDepartureMin = 0.;
		std::optional<double> systemArrivalMin;
		double fleetMultiplier = 1.;

		// Miscellaneous rules that are only used by the game data and not by the engine.
		std::map<std::string, int> miscRules;
	};


private:
	std::string name;
	std::string description;
	const Sprite *thumbnail = nullptr;
	Storage storage;
};
