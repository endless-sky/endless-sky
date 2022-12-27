/* test_randomevent.cpp
Copyright (c) 2022 by an anonymous author

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
#include "../../../source/RandomEvent.h"

// Include a helper for making conditions
#include "condition-tools.h"

// ... and any system includes needed for the test file.
#include <memory>
#include <string>
#include <vector>

namespace { // test namespace
// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.

// #endregion mock data

typedef RandomEvent<std::string, int> RandomEventType;
typedef RandomEvent<std::string, Condition<int>> ConditionalEventType;
const int minimumPeriod = RandomEventType::MinimumPeriod();

// #region unit tests
TEST_CASE( "RandomEvent Basics", "[RandomEvent]" ) {
	using T = RandomEventType;
	SECTION( "Class Traits" ) {
		CHECK_FALSE( std::is_trivial<T>::value );
		CHECK( std::is_nothrow_destructible<T>::value );
		CHECK( std::is_trivially_destructible<T>::value );
	}
	SECTION( "Construction Traits" ) {
		CHECK_FALSE( std::is_default_constructible<T>::value );
		CHECK_FALSE( std::is_trivially_default_constructible<T>::value );
		CHECK_FALSE( std::is_nothrow_default_constructible<T>::value );
		CHECK( std::is_copy_constructible<T>::value );
		CHECK( std::is_trivially_copy_constructible<T>::value );
		CHECK( std::is_nothrow_copy_constructible<T>::value );
		CHECK( std::is_move_constructible<T>::value );
		CHECK( std::is_trivially_move_constructible<T>::value );
		CHECK( std::is_nothrow_move_constructible<T>::value );
	}
	SECTION( "Copy Traits" ) {
		CHECK( std::is_copy_assignable<T>::value );
		CHECK( std::is_trivially_copyable<T>::value );
		CHECK( std::is_trivially_copy_assignable<T>::value );
		CHECK( std::is_nothrow_copy_assignable<T>::value );
	}
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable<T>::value );
		CHECK( std::is_trivially_move_assignable<T>::value );
		CHECK( std::is_nothrow_move_assignable<T>::value );
	}
}

SCENARIO( "Creating a RandomEvent", "[RandomEvent]") {
	const std::string eventString("test");
	GIVEN( "When created with zero period and not specifying overrideMinimum" ) {
		RandomEventType event(&eventString, 0, false);
		THEN( "the period should be the minimum value" ) {
			CHECK( event.Period() == minimumPeriod );
			CHECK( event.Get() == &eventString );
			CHECK_FALSE( event.HasConditions() );
		}
	}
	GIVEN( "When created zero period and overrideMinimum=false" ) {
		RandomEventType event(&eventString, 0, false);
		THEN( "the period should be the minimum value" ) {
			CHECK( event.Period() == minimumPeriod );
			CHECK( event.Get() == &eventString );
			CHECK_FALSE( event.HasConditions() );
		}
	}
	GIVEN( "When created with zero period and overrideMinimum=true" ) {
		RandomEventType event(&eventString, 0, true);
		THEN( "the period should be zero" ) {
			CHECK( event.Period() == 0 );
			CHECK( event.Get() == &eventString );
			CHECK_FALSE( event.HasConditions() );
		}
	}
	GIVEN( "When created with negative period with overrideMinimum=true" ) {
		RandomEventType event(&eventString, -131, true);
		THEN( "the period should be zero" ) {
			CHECK( event.Period() == 0 );
			CHECK( event.Get() == &eventString );
			CHECK_FALSE( event.HasConditions() );
		}
	}
	GIVEN( "When created with higher than the minimum period" ) {
		RandomEventType event(&eventString, minimumPeriod * 2);
		THEN( "the period should be the specified value" ) {
			CHECK( event.Period() == minimumPeriod * 2 );
			CHECK( event.Get() == &eventString );
			CHECK_FALSE( event.HasConditions() );
		}
	}
}

SCENARIO( "Creating a RandomEvent with a Condition period", "[RandomEvent][Condition]") {
	const std::string eventString("test");
	GIVEN( "When created with a missing period and overrideMinimum=true" ) {
		ConditionMaker vars;
		ConditionalEventType event(&eventString, vars.AsCondition("period"), true);
		THEN( "the period should be zero" ) {
			CHECK( event.Period() == 0 );
			CHECK( event.Period().Key() == "period" );
			CHECK( event.Get() == &eventString );
			CHECK( event.HasConditions() );
		}
		AND_WHEN( "setting to a valid period via UpdateConditions" ) {
			vars.Set("period", minimumPeriod * 2);
			event.UpdateConditions(vars.Store());
			THEN( "the period should be the specified value" ) {
				CHECK( event.Period() == minimumPeriod * 2 );
				CHECK( event.Period().Key() == "period" );
				CHECK( event.Get() == &eventString );
				CHECK( event.HasConditions() );
			}
			AND_WHEN( "setting to a negative period via UpdateConditions" ) {
				vars.Set("period", -999);
				event.UpdateConditions(vars.Store());
				THEN( "the period should be zero" ) {
					CHECK( event.Period() == 0 );
					CHECK( event.Period().Key() == "period" );
					CHECK( event.Get() == &eventString );
					CHECK( event.HasConditions() );
				}
			}
		}
	}
	GIVEN( "When created with a missing period and overrideMinimum=false" ) {
		ConditionMaker vars;
		ConditionalEventType event(&eventString, vars.AsCondition("period"), false);
		THEN( "the period should be the minimum value" ) {
			CHECK( event.Period() == minimumPeriod );
			CHECK( event.Period().Key() == "period" );
			CHECK( event.Get() == &eventString );
			CHECK( event.HasConditions() );
		}
		AND_WHEN( "setting to a valid period via UpdateConditions" ) {
			vars.Set("period", minimumPeriod*2);
			event.UpdateConditions(vars.Store());
			THEN( "the period should be the specified value" ) {
				CHECK( event.Period() == minimumPeriod * 2 );
				CHECK( event.Period().Key() == "period" );
				CHECK( event.Get() == &eventString );
				CHECK( event.HasConditions() );
			}
			AND_WHEN( "setting to a negative period via UpdateConditions" ) {
				vars.Set("period", -999);
				event.UpdateConditions(vars.Store());
				THEN( "the period should be the minimum value" ) {
					CHECK( event.Period() == minimumPeriod );
					CHECK( event.Period().Key() == "period" );
					CHECK( event.Get() == &eventString );
					CHECK( event.HasConditions() );
				}
			}
		}
	}
}
// #endregion unit tests



} // test namespace
