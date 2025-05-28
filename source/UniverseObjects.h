/* UniverseObjects.h
Copyright (c) 2021 by Michael Zahniser

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

#include "CategoryType.h"
#include "Set.h"
#include "Shop.h"

#include "CategoryList.h"
#include "Color.h"
#include "Conversation.h"
#include "Effect.h"
#include "Fleet.h"
#include "FormationPattern.h"
#include "Galaxy.h"
#include "GameEvent.h"
#include "Gamerules.h"
#include "Government.h"
#include "Hazard.h"
#include "Interface.h"
#include "Minable.h"
#include "Mission.h"
#include "News.h"
#include "Outfit.h"
#include "Person.h"
#include "Phrase.h"
#include "Planet.h"
#include "shader/Shader.h"
#include "Ship.h"
#include "StartConditions.h"
#include "Swizzle.h"
#include "System.h"
#include "test/Test.h"
#include "test/TestData.h"
#include "TextReplacements.h"
#include "Trade.h"
#include "Wormhole.h"

#include <atomic>
#include <filesystem>
#include <future>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

class ConditionsStore;
class Panel;
class PlayerInfo;
class Sprite;
class TaskQueue;



// This class contains all active game objects, representing the current state of the Endless Sky universe.
// All pointers to game objects must refer to the same UniverseObjects instance.
class UniverseObjects {
	// GameData currently is the orchestrating controller for all game definitions.
	friend class GameData;
	friend class TestData;
public:
	// Load game objects from the given directories of definitions.
	std::shared_future<void> Load(TaskQueue &queue, const std::vector<std::filesystem::path> &sources,
		const PlayerInfo &player, const ConditionsStore *globalConditions, bool debugMode = false);
	// Determine the fraction of data files read from disk.
	double GetProgress() const;
	// Resolve every game object dependency.
	void FinishLoading();

	// Apply the given change to the universe.
	void Change(const DataNode &node, const PlayerInfo &player);
	// Update the neighbor lists and other information for all the systems.
	// (This must be done any time a GameEvent creates or moves a system.)
	void UpdateSystems();

	// Check for objects that are referred to but never defined.
	void CheckReferences();

	// Draws the current menu background. Unlike accessing the menu background
	// through GameData, this function is thread-safe.
	void DrawMenuBackground(Panel *panel) const;

	// Modifiers for the stored data.
	void AddJumpRange(double neighborDistance);
	void DestroyPersons(const std::vector<std::string> &names);
	void ResetPersons();
	void SetDate(const Date &date);

	// Accessors for the stored data.
	const Set<Color> &Colors() const;
	const std::vector<Trade::Commodity> &Commodities() const;
	const Set<Conversation> &Conversations() const;
	const Set<Effect> &Effects() const;
	const Set<GameEvent> &Events() const;
	const Set<Fleet> &Fleets() const;
	const Set<FormationPattern> &Formations() const;
	const Set<Galaxy> &Galaxies() const;
	const CategoryList &GetCategory(CategoryType type);
	const Gamerules &GetGamerules() const;
	const TextReplacements &GetTextReplacements() const;
	const Set<Government> &Governments() const;
	bool HasLandingMessage(const Sprite *sprite) const;
	const Set<Hazard> &Hazards() const;
	const std::string HelpMessage(const std::string &name) const;
	const std::map<std::string, std::string> &HelpTemplates() const;
	const Set<Interface> &Interfaces() const;
	const std::string &LandingMessage(const Sprite *sprite) const;
	const Set<Minable> &Minables() const;
	const Set<Mission> &Missions() const;
	const Set<Outfit> &Outfits() const;
	const Set<Shop<Outfit>> &Outfitters() const;
	const Set<Person> &Persons() const;
	const Set<Phrase> &Phrases() const;
	const Set<Planet> &Planets() const;
	const std::string &Rating(const std::string &type, int level) const;
	const Set<Shader> &Shaders() const;
	const Set<Ship> &Ships() const;
	const Set<Shop<Ship>> &Shipyards() const;
	double SolarPower(const Sprite *sprite) const;
	double SolarWind(const Sprite *sprite) const;
	const Set<News> &SpaceportNews() const;
	const std::vector<Trade::Commodity> &SpecialCommodities() const;
	const Sprite *StarIcon(const Sprite *sprite) const;
	const std::vector<StartConditions> &StartOptions() const;
	const Set<Swizzle> &Swizzles() const;
	const Set<System> &Systems() const;
	const Set<TestData> &TestDataSets() const;
	const Set<Test> &Tests() const;
	const std::string &Tooltip(const std::string &label) const;
	const Set<Wormhole> &Wormholes() const;


private:
	void LoadFile(const std::filesystem::path &path, const PlayerInfo &player,
		const ConditionsStore *globalConditions, bool debugMode = false);


private:
	// A value in [0, 1] representing how many source files have been processed for content.
	std::atomic<double> progress;


private:
	Set<Color> colors;
	Set<Swizzle> swizzles;
	Set<Conversation> conversations;
	Set<Effect> effects;
	Set<GameEvent> events;
	Set<Fleet> fleets;
	Set<FormationPattern> formations;
	Set<Galaxy> galaxies;
	Set<Government> governments;
	Set<Hazard> hazards;
	Set<Interface> interfaces;
	Set<Minable> minables;
	Set<Mission> missions;
	Set<News> news;
	Set<Outfit> outfits;
	Set<Person> persons;
	Set<Phrase> phrases;
	Set<Planet> planets;
	Set<Shader> shaders;
	Set<Ship> ships;
	Set<System> systems;
	Set<Test> tests;
	Set<TestData> testDataSets;
	Set<Shop<Ship>> shipSales;
	Set<Shop<Outfit>> outfitSales;
	Set<Wormhole> wormholes;
	std::set<double> neighborDistances;

	Gamerules gamerules;
	TextReplacements substitutions;
	Trade trade;
	std::vector<StartConditions> startConditions;
	std::map<std::string, std::vector<std::string>> ratings;
	std::map<const Sprite *, std::string> landingMessages;
	std::map<const Sprite *, double> solarPower;
	std::map<const Sprite *, double> solarWind;
	std::map<const Sprite *, const Sprite *> starIcons;
	std::map<CategoryType, CategoryList> categories;

	std::map<std::string, std::string> tooltips;
	std::map<std::string, std::string> helpMessages;
	std::map<std::string, std::set<std::string>> disabled;

	// A local cache of the menu background interface for thread-safe access.
	mutable std::mutex menuBackgroundMutex;
	Interface menuBackgroundCache;
};
