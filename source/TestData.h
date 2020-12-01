/* TestData.h
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ENDLESS_SKY_AC_TESTDATA_H_
#define ENDLESS_SKY_AC_TESTDATA_H_

#include <string>

class DataNode;



// Class representing a dataset for automated testing
class TestData {
public:
	const std::string &Name() const;
	void Load(const DataNode &node, const std::string &sourceDataFilePath);
	// Function to inject the test-data into the game or into the games
	// environment. Savegames would be written as file.
	bool Inject() const;

	// Types of datafiles that can be stored.
	enum DataType {UNSPECIFIED, SAVEGAME};
	
private:
	// Name of the dataset
	std::string dataSetName;
	// Type of the dataset
	DataType dataSetType = DataType::UNSPECIFIED;
	// File containing the test-data
	std::string sourceDataFile;
};

#endif
