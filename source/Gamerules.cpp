/* Gamerules.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Gamerules.h"

#include "DataNode.h"

#include <algorithm>

using namespace std;

namespace {
	// Returns true if "token" is a valid boolean.
	bool ValidBool(const string &token) {
		return token == "true" || token == "false";
	}

	// Returns the token as a boolean.
	bool BoolVal(const string &token) {
		return token == "true";
	}
}



// Load a gamerules node.
void Gamerules::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Size() < 2)
		{
			child.PrintTrace("Skipping gamerule with no value:");
			continue;
		}

		const string &key = child.Token(0);
		const string &token = child.Token(1);
		
		if(key == "person spawnrate")
			personSpawnrate = max<int>(1, child.Value(1));
		else if(key == "universal ramscoop")
		{
			if(ValidBool(token))
				universalRamscoop = BoolVal(token);
			else
				child.PrintTrace("Skipping invalid boolean value:");
		}
		else
			child.PrintTrace("Skipping unrecognized gamerule:");
	}
}



int Gamerules::PersonSpawnrate() const
{
	return personSpawnrate;
}



bool Gamerules::UniversalRamscoopActive() const
{
	return universalRamscoop;
}
