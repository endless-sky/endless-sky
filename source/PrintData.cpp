/* PrintData.cpp
Copyright (c) 2014 by Michael Zahniser, 2022 by warp-core

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "PrintData.h"

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "GameEvent.h"
#include "LocationFilter.h"
#include "Outfit.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Port.h"
#include "Ship.h"
#include "Shop.h"
#include "System.h"
#include "Weapon.h"

#include <iostream>
#include <map>
#include <set>

using namespace std;

namespace {
	// For getting the name of a ship model or outfit.
	// The relevant method for each class has a different signature,
	// so use template specialisation to select the appropriate version of the method.
	template<class Type>
	string ObjectName(const Type &object) = delete;

	template<>
	string ObjectName(const Ship &object) { return object.TrueModelName(); }

	template<>
	string ObjectName(const Outfit &object) { return object.TrueName(); }


	// Take a set of items and a set of sales and print a list of each item followed by the sales it appears in.
	template<class Type>
	void PrintItemSales(const Set<Type> &items, const Set<Shop<Type>> &sales,
		const string &itemNoun, const string &saleNoun)
	{
		cout << DataWriter::Quote(itemNoun) << ',' << DataWriter::Quote(saleNoun) << '\n';
		map<string, set<string>> itemSales;
		for(auto &saleIt : sales)
			for(auto &itemIt : saleIt.second.Stock())
				itemSales[ObjectName(*itemIt)].insert(saleIt.first);
		for(auto &itemIt : items)
		{
			if(itemIt.first != ObjectName(itemIt.second))
				continue;
			cout << DataWriter::Quote(itemIt.first);
			for(auto &saleName : itemSales[itemIt.first])
				cout << ',' << DataWriter::Quote(saleName);
			cout << '\n';
		}
	}

	// Take a set of sales and print a list of each followed by the items it contains.
	// Will fail to compile for items not of type Ship or Outfit.
	template<class Type>
	void PrintSales(const Set<Shop<Type>> &sales, const string &saleNoun, const string &itemNoun)
	{
		cout << DataWriter::Quote(saleNoun) << ';' << DataWriter::Quote(itemNoun) << '\n';
		for(auto &saleIt : sales)
		{
			cout << DataWriter::Quote(saleIt.first);
			int index = 0;
			for(auto &item : saleIt.second.Stock())
				cout << (index++ ? ';' : ',') << DataWriter::Quote(ObjectName(*item));
			cout << '\n';
		}
	}


	// Take a Set and print a list of the names (keys) it contains.
	template<class Type>
	void PrintObjectList(const Set<Type> &objects, const string &name)
	{
		cout << DataWriter::Quote(name) << '\n';
		for(const auto &it : objects)
			cout << DataWriter::Quote(it.first) << endl;
	}

	// Takes a Set of objects and prints the key for each, followed by a list of its attributes.
	// The class 'Type' must have an accessible 'Attributes()' member method which returns a collection of strings.
	template<class Type>
	void PrintObjectAttributes(const Set<Type> &objects, const string &name)
	{
		cout << DataWriter::Quote(name) << ',' << DataWriter::Quote("attributes") << '\n';
		for(auto &it : objects)
		{
			cout << DataWriter::Quote(it.first);
			const Type &object = it.second;
			int index = 0;
			for(const string &attribute : object.Attributes())
				cout << (index++ ? ';' : ',') << DataWriter::Quote(attribute);
			cout << '\n';
		}
	}

	// Takes a Set of objects, which must have an accessible member `Attributes()`, returning a collection of strings.
	// Prints a list of all those string attributes and, for each, the list of keys of objects with that attribute.
	template<class Type>
	void PrintObjectsByAttribute(const Set<Type> &objects, const string &name)
	{
		cout << DataWriter::Quote("attribute") << ',' << DataWriter::Quote(name) << '\n';
		set<string> attributes;
		for(auto &it : objects)
		{
			const Type &object = it.second;
			for(const string &attribute : object.Attributes())
				attributes.insert(attribute);
		}
		for(const string &attribute : attributes)
		{
			cout << DataWriter::Quote(attribute);
			int index = 0;
			for(auto &it : objects)
			{
				const Type &object = it.second;
				if(object.Attributes().contains(attribute))
					cout << (index++ ? ';' : ',') << DataWriter::Quote(it.first);
			}
			cout << '\n';
		}
	}


	void Ships(const char *const *argv)
	{
		auto PrintBaseShipStats = []() -> void
		{
			cout << "model" << ',' << "category" << ',' << DataWriter::Quote("chassis cost") << ','
				<< DataWriter::Quote("loaded cost") << ',' << "shields" << ',' << "hull" << ',' << "mass" << ',' << "drag" << ','
				<< DataWriter::Quote("heat dissipation") << ',' << DataWriter::Quote("required crew") << ',' << "bunks" << ','
				<< DataWriter::Quote("cargo space") << ',' << "fuel" << ',' << DataWriter::Quote("outfit space") << ','
				<< DataWriter::Quote("weapon capacity") << ',' << DataWriter::Quote("engine capacity") << ','
				<< DataWriter::Quote("gun mounts") << ',' << DataWriter::Quote("turret mounts") << ','
				<< DataWriter::Quote("fighter bays") << ',' << DataWriter::Quote("drone bays") << '\n';

			for(auto &it : GameData::Ships())
			{
				// Skip variants and unnamed / partially-defined ships.
				if(it.second.TrueModelName() != it.first)
					continue;

				const Ship &ship = it.second;
				cout << DataWriter::Quote(it.first) << ',';

				const Outfit &attributes = ship.BaseAttributes();
				cout << DataWriter::Quote(attributes.Category()) << ',';
				cout << ship.ChassisCost() << ',';
				cout << ship.Cost() << ',';

				auto mass = attributes.Mass() ? attributes.Mass() : 1.;
				cout << ship.MaxShields() << ',';
				cout << ship.MaxHull() << ',';
				cout << mass << ',';
				cout << attributes.Get("drag") << ',';
				cout << ship.HeatDissipation() * 1000. << ',';
				cout << attributes.Get("required crew") << ',';
				cout << attributes.Get("bunks") << ',';
				cout << attributes.Get("cargo space") << ',';
				cout << attributes.Get("fuel capacity") << ',';

				cout << attributes.Get("outfit space") << ',';
				cout << attributes.Get("weapon capacity") << ',';
				cout << attributes.Get("engine capacity") << ',';

				int numTurrets = 0;
				int numGuns = 0;
				for(auto &hardpoint : ship.Weapons())
				{
					if(hardpoint.IsTurret())
						++numTurrets;
					else
						++numGuns;
				}
				cout << numGuns << ',' << numTurrets << ',';

				int numFighters = ship.BaysTotal("Fighter");
				int numDrones = ship.BaysTotal("Drone");
				cout << numFighters << ',' << numDrones << '\n';
			}
		};

		auto PrintLoadedShipStats = [](bool variants) -> void
		{
			cout << "model" << ',' << "category" << ',' << "cost" << ',' << "shields" << ',' << "hull" << ',' << "mass" << ','
				<< DataWriter::Quote("required crew") << ',' << "bunks" << ',' << DataWriter::Quote("cargo space") << ','
				<< "fuel" << ',' << DataWriter::Quote("outfit space") << ',' << DataWriter::Quote("weapon capacity") << ','
				<< DataWriter::Quote("engine capacity") << ',' << "speed" << ',' << "accel" << ',' << "turn" << ','
				<< DataWriter::Quote("energy generation") << ',' << DataWriter::Quote("max energy usage") << ','
				<< DataWriter::Quote("energy capacity") << ',' << DataWriter::Quote("idle/max heat") << ','
				<< DataWriter::Quote("max heat generation") << ',' << DataWriter::Quote("max heat dissipation") << ','
				<< DataWriter::Quote("gun mounts") << ',' << DataWriter::Quote("turret mounts") << ','
				<< DataWriter::Quote("fighter bays") << ',' << DataWriter::Quote("drone bays") << ',' << "deterrence" << '\n';

			for(auto &it : GameData::Ships())
			{
				// Skip variants and unnamed / partially-defined ships, unless specified otherwise.
				if(it.second.TrueModelName() != it.first && !variants)
					continue;

				const Ship &ship = it.second;
				cout << DataWriter::Quote(it.first) << ',';

				const Outfit &attributes = ship.Attributes();
				cout << DataWriter::Quote(attributes.Category()) << ',';
				cout << ship.Cost() << ',';

				auto mass = attributes.Mass() ? attributes.Mass() : 1.;
				cout << ship.MaxShields() << ',';
				cout << ship.MaxHull() << ',';
				cout << mass << ',';
				cout << attributes.Get("required crew") << ',';
				cout << attributes.Get("bunks") << ',';
				cout << attributes.Get("cargo space") << ',';
				cout << attributes.Get("fuel capacity") << ',';

				cout << ship.BaseAttributes().Get("outfit space") << ',';
				cout << ship.BaseAttributes().Get("weapon capacity") << ',';
				cout << ship.BaseAttributes().Get("engine capacity") << ',';
				cout << (attributes.Get("drag") ? (60. * attributes.Get("thrust") / attributes.Get("drag")) : 0) << ',';
				cout << 3600. * attributes.Get("thrust") / mass << ',';
				cout << 60. * attributes.Get("turn") / mass << ',';

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
					+ (attributes.Get("hull heat") * (1. + attributes.Get("hull heat multiplier")))
					+ (attributes.Get("shield heat") * (1. + attributes.Get("shield heat multiplier")))
					+ attributes.Get("solar heat")
					+ attributes.Get("cloaking heat");

				for(const auto &oit : ship.Outfits())
				{
					const Weapon *weapon = oit.first->GetWeapon().get();
					if(weapon && weapon->Reload())
					{
						double reload = weapon->Reload();
						energyConsumed += oit.second * weapon->FiringEnergy() / reload;
						heatProduced += oit.second * weapon->FiringHeat() / reload;
					}
				}
				cout << 60. * (attributes.Get("energy generation") + attributes.Get("solar collection")) << ',';
				cout << 60. * energyConsumed << ',';
				cout << attributes.Get("energy capacity") << ',';
				cout << ship.IdleHeat() / max(1., ship.MaximumHeat()) << ',';
				cout << 60. * heatProduced << ',';
				// Maximum heat is 100 degrees per ton. Bleed off rate is 1/1000 per 60th of a second, so:
				cout << 60. * ship.HeatDissipation() * ship.MaximumHeat() << ',';

				int numTurrets = 0;
				int numGuns = 0;
				for(auto &hardpoint : ship.Weapons())
				{
					if(hardpoint.IsTurret())
						++numTurrets;
					else
						++numGuns;
				}
				cout << numGuns << ',' << numTurrets << ',';

				int numFighters = ship.BaysTotal("Fighter");
				int numDrones = ship.BaysTotal("Drone");
				cout << numFighters << ',' << numDrones << ',';

				double deterrence = 0.;
				for(const Hardpoint &hardpoint : ship.Weapons())
				{
					const Weapon *weapon = hardpoint.GetWeapon();
					if(weapon)
					{
						if(weapon->Ammo() && !ship.OutfitCount(weapon->Ammo()))
							continue;
						double damage = weapon->ShieldDamage() + weapon->HullDamage()
							+ (weapon->RelativeShieldDamage() * ship.MaxShields())
							+ (weapon->RelativeHullDamage() * ship.MaxHull());
						deterrence += .12 * damage / weapon->Reload();
					}
				}
				cout << deterrence << '\n';
			}
		};

		auto PrintShipList = [](bool variants) -> void
		{
			for(auto &it : GameData::Ships())
			{
				// Skip variants and unnamed / partially-defined ships, unless specified otherwise.
				if(it.second.TrueModelName() != it.first && !variants)
					continue;

				cout << DataWriter::Quote(it.first) << "\n";
			}
		};

		bool loaded = false;
		bool variants = false;
		bool sales = false;
		bool list = false;

		for(const char *const *it = argv + 2; *it; ++it)
		{
			string arg = *it;
			if(arg == "--variants")
				variants = true;
			else if(arg == "--sales")
				sales = true;
			else if(arg == "--loaded")
				loaded = true;
			else if(arg == "--list")
				list = true;
		}

		if(sales)
			PrintItemSales(GameData::Ships(), GameData::Shipyards(), "ship", "shipyards");
		else if(loaded)
			PrintLoadedShipStats(variants);
		else if(list)
			PrintShipList(variants);
		else
			PrintBaseShipStats();
	}

	void Outfits(const char *const *argv)
	{
		auto PrintWeaponStats = []() -> void
		{
			cout << "name" << ',' << "category" << ',' << "cost" << ',' << "space" << ',' << "range" << ',' << "reload" << ','
				<< DataWriter::Quote("burst count") << ',' << DataWriter::Quote("burst reload") << ',' << "lifetime" << ','
				<< "shots/second" << ',' << "energy/shot" << ',' << "heat/shot" << ',' << "recoil/shot" << ',' << "energy/s" << ','
				<< "heat/s" << ',' << "recoil/s" << ',' << "shield/s" << ',' << "discharge/s" << ',' << "hull/s" << ','
				<< "corrosion/s" << ',' << "heat dmg/s" << ',' << DataWriter::Quote("burn dmg/s") << ','
				<< DataWriter::Quote("energy dmg/s") << ',' << DataWriter::Quote("ion dmg/s") << ','
				<< DataWriter::Quote("scrambling dmg/s") << ',' << DataWriter::Quote("slow dmg/s") << ','
				<< DataWriter::Quote("disruption dmg/s") << ',' << "piercing" << ',' << DataWriter::Quote("fuel dmg/s") << ','
				<< DataWriter::Quote("leak dmg/s") << ',' << "push/s" << ',' << ',' << "strength" << ','
				<< "deterrence" << '\n';

			for(auto &it : GameData::Outfits())
			{
				// Skip non-weapons and submunitions.
				const Outfit &outfit = it.second;
				const Weapon *weapon = outfit.GetWeapon().get();
				if(!weapon || outfit.Category().empty())
					continue;

				cout << DataWriter::Quote(it.first)<< ',';
				cout << DataWriter::Quote(outfit.Category()) << ',';
				cout << outfit.Cost() << ',';
				cout << -outfit.Get("weapon capacity") << ',';

				cout << weapon->Range() << ',';

				double reload = weapon->Reload();
				cout << reload << ',';
				cout << weapon->BurstCount() << ',';
				cout << weapon->BurstReload() << ',';
				cout << weapon->TotalLifetime() << ',';
				double fireRate = 60. / reload;
				cout << fireRate << ',';

				double firingEnergy = weapon->FiringEnergy();
				cout << firingEnergy << ',';
				firingEnergy *= fireRate;
				double firingHeat = weapon->FiringHeat();
				cout << firingHeat << ',';
				firingHeat *= fireRate;
				double firingForce = weapon->FiringForce();
				cout << firingForce << ',';
				firingForce *= fireRate;

				cout << firingEnergy << ',';
				cout << firingHeat << ',';
				cout << firingForce << ',';

				double shieldDmg = weapon->ShieldDamage() * fireRate;
				cout << shieldDmg << ',';
				double dischargeDmg = weapon->DischargeDamage() * 100. * fireRate;
				cout << dischargeDmg << ',';
				double hullDmg = weapon->HullDamage() * fireRate;
				cout << hullDmg << ',';
				double corrosionDmg = weapon->CorrosionDamage() * 100. * fireRate;
				cout << corrosionDmg << ',';
				double heatDmg = weapon->HeatDamage() * fireRate;
				cout << heatDmg << ',';
				double burnDmg = weapon->BurnDamage() * 100. * fireRate;
				cout << burnDmg << ',';
				double energyDmg = weapon->EnergyDamage() * fireRate;
				cout << energyDmg << ',';
				double ionDmg = weapon->IonDamage() * 100. * fireRate;
				cout << ionDmg << ',';
				double scramblingDmg = weapon->ScramblingDamage() * 100. * fireRate;
				cout << scramblingDmg << ',';
				double slowDmg = weapon->SlowingDamage() * fireRate;
				cout << slowDmg << ',';
				double disruptDmg = weapon->DisruptionDamage() * fireRate;
				cout << disruptDmg << ',';
				cout << weapon->Piercing() << ',';
				double fuelDmg = weapon->FuelDamage() * fireRate;
				cout << fuelDmg << ',';
				double leakDmg = weapon->LeakDamage() * 100. * fireRate;
				cout << leakDmg << ',';
				double hitforce = weapon->HitForce() * fireRate;
				cout << hitforce << ',';

				double strength = weapon->MissileStrength() + weapon->AntiMissile();
				cout << strength << ',';

				double damage = weapon->ShieldDamage() + weapon->HullDamage();
				double deterrence = .12 * damage / weapon->Reload();
				cout << deterrence << '\n';
			}

			cout.flush();
		};

		auto PrintEngineStats = []() -> void
		{
			cout << "name" << ',' << "cost" << ',' << "mass" << ',' << DataWriter::Quote("outfit space") << ','
				<< DataWriter::Quote("engine capacity") << ',' << "thrust/s" << ',' << DataWriter::Quote("thrust energy/s") << ','
				<< DataWriter::Quote("thrust heat/s") << ',' << "turn/s" << ',' << DataWriter::Quote("turn energy/s") << ','
				<< DataWriter::Quote("turn heat/s") << ',' << DataWriter::Quote("reverse thrust/s") << ','
				<< DataWriter::Quote("reverse energy/s") << ',' << DataWriter::Quote("reverse heat/s") << ','
				<< DataWriter::Quote("afterburner thrust/s") << ',' << DataWriter::Quote("afterburner energy/s") << ','
				<< DataWriter::Quote("afterburner heat/s") << ',' << DataWriter::Quote("afterburner fuel/s") << '\n';

			for(auto &it : GameData::Outfits())
			{
				// Skip non-engines.
				if(it.second.Category() != "Engines")
					continue;

				const Outfit &outfit = it.second;
				cout << DataWriter::Quote(it.first) << ',';
				cout << outfit.Cost() << ',';
				cout << outfit.Mass() << ',';
				cout << outfit.Get("outfit space") << ',';
				cout << outfit.Get("engine capacity") << ',';
				cout << outfit.Get("thrust") * 3600. << ',';
				cout << outfit.Get("thrusting energy") * 60. << ',';
				cout << outfit.Get("thrusting heat") * 60. << ',';
				cout << outfit.Get("turn") * 60. << ',';
				cout << outfit.Get("turning energy") * 60. << ',';
				cout << outfit.Get("turning heat") * 60. << ',';
				cout << outfit.Get("reverse thrust") * 3600. << ',';
				cout << outfit.Get("reverse thrusting energy") * 60. << ',';
				cout << outfit.Get("reverse thrusting heat") * 60. << ',';
				cout << outfit.Get("afterburner thrust") * 3600. << ',';
				cout << outfit.Get("afterburner energy") * 60. << ',';
				cout << outfit.Get("afterburner heat") * 60. << ',';
				cout << outfit.Get("afterburner fuel") * 60. << '\n';
			}

			cout.flush();
		};

		auto PrintPowerStats = []() -> void
		{
			cout << "name" << ',' << "cost" << ',' << "mass" << ',' << DataWriter::Quote("outfit space") << ','
				<< DataWriter::Quote("energy generation") << ',' << DataWriter::Quote("heat generation") << ','
				<< DataWriter::Quote("energy capacity") << '\n';

			for(auto &it : GameData::Outfits())
			{
				// Skip non-power.
				if(it.second.Category() != "Power")
					continue;

				const Outfit &outfit = it.second;
				cout << DataWriter::Quote(it.first) << ',';
				cout << outfit.Cost() << ',';
				cout << outfit.Mass() << ',';
				cout << outfit.Get("outfit space") << ',';
				cout << outfit.Get("energy generation") << ',';
				cout << outfit.Get("heat generation") << ',';
				cout << outfit.Get("energy capacity") << '\n';
			}

			cout.flush();
		};

		auto PrintOutfitsAllStats = []() -> void
		{
			set<string> attributes;
			for(auto &it : GameData::Outfits())
			{
				const Outfit &outfit = it.second;
				for(const auto &attribute : outfit.Attributes())
					attributes.insert(attribute.first);
			}

			cout << "name" << ',' << "category" << ',' << "cost" << ',' << "mass";
			for(const auto &attribute : attributes)
				cout << ',' << DataWriter::Quote(attribute);
			cout << '\n';

			for(auto &it : GameData::Outfits())
			{
				const Outfit &outfit = it.second;
				cout << DataWriter::Quote(outfit.TrueName()) << ',';
				cout << DataWriter::Quote(outfit.Category()) << ',';
				cout << outfit.Cost() << ',';
				cout << outfit.Mass();
				for(const auto &attribute : attributes)
					cout << ',' << outfit.Attributes().Get(attribute);
				cout << '\n';
			}
		};

		bool weapons = false;
		bool engines = false;
		bool power = false;
		bool sales = false;
		bool all = false;

		for(const char *const *it = argv + 1; *it; ++it)
		{
			string arg = *it;
			if(arg == "-w" || arg == "--weapons")
				weapons = true;
			else if(arg == "-e" || arg == "--engines")
				engines = true;
			else if(arg == "--power")
				power = true;
			else if(arg == "-s" || arg == "--sales")
				sales = true;
			else if(arg == "-a" || arg == "--all")
				all = true;
		}

		if(weapons)
			PrintWeaponStats();
		else if(engines)
			PrintEngineStats();
		else if(power)
			PrintPowerStats();
		else if(sales)
			PrintItemSales(GameData::Outfits(), GameData::Outfitters(), "outfit", "outfitters");
		else if(all)
			PrintOutfitsAllStats();
		else
			PrintObjectList(GameData::Outfits(), "outfit");
	}

	void Sales(const char *const *argv)
	{
		bool ships = false;
		bool outfits = false;

		for(const char *const *it = argv + 2; *it; ++it)
		{
			string arg = *it;
			if(arg == "-s" || arg == "--ships")
				ships = true;
			else if(arg == "-o" || arg == "--outfits")
				outfits = true;
		}

		if(!(ships || outfits))
		{
			ships = true;
			outfits = true;
		}
		if(ships)
			PrintSales(GameData::Shipyards(), "shipyards", "ships");
		if(outfits)
			PrintSales(GameData::Outfitters(), "outfitters", "outfits");
	}


	void Planets(const char *const *argv)
	{
		auto PrintPlanetDescriptions = []() -> void
		{
			cout << "planet::description::spaceport\n";
			for(auto &it : GameData::Planets())
			{
				cout << it.first << "::";
				const Planet &planet = it.second;
				for(auto &whenText : planet.Description())
					cout << whenText.second;
				cout << "::";
				for(auto &whenText : planet.GetPort().Description())
					cout << whenText.second;
				cout << "\n";
			}
		};

		bool descriptions = false;
		bool attributes = false;
		bool byAttribute = false;

		for(const char *const *it = argv + 2; *it; ++it)
		{
			string arg = *it;
			if(arg == "--descriptions")
				descriptions = true;
			else if(arg == "--attributes")
				attributes = true;
			else if(arg == "--reverse")
				byAttribute = true;
		}
		if(descriptions)
			PrintPlanetDescriptions();
		if(attributes && byAttribute)
			PrintObjectsByAttribute(GameData::Planets(), "planets");
		else if(attributes)
			PrintObjectAttributes(GameData::Planets(), "planet");
		if(!(descriptions || attributes))
			PrintObjectList(GameData::Planets(), "planet");
	}

	void Systems(const char *const *argv)
	{
		bool attributes = false;
		bool byAttribute = false;

		for(const char *const *it = argv + 2; *it; ++it)
		{
			string arg = *it;
			if(arg == "--attributes")
				attributes = true;
			else if(arg == "--reverse")
				byAttribute = true;
		}
		if(attributes && byAttribute)
			PrintObjectsByAttribute(GameData::Systems(), "systems");
		else if(attributes)
			PrintObjectAttributes(GameData::Systems(), "system");
		else
			PrintObjectList(GameData::Systems(), "system");
	}

	void LocationFilterMatches(const char *const *argv, PlayerInfo &player)
	{
		StellarObject::UsingMatchesCommand();
		DataFile file(cin);
		LocationFilter filter;
		const set<const System *> *visitedSystems = &player.VisitedSystems();
		const set<const Planet *> *visitedPlanets = &player.VisitedPlanets();
		for(const DataNode &node : file)
		{
			const string &key = node.Token(0);
			if(key == "changes" || (key == "event" && node.Size() == 1))
				for(const DataNode &child : node)
					GameData::Change(child, player);
			else if(key == "event")
			{
				const auto *event = GameData::Events().Get(node.Token(1));
				for(const auto &change : event->Changes())
					GameData::Change(change, player);
			}
			else if(key == "location")
			{
				filter.Load(node, visitedSystems, visitedPlanets);
				break;
			}
		}

		cout << "Systems matching provided location filter:\n";
		for(const auto &it : GameData::Systems())
			if(filter.Matches(&it.second))
				cout << it.first << '\n';
		cout << "Planets matching provided location filter:\n";
		for(const auto &it : GameData::Planets())
			if(filter.Matches(&it.second))
				cout << it.first << '\n';
	}


	const set<string> OUTFIT_ARGS = {
		"-w",
		"--weapons",
		"-e",
		"--engines",
		"--power",
		"-o",
		"--outfits"
	};

	const set<string> OTHER_VALID_ARGS = {
		"-s",
		"--ships",
		"--sales",
		"--planets",
		"--systems",
		"--matches"
	};
}



bool PrintData::IsPrintDataArgument(const char *const *argv)
{
	for(const char *const *it = argv + 1; *it; ++it)
	{
		string arg = *it;
		if(OTHER_VALID_ARGS.contains(arg) || OUTFIT_ARGS.contains(arg))
			return true;
	}
	return false;
}



void PrintData::Print(const char *const *argv, PlayerInfo &player)
{
	for(const char *const *it = argv + 1; *it; ++it)
	{
		string arg = *it;
		if(arg == "-s" || arg == "--ships")
		{
			Ships(argv);
			break;
		}
		else if(OUTFIT_ARGS.contains(arg))
		{
			Outfits(argv);
			break;
		}
		else if(arg == "--sales")
		{
			Sales(argv);
			break;
		}
		else if(arg == "--planets")
			Planets(argv);
		else if(arg == "--systems")
			Systems(argv);
		else if(arg == "--matches")
			LocationFilterMatches(argv, player);
	}
	cout.flush();
}



void PrintData::Help()
{
	cerr << "    -s, --ships: prints a table of ship stats (just the base stats, not considering any stored outfits)."
			<< endl;
	cerr << "        --sales: prints a table of ships with every 'shipyard' each appears in." << endl;
	cerr << "        --loaded: prints a table of ship stats accounting for installed outfits. Does not include variants."
			<< endl;
	cerr << "        --list: prints a list of all ship names." << endl;
	cerr << "    Use the modifier `--variants` with the above two commands to include variants." << endl;
	cerr << "    -w, --weapons: prints a table of weapon stats." << endl;
	cerr << "    -e, --engines: prints a table of engine stats." << endl;
	cerr << "    --power: prints a table of power outfit stats." << endl;
	cerr << "    -o, --outfits: prints a list of outfits." << endl;
	cerr << "        --sales: prints a list of outfits and every 'outfitter' each appears in." << endl;
	cerr << "        -a, --all: prints a table of outfits and all attributes used by any outfits present." << endl;
	cerr << "    --sales: prints a list of all shipyards and outfitters, and the ships or outfits they each contain."
			<< endl;
	cerr << "        -s, --ships: prints a list of shipyards and the ships they each contain." << endl;
	cerr << "        -o, --outfits: prints a list of outfitters and the outfits they each contain." << endl;
	cerr << "    --planets: prints a list of all planets." << endl;
	cerr << "        --descriptions: prints a table of all planets and their descriptions." << endl;
	cerr << "        --attributes: prints a table of all planets and their attributes." << endl;
	cerr << "            --reverse: prints a table of all planet attributes and which planets have them."
			<< endl;
	cerr << "    --systems: prints a list of all systems." << endl;
	cerr << "        --attributes: prints a list of all systems and their attributes." << endl;
	cerr << "            --reverse: prints a list of all system attributes and which systems have them."
			<< endl;
	cerr << "    --matches: prints a list of all planets and systems matching a location filter passed in STDIN."
			<< endl;
	cerr << "        The first node of the location filter should be `location`." << endl;
}
