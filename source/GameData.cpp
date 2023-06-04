/* GameData.cpp
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

#include "GameData.h"

#include "Audio.h"
#include "BatchShader.h"
#include "CategoryList.h"
#include "Color.h"
#include "Command.h"
#include "ConditionsStore.h"
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
#include "FormationPattern.h"
#include "Galaxy.h"
#include "GameEvent.h"
#include "Government.h"
#include "Hazard.h"
#include "ImageSet.h"
#include "Interface.h"
#include "LineShader.h"
#include "MaskManager.h"
#include "Minable.h"
#include "Mission.h"
#include "Music.h"
#include "News.h"
#include "Outfit.h"
#include "OutlineShader.h"
#include "Person.h"
#include "Phrase.h"
#include "Planet.h"
#include "Plugins.h"
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
#include "UniverseObjects.h"

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

using namespace std;

namespace {
	UniverseObjects objects;
	Set<Fleet> defaultFleets;
	Set<Government> defaultGovernments;
	Set<Planet> defaultPlanets;
	Set<System> defaultSystems;
	Set<Galaxy> defaultGalaxies;
	Set<Sale<Ship>> defaultShipSales;
	Set<Sale<Outfit>> defaultOutfitSales;
	Set<Wormhole> defaultWormholes;
	TextReplacements defaultSubstitutions;

	Politics politics;

	StarField background;

	SpriteQueue spriteQueue;

	vector<string> sources;
	map<const Sprite *, shared_ptr<ImageSet>> deferred;
	map<const Sprite *, int> preloaded;

	MaskManager maskManager;

	const Government *playerGovernment = nullptr;
	map<const System *, map<string, int>> purchases;

	ConditionsStore globalConditions;

	void LoadPlugin(const string &path)
	{
		const auto *plugin = Plugins::Load(path);
		if(plugin->enabled)
			sources.push_back(path);

		// Load the icon for the plugin, if any.
		auto icon = make_shared<ImageSet>(plugin->name);

		// Try adding all the possible icon variants.
		if(Files::Exists(path + "icon.png"))
			icon->Add(path + "icon.png");
		else if(Files::Exists(path + "icon.jpg"))
			icon->Add(path + "icon.jpg");

		if(Files::Exists(path + "icon@2x.png"))
			icon->Add(path + "icon@2x.png");
		else if(Files::Exists(path + "icon@2x.jpg"))
			icon->Add(path + "icon@2x.jpg");

		if(!icon->IsEmpty())
		{
			icon->ValidateFrames();
			spriteQueue.Add(icon);
		}
	}
}



future<void> GameData::BeginLoad(bool onlyLoadData, bool debugMode)
{
	// Initialize the list of "source" folders based on any active plugins.
	LoadSources();

	if(!onlyLoadData)
	{
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

			// Reduce the set of images to those that are valid.
			it.second->ValidateFrames();
			// For landscapes, remember all the source files but don't load them yet.
			if(ImageSet::IsDeferred(it.first))
				deferred[SpriteSet::Get(it.first)] = it.second;
			else
				spriteQueue.Add(it.second);
		}

		// Generate a catalog of music files.
		Music::Init(sources);
	}

	return objects.Load(sources, debugMode);
}



void GameData::FinishLoading()
{
	// Store the current state, to revert back to later.
	defaultFleets = objects.fleets;
	defaultGovernments = objects.governments;
	defaultPlanets = objects.planets;
	defaultSystems = objects.systems;
	defaultGalaxies = objects.galaxies;
	defaultShipSales = objects.shipSales;
	defaultOutfitSales = objects.outfitSales;
	defaultSubstitutions = objects.substitutions;
	defaultWormholes = objects.wormholes;
	playerGovernment = objects.governments.Get("Escort");

	politics.Reset();
}



void GameData::CheckReferences()
{
	objects.CheckReferences();
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



double GameData::GetProgress()
{
	// Cache progress completion seen, so clients are
	// isolated from the loading implementation details.
	static bool initiallyLoaded = false;
	if(initiallyLoaded)
		return 1.;

	double val = min(min(spriteQueue.GetProgress(), Audio::GetProgress()), objects.GetProgress());
	if(val >= 1.)
		initiallyLoaded = true;
	return val;
}



bool GameData::IsLoaded()
{
	return GetProgress() == 1.;
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



void GameData::ProcessSprites()
{
	spriteQueue.UploadSprites();
}



// Wait until all pending sprite uploads are completed.
void GameData::FinishLoadingSprites()
{
	spriteQueue.Finish();
}



// Get the list of resource sources (i.e. plugin folders).
const vector<string> &GameData::Sources()
{
	return sources;
}



// Get a reference to the UniverseObjects object.
UniverseObjects &GameData::Objects()
{
	return objects;
}



// Revert any changes that have been made to the universe.
void GameData::Revert()
{
	objects.fleets.Revert(defaultFleets);
	objects.governments.Revert(defaultGovernments);
	objects.planets.Revert(defaultPlanets);
	objects.systems.Revert(defaultSystems);
	objects.galaxies.Revert(defaultGalaxies);
	objects.shipSales.Revert(defaultShipSales);
	objects.outfitSales.Revert(defaultOutfitSales);
	objects.substitutions.Revert(defaultSubstitutions);
	objects.wormholes.Revert(defaultWormholes);
	for(auto &it : objects.persons)
		it.second.Restore();

	politics.Reset();
	purchases.clear();
}



void GameData::SetDate(const Date &date)
{
	for(auto &it : objects.systems)
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
					purchases[Systems().Get(grand.Token(0))][grand.Token(1)] += grand.Value(2);
		}
		else if(child.Token(0) == "system")
		{
			headings.clear();
			for(int index = 1; index < child.Size(); ++index)
				headings.push_back(child.Token(index));
		}
		else
		{
			System &system = *objects.systems.Get(child.Token(0));

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
	for(auto &it : objects.systems)
		it.second.StepEconomy();

	// Finally, send out the trade goods. This has to be done in a separate step
	// because otherwise whichever systems trade last would already have gotten
	// supplied by the other systems.
	for(auto &it : objects.systems)
	{
		System &system = it.second;
		if(!system.Links().empty())
			for(const Trade::Commodity &commodity : Commodities())
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
	objects.Change(node);
}



// Update the neighbor lists and other information for all the systems.
// This must be done any time that a change creates or moves a system.
void GameData::UpdateSystems()
{
	objects.UpdateSystems();
}



void GameData::AddJumpRange(double neighborDistance)
{
	objects.neighborDistances.insert(neighborDistance);
}



// Re-activate any special persons that were created previously but that are
// still alive.
void GameData::ResetPersons()
{
	for(auto &it : objects.persons)
		it.second.ClearPlacement();
}



// Mark all persons in the given list as dead.
void GameData::DestroyPersons(vector<string> &names)
{
	for(const string &name : names)
		objects.persons.Get(name)->Destroy();
}



const Set<Color> &GameData::Colors()
{
	return objects.colors;
}



const Set<Conversation> &GameData::Conversations()
{
	return objects.conversations;
}



const Set<Effect> &GameData::Effects()
{
	return objects.effects;
}




const Set<GameEvent> &GameData::Events()
{
	return objects.events;
}



const Set<Fleet> &GameData::Fleets()
{
	return objects.fleets;
}



const Set<FormationPattern> &GameData::Formations()
{
	return objects.formations;
}



const Set<Galaxy> &GameData::Galaxies()
{
	return objects.galaxies;
}



const Set<Government> &GameData::Governments()
{
	return objects.governments;
}



const Set<Hazard> &GameData::Hazards()
{
	return objects.hazards;
}



const Set<Interface> &GameData::Interfaces()
{
	return objects.interfaces;
}



const Set<Minable> &GameData::Minables()
{
	return objects.minables;
}




const Set<Mission> &GameData::Missions()
{
	return objects.missions;
}



const Set<News> &GameData::SpaceportNews()
{
	return objects.news;
}



const Set<Outfit> &GameData::Outfits()
{
	return objects.outfits;
}



const Set<Sale<Outfit>> &GameData::Outfitters()
{
	return objects.outfitSales;
}



const Set<Person> &GameData::Persons()
{
	return objects.persons;
}



const Set<Phrase> &GameData::Phrases()
{
	return objects.phrases;
}



const Set<Planet> &GameData::Planets()
{
	return objects.planets;
}



const Set<Ship> &GameData::Ships()
{
	return objects.ships;
}



const Set<Test> &GameData::Tests()
{
	return objects.tests;
}



const Set<TestData> &GameData::TestDataSets()
{
	return objects.testDataSets;
}



ConditionsStore &GameData::GlobalConditions()
{
	return globalConditions;
}



const Set<Sale<Ship>> &GameData::Shipyards()
{
	return objects.shipSales;
}



const Set<System> &GameData::Systems()
{
	return objects.systems;
}



const Set<Wormhole> &GameData::Wormholes()
{
	return objects.wormholes;
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
	return objects.startConditions;
}



const vector<Trade::Commodity> &GameData::Commodities()
{
	return objects.trade.Commodities();
}




const vector<Trade::Commodity> &GameData::SpecialCommodities()
{
	return objects.trade.SpecialCommodities();
}



// Custom messages to be shown when trying to land on certain stellar objects.
bool GameData::HasLandingMessage(const Sprite *sprite)
{
	return objects.landingMessages.count(sprite);
}



const string &GameData::LandingMessage(const Sprite *sprite)
{
	static const string EMPTY;
	auto it = objects.landingMessages.find(sprite);
	return (it == objects.landingMessages.end() ? EMPTY : it->second);
}



// Get the solar power and wind output of the given stellar object sprite.
double GameData::SolarPower(const Sprite *sprite)
{
	auto it = objects.solarPower.find(sprite);
	return (it == objects.solarPower.end() ? 0. : it->second);
}



double GameData::SolarWind(const Sprite *sprite)
{
	auto it = objects.solarWind.find(sprite);
	return (it == objects.solarWind.end() ? 0. : it->second);
}



// Strings for combat rating levels, etc.
const string &GameData::Rating(const string &type, int level)
{
	static const string EMPTY;
	auto it = objects.ratings.find(type);
	if(it == objects.ratings.end() || it->second.empty())
		return EMPTY;

	level = max(0, min<int>(it->second.size() - 1, level));
	return it->second[level];
}



// Collections for ship, bay type, outfit, and other categories.
const CategoryList &GameData::GetCategory(const CategoryType type)
{
	return objects.categories[type];
}



const StarField &GameData::Background()
{
	return background;
}



void GameData::SetHaze(const Sprite *sprite, bool allowAnimation)
{
	background.SetHaze(sprite, allowAnimation);
}



const string &GameData::Tooltip(const string &label)
{
	static const string EMPTY;
	auto it = objects.tooltips.find(label);
	// Special case: the "cost" and "sells for" labels include the percentage of
	// the full price, so they will not match exactly.
	if(it == objects.tooltips.end() && !label.compare(0, 4, "cost"))
		it = objects.tooltips.find("cost:");
	if(it == objects.tooltips.end() && !label.compare(0, 9, "sells for"))
		it = objects.tooltips.find("sells for:");
	return (it == objects.tooltips.end() ? EMPTY : it->second);
}



string GameData::HelpMessage(const string &name)
{
	static const string EMPTY;
	auto it = objects.helpMessages.find(name);
	return Command::ReplaceNamesWithKeys(it == objects.helpMessages.end() ? EMPTY : it->second);
}



const map<string, string> &GameData::HelpTemplates()
{
	return objects.helpMessages;
}



MaskManager &GameData::GetMaskManager()
{
	return maskManager;
}



const TextReplacements &GameData::GetTextReplacements()
{
	return objects.substitutions;
}



const Gamerules &GameData::GetGamerules()
{
	return objects.gamerules;
}



void GameData::LoadSources()
{
	sources.clear();
	sources.push_back(Files::Resources());

	vector<string> globalPlugins = Files::ListDirectories(Files::Resources() + "plugins/");
	for(const string &path : globalPlugins)
		if(Plugins::IsPlugin(path))
			LoadPlugin(path);

	vector<string> localPlugins = Files::ListDirectories(Files::Config() + "plugins/");
	for(const string &path : localPlugins)
		if(Plugins::IsPlugin(path))
			LoadPlugin(path);
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
		for(string &path : imageFiles)
			if(ImageSet::IsImage(path))
			{
				string name = ImageSet::Name(path.substr(start));

				shared_ptr<ImageSet> &imageSet = images[name];
				if(!imageSet)
					imageSet.reset(new ImageSet(name));
				imageSet->Add(std::move(path));
			}
	}
	return images;
}



// Thread-safe way to draw the menu background.
void GameData::DrawMenuBackground(Panel *panel)
{
	objects.DrawMenuBackground(panel);
}
