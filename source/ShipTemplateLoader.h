/* ShipTemplateLoader.h
Copyright (c) 2022 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SHIP_TEMPLATE_LOADER_H_
#define SHIP_TEMPLATE_LOADER_H_

#include "ShipTemplate.h"

class DataNode;
class UniverseObjects;



// Helper class to load a ship-template from a datafile. This class is mainly
// intended for loading from datafiles, but (runtime)ship loading from savegames
// will have quite a bit of overlap with the load functions in this class, so
// some code might be shared.
class ShipTemplateLoader {
public:
	// Constructor with the universe that we load for given as parameter.
	ShipTemplateLoader(UniverseObjects &universe);

	// Fully load a ShipsTemplate from a DataNode and return it. This class does
	// not provide save functionality; that is handled by the ShipTemplate itself.
	ShipTemplate Load(const DataNode &node);

	// Savegames typically contain a mix of a ships runtime-data and a ships
	// static data. This function should be used during loading of a savegame,
	// to allow the loader to transfer the loading of static data to the template loader.
	void LoadChild(ShipTemplate &shipTemplate, const DataNode &child);


private:
	// Reference to the universe in which the ships templates being loaded exist.
	UniverseObjects &universe;
};



#endif
