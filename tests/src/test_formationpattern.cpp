/* test_formationpattern.cpp
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
#include "../../source/FormationPattern.h"

// Include a helper for creating well-formed DataNodes (to use for loading formations).
#include "datanode-factory.h"

// ... and any system includes needed for the test file.
#include <map>
#include <string>


namespace { // test namespace

// #region mock data

std::string formation_delta_tail_px = std::string("") +
"formation \"Delta Tail (px)\"\n" +
"	flippable y\n" +
"	line\n" +
"		start -100 200\n" +
"		end 100 200\n" +
"		slots 2\n" +
"		centered\n" +
"		repeat\n" +
"			start -100 200\n" +
"			end 100 200\n" +
"			alternating\n" +
"			slots 1\n";

// #endregion mock data



// #region unit tests
SCENARIO( "Loading and using of a formation pattern", "[formationPattern][Positioning]" ) {
	GIVEN( "a pattern loaded in px" ) {
		auto delta_pxNode = AsDataNode(formation_delta_tail_px);
		FormationPattern delta_px;
		delta_px.Load(delta_pxNode);
		REQUIRE( delta_px.Name() == "Delta Tail (px)" );
		WHEN( "positions are requested") {
			FormationPattern::ActiveFormation af;
			THEN ( "the correct positions are calculated when nr of ships is unknown" ) {
				// No exact comparisons due to doubles, but we check if
				// the given points are very close to what they should be.
				auto it = delta_px.begin(af);
				REQUIRE( (*it).Distance(Point(-100, 200)) < 0.01 );
				CHECK( it.Ring() == 0 );
				++it;
				REQUIRE( (*it).Distance(Point(100, 200)) < 0.01 );
				CHECK( it.Ring() == 0 );
				++it;
				REQUIRE( (*it).Distance(Point(200, 400)) < 0.01 );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( (*it).Distance(Point(0, 400)) < 0.01 );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( (*it).Distance(Point(-200, 400)) < 0.01 );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( (*it).Distance(Point(-300, 600)) < 0.01 );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( (*it).Distance(Point(-100, 600)) < 0.01 );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( (*it).Distance(Point(100, 600)) < 0.01 );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( (*it).Distance(Point(300, 600)) < 0.01 );
				CHECK( it.Ring() == 2 );
			}
			THEN ( "the correct positions are calculated when nr of ships is known" ) {
				af.numberOfShips = 9;
				auto it = delta_px.begin(af);
				REQUIRE( (*it).Distance(Point(-100, 200)) < 0.01 );
				CHECK( it.Ring() == 0 );
				++it;
				REQUIRE( (*it).Distance(Point(100, 200)) < 0.01 );
				CHECK( it.Ring() == 0 );
				++it;
				REQUIRE( (*it).Distance(Point(200, 400)) < 0.01 );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( (*it).Distance(Point(0, 400)) < 0.01 );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( (*it).Distance(Point(-200, 400)) < 0.01 );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( (*it).Distance(Point(-300, 600)) < 0.01 );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( (*it).Distance(Point(-100, 600)) < 0.01 );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( (*it).Distance(Point(100, 600)) < 0.01 );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( (*it).Distance(Point(300, 600)) < 0.01 );
				CHECK( it.Ring() == 2 );
			}
		}
		WHEN( "there is one ship on a centered line" ) {
			FormationPattern::ActiveFormation af;
			af.numberOfShips = 1;
			THEN ( "it is in the center spot on odd lines" ) {
				auto it = delta_px.begin(af, 3);
				REQUIRE ( it.Ring() == 3 );
				CHECK( (*it).Distance(Point(0, 800)) < 0.01 );
			}
			THEN ( "it is near the center on even lines" ) {
				auto it = delta_px.begin(af, 4);
				REQUIRE ( it.Ring() == 4 );
				// X can be left of center or right of center at 100 pixels,
				// or can be in the exact center (depending on implementation).
				// We just allow all possible implementations in the test here.
				CHECK( (it->X() < 0.01 || (abs(it->X()) - 100) < 0.01 ) );
				CHECK( (it->Y() - 1000) < 0.01 );
			}
		}
	}
}
// #endregion unit tests


} // test namespace
