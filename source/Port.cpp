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
#include "image/SpriteSet.h"
#include "System.h"

#include <algorithm>

using namespace std;

namespace {
	const string SPACEPORT = "Space_port";
}



// Load a port's description from a node.
void Port::Load(const DataNode &node, const ConditionsStore *playerConditions)
{
	loaded = true;
	const int nameIndex = 1 + (node.Token(0) == "add");
	if(node.Size() > nameIndex)
		displayName = node.Token(nameIndex);

	// The "to recharge" and "to service" condition set maps should be cleared
	// if a new condition set is provided.
	bool overwriteToRecharge = true;
	bool overwriteToService = true;

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "recharges" && (child.HasChildren() || hasValue))
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
		else if(key == "services" && (child.HasChildren() || hasValue))
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
		else if(key == "description" && hasValue)
		{
			description.Load(child, playerConditions);

			// If we have a description but no name then use the default spaceport name.
			if(displayName.empty())
				displayName = SPACEPORT;
		}
		else if(key == "to" && child.Size() >= 2)
		{
			const string &conditional = child.Token(1);
			if(conditional == "bribe")
				toRequireBribe.Load(child, playerConditions);
			else if(conditional == "access")
				toAccess.Load(child, playerConditions);
			else if(conditional == "recharge" && child.Size() >= 3)
			{
				if(overwriteToRecharge)
				{
					overwriteToRecharge = false;
					toRecharge.clear();
				}
				ConditionSet conditionSet(child, playerConditions);
				for(int i = 2; i < child.Size(); ++i)
				{
					const string &listValue = child.Token(i);
					if(listValue == "all")
					{
						toRecharge[RechargeType::Shields] = conditionSet;
						toRecharge[RechargeType::Hull] = conditionSet;
						toRecharge[RechargeType::Energy] = conditionSet;
						toRecharge[RechargeType::Fuel] = conditionSet;
					}
					else if(listValue == "shields")
						toRecharge[RechargeType::Shields] = conditionSet;
					else if(listValue == "hull")
						toRecharge[RechargeType::Hull] = conditionSet;
					else if(listValue == "energy")
						toRecharge[RechargeType::Energy] = conditionSet;
					else if(listValue == "fuel")
						toRecharge[RechargeType::Fuel] = conditionSet;
					else
						child.PrintTrace("Skipping unrecognized attribute:");
				}
			}
			else if(conditional == "service" && child.Size() >= 3)
			{
				if(overwriteToService)
				{
					overwriteToService = false;
					toService.clear();
				}
				ConditionSet conditionSet(child, playerConditions);
				for(int i = 2; i < child.Size(); ++i)
				{
					const string &listValue = child.Token(i);
					if(listValue == "all")
					{
						toService[ServicesType::Trading] = conditionSet;
						toService[ServicesType::JobBoard] = conditionSet;
						toService[ServicesType::Bank] = conditionSet;
						toService[ServicesType::HireCrew] = conditionSet;
						toService[ServicesType::OffersMissions] = conditionSet;
					}
					else if(listValue == "trading")
						toService[ServicesType::Trading] = conditionSet;
					else if(listValue == "job board")
						toService[ServicesType::JobBoard] = conditionSet;
					else if(listValue == "bank")
						toService[ServicesType::Bank] = conditionSet;
					else if(listValue == "hire crew")
						toService[ServicesType::HireCrew] = conditionSet;
					else if(listValue == "offers missions")
						toService[ServicesType::OffersMissions] = conditionSet;
					else
						child.PrintTrace("Skipping unrecognized attribute:");
				}
			}
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void Port::LoadDefaultSpaceport()
{
	displayName = SPACEPORT;
	recharge = RechargeType::All;
	services = ServicesType::All;
	hasNews = true;
}



void Port::LoadUninhabitedSpaceport()
{
	displayName = SPACEPORT;
	recharge = RechargeType::All;
	services = ServicesType::OffersMissions;
	hasNews = true;
}



void Port::LoadDescription(const DataNode &node, const ConditionsStore *playerConditions)
{
	description.Load(node, playerConditions);
}



bool Port::CustomLoaded() const
{
	return loaded;
}



const string &Port::DisplayName() const
{
	return displayName;
}



const Paragraphs &Port::Description() const
{
	return description;
}



bool Port::RequiresBribe() const
{
	return !toRequireBribe.IsEmpty() && toRequireBribe.Test();
}



bool Port::CanAccess() const
{
	return toAccess.Test();
}



// Get all the possible sources that can get recharged at this port.
int Port::GetRecharges(bool isPlayer) const
{
	if(!isPlayer || !recharge)
		return recharge;

	int retVal = RechargeType::None;
	if(CanRecharge(RechargeType::Shields))
		retVal |= RechargeType::Shields;
	if(CanRecharge(RechargeType::Hull))
		retVal |= RechargeType::Hull;
	if(CanRecharge(RechargeType::Energy))
		retVal |= RechargeType::Energy;
	if(CanRecharge(RechargeType::Fuel))
		retVal |= RechargeType::Fuel;
	return retVal;
}



// Check whether the given recharging is possible.
bool Port::CanRecharge(int type, bool isPlayer) const
{
	bool hasType = (recharge & type);
	// The All type shouldn't be used when isPlayer is true.
	// If for some reason it is, behave as if isPlayer was false.
	if(!hasType || !isPlayer || type == RechargeType::All)
		return hasType;
	if(!CanAccess())
		return false;
	auto it = toRecharge.find(type);
	return it == toRecharge.end() || it->second.Test();
}



// Whether this port has any services available.
bool Port::HasServices(bool isPlayer) const
{
	if(!isPlayer || !services)
		return services;

	return HasService(ServicesType::Trading)
			|| HasService(ServicesType::JobBoard)
			|| HasService(ServicesType::Bank)
			|| HasService(ServicesType::HireCrew)
			|| HasService(ServicesType::OffersMissions);
}



// Check whether the given service is available.
bool Port::HasService(int type, bool isPlayer) const
{
	bool hasType = (services & type);
	// The All type shouldn't be used when isPlayer is true.
	// If for some reason it is, behave as if isPlayer was false.
	if(!hasType || !isPlayer || type == ServicesType::All)
		return hasType;
	if(!CanAccess())
		return false;
	auto it = toService.find(type);
	return it == toService.end() || it->second.Test();
}



bool Port::HasNews() const
{
	return hasNews;
}
