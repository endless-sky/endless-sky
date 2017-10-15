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
#include "FontSet.h"
#include "Galaxy.h"
#include "GameEvent.h"
#include "Government.h"
#include "Interface.h"
#include "LineShader.h"
#include "Minable.h"
#include "Mission.h"
#include "Music.h"
#include "Outfit.h"
#include "OutlineShader.h"
#include "Person.h"
#include "Phrase.h"
#include "Planet.h"
#include "PointerShader.h"
#include "Politics.h"
#include "RingShader.h"
#include "Sale.h"
#include "Set.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteQueue.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StarField.h"
#include "StartConditions.h"
#include "System.h"

#include <algorithm>
#include <iostream>
#include <map>
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
	Set<Interface> interfaces;
	Set<Minable> minables;
	Set<Mission> missions;
	Set<Outfit> outfits;
	Set<Person> persons;
	Set<Phrase> phrases;
	Set<Planet> planets;
	Set<Ship> ships;
	Set<System> systems;
	
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
	StartConditions startConditions;
	
	Trade trade;
	map<const System *, map<string, int>> purchases;
	
	map<const Sprite *, string> landingMessages;
	map<string, vector<string>> ratings;
	
	StarField background;
	
	map<string, string> tooltips;
	map<string, string> helpMessages;
	map<string, string> plugins;
	
	SpriteQueue spriteQueue;
	
	vector<string> sources;
	map<const Sprite *, vector<string>> deferred;
	map<const Sprite *, int> preloaded;
	
	const Government *playerGovernment = nullptr;
}



void GameData::BeginLoad(const char * const *argv)
{
	bool printShips = false;
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
	map<string, string> images;
	LoadImages(images);
	
	// From the name, strip out any frame number, plus the extension.
	for(const auto &it : images)
	{
		string name = Name(it.first);
		// For landscapes, remember all the source files but don't load them yet.
		if(name.substr(0, 5) == "land/")
			deferred[SpriteSet::Get(name)].push_back(it.second);
		else
			spriteQueue.Add(name, it.second);
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
	
	// Now that all the stars are loaded, update the neighbor lists.
	UpdateNeighbors();
	// And, update the ships with the outfits we've now finished loading.
	for(auto &it : ships)
		it.second.FinishLoading(true);
	for(const auto &it : persons)
		it.second.GetShip()->FinishLoading(true);
	
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
	if(printWeapons)
		PrintWeaponTable();
}



// Check for objects that are referred to but never defined.
void GameData::CheckReferences()
{
	for(const auto &it : effects)
		if(it.second.Name().empty())
			Files::LogError("Warning: effect \"" + it.first + "\" is referred to, but never defined.");
	for(const auto &it : events)
		if(it.second.Name().empty())
			Files::LogError("Warning: event \"" + it.first + "\" is referred to, but never defined.");
	for(const auto &it : fleets)
		if(!it.second.GetGovernment())
			Files::LogError("Warning: fleet \"" + it.first + "\" is referred to, but never defined.");
	for(const auto &it : governments)
		if(it.second.GetName().empty())
			Files::LogError("Warning: government \"" + it.first + "\" is referred to, but never defined.");
	for(const auto &it : minables)
		if(it.second.Name().empty())
			Files::LogError("Warning: minable \"" + it.first + "\" is referred to, but never defined.");
	for(const auto &it : missions)
		if(it.second.Name().empty())
			Files::LogError("Warning: mission \"" + it.first + "\" is referred to, but never defined.");
	for(const auto &it : outfits)
		if(it.second.Name().empty())
			Files::LogError("Warning: outfit \"" + it.first + "\" is referred to, but never defined.");
	for(const auto &it : phrases)
		if(it.second.Name().empty())
			Files::LogError("Warning: phrase \"" + it.first + "\" is referred to, but never defined.");
	for(const auto &it : planets)
		if(it.second.Name().empty())
			Files::LogError("Warning: planet \"" + it.first + "\" is referred to, but never defined.");
	for(const auto &it : ships)
		if(it.second.ModelName().empty())
			Files::LogError("Warning: ship \"" + it.first + "\" is referred to, but never defined.");
	for(const auto &it : systems)
		if(it.second.Name().empty())
			Files::LogError("Warning: system \"" + it.first + "\" is referred to, but never defined.");
}



void GameData::LoadShaders()
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
	SpriteShader::Init();
	
	background.Init(16384, 4096);
}



double GameData::Progress()
{
	return min(spriteQueue.Progress(), Audio::Progress());
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
	for(const string &path : dit->second)
		spriteQueue.Add(name, path);
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
		it.second.GetShip()->Restore();
	
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
		if(!purchases.empty())
		{
			out.Write("purchases");
			out.BeginChild();
			for(const auto &pit : purchases)
				for(const auto &cit : pit.second)
					out.Write(pit.first->Name(), cit.first, cit.second);
			out.EndChild();
		}
		out.WriteToken("system");
		for(const auto &cit : GameData::Commodities())
			out.WriteToken(cit.name);
		out.Write();
		
		for(const auto &sit : GameData::Systems())
		{
			// Skip systems that have no name.
			if(sit.first.empty() || sit.second.Name().empty())
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
		planets.Get(node.Token(1))->Load(node, shipSales, outfitSales);
	else if(node.Token(0) == "shipyard" && node.Size() >= 2)
		shipSales.Get(node.Token(1))->Load(node, ships);
	else if(node.Token(0) == "system" && node.Size() >= 2)
		systems.Get(node.Token(1))->Load(node, planets);
	else if(node.Token(0) == "link" && node.Size() >= 3)
		systems.Get(node.Token(1))->Link(systems.Get(node.Token(2)));
	else if(node.Token(0) == "unlink" && node.Size() >= 3)
		systems.Get(node.Token(1))->Unlink(systems.Get(node.Token(2)));
	else
		node.PrintTrace("Invalid \"event\" data:");
}



// Update the neighbor lists of all the systems. This must be done any time
// that a change creates or moves a system.
void GameData::UpdateNeighbors()
{
	for(auto &it : systems)
		it.second.UpdateNeighbors(systems);
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



const Set<Outfit> &GameData::Outfits()
{
	return outfits;
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



const StartConditions &GameData::Start()
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



// Strings for combat rating levels, etc.
const string &GameData::Rating(const std::string &type, int level)
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
		
		if(Files::Exists(*it + "icon.png"))
			spriteQueue.Add(name, *it + "icon.png");
		else if(Files::Exists(*it + "icon.jpg"))
			spriteQueue.Add(name, *it + "icon.jpg");
		
		if(Files::Exists(*it + "icon@2x.png"))
			spriteQueue.Add(name, *it + "icon@2x.png");
		else if(Files::Exists(*it + "icon@2x.jpg"))
			spriteQueue.Add(name, *it + "icon@2x.jpg");
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
			planets.Get(node.Token(1))->Load(node, shipSales, outfitSales);
		else if(key == "ship" && node.Size() >= 2)
		{
			// Allow multiple named variants of the same ship model.
			const string &name = node.Token((node.Size() > 2) ? 2 : 1);
			ships.Get(name)->Load(node);
		}
		else if(key == "shipyard" && node.Size() >= 2)
			shipSales.Get(node.Token(1))->Load(node, ships);
		else if(key == "start")
			startConditions.Load(node);
		else if(key == "system" && node.Size() >= 2)
			systems.Get(node.Token(1))->Load(node, planets);
		else if(key == "trade")
			trade.Load(node);
		else if(key == "landing message" && node.Size() >= 2)
		{
			for(const DataNode &child : node)
				landingMessages[SpriteSet::Get(child.Token(0))] = node.Token(1);
		}
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



void GameData::LoadImages(map<string, string> &images)
{
	for(const string &source : sources)
	{
		string directoryPath = source + "images/";
		vector<string> imageFiles = Files::RecursiveList(directoryPath);
		for(const string &path : imageFiles)
			LoadImage(path, images, directoryPath.length());
	}
}



void GameData::LoadImage(const string &path, map<string, string> &images, size_t start)
{
	bool isJpg = !path.compare(path.length() - 4, 4, ".jpg");
	bool isPng = !path.compare(path.length() - 4, 4, ".png");
	
	// This is an ordinary file. Check to see if it is an image.
	if(isJpg || isPng)
		images[path.substr(start)] = path;
}



string GameData::Name(const string &path)
{
	// The path always ends in a three-letter extension, ".png" or ".jpg".
	int end = path.length() - 4;
	
	// Check if the name ends in "@2x". If so, that's not part of the name.
	if(end > 3 && path[end - 3] == '@' && path[end - 2] == '2' && path[end - 1] == 'x')
		end -= 3;
	
	// Skip any numbers at the end of the name.
	int pos = end;
	while(pos--)
		if(path[pos] < '0' || path[pos] > '9')
			break;
	
	// This should never happen, but just in case someone creates a file named
	// "images/123.jpg" or something:
	if(pos < 0)
		pos = end;
	else if(path[pos] != '-' && path[pos] != '~' && path[pos] != '+' && path[pos] != '=')
		pos = end;
	
	return path.substr(0, pos);
}



void GameData::PrintShipTable()
{
	cout << "model" << '\t' << "cost" << '\t' << "shields" << '\t' << "hull" << '\t'
		<< "mass" << '\t' << "crew" << '\t' << "cargo" << '\t' << "bunks" << '\t'
		<< "fuel" << '\t' << "outfit" << '\t' << "weapon" << '\t' << "engine" << '\t'
		<< "speed" << '\t' << "accel" << '\t' << "turn" << '\t'
		<< "e_gen" << '\t' << "e_use" << '\t' << "h_gen" << '\t' << "h_max" << '\n';
	for(auto &it : ships)
	{
		// Skip variants.
		if(it.second.ModelName() != it.first)
			continue;
		
		const Ship &ship = it.second;
		cout << it.first << '\t';
		cout << ship.Cost() << '\t';
		
		const Outfit &attributes = ship.Attributes();
		cout << attributes.Get("shields") << '\t';
		cout << attributes.Get("hull") << '\t';
		cout << attributes.Get("mass") << '\t';
		cout << attributes.Get("required crew") << '\t';
		cout << attributes.Get("cargo space") << '\t';
		cout << attributes.Get("bunks") << '\t';
		cout << attributes.Get("fuel capacity") << '\t';
		
		cout << ship.BaseAttributes().Get("outfit space") << '\t';
		cout << ship.BaseAttributes().Get("weapon capacity") << '\t';
		cout << ship.BaseAttributes().Get("engine capacity") << '\t';
		cout << 60. * attributes.Get("thrust") / attributes.Get("drag") << '\t';
		cout << 3600. * attributes.Get("thrust") / attributes.Get("mass") << '\t';
		cout << 60. * attributes.Get("turn") / attributes.Get("mass") << '\t';
		
		double energy = attributes.Get("thrusting energy")
			+ attributes.Get("turning energy");
		double heat = attributes.Get("heat generation") - attributes.Get("cooling")
			+ attributes.Get("thrusting heat") + attributes.Get("turning heat");
		for(const auto &oit : ship.Outfits())
			if(oit.first->IsWeapon() && oit.first->Reload())
			{
				double reload = oit.first->Reload();
				energy += oit.second * oit.first->FiringEnergy() / reload;
				heat += oit.second * oit.first->FiringHeat() / reload;
			}
		cout << 60. * attributes.Get("energy generation") << '\t';
		cout << 60. * energy << '\t';
		cout << 60. * heat << '\t';
		// Maximum heat is 100 degrees per ton. Bleed off rate is 1/1000
		// per 60th of a second, so:
		cout << 60. * ship.Mass() * .1 * attributes.Get("heat dissipation") << '\n';
	}
	cout.flush();
}



void GameData::PrintWeaponTable()
{
	cout << "name" << '\t' << "cost" << '\t' << "space" << '\t' << "range" << '\t'
		<< "energy/s" << '\t' << "heat/s" << '\t' << "shield/s" << '\t' << "hull/s" << '\t'
		<< "homing" << '\t' << "strength" << '\n';
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
		
		double shield = outfit.ShieldDamage() * 60. / outfit.Reload();
		cout << shield << '\t';
		double hull = outfit.HullDamage() * 60. / outfit.Reload();
		cout << hull << '\t';
		
		cout << outfit.Homing() << '\t';
		double strength = outfit.MissileStrength() + outfit.AntiMissile();
		cout << strength << '\n';
	}
	cout.flush();
}
