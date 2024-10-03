/* test_ship.cpp
Copyright (c) 2021 by Benjamin Hauch

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
#include "../../../source/Ship.h"

// Include a helper for creating well-formed DataNodes.
#include "datanode-factory.h"

// ... and any system includes needed for the test file.
#include <memory>
#include <string>
#include <type_traits>

namespace { // test namespace

// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.

// #endregion mock data



// #region unit tests
SCENARIO( "Creating a Ship::Bay instance", "[ship][bay]" ) {
	// No default constructor.
	REQUIRE_FALSE( std::is_default_constructible_v<Ship::Bay> );

	GIVEN( "a reference position" ) {
		auto bay = Ship::Bay(20., 40., "Fighter");
		THEN( "the position is scaled by 50%" ) {
			CHECK_THAT( bay.point.X(), Catch::Matchers::WithinAbs(10., 0.0001) );
			CHECK_THAT( bay.point.Y(), Catch::Matchers::WithinAbs(20., 0.0001) );
		}
	}
	GIVEN( "a category string" ) {
		std::string value = "any string value";
		auto bay = Ship::Bay(0., 0., value);
		THEN( "the category is stored" ) {
			CHECK( bay.category == value );
		}
	}
}

SCENARIO( "A Ship::Bay instance is being copied", "[ship][bay]") {
	auto source = Ship::Bay(-10., 10., "Fighter");
	GIVEN( "the bay is occupied" ) {
		auto occupant = std::make_shared<Ship>();
		REQUIRE( occupant );
		source.ship = occupant;
		REQUIRE( source.ship );
		WHEN( "the copy is made via ctor" ) {
			Ship::Bay copy(source);
			THEN( "the copy has the correct attributes" ) {
				CHECK( copy.point.X() == source.point.X() );
				CHECK( copy.point.Y() == source.point.Y() );
				CHECK( copy.category == source.category );
				CHECK( copy.side == source.side );
				CHECK( copy.facing.Degrees() == source.facing.Degrees() );
				CHECK( copy.launchEffects == source.launchEffects );
			}
			THEN( "the copy is unoccupied" ) {
				CHECK_FALSE( copy.ship );
			}
			THEN( "the source is still occupied" ) {
				CHECK( source.ship == occupant );
			}
		}
		WHEN( "the copy is made via assignment" ) {
			Ship::Bay copy = source;
			THEN( "the copy has the correct attributes" ) {
				CHECK( copy.point.X() == source.point.X() );
				CHECK( copy.point.Y() == source.point.Y() );
				CHECK( copy.category == source.category );
				CHECK( copy.side == source.side );
				CHECK( copy.facing.Degrees() == source.facing.Degrees() );
				CHECK( copy.launchEffects == source.launchEffects );
			}
			THEN( "the copy is unoccupied" ) {
				CHECK_FALSE( copy.ship );
			}
			THEN( "the source is still occupied" ) {
				CHECK( source.ship == occupant );
			}
		}
	}
}
// Constructing useful Ship instances requires Ship::Load, which requires all of GameData & runtime deps.
SCENARIO("A Ship::Ship instance", "[ship][ship]") {

	// Ship example DataNode
	DataNode ship_node = AsDataNode(
"ship \"Aerie\"\n\
\tsprite \"ship/aerie\"\n\
\tthumbnail \"thumbnail/aerie\"\n\
\tattributes\n\
\t\tcategory \"Medium Warship\"\n\
\t\t\"cost\" 3500000\n\
\t\t\"shields\" 5700\n\
\t\t\"hull\" 1900\n\
\t\t\"required crew\" 10\n\
\t\t\"bunks\" 28\n\
\t\t\"mass\" 390\n\
\t\t\"drag\" 6.15\n\
\t\t\"heat dissipation\" .47\n\
\t\t\"fuel capacity\" 500\n\
\t\t\"cargo space\" 50\n\
\t\t\"outfit space\" 390\n\
\t\t\"weapon capacity\" 150\n\
\t\t\"engine capacity\" 95\n\
\t\tweapon\n\
\t\t\t\"blast radius\" 80\n\
\t\t\t\"shield damage\" 800\n\
\t\t\t\"hull damage\" 400\n\
\t\t\t\"hit force\" 1200\n\
\n\
\tengine 15 97\n\
\tengine - 15 97\n\
\tleak \"leak\" 50 50\n\
\tleak \"flame\" 50 80\n\
\tleak \"big leak\" 90 30\n\
\texplode \"tiny explosion\" 10\n\
\texplode \"small explosion\" 25\n\
\texplode \"medium explosion\" 25\n\
\texplode \"large explosion\" 10\n\
\t\"final explode\" \"final explosion medium\"\n\
\tdescription \"The Lionheart Aerie is a light carrier, designed to be just big enough for \
two fighter bays plus a decent armament of its own. Variations on this same ship design have been \
in use in the Deep for almost half a millennium, but this model comes with the very latest \
in generator and weapon technology.\""
	);
	GIVEN( "ship tests" ) {
		auto ship = Ship(ship_node);
		ship.FinishLoading(true);

		WHEN( "a basic ship constructor" ) {
			THEN( "check ship names" ) {
				CHECK( ship.TrueModelName() == "Aerie" );
				CHECK( ship.DisplayModelName() == "Aerie" );
				CHECK( ship.DisplayModelName(true) == "Aerie (MW)" );
				CHECK( ship.PluralModelName() == "Aeries");
				CHECK( ship.CategoryCode() == "MW" );
			}
		}
	}
}


// Test code goes here. Preferably, use scenario-driven language making use of the SCENARIO, GIVEN,
// WHEN, and THEN macros. (There will be cases where the more traditional TEST_CASE and SECTION macros
// are better suited to declaration of the public API.)

// When writing assertions, prefer the CHECK and CHECK_FALSE macros when probing the scenario, and prefer
// the REQUIRE / REQUIRE_FALSE macros for fundamental / "validity" assertions. If a CHECK fails, the rest
// of the block's statements will still be evaluated, but a REQUIRE failure will exit the current block.

// #endregion unit tests



} // test namespace
