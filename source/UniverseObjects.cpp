/* UniverseObjects.cpp
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

#include "UniverseObjects.h"

#include "DataFile.h"
#include "DataNode.h"
#include "Files.h"
#include "Information.h"
#include "Logger.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "TaskQueue.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <utility>
#include <vector>

using namespace std;



shared_future<void> UniverseObjects::Load(TaskQueue &queue, const vector<string> &sources, bool debugMode)
{
	progress = 0.;

	// We need to copy any variables used for loading to avoid a race condition.
	// 'this' is not copied, so 'this' shouldn't be accessed after calling this
	// function (except for calling GetProgress which is safe due to the atomic).
	return queue.Run([this, sources, debugMode]() noexcept -> void
		{
			vector<string> files;
			for(const string &source : sources)
			{
				// Iterate through the paths starting with the last directory given. That
				// is, things in folders near the start of the path have the ability to
				// override things in folders later in the path.
				auto list = Files::RecursiveList(source + "data/");
				files.reserve(files.size() + list.size());
				files.insert(files.end(),
						make_move_iterator(list.begin()),
						make_move_iterator(list.end()));
			}

			const double step = 1. / (static_cast<int>(files.size()) + 1);
			for(const auto &path : files)
			{
				LoadFile(path, debugMode);

				// Increment the atomic progress by one step.
				// We use acquire + release to prevent any reordering.
				auto val = progress.load(memory_order_acquire);
				progress.store(val + step, memory_order_release);
			}
			FinishLoading();
			progress = 1.;
		});
}



double UniverseObjects::GetProgress() const
{
	return progress.load(memory_order_acquire);
}



void UniverseObjects::FinishLoading()
{
	for(auto &&it : systems)
		it.second.FinishLoading();
	for(auto &&it : planets)
		it.second.FinishLoading(wormholes);

	// Now that all data is loaded, update the neighbor lists and other
	// system information. Make sure that the default jump range is among the
	// neighbor distances to be updated.
	neighborDistances.insert(System::DEFAULT_NEIGHBOR_DISTANCE);
	UpdateSystems();

	// And, update the ships with the outfits we've now finished loading.
	for(auto &&it : ships)
		it.second.FinishLoading(true);
	for(auto &&it : persons)
		it.second.FinishLoading();

	// Calculate minable values.
	for(auto &&it : minables)
		it.second.FinishLoading();

	for(auto &&it : startConditions)
		it.FinishLoading();
	// Remove any invalid starting conditions, so the game does not use incomplete data.
	startConditions.erase(remove_if(startConditions.begin(), startConditions.end(),
			[](const StartConditions &it) noexcept -> bool { return !it.IsValid(); }),
		startConditions.end()
	);

	// Process any disabled game objects.
	for(const auto &category : disabled)
	{
		if(category.first == "mission")
			for(const string &name : category.second)
				missions.Get(name)->NeverOffer();
		else if(category.first == "event")
			for(const string &name : category.second)
				events.Get(name)->Disable();
		else if(category.first == "person")
			for(const string &name : category.second)
				persons.Get(name)->NeverSpawn();
		else
			Logger::LogError("Unhandled \"disable\" keyword of type \"" + category.first + "\"");
	}

	// Sort all category lists.
	for(auto &list : categories)
		list.second.Sort();
}



// Apply the given change to the universe.
void UniverseObjects::Change(const DataNode &node)
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
		planets.Get(node.Token(1))->Load(node, wormholes);
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
	else if(node.Token(0) == "substitutions" && node.HasChildren())
		substitutions.Load(node);
	else if(node.Token(0) == "wormhole" && node.Size() >= 2)
		wormholes.Get(node.Token(1))->Load(node);
	else
		node.PrintTrace("Error: Invalid \"event\" data:");
}



// Update the neighbor lists and other information for all the systems.
// (This must be done any time a GameEvent creates or moves a system.)
void UniverseObjects::UpdateSystems()
{
	for(auto &it : systems)
	{
		// Skip systems that have no name.
		if(it.first.empty() || it.second.TrueName().empty())
			continue;
		it.second.UpdateSystem(systems, neighborDistances);

		// If there were changes to a system there might have been a change to a legacy
		// wormhole which we must handle.
		for(const auto &object : it.second.Objects())
			if(object.GetPlanet())
				planets.Get(object.GetPlanet()->TrueName())->FinishLoading(wormholes);
	}
}



// Check for objects that are referred to but never defined. Some elements, like
// fleets, don't need to be given a name if undefined. Others (like outfits and
// planets) are written to the player's save and need a name to prevent data loss.
void UniverseObjects::CheckReferences()
{
	// Log a warning for an "undefined" class object that was never loaded from disk.
	auto Warn = [](const string &noun, const string &name)
	{
		Logger::LogError("Warning: " + noun + " \"" + name + "\" is referred to, but not fully defined.");
	};
	// Class objects with a deferred definition should still get named when content is loaded.
	auto NameIfDeferred = [](const set<string> &deferred, auto &it)
	{
		if(deferred.count(it.first))
			it.second.SetTrueName(it.first);
		else
			return false;

		return true;
	};
	// Set the name of an "undefined" class object, so that it can be written to the player's save.
	auto NameAndWarn = [=](const string &noun, auto &it)
	{
		it.second.SetTrueName(it.first);
		Warn(noun, it.first);
	};
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
		Logger::LogError("Error: the \"default intro\" conversation must contain a \"name\" node.");
	// Effects are serialized as a part of ships.
	for(auto &&it : effects)
		if(it.second.Name().empty())
			NameAndWarn("effect", it);
	// Fleets are not serialized. Any changes via events are written as DataNodes and thus self-define.
	for(auto &&it : fleets)
	{
		// Plugins may alter stock fleets with new variants that exclusively use plugin ships.
		// Rather than disable the whole fleet due to these non-instantiable variants, remove them.
		it.second.RemoveInvalidVariants();
		if(!it.second.IsValid() && !deferred["fleet"].count(it.first))
			Warn("fleet", it.first);
	}
	// Government names are used in mission NPC blocks and LocationFilters.
	for(auto &&it : governments)
		if(it.second.GetTrueName().empty() && !NameIfDeferred(deferred["government"], it))
			NameAndWarn("government", it);
	// Minables are not serialized.
	for(const auto &it : minables)
		if(it.second.TrueName().empty())
			Warn("minable", it.first);
	// Stock missions are never serialized, and an accepted mission is
	// always fully defined (though possibly not "valid").
	for(const auto &it : missions)
		if(it.second.Name().empty())
			Warn("mission", it.first);

	// News are never serialized or named, except by events (which would then define them).

	// Outfit names are used by a number of classes.
	for(auto &&it : outfits)
		if(it.second.TrueName().empty())
			NameAndWarn("outfit", it);
	// Outfitters are never serialized.
	for(const auto &it : outfitSales)
		if(it.second.empty() && !deferred["outfitter"].count(it.first))
			Logger::LogError("Warning: outfitter \"" + it.first + "\" is referred to, but has no outfits.");
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
		if(it.second.TrueModelName().empty())
		{
			it.second.SetTrueModelName(it.first);
			Warn("ship", it.first);
		}
	// Shipyards are never serialized.
	for(const auto &it : shipSales)
		if(it.second.empty() && !deferred["shipyard"].count(it.first))
			Logger::LogError("Warning: shipyard \"" + it.first + "\" is referred to, but has no ships.");
	// System names are used by a number of classes.
	for(auto &&it : systems)
		if(it.second.TrueName().empty() && !NameIfDeferred(deferred["system"], it))
			NameAndWarn("system", it);
	// Hazards are never serialized.
	for(const auto &it : hazards)
		if(!it.second.IsValid())
			Warn("hazard", it.first);
	// Wormholes are never serialized.
	for(const auto &it : wormholes)
		if(it.second.DisplayName().empty())
			Warn("wormhole", it.first);
	// Formation patterns are not serialized, but their usage is.
	for(auto &&it : formations)
		if(it.second.Name().empty())
			NameAndWarn("formation", it);
	// Any stock colors should have been loaded from game data files.
	for(const auto &it : colors)
		if(!it.second.IsLoaded())
			Warn("color", it.first);
}



void UniverseObjects::LoadFile(const string &path, bool debugMode)
{
	// This is an ordinary file. Check to see if it is an image.
	if(path.length() < 4 || path.compare(path.length() - 4, 4, ".txt"))
		return;

	DataFile data(path);
	if(debugMode)
		Logger::LogError("Parsing: " + path);

	for(const DataNode &node : data)
	{
		const string &key = node.Token(0);
		if(key == "color" && node.Size() >= 5)
			colors.Get(node.Token(1))->Load(
				node.Value(2), node.Value(3), node.Value(4), node.Size() >= 6 ? node.Value(5) : 1.);
		else if(key == "conversation" && node.Size() >= 2)
			conversations.Get(node.Token(1))->Load(node);
		else if(key == "effect" && node.Size() >= 2)
			effects.Get(node.Token(1))->Load(node);
		else if(key == "event" && node.Size() >= 2)
			events.Get(node.Token(1))->Load(node);
		else if(key == "fleet" && node.Size() >= 2)
			fleets.Get(node.Token(1))->Load(node);
		else if(key == "formation" && node.Size() >= 2)
			formations.Get(node.Token(1))->Load(node);
		else if(key == "galaxy" && node.Size() >= 2)
			galaxies.Get(node.Token(1))->Load(node);
		else if(key == "government" && node.Size() >= 2)
			governments.Get(node.Token(1))->Load(node);
		else if(key == "hazard" && node.Size() >= 2)
			hazards.Get(node.Token(1))->Load(node);
		else if(key == "interface" && node.Size() >= 2)
		{
			interfaces.Get(node.Token(1))->Load(node);

			// If we modified the "menu background" interface, then
			// we also update our cache of it.
			if(node.Token(1) == "menu background")
			{
				lock_guard<mutex> lock(menuBackgroundMutex);
				menuBackgroundCache.Load(node);
			}
		}
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
			planets.Get(node.Token(1))->Load(node, wormholes);
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
					child.PrintTrace("Skipping unrecognized attribute:");
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
		else if(key == "category" && node.Size() >= 2)
		{
			static const map<string, CategoryType> category = {
				{"ship", CategoryType::SHIP},
				{"bay type", CategoryType::BAY},
				{"outfit", CategoryType::OUTFIT},
				{"series", CategoryType::SERIES}
			};
			auto it = category.find(node.Token(1));
			if(it == category.end())
			{
				node.PrintTrace("Skipping unrecognized category type:");
				continue;
			}
			categories[it->second].Load(node);
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
		else if(key == "substitutions" && node.HasChildren())
			substitutions.Load(node);
		else if(key == "wormhole" && node.Size() >= 2)
			wormholes.Get(node.Token(1))->Load(node);
		else if(key == "gamerules" && node.HasChildren())
			gamerules.Load(node);
		else if(key == "disable" && node.Size() >= 2)
		{
			static const set<string> canDisable = {"mission", "event", "person"};
			const string &category = node.Token(1);
			if(canDisable.count(category))
			{
				if(node.HasChildren())
					for(const DataNode &child : node)
						disabled[category].emplace(child.Token(0));
				if(node.Size() >= 3)
					for(int index = 2; index < node.Size(); ++index)
						disabled[category].emplace(node.Token(index));
			}
			else
				node.PrintTrace("Invalid use of keyword \"disable\" for class \"" + category + "\"");
		}
		else
			node.PrintTrace("Skipping unrecognized root object:");
	}
}



void UniverseObjects::DrawMenuBackground(Panel *panel) const
{
	lock_guard<mutex> lock(menuBackgroundMutex);
	menuBackgroundCache.Draw(Information(), panel);
}
