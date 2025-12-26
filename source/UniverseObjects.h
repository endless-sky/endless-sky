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
#include "Message.h"
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
	void Change(const DataNode &node, PlayerInfo &player);
	// Update the neighbor lists and other information for all the systems.
	// (This must be done any time a GameEvent creates or moves a system.)
	void UpdateSystems();
	// Determine which attributes may be required in order to use a wormhole.
	void RecomputeWormholeRequirements();

	// Check for objects that are referred to but never defined.
	void CheckReferences();

	// Draws the current menu background. Unlike accessing the menu background
	// through GameData, this function is thread-safe.
	void DrawMenuBackground(Panel *panel) const;


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
	Set<Message::Category> messageCategories;
	Set<Message> messages;
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

	// This is used for speeding up the route calculations.
	std::set<std::string> universeWormholeRequirements;
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
