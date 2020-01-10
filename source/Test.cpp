/* Test.cpp
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Test.h"

#include "PlayerInfo.h"
#include "UI.h"

using namespace std;



void Test::Load(const DataNode &node)
{
	if(node.Size() < 2)
	{
		node.PrintTrace("No name specified for test");
		return;
	}
	// If a test object is "loaded" twice, that is most likely an error (e.g.
	// due to a plugin containing a test with the same name as the base game
	// or another plugin). Tests should be globally unique.
	if(!name.empty())
	{
		node.PrintTrace("Duplicate definition of test");
		return;
	}
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "status" && child.Size() >= 2)
		{
			if (child.Token(1) == "Active")
				status = STATUS_ACTIVE;
			else if (child.Token(1) == "Known Failure")
				status = STATUS_KNOWN_FAILURE;
			else if (child.Token(1) == "Missing Feature")
				status = STATUS_MISSING_FEATURE;
		}
		else if (child.Token(0) == "sequence")
			for (const DataNode &seqChild : child)
			{
				testSteps.emplace_back(TestStep(seqChild));
			}
	}
}



const string &Test::Name() const
{
	return name;
}



string Test::StatusText() const
{
	switch (status)
	{
		case Test::STATUS_KNOWN_FAILURE:
			return "KNOWN FAILURE";
		case Test::STATUS_MISSING_FEATURE:
			return "MISSING FEATURE";
		case Test::STATUS_ACTIVE:
		default:
			return "ACTIVE";
	}
}



const vector<TestStep> &Test::TestSteps() const
{
	return testSteps;
}
