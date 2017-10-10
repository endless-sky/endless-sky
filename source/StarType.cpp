/* StarType.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "StarType.h"

#include "DataNode.h"

using namespace std;



StarType::StarType()
{
	name = "Name not found";
	stellarWind = 1;
	luminosity = 1;	
}



void StarType::Load(DataNode node)
{
	if(node.Size() < 2)
		return;
	name = node.Token(1);
	
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "wind")
			stellarWind = child.Value(1);
		else if(key == "luminosity")
			luminosity = child.Value(1);
	}
}



string StarType::GetName() const
{
	return name;
}



double StarType::GetStellarWind() const
{
	return stellarWind;
}



double StarType::GetLuminosity() const
{
	return luminosity;
}
