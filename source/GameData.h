/* GameData.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GAME_DATA_H_
#define GAME_DATA_H_

#include "Set.h"
#include "Trade.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class Color;
class Conversation;
class DataNode;
class DataWriter;
class Date;
class Effect;
class Fleet;
class Galaxy;
class GameEvent;
class Government;
class ImageSet;
class Interface;
class Minable;
class Mission;
class News;
class Outfit;
class Person;
class Phrase;
class Planet;
class Politics;
class Ship;
class Sprite;
class StarField;
class StartConditions;
class System;



// Class storing all the data used in the game: sprites, data files, etc. This
// data is globally accessible, but can only be modified in certain ways.
// Events that occur over the course of the game may change the state of the
// game data, so we must revert to the initial state when loading a new player
// and then apply whatever changes have happened in that particular player's
// universe.
class GameData {
public:
	static bool BeginLoad(const char * const *argv);
	// Check for objects that are referred to but never defined.
	static void CheckReferences();
	static void LoadShaders();
	static double Progress();
	// Begin loading a sprite that was previously deferred. Currently this is
	// done with all landscapes to speed up the program's startup.
	static void Preload(const Sprite *sprite);
	static void FinishLoading();
	
	// Get the list of resource sources (i.e. plugin folders).
	static const std::vector<std::string> &Sources();
	
	// Revert any changes that have been made to the universe.
	static void Revert();
	static void SetDate(const Date &date);
	// Functions for the dynamic economy.
	static void ReadEconomy(const DataNode &node);
	static void WriteEconomy(DataWriter &out);
	static void StepEconomy();
	static void AddPurchase(const System &system, const std::string &commodity, int tons);
	// Apply the given change to the universe.
	static void Change(const DataNode &node);
	// Update the neighbor lists of all the systems. This must be done any time
	// that a change creates or moves a system.
	static void UpdateNeighbors();
	
	// Re-activate any special persons that were created previously but that are
	// still alive.
	static void ResetPersons();
	// Mark all persons in the given list as dead.
	static void DestroyPersons(std::vector<std::string> &names);
	
	static const Set<Color> &Colors();
	static const Set<Conversation> &Conversations();
	static const Set<Effect> &Effects();
	static const Set<GameEvent> &Events();
	static const Set<Fleet> &Fleets();
	static const Set<Galaxy> &Galaxies();
	static const Set<Government> &Governments();
	static const Set<Interface> &Interfaces();
	static const Set<Minable> &Minables();
	static const Set<Mission> &Missions();
	static const Set<Outfit> &Outfits();
	static const Set<Person> &Persons();
	static const Set<Phrase> &Phrases();
	static const Set<Planet> &Planets();
	static const Set<Ship> &Ships();
	static const Set<System> &Systems();
	
	static const Government *PlayerGovernment();
	static Politics &GetPolitics();
	static const StartConditions &Start();
	
	static const std::vector<Trade::Commodity> &Commodities();
	static const std::vector<Trade::Commodity> &SpecialCommodities();
	
	// Custom messages to be shown when trying to land on certain stellar objects.
	static bool HasLandingMessage(const Sprite *sprite);
	static const std::string &LandingMessage(const Sprite *sprite);
	// Get the solar power and wind output of the given stellar object sprite.
	static double SolarPower(const Sprite *sprite);
	static double SolarWind(const Sprite *sprite);
	
	// Pick a random news object that applies to the given planet. If there is
	// no applicable news, this returns null.
	static const News *PickNews(const Planet *planet);
	
	// Strings for combat rating levels, etc.
	static const std::string &Rating(const std::string &type, int level);
	static const std::vector<std::string> &Categories(const std::string &type);
	
	static const StarField &Background();
	static void SetHaze(const Sprite *sprite);
	
	static const std::string &Tooltip(const std::string &label);
	static std::string HelpMessage(const std::string &name);
	static const std::map<std::string, std::string> &HelpTemplates();
	
	static const std::map<std::string, std::string> &PluginAboutText();
	
	
private:
	static void LoadSources();
	static void LoadFile(const std::string &path, bool debugMode);
	static std::map<std::string, std::shared_ptr<ImageSet>> FindImages();
	
	static void PrintShipTable();
	static void PrintWeaponTable();
};



#endif
