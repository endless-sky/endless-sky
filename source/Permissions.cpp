/* Permissions.cpp
Copyright (c) 2022 by Lemuria

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Files.h"
#include "Permissions.h"
#include "DataNode.h"

using namespace std;

void Permissions::Load (const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "sell")
			canSell = false;
		else {
			child.PrintTrace("Unrecognized permission");
		}
	}
}



const bool Permissions::CanSell() const
{
	return canSell;
}



const bool Permissions::CanUninstall() const
{
	return canUninstall;
}
