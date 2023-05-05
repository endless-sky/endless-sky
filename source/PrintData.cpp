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
#include "Ship.h"
#include "System.h"

#include <iostream>
#include <map>
#include <set>

using namespace std;

namespace {
	// For getting the name of a ship model or outfit.
	// The relevant method for each class has a different signature,
	// so use template specialisation to select the appropriate version of the method.
	template <class Type>
	string ObjectName(const Type &object) = delete;

	template <>
	string ObjectName(const Ship &object) { return object.ModelName(); }

	template <>
	string ObjectName(const Outfit &object) { return object.TrueName(); }


	// Take a set of items and a set of sales and print a list of each item followed by the sales it appears in.
	template <class Type>
	void PrintItemSales(const Set<Type> &items, const Set<Sale<Type>> &sales,
		const string &itemNoun, const string &saleNoun)
	{
		DataWriter writer;
		writer.SetSeparator(",").Write(itemNoun, saleNoun);
		map<string, set<string>> itemSales;
		for(auto &saleIt : sales)
			for(auto &itemIt : saleIt.second)
				itemSales[ObjectName(*itemIt)].insert(saleIt.first);

		for(auto &itemIt : items)
		{
			if(itemIt.first != ObjectName(itemIt.second))
				continue;
			writer.SetSeparator(",").WriteToken(itemIt.first);
			if(!itemSales[itemIt.first].empty())
			{
				// The first value is separated with a comma,
				// the rest use semicolons
				writer.WriteSeparator().SetSeparator(";");
				for(auto &saleName : itemSales[itemIt.first])
					writer.WriteToken(saleName);
			}
			writer.Write();
		}
		writer.SaveToStream(cout);
	}

	// Take a set of sales and print a list of each followed by the items it contains.
	// Will fail to compile for items not of type Ship or Outfit.
	template <class Type>
	void PrintSales(const Set<Sale<Type>> &sales, const string &saleNoun, const string &itemNoun)
	{
		DataWriter writer;
		writer.SetSeparator(",").Write(saleNoun, itemNoun);
		for(auto &saleIt : sales)
		{
			// The first value is separated with a comma,
			// the rest use semicolons
			writer.WriteToken(saleIt.first);
			writer.WriteSeparator().SetSeparator(";");
			for(auto &item : saleIt.second)
				writer.WriteToken(ObjectName(*item));
			writer.Write().SetSeparator(",");
		}
		writer.SaveToStream(cout);
	}


	// Take a Set and print a list of the names (keys) it contains.
	template <class Type>
	void PrintObjectList(const Set<Type> &objects, bool withQuotes, const string &name)
	{
		DataWriter writer;
		writer.Write(name);
		if(withQuotes)
			for(const auto &it : objects)
				writer.Write(it.first);
		else
			for(const auto &it : objects)
				writer.WriteRaw(it.first).Write();
		writer.SaveToStream(cout);
	}

	// Takes a Set of objects and prints the key for each, followed by a list of its attributes.
	// The class 'Type' must have an accessible 'Attributes()' member method which returns a collection of strings.
	template <class Type>
	void PrintObjectAttributes(const Set<Type> &objects, const string &name)
	{
		DataWriter writer;
		writer.SetSeparator(",").Write(name, "attributes");
		for(auto &it : objects)
		{
			// The first value is separated with a comma,
			// the rest use semicolons
			writer.WriteToken(it.first);
			writer.WriteSeparator().SetSeparator(";");
			for(const string &attribute : it.second.Attributes())
				writer.WriteToken(attribute);
			writer.Write().SetSeparator(",");
		}
		writer.SaveToStream(cout);
	}

	// Takes a Set of objects, which must have an accessible member `Attributes()`, returning a collection of strings.
	// Prints a list of all those string attributes and, for each, the list of keys of objects with that attribute.
	template <class Type>
	void PrintObjectsByAttribute(const Set<Type> &objects, const string &name)
	{
		DataWriter writer;
		writer.SetSeparator(",").Write("attribute", name);
		set<string> attributes;
		for(auto &it : objects)
		{
			const Type &object = it.second;
			for(const string &attribute : object.Attributes())
				attributes.insert(attribute);
		}
		for(const string &attribute : attributes)
		{
			writer.WriteToken(attribute);
			writer.WriteSeparator().SetSeparator(";");
			for(auto &it : objects)
				if(it.second.Attributes().count(attribute))
					writer.WriteToken(it.first);
			writer.Write().SetSeparator(",");
		}
		writer.SaveToStream(cout);
	}


	void Ships(const char *const *argv)
	{
		auto PrintBaseShipStats = []() -> void
		{
			string keys[] = {"shields", "hull",
					"drag", "required crew", "bunks", "cargo space", "fuel capacity",
					"outift space", "weapon space", "engine capacity"};

			DataWriter writer;
			writer.SetSeparator(",");
			writer.WriteToken("model", "category", "chassis cost", "loaded cost", "mass", "heat dissipation");
			for(auto &key : keys)
				writer.WriteToken(key);
			writer.Write("gun mounts", "turret mounts", "fighter bays", "drone bays");

			for(auto &it : GameData::Ships())
			{
				// Skip variants and unnamed / partially-defined ships.
				if(it.second.ModelName() != it.first)
					continue;

				// The first value is separated with a comma,
				// the rest use semicolons
				const Ship &ship = it.second;
				writer.WriteToken(it.first);
				writer.WriteSeparator().SetSeparator(";");

				const Outfit &attributes = ship.BaseAttributes();
				writer.WriteToken(attributes.Category(), ship.ChassisCost(), ship.Cost());
				writer.WriteToken(attributes.Mass() ? attributes.Mass() : 1.);
				writer.WriteToken(ship.HeatDissipation() * 1000.);

				for(auto &key : keys)
					writer.WriteToken(attributes.Get(key));

				int numTurrets = 0;
				int numGuns = 0;
				for(auto &hardpoint : ship.Weapons())
				{
					if(hardpoint.IsTurret())
						++numTurrets;
					else
						++numGuns;
				}
				writer.WriteToken(numGuns, numTurrets);

				int numFighters = ship.BaysTotal("Fighter");
				int numDrones = ship.BaysTotal("Drone");
				writer.WriteToken(numFighters, numDrones);
				writer.Write().SetSeparator(",");
			}
			writer.SaveToStream(cout);
		};

		auto PrintLoadedShipStats = [](bool variants) -> void
		{
			DataWriter writer;
			writer.SetSeparator(",");
			writer.WriteToken("model", "category", "cost", "shields", "hull", "mass", "required crew", "bunks");
			writer.WriteToken("cargo space", "fuel", "outfit space", "weapon capacity", "engine capacity");
			writer.WriteToken("speed", "accel", "turn", "energy generation", "max energy usage", "energy capacity");
			writer.WriteToken("idle/max heat", "max heat generation", "max heat dissipation");
			writer.WriteToken("gun mounts", "turret mounts", "fighter bays", "drone bays", "deterrence");
			writer.Write();

			for(auto &it : GameData::Ships())
			{
				// Skip variants and unnamed / partially-defined ships, unless specified otherwise.
				if(it.second.ModelName() != it.first && !variants)
					continue;

				// The first value is separated with a comma,
				// the rest use semicolons
				const Ship &ship = it.second;
				writer.WriteToken(it.first);
				writer.WriteSeparator().SetSeparator(";");

				const Outfit &attributes = ship.Attributes();
				const Outfit &baseAttributes = ship.BaseAttributes();
				writer.WriteToken(attributes.Category(), ship.Cost());

				auto mass = attributes.Mass() ? attributes.Mass() : 1.;
				writer.WriteToken(attributes.Get("shields"), attributes.Get("hull"));
				writer.WriteToken(mass);
				writer.WriteToken(attributes.Get("required crew"), attributes.Get("bunks"));
				writer.WriteToken(attributes.Get("cargo space"), attributes.Get("fuel capacity"));

				writer.WriteToken(baseAttributes.Get("outfit space"), baseAttributes.Get("weapon capacity"),
						baseAttributes.Get("outfit space"));
				writer.WriteToken(attributes.Get("drag") ? (60. * attributes.Get("thrust") / attributes.Get("drag")) : 0);
				writer.WriteToken(3600. * attributes.Get("thrust") / mass);
				writer.WriteToken(60. * attributes.Get("turn") / mass);

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
					if(oit.first->IsWeapon() && oit.first->Reload())
					{
						double reload = oit.first->Reload();
						energyConsumed += oit.second * oit.first->FiringEnergy() / reload;
						heatProduced += oit.second * oit.first->FiringHeat() / reload;
					}
				writer.WriteToken(60. * (attributes.Get("energy generation") + attributes.Get("solar collection")));
				writer.WriteToken(60. * energyConsumed);
				writer.WriteToken(attributes.Get("energy capacity"));
				writer.WriteToken(ship.IdleHeat() / max(1., ship.MaximumHeat()));
				writer.WriteToken(60. * heatProduced);
				// Maximum heat is 100 degrees per ton. Bleed off rate is 1/1000 per 60th of a second, so:
				writer.WriteToken(60. * ship.HeatDissipation() * ship.MaximumHeat());

				int numTurrets = 0;
				int numGuns = 0;
				for(auto &hardpoint : ship.Weapons())
				{
					if(hardpoint.IsTurret())
						++numTurrets;
					else
						++numGuns;
				}
				writer.WriteToken(numGuns, numTurrets);

				int numFighters = ship.BaysTotal("Fighter");
				int numDrones = ship.BaysTotal("Drone");
				writer.WriteToken(numFighters, numDrones);

				double deterrence = 0.;
				for(const Hardpoint &hardpoint : ship.Weapons())
					if(hardpoint.GetOutfit())
					{
						const Outfit *weapon = hardpoint.GetOutfit();
						if(weapon->Ammo() && !ship.OutfitCount(weapon->Ammo()))
							continue;
						double damage = weapon->ShieldDamage() + weapon->HullDamage()
							+ (weapon->RelativeShieldDamage() * ship.Attributes().Get("shields"))
							+ (weapon->RelativeHullDamage() * ship.Attributes().Get("hull"));
						deterrence += .12 * damage / weapon->Reload();
					}
				writer.Write(deterrence).SetSeparator(",");
			}
			writer.SaveToStream(cout);
		};

		auto PrintShipList = [](bool variants) -> void
		{
			DataWriter writer;
			for(auto &it : GameData::Ships())
			{
				// Skip variants and unnamed / partially-defined ships, unless specified otherwise.
				if(it.second.ModelName() != it.first && !variants)
					continue;

				writer.Write(it.first);
			}
			writer.SaveToStream(cout);
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
			DataWriter writer;
			writer.SetSeparator(",");
			writer.Write("name", "category", "cost", "space", "range",
					"reload", "burst count", "burst reload", "lifetime", "shots/second",
					"energy/shot", "heat/shot", "recoil/shot",
					"energy/s", "heat/s", "recoil/s", "shield/s",
					"discharge/s", "hull/s", "corrosion/s", "heat dmg/s", "burn dmg/s", "energy dmg/s", "ion dmg/s"
					"scrambling dmg/s", "slow dmg/s", "disruption dmg/s", "piercing", "fuel dmg/s", "leak dmg/s", "push/s",
					"homing", "strength", "deterrence");

			for(auto &it : GameData::Outfits())
			{
				// Skip non-weapons and submunitions.
				if(!it.second.IsWeapon() || it.second.Category().empty())
					continue;

				// The first value is separated with a comma,
				// the rest use semicolons
				const Outfit &outfit = it.second;
				writer.WriteToken(it.first);
				writer.WriteSeparator().SetSeparator(";");

				writer.WriteToken(outfit.Category(), outfit.Cost(), -outfit.Get("weapon capacity"));

				writer.WriteToken(outfit.Range());

				double reload = outfit.Reload();
				writer.WriteToken(reload, outfit.BurstCount(), outfit.BurstReload(), outfit.TotalLifetime());
				double fireRate = 60. / reload;
				writer.WriteToken(fireRate);

				double firingEnergy = outfit.FiringEnergy();
				writer.WriteToken(firingEnergy);
				firingEnergy *= fireRate;
				double firingHeat = outfit.FiringHeat();
				writer.WriteToken(firingHeat);
				firingHeat *= fireRate;
				double firingForce = outfit.FiringForce();
				writer.WriteToken(firingForce);
				firingForce *= fireRate;

				writer.WriteToken(firingEnergy, firingHeat, firingForce);

				double shieldDmg = outfit.ShieldDamage() * fireRate;
				writer.WriteToken(shieldDmg);
				double dischargeDmg = outfit.DischargeDamage() * 100. * fireRate;
				writer.WriteToken(dischargeDmg);
				double hullDmg = outfit.HullDamage() * fireRate;
				writer.WriteToken(hullDmg);
				double corrosionDmg = outfit.CorrosionDamage() * 100. * fireRate;
				writer.WriteToken(corrosionDmg);
				double heatDmg = outfit.HeatDamage() * fireRate;
				writer.WriteToken(heatDmg);
				double burnDmg = outfit.BurnDamage() * 100. * fireRate;
				writer.WriteToken(burnDmg);
				double energyDmg = outfit.EnergyDamage() * fireRate;
				writer.WriteToken(energyDmg);
				double ionDmg = outfit.IonDamage() * 100. * fireRate;
				writer.WriteToken(ionDmg);
				double scramblingDmg = outfit.ScramblingDamage() * 100. * fireRate;
				writer.WriteToken(scramblingDmg);
				double slowDmg = outfit.SlowingDamage() * fireRate;
				writer.WriteToken(slowDmg);
				double disruptDmg = outfit.DisruptionDamage() * fireRate;
				writer.WriteToken(disruptDmg);
				writer.WriteToken(outfit.Piercing());
				double fuelDmg = outfit.FuelDamage() * fireRate;
				writer.WriteToken(fuelDmg);
				double leakDmg = outfit.LeakDamage() * 100. * fireRate;
				writer.WriteToken(leakDmg);
				double hitforce = outfit.HitForce() * fireRate;
				writer.WriteToken(hitforce);

				writer.WriteToken(outfit.Homing());
				double strength = outfit.MissileStrength() + outfit.AntiMissile();
				writer.WriteToken(strength);

				double damage = outfit.ShieldDamage() + outfit.HullDamage();
				double deterrence = .12 * damage / outfit.Reload();
				writer.Write(deterrence).SetSeparator(",");
			}
			writer.SaveToStream(cout);
		};

		auto PrintEngineStats = []() -> void
		{
			DataWriter writer;
			writer.SetSeparator(",");
			writer.Write("name", "cost", "mass", "outfit space", "engine capacity", "thrust/s", "thrust energy/s",
					"thrust heat/s", "turn/s", "turn energy/s", "turn heat/s", "reverse thrust/s", "reverse energy/s",
					"reverse heat/s", "afterburner thrust/s", "afterburner energy/s", "afterburner heat/s", "afterburner fuel/s");

			for(auto &it : GameData::Outfits())
			{
				// Skip non-engines.
				if(it.second.Category() != "Engines")
					continue;

				// The first value is separated with a comma,
				// the rest use semicolons
				const Outfit &outfit = it.second;
				writer.WriteToken(it.first);
				writer.WriteSeparator().SetSeparator(";");

				writer.WriteToken(outfit.Cost(), outfit.Mass());
				writer.WriteToken(outfit.Get("outfit space"), outfit.Get("engine capacity"));
				writer.WriteToken(outfit.Get("thrust") * 3600., outfit.Get("thrusting energy") * 60.,
						outfit.Get("thrusting heat") * 60.);
				writer.WriteToken(outfit.Get("turn") * 60., outfit.Get("turning energy") * 60., outfit.Get("turning heat") * 60.);
				writer.WriteToken(outfit.Get("reverse thrust") * 3600., outfit.Get("reverse thrusting energy") * 60.,
						outfit.Get("reverse thrusting heat") * 60.);
				writer.WriteToken(outfit.Get("afterburner thrust") * 3600., outfit.Get("afterburner energy") * 60.);
				writer.WriteToken(outfit.Get("afterburner heat") * 60., outfit.Get("afterburner fuel") * 60.);

				writer.Write().SetSeparator(",");
			}

			writer.SaveToStream(cout);
		};

		auto PrintPowerStats = []() -> void
		{
			DataWriter writer;
			writer.SetSeparator(",");
			writer.Write("name", "cost", "mass", "outfit space", "energy generation", "heat generation", "energy capacity");

			for(auto &it : GameData::Outfits())
			{
				// Skip non-power.
				if(it.second.Category() != "Power")
					continue;

				// The first value is separated with a comma,
				// the rest use semicolons
				const Outfit &outfit = it.second;
				writer.WriteToken(it.first);
				writer.WriteSeparator().SetSeparator(";");

				writer.WriteToken(outfit.Cost(), outfit.Mass());
				writer.WriteToken(outfit.Get("outfit space"), outfit.Get("energy generation"), outfit.Get("heat generation"),
						outfit.Get("energy capacity"));
				writer.Write().SetSeparator(",");
			}

			writer.SaveToStream(cout);
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

			DataWriter writer;
			writer.SetSeparator(",");
			writer.WriteToken("name", "category", "cost", "mass");
			for(const auto &attribute : attributes)
				writer.WriteToken(attribute);
			writer.Write();

			for(auto &it : GameData::Outfits())
			{
				// The first value is separated with a comma,
				// the rest use semicolons
				const Outfit &outfit = it.second;
				writer.WriteToken(outfit.TrueName());
				writer.WriteSeparator().SetSeparator(";");
				writer.WriteToken(outfit.Category(), outfit.Cost(), outfit.Mass());
				for(const auto &attribute : attributes)
					writer.WriteToken(outfit.Attributes().Get(attribute));
				writer.Write().SetSeparator(",");
			}
			writer.SaveToStream(cout);
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
			PrintObjectList(GameData::Outfits(), true, "outfit");
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
			DataWriter writer;
			writer.SetSeparator(",");
			writer.Write("planet", "description", "spaceport");
			for(auto &it : GameData::Planets())
			{
				writer.WriteToken(it.first);
				writer.WriteSeparator().SetSeparator(";");
				const Planet &planet = it.second;
				writer.WriteToken(planet.Description(), planet.SpaceportDescription());
				writer.Write().SetSeparator(",");
			}
			writer.SaveToStream(cout);
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
			PrintObjectList(GameData::Planets(), false, "planet");
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
			PrintObjectList(GameData::Systems(), false, "system");
	}

	void LocationFilterMatches(const char *const *argv)
	{
		DataFile file(cin);
		LocationFilter filter;
		for(const DataNode &node : file)
		{
			if(node.Token(0) == "changes" || (node.Token(0) == "event" && node.Size() == 1))
				for(const DataNode &child : node)
					GameData::Change(child);
			else if(node.Token(0) == "event")
			{
				const auto *event = GameData::Events().Get(node.Token(1));
				for(const auto &change : event->Changes())
					GameData::Change(change);
			}
			else if(node.Token(0) == "location")
			{
				filter.Load(node);
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
		if(OTHER_VALID_ARGS.count(arg) || OUTFIT_ARGS.count(arg))
			return true;
	}
	return false;
}



void PrintData::Print(const char *const *argv)
{
	for(const char *const *it = argv + 1; *it; ++it)
	{
		string arg = *it;
		if(arg == "-s" || arg == "--ships")
		{
			Ships(argv);
			break;
		}
		else if(OUTFIT_ARGS.count(arg))
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
			LocationFilterMatches(argv);
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
