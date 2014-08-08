/* Mission.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Mission.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "DistanceMap.h"
#include "GameData.h"
#include "Random.h"
#include "Trade.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace std;

namespace {
	// Pick a destination within the given number of jumps.
	const Planet *Pick(const DistanceMap &distance, int minJumps, int maxJumps);
	const Trade::Commodity &PickCommodity(const System &from, const System &to);
}



Mission Mission::Cargo(const Planet *source, const DistanceMap &distance)
{
	Mission mission;
	
	// First, pick the destination planet.
	mission.destination = Pick(distance, 2, 10);
	
	const System &from = *source->GetSystem();
	const System &to = *mission.destination->GetSystem();
	const Trade::Commodity &commodity = PickCommodity(from, to);
	mission.cargo = commodity.items[Random::Int(commodity.items.size())];
	mission.cargoSize = Random::Polya(2, .1) + 5;
	int jumps = distance.Distance(&to);
	// All missions should have a "base pay" to make up for the fact that doing
	// many short missions is more complicated and time consuming than doing one
	// long, big haul.
	mission.payment = 1000 + 100 * mission.cargoSize * jumps;
	
	mission.name = "Delivery to " + mission.destination->Name();
	mission.description = "Deliver " + to_string(mission.cargoSize) + " tons of "
		+ mission.cargo + " to " + mission.destination->Name() + " in the "
		+ mission.destination->GetSystem()->Name() + " system. Payment is "
		+ to_string(mission.payment) + " credits.";
	mission.successMessage = "The dock workers unload the " + mission.cargo
		+ " and pay you " + to_string(mission.payment) + " credits.";
	
	// This should automatically use move semantics.
	return mission;
}



Mission Mission::Passenger(const Planet *source, const DistanceMap &distance)
{
	Mission mission;
	
	// First, pick the destination planet.
	mission.destination = Pick(distance, 2, 10);
	int jumps = distance.Distance(mission.destination->GetSystem());
	int count = Random::Polya(10, .9) + 1;
	mission.passengers = count;
	// All missions should have a "base pay" to make up for the fact that doing
	// many short missions is more complicated and time consuming than doing one
	// long, big haul.
	mission.payment = 1000 + 1000 * count * jumps;
	
	if(count == 1)
	{
		mission.name = "Ferry passenger to ";
		mission.description = "This passenger wants to go to ";
		mission.successMessage = "Your passenger disembarks, and pays you ";
	}
	else
	{
		mission.name = "Ferry passengers to ";
		mission.description = to_string(count) + " passengers want to go to ";
		mission.successMessage = "Your passengers disembark, and pay you ";
	}
	mission.name += mission.destination->Name();
	mission.description += mission.destination->Name() + " in the "
		+ mission.destination->GetSystem()->Name() + " system. Payment is "
		+ to_string(mission.payment) + " credits.";
	mission.successMessage += to_string(mission.payment) + " credits.";
	
	// This should automatically use move semantics.
	return mission;
}



void Mission::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		name = node.Token(1);
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "destination" && child.Size() >= 2)
			destination = GameData::Planets().Get(child.Token(1));
		else if(child.Token(0) == "cargo" && child.Size() >= 2)
		{
			cargo = child.Token(1);
			if(child.Size() >= 3)
				cargoSize = child.Value(2);
		}
		else if(child.Token(0) == "passengers" && child.Size() >= 2)
			passengers = child.Value(1);
		else if(child.Token(0) == "payment" && child.Size() >= 2)
			payment = child.Value(1);
		else if(child.Token(0) == "description" && child.Size() >= 2)
			description = child.Token(1);
		else if(child.Token(0) == "success")
		{
			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "message" && grand.Size() >= 2)
					successMessage = grand.Token(1);
				else if(grand.Token(0) == "mission" && grand.Size() >= 2)
					next = GameData::Missions().Get(grand.Token(1));
			}
		}
		else if(child.Token(0) == "require")
		{
			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "planet" && grand.Size() >= 2)
					sourcePlanets.insert(GameData::Planets().Get(grand.Token(1)));
			}
		}
		else if(child.Token(0) == "conversation")
			introduction.Load(child);
	}
}



void Mission::Save(DataWriter &out, const string &tag) const
{
	out.Write(tag, name);
	out.BeginChild();
		if(destination)
			out.Write("destination", destination->Name());
		if(!cargo.empty())
			out.Write("cargo", cargo, cargoSize);
		if(passengers)
			out.Write("passengers", passengers);
		if(payment)
			out.Write("payment", payment);
		if(!description.empty())
			out.Write("description", description);
		if(!successMessage.empty())
		{
			out.Write("success");
			out.BeginChild();
				out.Write("message", successMessage);
			out.EndChild();
		}
	out.EndChild();
}



const string &Mission::Name() const
{
	return name;
}



const Planet *Mission::Destination() const
{
	return destination;
}



const string &Mission::Description() const
{
	return description;
}



bool Mission::IsAvailableAt(const Planet *planet, const map<string, int> &conditions) const
{
	auto it = conditions.find(name);
	if(it != conditions.end() && it->second)
		return false;
	
	return (sourcePlanets.find(planet) != sourcePlanets.end());
}



const Conversation &Mission::Introduction() const
{
	return introduction;
}



const string &Mission::Cargo() const
{
	return cargo;
}



int Mission::CargoSize() const
{
	return cargoSize;
}



int Mission::Passengers() const
{
	return passengers;
}



int Mission::Payment() const
{
	return payment;
}


const string &Mission::SuccessMessage() const
{
	return successMessage;
}



const Mission *Mission::Next() const
{
	return next;
}



namespace {
	// Pick a destination within the given number of jumps.
	const Planet *Pick(const DistanceMap &distance, int minJumps, int maxJumps)
	{
		vector<const Planet *> options;
		for(const auto &it : GameData::Planets())
		{
			const Planet &planet = it.second;
			if(!planet.HasSpaceport())
				continue;
			
			int jumps = distance.Distance(planet.GetSystem());
			if(jumps >= minJumps && jumps <= maxJumps)
				options.push_back(&planet);
		}
		return options[Random::Int(options.size())];
	}
	
	
	
	const Trade::Commodity &PickCommodity(const System &from, const System &to)
	{
		vector<int> weight;
		int total = 0;
		for(const Trade::Commodity &commodity : GameData::Commodities())
		{
			// For every 100 credits in profit you can make, double the chance
			// of this commodity being chosen.
			double profit = to.Trade(commodity.name) - from.Trade(commodity.name);
			int w = max(1, static_cast<int>(100. * pow(2., profit * .01)));
			weight.push_back(w);
			total += w;
		}
		// Pick a random commodity based on those weights.
		int r = Random::Int(total);
		for(unsigned i = 0; i < weight.size(); ++i)
		{
			r -= weight[i];
			if(r < 0)
				return GameData::Commodities()[i];
		}
		// Control will never reach here, but to satisfy the compiler:
		return GameData::Commodities().front();
	}
}
