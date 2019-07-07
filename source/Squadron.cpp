/* Squadron.cpp
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "DataNode.h"
#include "Ship.h"
#include "Squadron.h"

#include <string>

using namespace std;

void Squadron::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "template")
			templateShip = new Ship(child);
		else if (key == "desired" && child.Size() >= 2)
			desired = child.Value(1);
		else if (key == "actual" && child.Size() >= 2)
			actual = child.Value(1);
	}
}
