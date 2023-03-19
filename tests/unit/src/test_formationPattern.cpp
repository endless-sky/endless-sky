/* test_formationPattern.cpp
Copyright (c) 2021 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../../source/FormationPattern.h"

// Include a helper for creating well-formed DataNodes (to use for loading formations).
#include "datanode-factory.h"

// ... and any system includes needed for the test file.
#include <map>
#include <string>

namespace { // test namespace

// #region mock data
bool Near(const Point a, const Point b)
{
	if(!(a.Distance(b) == Approx(0.)))
	{
		if(a.Distance(b) < 0.001)
		{
			INFO("Distance under 0.001, but beyond Approx range!");
			return true;
		}
		return false;
	}
	return true;
}



std::string formation_empty =
R"(formation "Empty"
)";

std::string formation_delta_tail_px =
R"'(formation "Delta Tail (px)"
	position -100 200
	position 100 200
	position 200 400
	position 0 400
	position -200 400
	position -300 600
	position -100 600
	position 100 600
	position 300 600
)'";

std::string formation_tail_px_point =
R"'(formation "Tail (px point)"
	position -100 0
	position -200 0
	position -300 0
	position -400 0
	position -500 0
	position -600 0
	position -700 0
	position -800 0
)'";

// #endregion mock data



// #region unit tests
SCENARIO( "Loading and using of a formation pattern", "[formationPattern][Positioning]" ) {
	GIVEN( "a completely empty formation pattern" ) {
		auto emptyNode = AsDataNode(formation_empty);
		FormationPattern emptyFormation;
		emptyFormation.Load(emptyNode);
		REQUIRE( emptyFormation.Name() == "Empty");
		WHEN( "positions are requested") {
			auto it = emptyFormation.begin();
			THEN ( "all returned positions are near Point(0,0)" ) {
				CHECK( Near(*it, Point(0, 0)) );
				++it;
				CHECK( Near(*it, Point(0, 0)) );
				++it;
				CHECK( Near(*it, Point(0, 0)) );
				++it;
				CHECK( Near(*it, Point(0, 0)) );
			}
		}
	}
	GIVEN( "a formation pattern specified in points" ) {
		auto tailNode = AsDataNode(formation_tail_px_point);
		FormationPattern tailFormation;
		tailFormation.Load(tailNode);
		REQUIRE( tailFormation.Name() == "Tail (px point)");
		WHEN( "positions are requested") {
			auto it = tailFormation.begin();
			THEN ( "all returned positions are as expected" ) {
				CHECK( Near(*it, Point(-100, 0)) );
				++it;
				CHECK( Near(*it, Point(-200, 0)) );
				++it;
				CHECK( Near(*it, Point(-300, 0)) );
				++it;
				CHECK( Near(*it, Point(-400, 0)) );
				++it;
				CHECK( Near(*it, Point(-500, 0)) );
				++it;
				CHECK( Near(*it, Point(-600, 0)) );
				++it;
				CHECK( Near(*it, Point(-700, 0)) );
				++it;
				CHECK( Near(*it, Point(-800, 0)) );
			}
		}
	}
	GIVEN( "a formation pattern loaded in px" ) {
		auto delta_pxNode = AsDataNode(formation_delta_tail_px);
		FormationPattern delta_px;
		delta_px.Load(delta_pxNode);
		REQUIRE( delta_px.Name() == "Delta Tail (px)" );
		WHEN( "positions are requested") {
			THEN ( "the correct positions are calculated" ) {
				// No exact comparisons due to doubles, but we check if
				// the given points are very close to what they should be.
				auto it = delta_px.begin();
				REQUIRE( Near(*it, Point(-100, 200)) );
				++it;
				REQUIRE( Near(*it, Point(100, 200)) );
				++it;
				REQUIRE( Near(*it, Point(200, 400)) );
				++it;
				REQUIRE( Near(*it, Point(0, 400)) );
				++it;
				REQUIRE( Near(*it, Point(-200, 400)) );
				++it;
				REQUIRE( Near(*it, Point(-300, 600)) );
				++it;
				REQUIRE( Near(*it, Point(-100, 600)) );
				++it;
				REQUIRE( Near(*it, Point(100, 600)) );
				++it;
				REQUIRE( Near(*it, Point(300, 600)) );
			}
		}
	}
}
// #endregion unit tests


} // test namespace
