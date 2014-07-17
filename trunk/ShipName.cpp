/* ShipName.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipName.h"

#include "DataNode.h"

#include <cstdlib>

using namespace std;



void ShipName::Load(const DataNode &node)
{
	if(node.Token(0) != "shipName")
		return;
	
	words.push_back(vector<vector<string>>());
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "word")
		{
			words.back().push_back(vector<string>());
			for(const DataNode &grand : child)
				words.back().back().push_back(grand.Token(0));
		}
	}
}



string ShipName::Get() const
{
	string result;
	if(words.empty())
		return result;
	
	for(const vector<string> &v : words[rand() % words.size()])
		result += v[rand() % v.size()];
	
	return result;
}
