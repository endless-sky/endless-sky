/* GameObjects.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameObjects.h"

#include "CategoryTypes.h"
#include "Conversation.h"
#include "DataFile.h"
#include "Effect.h"
#include "Files.h"
#include "Fleet.h"
#include "Galaxy.h"
#include "GameEvent.h"
#include "Government.h"
#include "Hazard.h"
#include "Interface.h"
#include "Minable.h"
#include "Mission.h"
#include "News.h"
#include "Person.h"
#include "Planet.h"
#include "Phrase.h"
#include "Ship.h"
#include "SpriteSet.h"
#include "StartConditions.h"
#include "System.h"
#include "Test.h"
#include "TestData.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace {
	// TODO (C++14): make these 3 methods generic lambdas visible only to the CheckReferences method.
	// Log a warning for an "undefined" class object that was never loaded from disk.
	void Warn(const string &noun, const string &name)
	{
		Files::LogError("Warning: " + noun + " \"" + name + "\" is referred to, but not fully defined.");
	}
	// Class objects with a deferred definition should still get named when content is loaded.
	template <class Type>
	bool NameIfDeferred(const set<string> &deferred, pair<const string, Type> &it)
	{
		if(deferred.count(it.first))
		{
			it.second.SetName(it.first);
			return true;
		}
		return false;
	}
	// Set the name of an "undefined" class object, so that it can be written to the player's save.
	template <class Type>
	void NameAndWarn(const string &noun, pair<const string, Type> &it)
	{
		it.second.SetName(it.first);
		Warn(noun, it.first);
	}
}



void GameObjects::Load(const DataFile &data)
{
	for(const DataNode &node : data)
		Load(node);
}



void GameObjects::Load(const DataNode &node)
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
		testDataSets.Get(node.Token(1))->Load(node);
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
	else if(key == "category" && node.Size() >= 2)
	{
		static const map<string, CategoryType> category = {
			{"ship", CategoryType::SHIP},
			{"bay type", CategoryType::BAY},
			{"outfit", CategoryType::OUTFIT}
		};
		auto it = category.find(node.Token(1));
		if(it == category.end())
		{
			node.PrintTrace("Skipping unrecognized category:");
			return;
		}

		vector<string> &categoryList = categories[it->second];
		for(const DataNode &child : node)
		{
			// If a given category already exists, it will be
			// moved to the back of the list.
			const auto it = find(categoryList.begin(), categoryList.end(), child.Token(0));
			if(it != categoryList.end())
				categoryList.erase(it);
			categoryList.push_back(child.Token(0));
		}
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



// Apply the given change to the universe.
void GameObjects::Change(const DataNode &node)
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



// Check for objects that are referred to but never defined. Some elements, like
// fleets, don't need to be given a name if undefined. Others (like outfits and
// planets) are written to the player's save and need a name to prevent data loss.
void GameObjects::CheckReferences()
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



void GameObjects::FinishLoading()
{
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
}
