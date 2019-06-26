/* Test.h
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TESTDATA_H_
#define TESTDATA_H_

#include <string>

// Class representing a dataset for automated testing
class TestData {
public:
	TestData();
	virtual std::string Name();
	virtual void Load(const DataNode &node, const std::string sourceDataFilePath);
	// Function to inject the test-data into the game or into the games
	// environment. Savegames would be written as file.
	virtual bool Inject() const;
	
	virtual ~TestData();
	
	// Types of datafiles that can be stored.
	static const int SAVEGAME = 1;
	
private:
	// Name of the dataset
	std::string dataSetName;
	// Type of the dataset
	int dataSetType;
	// File containing the test-data
	std::string sourceDataFile;
};

#endif
