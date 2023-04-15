/* Port.cpp
Copyright (c) 2021 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Port.h"

#include "DataNode.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "PlayerInfo.h"
#include "Politics.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "SpriteSet.h"
#include "System.h"

#include <algorithm>

using namespace std;

namespace {
	const string SPACEPORT = "Space_port";
}



// Load a port's description from a node.
void Port::Load(const DataNode &node)
{
	if(node.Size() > 1)
		name = node.Token(1);

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);

		if(key == "recharges" && child.HasChildren())
		{
			for(const DataNode &grand : child)
			{
				const string &grandKey = grand.Token(0);

				if(grandKey == "shields")
					recharge |= Port::RechargeType::Shields;
				else if(grandKey == "hull")
					recharge |= Port::RechargeType::Hull;
				else if(grandKey == "energy")
					recharge |= Port::RechargeType::Energy;
				else if(grandKey == "fuel")
					recharge |= Port::RechargeType::Fuel;
			}
		}
		else if(key == "services" && child.HasChildren())
		{
			for(const DataNode &grand : child)
			{
				const string &grandKey = grand.Token(0);

				if(grandKey == "trading")
					services |= Port::ServicesType::Trading;
				else if(grandKey == "job board")
					services |= Port::ServicesType::JobBoard;
				else if(grandKey == "bank")
					services |= Port::ServicesType::Bank;
				else if(grandKey == "hire crew")
					services |= Port::ServicesType::HireCrew;
				else if(grandKey == "offers missions")
					services |= Port::ServicesType::OffersMissions;
			}
		}
		else if(key == "news")
			hasNews = true;
		else if(key == "description" && child.Size() >= 2)
		{
			const string &value = child.Token(1);
			if(!description.empty() && !value.empty() && value[0] > ' ')
				description += '\t';
			description += value;
			description += '\n';

			// If we have a description but no name then use the default spaceport name.
			if(name.empty())
				name = SPACEPORT;
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void Port::LoadDefaultSpaceport()
{
	name = SPACEPORT;
	recharge = RechargeType::All;
	services = ServicesType::All;
	hasNews = true;
}



// Whether this port has any services available.
bool Port::HasServices() const
{
	return services;
}



// Get all the possible sources that can get recharged at this port.
int Port::GetRecharges() const
{
	return recharge;
}



const string &Port::Name() const
{
	return name;
}



string &Port::Description()
{
	return description;
}



const string &Port::Description() const
{
	return description;
}



// Check whether the given recharging is possible.
bool Port::CanRecharge(int type) const
{
	return (recharge & type);
}



// Check whether the given service is available.
bool Port::HasService(int type) const
{
	return (services & type);
}



bool Port::HasNews() const
{
	return hasNews;
}
