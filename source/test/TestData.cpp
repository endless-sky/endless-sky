/* TestData.cpp
Copyright (c) 2019-2020 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "TestData.h"

#include "../DataFile.h"
#include "../DataNode.h"
#include "../DataWriter.h"
#include "../Files.h"
#include "../GameData.h"
#include "../Mission.h"
#include "../UniverseObjects.h"

using namespace std;



const string &TestData::Name() const
{
	return dataSetName;
}



// Loader to load the generic test-data entry
void TestData::Load(const DataNode &node, const filesystem::path &sourceDataFilePath)
{
	sourceDataFile = sourceDataFilePath;
	if(node.Size() < 2)
	{
		node.PrintTrace("Unnamed test data:");
		return;
	}
	if(node.Token(0) != "test-data")
	{
		node.PrintTrace("Unsupported root node:");
		return;
	}
	dataSetName = node.Token(1);

	for(const DataNode &child : node)
		// Only need to parse the category for now. The contents will be
		// scanned for at write-out of the test-data.
		if(child.Token(0) == "category" && child.Size() >= 2)
		{
			if(child.Token(1) == "savegame")
				dataSetType = Type::SAVEGAME;
			else if(child.Token(1) == "mission")
				dataSetType = Type::MISSION;
			else
				child.PrintTrace("Skipping unsupported category:");
		}
}



// Inject the test-data to the proper location.
bool TestData::Inject(const ConditionsStore *playerConditions,
	const set<const System *> *visitedSystems, const set<const Planet *> *visitedPlanets) const
{
	// Check if we have the required data to inject.
	if(dataSetName.empty() || sourceDataFile.empty())
		return false;

	// Determine data-type and call the relevant function.
	switch(dataSetType)
	{
		case Type::SAVEGAME:
			return InjectSavegame();
		case Type::MISSION:
			return InjectMission(playerConditions, visitedSystems, visitedPlanets);
		default:
			return false;
	}
}



const DataNode *TestData::GetContentsNode(const DataFile &sourceData) const
{
	for(const DataNode &rootNode : sourceData)
		// Check if we have found our dataset.
		if(rootNode.Size() > 1 && rootNode.Token(0) == "test-data" && rootNode.Token(1) == dataSetName)
			// Scan for the contents tag
			for(const DataNode &dataNode : rootNode)
				if(dataNode.Token(0) == "contents")
					return &dataNode;
	return nullptr;
}



// Write out testdata as savegame into the saves directory.
bool TestData::InjectSavegame() const
{
	const DataFile sourceData(sourceDataFile);
	// Get the contents node in the test data.
	const auto &nodePtr = GetContentsNode(sourceData);
	if(!nodePtr)
		return false;
	const DataNode &dataNode = *nodePtr;
	// Then write out the complete contents to the target file
	// Savegame data is written to the saves directory. Other test data
	// types might be injected differently, e.g. direct object loading.
	DataWriter dataWriter(Files::Saves() / (dataSetName + ".txt"));
	for(const DataNode &child : dataNode)
		dataWriter.Write(child);

	// Data was found and written. We are done successfully.
	return true;
}



bool TestData::InjectMission(const ConditionsStore *playerConditions,
	const set<const System *> *visitedSystems, const set<const Planet *> *visitedPlanets) const
{
	const DataFile sourceData(sourceDataFile);
	// Get the contents node in the test data.
	const auto &nodePtr = GetContentsNode(sourceData);
	if(!nodePtr)
		return false;

	const DataNode &dataNode = *nodePtr;
	for(const DataNode &node : dataNode)
		if(node.Token(0) == "mission" && node.Size() > 1)
			GameData::Objects().missions.Get(node.Token(1))->Load(node, playerConditions, visitedSystems, visitedPlanets);

	return true;
}
