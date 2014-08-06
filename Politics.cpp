/* Politics.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Politics.h"

#include "GameData.h"

#include <algorithm>

using namespace std;



// Reset to the initial political state defined in the game data.
void Politics::Reset()
{
	attitudeToward.clear();
	reputationWith.clear();
	provoked.clear();
	
	for(const auto &it : GameData::Governments())
	{
		const Government *gov = &it.second;
		reputationWith[gov] = it.second.InitialPlayerReputation();
		for(const auto &oit : GameData::Governments())
			attitudeToward[gov][&oit.second] = oit.second.InitialAttitudeToward(gov);
		attitudeToward[gov][gov] = 1.;
	}
}



bool Politics::IsEnemy(const Government *first, const Government *second) const
{
	if(first == second)
		return false;
	
	// Just for simplicity, if one of the governments is the player, make sure
	// it is the first one.
	if(second == GameData::PlayerGovernment())
		swap(first, second);
	if(first == GameData::PlayerGovernment())
	{
		if(provoked.find(second) != provoked.end())
			return true;
		
		auto it = reputationWith.find(second);
		return (it != reputationWith.end() && it->second < 0.);
	}
	
	// Neither government is the player, so the question of enemies depends only
	// on the attitude matrix.
	return (Attitude(first, second) < 0. || Attitude(second, first) < 0.);
}



// Get the attitude of one government toward another. This does not apply to
// the player's government, which uses "reputation" instead.
double Politics::Attitude(const Government *gov, const Government *other) const
{
	auto oit = attitudeToward.find(other);
	if(oit == attitudeToward.end())
		return 0.;
	
	auto it = oit->second.find(gov);
	if(it == oit->second.end())
		return 0.;
	
	return it->second; 
}



// Set the attitude of the given government toward the other government. A
// positive value means they are allies, i.e. anything that affects your
// reputation with one affects it with the other. A negative value means
// that whatever hurts your reputation with one, helps it with the other.
// The value should be between -1 and 1, controlling how strongly your
// reputation is affected.
void Politics::SetAttitude(const Government *gov, const Government *other, double value)
{
	attitudeToward[other][gov] = value;
}



// Commit the given "offense" against the given government (which may not
// actually consider it to be an offense). This may result in temporary
// hostilities (if the even type is PROVOKE), or a permanent change to your
// reputation.
void Politics::Offend(const Government *gov, ShipEvent::Type type, int count)
{
	if(gov == GameData::PlayerGovernment())
		return;
	
	for(const auto &other : attitudeToward[gov])
	{
		// You can provoke a government even by attacking an empty ship, such as
		// a drone (count = 0, because count = crew).
		if(type == ShipEvent::PROVOKE)
		{
			if(other.second > 0.)
				provoked.insert(other.first);
		}
		else if(count * other.second)
		{
			double penalty = (count * other.second) * other.first->PenaltyFor(type);
			if(type == ShipEvent::ATROCITY)
				reputationWith[other.first] = min(0., reputationWith[other.first]);
			
			reputationWith[other.first] -= penalty;
		}
	}
}



// Get or set your reputation with the given government.
double Politics::Reputation(const Government *gov)
{
	auto it = reputationWith.find(gov);
	return (it == reputationWith.end() ? 0. : it->second);
}



void Politics::AddReputation(const Government *gov, double value)
{
	reputationWith[gov] += value;
}



void Politics::SetReputation(const Government *gov, double value)
{
	reputationWith[gov] = value;
}



// Reset any temporary provocation (typically because a day has passed).
void Politics::ResetProvocation()
{
	provoked.clear();
}
