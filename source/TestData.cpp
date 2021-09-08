/* TestData.cpp
Copyright (c) 2019-2020 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "TestData.h"

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"

using namespace std;



// Loader to load the generic test-data entry
void TestData::Load(const DataNode &node)
{
	this->node = node;
	if(node.Size() < 2)
	{
		node.PrintTrace("Skipping unnamed test data:");
		return;
	}
	if(node.Token(0) != "test-data")
	{
		node.PrintTrace("Skipping unsupported root node:");
		return;
	}
	
	for(const DataNode &child : node)
		// Only need to parse the category for now. The contents will be
		// scanned for at write-out of the test-data.
		if(child.Size() > 1 && child.Token(0) == "category")
		{
			if(child.Token(1) == "savegame")
				dataSetType = Type::SAVEGAME;
			else
				child.PrintTrace("Skipping unsupported category:");
		}
}



// Inject the test-data to the proper location.
bool TestData::Inject() const
{
	// Determine data-type and call the relevant function.
	switch(dataSetType)
	{
		case Type::SAVEGAME:
			return InjectSavegame();
		default:
			return false;
	}
}



// Write out testdata as savegame into the saves directory.
bool TestData::InjectSavegame() const
{
	// Scan for the contents keyword
	// Then write out the complete contents to the target file
	// Scan for the contents tag
	for(const DataNode &dataNode : node)
		if(dataNode.Token(0) == "contents")
		{
			// Savegame data is written to the saves directory. Other test data
			// types might be injected differently, e.g. direct object loading.
			DataWriter dataWriter(Files::Saves() + node.Token(1) + ".txt");
			for(const DataNode &child : dataNode)
				dataWriter.Write(child);

			// Data was found and written. We are done succesfully.
			return true;
		}

	// Content section was not found. (Should we just create an empty file here?)
	return false;
}
