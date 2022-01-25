/* ShipsFactory.cpp
Copyright (c) 2022 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipsFactory.h"

#include "DataNode.h"
#include "Files.h"
#include "GameData.h"
#include "SpriteSet.h"
#include "UniverseObjects.h"

#include <algorithm>

using namespace std;

namespace {
	// Get an overview of how many weapon-outfits are equipped.
	map<const Outfit *, int> GetEquipped(const vector<Hardpoint> &weapons)
	{
		map<const Outfit *, int> equipped;
		for(const Hardpoint &hardpoint : weapons)
			if(hardpoint.GetOutfit())
				++equipped[hardpoint.GetOutfit()];
		return equipped;
	}
}



ShipsFactory::ShipsFactory(UniverseObjects &universe): universe(universe)
{
}



// Load a ship from a datafile. Creation of the Ship object itself has
// already been done by the caller. Getting only the DataNode as parameter
// and returning a shared_ptr<Ship> is nicer than getting the ship as
// parameter by reference, but the ES code allows overwriting ship-
// definitions by a new load, so we need to support overwriting existing
// ship definitions here.
void ShipsFactory::LoadShip(Ship &ship, const DataNode &node) const
{
	if(node.Size() >= 2)
	{
		ship.SetModelName(node.Token(1));
		ship.SetPluralModelName(node.Token(1) + 's');
	}
	if(node.Size() >= 3)
	{
		ship.SetBaseModel(GameData::Ships().Get(ship.ModelName()));
		ship.SetVariantName(node.Token(2));
	}
	ship.SetIsDefined(true);

	ship.SetGovernment(GameData::PlayerGovernment());

	// Note: I do not clear the attributes list here so that it is permissible
	// to override one ship definition with another.
	bool hasEngine = false;
	bool hasArmament = false;
	bool hasBays = false;
	bool hasExplode = false;
	bool hasLeak = false;
	bool hasFinalExplode = false;
	bool hasOutfits = false;
	bool hasDescription = false;
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool add = (key == "add");
		if(add && (child.Size() < 2 || child.Token(1) != "attributes"))
		{
			child.PrintTrace("Skipping invalid use of 'add' with " + (child.Size() < 2
					? "no key." : "key: " + child.Token(1)));
			continue;
		}
		if(key == "sprite")
			ship.LoadSprite(child);
		else if(child.Token(0) == "thumbnail" && child.Size() >= 2)
			ship.SetThumbnail(SpriteSet::Get(child.Token(1)));
		else if(key == "name" && child.Size() >= 2)
			ship.SetName(child.Token(1));
		else if(key == "plural" && child.Size() >= 2)
			ship.SetPluralModelName(child.Token(1));
		else if(key == "noun" && child.Size() >= 2)
			ship.SetNoun(child.Token(1));
		else if(key == "swizzle" && child.Size() >= 2)
			ship.SetCustomSwizzle(child.Value(1));
		else if(key == "uuid" && child.Size() >= 2)
		{
			auto uuid = EsUuid::FromString(child.Token(1));
			ship.SetUUID(uuid);
		}
		else if(key == "attributes" || add)
		{
			if(!add)
				ship.BaseAttributes().Load(child);
			else
			{
				ship.SetAddAttributes(true);
				ship.Attributes().Load(child);
			}
		}
		else if((key == "engine" || key == "reverse engine" || key == "steering engine") && child.Size() >= 3)
		{
			if(!hasEngine)
			{
				ship.EnginePoints().clear();
				ship.ReverseEnginePoints().clear();
				ship.SteeringEnginePoints().clear();
				hasEngine = true;
			}
			bool reverse = (key == "reverse engine");
			bool steering = (key == "steering engine");

			vector<Ship::EnginePoint> &editPoints = (!steering && !reverse) ? ship.EnginePoints() :
				(reverse ? ship.ReverseEnginePoints() : ship.SteeringEnginePoints());
			editPoints.emplace_back(0.5 * child.Value(1), 0.5 * child.Value(2),
				(child.Size() > 3 ? child.Value(3) : 1.));
			Ship::EnginePoint &engine = editPoints.back();
			if(reverse)
				engine.facing = Angle(180.);
			for(const DataNode &grand : child)
			{
				const string &grandKey = grand.Token(0);
				if(grandKey == "zoom" && grand.Size() >= 2)
					engine.zoom = grand.Value(1);
				else if(grandKey == "angle" && grand.Size() >= 2)
					engine.facing += Angle(grand.Value(1));
				else
					Ship::HandleEngineToken(engine, steering, grandKey);
			}
		}
		else if(key == "gun" || key == "turret")
		{
			if(!hasArmament)
			{
				ship.GetArmament().Clear();
				hasArmament = true;
			}
			const Outfit *outfit = nullptr;
			Point hardpoint;
			if(child.Size() >= 3)
			{
				hardpoint = Point(child.Value(1), child.Value(2));
				if(child.Size() >= 4)
					outfit = GameData::Outfits().Get(child.Token(3));
			}
			else
			{
				if(child.Size() >= 2)
					outfit = GameData::Outfits().Get(child.Token(1));
			}
			Angle gunPortAngle = Angle(0.);
			bool gunPortParallel = false;
			bool drawUnder = (key == "gun");
			if(child.HasChildren())
			{
				for(const DataNode &grand : child)
				{
					if(grand.Token(0) == "angle" && grand.Size() >= 2)
						gunPortAngle = grand.Value(1);
					else if(grand.Token(0) == "parallel")
						gunPortParallel = true;
					else if(grand.Token(0) == "under")
						drawUnder = true;
					else if(grand.Token(0) == "over")
						drawUnder = false;
					else
						grand.PrintTrace("Skipping unrecognized attribute:");
				}
			}
			if(key == "gun")
				ship.GetArmament().AddGunPort(hardpoint, gunPortAngle, gunPortParallel, drawUnder, outfit);
			else
				ship.GetArmament().AddTurret(hardpoint, drawUnder, outfit);
			// Print a warning for the first hardpoint after 32, i.e. only 1 warning per ship.
			if(ship.GetArmament().Get().size() == 33)
				child.PrintTrace("Warning: ship has more than 32 weapon hardpoints. Some weapons may not fire:");
		}
		else if(key == "never disabled")
			ship.SetNeverDisabled(true);
		else if(key == "uncapturable")
			ship.SetCapturable(false);
		else if(((key == "fighter" || key == "drone") && child.Size() >= 3) ||
			(key == "bay" && child.Size() >= 4))
		{
			// While the `drone` and `fighter` keywords are supported for backwards compatibility, the
			// standard format is `bay <ship-category>`, with the same signature for other values.
			string category = "Fighter";
			int childOffset = 0;
			if(key == "drone")
				category = "Drone";
			else if(key == "bay")
			{
				category = child.Token(1);
				childOffset += 1;
			}

			if(!hasBays)
			{
				ship.Bays().clear();
				hasBays = true;
			}
			ship.Bays().emplace_back(child.Value(1 + childOffset), child.Value(2 + childOffset), category);
			Ship::Bay &bay = ship.Bays().back();
			for(int i = 3 + childOffset; i < child.Size(); ++i)
				Ship::HandleBayToken(bay, child.Token(i));
			if(child.HasChildren())
				for(const DataNode &grand : child)
				{
					// Load in the effect(s) to be displayed when the ship launches.
					if(grand.Token(0) == "launch effect" && grand.Size() >= 2)
					{
						int count = grand.Size() >= 3 ? static_cast<int>(grand.Value(2)) : 1;
						const Effect *e = GameData::Effects().Get(grand.Token(1));
						bay.launchEffects.insert(bay.launchEffects.end(), count, e);
					}
					else if(grand.Token(0) == "angle" && grand.Size() >= 2)
						bay.facing = Angle(grand.Value(1));
					else
					{
						bool handled = Ship::HandleBayToken(bay, grand.Token(0));
						if(!handled)
							grand.PrintTrace("Skipping unrecognized attribute:");
					}
				}
		}
		else if(key == "leak" && child.Size() >= 2)
		{
			if(!hasLeak)
			{
				ship.Leaks().clear();
				hasLeak = true;
			}
			Ship::Leak leak(GameData::Effects().Get(child.Token(1)));
			if(child.Size() >= 3)
				leak.openPeriod = child.Value(2);
			if(child.Size() >= 4)
				leak.closePeriod = child.Value(3);
			ship.Leaks().push_back(leak);
		}
		else if(key == "explode" && child.Size() >= 2)
		{
			if(!hasExplode)
			{
				ship.ClearExplosionEffects();
				hasExplode = true;
			}
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			ship.AddExplosionEffect(GameData::Effects().Get(child.Token(1)), count);
		}
		else if(key == "final explode" && child.Size() >= 2)
		{
			if(!hasFinalExplode)
			{
				ship.ClearFinalExplosions();
				hasFinalExplode = true;
			}
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			ship.AddFinalExplosion(GameData::Effects().Get(child.Token(1)), count);
		}
		else if(key == "outfits")
		{
			if(!hasOutfits)
			{
				ship.Outfits().clear();
				hasOutfits = true;
			}
			for(const DataNode &grand : child)
			{
				int count = (grand.Size() >= 2) ? grand.Value(1) : 1;
				if(count > 0)
					ship.Outfits()[GameData::Outfits().Get(grand.Token(0))] += count;
				else
					grand.PrintTrace("Skipping invalid outfit count:");
			}

			// Verify we have at least as many installed outfits as were identified as "equipped."
			// If not (e.g. a variant definition), ensure FinishLoading equips into a blank slate.
			if(!hasArmament)
				for(const auto &pair : GetEquipped(ship.Weapons()))
				{
					auto it = ship.Outfits().find(pair.first);
					if(it == ship.Outfits().end() || it->second < pair.second)
					{
						ship.GetArmament().UninstallAll();
						break;
					}
				}
		}
		else if(key == "cargo")
			ship.Cargo().Load(child);
		else if(key == "crew" && child.Size() >= 2)
			ship.SetCrew(static_cast<int>(child.Value(1)));
		else if(key == "fuel" && child.Size() >= 2)
			ship.SetFuel(child.Value(1));
		else if(key == "shields" && child.Size() >= 2)
			ship.SetShields(child.Value(1));
		else if(key == "hull" && child.Size() >= 2)
			ship.SetHull(child.Value(1));
		else if(key == "position" && child.Size() >= 3)
			ship.SetPosition(Point(child.Value(1), child.Value(2)));
		else if(key == "system" && child.Size() >= 2)
			ship.SetSystem(GameData::Systems().Get(child.Token(1)));
		else if(key == "planet" && child.Size() >= 2)
			ship.SetPlanet(GameData::Planets().Get(child.Token(1)));
		else if(key == "destination system" && child.Size() >= 2)
			ship.SetTargetSystem(GameData::Systems().Get(child.Token(1)));
		else if(key == "parked")
			ship.SetIsParked(true);
		else if(key == "description" && child.Size() >= 2)
		{
			if(!hasDescription)
			{
				ship.Description().clear();
				hasDescription = true;
			}
			ship.Description() += child.Token(1) + '\n';
		}
		else if(key != "actions")
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



// When loading a ship, some of the outfits it lists may not have been
// loaded yet. So, wait until everything has been loaded, then call this.
void ShipsFactory::FinishLoading(Ship &ship, bool isNewInstance) const
{
	const Ship *model = nullptr;
	if(GameData::Ships().Has(ship.ModelName()))
		model = GameData::Ships().Get(ship.ModelName());
	ship.FinishLoading(isNewInstance, model);

	// Ensure that all defined bays are of a valid category. Remove and warn about any
	// invalid bays. Add a default "launch effect" to any remaining internal bays if
	// this ship is crewed (i.e. pressurized).
	string warning;
	const auto &bayCategories = GameData::Category(CategoryType::BAY);
	for(auto it = ship.Bays().begin(); it != ship.Bays().end(); )
	{
		Ship::Bay &bay = *it;
		if(find(bayCategories.begin(), bayCategories.end(), bay.category) == bayCategories.end())
		{
			warning += "Invalid bay category: " + bay.category + "\n";
			it = ship.Bays().erase(it);
			continue;
		}
		else
			++it;
		if(bay.side == Ship::Bay::INSIDE && bay.launchEffects.empty() && ship.Crew())
			bay.launchEffects.emplace_back(GameData::Effects().Get("basic launch"));
	}
	ship.SetCanBeCarried(find(bayCategories.begin(), bayCategories.end(), ship.Attributes().Category()) != bayCategories.end());

	if(!warning.empty())
	{
		// Print the invalid Bay warning if we encountered an invalid bay.
		string message = (!ship.Name().empty() ? "Ship \"" + ship.Name() + "\" " : "") + "(" + ship.VariantName() + "):\n";
		Files::LogError(message + warning);
	}

	// Load the default effects for this ship.
	ship.SetEffectIonSpark(GameData::Effects().Get("ion spark"));
	ship.SetEffectDisruptionSpark(GameData::Effects().Get("disruption spark"));
	ship.SetEffectSlowingSpark(GameData::Effects().Get("slowing spark"));
	ship.SetEffectDischargeSpark(GameData::Effects().Get("discharge spark"));
	ship.SetEffectCorrosionSpark(GameData::Effects().Get("corrosion spark"));
	ship.SetEffectLeakageSpark(GameData::Effects().Get("leakage spark"));
	ship.SetEffectBurningSpark(GameData::Effects().Get("burning spark"));
	ship.SetEffectSmoke(GameData::Effects().Get("smoke"));
	ship.SetEffectJumpDrive(GameData::Effects().Get("jump drive"));	
}
