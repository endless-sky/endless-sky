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

#include "audio/Audio.h"
#include "shader/BatchShader.h"
#include "CategoryList.h"
#include "Color.h"
#include "Command.h"
#include "ConditionsStore.h"
#include "Conversation.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Effect.h"
#include "Files.h"
#include "shader/FillShader.h"
#include "Fleet.h"
#include "shader/FogShader.h"
#include "text/FontSet.h"
#include "FormationPattern.h"
#include "Galaxy.h"
#include "GameEvent.h"
#include "Government.h"
#include "Hazard.h"
#include "image/ImageSet.h"
#include "Interface.h"
#include "shader/LineShader.h"
#include "image/MaskManager.h"
#include "Minable.h"
#include "Mission.h"
#include "audio/Music.h"
#include "News.h"
#include "Outfit.h"
#include "shader/OutlineShader.h"
#include "Person.h"
#include "Phrase.h"
#include "Planet.h"
#include "Plugins.h"
#include "shader/PointerShader.h"
#include "Politics.h"
#include "RenderBuffer.h"
#include "shader/RingShader.h"
#include "Ship.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "shader/StarField.h"
#include "StartConditions.h"
#include "System.h"
#include "TaskQueue.h"
#include "test/Test.h"
#include "test/TestData.h"
#include "UniverseObjects.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <iostream>
#include <queue>
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
	Set<Shop<Ship>> defaultShipSales;
	Set<Shop<Outfit>> defaultOutfitSales;
	Set<Wormhole> defaultWormholes;
	Set<Person> defaultPersons;
	TextReplacements defaultSubstitutions;

	Politics politics;

	StarField background;

	vector<filesystem::path> sources;
	map<const Sprite *, shared_ptr<ImageSet>> deferred;
	map<const Sprite *, int> preloaded;

	MaskManager maskManager;

	const Government *playerGovernment = nullptr;
	map<const System *, map<string, int>> purchases;

	ConditionsStore globalConditions;

	bool preventSpriteUpload = false;

	// Tracks the progress of loading the sprites when the game starts.
	std::atomic<bool> queuedAllImages = false;
	std::atomic<int> spritesLoaded = 0;
	std::atomic<int> totalSprites = 0;

	// List of image sets that are waiting to be uploaded to the GPU.
	mutex imageQueueMutex;
	queue<shared_ptr<ImageSet>> imageQueue;

	// Loads a sprite and queues it for upload to the GPU.
	void LoadSprite(TaskQueue &queue, const shared_ptr<ImageSet> &image)
	{
		queue.Run([image] { image->Load(); },
			[image] { image->Upload(SpriteSet::Modify(image->Name()), !preventSpriteUpload); });
	}

	void LoadSpriteQueued(TaskQueue &queue, const shared_ptr<ImageSet> &image);
	// Loads a sprite from the image queue, recursively.
	void LoadSpriteQueued(TaskQueue &queue)
	{
		if(imageQueue.empty())
			return;

		// Start loading the next image in the list.
		// This is done to save memory on startup.
		LoadSpriteQueued(queue, imageQueue.front());
		imageQueue.pop();
	}

	// Loads a sprite from the given image, with progress tracking.
	// Recursively loads the next image in the queue, if any.
	void LoadSpriteQueued(TaskQueue &queue, const shared_ptr<ImageSet> &image)
	{
		queue.Run([image] { image->Load(); },
			[image, &queue]
			{
				image->Upload(SpriteSet::Modify(image->Name()), !preventSpriteUpload);
				++spritesLoaded;

				// Start loading the next image in the queue, if any.
				lock_guard lock(imageQueueMutex);
				LoadSpriteQueued(queue);
			});
	}

	void LoadPlugin(TaskQueue &queue, const filesystem::path &path)
	{
		const auto *plugin = Plugins::Load(path);
		if(!plugin)
			return;

		if(plugin->enabled)
			sources.push_back(path);

		// Load the icon for the plugin, if any.
		auto icon = make_shared<ImageSet>(plugin->name);

		// Try adding all the possible icon variants.
		for(const string &extension : ImageBuffer::ImageExtensions())
		{
			filesystem::path iconPath = path / ("icon" + extension);
			if(Files::Exists(iconPath))
			{
				icon->Add(iconPath);
				break;
			}
		}
		for(const string &extension : ImageBuffer::ImageExtensions())
		{
			filesystem::path iconPath = path / ("icon@2x" + extension);
			if(Files::Exists(iconPath))
			{
				icon->Add(iconPath);
				break;
			}
		}

		if(!icon->IsEmpty())
		{
			icon->ValidateFrames();
			LoadSprite(queue, icon);
		}
	}
}



shared_future<void> GameData::BeginLoad(TaskQueue &queue, const PlayerInfo &player,
		bool onlyLoadData, bool debugMode, bool preventUpload)
{
	preventSpriteUpload = preventUpload;

	// Initialize the list of "source" folders based on any active plugins.
	LoadSources(queue);

	if(!onlyLoadData)
	{
		queue.Run([&queue] {
			// Now, read all the images in all the path directories. For each unique
			// name, only remember one instance, letting things on the higher priority
			// paths override the default images.
			map<string, shared_ptr<ImageSet>> images = FindImages();

			// From the name, strip out any frame number, plus the extension.
			for(auto &it : images)
			{
				// This should never happen, but just in case:
				if(!it.second)
					continue;

				// Reduce the set of images to those that are valid.
				it.second->ValidateFrames();
				// For landscapes, remember all the source files but don't load them yet.
				if(ImageSet::IsDeferred(it.first))
					deferred[SpriteSet::Get(it.first)] = std::move(it.second);
				else
				{
					lock_guard lock(imageQueueMutex);
					imageQueue.push(std::move(std::move(it.second)));
					++totalSprites;
				}
			}
			queuedAllImages = true;

			// Launch the tasks to actually load the images, making sure not to exceed the amount
			// of tasks the main thread can handle in a single frame to limit peak memory usage.
			{
				lock_guard lock(imageQueueMutex);
				for(int i = 0; i < TaskQueue::MAX_SYNC_TASKS; ++i)
					LoadSpriteQueued(queue);
			}

			// Generate a catalog of music files.
			Music::Init(sources);
		});
	}

	return objects.Load(queue, sources, player, &globalConditions, debugMode);
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
	defaultPersons = objects.persons;
	playerGovernment = objects.governments.Get("Escort");

	politics.Reset();
	background.FinishLoading();
}



void GameData::CheckReferences()
{
	objects.CheckReferences();
}



void GameData::LoadSettings()
{
	// Load the key settings.
	Command::LoadSettings(Files::Resources() / "keys.txt");
	Command::LoadSettings(Files::Config() / "keys.txt");
}



void GameData::LoadShaders()
{
	// The found shader files. The first element is the vertex shader,
	// the second is the fragment shader.
	map<string, pair<string, string>> loaded;
	for(const filesystem::path &source : sources)
	{
		filesystem::path base = source / "shaders";
		if(Files::Exists(base))
			for(filesystem::path shaderFile : Files::RecursiveList(base))
			{
				filesystem::path shader = shaderFile;
#ifdef ES_GLES
				// Allow specifying different shaders for GL and GLES.
				// In this case, only the appropriate shader is loaded.
				if(shaderFile.extension() == ".gles")
					shader = shader.parent_path() / shader.stem();
#else
				if(shaderFile.extension() == ".gl")
					shader = shader.parent_path() / shader.stem();
#endif
				string name = (shader.parent_path() / shader.stem()).lexically_relative(base).generic_string();
				if(shader.extension() == ".vert")
					loaded[name].first = shaderFile.string();
				else if(shader.extension() == ".frag")
					loaded[name].second = shaderFile.string();
			}
	}

	// If there is both a fragment and a vertex shader available,
	// it can be turned into a shader object.
	for(const auto &[key, s] : loaded)
		if(!s.first.empty() && !s.second.empty())
			objects.shaders.Get(key)->Load(Files::Read(s.first).c_str(), Files::Read(s.second).c_str());

	FillShader::Init();
	FogShader::Init();
	LineShader::Init();
	OutlineShader::Init();
	PointerShader::Init();
	RingShader::Init();
	SpriteShader::Init();
	BatchShader::Init();
	RenderBuffer::Init();

	FontSet::Add(Files::Images() / "font/ubuntu14r.png", 14);
	FontSet::Add(Files::Images() / "font/ubuntu18r.png", 18);

	background.Init(16384, 4096);
}



double GameData::GetProgress()
{
	double spriteProgress = 0.;
	if(queuedAllImages)
	{
		if(!totalSprites)
			spriteProgress = 1.;
		else
			spriteProgress = static_cast<double>(spritesLoaded) / totalSprites;
	}
	return min({spriteProgress, Audio::GetProgress(), objects.GetProgress()});
}



bool GameData::IsLoaded()
{
	return GetProgress() == 1.;
}



// Begin loading a sprite that was previously deferred. Currently this is
// done with all landscapes to speed up the program's startup.
void GameData::Preload(TaskQueue &queue, const Sprite *sprite)
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
	pit = preloaded.begin();
	while(pit != preloaded.end())
	{
		++pit->second;
		if(pit->second >= 20)
		{
			// Unloading needs to be queued on the main thread.
			queue.Run({}, [name = pit->first->Name()] { SpriteSet::Modify(name)->Unload(); });
			pit = preloaded.erase(pit);
		}
		else
			++pit;
	}

	// Now, load all the files for this sprite.
	preloaded[sprite] = 0;
	LoadSprite(queue, dit->second);
}



// Get the list of resource sources (i.e. plugin folders).
const vector<filesystem::path> &GameData::Sources()
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
	objects.persons.Revert(defaultPersons);
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
		const string &key = child.Token(0);
		if(key == "purchases")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 3 && grand.Value(2))
					purchases[Systems().Get(grand.Token(0))][grand.Token(1)] += grand.Value(2);
		}
		else if(key == "system")
		{
			headings.clear();
			for(int index = 1; index < child.Size(); ++index)
				headings.push_back(child.Token(index));
		}
		else
		{
			System &system = *objects.systems.Get(key);

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
					{ return lhs->first->TrueName() < rhs->first->TrueName(); },
				[&out](const Purchase &pit)
				{
					// Write purchases for all systems, even ones from removed plugins.
					for(const auto &cit : pit.second)
						out.Write(pit.first->TrueName(), cit.first, cit.second);
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

			out.WriteToken(sit.second.TrueName());
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
void GameData::Change(const DataNode &node, PlayerInfo &player)
{
	objects.Change(node, player);
}



// Update the neighbor lists and other information for all the systems.
// This must be done any time that a change creates or moves a system.
void GameData::UpdateSystems()
{
	objects.UpdateSystems();
}



void GameData::RecomputeWormholeRequirements()
{
	objects.RecomputeWormholeRequirements();
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



const Set<Swizzle> &GameData::Swizzles()
{
	return objects.swizzles;
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



const Set<Message::Category> &GameData::MessageCategories()
{
	return objects.messageCategories;
}



const Set<Message> &GameData::Messages()
{
	return objects.messages;
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



const Set<Shop<Outfit>> &GameData::Outfitters()
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



const Set<Shader> &GameData::Shaders()
{
	return objects.shaders;
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



const Set<Shop<Ship>> &GameData::Shipyards()
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



const std::set<std::string> &GameData::UniverseWormholeRequirements()
{
	return objects.universeWormholeRequirements;
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
	return objects.landingMessages.contains(sprite);
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



// Get the map icon of the given stellar object sprite.
const Sprite *GameData::StarIcon(const Sprite *sprite)
{
	const auto it = objects.starIcons.find(sprite);
	return (it == objects.starIcons.end() ? nullptr : it->second);
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



void GameData::StepBackground(const Point &vel, double zoom)
{
	background.Step(vel, zoom);
}



const Point &GameData::GetBackgroundPosition()
{
	return background.Position();
}



void GameData::SetBackgroundPosition(const Point &position)
{
	background.SetPosition(position);
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
	if(it == objects.tooltips.end() && label.starts_with("cost"))
		it = objects.tooltips.find("cost:");
	if(it == objects.tooltips.end() && label.starts_with("sells for"))
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



void GameData::LoadSources(TaskQueue &queue)
{
	sources.clear();
	sources.push_back(Files::Resources());

	vector<filesystem::path> globalPlugins = Files::ListDirectories(Files::GlobalPlugins());
	for(const auto &path : globalPlugins)
		if(Plugins::IsPlugin(path))
			LoadPlugin(queue, path);
	// Load unzipped plugins first to give them precedence, then load the zipped plugins.
	globalPlugins = Files::List(Files::GlobalPlugins());
	for(const auto &path : globalPlugins)
		if(path.extension() == ".zip" && Plugins::IsPlugin(path))
			LoadPlugin(queue, path);

	vector<filesystem::path> localPlugins = Files::ListDirectories(Files::UserPlugins());
	for(const auto &path : localPlugins)
		if(Plugins::IsPlugin(path))
			LoadPlugin(queue, path);
	localPlugins = Files::List(Files::UserPlugins());
	for(const auto &path : localPlugins)
		if(path.extension() == ".zip" && Plugins::IsPlugin(path))
			LoadPlugin(queue, path);
}



map<string, shared_ptr<ImageSet>> GameData::FindImages()
{
	map<string, shared_ptr<ImageSet>> images;
	for(const auto &source : sources)
	{
		// All names will only include the portion of the path that comes after
		// this directory prefix.
		filesystem::path directoryPath = source / "images";

		vector<filesystem::path> imageFiles = Files::RecursiveList(directoryPath);
		for(auto &path : imageFiles)
			if(ImageSet::IsImage(path))
			{
				ImageFileData data(path, directoryPath);

				shared_ptr<ImageSet> &imageSet = images[data.name];
				if(!imageSet)
					imageSet.reset(new ImageSet(data.name));
				imageSet->Add(std::move(data));
			}
	}
	return images;
}



// Thread-safe way to draw the menu background.
void GameData::DrawMenuBackground(Panel *panel)
{
	objects.DrawMenuBackground(panel);
}
