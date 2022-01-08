/* UniverseObjects.h
Copyright (c) 2021 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef UNIVERSE_OBJECTS_H_
#define UNIVERSE_OBJECTS_H_

#include "CategoryTypes.h"
#include "Sale.h"
#include "Set.h"

#include "Color.h"
#include "Conversation.h"
#include "CustomSale.h"
#include "Effect.h"
#include "Fleet.h"
#include "Galaxy.h"
#include "GameEvent.h"
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
#include "Ship.h"
#include "StartConditions.h"
#include "System.h"
#include "Test.h"
#include "TestData.h"
#include "TextReplacements.h"
#include "Trade.h"

#include <future>
#include <map>
#include <string>
#include <vector>


class Sprite;



// This class contains all active game objects, representing the current state of the Endless Sky universe.
// All pointers to game objects must refer to the same UniverseObjects instance.
class UniverseObjects {
	// GameData currently is the orchestrating controller for all game definitions.
	friend class GameData;
public:
	// Load game objects from the given directories of definitions.
	std::future<void> Load(const std::vector<std::string> &sources, bool debugMode = false);
	// Determine the fraction of data files read from disk.
	double GetProgress() const;
	// Resolve every game object dependency.
	void FinishLoading();

	// Apply the given change to the universe.
	void Change(const DataNode &node);
	// Update the neighbor lists and other information for all the systems.
	// (This must be done any time a GameEvent creates or moves a system.)
	void UpdateSystems();

	// Check for objects that are referred to but never defined.
	void CheckReferences();


private:
	void LoadFile(const std::string &path, bool debugMode = false);


private:
	// A value in [0, 1] representing how many source files have been processed for content.
	std::atomic<double> progress;


private:
	Set<Color> colors;
	Set<Conversation> conversations;
	Set<CustomSale> customSales;
	Set<Effect> effects;
	Set<GameEvent> events;
	Set<Fleet> fleets;
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
	Set<Ship> ships;
	Set<System> systems;
	Set<Test> tests;
	Set<TestData> testDataSets;
	Set<Sale<Ship>> shipSales;
	Set<Sale<Outfit>> outfitSales;
	std::set<double> neighborDistances;

	TextReplacements substitutions;
	Trade trade;
	std::vector<StartConditions> startConditions;
	std::map<std::string, std::vector<std::string>> ratings;
	std::map<const Sprite *, std::string> landingMessages;
	std::map<const Sprite *, double> solarPower;
	std::map<const Sprite *, double> solarWind;
	std::map<CategoryType, std::vector<std::string>> categories;

	std::map<std::string, std::string> tooltips;
	std::map<std::string, std::string> helpMessages;


};



#endif
