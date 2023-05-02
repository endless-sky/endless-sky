/* TestData.h
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

#ifndef ENDLESS_SKY_AC_TESTDATA_H_
#define ENDLESS_SKY_AC_TESTDATA_H_

#include <string>

class DataFile;
class DataNode;



// Class representing a dataset for automated testing
class TestData {
public:
	const std::string &Name() const;
	void Load(const DataNode &node, const std::string &sourceDataFilePath);
	// Function to inject the test-data into the game or into the games
	// environment.
	bool Inject() const;

	// Types of datafiles that can be stored.
	enum class Type {UNSPECIFIED, SAVEGAME, MISSION};



private:
	const DataNode *GetContentsNode(const DataFile &sourceData) const;

	// Writes out testdata as savegame file.
	bool InjectSavegame() const;

	// Loads a mission stored in testdata into a Mission through GameData.
	bool InjectMission() const;


private:
	// Name of the dataset
	std::string dataSetName;
	// Type of the dataset
	Type dataSetType = Type::UNSPECIFIED;
	// File containing the test-data
	std::string sourceDataFile;
};

#endif
