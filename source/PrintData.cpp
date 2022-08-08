/* PrintData.cpp
Copyright (c) 2022 by warp-core

Main function for Endless Sky, a space exploration and combat RPG.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "PrintData.h"

#include "GameData.h"
#include "Outfit.h"
#include "Ship.h"

#include <iostream>
#include <map>
#include <set>

using namespace std;



void PrintData::Print(char *argv[])
{
	const char *const *it = argv + 2;
	if(!*it)
		return;
	string arg = *it;
	if(arg == "-s" || arg == "--ships")
		Ships(argv);
	else if(arg == "-w" || arg == "--weapons")
		Weapons(argv);
	else if(arg == "-e" || arg == "--engines")
		PrintEngineStats();
	else if(arg == "-p" || arg == "--power")
		PrintPowerStats();
	else if(arg == "-o" || arg == "--outfits")
		Outfits(argv);
	else if(arg == "-h" || arg == "--help")
		Help();
	cout.flush();
}



void PrintData::Help()
{
	cerr << "--printdata: follow with arguments to print diffferent data." << endl;
	cerr << "    -s, --ships: prints table of various base stats of all ships (not variants)." << endl;
	cerr << "        -d, --deterrences: prints ship names and deterrence values, not variants." << endl;
	cerr << "        -v, --variants: includes variants." << endl;
	cerr << "        -s, --sales: prints a list of ships and the outfitters they appear in." << endl;
	cerr << "        -o, --old: the older version of the PrintShipTable() function." << endl;
	cerr << "    -w, --weapons: prints table of weapons and various stats." << endl;
	cerr << "        -d, --deterrences: prints list of weapons and deterrence values." << endl;
	cerr << "    -e, --engines: prints table of engines and various stats." << endl;
	cerr << "    -p, --power: prints table of power outfits and various stats." << endl;
	cerr << "    -o, --outfits: prints a list of outfits." << endl;
	cerr << "        -s, --sales: prints a list of outfits and all the outfitters they appear in." << endl;
}



void PrintData::Ships(char *argv[])
{
	bool deterrence = false;
	bool variants = false;
	bool sales = false;

	for(const char *const *it = argv + 3; *it; ++it)
	{
		string arg = *it;
		if(arg == "-d" || arg == "--deterrences")
			deterrence = true;
		else if(arg == "-v" || arg == "--variants")
			variants = true;
		else if(arg == "-s" || arg == "--sales")
			sales = true;
		else if(arg == "-o" || arg == "--old")
		{
			PrintShipOldTable();
			return;
		}
	}

	if(deterrence)
		PrintShipDeterrences(variants);
	else if(sales)
		PrintShipShipyards();
	else
		PrintShipBaseStats();
}



void PrintData::PrintShipBaseStats()
{
	cout << "model" << ',' << "category" << ',' << "chassis cost" << ',' << "loaded cost" << ',' << "shields" << ','
		<< "hull" << ',' << "mass" << ',' << "drag" << ',' << "heat dis" << ','
		<< "crew" << ',' << "bunks" << ',' << "cargo" << ',' << "fuel" << ','
		<< "outfit" << ',' << "weapon" << ',' << "engine" << ',' << "gun mounts" << ','
		<< "turret mounts" << ',' << "fighter bays" << ',' << "drone bays" << '\n';
	for(auto &it : GameData::Ships())
	{
		// Skip variants and unnamed / partially-defined ships.
		if(it.second.ModelName() != it.first)
			continue;

		const Ship &ship = it.second;
		cout << it.first << ',';

		const Outfit &attributes = ship.BaseAttributes();
		cout << attributes.Category() << ',';
		cout << ship.ChassisCost() << ',';
		cout << ship.Cost() << ',';

		auto mass = attributes.Mass() ? attributes.Mass() : 1.;
		cout << attributes.Get("shields") << ',';
		cout << attributes.Get("hull") << ',';
		cout << mass << ',';
		cout << attributes.Get("drag") << ',';
		cout << ship.HeatDissipation() * 1000.<< ',';
		cout << attributes.Get("required crew") << ',';
		cout << attributes.Get("bunks") << ',';
		cout << attributes.Get("cargo space") << ',';
		cout << attributes.Get("fuel capacity") << ',';

		cout << ship.BaseAttributes().Get("outfit space") << ',';
		cout << ship.BaseAttributes().Get("weapon capacity") << ',';
		cout << ship.BaseAttributes().Get("engine capacity") << ',';

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
}



void PrintData::PrintShipDeterrences(bool variants)
{
	cout << "model" << ',' << "deterrence" << '\n';
	for(auto &it : GameData::Ships())
	{
		if(!variants && it.second.ModelName() != it.first)
			continue;

		const Ship &ship = it.second;
		double deterrence = 0;
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
		cout << it.first << ',' << deterrence << '\n';
	}
}



void PrintData::PrintShipShipyards()
{
	cout << "ship" << ',' << "shipyards" << '\n';
	map<string, set<string>> ships;
	for(auto &it : GameData::Shipyards())
	{
		for(auto &it2 : it.second)
		{
			ships[it2->ModelName()].insert(it.first);
		}
	}
	for(auto &it : GameData::Ships())
	{
		if(it.first != it.second.ModelName())
			continue;
		cout << it.first;
		for(auto &it2 : ships[it.first])
		{
			cout << ',' << it2;
		}
		cout << '\n';
	}
}



void PrintData::PrintShipOldTable()
{
	cout << "model" << '\t' << "cost" << '\t' << "shields" << '\t' << "hull" << '\t'
		<< "mass" << '\t' << "crew" << '\t' << "cargo" << '\t' << "bunks" << '\t'
		<< "fuel" << '\t' << "outfit" << '\t' << "weapon" << '\t' << "engine" << '\t'
		<< "speed" << '\t' << "accel" << '\t' << "turn" << '\t'
		<< "energy generation" << '\t' << "max energy usage" << '\t' << "energy capacity" << '\t'
		<< "idle/max heat" << '\t' << "max heat generation" << '\t' << "max heat dissipation" << '\t'
		<< "gun mounts" << '\t' << "turret mounts" << '\n';
	for(auto &it : GameData::Ships())
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
}



void PrintData::Weapons(char *argv[])
{
	for(const char *const *it = argv + 3; *it; ++it)
	{
		string arg = *it;
		if(arg == "-d" || arg == "--deterrence")
			PrintWeaponDeterrences();
		else
			PrintWeaponStats();
	}
}



void PrintData::PrintWeaponStats()
{
	cout << "name" << ',' << "cost" << ',' << "space" << ',' << "range" << ','
		<< "energy/s" << ',' << "heat/s" << ',' << "recoil/s" << ','
		<< "shield/s" << ',' << "hull/s" << ',' << "heatdmg/s" << ',' << "push/s" << ','
		<< "homing" << ',' << "strength" << ',';
	for(auto &it : GameData::Outfits())
	{
		// Skip non-weapons and submunitions.
		if(!it.second.IsWeapon() || it.second.Category().empty())
			continue;

		const Outfit &outfit = it.second;
		cout << it.first << ',';
		cout << outfit.Cost() << ',';
		cout << -outfit.Get("weapon capacity") << ',';

		cout << outfit.Range() << ',';

		double energy = outfit.FiringEnergy() * 60. / outfit.Reload();
		cout << energy << ',';
		double heat = outfit.FiringHeat() * 60. / outfit.Reload();
		cout << heat << ',';
		double firingforce = outfit.FiringForce() * 60. / outfit.Reload();
		cout << firingforce << ',';

		double shield = outfit.ShieldDamage() * 60. / outfit.Reload();
		cout << shield << ',';
		double hull = outfit.HullDamage() * 60. / outfit.Reload();
		cout << hull << ',';
		double heatDmg = outfit.HeatDamage() * 60. / outfit.Reload();
		cout << heatDmg << ',';
		double hitforce = outfit.HitForce() * 60. / outfit.Reload();
		cout << hitforce << ',';

		cout << outfit.Homing() << ',';
		double strength = outfit.MissileStrength() + outfit.AntiMissile();
		cout << strength << '\n';
	}
	cout.flush();
}



void PrintData::PrintWeaponDeterrences()
{
	cout << "name" << ',' << "deterrence" << ',' << "outfit" << ',' << "weapon" << '\n';
	for(auto &it : GameData::Outfits())
	{
		// Skip non-weapons and submunitions.
		if(!it.second.IsWeapon() || it.second.Category().empty())
			continue;
		const Outfit &weapon = it.second;
		double damage = weapon.ShieldDamage() + weapon.HullDamage();
		double deterrence = .12 * damage / weapon.Reload();
		cout << it.first << ',' << deterrence << ',' << -weapon.Get("outfit space") << ',' << -weapon.Get("weapon capacity") << '\n';
	}
}



void PrintData::PrintEngineStats()
{
	cout << "name" << '\t' << "cost" << '\t' << "mass" << '\t' << "outfit space" << '\t'
		<< "engine capacity" << '\t' << "thrust/s" << '\t' << "thrust energy/s" << '\t'
		<< "thrust heat/s" << '\t' << "turn/s" << '\t' << "turn energy/s" << '\t'
		<< "turn heat/s" << '\t' << "reverse thrust/s" << '\t' << "reverse energy/s" << '\t'
		<< "reverse heat/s" << '\n';
	for(auto &it : GameData::Outfits())
	{
		// Ship non-engines.
		if(it.second.Category() != "Engines")
			continue;

		const Outfit &outfit = it.second;
		cout << it.first << ',';
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
		cout << outfit.Get("reverse thrusting heat") * 60. << '\n';
	}
	cout.flush();
}



void PrintData::PrintPowerStats()
{
	cout << "name" << ',' << "cost" << ',' << "mass" << ',' << "outfit space" << ','
		<< "energy gen" << ',' << "heat gen" << ',' << "energy cap" << '\n';
	for(auto &it : GameData::Outfits())
	{
		// Ship non-power.
		if(it.second.Category() != "Power")
			continue;

		const Outfit &outfit = it.second;
		cout << it.first << ',';
		cout << outfit.Cost() << ',';
		cout << outfit.Mass() << ',';
		cout << outfit.Get("outfit space") << ',';
		cout << outfit.Get("energy generation") << ',';
		cout << outfit.Get("heat generation") << ',';
		cout << outfit.Get("energy capacity") << '\n';
	}
	cout.flush();
}



void PrintData::Outfits(char *argv[])
{
	for(const char *const *it = argv + 3; *it; ++it)
	{
		string arg = *it;
		if(arg == "-s" || arg == "--sales")
			PrintOutfitOutfitters();
		else
			PrintOutfitsList();
	}
}



void PrintData::PrintOutfitsList()
{
	cout << "outfit name" << '\n';
	for(auto &it : GameData::Outfits())
		cout << it.first << '\n';
}



void PrintData::PrintOutfitOutfitters()
{
	cout << "outfits" << ',' << "outfitters" << '\n';
	map<string, set<string>> outfits;
	for(auto &it : GameData::Outfitters())
	{
		for(auto &it2 : it.second)
		{
			outfits[it2->Name()].insert(it.first);
		}
	}
	for(auto &it : GameData::Outfits())
	{
		cout << it.first;
		for(auto &it2 : outfits[it.first])
		{
			cout << ',' << it2;
		}
		cout << '\n';
	}
}

