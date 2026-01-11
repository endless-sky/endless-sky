/* GameData.h
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

#include "CategoryType.h"
#include "Message.h"
#include "Set.h"
#include "Shop.h"
#include "Swizzle.h"
#include "Trade.h"

#include <filesystem>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

class CategoryList;
class Color;
class ConditionsStore;
class Conversation;
class DataNode;
class DataWriter;
class Date;
class Effect;
class Fleet;
class FormationPattern;
class Galaxy;
class GameEvent;
class Gamerules;
class Government;
class Hazard;
class ImageSet;
class Interface;
class MaskManager;
class Minable;
class Mission;
class News;
class Outfit;
class Panel;
class Person;
class Phrase;
class Planet;
class PlayerInfo;
class Point;
class Politics;
class Shader;
class Ship;
class Sprite;
class StarField;
class StartConditions;
class System;
class TaskQueue;
class Test;
class TestData;
class TextReplacements;
class UniverseObjects;
class Wormhole;



// Class storing all the data used in the game: sprites, data files, etc. This
// data is globally accessible, but can only be modified in certain ways.
// Events that occur over the course of the game may change the state of the
// game data, so we must revert to the initial state when loading a new player
// and then apply whatever changes have happened in that particular player's
// universe.
class GameData {
public:
	static std::shared_future<void> BeginLoad(TaskQueue &queue, const PlayerInfo &player,
		bool onlyLoadData, bool debugMode, bool preventUpload);
	static void FinishLoading();
	// Check for objects that are referred to but never defined.
	static void CheckReferences();
	static void LoadSettings();
	static void LoadShaders();
	static double GetProgress();
	// Whether initial game loading is complete (data, sprites and audio are loaded).
	static bool IsLoaded();
	// Begin loading a sprite that was previously deferred. Currently this is
	// done with all landscapes to speed up the program's startup.
	static void Preload(TaskQueue &queue, const Sprite *sprite);

	// Get the list of resource sources (i.e. plugin folders).
	static const std::vector<std::filesystem::path> &Sources();

	// Get a reference to the UniverseObjects object.
	static UniverseObjects &Objects();

	// Revert any changes that have been made to the universe.
	static void Revert();
	static void SetDate(const Date &date);
	// Functions for the dynamic economy.
	static void ReadEconomy(const DataNode &node);
	static void WriteEconomy(DataWriter &out);
	static void StepEconomy();
	static void AddPurchase(const System &system, const std::string &commodity, int tons);
	// Apply the given change to the universe.
	static void Change(const DataNode &node, PlayerInfo &player);
	// Update the neighbor lists and other information for all the systems.
	// This must be done any time that a change creates or moves a system.
	static void UpdateSystems();
	static void RecomputeWormholeRequirements();
	static void AddJumpRange(double neighborDistance);

	// Re-activate any special persons that were created previously but that are
	// still alive.
	static void ResetPersons();
	// Mark all persons in the given list as dead.
	static void DestroyPersons(std::vector<std::string> &names);

	static const Set<Color> &Colors();
	static const Set<Swizzle> &Swizzles();
	static const Set<Conversation> &Conversations();
	static const Set<Effect> &Effects();
	static const Set<GameEvent> &Events();
	static const Set<Fleet> &Fleets();
	static const Set<FormationPattern> &Formations();
	static const Set<Galaxy> &Galaxies();
	static const Set<Government> &Governments();
	static const Set<Hazard> &Hazards();
	static const Set<Interface> &Interfaces();
	static const Set<Message::Category> &MessageCategories();
	static const Set<Message> &Messages();
	static const Set<Minable> &Minables();
	static const Set<Mission> &Missions();
	static const Set<News> &SpaceportNews();
	static const Set<Outfit> &Outfits();
	static const Set<Shop<Outfit>> &Outfitters();
	static const Set<Person> &Persons();
	static const Set<Phrase> &Phrases();
	static const Set<Planet> &Planets();
	static const Set<Shader> &Shaders();
	static const Set<Ship> &Ships();
	static const Set<Shop<Ship>> &Shipyards();
	static const Set<System> &Systems();
	static const Set<Test> &Tests();
	static const Set<TestData> &TestDataSets();
	static const Set<Wormhole> &Wormholes();

	static const std::set<std::string> &UniverseWormholeRequirements();

	static ConditionsStore &GlobalConditions();

	static const Government *PlayerGovernment();
	static Politics &GetPolitics();
	static const std::vector<StartConditions> &StartOptions();

	static const std::vector<Trade::Commodity> &Commodities();
	static const std::vector<Trade::Commodity> &SpecialCommodities();

	// Custom messages to be shown when trying to land on certain stellar objects.
	static bool HasLandingMessage(const Sprite *sprite);
	static const std::string &LandingMessage(const Sprite *sprite);
	// Get the solar power and wind output of the given stellar object sprite.
	static double SolarPower(const Sprite *sprite);
	static double SolarWind(const Sprite *sprite);
	static const Sprite *StarIcon(const Sprite *sprite);

	// Strings for combat rating levels, etc.
	static const std::string &Rating(const std::string &type, int level);
	// Collections for ship, bay type, outfit, and other categories.
	static const CategoryList &GetCategory(const CategoryType type);

	static const StarField &Background();
	static void StepBackground(const Point &vel, double zoom = 1.);
	static const Point &GetBackgroundPosition();
	static void SetBackgroundPosition(const Point &position);
	static void SetHaze(const Sprite *sprite, bool allowAnimation);

	static const std::string &Tooltip(const std::string &label);
	static std::string HelpMessage(const std::string &name);
	static const std::map<std::string, std::string> &HelpTemplates();

	static MaskManager &GetMaskManager();

	static const TextReplacements &GetTextReplacements();

	static const Gamerules &GetGamerules();

	// Thread-safe way to draw the menu background.
	static void DrawMenuBackground(Panel *panel);


private:
	static void LoadSources(TaskQueue &queue);
	static std::map<std::string, std::shared_ptr<ImageSet>> FindImages();
};
