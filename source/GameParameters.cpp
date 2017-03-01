/* GameParameters.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameParameters.h"

using namespace std;

namespace {
	vector<string> shipCategories = {
		"Transport",
		"Light Freighter",
		"Heavy Freighter",
		"Interceptor",
		"Light Warship",
		"Medium Warship",
		"Heavy Warship",
		"Fighter",
		"Drone"
	};

	// What fraction of its cost a fully depreciated item has left:
	double depreciationFull = 0.25;
	
	double depreciationDaily = 0.99;
	
	int depreciationMaxAge = 1000;
}




void GameParameters::Load(const DataNode &node)
{
	for(const DataNode &node : node)
	{
		const string &key = node.Token(0);
		if(key == "depreciation")
		{
			LoadDepreciation(node);
		}
		else if(key == "ship categories")
		{
			LoadShipCategories(node);
		}
	}
}

void GameParameters::LoadDepreciation(const DataNode &node)
{
	for(const DataNode &node : node)
	{
		const string &key = node.Token(0);
		if(key == "full")
		{
			depreciationFull = node.Value(1);
		}
		else if(key == "daily")
		{
			depreciationDaily = node.Value(1);
		}
		else if(key == "max age")
		{
			depreciationMaxAge = (int)node.Value(1);
		}
	}
}

void GameParameters::LoadShipCategories(const DataNode &node)
{
	shipCategories.clear();

	for(const DataNode &node : node)
	{
		shipCategories.push_back(node.Token(0));
	}
}

const vector<string> &GameParameters::ShipCategories() const
{
	return shipCategories;
}

double GameParameters::DepreciationFull() const
{
	return depreciationFull;
}

double GameParameters::DepreciationDaily() const
{
	return depreciationDaily;
}

int GameParameters::DepreciationMaxAge() const
{
	return depreciationMaxAge;
}