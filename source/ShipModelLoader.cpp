/* ShipModelLoader.cpp
Copyright (c) 2022 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipModelLoader.h"

using namespace std;



// Constructor with the universe that we load for given as parameter.
ShipModelLoader::ShipModelLoader(UniverseObjects &universe) : universe(universe)
{
}



// Fully load a ShipsTemplate from a DataNode and return it. This class does
// not provide save functionality; that is handled by the ShipModel itself.
ShipModel ShipModelLoader::Load(const DataNode &node)
{
	ShipModel model;
	
	if(node.Size() >= 2)
	{
		model.SetModelName(node.Token(1));
		model.pluralModelName = node.Token(1) + 's';
	}
	if(node.Size() >= 3)
	{
		ship.base = universe.Ships().Get(ship.ModelName());
		ship.variantName = node.Token(2);
	}
	
	for(const DataNode &child : node){
		LoadChild(model, child);
	}
}

// Savegames typically contain a mix of a ships runtime-data and a ships
// static data. This function should be used during loading of a savegame,
// to allow the loader to transfer the loading of static data to the template loader.
void ShipModelLoader::LoadChild(ShipModel &shipTemplate, const DataNode &child);
{
}



