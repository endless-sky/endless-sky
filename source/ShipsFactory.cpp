/* ShipsFactory.cpp
Copyright (c) 2022 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipsFactory.h"

#include "DataNode.h"
#include "GameData.h"
#include "UniverseObjects.h"

using namespace std;



ShipsFactory::ShipsFactory(UniverseObjects &universe): universe(universe)
{
}



// Load a ship from a datafile. Creation of the Ship object itself has
// already been done by the caller. Getting only the DataNode as parameter
// and returning a shared_ptr<Ship> is nicer than getting the ship as
// parameter by reference, but the ES code allows overwriting ship-
// definitions by a new load, so we need to support overwriting existing
// ship definitions here.
void ShipsFactory::LoadShip(Ship &ship, const DataNode &data) const
{
	ship.Load(data);
}



// When loading a ship, some of the outfits it lists may not have been
// loaded yet. So, wait until everything has been loaded, then call this.
void ShipsFactory::FinishLoading(Ship &ship, bool isNewInstance) const
{
	ship.FinishLoading(isNewInstance);
}
