/* GameObjects.h
Copyright (c) 2021 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GAME_OBJECTS_H_
#define GAME_OBJECTS_H_

#include "CategoryTypes.h"
#include "Sale.h"
#include "Set.h"
#include "Trade.h"

#include "Color.h"
#include "Conversation.h"
#include "Effect.h"
#include "Fleet.h"
#include "Galaxy.h"
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

#include <future>
#include <map>
#include <string>
#include <vector>


class Sprite;



// Class representing every game object.
class GameObjects {
public:
	Set<Color> colors;
	Set<Conversation> conversations;
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

	Trade trade;
	std::vector<StartConditions> startConditions;
	std::map<std::string, std::vector<std::string>> ratings;
	std::map<const Sprite *, std::string> landingMessages;
	std::map<const Sprite *, double> solarPower;
	std::map<const Sprite *, double> solarWind;
	std::map<CategoryType, std::vector<std::string>> categories;

	std::map<std::string, std::string> tooltips;
	std::map<std::string, std::string> helpMessages;

public:
	// Load the game objects from the given definitions.
	std::future<void> Load(const std::vector<std::string> &sources, bool debugMode = false);
	void LoadFile(const std::string &path, bool debugMode = false);
	// Apply the given change to the universe.
	void Change(const DataNode &node);
	// Update the neighbor lists and other information for all the systems.
	// This must be done any time that a change creates or moves a system.
	void UpdateSystems();

	double Progress() const;

	// Check for objects that are referred to but never defined.
	void CheckReferences();
	// Resolve every game object dependency.
	void FinishLoading();

private:
	std::atomic<double> progress;
};



#endif
