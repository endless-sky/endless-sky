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



// Test code goes here. Preferably, use scenario-driven language making use of the SCENARIO, GIVEN,
// WHEN, and THEN macros. (There will be cases where the more traditional TEST_CASE and SECTION macros
// are better suited to declaration of the public API.)

// When writing assertions, prefer the CHECK and CHECK_FALSE macros when probing the scenario, and prefer
// the REQUIRE / REQUIRE_FALSE macros for fundamental / "validity" assertions. If a CHECK fails, the rest
// of the block's statements will still be evaluated, but a REQUIRE failure will exit the current block.

// #endregion unit tests



} // test namespace
