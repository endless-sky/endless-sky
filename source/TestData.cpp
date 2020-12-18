/* TestData.cpp
Copyright (c) 2019 by Peter van der Meer

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



const string &TestData::Name() const
{
	return dataSetName;
}



// Loader to load the generic test-data entry
void TestData::Load(const DataNode &node, const string &sourceDataFilePath)
{
	sourceDataFile = sourceDataFilePath;
	if(node.Size() < 2)
	{
		node.PrintTrace("No name specified for test-data dataset");
		return;
	}
	if(node.Token(0) != "test-data")
	{
		node.PrintTrace("Non-test found in test-data dataset parsing");
		return;
	}
	dataSetName = node.Token(1);
	
	for(const DataNode &child : node)
		// Only need to parse the category for now. The contents will be
		// scanned for at write-out of the test-data.
		if(child.Size() > 1 && child.Token(0) == "category")
		{
			if(child.Token(1) == "savegame")
				dataSetType = Type::SAVEGAME;
			else
				node.PrintTrace("Unknown test data category:");
		}
}



// Write out testdata as savegame into the saves directory.
bool TestData::InjectSavegame() const
{
	// Check if we have the required data to write out the savegame.
	if(dataSetName.empty() || sourceDataFile.empty())
		return false;
	
	// Open the source-file and scan until we find the test-data
	// Then scan for the contents keyword
	// Then write out the complete contents to the target file
	DataFile sourceData{sourceDataFile};
	for(const DataNode &rootNode : sourceData)
		// Check if we have found our dataset
		if(rootNode.Size() > 1 && rootNode.Token(0) == "test-data" && rootNode.Token(1) == dataSetName)
		{
			// Scan for the contents tag
			for(const DataNode &dataNode : rootNode)
				if(dataNode.Token(0) == "contents")
				{
					// Savegame data gets written to the saves directory
					// in a file with dataSetName as filename and with a .txt
					// file extention.
					// Other test-data might be injected in a different way,
					// for example by direct loading into the relevant data
					// structures.
					DataWriter dataWriter(Files::Saves() + dataSetName + ".txt");
					for(const DataNode &child : dataNode)
						dataWriter.Write(child);

					// Data was found and written. We are done succesfully.
					return true;
				}

			// Content section was not found. (Should we just create an empty file here?)
			return false;
		}
	
	// Data-section was no longer found.
	return false;
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
