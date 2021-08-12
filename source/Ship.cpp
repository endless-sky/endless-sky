/* Ship.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Ship.h"

#include "Audio.h"
#include "CategoryTypes.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Effect.h"
#include "Files.h"
#include "Flotsam.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Mask.h"
#include "Messages.h"
#include "Phrase.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Projectile.h"
#include "Random.h"
#include "ShipEvent.h"
#include "Sound.h"
#include "SpriteSet.h"
#include "Sprite.h"
#include "StellarObject.h"
#include "System.h"
#include "Visual.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

using namespace std;

namespace {
	const string FIGHTER_REPAIR = "Repair fighters in";
	const vector<string> BAY_SIDE = {"inside", "over", "under"};
	const vector<string> BAY_FACING = {"forward", "left", "right", "back"};
	const vector<Angle> BAY_ANGLE = {Angle(0.), Angle(-90.), Angle(90.), Angle(180.)};
	
	const vector<string> ENGINE_SIDE = {"under", "over"};
	const vector<string> STEERING_FACING = {"none", "left", "right"};
	
	const double MAXIMUM_TEMPERATURE = 100.;
	
	const double SCAN_TIME = 60.;
	
	// Helper function to transfer energy to a given stat if it is less than the
	// given maximum value.
	void DoRepair(double &stat, double &available, double maximum)
	{
		double transfer = max(0., min(available, maximum - stat));
		stat += transfer;
		available -= transfer;
	}
	
	// Helper function to repair a given stat up to its maximum, limited by
	// how much repair is available and how much energy, fuel, and heat are available.
	// Updates the stat, the available amount, and the energy, fuel, and heat amounts.
	void DoRepair(double &stat, double &available, double maximum, double &energy, double energyCost, double &fuel, double fuelCost, double &heat, double heatCost)
	{
		if(available <= 0. || stat >= maximum)
			return;
		
		// Energy, heat, and fuel costs are the energy, fuel, or heat required per unit repaired.
		if(energyCost > 0.)
			available = min(available, energy / energyCost);
		if(fuelCost > 0.)
			available = min(available, fuel / fuelCost);
		if(heatCost < 0.)
			available = min(available, heat / -heatCost);
		
		double transfer = min(available, maximum - stat);
		if(transfer > 0.)
		{
			stat += transfer;
			available -= transfer;
			energy -= transfer * energyCost;
			fuel -= transfer * fuelCost;
			heat += transfer * heatCost;
		}
	}
	
	// Helper function to reduce a given status effect according 
	// to its resistance, limited by how much energy, fuel, and heat are available.
	// Updates the stat and the energy, fuel, and heat amounts.
	void DoStatusEffect(bool isDeactivated, double &stat, double resistance, double &energy, double energyCost, double &fuel, double fuelCost, double &heat, double heatCost)
	{
		if(isDeactivated || resistance <= 0.)
		{
			stat = .99 * stat;
			return;
		}
		
		// Calculate how much resistance can be used assuming no
		// energy or fuel cost.
		resistance = .99 * stat - max(0., .99 * stat - resistance);
		
		// Limit the resistance by the available energy, heat, and fuel.
		if(energyCost > 0.)
			resistance = min(resistance, energy / energyCost);
		if(fuelCost > 0.)
			resistance = min(resistance, fuel / fuelCost);
		if(heatCost < 0.)
			resistance = min(resistance, heat / -heatCost);
		
		// Update the stat, energy, heat, and fuel given how much resistance is being used.
		if(resistance > 0.)
		{
			stat = max(0., .99 * stat - resistance);
			energy -= resistance * energyCost;
			fuel -= resistance * fuelCost;
			heat += resistance * heatCost;
		}
		else
			stat = .99 * stat;
	}
}



// Construct and Load() at the same time.
Ship::Ship(const DataNode &node)
{
	Load(node);
}



void Ship::Load(const DataNode &node)
{
	if(node.Size() >= 2)
	{
		modelName = node.Token(1);
		pluralModelName = modelName + 's';
	}
	if(node.Size() >= 3)
	{
		base = GameData::Ships().Get(modelName);
		variantName = node.Token(2);
	}
	isDefined = true;
	
	government = GameData::PlayerGovernment();
	equipped.clear();
	
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
			LoadSprite(child);
		else if(child.Token(0) == "thumbnail" && child.Size() >= 2)
			thumbnail = SpriteSet::Get(child.Token(1));
		else if(key == "name" && child.Size() >= 2)
			name = child.Token(1);
		else if(key == "plural" && child.Size() >= 2)
			pluralModelName = child.Token(1);
		else if(key == "noun" && child.Size() >= 2)
			noun = child.Token(1);
		else if(key == "swizzle" && child.Size() >= 2)
			customSwizzle = child.Value(1);
		else if(key == "uuid" && child.Size() >= 2)
			uuid = EsUuid::FromString(child.Token(1));
		else if(key == "attributes" || add)
		{
			if(!add)
				baseAttributes.Load(child);
			else
			{
				addAttributes = true;
				attributes.Load(child);
			}
		}
		else if((key == "engine" || key == "reverse engine" || key == "steering engine") && child.Size() >= 3)
		{
			if(!hasEngine)
			{
				enginePoints.clear();
				reverseEnginePoints.clear();
				steeringEnginePoints.clear();
				hasEngine = true;
			}
			bool reverse = (key == "reverse engine");
			bool steering = (key == "steering engine");
			
			vector<EnginePoint> &editPoints = (!steering && !reverse) ? enginePoints : 
				(reverse ? reverseEnginePoints : steeringEnginePoints); 
			editPoints.emplace_back(0.5 * child.Value(1), 0.5 * child.Value(2),
				(child.Size() > 3 ? child.Value(3) : 1.));
			EnginePoint &engine = editPoints.back();
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
				{
					for(unsigned j = 1; j < ENGINE_SIDE.size(); ++j)
						if(grandKey == ENGINE_SIDE[j])
							engine.side = j;
					if(steering)
						for(unsigned j = 1; j < STEERING_FACING.size(); ++j)
							if(grandKey == STEERING_FACING[j])
								engine.steering = j;
				}
			}
		}
		else if(key == "gun" || key == "turret")
		{
			if(!hasArmament)
			{
				armament = Armament();
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
					if(grand.Token(0) == "angle" && grand.Size() >= 2)
						gunPortAngle = grand.Value(1);
					else if(grand.Token(0) == "parallel")
						gunPortParallel = true;
					else if(grand.Token(0) == "under")
						drawUnder = true;
					else if(grand.Token(0) == "over")
						drawUnder = false;
					else
						child.PrintTrace("Warning: Child nodes of \"" + key + "\" tokens can only be \"angle\" or \"parallel\":");
			}
			if(outfit)
				++equipped[outfit];
			if(key == "gun")
				armament.AddGunPort(hardpoint, gunPortAngle, gunPortParallel, drawUnder, outfit);
			else
				armament.AddTurret(hardpoint, drawUnder, outfit);
			// Print a warning for the first hardpoint after 32, i.e. only 1 warning per ship.
			if(armament.Get().size() == 33)
				child.PrintTrace("Warning: ship has more than 32 weapon hardpoints. Some weapons may not fire:");
		}
		else if(key == "never disabled")
			neverDisabled = true;
		else if(key == "uncapturable")
			isCapturable = false;
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
				bays.clear();
				hasBays = true;
			}
			bays.emplace_back(child.Value(1 + childOffset), child.Value(2 + childOffset), category);
			Bay &bay = bays.back();
			for(int i = 3 + childOffset; i < child.Size(); ++i)
			{
				for(unsigned j = 1; j < BAY_SIDE.size(); ++j)
					if(child.Token(i) == BAY_SIDE[j])
						bay.side = j;
				for(unsigned j = 1; j < BAY_FACING.size(); ++j)
					if(child.Token(i) == BAY_FACING[j])
						bay.facing = BAY_ANGLE[j];
			}
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
						bool handled = false;
						for(unsigned i = 1; i < BAY_SIDE.size(); ++i)
							if(grand.Token(0) == BAY_SIDE[i])
							{
								bay.side = i;
								handled = true;
							}
						for(unsigned i = 1; i < BAY_FACING.size(); ++i)
							if(grand.Token(0) == BAY_FACING[i])
							{
								bay.facing = BAY_ANGLE[i];
								handled = true;
							}
						if(!handled)
							grand.PrintTrace("Child nodes of \"bay\" tokens can only be \"launch effect\", \"angle\", or a facing keyword:");
					}
				}
		}
		else if(key == "leak" && child.Size() >= 2)
		{
			if(!hasLeak)
			{
				leaks.clear();
				hasLeak = true;
			}
			Leak leak(GameData::Effects().Get(child.Token(1)));
			if(child.Size() >= 3)
				leak.openPeriod = child.Value(2);
			if(child.Size() >= 4)
				leak.closePeriod = child.Value(3);
			leaks.push_back(leak);
		}
		else if(key == "explode" && child.Size() >= 2)
		{
			if(!hasExplode)
			{
				explosionEffects.clear();
				explosionTotal = 0;
				hasExplode = true;
			}
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			explosionEffects[GameData::Effects().Get(child.Token(1))] += count;
			explosionTotal += count;
		}
		else if(key == "final explode" && child.Size() >= 2)
		{
			if(!hasFinalExplode)
			{
				finalExplosions.clear();
				hasFinalExplode = true;
			}
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			finalExplosions[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(key == "outfits")
		{
			if(!hasOutfits)
			{
				outfits.clear();
				hasOutfits = true;
			}
			for(const DataNode &grand : child)
			{
				int count = (grand.Size() >= 2) ? grand.Value(1) : 1;
				if(count > 0)
					outfits[GameData::Outfits().Get(grand.Token(0))] += count;
				else
					grand.PrintTrace("Skipping invalid outfit count:");
			}
		}
		else if(key == "cargo")
			cargo.Load(child);
		else if(key == "crew" && child.Size() >= 2)
			crew = static_cast<int>(child.Value(1));
		else if(key == "fuel" && child.Size() >= 2)
			fuel = child.Value(1);
		else if(key == "shields" && child.Size() >= 2)
			shields = child.Value(1);
		else if(key == "hull" && child.Size() >= 2)
			hull = child.Value(1);
		else if(key == "position" && child.Size() >= 3)
			position = Point(child.Value(1), child.Value(2));
		else if(key == "system" && child.Size() >= 2)
			currentSystem = GameData::Systems().Get(child.Token(1));
		else if(key == "planet" && child.Size() >= 2)
		{
			zoom = 0.;
			landingPlanet = GameData::Planets().Get(child.Token(1));
		}
		else if(key == "destination system" && child.Size() >= 2)
			targetSystem = GameData::Systems().Get(child.Token(1));
		else if(key == "parked")
			isParked = true;
		else if(key == "description" && child.Size() >= 2)
		{
			if(!hasDescription)
			{
				description.clear();
				hasDescription = true;
			}
			description += child.Token(1);
			description += '\n';
		}
		else if(key != "actions")
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



// When loading a ship, some of the outfits it lists may not have been
// loaded yet. So, wait until everything has been loaded, then call this.
void Ship::FinishLoading(bool isNewInstance)
{
	// All copies of this ship should save pointers to the "explosion" weapon
	// definition stored safely in the ship model, which will not be destroyed
	// until GameData is when the program quits. Also copy other attributes of
	// the base model if no overrides were given.
	if(GameData::Ships().Has(modelName))
	{
		const Ship *model = GameData::Ships().Get(modelName);
		explosionWeapon = &model->BaseAttributes();
		if(pluralModelName.empty())
			pluralModelName = model->pluralModelName;
		if(noun.empty())
			noun = model->noun;
		if(!thumbnail)
			thumbnail = model->thumbnail;
	}
	
	// If this ship has a base class, copy any attributes not defined here.
	// Exception: uncapturable and "never disabled" flags don't carry over.
	if(base && base != this)
	{
		if(!GetSprite())
			reinterpret_cast<Body &>(*this) = *base;
		if(customSwizzle == -1)
			customSwizzle = base->CustomSwizzle();
		if(baseAttributes.Attributes().empty())
			baseAttributes = base->baseAttributes;
		if(bays.empty() && !base->bays.empty())
			bays = base->bays;
		if(enginePoints.empty())
			enginePoints = base->enginePoints;
		if(reverseEnginePoints.empty())
			reverseEnginePoints = base->reverseEnginePoints;
		if(steeringEnginePoints.empty())
			steeringEnginePoints = base->steeringEnginePoints;
		if(explosionEffects.empty())
		{
			explosionEffects = base->explosionEffects;
			explosionTotal = base->explosionTotal;
		}
		if(finalExplosions.empty())
			finalExplosions = base->finalExplosions;
		if(outfits.empty())
			outfits = base->outfits;
		if(description.empty())
			description = base->description;
		
		bool hasHardpoints = false;
		for(const Hardpoint &hardpoint : armament.Get())
			if(hardpoint.GetPoint())
				hasHardpoints = true;
		
		if(!hasHardpoints)
		{
			// Check if any hardpoint locations were not specified.
			auto bit = base->Weapons().begin();
			auto bend = base->Weapons().end();
			auto nextGun = armament.Get().begin();
			auto nextTurret = armament.Get().begin();
			auto end = armament.Get().end();
			Armament merged;
			// Reset the "equipped" map to match exactly what the code below
			// places in the weapon hardpoints.
			equipped.clear();
			for( ; bit != bend; ++bit)
			{
				if(!bit->IsTurret())
				{
					while(nextGun != end && nextGun->IsTurret())
						++nextGun;
					const Outfit *outfit = (nextGun == end) ? nullptr : nextGun->GetOutfit();
					merged.AddGunPort(bit->GetPoint() * 2., bit->GetBaseAngle(), bit->IsParallel(), bit->IsUnder(), outfit);
					if(nextGun != end)
					{
						if(outfit)
							++equipped[outfit];
						++nextGun;
					}
				}
				else
				{
					while(nextTurret != end && !nextTurret->IsTurret())
						++nextTurret;
					const Outfit *outfit = (nextTurret == end) ? nullptr : nextTurret->GetOutfit();
					merged.AddTurret(bit->GetPoint() * 2., bit->IsUnder(), outfit);
					if(nextTurret != end)
					{
						if(outfit)
							++equipped[outfit];
						++nextTurret;
					}
				}
			}
			armament = merged;
		}
	}
	// Check that all the "equipped" weapons actually match what your ship
	// has, and that they are truly weapons. Remove any excess weapons and
	// warn if any non-weapon outfits are "installed" in a hardpoint.
	for(auto &it : equipped)
	{
		int excess = it.second - outfits[it.first];
		if(excess > 0)
		{
			// If there are more hardpoints specifying this outfit than there
			// are instances of this outfit installed, remove some of them.
			armament.Add(it.first, -excess);
			it.second -= excess;
			
			string warning = modelName;
			if(!name.empty())
				warning += " \"" + name + "\"";
			warning += ": outfit \"" + it.first->Name() + "\" equipped but not included in outfit list.";
			Files::LogError(warning);
		}
		else if(!it.first->IsWeapon())
		{
			// This ship was specified with a non-weapon outfit in a
			// hardpoint. Hardpoint::Install removes it, but issue a
			// warning so the definition can be fixed.
			string warning = modelName;
			if(!name.empty())
				warning += " \"" + name + "\"";
			warning += ": outfit \"" + it.first->Name() + "\" is not a weapon, but is installed as one.";
			Files::LogError(warning);
		}
	}
	
	// Mark any drone that has no "automaton" value as an automaton, to
	// grandfather in the drones from before that attribute existed.
	if(baseAttributes.Category() == "Drone" && !baseAttributes.Get("automaton"))
		baseAttributes.Set("automaton", 1.);
	
	baseAttributes.Set("gun ports", armament.GunCount());
	baseAttributes.Set("turret mounts", armament.TurretCount());
	
	if(addAttributes)
	{
		// Store attributes from an "add attributes" node in the ship's
		// baseAttributes so they can be written to the save file.
		baseAttributes.Add(attributes);
		addAttributes = false;
	}
	// Add the attributes of all your outfits to the ship's base attributes.
	attributes = baseAttributes;
	vector<string> undefinedOutfits;
	for(const auto &it : outfits)
	{
		if(!it.first->IsDefined())
		{
			undefinedOutfits.emplace_back("\"" + it.first->Name() + "\"");
			continue;
		}
		attributes.Add(*it.first, it.second);
		// Some ship variant definitions do not specify which weapons
		// are placed in which hardpoint. Add any weapons that are not
		// yet installed to the ship's armament.
		if(it.first->IsWeapon())
		{
			int count = it.second;
			auto eit = equipped.find(it.first);
			if(eit != equipped.end())
				count -= eit->second;
			
			if(count)
				armament.Add(it.first, count);
		}
	}
	if(!undefinedOutfits.empty())
	{
		bool plural = undefinedOutfits.size() > 1;
		// Print the ship name once, then all undefined outfits. If we're reporting for a stock ship, then it
		// doesn't have a name, and missing outfits aren't named yet either. A variant name might exist, though.
		string message;
		if(isYours)
		{
			message = "Player ship " + modelName + " \"" + name + "\":";
			string PREFIX = plural ? "\n\tUndefined outfit " : " undefined outfit ";
			for(auto &&outfit : undefinedOutfits)
				message += PREFIX + outfit;
		}
		else
		{
			message = variantName.empty() ? "Stock ship \"" + modelName + "\": "
				: modelName + " variant \"" + variantName + "\": ";
			message += to_string(undefinedOutfits.size()) + " undefined outfit" + (plural ? "s" : "") + " installed.";
		}
		
		Files::LogError(message);
	}
	// Inspect the ship's armament to ensure that guns are in gun ports and
	// turrets are in turret mounts. This can only happen when the armament
	// is configured incorrectly in a ship or variant definition. Do not
	// bother printing this warning if the outfit is not fully defined.
	for(const Hardpoint &hardpoint : armament.Get())
	{
		const Outfit *outfit = hardpoint.GetOutfit();
		if(outfit && outfit->IsDefined()
				&& (hardpoint.IsTurret() != (outfit->Get("turret mounts") != 0.)))
		{
			string warning = (!isYours && !variantName.empty()) ? "variant \"" + variantName + "\"" : modelName;
			if(!name.empty())
				warning += " \"" + name + "\"";
			warning += ": outfit \"" + outfit->Name() + "\" installed as a ";
			warning += (hardpoint.IsTurret() ? "turret but is a gun.\n\tturret" : "gun but is a turret.\n\tgun");
			warning += to_string(2. * hardpoint.GetPoint().X()) + " " + to_string(2. * hardpoint.GetPoint().Y());
			warning += " \"" + outfit->Name() + "\"";
			Files::LogError(warning);
		}
	}
	cargo.SetSize(attributes.Get("cargo space"));
	equipped.clear();
	armament.FinishLoading();
	
	// Figure out how far from center the farthest hardpoint is.
	weaponRadius = 0.;
	for(const Hardpoint &hardpoint : armament.Get())
		weaponRadius = max(weaponRadius, hardpoint.GetPoint().Length());
	
	// If this ship is being instantiated for the first time, make sure its
	// crew, fuel, etc. are all refilled.
	if(isNewInstance)
		Recharge(true);
	
	// Ensure that all defined bays are of a valid category. Remove and warn about any
	// invalid bays. Add a default "launch effect" to any remaining internal bays if
	// this ship is crewed (i.e. pressurized).
	string warning;
	const auto &bayCategories = GameData::Category(CategoryType::BAY);
	for(auto it = bays.begin(); it != bays.end(); )
	{
		Bay &bay = *it;
		if(find(bayCategories.begin(), bayCategories.end(), bay.category) == bayCategories.end())
		{
			warning += "Invalid bay category: " + bay.category + "\n";
			it = bays.erase(it);
			continue;
		}
		else
			++it;
		if(bay.side == Bay::INSIDE && bay.launchEffects.empty() && Crew())
			bay.launchEffects.emplace_back(GameData::Effects().Get("basic launch"));
	}
	
	canBeCarried = find(bayCategories.begin(), bayCategories.end(), attributes.Category()) != bayCategories.end();
	
	// Issue warnings if this ship has is misconfigured, e.g. is missing required values
	// or has negative outfit, cargo, weapon, or engine capacity.
	for(auto &&attr : set<string>{"outfit space", "cargo space", "weapon capacity", "engine capacity"})
	{
		double val = attributes.Get(attr);
		if(val < 0)
			warning += attr + ": " + Format::Number(val) + "\n";
	}
	if(attributes.Get("drag") <= 0.)
	{
		warning += "Defaulting " + string(attributes.Get("drag") ? "invalid" : "missing") + " \"drag\" attribute to 100.0\n";
		attributes.Set("drag", 100.);
	}
	if(!warning.empty())
	{
		// This check is mostly useful for variants and stock ships, which have
		// no names. Print the outfits to facilitate identifying this ship definition.
		string message = (!name.empty() ? "Ship \"" + name + "\" " : "") + "(" + VariantName() + "):\n";
		ostringstream outfitNames;
		outfitNames << "has outfits:\n";
		for(const auto &it : outfits)
			outfitNames << '\t' << it.second << " " + it.first->Name() << endl;
		Files::LogError(message + warning + outfitNames.str());
	}
	
	// Ships read from a save file may have non-default shields or hull.
	// Perform a full IsDisabled calculation.
	isDisabled = true;
	isDisabled = IsDisabled();
	
	// Cache this ship's jump range.
	jumpRange = JumpRange(false);
	
	// A saved ship may have an invalid target system. Since all game data is loaded and all player events are
	// applied at this point, any target system that is not accessible should be cleared. Note: this does not
	// account for systems accessible via wormholes, but also does not need to as AI will route the ship properly.
	if(!isNewInstance && targetSystem)
	{
		string message = "Warning: " + string(isYours ? "player-owned " : "NPC ") + modelName + " \"" + name + "\": "
			"Cannot reach target system \"" + targetSystem->Name();
		if(!currentSystem)
		{
			Files::LogError(message + "\" (no current system).");
			targetSystem = nullptr;
		}
		else if(!currentSystem->Links().count(targetSystem) && (!jumpRange || !currentSystem->JumpNeighbors(jumpRange).count(targetSystem)))
		{
			Files::LogError(message + "\" by hyperlink or jump from system \"" + currentSystem->Name() + ".\"");
			targetSystem = nullptr;
		}
	}
}



// Check if this ship (model) and its outfits have been defined.
bool Ship::IsValid() const
{
	for(auto &&outfit : outfits)
		if(!outfit.first->IsDefined())
			return false;
	
	return isDefined;
}



// Save a full description of this ship, as currently configured.
void Ship::Save(DataWriter &out) const
{
	out.Write("ship", modelName);
	out.BeginChild();
	{
		out.Write("name", name);
		if(pluralModelName != modelName + 's')
			out.Write("plural", pluralModelName);
		if(!noun.empty())
			out.Write("noun", noun);
		SaveSprite(out);
		if(thumbnail)
			out.Write("thumbnail", thumbnail->Name());
		
		if(neverDisabled)
			out.Write("never disabled");
		if(!isCapturable)
			out.Write("uncapturable");
		if(customSwizzle >= 0)
			out.Write("swizzle", customSwizzle);
		
		out.Write("uuid", uuid.ToString());
		
		out.Write("attributes");
		out.BeginChild();
		{
			out.Write("category", baseAttributes.Category());
			out.Write("cost", baseAttributes.Cost());
			out.Write("mass", baseAttributes.Mass());
			for(const auto &it : baseAttributes.FlareSprites())
				for(int i = 0; i < it.second; ++i)
					it.first.SaveSprite(out, "flare sprite");
			for(const auto &it : baseAttributes.FlareSounds())
				for(int i = 0; i < it.second; ++i)
					out.Write("flare sound", it.first->Name());
			for(const auto &it : baseAttributes.ReverseFlareSprites())
				for(int i = 0; i < it.second; ++i)
					it.first.SaveSprite(out, "reverse flare sprite");
			for(const auto &it : baseAttributes.ReverseFlareSounds())
				for(int i = 0; i < it.second; ++i)
					out.Write("reverse flare sound", it.first->Name());
			for(const auto &it : baseAttributes.SteeringFlareSprites())
				for(int i = 0; i < it.second; ++i)
					it.first.SaveSprite(out, "steering flare sprite");
			for(const auto &it : baseAttributes.SteeringFlareSounds())
				for(int i = 0; i < it.second; ++i)
					out.Write("steering flare sound", it.first->Name());
			for(const auto &it : baseAttributes.AfterburnerEffects())
				for(int i = 0; i < it.second; ++i)
					out.Write("afterburner effect", it.first->Name());
			for(const auto &it : baseAttributes.JumpEffects())
				for(int i = 0; i < it.second; ++i)
					out.Write("jump effect", it.first->Name());
			for(const auto &it : baseAttributes.JumpSounds())
				for(int i = 0; i < it.second; ++i)
					out.Write("jump sound", it.first->Name());
			for(const auto &it : baseAttributes.JumpInSounds())
				for(int i = 0; i < it.second; ++i)
					out.Write("jump in sound", it.first->Name());
			for(const auto &it : baseAttributes.JumpOutSounds())
				for(int i = 0; i < it.second; ++i)
					out.Write("jump out sound", it.first->Name());
			for(const auto &it : baseAttributes.HyperSounds())
				for(int i = 0; i < it.second; ++i)
					out.Write("hyperdrive sound", it.first->Name());
			for(const auto &it : baseAttributes.HyperInSounds())
				for(int i = 0; i < it.second; ++i)
					out.Write("hyperdrive in sound", it.first->Name());
			for(const auto &it : baseAttributes.HyperOutSounds())
				for(int i = 0; i < it.second; ++i)
					out.Write("hyperdrive out sound", it.first->Name());
			for(const auto &it : baseAttributes.Attributes())
				if(it.second)
					out.Write(it.first, it.second);
		}
		out.EndChild();
		
		out.Write("outfits");
		out.BeginChild();
		{
			using OutfitElement = pair<const Outfit *const, int>;
			WriteSorted(outfits,
				[](const OutfitElement *lhs, const OutfitElement *rhs)
					{ return lhs->first->Name() < rhs->first->Name(); },
				[&out](const OutfitElement &it){
					if(it.second == 1)
						out.Write(it.first->Name());
					else
						out.Write(it.first->Name(), it.second);
				});
		}
		out.EndChild();
		
		cargo.Save(out);
		out.Write("crew", crew);
		out.Write("fuel", fuel);
		out.Write("shields", shields);
		out.Write("hull", hull);
		out.Write("position", position.X(), position.Y());
		
		for(const EnginePoint &point : enginePoints)
		{
			out.Write("engine", 2. * point.X(), 2. * point.Y());
			out.BeginChild();
			out.Write("zoom", point.zoom);
			out.Write("angle", point.facing.Degrees());
			out.Write(ENGINE_SIDE[point.side]);
			out.EndChild();
				
		}
		for(const EnginePoint &point : reverseEnginePoints)
		{
			out.Write("reverse engine", 2. * point.X(), 2. * point.Y());
			out.BeginChild();
			out.Write("zoom", point.zoom);
			out.Write("angle", point.facing.Degrees() - 180.);
			out.Write(ENGINE_SIDE[point.side]);
			out.EndChild();
		}
		for(const EnginePoint &point : steeringEnginePoints)
		{
			out.Write("steering engine", 2. * point.X(), 2. * point.Y());
			out.BeginChild();
			out.Write("zoom", point.zoom);
			out.Write("angle", point.facing.Degrees());
			out.Write(ENGINE_SIDE[point.side]);
			out.Write(STEERING_FACING[point.steering]);
			out.EndChild();
		}
		for(const Hardpoint &hardpoint : armament.Get())
		{
			const char *type = (hardpoint.IsTurret() ? "turret" : "gun");
			if(hardpoint.GetOutfit())
				out.Write(type, 2. * hardpoint.GetPoint().X(), 2. * hardpoint.GetPoint().Y(),
					hardpoint.GetOutfit()->Name());
			else
				out.Write(type, 2. * hardpoint.GetPoint().X(), 2. * hardpoint.GetPoint().Y());
			double hardpointAngle = hardpoint.GetBaseAngle().Degrees();
			out.BeginChild();
			{
				if(hardpointAngle)
					out.Write("angle", hardpointAngle);
				if(hardpoint.IsParallel())
					out.Write("parallel");
				if(hardpoint.IsUnder())
					out.Write("under");
				else
					out.Write("over");
			}
			out.EndChild();
		}
		for(const Bay &bay : bays)
		{
			double x = 2. * bay.point.X();
			double y = 2. * bay.point.Y();

			out.Write("bay", bay.category, x, y);
			
			if(!bay.launchEffects.empty() || bay.facing.Degrees() || bay.side)
			{
				out.BeginChild();
				{
					if(bay.facing.Degrees())
						out.Write("angle", bay.facing.Degrees());
					if(bay.side)
						out.Write(BAY_SIDE[bay.side]);
					for(const Effect *effect : bay.launchEffects)
						out.Write("launch effect", effect->Name());
				}
				out.EndChild();
			}
		}
		for(const Leak &leak : leaks)
			out.Write("leak", leak.effect->Name(), leak.openPeriod, leak.closePeriod);
		
		using EffectElement = pair<const Effect *const, int>;
		auto effectSort = [](const EffectElement *lhs, const EffectElement *rhs)
			{ return lhs->first->Name() < rhs->first->Name(); };
		WriteSorted(explosionEffects, effectSort, [&out](const EffectElement &it)
		{
			if(it.second)
				out.Write("explode", it.first->Name(), it.second);
		});
		WriteSorted(finalExplosions, effectSort, [&out](const EffectElement &it)
		{
			if(it.second)
				out.Write("final explode", it.first->Name(), it.second);
		});
		
		if(currentSystem)
			out.Write("system", currentSystem->Name());
		else
		{
			// A carried ship is saved in its carrier's system.
			shared_ptr<const Ship> parent = GetParent();
			if(parent && parent->currentSystem)
				out.Write("system", parent->currentSystem->Name());
		}
		if(landingPlanet)
			out.Write("planet", landingPlanet->TrueName());
		if(targetSystem)
			out.Write("destination system", targetSystem->Name());
		if(isParked)
			out.Write("parked");
	}
	out.EndChild();
}



const EsUuid &Ship::UUID() const noexcept
{
	return uuid;
}



void Ship::SetUUID(const EsUuid &id)
{
	uuid.clone(id);
}



const string &Ship::Name() const
{
	return name;
}



// Set / Get the name of this class of ships, e.g. "Marauder Raven."
void Ship::SetModelName(const string &model)
{
	this->modelName = model;
}



const string &Ship::ModelName() const
{
	return modelName;
}



const string &Ship::PluralModelName() const
{
	return pluralModelName;
}



// Get the name of this ship as a variant.
const string &Ship::VariantName() const
{
	return variantName.empty() ? modelName : variantName;
}



// Get the generic noun (e.g. "ship") to be used when describing this ship.
const string &Ship::Noun() const
{
	static const string SHIP = "ship";
	return noun.empty() ? SHIP : noun;
}



// Get this ship's description.
const string &Ship::Description() const
{
	return description;
}



// Get the shipyard thumbnail for this ship.
const Sprite *Ship::Thumbnail() const
{
	return thumbnail;
}



// Get this ship's cost.
int64_t Ship::Cost() const
{
	return attributes.Cost();
}



// Get the cost of this ship's chassis, with no outfits installed.
int64_t Ship::ChassisCost() const
{
	return baseAttributes.Cost();
}



// Check if this ship is configured in such a way that it would be difficult
// or impossible to fly.
vector<string> Ship::FlightCheck() const
{
	auto checks = vector<string>{};
	
	double generation = attributes.Get("energy generation") - attributes.Get("energy consumption");
	double burning = attributes.Get("fuel energy");
	double solar = attributes.Get("solar collection");
	double battery = attributes.Get("energy capacity");
	double energy = generation + burning + solar + battery;
	double fuelChange = attributes.Get("fuel generation") - attributes.Get("fuel consumption");
	double fuelCapacity = attributes.Get("fuel capacity");
	double fuel = fuelCapacity + fuelChange;
	double thrust = attributes.Get("thrust");
	double reverseThrust = attributes.Get("reverse thrust");
	double afterburner = attributes.Get("afterburner thrust");
	double thrustEnergy = attributes.Get("thrusting energy");
	double turn = attributes.Get("turn");
	double turnEnergy = attributes.Get("turning energy");
	double hyperDrive = attributes.Get("hyperdrive");
	double jumpDrive = attributes.Get("jump drive");
	
	// Report the first error condition that will prevent takeoff:
	if(IdleHeat() >= MaximumHeat())
		checks.emplace_back("overheating!");
	else if(energy <= 0.)
		checks.emplace_back("no energy!");
	else if((energy - burning <= 0.) && (fuel <= 0.))
		checks.emplace_back("no fuel!");
	else if(!thrust && !reverseThrust && !afterburner)
		checks.emplace_back("no thruster!");
	else if(!turn)
		checks.emplace_back("no steering!");
	
	// If no errors were found, check all warning conditions:
	if(checks.empty())
	{
		if(!thrust && !reverseThrust)
			checks.emplace_back("afterburner only?");
		if(!thrust && !afterburner)
			checks.emplace_back("reverse only?");
		if(!generation && !solar && !burning)
			checks.emplace_back("battery only?");
		if(energy < thrustEnergy)
			checks.emplace_back("limited thrust?");
		if(energy < turnEnergy)
			checks.emplace_back("limited turn?");
		if(energy - .8 * solar < .2 * (turnEnergy + thrustEnergy))
			checks.emplace_back("solar power?");
		if(fuel < 0.)
			checks.emplace_back("fuel?");
		if(!canBeCarried)
		{
			if(!hyperDrive && !jumpDrive)
				checks.emplace_back("no hyperdrive?");
			if(fuelCapacity < JumpFuel())
				checks.emplace_back("no fuel?");
		}
		for(const auto &it : outfits)
			if(it.first->IsWeapon() && it.first->FiringEnergy() > energy)
			{
				checks.emplace_back("insufficient energy to fire?");
				break;
			}
	}
	
	return checks;
}



void Ship::SetPosition(Point position)
{
	this->position = position;
}



// Instantiate a newly-created ship in-flight.
void Ship::Place(Point position, Point velocity, Angle angle)
{
	this->position = position;
	this->velocity = velocity;
	this->angle = angle;
	
	// If landed, place the ship right above the planet.
	// Escorts should take off a bit behind their flagships.
	if(landingPlanet)
	{
		landingPlanet = nullptr;
		zoom = parent.lock() ? (-.2 + -.8 * Random::Real()) : 0.;
	}
	else
		zoom = 1.;
	// Make sure various special status values are reset.
	heat = IdleHeat();
	ionization = 0.;
	disruption = 0.;
	slowness = 0.;
	shieldDelay = 0;
	hullDelay = 0;
	isInvisible = !HasSprite();
	jettisoned.clear();
	hyperspaceCount = 0;
	forget = 1;
	targetShip.reset();
	shipToAssist.reset();
	if(government)
	{
		auto swizzle = customSwizzle >= 0 ? customSwizzle : government->GetSwizzle();
		SetSwizzle(swizzle);

		// Set swizzle for any carried ships too.
		for(const auto &bay : bays)
		{
			if(bay.ship)
				bay.ship->SetSwizzle(bay.ship->customSwizzle >= 0 ? bay.ship->customSwizzle : swizzle);
		}
	}
}



// Set the name of this particular ship.
void Ship::SetName(const string &name)
{
	this->name = name;
}



// Set which system this ship is in.
void Ship::SetSystem(const System *system)
{
	currentSystem = system;
}



void Ship::SetPlanet(const Planet *planet)
{
	zoom = !planet;
	landingPlanet = planet;
}



void Ship::SetGovernment(const Government *government)
{
	if(government)
		SetSwizzle(customSwizzle >= 0 ? customSwizzle : government->GetSwizzle());
	this->government = government;
}



void Ship::SetIsSpecial(bool special)
{
	isSpecial = special;
}



bool Ship::IsSpecial() const
{
	return isSpecial;
}



void Ship::SetIsYours(bool yours)
{
	isYours = yours;
}



bool Ship::IsYours() const
{
	return isYours;
}



void Ship::SetIsParked(bool parked)
{
	isParked = parked;
}



bool Ship::IsParked() const
{
	return isParked;
}



bool Ship::HasDeployOrder() const
{
	return shouldDeploy;
}



void Ship::SetDeployOrder(bool shouldDeploy)
{
	this->shouldDeploy = shouldDeploy;
}



const Personality &Ship::GetPersonality() const
{
	return personality;
}



void Ship::SetPersonality(const Personality &other)
{
	personality = other;
}



void Ship::SetHail(const Phrase &phrase)
{
	hail = &phrase;
}



string Ship::GetHail(const PlayerInfo &player) const
{
	map<string, string> subs;
	
	subs["<first>"] = player.FirstName();
	subs["<last>"] = player.LastName();
	if(player.Flagship())
		subs["<ship>"] = player.Flagship()->Name();
	
	subs["<npc>"] = Name();
	subs["<system>"] = player.GetSystem()->Name();
	subs["<date>"] = player.GetDate().ToString();
	subs["<day>"] = player.GetDate().LongString();
	
	string hailStr = hail ? hail->Get() : government ? government->GetHail(isDisabled) : "";
	return Format::Replace(hailStr, subs);
}



// Set the commands for this ship to follow this timestep.
void Ship::SetCommands(const Command &command)
{
	commands = command;
}



const Command &Ship::Commands() const
{
	return commands;
}



// Move this ship. A ship may create effects as it moves, in particular if
// it is in the process of blowing up. If this returns false, the ship
// should be deleted.
void Ship::Move(vector<Visual> &visuals, list<shared_ptr<Flotsam>> &flotsam)
{
	// Check if this ship has been in a different system from the player for so
	// long that it should be "forgotten." Also eliminate ships that have no
	// system set because they just entered a fighter bay.
	forget += !isInSystem;
	isThrusting = false;
	isReversing = false;
	isSteering = false;
	steeringDirection = 0.;
	if((!isSpecial && forget >= 1000) || !currentSystem)
	{
		MarkForRemoval();
		return;
	}
	isInSystem = false;
	if(!fuel || !(attributes.Get("hyperdrive") || attributes.Get("jump drive")))
		hyperspaceSystem = nullptr;
	
	// Adjust the error in the pilot's targeting.
	personality.UpdateConfusion(commands.IsFiring());
	
	// Generate energy, heat, etc.
	DoGeneration();

	// Handle ionization effects, etc.
	if(ionization)
		CreateSparks(visuals, "ion spark", ionization * .1);
	if(disruption)
		CreateSparks(visuals, "disruption spark", disruption * .1);
	if(slowness)
		CreateSparks(visuals, "slowing spark", slowness * .1);
	// Jettisoned cargo effects (only for ships in the current system).
	if(!jettisoned.empty() && !forget)
	{
		jettisoned.front()->Place(*this);
		flotsam.splice(flotsam.end(), jettisoned, jettisoned.begin());
	}
	int requiredCrew = RequiredCrew();
	double slowMultiplier = 1. / (1. + slowness * .05);
	
	// Move the turrets.
	if(!isDisabled)
		armament.Aim(commands);
	
	if(!isInvisible)
	{
		// If you are forced to decloak (e.g. by running out of fuel) you can't
		// initiate cloaking again until you are fully decloaked.
		if(!cloak)
			cloakDisruption = max(0., cloakDisruption - 1.);
		
		double cloakingSpeed = attributes.Get("cloak");
		bool canCloak = (!isDisabled && cloakingSpeed > 0. && !cloakDisruption
			&& fuel >= attributes.Get("cloaking fuel")
			&& energy >= attributes.Get("cloaking energy"));
		if(commands.Has(Command::CLOAK) && canCloak)
		{
			cloak = min(1., cloak + cloakingSpeed);
			fuel -= attributes.Get("cloaking fuel");
			energy -= attributes.Get("cloaking energy");
			heat += attributes.Get("cloaking heat");
		}
		else if(cloakingSpeed)
		{
			cloak = max(0., cloak - cloakingSpeed);
			// If you're trying to cloak but are unable to (too little energy or
			// fuel) you're forced to decloak fully for one frame before you can
			// engage cloaking again.
			if(commands.Has(Command::CLOAK))
				cloakDisruption = max(cloakDisruption, 1.);
		}
		else
			cloak = 0.;
	}
	
	if(IsDestroyed())
	{
		// Make sure the shields are zero, as well as the hull.
		shields = 0.;
		
		// Once we've created enough little explosions, die.
		if(explosionCount == explosionTotal || forget)
		{
			if(!forget)
			{
				const Effect *effect = GameData::Effects().Get("smoke");
				double size = Width() + Height();
				double scale = .03 * size + .5;
				double radius = .2 * size;
				int debrisCount = attributes.Mass() * .07;
				
				// Estimate how many new visuals will be added during destruction.
				visuals.reserve(visuals.size() + debrisCount + explosionTotal + finalExplosions.size());
				
				for(int i = 0; i < debrisCount; ++i)
				{
					Angle angle = Angle::Random();
					Point effectVelocity = velocity + angle.Unit() * (scale * Random::Real());
					Point effectPosition = position + radius * angle.Unit();
					
					visuals.emplace_back(*effect, std::move(effectPosition), std::move(effectVelocity), std::move(angle));
				}
				
				for(unsigned i = 0; i < explosionTotal / 2; ++i)
					CreateExplosion(visuals, true);
				for(const auto &it : finalExplosions)
					visuals.emplace_back(*it.first, position, velocity, angle);
				// For everything in this ship's cargo hold there is a 25% chance
				// that it will survive as flotsam.
				for(const auto &it : cargo.Commodities())
					Jettison(it.first, Random::Binomial(it.second, .25));
				for(const auto &it : cargo.Outfits())
					Jettison(it.first, Random::Binomial(it.second, .25));
				// Ammunition has a 5% chance to survive as flotsam
				for(const auto &it : outfits)
					if(it.first->Category() == "Ammunition")
						Jettison(it.first, Random::Binomial(it.second, .05));
				for(shared_ptr<Flotsam> &it : jettisoned)
					it->Place(*this);
				flotsam.splice(flotsam.end(), jettisoned);
				
				// Any ships that failed to launch from this ship are destroyed.
				for(Bay &bay : bays)
					if(bay.ship)
						bay.ship->Destroy();
			}
			energy = 0.;
			heat = 0.;
			ionization = 0.;
			fuel = 0.;
			velocity = Point();
			MarkForRemoval();
			return;
		}
		
		// If the ship is dead, it first creates explosions at an increasing
		// rate, then disappears in one big explosion.
		++explosionRate;
		if(Random::Int(1024) < explosionRate)
			CreateExplosion(visuals);
		
		// Handle hull "leaks."
		for(const Leak &leak : leaks)
			if(leak.openPeriod > 0 && !Random::Int(leak.openPeriod))
			{
				activeLeaks.push_back(leak);
				const vector<Point> &outline = GetMask().Points();
				if(outline.size() < 2)
					break;
				int i = Random::Int(outline.size() - 1);
				
				// Position the leak along the outline of the ship, facing outward.
				activeLeaks.back().location = (outline[i] + outline[i + 1]) * .5;
				activeLeaks.back().angle = Angle(outline[i] - outline[i + 1]) + Angle(90.);
			}
		for(Leak &leak : activeLeaks)
			if(leak.effect)
			{
				// Leaks always "flicker" every other frame.
				if(Random::Int(2))
					visuals.emplace_back(*leak.effect,
						angle.Rotate(leak.location) + position,
						velocity,
						leak.angle + angle);
				
				if(leak.closePeriod > 0 && !Random::Int(leak.closePeriod))
					leak.effect = nullptr;
			}
	}
	else if(hyperspaceSystem || hyperspaceCount)
	{
		// Don't apply external acceleration while jumping.
		acceleration = Point();
		
		// Enter hyperspace.
		int direction = hyperspaceSystem ? 1 : -1;
		hyperspaceCount += direction;
		static const int HYPER_C = 100;
		static const double HYPER_A = 2.;
		static const double HYPER_D = 1000.;
		if(hyperspaceSystem)
			fuel -= hyperspaceFuelCost / HYPER_C;
		
		// Create the particle effects for the jump drive. This may create 100
		// or more particles per ship per turn at the peak of the jump.
		if(isUsingJumpDrive && !forget)
		{
			double sparkAmount = hyperspaceCount * Width() * Height() * .000006;
			const map<const Effect *, int> &jumpEffects = attributes.JumpEffects();
			if(jumpEffects.empty())
				CreateSparks(visuals, "jump drive", sparkAmount);
			else
			{
				// Spread the amount of particle effects created among all jump effects.
				sparkAmount /= jumpEffects.size();
				for(const auto &effect : jumpEffects)
					CreateSparks(visuals, effect.first, sparkAmount);
			}
		}
		
		if(hyperspaceCount == HYPER_C)
		{
			currentSystem = hyperspaceSystem;
			hyperspaceSystem = nullptr;
			targetSystem = nullptr;
			// Check if the target planet is in the destination system or not.
			const Planet *planet = (targetPlanet ? targetPlanet->GetPlanet() : nullptr);
			if(!planet || planet->IsWormhole() || !planet->IsInSystem(currentSystem))
				targetPlanet = nullptr;
			// Check if your parent has a target planet in this system.
			shared_ptr<Ship> parent = GetParent();
			if(!targetPlanet && parent && parent->targetPlanet)
			{
				planet = parent->targetPlanet->GetPlanet();
				if(planet && !planet->IsWormhole() && planet->IsInSystem(currentSystem))
					targetPlanet = parent->targetPlanet;
			}
			direction = -1;
			
			// If you have a target planet in the destination system, exit
			// hyperpace aimed at it. Otherwise, target the first planet that
			// has a spaceport.
			Point target;
			// Except when you arrive at an extra distance from the target,
			// in that case always use the system-center as target.
			double extraArrivalDistance = isUsingJumpDrive ? currentSystem->ExtraJumpArrivalDistance() : currentSystem->ExtraHyperArrivalDistance();
			
			if(extraArrivalDistance == 0)
			{
				if(targetPlanet)
					target = targetPlanet->Position();
				else
				{
					for(const StellarObject &object : currentSystem->Objects())
						if(object.HasSprite() && object.HasValidPlanet()
								&& object.GetPlanet()->HasSpaceport())
						{
							target = object.Position();
							break;
						}
				}
			}
			
			if(isUsingJumpDrive)
			{
				position = target + Angle::Random().Unit() * (300. * (Random::Real() + 1.) + extraArrivalDistance);
				return;
			}
			
			// Have all ships exit hyperspace at the same distance so that
			// your escorts always stay with you.
			double distance = (HYPER_C * HYPER_C) * .5 * HYPER_A + HYPER_D;
			distance += extraArrivalDistance;
			position = (target - distance * angle.Unit());
			position += hyperspaceOffset;
			// Make sure your velocity is in exactly the direction you are
			// traveling in, so that when you decelerate there will not be a
			// sudden shift in direction at the end.
			velocity = velocity.Length() * angle.Unit();
		}
		if(!isUsingJumpDrive)
		{
			velocity += (HYPER_A * direction) * angle.Unit();
			if(!hyperspaceSystem)
			{
				// Exit hyperspace far enough from the planet to be able to land.
				// This does not take drag into account, so it is always an over-
				// estimate of how long it will take to stop.
				// We start decelerating after rotating about 150 degrees (that
				// is, about acos(.8) from the proper angle). So:
				// Stopping distance = .5*a*(v/a)^2 + (150/turn)*v.
				// Exit distance = HYPER_D + .25 * v^2 = stopping distance.
				double exitV = max(HYPER_A, MaxVelocity());
				double a = (.5 / Acceleration() - .25);
				double b = 150. / TurnRate();
				double discriminant = b * b - 4. * a * -HYPER_D;
				if(discriminant > 0.)
				{
					double altV = (-b + sqrt(discriminant)) / (2. * a);
					if(altV > 0. && altV < exitV)
						exitV = altV;
				}
				if(velocity.Length() <= exitV)
				{
					velocity = angle.Unit() * exitV;
					hyperspaceCount = 0;
				}
			}
		}
		position += velocity;
		if(GetParent() && GetParent()->currentSystem == currentSystem)
		{
			hyperspaceOffset = position - GetParent()->position;
			double length = hyperspaceOffset.Length();
			if(length > 1000.)
				hyperspaceOffset *= 1000. / length;
		}
		
		return;
	}
	else if(landingPlanet || zoom < 1.f)
	{
		// Don't apply external acceleration while landing.
		acceleration = Point();
		
		// If a ship was disabled at the very moment it began landing, do not
		// allow it to continue landing.
		if(isDisabled)
			landingPlanet = nullptr;
		
		// Special ships do not disappear forever when they land; they
		// just slowly refuel.
		if(landingPlanet && zoom)
		{
			// Move the ship toward the center of the planet while landing.
			if(GetTargetStellar())
				position = .97 * position + .03 * GetTargetStellar()->Position();
			zoom -= .02f;
			if(zoom < 0.f)
			{
				// If this is not a special ship, it ceases to exist when it
				// lands on a true planet. If this is a wormhole, the ship is
				// instantly transported.
				if(landingPlanet->IsWormhole())
				{
					currentSystem = landingPlanet->WormholeDestination(currentSystem);
					for(const StellarObject &object : currentSystem->Objects())
						if(object.GetPlanet() == landingPlanet)
							position = object.Position();
					SetTargetStellar(nullptr);
					landingPlanet = nullptr;
				}
				else if(!isSpecial || personality.IsFleeing())
				{
					MarkForRemoval();
					return;
				}
				
				zoom = 0.f;
			}
		}
		// Only refuel if this planet has a spaceport.
		else if(fuel >= attributes.Get("fuel capacity")
				|| !landingPlanet || !landingPlanet->HasSpaceport())
		{
			zoom = min(1.f, zoom + .02f);
			SetTargetStellar(nullptr);
			landingPlanet = nullptr;
		}
		else
			fuel = min(fuel + 1., attributes.Get("fuel capacity"));
		
		// Move the ship at the velocity it had when it began landing, but
		// scaled based on how small it is now.
		if(zoom > 0.f)
			position += velocity * zoom;
		
		return;
	}
	if(isDisabled)
	{
		// If you're disabled, you can't initiate landing or jumping.
	}
	else if(commands.Has(Command::LAND) && CanLand())
		landingPlanet = GetTargetStellar()->GetPlanet();
	else if(commands.Has(Command::JUMP) && IsReadyToJump())
	{
		hyperspaceSystem = GetTargetSystem();
		isUsingJumpDrive = !attributes.Get("hyperdrive") || !currentSystem->Links().count(hyperspaceSystem);
		hyperspaceFuelCost = JumpFuel(hyperspaceSystem);
	}
	
	if(pilotError)
		--pilotError;
	else if(pilotOkay)
		--pilotOkay;
	else if(isDisabled)
	{
		// If the ship is disabled, don't show a warning message due to missing crew.
	}
	else if(requiredCrew && static_cast<int>(Random::Int(requiredCrew)) >= Crew())
	{
		pilotError = 30;
		if(parent.lock() || !isYours)
			Messages::Add("The " + name + " is moving erratically because there are not enough crew to pilot it."
				, Messages::Importance::Low);
		else
			Messages::Add("Your ship is moving erratically because you do not have enough crew to pilot it."
				, Messages::Importance::Low);
	}
	else
		pilotOkay = 30;
	
	// This ship is not landing or entering hyperspace. So, move it. If it is
	// disabled, all it can do is slow down to a stop.
	double mass = Mass();
	bool isUsingAfterburner = false;
	if(isDisabled)
		velocity *= 1. - attributes.Get("drag") / mass;
	else if(!pilotError)
	{
		if(commands.Turn())
		{
			// Check if we are able to turn.
			double cost = attributes.Get("turning energy");
			if(energy < cost * fabs(commands.Turn()))
				commands.SetTurn(commands.Turn() * energy / (cost * fabs(commands.Turn())));
			
			if(commands.Turn())
			{
				isSteering = true;
				steeringDirection = commands.Turn();
				// If turning at a fraction of the full rate (either from lack of
				// energy or because of tracking a target), only consume a fraction
				// of the turning energy and produce a fraction of the heat.
				double scale = fabs(commands.Turn());
				energy -= scale * cost;
				heat += scale * attributes.Get("turning heat");
				angle += commands.Turn() * TurnRate() * slowMultiplier;
			}
		}
		double thrustCommand = commands.Has(Command::FORWARD) - commands.Has(Command::BACK);
		double thrust = 0.;
		if(thrustCommand)
		{
			// Check if we are able to apply this thrust.
			double cost = attributes.Get((thrustCommand > 0.) ?
				"thrusting energy" : "reverse thrusting energy");
			if(energy < cost)
				thrustCommand *= energy / cost;
			
			if(thrustCommand)
			{
				// If a reverse thrust is commanded and the capability does not
				// exist, ignore it (do not even slow under drag).
				isThrusting = (thrustCommand > 0.);
				isReversing = !isThrusting && attributes.Get("reverse thrust");
				thrust = attributes.Get(isThrusting ? "thrust" : "reverse thrust");
				if(thrust)
				{
					double scale = fabs(thrustCommand);
					energy -= scale * cost;
					heat += scale * attributes.Get(isThrusting ? "thrusting heat" : "reverse thrusting heat");
					acceleration += angle.Unit() * (thrustCommand * thrust / mass);
				}
			}
		}
		bool applyAfterburner = (commands.Has(Command::AFTERBURNER) || (thrustCommand > 0. && !thrust))
				&& !CannotAct();
		if(applyAfterburner)
		{
			thrust = attributes.Get("afterburner thrust");
			double fuelCost = attributes.Get("afterburner fuel");
			double energyCost = attributes.Get("afterburner energy");
			if(thrust && fuel >= fuelCost && energy >= energyCost)
			{
				heat += attributes.Get("afterburner heat");
				fuel -= fuelCost;
				energy -= energyCost;
				acceleration += angle.Unit() * thrust / mass;
				
				// Only create the afterburner effects if the ship is in the player's system.
				isUsingAfterburner = !forget;
			}
		}
	}
	if(acceleration)
	{
		acceleration *= slowMultiplier;
		Point dragAcceleration = acceleration - velocity * (attributes.Get("drag") / mass);
		// Make sure dragAcceleration has nonzero length, to avoid divide by zero.
		if(dragAcceleration)
		{
			// What direction will the net acceleration be if this drag is applied?
			// If the net acceleration will be opposite the thrust, do not apply drag.
			dragAcceleration *= .5 * (acceleration.Unit().Dot(dragAcceleration.Unit()) + 1.);
			
			// A ship can only "cheat" to stop if it is moving slow enough that
			// it could stop completely this frame. This is to avoid overshooting
			// when trying to stop and ending up headed in the other direction.
			if(commands.Has(Command::STOP))
			{
				// How much acceleration would it take to come to a stop in the
				// direction normal to the ship's current facing? This is only
				// possible if the acceleration plus drag vector is in the
				// opposite direction from the velocity vector when both are
				// projected onto the current facing vector, and the acceleration
				// vector is the larger of the two.
				double vNormal = velocity.Dot(angle.Unit());
				double aNormal = dragAcceleration.Dot(angle.Unit());
				if((aNormal > 0.) != (vNormal > 0.) && fabs(aNormal) > fabs(vNormal))
					dragAcceleration = -vNormal * angle.Unit();
			}
			velocity += dragAcceleration;
		}
		acceleration = Point();
	}
	
	// Boarding:
	shared_ptr<const Ship> target = GetTargetShip();
	// If this is a fighter or drone and it is not assisting someone at the
	// moment, its boarding target should be its parent ship.
	if(CanBeCarried() && !(target && target == GetShipToAssist()))
		target = GetParent();
	if(target && !isDisabled)
	{
		Point dp = (target->position - position);
		double distance = dp.Length();
		Point dv = (target->velocity - velocity);
		double speed = dv.Length();
		isBoarding = (distance < 50. && speed < 1. && commands.Has(Command::BOARD));
		if(isBoarding && !CanBeCarried())
		{
			if(!target->IsDisabled() && government->IsEnemy(target->government))
				isBoarding = false;
			else if(target->IsDestroyed() || target->IsLanding() || target->IsHyperspacing()
					|| target->GetSystem() != GetSystem())
				isBoarding = false;
		}
		if(isBoarding && !pilotError)
		{
			Angle facing = angle;
			bool left = target->Unit().Cross(facing.Unit()) < 0.;
			double turn = left - !left;
			
			// Check if the ship will still be pointing to the same side of the target
			// angle if it turns by this amount.
			facing += TurnRate() * turn;
			bool stillLeft = target->Unit().Cross(facing.Unit()) < 0.;
			if(left != stillLeft)
				turn = 0.;
			angle += TurnRate() * turn;
			
			velocity += dv.Unit() * .1;
			position += dp.Unit() * .5;
			
			if(distance < 10. && speed < 1. && (CanBeCarried() || !turn))
			{
				if(cloak)
				{
					// Allow the player to get all the way to the end of the
					// boarding sequence (including locking on to the ship) but
					// not to actually board, if they are cloaked.
					if(isYours)
						Messages::Add("You cannot board a ship while cloaked.", Messages::Importance::High);
				}
				else
				{
					isBoarding = false;
					bool isEnemy = government->IsEnemy(target->government);
					if(isEnemy && Random::Real() < target->Attributes().Get("self destruct"))
					{
						Messages::Add("The " + target->ModelName() + " \"" + target->Name()
							+ "\" has activated its self-destruct mechanism.", Messages::Importance::High);
						GetTargetShip()->SelfDestruct();
					}
					else
						hasBoarded = true;
				}
			}
		}
	}
	
	// Clear your target if it is destroyed. This is only important for NPCs,
	// because ordinary ships cease to exist once they are destroyed.
	target = GetTargetShip();
	if(target && target->IsDestroyed() && target->explosionCount >= target->explosionTotal)
		targetShip.reset();
	
	// Finally, move the ship and create any movement visuals.
	position += velocity;
	if(isUsingAfterburner && !Attributes().AfterburnerEffects().empty())
		for(const EnginePoint &point : enginePoints)
		{
			Point pos = angle.Rotate(point) * Zoom() + position;
			// Stream the afterburner effects outward in the direction the engines are facing.
			Point effectVelocity = velocity - 6. * angle.Unit();
			for(auto &&it : Attributes().AfterburnerEffects())
				for(int i = 0; i < it.second; ++i)
					visuals.emplace_back(*it.first, pos, effectVelocity, angle);
		}
}



// Generate energy, heat, etc. (This is called by Move().)
void Ship::DoGeneration()
{
	// First, allow any carried ships to do their own generation.
	for(const Bay &bay : bays)
		if(bay.ship)
			bay.ship->DoGeneration();
	
	// Shield and hull recharge. This uses whatever energy is left over from the
	// previous frame, so that it will not steal energy from movement, etc.
	if(!isDisabled)
	{
		// Priority of repairs:
		// 1. Ship's own hull
		// 2. Ship's own shields
		// 3. Hull of carried fighters
		// 4. Shields of carried fighters
		// 5. Transfer of excess energy and fuel to carried fighters.
		
		const double hullAvailable = attributes.Get("hull repair rate") * (1. + attributes.Get("hull repair multiplier"));
		const double hullEnergy = (attributes.Get("hull energy") * (1. + attributes.Get("hull energy multiplier"))) / hullAvailable;
		const double hullFuel = (attributes.Get("hull fuel") * (1. + attributes.Get("hull fuel multiplier"))) / hullAvailable;
		const double hullHeat = (attributes.Get("hull heat") * (1. + attributes.Get("hull heat multiplier"))) / hullAvailable;
		double hullRemaining = hullAvailable;
		if(!hullDelay)
			DoRepair(hull, hullRemaining, attributes.Get("hull"), energy, hullEnergy, fuel, hullFuel, heat, hullHeat);
		
		const double shieldsAvailable = attributes.Get("shield generation") * (1. + attributes.Get("shield generation multiplier"));
		const double shieldsEnergy = (attributes.Get("shield energy") * (1. + attributes.Get("shield energy multiplier"))) / shieldsAvailable;
		const double shieldsFuel = (attributes.Get("shield fuel") * (1. + attributes.Get("shield fuel multiplier"))) / shieldsAvailable;
		const double shieldsHeat = (attributes.Get("shield heat") * (1. + attributes.Get("shield heat multiplier"))) / shieldsAvailable;
		double shieldsRemaining = shieldsAvailable;
		if(!shieldDelay)
			DoRepair(shields, shieldsRemaining, attributes.Get("shields"), energy, shieldsEnergy, fuel, shieldsFuel, heat, shieldsHeat);
		
		if(!bays.empty())
		{
			// If this ship is carrying fighters, determine their repair priority.
			vector<pair<double, Ship *>> carried;
			for(const Bay &bay : bays)
				if(bay.ship)
					carried.emplace_back(1. - bay.ship->Health(), bay.ship.get());
			sort(carried.begin(), carried.end(), (isYours && Preferences::Has(FIGHTER_REPAIR))
				// Players may use a parallel strategy, to launch fighters in waves.
				? [] (const pair<double, Ship *> &lhs, const pair<double, Ship *> &rhs)
					{ return lhs.first > rhs.first; }
				// The default strategy is to prioritize the healthiest ship first, in
				// order to get fighters back out into the battle as soon as possible.
				: [] (const pair<double, Ship *> &lhs, const pair<double, Ship *> &rhs)
					{ return lhs.first < rhs.first; }
			);
			
			// Apply shield and hull repair to carried fighters.
			for(const pair<double, Ship *> &it : carried)
			{
				Ship &ship = *it.second;
				if(!hullDelay)
					DoRepair(ship.hull, hullRemaining, ship.attributes.Get("hull"), energy, hullEnergy, heat, hullHeat, fuel, hullFuel);
				if(!shieldDelay)
					DoRepair(ship.shields, shieldsRemaining, ship.attributes.Get("shields"), energy, shieldsEnergy, heat, shieldsHeat, fuel, shieldsFuel);
			}
			
			// Now that there is no more need to use energy for hull and shield
			// repair, if there is still excess energy, transfer it.
			double energyRemaining = min(0., energy - attributes.Get("energy capacity"));
			double fuelRemaining = min(0., fuel - attributes.Get("fuel capacity"));
			for(const pair<double, Ship *> &it : carried)
			{
				Ship &ship = *it.second;
				DoRepair(ship.energy, energyRemaining, ship.attributes.Get("energy capacity"));
				DoRepair(ship.fuel, fuelRemaining, ship.attributes.Get("fuel capacity"));
			}
		}
		// Decrease the shield and hull delays by 1 now that shield generation
		// and hull repair have been skipped over.
		shieldDelay = max(0, shieldDelay - 1);
		hullDelay = max(0, hullDelay - 1);
	}
	
	// Handle ionization effects, etc.
	// TODO: Mothership gives status resistance to carried ships?
	if(ionization)
	{
		double ionResistance = attributes.Get("ion resistance");
		double ionEnergy = attributes.Get("ion resistance energy") / ionResistance;
		double ionFuel = attributes.Get("ion resistance fuel") / ionResistance;
		double ionHeat = attributes.Get("ion resistance heat") / ionResistance;
		DoStatusEffect(isDisabled, ionization, ionResistance, energy, ionEnergy, fuel, ionFuel, heat, ionHeat);
	}
	
	if(disruption)
	{
		double disruptionResistance = attributes.Get("disruption resistance");
		double disruptionEnergy = attributes.Get("disruption resistance energy") / disruptionResistance;
		double disruptionFuel = attributes.Get("disruption resistance fuel") / disruptionResistance;
		double disruptionHeat = attributes.Get("disruption resistance heat") / disruptionResistance;
		DoStatusEffect(isDisabled, disruption, disruptionResistance, energy, disruptionEnergy, fuel, disruptionFuel, heat, disruptionHeat);
	}
	
	if(slowness)
	{
		double slowingResistance = attributes.Get("slowing resistance");
		double slowingEnergy = attributes.Get("slowing resistance energy") / slowingResistance;
		double slowingFuel = attributes.Get("slowing resistance fuel") / slowingResistance;
		double slowingHeat = attributes.Get("slowing resistance heat") / slowingResistance;
		DoStatusEffect(isDisabled, slowness, slowingResistance, energy, slowingEnergy, fuel, slowingFuel, heat, slowingHeat);
	}
	
	// When ships recharge, what actually happens is that they can exceed their
	// maximum capacity for the rest of the turn, but must be clamped to the
	// maximum here before they gain more. This is so that, for example, a ship
	// with no batteries but a good generator can still move.
	energy = min(energy, attributes.Get("energy capacity"));
	fuel = min(fuel, attributes.Get("fuel capacity"));
	
	heat -= heat * HeatDissipation();
	if(heat > MaximumHeat())
		isOverheated = true;
	else if(heat < .9 * MaximumHeat())
		isOverheated = false;
	
	double maxShields = attributes.Get("shields");
	shields = min(shields, maxShields);
	double maxHull = attributes.Get("hull");
	hull = min(hull, maxHull);
	
	isDisabled = isOverheated || hull < MinimumHull() || (!crew && RequiredCrew());
	
	// Whenever not actively scanning, the amount of scan information the ship
	// has "decays" over time. For a scanner with a speed of 1, one second of
	// uninterrupted scanning is required to successfully scan its target.
	// Only apply the decay if not already done scanning the target.
	if(cargoScan < SCAN_TIME)
		cargoScan = max(0., cargoScan - 1.);
	if(outfitScan < SCAN_TIME)
		outfitScan = max(0., outfitScan - 1.);
	
	// Update ship supply levels.
	energy -= ionization;
	if(isDisabled)
		PauseAnimation();
	else
	{
		// Ramscoops work much better when close to the system center. Even if a
		// ship has no ramscoop, it can harvest a tiny bit of fuel by flying
		// close to the star. Carried fighters can't collect fuel or energy this way.
		if(currentSystem)
		{
			double scale = .2 + 1.8 / (.001 * position.Length() + 1);
			fuel += currentSystem->SolarWind() * .03 * scale * (sqrt(attributes.Get("ramscoop")) + .05 * scale);
			
			double solarScaling = currentSystem->SolarPower() * scale;
			energy += solarScaling * attributes.Get("solar collection");
			heat += solarScaling * attributes.Get("solar heat");
		}
		
		double coolingEfficiency = CoolingEfficiency();
		energy += attributes.Get("energy generation") - attributes.Get("energy consumption");
		fuel += attributes.Get("fuel generation");
		heat += attributes.Get("heat generation");
		heat -= coolingEfficiency * attributes.Get("cooling");
		
		// Convert fuel into energy and heat only when the required amount of fuel is available.
		if(attributes.Get("fuel consumption") <= fuel)
		{	
			fuel -= attributes.Get("fuel consumption");
			energy += attributes.Get("fuel energy");
			heat += attributes.Get("fuel heat");
		}
		
		// Apply active cooling. The fraction of full cooling to apply equals
		// your ship's current fraction of its maximum temperature.
		double activeCooling = coolingEfficiency * attributes.Get("active cooling");
		if(activeCooling > 0. && heat > 0. && energy >= 0.)
		{
			// Although it's a misuse of this feature, handle the case where
			// "active cooling" does not require any energy.
			double coolingEnergy = attributes.Get("cooling energy");
			if(coolingEnergy)
			{
				double spentEnergy = min(energy, coolingEnergy * min(1., Heat()));
				heat -= activeCooling * spentEnergy / coolingEnergy;
				energy -= spentEnergy;
			}
			else
				heat -= activeCooling;
		}
	}
	
	// Don't allow any levels to drop below zero.
	fuel = max(0., fuel);
	energy = max(0., energy);
	heat = max(0., heat);
}



// Launch any ships that are ready to launch.
void Ship::Launch(list<shared_ptr<Ship>> &ships, vector<Visual> &visuals)
{
	// Allow carried ships to launch from a disabled ship, but not from a ship that
	// is landing, jumping, or cloaked. If already destroyed (e.g. self-destructing),
	// eject any ships still docked, possibly destroying them in the process.
	bool ejecting = IsDestroyed();
	if(!ejecting && (!commands.Has(Command::DEPLOY) || zoom != 1.f || hyperspaceCount || cloak))
		return;
	
	for(Bay &bay : bays)
		if(bay.ship && ((bay.ship->Commands().Has(Command::DEPLOY) && !Random::Int(40 + 20 * !bay.ship->attributes.Get("automaton")))
				|| (ejecting && !Random::Int(6))))
		{
			// Resupply any ships launching of their own accord.
			if(!ejecting)
			{
				// TODO: Restock fighter weaponry that needs ammo.
				
				// This ship will refuel naturally based on the carrier's fuel
				// collection, but the carrier may have some reserves to spare.
				double maxFuel = bay.ship->attributes.Get("fuel capacity");
				if(maxFuel)
				{
					double spareFuel = fuel - JumpFuel();
					if(spareFuel > 0.)
						TransferFuel(min(maxFuel - bay.ship->fuel, spareFuel), bay.ship.get());
					// If still low or out-of-fuel, re-stock the carrier and don't launch.
					if(bay.ship->fuel < .25 * maxFuel)
					{
						TransferFuel(bay.ship->fuel, this);
						continue;
					}
				}
			}
			// Those being ejected may be destroyed if they are already injured.
			else if(bay.ship->Health() < Random::Real())
				bay.ship->SelfDestruct();
			
			ships.push_back(bay.ship);
			double maxV = bay.ship->MaxVelocity() * (1 + bay.ship->IsDestroyed());
			Point exitPoint = position + angle.Rotate(bay.point);
			// When ejected, ships depart haphazardly.
			Angle launchAngle = ejecting ? Angle(exitPoint - position) : angle + bay.facing;
			Point v = velocity + (.3 * maxV) * launchAngle.Unit() + (.2 * maxV) * Angle::Random().Unit();
			bay.ship->Place(exitPoint, v, launchAngle);
			bay.ship->SetSystem(currentSystem);
			bay.ship->SetParent(shared_from_this());
			bay.ship->UnmarkForRemoval();
			// Update the cached sum of carried ship masses.
			carriedMass -= bay.ship->Mass();
			// Create the desired launch effects.
			for(const Effect *effect : bay.launchEffects)
				visuals.emplace_back(*effect, exitPoint, velocity, launchAngle);
			
			bay.ship.reset();
		}
}



// Check if this ship is boarding another ship.
shared_ptr<Ship> Ship::Board(bool autoPlunder)
{
	if(!hasBoarded)
		return shared_ptr<Ship>();
	hasBoarded = false;
	
	shared_ptr<Ship> victim = GetTargetShip();
	if(CannotAct() || !victim || victim->IsDestroyed() || victim->GetSystem() != GetSystem())
		return shared_ptr<Ship>();
	
	// For a fighter or drone, "board" means "return to ship."
	if(CanBeCarried())
	{
		SetTargetShip(shared_ptr<Ship>());
		if(!victim->IsDisabled() && victim->GetGovernment() == government)
			victim->Carry(shared_from_this());
		return shared_ptr<Ship>();
	}
	
	// Board a friendly ship, to repair or refuel it.
	if(!government->IsEnemy(victim->GetGovernment()))
	{
		SetShipToAssist(shared_ptr<Ship>());
		SetTargetShip(shared_ptr<Ship>());
		bool helped = victim->isDisabled;
		victim->hull = min(max(victim->hull, victim->MinimumHull() * 1.5), victim->attributes.Get("hull"));
		victim->isDisabled = false;
		// Transfer some fuel if needed.
		if(!victim->JumpsRemaining() && CanRefuel(*victim))
		{
			helped = true;
			TransferFuel(victim->JumpFuelMissing(), victim.get());
		}
		if(helped)
		{
			pilotError = 120;
			victim->pilotError = 120;
		}
		return victim;
	}
	if(!victim->IsDisabled())
		return shared_ptr<Ship>();
	
	// If the boarding ship is the player, they will choose what to plunder.
	// Always take fuel if you can.
	victim->TransferFuel(victim->fuel, this);
	if(autoPlunder)
	{
		// Take any commodities that fit.
		victim->cargo.TransferAll(cargo, false);
		
		// Pause for two seconds before moving on.
		pilotError = 120;
	}
	
	// Stop targeting this ship (so you will not board it again right away).
	if(!autoPlunder || personality.Disables())
		SetTargetShip(shared_ptr<Ship>());
	return victim;
}



// Scan the target, if able and commanded to. Return a ShipEvent bitmask
// giving the types of scan that succeeded.
int Ship::Scan()
{
	if(!commands.Has(Command::SCAN) || CannotAct())
		return 0;
	
	shared_ptr<const Ship> target = GetTargetShip();
	if(!(target && target->IsTargetable()))
		return 0;
	
	// The range of a scanner is proportional to the square root of its power.
	double cargoDistance = 100. * sqrt(attributes.Get("cargo scan power"));
	double outfitDistance = 100. * sqrt(attributes.Get("outfit scan power"));
	
	// Bail out if this ship has no scanners.
	if(!cargoDistance && !outfitDistance)
		return 0;
	
	// Scanning speed also uses a square root, so you need four scanners to get
	// twice the speed out of them.
	double cargoSpeed = sqrt(attributes.Get("cargo scan speed"));
	if(!cargoSpeed)
		cargoSpeed = 1.;
	double outfitSpeed = sqrt(attributes.Get("outfit scan speed"));
	if(!outfitSpeed)
		outfitSpeed = 1.;
	
	// Check how close this ship is to the target it is trying to scan.
	double distance = (target->position - position).Length();
	
	// Check if either scanner has finished scanning.
	bool startedScanning = false;
	bool activeScanning = false;
	int result = 0;
	auto doScan = [&](double &elapsed, const double speed, const double scannerRange, const int event) -> void
	{
		if(elapsed < SCAN_TIME && distance < scannerRange)
		{
			startedScanning |= !elapsed;
			activeScanning = true;
			// To make up for the scan decay above:
			elapsed += speed + 1.;
			if(elapsed >= SCAN_TIME)
				result |= event;
		}
	};
	doScan(cargoScan, cargoSpeed, cargoDistance, ShipEvent::SCAN_CARGO);
	doScan(outfitScan, outfitSpeed, outfitDistance, ShipEvent::SCAN_OUTFITS);
	
	// Play the scanning sound if the actor or the target is the player's ship.
	if(isYours || (target->isYours && activeScanning))
		Audio::Play(Audio::Get("scan"), Position());
	
	if(startedScanning && isYours)
	{
		if(!target->Name().empty())
			Messages::Add("Attempting to scan the " + target->Noun() + " \"" + target->Name() + "\"."
				, Messages::Importance::Low);
		else
			Messages::Add("Attempting to scan the selected " + target->Noun() + "."
				, Messages::Importance::Low);
	}
	else if(startedScanning && target->isYours)
		Messages::Add("The " + government->GetName() + " " + Noun() + " \""
			+ Name() + "\" is attempting to scan you.", Messages::Importance::Low);
	
	if(target->isYours && !isYours)
	{
		if(result & ShipEvent::SCAN_CARGO)
			Messages::Add("The " + government->GetName() + " " + Noun() + " \""
					+ Name() + "\" completed its scan of your cargo.", Messages::Importance::High);
		if(result & ShipEvent::SCAN_OUTFITS)
			Messages::Add("The " + government->GetName() + " " + Noun() + " \""
					+ Name() + "\" completed its scan of your outfits.", Messages::Importance::High);
	}
	
	return result;
}



// Find out what fraction of the scan is complete.
double Ship::CargoScanFraction() const
{
	return cargoScan / SCAN_TIME;
}



double Ship::OutfitScanFraction() const
{
	return outfitScan / SCAN_TIME;
}



// Fire any weapons that are ready to fire. If an anti-missile is ready,
// instead of firing here this function returns true and it can be fired if
// collision detection finds a missile in range.
bool Ship::Fire(vector<Projectile> &projectiles, vector<Visual> &visuals)
{
	isInSystem = true;
	forget = 0;
	
	// A ship that is about to die creates a special single-turn "projectile"
	// representing its death explosion.
	if(IsDestroyed() && explosionCount == explosionTotal && explosionWeapon)
		projectiles.emplace_back(position, explosionWeapon);
	
	if(CannotAct())
		return false;
	
	antiMissileRange = 0.;
	
	const vector<Hardpoint> &hardpoints = armament.Get();
	for(unsigned i = 0; i < hardpoints.size(); ++i)
	{
		const Weapon *weapon = hardpoints[i].GetOutfit();
		if(weapon && CanFire(weapon))
		{
			if(weapon->AntiMissile())
				antiMissileRange = max(antiMissileRange, weapon->Velocity() + weaponRadius);
			else if(commands.HasFire(i))
				armament.Fire(i, *this, projectiles, visuals);
		}
	}
	
	armament.Step(*this);
	
	return antiMissileRange;
}



// Fire an anti-missile.
bool Ship::FireAntiMissile(const Projectile &projectile, vector<Visual> &visuals)
{
	if(projectile.Position().Distance(position) > antiMissileRange)
		return false;
	if(CannotAct())
		return false;
	
	const vector<Hardpoint> &hardpoints = armament.Get();
	for(unsigned i = 0; i < hardpoints.size(); ++i)
	{
		const Weapon *weapon = hardpoints[i].GetOutfit();
		if(weapon && CanFire(weapon))
			if(armament.FireAntiMissile(i, *this, projectile, visuals))
				return true;
	}
	
	return false;
}



const System *Ship::GetSystem() const
{
	return currentSystem;
}



// If the ship is landed, get the planet it has landed on.
const Planet *Ship::GetPlanet() const
{
	return zoom ? nullptr : landingPlanet;
}



bool Ship::IsCapturable() const
{
	return isCapturable;
}



bool Ship::IsTargetable() const
{
	return (zoom == 1.f && !explosionRate && !forget && !isInvisible && cloak < 1. && hull >= 0. && hyperspaceCount < 70);
}



bool Ship::IsOverheated() const
{
	return isOverheated;
}



bool Ship::IsDisabled() const
{
	if(!isDisabled)
		return false;
	
	double minimumHull = MinimumHull();
	bool needsCrew = RequiredCrew() != 0;
	return (hull < minimumHull || (!crew && needsCrew));
}



bool Ship::IsBoarding() const
{
	return isBoarding;
}



bool Ship::IsLanding() const
{
	return landingPlanet;
}



// Check if this ship is currently able to begin landing on its target.
bool Ship::CanLand() const
{
	if(!GetTargetStellar() || !GetTargetStellar()->GetPlanet() || isDisabled || IsDestroyed())
		return false;
	
	if(!GetTargetStellar()->GetPlanet()->CanLand(*this))
		return false;
	
	Point distance = GetTargetStellar()->Position() - position;
	double speed = velocity.Length();
	
	return (speed < 1. && distance.Length() < GetTargetStellar()->Radius());
}



bool Ship::CannotAct() const
{
	return (zoom != 1.f || isDisabled || hyperspaceCount || pilotError || cloak);
}



double Ship::Cloaking() const
{
	return isInvisible ? 1. : cloak;
}



bool Ship::IsEnteringHyperspace() const
{
	return hyperspaceSystem;
}



bool Ship::IsHyperspacing() const
{
	return hyperspaceCount != 0;
}



// Check if this ship is hyperspacing, specifically via a jump drive.
bool Ship::IsUsingJumpDrive() const
{
	return (hyperspaceSystem || hyperspaceCount) && isUsingJumpDrive;
}



// Check if this ship is currently able to enter hyperspace to it target.
bool Ship::IsReadyToJump(bool waitingIsReady) const
{
	// Ships can't jump while waiting for someone else, carried, or if already jumping.
	if(IsDisabled() || (!waitingIsReady && commands.Has(Command::WAIT))
			|| hyperspaceCount || !targetSystem || !currentSystem)
		return false;
	
	// Check if the target system is valid and there is enough fuel to jump.
	double fuelCost = JumpFuel(targetSystem);
	if(!fuelCost || fuel < fuelCost)
		return false;
	
	Point direction = targetSystem->Position() - currentSystem->Position();
	bool isJump = !attributes.Get("hyperdrive") || !currentSystem->Links().count(targetSystem);
	double scramThreshold = attributes.Get("scram drive");
	
	// The ship can only enter hyperspace if it is traveling slowly enough
	// and pointed in the right direction.
	if(!isJump && scramThreshold)
	{
		double deviation = fabs(direction.Unit().Cross(velocity));
		if(deviation > scramThreshold)
			return false;
	}
	else if(velocity.Length() > attributes.Get("jump speed"))
		return false;
	
	if(!isJump)
	{
		// Figure out if we're within one turn step of facing this system.
		bool left = direction.Cross(angle.Unit()) < 0.;
		Angle turned = angle + TurnRate() * (left - !left);
		bool stillLeft = direction.Cross(turned.Unit()) < 0.;
	
		if(left == stillLeft)
			return false;
	}
	
	return true;
}



// Get this ship's custom swizzle.
int Ship::CustomSwizzle() const
{
	return customSwizzle;
}


// Check if the ship is thrusting. If so, the engine sound should be played.
bool Ship::IsThrusting() const
{
	return isThrusting;
}



bool Ship::IsReversing() const
{
	return isReversing;
}



bool Ship::IsSteering() const
{
	return isSteering;
}



double Ship::SteeringDirection() const
{
	return steeringDirection;
}



// Get the points from which engine flares should be drawn.
const vector<Ship::EnginePoint> &Ship::EnginePoints() const
{
	return enginePoints;
}



const vector<Ship::EnginePoint> &Ship::ReverseEnginePoints() const
{
	return reverseEnginePoints;
}



const vector<Ship::EnginePoint> &Ship::SteeringEnginePoints() const
{
	return steeringEnginePoints;
}



// Reduce a ship's hull to low enough to disable it. This is so a ship can be
// created as a derelict.
void Ship::Disable()
{
	shields = 0.;
	hull = min(hull, .5 * MinimumHull());
	isDisabled = true;
}



// Mark a ship as destroyed.
void Ship::Destroy()
{
	hull = -1.;
}



// Trigger the death of this ship.
void Ship::SelfDestruct()
{
	Destroy();
	explosionRate = 1024;
}



void Ship::Restore()
{
	hull = 0.;
	explosionCount = 0;
	explosionRate = 0;
	UnmarkForRemoval();
	Recharge(true);
}



// Check if this ship has been destroyed.
bool Ship::IsDestroyed() const
{
	return (hull < 0.);
}



// Recharge and repair this ship (e.g. because it has landed).
void Ship::Recharge(bool atSpaceport)
{
	if(IsDestroyed())
		return;
	
	if(atSpaceport)
	{
		crew = min<int>(max(crew, RequiredCrew()), attributes.Get("bunks"));
		fuel = attributes.Get("fuel capacity");
	}
	pilotError = 0;
	pilotOkay = 0;
	
	if(atSpaceport || attributes.Get("shield generation"))
		shields = attributes.Get("shields");
	if(atSpaceport || attributes.Get("hull repair rate"))
		hull = attributes.Get("hull");
	if(atSpaceport || attributes.Get("energy generation"))
		energy = attributes.Get("energy capacity");
	
	heat = IdleHeat();
	ionization = 0.;
	disruption = 0.;
	slowness = 0.;
	shieldDelay = 0;
	hullDelay = 0;
}



bool Ship::CanRefuel(const Ship &other) const
{
	return (fuel - JumpFuel(targetSystem) >= other.JumpFuelMissing());
}



double Ship::TransferFuel(double amount, Ship *to)
{
	amount = max(fuel - attributes.Get("fuel capacity"), amount);
	if(to)
	{
		amount = min(to->attributes.Get("fuel capacity") - to->fuel, amount);
		to->fuel += amount;
	}
	fuel -= amount;
	return amount;
}



// Convert this ship from one government to another, as a result of boarding
// actions (if the player is capturing) or player death (poor decision-making).
void Ship::WasCaptured(const shared_ptr<Ship> &capturer)
{
	// Repair up to the point where this ship is just barely not disabled.
	hull = min(max(hull, MinimumHull() * 1.5), attributes.Get("hull"));
	isDisabled = false;
	
	// Set the new government.
	government = capturer->GetGovernment();
	
	// Transfer some crew over. Only transfer the bare minimum unless even that
	// is not possible, in which case, share evenly.
	int totalRequired = capturer->RequiredCrew() + RequiredCrew();
	int transfer = RequiredCrew() - crew;
	if(transfer > 0)
	{
		if(totalRequired > capturer->Crew() + crew)
			transfer = max(crew ? 0 : 1, (capturer->Crew() * transfer) / totalRequired);
		capturer->AddCrew(-transfer);
		AddCrew(transfer);
	}
	
	commands.Clear();
	// Set the capturer as this ship's parent.
	SetParent(capturer);
	// Clear this ship's previous targets.
	SetTargetShip(shared_ptr<Ship>());
	SetTargetStellar(nullptr);
	SetTargetSystem(nullptr);
	shipToAssist.reset();
	targetAsteroid.reset();
	targetFlotsam.reset();
	hyperspaceSystem = nullptr;
	landingPlanet = nullptr;
	
	// This ship behaves like its new parent does.
	isSpecial = capturer->isSpecial;
	isYours = capturer->isYours;
	personality = capturer->personality;
	
	// Fighters should flee a disabled ship, but if the player manages to capture
	// the ship before they flee, the fighters are captured, too.
	for(const Bay &bay : bays)
		if(bay.ship)
			bay.ship->WasCaptured(capturer);
	// If a flagship is captured, its escorts become independent.
	for(const auto &it : escorts)
	{
		shared_ptr<Ship> escort = it.lock();
		if(escort)
			escort->parent.reset();
	}
	// This ship should not care about its now-unallied escorts.
	escorts.clear();
}



// Get characteristics of this ship, as a fraction between 0 and 1.
double Ship::Shields() const
{
	double maximum = attributes.Get("shields");
	return maximum ? min(1., shields / maximum) : 0.;
}



double Ship::Hull() const
{
	double maximum = attributes.Get("hull");
	return maximum ? min(1., hull / maximum) : 1.;
}



double Ship::Fuel() const
{
	double maximum = attributes.Get("fuel capacity");
	return maximum ? min(1., fuel / maximum) : 0.;
}



double Ship::Energy() const
{
	double maximum = attributes.Get("energy capacity");
	return maximum ? min(1., energy / maximum) : (hull > 0.) ? 1. : 0.;
}



// Allow returning a heat value greater than 1 (i.e. conveying how overheated
// this ship has become).
double Ship::Heat() const
{
	double maximum = MaximumHeat();
	return maximum ? heat / maximum : 1.;
}



// Get the ship's "health," where <=0 is disabled and 1 means full health.
double Ship::Health() const
{
	double minimumHull = MinimumHull();
	double hullDivisor = attributes.Get("hull") - minimumHull;
	double divisor = attributes.Get("shields") + hullDivisor;
	// This should not happen, but just in case.
	if(divisor <= 0. || hullDivisor <= 0.)
		return 0.;
	
	double spareHull = hull - minimumHull;
	// Consider hull-only and pooled health, compensating for any reductions by disruption damage.
	return min(spareHull / hullDivisor, (spareHull + shields / (1. + disruption * .01)) / divisor);
}



// Get the hull fraction at which this ship is disabled.
double Ship::DisabledHull() const
{
	double hull = attributes.Get("hull");
	double minimumHull = MinimumHull();
	
	return (hull > 0. ? minimumHull / hull : 0.);
}



int Ship::JumpsRemaining(bool followParent) const
{
	// Make sure this ship has some sort of hyperdrive, and if so return how
	// many jumps it can make.
	double jumpFuel = 0.;
	if(!targetSystem && followParent)
	{
		// If this ship has no destination, the parent's substitutes for it,
		// but only if the location is reachable.
		auto p = GetParent();
		if(p)
			jumpFuel = JumpFuel(p->GetTargetSystem());
	}
	if(!jumpFuel)
		jumpFuel = JumpFuel(targetSystem);
	return jumpFuel ? fuel / jumpFuel : 0.;
}



double Ship::JumpFuel(const System *destination) const
{
	// A currently-carried ship requires no fuel to jump, because it cannot jump.
	if(!currentSystem)
		return 0.;
	
	// If no destination is given, return the maximum fuel per jump.
	if(!destination)
		return max(JumpDriveFuel(), HyperdriveFuel());
	
	bool linked = currentSystem->Links().count(destination);
	// Figure out what sort of jump we're making.
	if(attributes.Get("hyperdrive") && linked)
		return HyperdriveFuel();
	
	if(attributes.Get("jump drive") && currentSystem->JumpNeighbors(JumpRange()).count(destination))
		return JumpDriveFuel((linked || currentSystem->JumpRange()) ? 0. : currentSystem->Position().Distance(destination->Position()));
	
	// If the given system is not a possible destination, return 0.
	return 0.;
}



double Ship::JumpRange(bool getCached) const
{
	if(getCached)
		return jumpRange;
	
	// Ships without a jump drive have no jump range.
	if(!attributes.Get("jump drive"))
		return 0.;
	
	// Find the outfit that provides the farthest jump range.
	double best = 0.;
	// Make it possible for the jump range to be integrated into a ship.
	if(baseAttributes.Get("jump drive"))
	{
		best = baseAttributes.Get("jump range");
		if(!best)
			best = System::DEFAULT_NEIGHBOR_DISTANCE;
	}
	// Search through all the outfits.
	for(const auto &it : outfits)
		if(it.first->Get("jump drive"))
		{
			double range = it.first->Get("jump range");
			if(!range)
				range = System::DEFAULT_NEIGHBOR_DISTANCE;
			if(!best || range > best)
				best = range;
		}
	return best;
}



// Get the cost of making a jump of the given type (if possible).
double Ship::HyperdriveFuel() const
{
	// Don't bother searching through the outfits if there is no hyperdrive.
	if(!attributes.Get("hyperdrive"))
		return JumpDriveFuel();
	
	if(attributes.Get("scram drive"))
		return BestFuel("hyperdrive", "scram drive", 150.);
	
	return BestFuel("hyperdrive", "", 100.);
}



double Ship::JumpDriveFuel(double jumpDistance) const
{
	// Don't bother searching through the outfits if there is no jump drive.
	if(!attributes.Get("jump drive"))
		return 0.;
	
	return BestFuel("jump drive", "", 200., jumpDistance);
}



double Ship::JumpFuelMissing() const
{
	// Used for smart refueling: transfer only as much as really needed
	// includes checking if fuel cap is high enough at all
	double jumpFuel = JumpFuel(targetSystem);
	if(!jumpFuel || fuel > jumpFuel || jumpFuel > attributes.Get("fuel capacity"))
		return 0.;
	
	return jumpFuel - fuel;
}



// Get the heat level at idle.
double Ship::IdleHeat() const
{
	// This ship's cooling ability:
	double coolingEfficiency = CoolingEfficiency();
	double cooling = coolingEfficiency * attributes.Get("cooling");
	double activeCooling = coolingEfficiency * attributes.Get("active cooling");
	
	// Idle heat is the heat level where:
	// heat = heat * diss + heatGen - cool - activeCool * heat / (100 * mass)
	// heat = heat * (diss - activeCool / (100 * mass)) + (heatGen - cool)
	// heat * (1 - diss + activeCool / (100 * mass)) = (heatGen - cool)
	double production = max(0., attributes.Get("heat generation") - cooling);
	double dissipation = HeatDissipation() + activeCooling / MaximumHeat();
	if(!dissipation) return production ? numeric_limits<double>::max() : 0;
	return production / dissipation;
}



// Get the heat dissipation, in heat units per heat unit per frame.
double Ship::HeatDissipation() const
{
	return .001 * attributes.Get("heat dissipation");
}



// Get the maximum heat level, in heat units (not temperature).
double Ship::MaximumHeat() const
{
	return MAXIMUM_TEMPERATURE * (cargo.Used() + attributes.Mass());
}



// Calculate the multiplier for cooling efficiency.
double Ship::CoolingEfficiency() const
{
	// This is an S-curve where the efficiency is 100% if you have no outfits
	// that create "cooling inefficiency", and as that value increases the
	// efficiency stays high for a while, then drops off, then approaches 0.
	double x = attributes.Get("cooling inefficiency");
	return 2. + 2. / (1. + exp(x / -2.)) - 4. / (1. + exp(x / -4.));
}



int Ship::Crew() const
{
	return crew;
}



int Ship::RequiredCrew() const
{
	if(attributes.Get("automaton"))
		return 0;
	
	// Drones do not need crew, but all other ships need at least one.
	return max<int>(1, attributes.Get("required crew"));
}



void Ship::AddCrew(int count)
{
	crew = min<int>(crew + count, attributes.Get("bunks"));
}



// Check if this is a ship that can be used as a flagship.
bool Ship::CanBeFlagship() const
{
	return RequiredCrew() && Crew() && !IsDisabled();
}



double Ship::Mass() const
{
	return carriedMass + cargo.Used() + attributes.Mass();
}



double Ship::TurnRate() const
{
	return attributes.Get("turn") / Mass();
}



double Ship::Acceleration() const
{
	double thrust = attributes.Get("thrust");
	return (thrust ? thrust : attributes.Get("afterburner thrust")) / Mass();
}



double Ship::MaxVelocity() const
{
	// v * drag / mass == thrust / mass
	// v * drag == thrust
	// v = thrust / drag
	double thrust = attributes.Get("thrust");
	return (thrust ? thrust : attributes.Get("afterburner thrust")) / attributes.Get("drag");
}



double Ship::MaxReverseVelocity() const
{
	return attributes.Get("reverse thrust") / attributes.Get("drag");
}



// This ship just got hit by the given weapon. Take damage
// according to the weapon and the characteristics of how
// it hit this ship, and add any visuals created as a result
// of being hit.
int Ship::TakeDamage(vector<Visual> &visuals, const Weapon &weapon, double damageScaling, double distanceTraveled, const Point &damagePosition, const Government *sourceGovernment, bool isBlast)
{
	if(isBlast && weapon.IsDamageScaled())
	{
		// Scale blast damage based on the distance from the blast
		// origin and if the projectile uses a trigger radius. The
		// point of contact must be measured on the sprite outline.
		// scale = (1 + (tr / (2 * br))^2) / (1 + r^4)^2
		double blastRadius = max(1., weapon.BlastRadius());
		double radiusRatio = weapon.TriggerRadius() / blastRadius;
		double k = !radiusRatio ? 1. : (1. + .25 * radiusRatio * radiusRatio);
		// Rather than exactly compute the distance between the explosion and
		// the closest point on the ship, estimate it using the mask's Radius.
		double d = max(0., (damagePosition - position).Length() - GetMask().Radius());
		double rSquared = d * d / (blastRadius * blastRadius);
		damageScaling *= k / ((1. + rSquared * rSquared) * (1. + rSquared * rSquared));
	}
	if(weapon.HasDamageDropoff())
		damageScaling *= weapon.DamageDropoff(distanceTraveled);
	
	double shieldDamage = (weapon.ShieldDamage() + weapon.RelativeShieldDamage() * attributes.Get("shields"))
		* damageScaling / (1. + attributes.Get("shield protection"));
	double hullDamage = (weapon.HullDamage() + weapon.RelativeHullDamage() * attributes.Get("hull"))
		* damageScaling / (1. + attributes.Get("hull protection"));
	double energyDamage = (weapon.EnergyDamage() + weapon.RelativeEnergyDamage() * attributes.Get("energy capacity"))
		* damageScaling / (1. + attributes.Get("energy protection"));
	double fuelDamage = (weapon.FuelDamage() + weapon.RelativeFuelDamage() * attributes.Get("fuel capacity"))
		* damageScaling / (1. + attributes.Get("fuel protection"));
	double heatDamage = (weapon.HeatDamage() + weapon.RelativeHeatDamage() * MaximumHeat())
		* damageScaling / (1. + attributes.Get("heat protection"));
	double ionDamage = weapon.IonDamage() * damageScaling / (1. + attributes.Get("ion protection"));
	double disruptionDamage = weapon.DisruptionDamage() * damageScaling / (1. + attributes.Get("disruption protection"));
	double slowingDamage = weapon.SlowingDamage() * damageScaling / (1. + attributes.Get("slowing protection"));
	double hitForce = weapon.HitForce() * damageScaling / (1. + attributes.Get("force protection"));
	bool wasDisabled = IsDisabled();
	bool wasDestroyed = IsDestroyed();
	
	double shieldFraction = 1. - max(0., min(1., weapon.Piercing() / (1. + attributes.Get("piercing protection")) - attributes.Get("piercing resistance")));
	shieldFraction *= 1. / (1. + disruption * .01);
	if(shields <= 0.)
		shieldFraction = 0.;
	else if(shieldDamage > shields)
		shieldFraction = min(shieldFraction, shields / shieldDamage);
	shields -= shieldDamage * shieldFraction;
	if(shieldDamage && !isDisabled)
	{
		int disabledDelay = static_cast<int>(attributes.Get("depleted shield delay"));
		shieldDelay = max(shieldDelay, (shields <= 0. && disabledDelay) ? disabledDelay : static_cast<int>(attributes.Get("shield delay")));
	}
	hull -= hullDamage * (1. - shieldFraction);
	if(hullDamage && !isDisabled)
		hullDelay = max(hullDelay, static_cast<int>(attributes.Get("repair delay")));
	// For the following damage types, the total effect depends on how much is
	// "leaking" through the shields.
	double leakage = (1. - .5 * shieldFraction);
	// Code in Ship::Move() will handle making sure the fuel and energy amounts
	// stays in the allowable ranges.
	energy -= energyDamage * leakage;
	fuel -= fuelDamage * leakage;
	heat += heatDamage * leakage;
	ionization += ionDamage * leakage;
	disruption += disruptionDamage * leakage;
	slowness += slowingDamage * leakage;
	
	if(hitForce)
	{
		Point d = position - damagePosition;
		double distance = d.Length();
		if(distance)
			ApplyForce((hitForce / distance) * d, weapon.IsGravitational());
	}
	
	// Recalculate the disabled ship check.
	isDisabled = true;
	isDisabled = IsDisabled();
	
	// Report what happened to this ship from this weapon.
	int type = 0;
	if(!wasDisabled && isDisabled)
	{
		type |= ShipEvent::DISABLE;
		hullDelay = max(hullDelay, static_cast<int>(attributes.Get("disabled repair delay")));
	}
	if(!wasDestroyed && IsDestroyed())
		type |= ShipEvent::DESTROY;
	
	// Inflicted heat damage may also disable a ship, but does not trigger a "DISABLE" event.
	if(heat > MaximumHeat())
	{
		isOverheated = true;
		isDisabled = true;
	}
	else if(heat < .9 * MaximumHeat())
		isOverheated = false;
	
	// If this ship was hit directly and did not consider itself an enemy of the
	// ship that hit it, it is now "provoked" against that government.
	if(!isBlast && sourceGovernment && !sourceGovernment->IsEnemy(government)
			&& (Shields() < .9 || Hull() < .9 || !personality.IsForbearing())
			&& !personality.IsPacifist() && weapon.DoesDamage())
		type |= ShipEvent::PROVOKE;
	
	// Create target effect visuals, if there are any.
	for(const auto &effect : weapon.TargetEffects())
		CreateSparks(visuals, effect.first, effect.second * damageScaling);
	
	return type;
}



// Apply a force to this ship, accelerating it. This might be from a weapon
// impact, or from firing a weapon, for example.
void Ship::ApplyForce(const Point &force, bool gravitational)
{
	if(gravitational)
	{
		// Treat all ships as if they have a mass of 400. This prevents
		// gravitational hit force values from needing to be extremely
		// small in order to have a reasonable effect.
		acceleration += force / 400.;
		return;
	}
	
	double currentMass = Mass();
	if(!currentMass)
		return;
	
	// Reduce acceleration of small ships and increase acceleration of large
	// ones by having 30% of the force be based on a fixed mass of 400, i.e. the
	// mass of a typical light warship:
	acceleration += force * (.3 / 400. + .7 / currentMass);
}



bool Ship::HasBays() const
{
	return !bays.empty();
}



// Check how many bays are not occupied at present. This does not check whether
// one of your escorts plans to use that bay.
int Ship::BaysFree(const string &category) const
{
	int count = 0;
	for(const Bay &bay : bays)
		count += (bay.category == category) && !bay.ship;
	return count;
}



// Check how many bays this ship has of a given category.
int Ship::BaysTotal(const string &category) const
{
	int count = 0;
	for(const Bay &bay : bays)
		count += (bay.category == category);
	return count;
}



// Check if this ship has a bay free for the given ship, and the bay is
// not reserved for one of its existing escorts.
bool Ship::CanCarry(const Ship &ship) const
{
	if(!ship.CanBeCarried())
		return false;
	// Check only for the category that we are interested in.
	const string &category = ship.attributes.Category();
	
	int free = BaysTotal(category);
	if(!free)
		return false;
	
	for(const auto &it : escorts)
	{
		auto escort = it.lock();
		if(escort && escort.get() != &ship && escort->attributes.Category() == category 
			&& !escort->IsDestroyed())
			--free;
	}
	return (free > 0);
}



void Ship::AllowCarried(bool allowCarried)
{
	const auto &bayCategories = GameData::Category(CategoryType::BAY);
	canBeCarried = allowCarried && find(bayCategories.begin(), bayCategories.end(), attributes.Category()) != bayCategories.end();
}



bool Ship::CanBeCarried() const
{
	return canBeCarried;
}



bool Ship::Carry(const shared_ptr<Ship> &ship)
{
	if(!ship || !ship->CanBeCarried())
		return false;
	
	// Check only for the category that we are interested in.
	const string &category = ship->attributes.Category();
	
	for(Bay &bay : bays)
		if((bay.category == category) && !bay.ship)
		{
			bay.ship = ship;
			ship->SetSystem(nullptr);
			ship->SetPlanet(nullptr);
			ship->SetTargetSystem(nullptr);
			ship->SetTargetStellar(nullptr);
			ship->SetParent(shared_from_this());
			ship->isThrusting = false;
			ship->isReversing = false;
			ship->isSteering = false;
			ship->commands.Clear();
			// If this fighter collected anything in space, try to store it
			// (unless this is a player-owned ship).
			if(!isYours && cargo.Free() && !ship->Cargo().IsEmpty())
				ship->Cargo().TransferAll(cargo);
			// Return unused fuel to the carrier, for any launching fighter that needs it.
			ship->TransferFuel(ship->fuel, this);
			
			// Update the cached mass of the mothership.
			carriedMass += ship->Mass();
			return true;
		}
	return false;
}



void Ship::UnloadBays()
{
	for(Bay &bay : bays)
		if(bay.ship)
		{
			carriedMass -= bay.ship->Mass();
			bay.ship->SetSystem(currentSystem);
			bay.ship->SetPlanet(landingPlanet);
			bay.ship.reset();
		}
}



const vector<Ship::Bay> &Ship::Bays() const
{
	return bays;
}



// Adjust the positions and velocities of any visible carried fighters or
// drones. If any are visible, return true.
bool Ship::PositionFighters() const
{
	bool hasVisible = false;
	for(const Bay &bay : bays)
		if(bay.ship && bay.side)
		{
			hasVisible = true;
			bay.ship->position = angle.Rotate(bay.point) * Zoom() + position;
			bay.ship->velocity = velocity;
			bay.ship->angle = angle + bay.facing;
			bay.ship->zoom = zoom;
		}
	return hasVisible;
}



CargoHold &Ship::Cargo()
{
	return cargo;
}



const CargoHold &Ship::Cargo() const
{
	return cargo;
}



// Display box effects from jettisoning this much cargo.
void Ship::Jettison(const string &commodity, int tons, bool wasAppeasing)
{
	cargo.Remove(commodity, tons);
	
	// Jettisoned cargo must carry some of the ship's heat with it. Otherwise
	// jettisoning cargo would increase the ship's temperature.
	heat -= tons * MAXIMUM_TEMPERATURE * Heat();
	
	const Government *notForGov = wasAppeasing ? GetGovernment() : nullptr;
	
	for( ; tons > 0; tons -= Flotsam::TONS_PER_BOX)
		jettisoned.emplace_back(new Flotsam(commodity, (Flotsam::TONS_PER_BOX < tons) ? Flotsam::TONS_PER_BOX : tons, notForGov));
}



void Ship::Jettison(const Outfit *outfit, int count, bool wasAppeasing)
{
	if(count < 0)
		return;

	cargo.Remove(outfit, count);
	
	// Jettisoned cargo must carry some of the ship's heat with it. Otherwise
	// jettisoning cargo would increase the ship's temperature.
	double mass = outfit->Mass();
	heat -= count * mass * MAXIMUM_TEMPERATURE * Heat();
	
	const Government *notForGov = wasAppeasing ? GetGovernment() : nullptr;
	
	const int perBox = (mass <= 0.) ? count : (mass > Flotsam::TONS_PER_BOX) ? 1 : static_cast<int>(Flotsam::TONS_PER_BOX / mass);
	while(count > 0)
	{
		jettisoned.emplace_back(new Flotsam(outfit, (perBox < count) ? perBox : count, notForGov));
		count -= perBox;
	}
}



const Outfit &Ship::Attributes() const
{
	return attributes;
}



const Outfit &Ship::BaseAttributes() const
{
	return baseAttributes;
}



// Get outfit information.
const map<const Outfit *, int> &Ship::Outfits() const
{
	return outfits;
}



int Ship::OutfitCount(const Outfit *outfit) const
{
	auto it = outfits.find(outfit);
	return (it == outfits.end()) ? 0 : it->second;
}



// Add or remove outfits. (To remove, pass a negative number.)
void Ship::AddOutfit(const Outfit *outfit, int count)
{
	if(outfit && count)
	{
		auto it = outfits.find(outfit);
		if(it == outfits.end())
			outfits[outfit] = count;
		else
		{
			it->second += count;
			if(!it->second)
				outfits.erase(it);
		}
		attributes.Add(*outfit, count);
		if(outfit->IsWeapon())
			armament.Add(outfit, count);
		
		if(outfit->Get("cargo space"))
			cargo.SetSize(attributes.Get("cargo space"));
		if(outfit->Get("hull"))
			hull += outfit->Get("hull") * count;
		// If the added or removed outfit is a jump drive, recalculate
		// and cache this ship's jump range.
		if(outfit->Get("jump drive"))
			jumpRange = JumpRange(false);
	}
}



// Get the list of weapons.
Armament &Ship::GetArmament()
{
	return armament;
}



const vector<Hardpoint> &Ship::Weapons() const
{
	return armament.Get();
}



// Check if we are able to fire the given weapon (i.e. there is enough
// energy, ammo, and fuel to fire it).
bool Ship::CanFire(const Weapon *weapon) const
{
	if(!weapon || !weapon->IsWeapon())
		return false;
	
	if(weapon->Ammo())
	{
		auto it = outfits.find(weapon->Ammo());
		if(it == outfits.end() || it->second < weapon->AmmoUsage())
			return false;
	}
	
	if(energy < weapon->FiringEnergy() + weapon->RelativeFiringEnergy() * attributes.Get("energy capacity"))
		return false;
	if(fuel < weapon->FiringFuel() + weapon->RelativeFiringFuel() * attributes.Get("fuel capacity"))
		return false;
	// We do check hull, but we don't check shields. Ships can survive with all shields depleted.
	// Ships should not disable themselves, so we check if we stay above minimumHull.
	if(hull - MinimumHull() < weapon->FiringHull() + weapon->RelativeFiringHull() * attributes.Get("hull"))
		return false;

	// If a weapon requires heat to fire, (rather than generating heat), we must
	// have enough heat to spare.
	if(heat < -(weapon->FiringHeat() + (!weapon->RelativeFiringHeat()
			? 0. : weapon->RelativeFiringHeat() * MaximumHeat())))
		return false;
	// Repeat this for various effects which shouldn't drop below 0.
	if(ionization < -weapon->FiringIon())
		return false;
	if(disruption < -weapon->FiringDisruption())
		return false;
	if(slowness < -weapon->FiringSlowing())
		return false;
	
	return true;
}



// Fire the given weapon (i.e. deduct whatever energy, ammo, hull, shields
// or fuel it uses and add whatever heat it generates. Assume that CanFire()
// is true.
void Ship::ExpendAmmo(const Weapon &weapon)
{
	// Compute this ship's initial capacities, in case the consumption of the ammunition outfit(s)
	// modifies them, so that relative costs are calculated based on the pre-firing state of the ship.
	const double relativeEnergyChange = weapon.RelativeFiringEnergy() * attributes.Get("energy capacity");
	const double relativeFuelChange = weapon.RelativeFiringFuel() * attributes.Get("fuel capacity");
	const double relativeHeatChange = !weapon.RelativeFiringHeat() ? 0. : weapon.RelativeFiringHeat() * MaximumHeat();
	const double relativeHullChange = weapon.RelativeFiringHull() * attributes.Get("hull");
	const double relativeShieldChange = weapon.RelativeFiringShields() * attributes.Get("shields");
	
	if(const Outfit *ammo = weapon.Ammo())
	{
		// Some amount of the ammunition mass to be removed from the ship carries thermal energy.
		// A realistic fraction applicable to all cases cannot be computed, so assume 50%.
		heat -= weapon.AmmoUsage() * .5 * ammo->Mass() * MAXIMUM_TEMPERATURE * Heat();
		AddOutfit(ammo, -weapon.AmmoUsage());
	}
	
	energy -= weapon.FiringEnergy() + relativeEnergyChange;
	fuel -= weapon.FiringFuel() + relativeFuelChange;
	heat += weapon.FiringHeat() + relativeHeatChange;
	shields -= weapon.FiringShields() + relativeShieldChange;
	
	// Since weapons fire from within the shields, hull and "status" damages are dealt in full.
	hull -= weapon.FiringHull() + relativeHullChange;
	ionization += weapon.FiringIon();
	disruption += weapon.FiringDisruption();
	slowness += weapon.FiringSlowing();
}



// Each ship can have a target system (to travel to), a target planet (to
// land on) and a target ship (to move to, and attack if hostile).
shared_ptr<Ship> Ship::GetTargetShip() const
{
	return targetShip.lock();
}



shared_ptr<Ship> Ship::GetShipToAssist() const
{
	return shipToAssist.lock();
}



const StellarObject *Ship::GetTargetStellar() const
{
	return targetPlanet;
}



const System *Ship::GetTargetSystem() const
{
	return (targetSystem == currentSystem) ? nullptr : targetSystem;
}



// Mining target.
shared_ptr<Minable> Ship::GetTargetAsteroid() const
{
	return targetAsteroid.lock();
}



shared_ptr<Flotsam> Ship::GetTargetFlotsam() const
{
	return targetFlotsam.lock();
}



// Set this ship's targets.
void Ship::SetTargetShip(const shared_ptr<Ship> &ship)
{
	if(ship != GetTargetShip())
	{
		targetShip = ship;
		// When you change targets, clear your scanning records.
		cargoScan = 0.;
		outfitScan = 0.;
	}
	targetAsteroid.reset();
}



void Ship::SetShipToAssist(const shared_ptr<Ship> &ship)
{
	shipToAssist = ship;
}



void Ship::SetTargetStellar(const StellarObject *object)
{
	targetPlanet = object;
}



void Ship::SetTargetSystem(const System *system)
{
	targetSystem = system;
}



// Mining target.
void Ship::SetTargetAsteroid(const shared_ptr<Minable> &asteroid)
{
	targetAsteroid = asteroid;
	targetShip.reset();
}



void Ship::SetTargetFlotsam(const shared_ptr<Flotsam> &flotsam)
{
	targetFlotsam = flotsam;
}



void Ship::SetParent(const shared_ptr<Ship> &ship)
{
	shared_ptr<Ship> oldParent = parent.lock();
	if(oldParent)
		oldParent->RemoveEscort(*this);
	
	parent = ship;
	if(ship)
		ship->AddEscort(*this);
}



shared_ptr<Ship> Ship::GetParent() const
{
	return parent.lock();
}



const vector<weak_ptr<Ship>> &Ship::GetEscorts() const
{
	return escorts;
}



// Add escorts to this ship. Escorts look to the parent ship for movement
// cues and try to stay with it when it lands or goes into hyperspace.
void Ship::AddEscort(Ship &ship)
{
	escorts.push_back(ship.shared_from_this());
}



void Ship::RemoveEscort(const Ship &ship)
{
	auto it = escorts.begin();
	for( ; it != escorts.end(); ++it)
		if(it->lock().get() == &ship)
		{
			escorts.erase(it);
			return;
		}
}



double Ship::MinimumHull() const
{
	if(neverDisabled)
		return 0.;
	
	double maximumHull = attributes.Get("hull");
	double absoluteThreshold = attributes.Get("absolute threshold");
	if(absoluteThreshold > 0.)
		return absoluteThreshold;
	
	double thresholdPercent = attributes.Get("threshold percentage");
	double transition = 1 / (1 + 0.0005 * maximumHull);
	double minimumHull = maximumHull * (thresholdPercent > 0. ? min(thresholdPercent, 1.) : 0.1 * (1. - transition) + 0.5 * transition);

	return max(0., floor(minimumHull + attributes.Get("hull threshold")));
}



// Find out how much fuel is consumed by the hyperdrive of the given type.
double Ship::BestFuel(const string &type, const string &subtype, double defaultFuel, double jumpDistance) const
{
	// Find the outfit that provides the least costly hyperjump.
	double best = 0.;
	// Make it possible for a hyperdrive to be integrated into a ship.
	if(baseAttributes.Get(type) && (subtype.empty() || baseAttributes.Get(subtype)))
	{
		// If a distance was given, then we know that we are making a jump.
		// Only use the fuel from a jump drive if it is capable of making
		// the given jump. We can guarantee that at least one jump drive
		// is capable of making the given jump, as the destination must
		// be among the neighbors of the current system.
		double jumpRange = baseAttributes.Get("jump range");
		if(!jumpRange)
			jumpRange = System::DEFAULT_NEIGHBOR_DISTANCE;
		// If no distance was given then we're either using a hyperdrive
		// or refueling this ship, in which case this if statement will
		// always pass.
		if(jumpRange >= jumpDistance)
		{
			best = baseAttributes.Get("jump fuel");
			if(!best)
				best = defaultFuel;
		}
	}
	// Search through all the outfits.
	for(const auto &it : outfits)
		if(it.first->Get(type) && (subtype.empty() || it.first->Get(subtype)))
		{
			double jumpRange = it.first->Get("jump range");
			if(!jumpRange)
				jumpRange = System::DEFAULT_NEIGHBOR_DISTANCE;
			if(jumpRange >= jumpDistance)
			{
				double fuel = it.first->Get("jump fuel");
				if(!fuel)
					fuel = defaultFuel;
				if(!best || fuel < best)
					best = fuel;
			}
		}
	return best;
}



void Ship::CreateExplosion(vector<Visual> &visuals, bool spread)
{
	if(!HasSprite() || !GetMask().IsLoaded() || explosionEffects.empty())
		return;
	
	// Bail out if this loops enough times, just in case.
	for(int i = 0; i < 10; ++i)
	{
		Point point((Random::Real() - .5) * Width(),
			(Random::Real() - .5) * Height());
		if(GetMask().Contains(point, Angle()))
		{
			// Pick an explosion.
			int type = Random::Int(explosionTotal);
			auto it = explosionEffects.begin();
			for( ; it != explosionEffects.end(); ++it)
			{
				type -= it->second;
				if(type < 0)
					break;
			}
			Point effectVelocity = velocity;
			if(spread)
			{
				double scale = .04 * (Width() + Height());
				effectVelocity += Angle::Random().Unit() * (scale * Random::Real());
			}
			visuals.emplace_back(*it->first, angle.Rotate(point) + position, std::move(effectVelocity), angle);
			++explosionCount;
			return;
		}
	}
}



// Place a "spark" effect, like ionization or disruption.
void Ship::CreateSparks(vector<Visual> &visuals, const string &name, double amount)
{
	CreateSparks(visuals, GameData::Effects().Get(name), amount);
}



void Ship::CreateSparks(vector<Visual> &visuals, const Effect *effect, double amount)
{
	if(forget)
		return;
	
	// Limit the number of sparks, depending on the size of the sprite.
	amount = min(amount, Width() * Height() * .0006);
	// Preallocate capacity, in case we're adding a non-trivial number of sparks.
	visuals.reserve(visuals.size() + static_cast<int>(amount));
	
	while(true)
	{
		amount -= Random::Real();
		if(amount <= 0.)
			break;
		
		Point point((Random::Real() - .5) * Width(),
			(Random::Real() - .5) * Height());
		if(GetMask().Contains(point, Angle()))
			visuals.emplace_back(*effect, angle.Rotate(point) + position, velocity, angle);
	}
}
