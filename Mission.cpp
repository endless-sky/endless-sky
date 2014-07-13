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
	const Planet *Pick(const DistanceMap &distance, const GameData &data, int minJumps, int maxJumps);
	const Trade::Commodity &PickCommodity(const GameData &data, const System &from, const System &to);
}



Mission Mission::Cargo(const GameData &data, const Planet *source, const DistanceMap &distance)
{
	Mission mission;
	
	// First, pick the destination planet.
	mission.destination = Pick(distance, data, 2, 10);
	
	const System &from = *source->GetSystem();
	const System &to = *mission.destination->GetSystem();
	const Trade::Commodity &commodity = PickCommodity(data, from, to);
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
	
	// TODO: get move semantics working.
	return mission;
}



Mission Mission::Passenger(const GameData &data, const Planet *source, const DistanceMap &distance)
{
	Mission mission;
	
	// First, pick the destination planet.
	mission.destination = Pick(distance, data, 2, 10);
	int jumps = distance.Distance(mission.destination->GetSystem());
	int count = Random::Polya(10, .9) + 1;
	mission.passengers = count;
	// All missions should have a "base pay" to make up for the fact that doing
	// many short missions is more complicated and time consuming than doing one
	// long, big haul.
	mission.payment = 1000 + 1000 * count * jumps;
	
	if(count == 1)
	{
		mission.name = "Ferry a passenger to ";
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
	
	// TODO: get move semantics working.
	return mission;
}



void Mission::Load(const DataFile::Node &node, const GameData &data)
{
	if(node.Size() >= 2)
		name = node.Token(1);
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "destination" && child.Size() >= 2)
			destination = data.Planets().Get(child.Token(1));
		if(child.Token(0) == "cargo" && child.Size() >= 2)
		{
			cargo = child.Token(1);
			if(child.Size() >= 3)
				cargoSize = child.Value(2);
		}
		if(child.Token(0) == "passengers" && child.Size() >= 2)
			passengers = child.Value(1);
		if(child.Token(0) == "payment" && child.Size() >= 2)
			payment = child.Value(1);
		if(child.Token(0) == "description" && child.Size() >= 2)
			description = child.Token(1);
		if(child.Token(0) == "successMessage" && child.Size() >= 2)
			successMessage = child.Token(1);
	}
}



void Mission::Save(ostream &out, const string &tag) const
{
	out << tag << " \"" << name << "\"\n";
	if(destination)
		out << "\tdestination \"" << destination->Name() << "\"\n";
	if(!cargo.empty())
		out << "\tcargo \"" << cargo << "\" " << cargoSize << "\n";
	if(passengers)
		out << "\tpassengers " << passengers << "\n";
	if(payment)
		out << "\tpayment " << payment << "\n";
	if(!description.empty())
		out << "\tdescription \"" << description << "\"\n";
	if(!successMessage.empty())
		out << "\tsuccessMessage \"" << successMessage << "\"\n";
}



const string &Mission::Name() const
{
	return name;
}



const Planet *Mission::Destination() const
{
	return destination;
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



const string &Mission::Description() const
{
	return description;
}



const string &Mission::SuccessMessage() const
{
	return successMessage;
}



namespace {
	// Pick a destination within the given number of jumps.
	const Planet *Pick(const DistanceMap &distance, const GameData &data, int minJumps, int maxJumps)
	{
		vector<const Planet *> options;
		for(const auto &it : data.Planets())
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
	
	
	
	const Trade::Commodity &PickCommodity(const GameData &data, const System &from, const System &to)
	{
		vector<int> weight;
		int total = 0;
		for(const Trade::Commodity &commodity : data.Commodities())
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
				return data.Commodities()[i];
		}
		// Control will never reach here, but to satisfy the compiler:
		return data.Commodities().front();
	}
}
