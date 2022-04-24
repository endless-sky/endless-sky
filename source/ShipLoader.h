/* ShipLoader.h
Copyright (c) 2022 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Ship.h"



class UniverseObjects;



class ShipLoader
{
public:
	ShipLoader(UniverseObjects &universe);

	// Load a given ship from a DataNode.
	void LoadShip(Ship &ship, const DataNode &node) const;

	// When loading a ship, some of the outfits it lists may not have been
	// loaded yet. So, wait until everything has been loaded, then call this.
	void FinishLoading(Ship &ship, bool isNewInstance) const;


private:
	// Reference to the universe in which this ship operates.
	UniverseObjects &universe;
};
