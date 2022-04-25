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


std::string formation_empty_by_skips =
R"(formation "Empty By Skips"
	line
		start -100 200
		end 100 200
		positions 2
			skip first
			skip last
		repeat
			start -100 200
			end 100 200
			alternating
)";

std::string formation_delta_tail_px =
R"'(formation "Delta Tail (px)"
	flippable y
	line
		start -100 200
		end 100 200
		positions 2
		centered
		repeat
			start -100 200
			end 100 200
			alternating
			"positions delta" 1
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

std::string formation_arc_px =
R"'(formation "Arc (px)"
	arc
		anchor 0 -100
		start 0 -237
		angle 180
		positions 3
		centered
)'";

std::string formation_polar_dimensions =
R"'(formation "Polar and Dimensions"
	position polar 90 287
	position polar diameter 270 3
	position polar 0 120
	position polar width 180 321
	position height 0 4
	position width 1 2
	position diameter -2 -2.5
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
		WHEN( "positions are requested") {
			auto it = emptyFormation.begin(diameterToPx, widthToPx, heightToPx);
			THEN ( "all returned positions are near Point(0,0) and on Ring 0" ) {
				CHECK( Near(*it, Point(0, 0)) );
				CHECK( it.Ring() == 0 );
				++it;
				CHECK( Near(*it, Point(0, 0)) );
				++it;
				CHECK( Near(*it, Point(0, 0)) );
				++it;
				CHECK( Near(*it, Point(0, 0)) );
				CHECK( it.Ring() == 0 );
			}
		}
	}
	GIVEN( "a formation pattern empty because of skipping" ) {
		auto emptyNode = AsDataNode(formation_empty_by_skips);
		FormationPattern emptyFormation;
		emptyFormation.Load(emptyNode);
		REQUIRE( emptyFormation.Name() == "Empty By Skips");
		double diameterToPx = 0.;
		double widthToPx = 0.;
		double heightToPx = 0.;
		WHEN( "positions are requested") {
			auto it = emptyFormation.begin(diameterToPx, widthToPx, heightToPx);
			THEN ( "all returned positions are near Point(0,0) and on Ring 0" ) {
				CHECK( Near(*it, Point(0, 0)) );
				CHECK( it.Ring() == 0 );
				++it;
				CHECK( Near(*it, Point(0, 0)) );
				++it;
				CHECK( Near(*it, Point(0, 0)) );
				++it;
				CHECK( Near(*it, Point(0, 0)) );
				CHECK( it.Ring() == 0 );
			}
		}
	}
	GIVEN( "a formation pattern specified in points" ) {
		auto tailNode = AsDataNode(formation_tail_px_point);
		FormationPattern tailFormation;
		tailFormation.Load(tailNode);
		REQUIRE( tailFormation.Name() == "Tail (px point)");
		double diameterToPx = 0.;
		double widthToPx = 0.;
		double heightToPx = 0.;
		WHEN( "positions are requested") {
			auto it = tailFormation.begin(diameterToPx, widthToPx, heightToPx);
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
		double diameterToPx = 0.;
		double widthToPx = 0.;
		double heightToPx = 0.;
		WHEN( "positions are requested") {
			THEN ( "the correct positions are calculated when nr of ships is unknown" ) {
				// No exact comparisons due to doubles, but we check if
				// the given points are very close to what they should be.
				auto it = delta_px.begin(diameterToPx, widthToPx, heightToPx);
				REQUIRE( Near(*it, Point(-100, 200)) );
				CHECK( it.Ring() == 0 );
				++it;
				REQUIRE( Near(*it, Point(100, 200)) );
				CHECK( it.Ring() == 0 );
				++it;
				REQUIRE( Near(*it, Point(200, 400)) );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( Near(*it, Point(0, 400)) );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( Near(*it, Point(-200, 400)) );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( Near(*it, Point(-300, 600)) );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( Near(*it, Point(-100, 600)) );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( Near(*it, Point(100, 600)) );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( Near(*it, Point(300, 600)) );
				CHECK( it.Ring() == 2 );
			}
			THEN ( "the correct positions are calculated when nr of ships is known" ) {
				unsigned int startingRing = 0;
				unsigned int shipsToPlace = 9;
				auto it = delta_px.begin(diameterToPx, widthToPx, heightToPx, startingRing, shipsToPlace);
				REQUIRE( Near(*it, Point(-100, 200)) );
				CHECK( it.Ring() == 0 );
				++it;
				REQUIRE( Near(*it, Point(100, 200)) );
				CHECK( it.Ring() == 0 );
				++it;
				REQUIRE( Near(*it, Point(200, 400)) );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( Near(*it, Point(0, 400)) );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( Near(*it, Point(-200, 400)) );
				CHECK( it.Ring() == 1 );
				++it;
				REQUIRE( Near(*it, Point(-300, 600)) );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( Near(*it, Point(-100, 600)) );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( Near(*it, Point(100, 600)) );
				CHECK( it.Ring() == 2 );
				++it;
				REQUIRE( Near(*it, Point(300, 600)) );
				CHECK( it.Ring() == 2 );
			}
		}
		WHEN( "there is one ship on a centered line" ) {
			unsigned int shipsToPlace = 1;
			THEN ( "it is in the center spot on odd lines" ) {
				unsigned int startingRing = 3;
				auto it = delta_px.begin(diameterToPx, widthToPx, heightToPx, startingRing, shipsToPlace);
				REQUIRE ( it.Ring() == 3 );
				CHECK( Near(*it, Point(0, 800)) );
			}
			THEN ( "it is near the center on even lines" ) {
				unsigned int startingRing = 4;
				auto it = delta_px.begin(diameterToPx, widthToPx, heightToPx, startingRing, shipsToPlace);
				REQUIRE ( it.Ring() == 4 );
				// X can be left of center or right of center at a distance of
				// 100 pixels, or can be in the exact center (depending on
				// implementation).
				// We just allow all those possible implementations in the test.
				CHECK(( it->X() == Approx(0.) || abs(it->X()) == Approx(100.) ));
				CHECK( it->Y() == Approx(1000.) );
			}
		}
		WHEN( "there are two ships on a centered line" ) {
			double diameterToPx = 0.;
			double widthToPx = 0.;
			double heightToPx = 0.;
			unsigned int startingRing = 2;
			unsigned int numberOfShips = 2;
			THEN ( "they are on the left and right spots near the center on even lines" ) {
				auto it = delta_px.begin(diameterToPx, widthToPx, heightToPx, startingRing, numberOfShips);
				CHECK( Near(*it, Point(-100, 600)) );
				++it;
				CHECK( Near(*it, Point(100, 600)) );
			}
		}
	}
	GIVEN( "a formation pattern with arcs" ) {
		auto formationNode = AsDataNode(formation_arc_px);
		FormationPattern formation;
		formation.Load(formationNode);
		REQUIRE( formation.Name() == "Arc (px)" );
		double diameterToPx = 0.;
		double widthToPx = 0.;
		double heightToPx = 0.;
		WHEN( "positions are requested") {
			THEN ( "the correct positions are calculated" ) {
				auto it = formation.begin(diameterToPx, widthToPx, heightToPx);
				REQUIRE( Near(*it, Point(0, -337)) );
				++it;
				REQUIRE( Near(*it, Point(237., -100.)) );
				++it;
				REQUIRE( Near(*it, Point(0., 137.)) );
			}
			THEN ( "the correct positions are calculated when nr of ships is set" ) {
				unsigned int startingRing = 0;
				unsigned int numberOfShips = 3;
				auto it = formation.begin(diameterToPx, widthToPx, heightToPx, startingRing, numberOfShips);
				REQUIRE( Near(*it, Point(0, -337)) );
				++it;
				REQUIRE( Near(*it, Point(237., -100.)) );
				++it;
				REQUIRE( Near(*it, Point(0., 137.)) );
			}
			THEN ( "the middle coordinate is given when only 1 ship is on the centered arc" ) {
				unsigned int startingRing = 0;
				unsigned int numberOfShips = 1;
				auto it = formation.begin(diameterToPx, widthToPx, heightToPx, startingRing, numberOfShips);
				REQUIRE( Near(*it, Point(237., -100.)) );
			}
		}
	}
	GIVEN( "a formation pattern with polar coordinates and ship-dimension coordinates" ) {
		auto formationNode = AsDataNode(formation_polar_dimensions);
		FormationPattern formation;
		formation.Load(formationNode);
		REQUIRE( formation.Name() == "Polar and Dimensions" );
		double diameterToPx = 140.;
		double widthToPx = 80.;
		double heightToPx = 60.;
		WHEN( "positions are requested") {
			THEN ( "the correct positions are calculated when nr of ships is unknown" ) {
				auto it = formation.begin(diameterToPx, widthToPx, heightToPx);
				REQUIRE( Near(*it, Point(287, 0)) );
				++it;
				REQUIRE( Near(*it, Point(-diameterToPx * 3, 0)) );
				++it;
				REQUIRE( Near(*it, Point(0, -120)) );
				++it;
				REQUIRE( Near(*it, Point(0, widthToPx * 321)) );
				++it;
				REQUIRE( Near(*it, Point(0, heightToPx * 4)) );
				++it;
				REQUIRE( Near(*it, Point(widthToPx * 1, widthToPx * 2)) );
				++it;
				REQUIRE( Near(*it, Point(diameterToPx * -2, diameterToPx * -2.5)) );
			}
		}
	}
}
// #endregion unit tests


} // test namespace
