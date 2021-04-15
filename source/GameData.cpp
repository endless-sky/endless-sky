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
	Set<Outfit> outfits;
	Set<Person> persons;
	Set<Phrase> phrases;
	Set<Planet> planets;
	Set<Ship> ships;
	Set<System> systems;
	Set<Test> tests;
	Set<TestData> testDataSets;
	set<double> neighborDistances;
	
	Set<Sale<Ship>> shipSales;
	Set<Sale<Outfit>> outfitSales;
	
	Set<Fleet> defaultFleets;
	Set<Government> defaultGovernments;
	Set<Planet> defaultPlanets;
	Set<System> defaultSystems;
	Set<Galaxy> defaultGalaxies;
	Set<Sale<Ship>> defaultShipSales;
	Set<Sale<Outfit>> defaultOutfitSales;
	
	Politics politics;
	vector<StartConditions> startConditions;
	
	Trade trade;
	map<const System *, map<string, int>> purchases;
	
	map<const Sprite *, string> landingMessages;
	map<const Sprite *, double> solarPower;
	map<const Sprite *, double> solarWind;
	Set<News> news;
	map<string, vector<string>> ratings;
	
	StarField background;
	
	map<string, string> tooltips;
	map<string, string> helpMessages;
	map<string, string> plugins;
	
	SpriteQueue spriteQueue;
	// Whether sprites and audio have finished loading at game startup.
	bool initiallyLoaded = false;
	
	vector<string> sources;
	map<const Sprite *, shared_ptr<ImageSet>> deferred;
	map<const Sprite *, int> preloaded;
	
	const Government *playerGovernment = nullptr;
	
	// TODO (C++14): make these 3 methods generic lambdas visible only to the CheckReferences method.
	// Log a warning for an "undefined" class object that was never loaded from disk.
	void Warn(const string &noun, const string &name)
	{
		Files::LogError("Warning: " + noun + " \"" + name + "\" is referred to, but never defined.");
	}
	// Class objects with a deferred definition should still get named when content is loaded.
	template <class Type>
	bool NameIfDeferred(const set<string> &deferred, pair<const string, Type> &it)
	{
		if(deferred.count(it.first))
			it.second.SetName(it.first);
		else
			return false;
		
		return true;
	}
	// Set the name of an "undefined" class object, so that it can be written to the player's save.
	template <class Type>
	void NameAndWarn(const string &noun, pair<const string, Type> &it)
	{
		it.second.SetName(it.first);
		Warn(noun, it.first);
	}
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
	
	// Now, read all the images in all the path directories. For each unique
	// name, only remember one instance, letting things on the higher priority
	// paths override the default images.
	map<string, shared_ptr<ImageSet>> images = FindImages();
	
	// From the name, strip out any frame number, plus the extension.
	for(const auto &it : images)
	{
		// This should never happen, but just in case:
		if(!it.second)
			continue;
		
		// Check that the image set is complete.
		it.second->Check();
		// For landscapes, remember all the source files but don't load them yet.
		if(ImageSet::IsDeferred(it.first))
			deferred[SpriteSet::Get(it.first)] = it.second;
		else
			spriteQueue.Add(it.second);
	}
	
	// Generate a catalog of music files.
	Music::Init(sources);
	
	for(const string &source : sources)
	{
		// Iterate through the paths starting with the last directory given. That
		// is, things in folders near the start of the path have the ability to
		// override things in folders later in the path.
		vector<string> dataFiles = Files::RecursiveList(source + "data/");
		for(const string &path : dataFiles)
			LoadFile(path, debugMode);
	}
	
	// Now that all data is loaded, update the neighbor lists and other
	// system information. Make sure that the default jump range is among the
	// neighbor distances to be updated.
	AddJumpRange(System::DEFAULT_NEIGHBOR_DISTANCE);
	UpdateSystems();
	// And, update the ships with the outfits we've now finished loading.
	for(auto &&it : ships)
		it.second.FinishLoading(true);
	for(auto &&it : persons)
		it.second.FinishLoading();
	for(auto &&it : startConditions)
		it.FinishLoading();
	
	// Remove any invalid starting conditions, so the game does not use incomplete data.
	startConditions.erase(remove_if(startConditions.begin(), startConditions.end(),
			[](const StartConditions &it) noexcept -> bool { return !it.IsValid(); }),
		startConditions.end()
	);
	
	// Store the current state, to revert back to later.
	defaultFleets = fleets;
	defaultGovernments = governments;
	defaultPlanets = planets;
	defaultSystems = systems;
	defaultGalaxies = galaxies;
	defaultShipSales = shipSales;
	defaultOutfitSales = outfitSales;
	playerGovernment = governments.Get("Escort");
	
	politics.Reset();
	
	if(printShips)
		PrintShipTable();
	if(printTests)
		PrintTestsTable();
	if(printWeapons)
		PrintWeaponTable();
	return !(printShips || printWeapons || printTests);
}



// Check for objects that are referred to but never defined. Some elements, like
// fleets, don't need to be given a name if undefined. Others (like outfits and
// planets) are written to the player's save and need a name to prevent data loss.
void GameData::CheckReferences()
{
	// Parse all GameEvents for object definitions.
	auto deferred = map<string, set<string>>{};
	for(auto &&it : events)
	{
		// Stock GameEvents are serialized in MissionActions by name.
		if(it.second.Name().empty())
			NameAndWarn("event", it);
		else
		{
			// Any already-named event (i.e. loaded) may alter the universe.
			auto definitions = GameEvent::DeferredDefinitions(it.second.Changes());
			for(auto &&type : definitions)
				deferred[type.first].insert(type.second.begin(), type.second.end());
		}
	}
	
	// Stock conversations are never serialized.
	for(const auto &it : conversations)
		if(it.second.IsEmpty())
			Warn("conversation", it.first);
	// The "default intro" conversation must invoke the prompt to set the player's name.
	if(!conversations.Get("default intro")->IsValidIntro())
		Files::LogError("Error: the \"default intro\" conversation must contain a \"name\" node.");
	// Effects are serialized as a part of ships.
	for(auto &&it : effects)
		if(it.second.Name().empty())
			NameAndWarn("effect", it);
	// Fleets are not serialized. Any changes via events are written as DataNodes and thus self-define.
	for(const auto &it : fleets)
		if(!it.second.IsValid() && !deferred["fleet"].count(it.first))
			Warn("fleet", it.first);
	// Government names are used in mission NPC blocks and LocationFilters.
	for(auto &&it : governments)
		if(it.second.GetTrueName().empty() && !NameIfDeferred(deferred["government"], it))
			NameAndWarn("government", it);
	// Minables are not serialized.
	for(const auto &it : minables)
		if(it.second.Name().empty())
			Warn("minable", it.first);
	// Stock missions are never serialized, and an accepted mission is
	// always fully defined (though possibly not "valid").
	for(const auto &it : missions)
		if(it.second.Name().empty())
			Warn("mission", it.first);
	
	// News are never serialized or named, except by events (which would then define them).
	
	// Outfit names are used by a number of classes.
	for(auto &&it : outfits)
		if(it.second.Name().empty())
			NameAndWarn("outfit", it);
	// Outfitters are never serialized.
	for(const auto &it : outfitSales)
		if(it.second.empty() && !deferred["outfitter"].count(it.first))
			Files::LogError("Warning: outfitter \"" + it.first + "\" is referred to, but has no outfits.");
	// Phrases are never serialized.
	for(const auto &it : phrases)
		if(it.second.Name().empty())
			Warn("phrase", it.first);
	// Planet names are used by a number of classes.
	for(auto &&it : planets)
		if(it.second.TrueName().empty() && !NameIfDeferred(deferred["planet"], it))
			NameAndWarn("planet", it);
	// Ship model names are used by missions and depreciation.
	for(auto &&it : ships)
		if(it.second.ModelName().empty())
		{
			it.second.SetModelName(it.first);
			Warn("ship", it.first);
		}
	// Shipyards are never serialized.
	for(const auto &it : shipSales)
		if(it.second.empty() && !deferred["shipyard"].count(it.first))
			Files::LogError("Warning: shipyard \"" + it.first + "\" is referred to, but has no ships.");
	// System names are used by a number of classes.
	for(auto &&it : systems)
		if(it.second.Name().empty() && !NameIfDeferred(deferred["system"], it))
			NameAndWarn("system", it);
}



void GameData::LoadShaders(bool useShaderSwizzle)
{
	FontSet::Add(Files::Images() + "font/ubuntu14r.png", 14);
	FontSet::Add(Files::Images() + "font/ubuntu18r.png", 18);
	
	// Load the key settings.
	Command::LoadSettings(Files::Resources() + "keys.txt");
	Command::LoadSettings(Files::Config() + "keys.txt");
	
	FillShader::Init();
	FogShader::Init();
	LineShader::Init();
	OutlineShader::Init();
	PointerShader::Init();
	RingShader::Init();
	SpriteShader::Init(useShaderSwizzle);
	BatchShader::Init();
	
	background.Init(16384, 4096);
}



double GameData::Progress()
{
	auto progress = min(spriteQueue.Progress(), Audio::GetProgress());
	if(progress == 1.)
	{
		if(!initiallyLoaded)
		{
			// Now that we have finished loading all the basic sprites, we can look for invalid file paths,
			// e.g. due to capitalization errors or other typos. Landscapes are allowed to still be empty.
			auto unloaded = SpriteSet::CheckReferences();
			for(const auto &path : unloaded)
				if(path.compare(0, 5, "land/") != 0)
					Files::LogError("Warning: image \"" + path + "\" is referred to, but has no pixels.");
			initiallyLoaded = true;
		}
	}
	return progress;
}



bool GameData::IsLoaded()
{
	return initiallyLoaded;
}



// Begin loading a sprite that was previously deferred. Currently this is
// done with all landscapes to speed up the program's startup.
void GameData::Preload(const Sprite *sprite)
{
	// Make sure this sprite actually is one that uses deferred loading.
	auto dit = deferred.find(sprite);
	if(!sprite || dit == deferred.end())
		return;
	
	// If this sprite is one of the currently loaded ones, there is no need to
	// load it again. But, make note of the fact that it is the most recently
	// asked-for sprite.
	map<const Sprite *, int>::iterator pit = preloaded.find(sprite);
	if(pit != preloaded.end())
	{
		for(pair<const Sprite * const, int> &it : preloaded)
			if(it.second < pit->second)
				++it.second;
		
		pit->second = 0;
		return;
	}
	
	// This sprite is not currently preloaded. Check to see whether we already
	// have the maximum number of sprites loaded, in which case the oldest one
	// must be unloaded to make room for this one.
	const string &name = sprite->Name();
	pit = preloaded.begin();
	while(pit != preloaded.end())
	{
		++pit->second;
		if(pit->second >= 20)
		{
			spriteQueue.Unload(name);
			pit = preloaded.erase(pit);
		}
		else
			++pit;
	}
	
	// Now, load all the files for this sprite.
	preloaded[sprite] = 0;
	spriteQueue.Add(dit->second);
}



void GameData::FinishLoading()
{
	spriteQueue.Finish();
}



// Get the list of resource sources (i.e. plugin folders).
const vector<string> &GameData::Sources()
{
	return sources;
}



// Revert any changes that have been made to the universe.
void GameData::Revert()
{
	fleets.Revert(defaultFleets);
	governments.Revert(defaultGovernments);
	planets.Revert(defaultPlanets);
	systems.Revert(defaultSystems);
	galaxies.Revert(defaultGalaxies);
	shipSales.Revert(defaultShipSales);
	outfitSales.Revert(defaultOutfitSales);
	for(auto &it : persons)
		it.second.Restore();
	
	politics.Reset();
	purchases.clear();
}



void GameData::SetDate(const Date &date)
{
	for(auto &it : systems)
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
					purchases[systems.Get(grand.Token(0))][grand.Token(1)] += grand.Value(2);
		}
		else if(child.Token(0) == "system")
		{
			headings.clear();
			for(int index = 1; index < child.Size(); ++index)
				headings.push_back(child.Token(index));
		}
		else
		{
			System &system = *systems.Get(child.Token(0));
			
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
	for(auto &it : systems)
		it.second.StepEconomy();
	
	// Finally, send out the trade goods. This has to be done in a separate step
	// because otherwise whichever systems trade last would already have gotten
	// supplied by the other systems.
	for(auto &it : systems)
	{
		System &system = it.second;
		if(!system.Links().empty())
			for(const Trade::Commodity &commodity : trade.Commodities())
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



// Apply the given change to the universe.
void GameData::Change(const DataNode &node)
{
	if(node.Token(0) == "fleet" && node.Size() >= 2)
		fleets.Get(node.Token(1))->Load(node);
	else if(node.Token(0) == "galaxy" && node.Size() >= 2)
		galaxies.Get(node.Token(1))->Load(node);
	else if(node.Token(0) == "government" && node.Size() >= 2)
		governments.Get(node.Token(1))->Load(node);
	else if(node.Token(0) == "outfitter" && node.Size() >= 2)
		outfitSales.Get(node.Token(1))->Load(node, outfits);
	else if(node.Token(0) == "planet" && node.Size() >= 2)
		planets.Get(node.Token(1))->Load(node);
	else if(node.Token(0) == "shipyard" && node.Size() >= 2)
		shipSales.Get(node.Token(1))->Load(node, ships);
	else if(node.Token(0) == "system" && node.Size() >= 2)
		systems.Get(node.Token(1))->Load(node, planets);
	else if(node.Token(0) == "news" && node.Size() >= 2)
		news.Get(node.Token(1))->Load(node);
	else if(node.Token(0) == "link" && node.Size() >= 3)
		systems.Get(node.Token(1))->Link(systems.Get(node.Token(2)));
	else if(node.Token(0) == "unlink" && node.Size() >= 3)
		systems.Get(node.Token(1))->Unlink(systems.Get(node.Token(2)));
	else
		node.PrintTrace("Invalid \"event\" data:");
}



// Update the neighbor lists and other information for all the systems.
// This must be done any time that a change creates or moves a system.
void GameData::UpdateSystems()
{
	for(auto &it : systems)
	{
		// Skip systems that have no name.
		if(it.first.empty() || it.second.Name().empty())
			continue;
		it.second.UpdateSystem(systems, neighborDistances);
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
	for(auto &it : persons)
		it.second.ClearPlacement();
}



// Mark all persons in the given list as dead.
void GameData::DestroyPersons(vector<string> &names)
{
	for(const string &name : names)
		persons.Get(name)->Destroy();
}



const Set<Color> &GameData::Colors()
{
	return colors;
}



const Set<Conversation> &GameData::Conversations()
{
	return conversations;
}



const Set<Effect> &GameData::Effects()
{
	return effects;
}




const Set<GameEvent> &GameData::Events()
{
	return events;
}



const Set<Fleet> &GameData::Fleets()
{
	return fleets;
}



const Set<Galaxy> &GameData::Galaxies()
{
	return galaxies;
}



const Set<Government> &GameData::Governments()
{
	return governments;
}



const Set<Hazard> &GameData::Hazards()
{
	return hazards;
}



const Set<Interface> &GameData::Interfaces()
{
	return interfaces;
}



const Set<Minable> &GameData::Minables()
{
	return minables;
}




const Set<Mission> &GameData::Missions()
{
	return missions;
}



const Set<News> &GameData::SpaceportNews()
{
	return news;
}



const Set<Outfit> &GameData::Outfits()
{
	return outfits;
}



const Set<Sale<Outfit>> &GameData::Outfitters()
{
	return outfitSales;
}



const Set<Person> &GameData::Persons()
{
	return persons;
}



const Set<Phrase> &GameData::Phrases()
{
	return phrases;
}



const Set<Planet> &GameData::Planets()
{
	return planets;
}



const Set<Ship> &GameData::Ships()
{
	return ships;
}



const Set<Test> &GameData::Tests()
{
	return tests;
}



const Set<TestData> &GameData::TestDataSets()
{
	return testDataSets;
}



const Set<Sale<Ship>> &GameData::Shipyards()
{
	return shipSales;
}



const Set<System> &GameData::Systems()
{
	return systems;
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
	return startConditions;
}



const vector<Trade::Commodity> &GameData::Commodities()
{
	return trade.Commodities();
}




const vector<Trade::Commodity> &GameData::SpecialCommodities()
{
	return trade.SpecialCommodities();
}



// Custom messages to be shown when trying to land on certain stellar objects.
bool GameData::HasLandingMessage(const Sprite *sprite)
{
	return landingMessages.count(sprite);
}



const string &GameData::LandingMessage(const Sprite *sprite)
{
	static const string EMPTY;
	auto it = landingMessages.find(sprite);
	return (it == landingMessages.end() ? EMPTY : it->second);
}



// Get the solar power and wind output of the given stellar object sprite.
double GameData::SolarPower(const Sprite *sprite)
{
	auto it = solarPower.find(sprite);
	return (it == solarPower.end() ? 0. : it->second);
}



double GameData::SolarWind(const Sprite *sprite)
{
	auto it = solarWind.find(sprite);
	return (it == solarWind.end() ? 0. : it->second);
}



// Strings for combat rating levels, etc.
const string &GameData::Rating(const string &type, int level)
{
	static const string EMPTY;
	auto it = ratings.find(type);
	if(it == ratings.end() || it->second.empty())
		return EMPTY;
	
	level = max(0, min<int>(it->second.size() - 1, level));
	return it->second[level];
}



const StarField &GameData::Background()
{
	return background;
}



void GameData::SetHaze(const Sprite *sprite)
{
	background.SetHaze(sprite);
}



const string &GameData::Tooltip(const string &label)
{
	static const string EMPTY;
	auto it = tooltips.find(label);
	// Special case: the "cost" and "sells for" labels include the percentage of
	// the full price, so they will not match exactly.
	if(it == tooltips.end() && !label.compare(0, 4, "cost"))
		it = tooltips.find("cost:");
	if(it == tooltips.end() && !label.compare(0, 9, "sells for"))
		it = tooltips.find("sells for:");
	return (it == tooltips.end() ? EMPTY : it->second);
}



string GameData::HelpMessage(const string &name)
{
	static const string EMPTY;
	auto it = helpMessages.find(name);
	return Command::ReplaceNamesWithKeys(it == helpMessages.end() ? EMPTY : it->second);
}



const map<string, string> &GameData::HelpTemplates()
{
	return helpMessages;
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
		
		// Create an image set for the plugin icon.
		shared_ptr<ImageSet> icon(new ImageSet(name));
		
		// Try adding all the possible icon variants.
		if(Files::Exists(*it + "icon.png"))
			icon->Add(*it + "icon.png");
		else if(Files::Exists(*it + "icon.jpg"))
			icon->Add(*it + "icon.jpg");
		
		if(Files::Exists(*it + "icon@2x.png"))
			icon->Add(*it + "icon@2x.png");
		else if(Files::Exists(*it + "icon@2x.jpg"))
			icon->Add(*it + "icon@2x.jpg");
		
		spriteQueue.Add(icon);
	}
}



void GameData::LoadFile(const string &path, bool debugMode)
{
	// This is an ordinary file. Check to see if it is an image.
	if(path.length() < 4 || path.compare(path.length() - 4, 4, ".txt"))
		return;
	
	DataFile data(path);
	if(debugMode)
		Files::LogError("Parsing: " + path);
	
	for(const DataNode &node : data)
	{
		const string &key = node.Token(0);
		if(key == "color" && node.Size() >= 6)
			colors.Get(node.Token(1))->Load(
				node.Value(2), node.Value(3), node.Value(4), node.Value(5));
		else if(key == "conversation" && node.Size() >= 2)
			conversations.Get(node.Token(1))->Load(node);
		else if(key == "effect" && node.Size() >= 2)
			effects.Get(node.Token(1))->Load(node);
		else if(key == "event" && node.Size() >= 2)
			events.Get(node.Token(1))->Load(node);
		else if(key == "fleet" && node.Size() >= 2)
			fleets.Get(node.Token(1))->Load(node);
		else if(key == "galaxy" && node.Size() >= 2)
			galaxies.Get(node.Token(1))->Load(node);
		else if(key == "government" && node.Size() >= 2)
			governments.Get(node.Token(1))->Load(node);
		else if(key == "hazard" && node.Size() >= 2)
			hazards.Get(node.Token(1))->Load(node);
		else if(key == "interface" && node.Size() >= 2)
			interfaces.Get(node.Token(1))->Load(node);
		else if(key == "minable" && node.Size() >= 2)
			minables.Get(node.Token(1))->Load(node);
		else if(key == "mission" && node.Size() >= 2)
			missions.Get(node.Token(1))->Load(node);
		else if(key == "outfit" && node.Size() >= 2)
			outfits.Get(node.Token(1))->Load(node);
		else if(key == "outfitter" && node.Size() >= 2)
			outfitSales.Get(node.Token(1))->Load(node, outfits);
		else if(key == "person" && node.Size() >= 2)
			persons.Get(node.Token(1))->Load(node);
		else if(key == "phrase" && node.Size() >= 2)
			phrases.Get(node.Token(1))->Load(node);
		else if(key == "planet" && node.Size() >= 2)
			planets.Get(node.Token(1))->Load(node);
		else if(key == "ship" && node.Size() >= 2)
		{
			// Allow multiple named variants of the same ship model.
			const string &name = node.Token((node.Size() > 2) ? 2 : 1);
			ships.Get(name)->Load(node);
		}
		else if(key == "shipyard" && node.Size() >= 2)
			shipSales.Get(node.Token(1))->Load(node, ships);
		else if(key == "start" && node.HasChildren())
		{
			// This node may either declare an immutable starting scenario, or one that is open to extension
			// by other nodes (e.g. plugins may customize the basic start, rather than provide a unique start).
			if(node.Size() == 1)
				startConditions.emplace_back(node);
			else
			{
				const string &identifier = node.Token(1);
				auto existingStart = find_if(startConditions.begin(), startConditions.end(),
					[&identifier](const StartConditions &it) noexcept -> bool { return it.Identifier() == identifier; });
				if(existingStart != startConditions.end())
					existingStart->Load(node);
				else
					startConditions.emplace_back(node);
			}
		}
		else if(key == "system" && node.Size() >= 2)
			systems.Get(node.Token(1))->Load(node, planets);
		else if((key == "test") && node.Size() >= 2)
			tests.Get(node.Token(1))->Load(node);
		else if((key == "test-data") && node.Size() >= 2)
			testDataSets.Get(node.Token(1))->Load(node, path);
		else if(key == "trade")
			trade.Load(node);
		else if(key == "landing message" && node.Size() >= 2)
		{
			for(const DataNode &child : node)
				landingMessages[SpriteSet::Get(child.Token(0))] = node.Token(1);
		}
		else if(key == "star" && node.Size() >= 2)
		{
			const Sprite *sprite = SpriteSet::Get(node.Token(1));
			for(const DataNode &child : node)
			{
				if(child.Token(0) == "power" && child.Size() >= 2)
					solarPower[sprite] = child.Value(1);
				else if(child.Token(0) == "wind" && child.Size() >= 2)
					solarWind[sprite] = child.Value(1);
				else
					child.PrintTrace("Unrecognized star attribute:");
			}
		}
		else if(key == "news" && node.Size() >= 2)
			news.Get(node.Token(1))->Load(node);
		else if(key == "rating" && node.Size() >= 2)
		{
			vector<string> &list = ratings[node.Token(1)];
			list.clear();
			for(const DataNode &child : node)
				list.push_back(child.Token(0));
		}
		else if((key == "tip" || key == "help") && node.Size() >= 2)
		{
			string &text = (key == "tip" ? tooltips : helpMessages)[node.Token(1)];
			text.clear();
			for(const DataNode &child : node)
			{
				if(!text.empty())
				{
					text += '\n';
					if(child.Token(0)[0] != '\t')
						text += '\t';
				}
				text += child.Token(0);
			}
		}
		else
			node.PrintTrace("Skipping unrecognized root object:");
	}
}



map<string, shared_ptr<ImageSet>> GameData::FindImages()
{
	map<string, shared_ptr<ImageSet>> images;
	for(const string &source : sources)
	{
		// All names will only include the portion of the path that comes after
		// this directory prefix.
		string directoryPath = source + "images/";
		size_t start = directoryPath.size();
		
		vector<string> imageFiles = Files::RecursiveList(directoryPath);
		for(const string &path : imageFiles)
			if(ImageSet::IsImage(path))
			{
				string name = ImageSet::Name(path.substr(start));
				
				shared_ptr<ImageSet> &imageSet = images[name];
				if(!imageSet)
					imageSet.reset(new ImageSet(name));
				imageSet->Add(path);
			}
	}
	return images;
}



// This prints out the list of tests that are available and their status
// (active/missing feature/known failure)..
void GameData::PrintTestsTable()
{
	cout << "status" << '\t' << "name" << '\n';
	for(auto &it : tests)
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
	for(auto &it : ships)
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
	for(auto &it : outfits)
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
