/*
 * TestData.cpp
 * Copyright (c) 2019-2020 by Peter van der Meer
 *
 * Endless Sky is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "TestData.h"

#include "../DataFile.h"
#include "../DataNode.h"
#include "../DataWriter.h"
#include "../Files.h"
#include "../GameData.h"
#include "../Mission.h"
#include "../UniverseObjects.h"

#include <string>
#include <utility>

using namespace std;



// Return the name of the test data set.
const string &TestData::Name() const
{
	return dataSetName;
}



// Load and parse the root node describing the test-data block.
//
// The dataSetName is the name of the test data set, used for referencing
// in error messages or injecting logic. The category identifies how we plan
// to handle or inject the data (e.g. "mission", "savegame").
void TestData::Load(const DataNode &node, const filesystem::path &sourceDataFilePath)
{
	sourceDataFile = sourceDataFilePath;

	if(node.Size() < 2)
	{
		node.PrintTrace("Error: Unnamed test data:");
		return;
	}
	if(node.Token(0) != "test-data")
	{
		node.PrintTrace("Error: Unsupported root node:");
		return;
	}

	dataSetName = node.Token(1);

	// Attempt to parse the "category <type>" line.
	for(const DataNode &child : node)
	{
		if(child.Size() > 1 && child.Token(0) == "category")
		{
			const string &cat = child.Token(1);
			if(cat == "savegame")
				dataSetType = Type::SAVEGAME;
			else if(cat == "mission")
				dataSetType = Type::MISSION;
			else
				child.PrintTrace("Skipping unsupported category:");
		}
	}
}



// Attempt to inject the loaded test data into the game in the appropriate manner.
//
// Returns true on success, false otherwise.
bool TestData::Inject() const
{
	if(dataSetName.empty() || sourceDataFile.empty())
		return false;

	switch(dataSetType)
	{
	case Type::SAVEGAME:
		return InjectSavegame();
	case Type::MISSION:
		return InjectMission();
	default:
		return false;
	}
}



// Helper function to locate the "contents" sub-node for this data set inside a file.
const DataNode *TestData::GetContentsNode(const DataFile &sourceData) const
{
	for(const DataNode &rootNode : sourceData)
	{
		// Check if this block is our "test-data <Name>"
		if(rootNode.Size() > 1 && rootNode.Token(0) == "test-data"
		   && rootNode.Token(1) == dataSetName)
		{
			// Then scan for "contents" block
			for(const DataNode &dataNode : rootNode)
				if(dataNode.Token(0) == "contents")
					return &dataNode;
		}
	}
	return nullptr;
}



// If test-data is in the "savegame" category, we write its contents out to a
// file in the "saves" folder: e.g. "MyTestData.txt".
bool TestData::InjectSavegame() const
{
	DataFile sourceData(sourceDataFile);

	const DataNode *nodePtr = GetContentsNode(sourceData);
	if(!nodePtr)
		return false;

	const DataNode &contentsNode = *nodePtr;
	DataWriter dataWriter(Files::Saves() / (dataSetName + ".txt"));
	for(const DataNode &child : contentsNode)
		dataWriter.Write(child);

	return true;
}



// If test-data is in the "mission" category, we parse its "mission" lines
// as though they were loaded from data files. This modifies GameData mission objects.
bool TestData::InjectMission() const
{
	DataFile sourceData(sourceDataFile);

	const DataNode *nodePtr = GetContentsNode(sourceData);
	if(!nodePtr)
		return false;

	const DataNode &contentsNode = *nodePtr;
	for(const DataNode &node : contentsNode)
	{
		if(node.Token(0) == "mission" && node.Size() > 1)
		{
			const string &missionName = node.Token(1);
			// Access the existing mission data from GameData, then load from node
			if(auto *missionPtr = GameData::Objects().missions.Get(missionName))
				missionPtr->Load(node);
		}
	}
	return true;
}
