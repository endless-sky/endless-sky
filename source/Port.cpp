/* Port.cpp
Copyright (c) 2023 by Michael Zahniser

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
	loaded = true;
	const int nameIndex = 1 + (node.Token(0) == "add");
	if(node.Size() > nameIndex)
		name = node.Token(nameIndex);

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);

		if(key == "recharges" && (child.HasChildren() || child.Size() >= 2))
		{
			auto setRecharge = [&](const DataNode &valueNode, const string &value) noexcept -> void {
				if(value == "all")
					recharge |= RechargeType::All;
				else if(value == "shields")
					recharge |= RechargeType::Shields;
				else if(value == "hull")
					recharge |= RechargeType::Hull;
				else if(value == "energy")
					recharge |= RechargeType::Energy;
				else if(value == "fuel")
					recharge |= RechargeType::Fuel;
				else
					valueNode.PrintTrace("Skipping unrecognized attribute:");
			};
			for(int i = 1; i < child.Size(); ++i)
				setRecharge(child, child.Token(i));
			for(const DataNode &grand : child)
				setRecharge(grand, grand.Token(0));
		}
		else if(key == "services" && (child.HasChildren() || child.Size() >= 2))
		{
			auto setServices = [&](const DataNode &valueNode, const string &value) noexcept -> void {
				if(value == "all")
					services |= ServicesType::All;
				else if(value == "trading")
					services |= ServicesType::Trading;
				else if(value == "job board")
					services |= ServicesType::JobBoard;
				else if(value == "bank")
					services |= ServicesType::Bank;
				else if(value == "hire crew")
					services |= ServicesType::HireCrew;
				else if(value == "offers missions")
					services |= ServicesType::OffersMissions;
				else
					valueNode.PrintTrace("Skipping unrecognized attribute:");
			};
			for(int i = 1; i < child.Size(); ++i)
				setServices(child, child.Token(i));
			for(const DataNode &grand : child)
				setServices(grand, grand.Token(0));
		}
		else if(key == "news")
			hasNews = true;
		else if(key == "description" && child.Size() >= 2)
		{
			description.Load(child);

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



void Port::LoadUninhabitedSpaceport()
{
	name = SPACEPORT;
	recharge = RechargeType::All;
	services = ServicesType::OffersMissions;
	hasNews = true;
}



void Port::LoadDescription(const DataNode &node)
{
	description.Load(node);
}



bool Port::CustomLoaded() const
{
	return loaded;
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



const Paragraphs &Port::Description() const
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
