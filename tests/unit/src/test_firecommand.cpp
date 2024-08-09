/* test_firecommand.cpp
Copyright (c) 2021 by quyykk

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
#include "../../../source/FireCommand.h"

// ... and any system includes needed for the test file.

namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Creating a FireCommand instance", "[firecommand]") {
	GIVEN( "an instance" ) {
		FireCommand command;
		THEN( "it has the correct default properties" ) {
			CHECK_FALSE( command.IsFiring() );
			CHECK_FALSE( command.HasFire(0) );
			CHECK_FALSE( command.Aim(0) );
		}
	}
}

SCENARIO( "A FireCommand instance is being copied", "[firecommand]" ) {
	FireCommand command;
	GIVEN( "a specific bitset" ) {
		command.SetHardpoints(5);
		command.SetFire(0);
		command.SetFire(3);
		command.SetAim(2, 1.);

		WHEN( "the copy is made" ) {
			auto copy = command;
			THEN( "the copy has the correct properties" ) {
				CHECK( copy.IsFiring() == command.IsFiring() );
				CHECK( copy.HasFire(0) == command.HasFire(0) );
				CHECK( copy.HasFire(1) == command.HasFire(1) );
				CHECK( copy.HasFire(2) == command.HasFire(2) );
				CHECK( copy.HasFire(3) == command.HasFire(3) );
				CHECK( copy.HasFire(4) == command.HasFire(4) );
				CHECK_THAT( copy.Aim(0), Catch::Matchers::WithinAbs(command.Aim(0), 0.0001) );
				CHECK_THAT( copy.Aim(1), Catch::Matchers::WithinAbs(command.Aim(1), 0.0001) );
				CHECK_THAT( copy.Aim(2), Catch::Matchers::WithinAbs(command.Aim(2), 0.0001) );
				CHECK_THAT( copy.Aim(3), Catch::Matchers::WithinAbs(command.Aim(3), 0.0001) );
				CHECK_THAT( copy.Aim(4), Catch::Matchers::WithinAbs(command.Aim(4), 0.0001) );
			}
			THEN( "the two bitsets are independent" ) {
				command.SetAim(1, -1.);
				CHECK_THAT( command.Aim(1), Catch::Matchers::WithinAbs(-1, 0.0001) );
				CHECK_FALSE( copy.Aim(1) );

				copy.SetFire(4);
				CHECK_FALSE( command.HasFire(4) );
				CHECK( copy.HasFire(4) );
			}
		}
	}
}

SCENARIO( "A FireCommand instance is being used", "[firecommand]") {
	GIVEN( "an empty FireCommand" ) {
		FireCommand command;
		THEN( "resizing it works" ) {
			command.SetHardpoints(20);
			command.SetFire(0);
			command.SetFire(18);
			CHECK( command.HasFire(0) );
			CHECK( command.HasFire(18) );
		}
	}
	GIVEN( "a FireCommand of a specific size" ) {
		FireCommand command;
		command.SetHardpoints(10);

		AND_GIVEN( "an index is firing" ) {
			command.SetFire(0);
			command.SetFire(4);
			command.SetFire(9);

			REQUIRE( command.HasFire(0) );
			REQUIRE( command.HasFire(4) );
			REQUIRE( command.HasFire(9) );
			WHEN( "clear is called" ) {
				command.Clear();
				THEN( "the command is empty" ) {
					CHECK_FALSE( command.HasFire(0) );
					CHECK_FALSE( command.HasFire(4) );
					CHECK_FALSE( command.HasFire(9) );
				}
			}
		}
		AND_GIVEN( "an index is aiming" ) {
			command.SetAim(0, -1.);
			command.SetAim(4, 1.);
			command.SetAim(9, 1.);

			REQUIRE_THAT( command.Aim(0), Catch::Matchers::WithinAbs(-1., 0.0001) );
			REQUIRE_THAT( command.Aim(4), Catch::Matchers::WithinAbs(1., 0.0001) );
			REQUIRE_THAT( command.Aim(9), Catch::Matchers::WithinAbs(1., 0.0001) );
			WHEN( "clear is called" ) {
				command.Clear();
				THEN( "the command is empty" ) {
					CHECK_THAT( command.Aim(0), Catch::Matchers::WithinAbs(0., 0.0001) );
					CHECK_THAT( command.Aim(4), Catch::Matchers::WithinAbs(0., 0.0001) );
					CHECK_THAT( command.Aim(9), Catch::Matchers::WithinAbs(0., 0.0001) );
				}
			}
		}
	}
	GIVEN( "two non-empty FireCommands" ) {
		FireCommand one;
		one.SetHardpoints(4);
		one.SetFire(3);
		one.SetFire(2);
		CHECK( one.IsFiring() );

		FireCommand two;
		two.SetHardpoints(3);
		two.SetFire(1);
		CHECK( two.IsFiring() );

		THEN( "UpdateWith works" ) {
			two.UpdateWith(one);
			CHECK_FALSE( two.HasFire(0) );
			CHECK_FALSE( two.HasFire(1) );
			CHECK( two.HasFire(2) );
			CHECK( two.IsFiring() );
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
