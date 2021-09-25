/* test_dictionary.cpp
Copyright (c) 2021 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/Dictionary.h"

// Include helpers here (if needed).

// ... and any system includes needed for the test file.
#include <map>
#include <string>

namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Having an empty Dictionary" , "[Dictionary][Empty]" ) {
	Dictionary emptyA;
	GIVEN( "A second empty Dictionary" )
	{
		Dictionary emptyB;
		THEN( "They should compare as equal" )
		{
			CHECK(emptyA.Equals(emptyB));
		}
	}
	GIVEN( "A second non-empty Dictionary" )
	{
		Dictionary fullC;
		fullC["dataEntry"] = 5.0;
		THEN( "They should compare as not-equal")
		{
			CHECK_FALSE(emptyA.Equals(fullC));
			CHECK_FALSE(fullC.Equals(emptyA));
		}
	}
}

SCENARIO( "Having an non-empty Dictionary" , "[Dictionary][NonEmpty}" ) {
	Dictionary fullA;
	fullA["dataOne"] = 6.0;
	fullA["zIndex"] = -0.2;
	fullA["abstract"] = 1.3;
	GIVEN( "A second Dictionary with the same content" )
	{
		Dictionary fullB;
		fullB["abstract"] = 1.3;
		fullB["dataOne"] = 6.0;
		fullB["zIndex"] = -0.2;
		THEN( "They should compare as equal" )
		{
			CHECK(fullA.Equals(fullB));
		}
	}
	GIVEN( "A second Dictionary with a different key" )
	{
		Dictionary fullB;
		fullB["abstract"] = 1.3;
		fullB["dataOne"] = 6.0;
		fullB["yIndex"] = -0.2;
		THEN( "They should compare as not-equal" )
		{
			CHECK_FALSE(fullA.Equals(fullB));
			CHECK_FALSE(fullB.Equals(fullA));
		}
	}
	GIVEN( "A second Dictionary with a different value" )
	{
		Dictionary fullB;
		fullB["abstract"] = 1.3;
		fullB["dataOne"] = 12.0;
		fullB["zIndex"] = -0.2;
		THEN( "They should compare as not-equal" )
		{
			CHECK_FALSE(fullA.Equals(fullB));
			CHECK_FALSE(fullB.Equals(fullA));
		}
	}
}
// #endregion unit tests



} // test namespace
