/* TestStep.cpp
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ConditionSet.h"
#include "DataNode.h"
#include "TestStep.h"
#include <string>


TestStep::TestStep()
{
	// Initialize with some sensible default values.
	stepType = 0;
	saveGameName = "";
}




int TestStep::StepType()
{
	return stepType;
}



const std::string TestStep::SaveGameName()
{
	return saveGameName;
}



void TestStep::Load(const DataNode &node)
{
}
