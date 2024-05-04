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
		double diameterToPx = 0.;
		double widthToPx = 0.;
		double heightToPx = 0.;
		double centerBodyRadius = 0.;
		WHEN( "positions are requested") {
			auto it = emptyFormation.begin(diameterToPx, widthToPx, heightToPx, centerBodyRadius);
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
}
// #endregion unit tests


} // test namespace
