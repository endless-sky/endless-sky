/* GameData.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameData.h"

#include "Audio.h"
#include "BatchShader.h"
#include "Color.h"
#include "Command.h"
#include "Conversation.h"
#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Effect.h"
#include "Files.h"
#include "FillShader.h"
#include "Fleet.h"
#include "FogShader.h"
#include "text/FontSet.h"
#include "Galaxy.h"
#include "GameEvent.h"
#include "GameObjects.h"
#include "Government.h"
#include "Hazard.h"
#include "ImageSet.h"
#include "Interface.h"
#include "LineShader.h"
#include "Minable.h"
#include "Mission.h"
#include "Music.h"
#include "News.h"
#include "Outfit.h"
#include "OutlineShader.h"
#include "Person.h"
#include "Phrase.h"
#include "Planet.h"
#include "PointerShader.h"
#include "Politics.h"
#include "Random.h"
#include "Render.h"
#include "RingShader.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteQueue.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StarField.h"
#include "StartConditions.h"
#include "System.h"
#include "Test.h"
#include "TestData.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <utility>
#include <vector>

class Sprite;

using namespace std;

namespace {
	GameObjects *objects;
	set<double> neighborDistances;
	
	Set<Fleet> defaultFleets;
	Set<Government> defaultGovernments;
	Set<Planet> defaultPlanets;
	Set<System> defaultSystems;
	Set<Galaxy> defaultGalaxies;
	Set<Sale<Ship>> defaultShipSales;
	Set<Sale<Outfit>> defaultOutfitSales;
	
	Politics politics;
	
	const Government *playerGovernment = nullptr;
	
	map<const System *, map<string, int>> purchases;
	
	map<string, string> plugins;
	vector<string> sources;
}



bool GameData::BeginLoad(const char * const *argv)
{
	bool printShips = false;
	bool printTests = false;
	bool printWeapons = false;
	bool debugMode = false;
	for(const char * const *it = argv + 1; *it; ++it)
	{
		if((*it)[0] == '-')
		{
			string arg = *it;
			if(arg == "-s" || arg == "--ships")
				printShips = true;
			if(arg == "-w" || arg == "--weapons")
				printWeapons = true;
			if(arg == "--tests")
				printTests = true;
			if(arg == "-d" || arg == "--debug")
				debugMode = true;
			continue;
		}
	}
	Files::Init(argv);
	
	// Initialize the list of "source" folders based on any active plugins.
	LoadSources();
	
	Render::Load();
	
	for(const string &source : sources)
	{
		// Iterate through the paths starting with the last directory given. That
		// is, things in folders near the start of the path have the ability to
		// override things in folders later in the path.
		vector<string> dataFiles = Files::RecursiveList(source + "data/");
		for(const string &path : dataFiles)
		{
			// This is an ordinary file. Check to see if it is an image.
			if(path.length() < 4 || path.compare(path.length() - 4, 4, ".txt"))
				continue;

			if(debugMode)
				Files::LogError("Parsing: " + path);
			objects->Load(DataFile(path));
		}
	}
	
	// Now that all data is loaded, update the neighbor lists and other
	// system information. Make sure that the default jump range is among the
	// neighbor distances to be updated.
	AddJumpRange(System::DEFAULT_NEIGHBOR_DISTANCE);
	UpdateSystems();

	objects->FinishLoading();
	
	// Store the current state, to revert back to later.
	defaultFleets = objects->fleets;
	defaultGovernments = objects->governments;
	defaultPlanets = objects->planets;
	defaultSystems = objects->systems;
	defaultGalaxies = objects->galaxies;
	defaultShipSales = objects->shipSales;
	defaultOutfitSales = objects->outfitSales;
	playerGovernment = objects->governments.Get("Escort");
	
	politics.Reset();
	
	if(printShips)
		PrintShipTable();
	if(printTests)
		PrintTestsTable();
	if(printWeapons)
		PrintWeaponTable();
	return !(printShips || printWeapons || printTests);
}



// Get the list of resource sources (i.e. plugin folders).
const vector<string> &GameData::Sources()
{
	return sources;
}



// Get the list of game objects.
GameObjects &GameData::Objects()
{
	return *objects;
}



void GameData::SetObjects(GameObjects &objects)
{
	::objects = &objects;
}



// Revert any changes that have been made to the universe.
void GameData::Revert()
{
	objects->fleets.Revert(defaultFleets);
	objects->governments.Revert(defaultGovernments);
	objects->planets.Revert(defaultPlanets);
	objects->systems.Revert(defaultSystems);
	objects->galaxies.Revert(defaultGalaxies);
	objects->shipSales.Revert(defaultShipSales);
	objects->outfitSales.Revert(defaultOutfitSales);
	for(auto &it : objects->persons)
		it.second.Restore();
	
	politics.Reset();
	purchases.clear();
}



void GameData::SetDate(const Date &date)
{
	for(auto &it : objects->systems)
		it.second.SetDate(date);
	politics.ResetDaily();
}



void GameData::ReadEconomy(const DataNode &node)
{
	if(!node.Size() || node.Token(0) != "economy")
		return;
	
	vector<string> headings;
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "purchases")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 3 && grand.Value(2))
					purchases[objects->systems.Get(grand.Token(0))][grand.Token(1)] += grand.Value(2);
		}
		else if(child.Token(0) == "system")
		{
			headings.clear();
			for(int index = 1; index < child.Size(); ++index)
				headings.push_back(child.Token(index));
		}
		else
		{
			System &system = *objects->systems.Get(child.Token(0));
			
			int index = 0;
			for(const string &commodity : headings)
				system.SetSupply(commodity, child.Value(++index));
		}
	}
}



void GameData::WriteEconomy(DataWriter &out)
{
	out.Write("economy");
	out.BeginChild();
	{
		// Write each system and the commodity quantities purchased there.
		if(!purchases.empty())
		{
			out.Write("purchases");
			out.BeginChild();
			using Purchase = pair<const System *const, map<string, int>>;
			WriteSorted(purchases,
				[](const Purchase *lhs, const Purchase *rhs)
					{ return lhs->first->Name() < rhs->first->Name(); },
				[&out](const Purchase &pit)
				{
					// Write purchases for all systems, even ones from removed plugins.
					for(const auto &cit : pit.second)
						out.Write(pit.first->Name(), cit.first, cit.second);
				});
			out.EndChild();
		}
		// Write the "header" row.
		out.WriteToken("system");
		for(const auto &cit : GameData::Commodities())
			out.WriteToken(cit.name);
		out.Write();
		
		// Write the per-system data for all systems that are either known-valid, or non-empty.
		for(const auto &sit : GameData::Systems())
		{
			if(!sit.second.IsValid() && !sit.second.HasTrade())
				continue;
			
			out.WriteToken(sit.second.Name());
			for(const auto &cit : GameData::Commodities())
				out.WriteToken(static_cast<int>(sit.second.Supply(cit.name)));
			out.Write();
		}
	}
	out.EndChild();
}



void GameData::StepEconomy()
{
	// First, apply any purchases the player made. These are deferred until now
	// so that prices will not change as you are buying or selling goods.
	for(const auto &pit : purchases)
	{
		System &system = const_cast<System &>(*pit.first);
		for(const auto &cit : pit.second)
			system.SetSupply(cit.first, system.Supply(cit.first) - cit.second);
	}
	purchases.clear();
	
	// Then, have each system generate new goods for local use and trade.
	for(auto &it : objects->systems)
		it.second.StepEconomy();
	
	// Finally, send out the trade goods. This has to be done in a separate step
	// because otherwise whichever systems trade last would already have gotten
	// supplied by the other systems.
	for(auto &it : objects->systems)
	{
		System &system = it.second;
		if(!system.Links().empty())
			for(const Trade::Commodity &commodity : objects->trade.Commodities())
			{
				double supply = system.Supply(commodity.name);
				for(const System *neighbor : system.Links())
				{
					double scale = neighbor->Links().size();
					if(scale)
						supply += neighbor->Exports(commodity.name) / scale;
				}
				system.SetSupply(commodity.name, supply);
			}
	}
}



void GameData::AddPurchase(const System &system, const string &commodity, int tons)
{
	if(tons < 0)
		purchases[&system][commodity] += tons;
}



// Update the neighbor lists and other information for all the systems.
// This must be done any time that a change creates or moves a system.
void GameData::UpdateSystems()
{
	for(auto &it : objects->systems)
	{
		// Skip systems that have no name.
		if(it.first.empty() || it.second.Name().empty())
			continue;
		it.second.UpdateSystem(objects->systems, neighborDistances);
	}
}



void GameData::AddJumpRange(double neighborDistance)
{
	neighborDistances.insert(neighborDistance);
}



// Re-activate any special persons that were created previously but that are
// still alive.
void GameData::ResetPersons()
{
	for(auto &it : objects->persons)
		it.second.ClearPlacement();
}



// Mark all persons in the given list as dead.
void GameData::DestroyPersons(vector<string> &names)
{
	for(const string &name : names)
		objects->persons.Get(name)->Destroy();
}



const Set<Color> &GameData::Colors()
{
	return objects->colors;
}



const Set<Conversation> &GameData::Conversations()
{
	return objects->conversations;
}



const Set<Effect> &GameData::Effects()
{
	return objects->effects;
}




const Set<GameEvent> &GameData::Events()
{
	return objects->events;
}



const Set<Fleet> &GameData::Fleets()
{
	return objects->fleets;
}



const Set<Galaxy> &GameData::Galaxies()
{
	return objects->galaxies;
}



const Set<Government> &GameData::Governments()
{
	return objects->governments;
}



const Set<Hazard> &GameData::Hazards()
{
	return objects->hazards;
}



const Set<Interface> &GameData::Interfaces()
{
	return objects->interfaces;
}



const Set<Minable> &GameData::Minables()
{
	return objects->minables;
}




const Set<Mission> &GameData::Missions()
{
	return objects->missions;
}



const Set<News> &GameData::SpaceportNews()
{
	return objects->news;
}



const Set<Outfit> &GameData::Outfits()
{
	return objects->outfits;
}



const Set<Sale<Outfit>> &GameData::Outfitters()
{
	return objects->outfitSales;
}



const Set<Person> &GameData::Persons()
{
	return objects->persons;
}



const Set<Phrase> &GameData::Phrases()
{
	return objects->phrases;
}



const Set<Planet> &GameData::Planets()
{
	return objects->planets;
}



const Set<Ship> &GameData::Ships()
{
	return objects->ships;
}



const Set<Test> &GameData::Tests()
{
	return objects->tests;
}



const Set<TestData> &GameData::TestDataSets()
{
	return objects->testDataSets;
}



const Set<Sale<Ship>> &GameData::Shipyards()
{
	return objects->shipSales;
}



const Set<System> &GameData::Systems()
{
	return objects->systems;
}



const Government *GameData::PlayerGovernment()
{
	return playerGovernment;
}



Politics &GameData::GetPolitics()
{
	return politics;
}



const vector<StartConditions> &GameData::StartOptions()
{
	return objects->startConditions;
}



const vector<Trade::Commodity> &GameData::Commodities()
{
	return objects->trade.Commodities();
}




const vector<Trade::Commodity> &GameData::SpecialCommodities()
{
	return objects->trade.SpecialCommodities();
}



// Custom messages to be shown when trying to land on certain stellar objects->
bool GameData::HasLandingMessage(const Sprite *sprite)
{
	return objects->landingMessages.count(sprite);
}



const string &GameData::LandingMessage(const Sprite *sprite)
{
	static const string EMPTY;
	auto it = objects->landingMessages.find(sprite);
	return (it == objects->landingMessages.end() ? EMPTY : it->second);
}



// Get the solar power and wind output of the given stellar object sprite.
double GameData::SolarPower(const Sprite *sprite)
{
	auto it = objects->solarPower.find(sprite);
	return (it == objects->solarPower.end() ? 0. : it->second);
}



double GameData::SolarWind(const Sprite *sprite)
{
	auto it = objects->solarWind.find(sprite);
	return (it == objects->solarWind.end() ? 0. : it->second);
}



// Strings for combat rating levels, etc.
const string &GameData::Rating(const string &type, int level)
{
	static const string EMPTY;
	auto it = objects->ratings.find(type);
	if(it == objects->ratings.end() || it->second.empty())
		return EMPTY;
	
	level = max(0, min<int>(it->second.size() - 1, level));
	return it->second[level];
}



// Strings for ship, bay type, and outfit categories.
const vector<string> &GameData::Category(const CategoryType type)
{
	return objects->categories[type];
}



const string &GameData::Tooltip(const string &label)
{
	static const string EMPTY;
	auto it = objects->tooltips.find(label);
	// Special case: the "cost" and "sells for" labels include the percentage of
	// the full price, so they will not match exactly.
	if(it == objects->tooltips.end() && !label.compare(0, 4, "cost"))
		it = objects->tooltips.find("cost:");
	if(it == objects->tooltips.end() && !label.compare(0, 9, "sells for"))
		it = objects->tooltips.find("sells for:");
	return (it == objects->tooltips.end() ? EMPTY : it->second);
}



string GameData::HelpMessage(const string &name)
{
	static const string EMPTY;
	auto it = objects->helpMessages.find(name);
	return Command::ReplaceNamesWithKeys(it == objects->helpMessages.end() ? EMPTY : it->second);
}



const map<string, string> &GameData::HelpTemplates()
{
	return objects->helpMessages;
}



const map<string, string> &GameData::PluginAboutText()
{
	return plugins;
}



void GameData::LoadSources()
{
	sources.clear();
	sources.push_back(Files::Resources());
	
	vector<string> globalPlugins = Files::ListDirectories(Files::Resources() + "plugins/");
	for(const string &path : globalPlugins)
	{
		if(Files::Exists(path + "data") || Files::Exists(path + "images") || Files::Exists(path + "sounds"))
			sources.push_back(path);
	}
	
	vector<string> localPlugins = Files::ListDirectories(Files::Config() + "plugins/");
	for(const string &path : localPlugins)
	{
		if(Files::Exists(path + "data") || Files::Exists(path + "images") || Files::Exists(path + "sounds"))
			sources.push_back(path);
	}
	
	// Load the plugin data, if any.
	for(auto it = sources.begin() + 1; it != sources.end(); ++it)
	{
		// Get the name of the folder containing the plugin.
		size_t pos = it->rfind('/', it->length() - 2) + 1;
		string name = it->substr(pos, it->length() - 1 - pos);
		
		// Load the about text and the icon, if any.
		plugins[name] = Files::Read(*it + "about.txt");
		
		Render::LoadPlugin(*it, name);
	}
}



// This prints out the list of tests that are available and their status
// (active/missing feature/known failure)..
void GameData::PrintTestsTable()
{
	cout << "status" << '\t' << "name" << '\n';
	for(auto &it : objects->tests)
	{
		const Test &test = it.second;
		cout << test.StatusText() << '\t';
		cout << "\"" << test.Name() << "\"" << '\n';
	}
	cout.flush();
}



void GameData::PrintShipTable()
{
	cout << "model" << '\t' << "cost" << '\t' << "shields" << '\t' << "hull" << '\t'
		<< "mass" << '\t' << "crew" << '\t' << "cargo" << '\t' << "bunks" << '\t'
		<< "fuel" << '\t' << "outfit" << '\t' << "weapon" << '\t' << "engine" << '\t'
		<< "speed" << '\t' << "accel" << '\t' << "turn" << '\t'
		<< "energy generation" << '\t' << "max energy usage" << '\t' << "energy capacity" << '\t'
		<< "idle/max heat" << '\t' << "max heat generation" << '\t' << "max heat dissipation" << '\t'
		<< "gun mounts" << '\t' << "turret mounts" << '\n';
	for(auto &it : objects->ships)
	{
		// Skip variants and unnamed / partially-defined ships.
		if(it.second.ModelName() != it.first)
			continue;
		
		const Ship &ship = it.second;
		cout << it.first << '\t';
		cout << ship.Cost() << '\t';
		
		const Outfit &attributes = ship.Attributes();
		auto mass = attributes.Mass() ? attributes.Mass() : 1.;
		cout << attributes.Get("shields") << '\t';
		cout << attributes.Get("hull") << '\t';
		cout << mass << '\t';
		cout << attributes.Get("required crew") << '\t';
		cout << attributes.Get("cargo space") << '\t';
		cout << attributes.Get("bunks") << '\t';
		cout << attributes.Get("fuel capacity") << '\t';
		
		cout << ship.BaseAttributes().Get("outfit space") << '\t';
		cout << ship.BaseAttributes().Get("weapon capacity") << '\t';
		cout << ship.BaseAttributes().Get("engine capacity") << '\t';
		cout << (attributes.Get("drag") ? (60. * attributes.Get("thrust") / attributes.Get("drag")) : 0) << '\t';
		cout << 3600. * attributes.Get("thrust") / mass << '\t';
		cout << 60. * attributes.Get("turn") / mass << '\t';
		
		double energyConsumed = attributes.Get("energy consumption")
			+ max(attributes.Get("thrusting energy"), attributes.Get("reverse thrusting energy"))
			+ attributes.Get("turning energy")
			+ attributes.Get("afterburner energy")
			+ attributes.Get("fuel energy")
			+ (attributes.Get("hull energy") * (1 + attributes.Get("hull energy multiplier")))
			+ (attributes.Get("shield energy") * (1 + attributes.Get("shield energy multiplier")))
			+ attributes.Get("cooling energy")
			+ attributes.Get("cloaking energy");
		
		double heatProduced = attributes.Get("heat generation") - attributes.Get("cooling")
			+ max(attributes.Get("thrusting heat"), attributes.Get("reverse thrusting heat"))
			+ attributes.Get("turning heat")
			+ attributes.Get("afterburner heat")
			+ attributes.Get("fuel heat")
			+ (attributes.Get("hull heat") * (1 + attributes.Get("hull heat multiplier")))
			+ (attributes.Get("shield heat") * (1 + attributes.Get("shield heat multiplier")))
			+ attributes.Get("solar heat")
			+ attributes.Get("cloaking heat");
		
		for(const auto &oit : ship.Outfits())
			if(oit.first->IsWeapon() && oit.first->Reload())
			{
				double reload = oit.first->Reload();
				energyConsumed += oit.second * oit.first->FiringEnergy() / reload;
				heatProduced += oit.second * oit.first->FiringHeat() / reload;
			}
		cout << 60. * (attributes.Get("energy generation") + attributes.Get("solar collection")) << '\t';
		cout << 60. * energyConsumed << '\t';
		cout << attributes.Get("energy capacity") << '\t';
		cout << ship.IdleHeat() / max(1., ship.MaximumHeat()) << '\t';
		cout << 60. * heatProduced << '\t';
		// Maximum heat is 100 degrees per ton. Bleed off rate is 1/1000 per 60th of a second, so:
		cout << 60. * ship.HeatDissipation() * ship.MaximumHeat() << '\t';

		int numTurrets = 0;
		int numGuns = 0;
		for(auto &hardpoint : ship.Weapons())
		{
			if(hardpoint.IsTurret())
				++numTurrets;
			else
				++numGuns;
		}
		cout << numGuns << '\t' << numTurrets << '\n';
	}
	cout.flush();
}



void GameData::PrintWeaponTable()
{
	cout << "name" << '\t' << "cost" << '\t' << "space" << '\t' << "range" << '\t'
		<< "energy/s" << '\t' << "heat/s" << '\t' << "recoil/s" << '\t'
		<< "shield/s" << '\t' << "hull/s" << '\t' << "push/s" << '\t'
		<< "homing" << '\t' << "strength" <<'\n';
	for(auto &it : objects->outfits)
	{
		// Skip non-weapons and submunitions.
		if(!it.second.IsWeapon() || it.second.Category().empty())
			continue;
		
		const Outfit &outfit = it.second;
		cout << it.first << '\t';
		cout << outfit.Cost() << '\t';
		cout << -outfit.Get("weapon capacity") << '\t';
		
		cout << outfit.Range() << '\t';
		
		double energy = outfit.FiringEnergy() * 60. / outfit.Reload();
		cout << energy << '\t';
		double heat = outfit.FiringHeat() * 60. / outfit.Reload();
		cout << heat << '\t';
		double firingforce = outfit.FiringForce() * 60. / outfit.Reload();
		cout << firingforce << '\t';
		
		double shield = outfit.ShieldDamage() * 60. / outfit.Reload();
		cout << shield << '\t';
		double hull = outfit.HullDamage() * 60. / outfit.Reload();
		cout << hull << '\t';
		double hitforce = outfit.HitForce() * 60. / outfit.Reload();
		cout << hitforce << '\t';
		
		cout << outfit.Homing() << '\t';
		double strength = outfit.MissileStrength() + outfit.AntiMissile();
		cout << strength << '\n';
	}
	cout.flush();
}
