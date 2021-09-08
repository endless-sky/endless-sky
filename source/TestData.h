/* TestData.h
Copyright (c) 2019-2020 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ENDLESS_SKY_AC_TESTDATA_H_
#define ENDLESS_SKY_AC_TESTDATA_H_

#include "DataNode.h"

#include <string>



// Class representing a dataset for automated testing
class TestData {
public:
	void Load(const DataNode &node);
	// Function to inject the test-data into the game or into the games
	// environment.
	bool Inject() const;

	// Types of datafiles that can be stored.
	enum class Type {UNSPECIFIED, SAVEGAME};



private:
	// Writes out testdata as savegame file.
	bool InjectSavegame() const;



private:
	// Type of the dataset
	Type dataSetType = Type::UNSPECIFIED;
	// Node containing the test-data
	DataNode node;
};

#endif
