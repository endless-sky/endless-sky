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
#include "../../../source/Condition.h"

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

const int intValue = 1;
const double value = intValue;
const double otherValue = 2.0;
const double tinyValue = 1e-30;
const std::string key = "key";
const std::string otherKey = "anotherkey";

// For validation checks
const double badValue = -1.0;
const double goodValue = 1.0;
const double anotherGoodValue = 2.0;
bool Validate(double d)
{
	return d >= 0;
}

// #endregion mock data

// #region unit tests
TEST_CASE( "Condition Basics", "[Condition]" ) {
	using T = Condition<double>;
	SECTION( "Class Traits" ) {
		CHECK_FALSE( std::is_trivial<T>::value );
		CHECK( std::is_nothrow_destructible<T>::value );
		CHECK_FALSE( std::is_trivially_destructible<T>::value );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible<T>::value );
		CHECK_FALSE( std::is_trivially_default_constructible<T>::value );
		CHECK_FALSE( std::is_nothrow_default_constructible<T>::value );
		CHECK( std::is_copy_constructible<T>::value );
		CHECK_FALSE( std::is_trivially_copy_constructible<T>::value );
		CHECK_FALSE( std::is_nothrow_copy_constructible<T>::value );
		CHECK( std::is_move_constructible<T>::value );
		CHECK_FALSE( std::is_trivially_move_constructible<T>::value );
		CHECK_FALSE( std::is_nothrow_move_constructible<T>::value );
	}
	SECTION( "Copy Traits" ) {
		CHECK( std::is_copy_assignable<T>::value );
		CHECK_FALSE( std::is_trivially_copyable<T>::value );
		CHECK_FALSE( std::is_trivially_copy_assignable<T>::value );
		CHECK_FALSE( std::is_nothrow_copy_assignable<T>::value );
	}
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable<T>::value );
		CHECK_FALSE( std::is_trivially_move_assignable<T>::value );
		CHECK_FALSE( std::is_nothrow_move_assignable<T>::value );
	}
}

SCENARIO( "Creating a Condition" , "[Condition][Creation]" ) {
	GIVEN( "a Condition<double>" ) {
		WHEN( "it is default initialized" ) {
			Condition<double> condition;
			THEN( "the contents should be empty" ) {
				CHECK( condition.Value() == 0 );
				CHECK( condition.Key().empty() );
				CHECK( condition.IsLiteral() );
				CHECK_FALSE( condition.HasConditions() );
				CHECK_FALSE( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == 0);
				CHECK( static_cast<double>(condition) == condition.Value());
			}
			THEN( "it should have the same origin as itself" ) {
				CHECK( condition.SameOrigin(condition) );
			}
			AND_WHEN( "UpdateConditions is called" ) {
				ConditionMaker vars({ { otherKey, otherValue } });
				condition.UpdateConditions(vars.Store());
				THEN( "the key and value should not change" ) {
					CHECK( condition.Value() == 0 );
					CHECK( condition.IsLiteral() );
					CHECK_FALSE( condition.HasConditions() );
					CHECK( condition.Key().empty() );
				}
			}
		}
		WHEN( "it is initialized with a literal" ) {
			Condition<double> condition(value);
			THEN( "it should have that value but no key" ) {
				CHECK( condition.Value() == value );
				CHECK( condition.Key().empty() );
				CHECK( condition.IsLiteral() );
				CHECK_FALSE( condition.HasConditions() );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == value);
				CHECK( static_cast<double>(condition) == condition.Value());
			}
			AND_WHEN( "SameOrigin() is called with a Condition that has a key" ) {
				Condition<double> named(value, key);
				THEN( "the result should be false" ) {
					CHECK( named.Value() == condition.Value() );
					CHECK_FALSE( named.IsLiteral() );
					CHECK_FALSE( condition.SameOrigin(named) );
					CHECK_FALSE( named.SameOrigin(condition) );
				}
				THEN( "it should have the same origin as itself" ) {
					CHECK( condition.SameOrigin(condition) );
				}
			}
			AND_WHEN( "SameOrigin() is called with a Condition that has the same value and no key" ) {
				Condition<double> same(value);
				THEN( "the result should be true" ) {
					CHECK( same.IsLiteral() );
					CHECK( condition.SameOrigin(same) );
					CHECK( same.SameOrigin(condition) );
				}
			}
			AND_WHEN( "SameOrigin() is called with a Condition that has a different value and no key" ) {
				Condition<double> other(otherValue);
				THEN( "the result should be false" ) {
					CHECK( other.IsLiteral() );
					CHECK_FALSE( condition.SameOrigin(other) );
					CHECK_FALSE( other.SameOrigin(condition) );
				}
			}
			AND_WHEN( "UpdateConditions is called" ) {
				ConditionMaker vars({ { otherKey, otherValue } });
				condition.UpdateConditions(vars.Store());
				THEN( "the key and value should not change" ) {
					CHECK( condition.Value() == value );
					CHECK( condition.IsLiteral() );
					CHECK_FALSE( condition.HasConditions() );
					CHECK( condition.Key().empty() );
				}
			}
		}
		WHEN( "it is initialized with a key and value" ) {
			Condition<double> condition(value, key);
			THEN( "it should have that value but no key" ) {
				CHECK( condition.Value() == value );
				CHECK( condition.Key() == key );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == value);
				CHECK( static_cast<double>(condition) == condition.Value());
			}
			THEN( "it should have the same origin as itself" ) {
				CHECK( condition.SameOrigin(condition) );
			}
			AND_WHEN( "SameOrigin() is called with a Condition that has the same name but a different value" ) {
				Condition<double> other(otherValue, key);
				THEN( "the result should be true" ) {
					CHECK( other.Value() == otherValue );
					CHECK( condition.SameOrigin(other) );
					CHECK( other.SameOrigin(condition) );
				}
			}
			AND_WHEN( "SameOrigin() is called with a Condition that has a different name but the same value" ) {
				Condition<double> other(condition.Value(), "notkey");
				THEN( "the result should be true" ) {
					CHECK( other.Key() == "notkey" );
					CHECK_FALSE( condition.SameOrigin(other) );
					CHECK_FALSE( other.SameOrigin(condition) );
				}
			}
			AND_WHEN( "UpdateConditions is called without that key" ) {
				ConditionMaker vars({ { otherKey, otherValue } });
				condition.UpdateConditions(vars.Store());
				THEN( "the key and value should not change" ) {
					CHECK( condition.Value() == value );
					CHECK_FALSE( condition.IsLiteral() );
					CHECK( condition.HasConditions() );
					CHECK( condition.Key() == key );
				}
			}
			AND_WHEN( "UpdateConditions is called with that key" ) {
				ConditionMaker vars({ { key, otherValue } });
				condition.UpdateConditions(vars.Store());
				THEN( "the key should not change" ) {
					CHECK( condition.Key() == key );
					CHECK_FALSE( condition.IsLiteral() );
					CHECK( condition.HasConditions() );
				}
				THEN( "the value should change" ) {
					CHECK( condition.Value() == otherValue );
				}
			}
		}
		WHEN( "it is copy constructed from another Condition<double>" ) {
			Condition<double> condition(value, key);
			Condition<double> copy(condition);
			THEN( "it should have the same key and value" ) {
				CHECK( copy.Value() == value );
				CHECK( copy.Key() == key );
				CHECK_FALSE( copy.IsLiteral() );
				CHECK( copy.HasConditions() );
				CHECK( static_cast<bool>(copy) );
				CHECK( static_cast<double>(copy) == value);
				CHECK( static_cast<double>(copy) == copy.Value());
				CHECK( condition.Key() == copy.Key() );
				CHECK( condition.Value() == copy.Value() );
			}
			THEN( "it should have the same origin as itself" ) {
				CHECK( condition.SameOrigin(condition) );
			}
			THEN( "they should have the same origin" ) {
				CHECK( condition.SameOrigin(copy) );
				CHECK( copy.SameOrigin(condition) );
			}
			AND_WHEN( "UpdateConditions is called without that key" ) {
				ConditionMaker vars({ { otherKey, otherValue } });
				copy.UpdateConditions(vars.Store());
				THEN( "the key and value should not change" ) {
					CHECK( copy.Value() == value );
					CHECK_FALSE( copy.IsLiteral() );
					CHECK( copy.HasConditions() );
					CHECK( copy.Key() == key );
				}
			}
			AND_WHEN( "UpdateConditions is called with that key" ) {
				ConditionMaker vars({ { key, otherValue } });
				copy.UpdateConditions(vars.Store());
				THEN( "the key should not change" ) {
					CHECK( copy.Key() == key );
					CHECK_FALSE( copy.IsLiteral() );
					CHECK( copy.HasConditions() );
				}
				THEN( "the value should change" ) {
					CHECK( copy.Value() == otherValue );
				}
			}
		}
		WHEN( "it is copy constructed from a Condition<int>" ) {
			Condition<int> condition(intValue, key);
			Condition<double> copy(condition);
			THEN( "it should have the double version of that int as its value" ) {
				CHECK( copy.Value() == static_cast<double>(condition.Value()) );
				CHECK_FALSE( copy.IsLiteral() );
				CHECK( copy.HasConditions() );
				CHECK( static_cast<bool>(copy) );
				CHECK( condition.Key() == copy.Key() );
				CHECK( condition.Value() == copy.Value() );
			}
			THEN( "it should have the same origin as itself" ) {
				CHECK( condition.SameOrigin(condition) );
			}
			THEN( "they should have the same key" ) {
				CHECK( condition.Key() == copy.Key() );
			}
			THEN( "they should have the same origin" ) {
				CHECK( condition.SameOrigin(copy) );
				CHECK( copy.SameOrigin(condition) );
			}
			AND_WHEN( "UpdateConditions is called without that key" ) {
				ConditionMaker vars({ { otherKey, otherValue } });
				copy.UpdateConditions(vars.Store());
				THEN( "the key and value should not change" ) {
					CHECK( copy.Value() == value );
					CHECK_FALSE( copy.IsLiteral() );
					CHECK( copy.HasConditions() );
					CHECK( copy.Key() == key );
				}
			}
			AND_WHEN( "UpdateConditions is called with that key" ) {
				ConditionMaker vars({ { key, otherValue } });
				copy.UpdateConditions(vars.Store());
				THEN( "the key should not change" ) {
					CHECK( copy.Key() == key );
					CHECK_FALSE( copy.IsLiteral() );
					CHECK( copy.HasConditions() );
				}
				THEN( "the value should change" ) {
					CHECK( copy.Value() == otherValue );
				}
			}
		}
		WHEN( "it is initialized with a tiny value" ) {
			Condition<double> condition(tinyValue);
			THEN( "it is false" ) {
				CHECK_FALSE( static_cast<bool>(condition) );
			}
		}
	}
}

SCENARIO( "Validating a Condition" , "[Condition][Validation]" ) {
	GIVEN( "A condition initialized without a key and with a value that fails validation" ) {
		WHEN( "calling UpdateCondition without the key" ) {
			Condition<double> condition(badValue);
			ConditionMaker vars({ { otherKey, goodValue } });
			condition.UpdateConditions(vars.Store(), Validate);
			THEN( "it should not gain a key" ) {
				CHECK( condition.Key().empty() );
				CHECK( condition.IsLiteral() );
				CHECK_FALSE( condition.HasConditions() );
			}
			THEN( "the value should be the type default (0.0)" ) {
				CHECK( condition.Value() == 0.0 );
				CHECK_FALSE( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == 0.0 );
			}
		}
	}
	GIVEN( "A condition initialized with a value that fails validation and a key" ) {
		WHEN( "calling UpdateCondition without the key" ) {
			Condition<double> condition(badValue, key);
			ConditionMaker vars({ { otherKey, goodValue } });
			condition.UpdateConditions(vars.Store(), Validate);
			THEN( "the key should not change" ) {
				CHECK( condition.Key() == key );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
			}
			THEN( "the value should be the type default (0.0)" ) {
				CHECK( condition.Value() == 0.0 );
				CHECK_FALSE( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == 0.0 );
			}
		}
		WHEN( "calling UpdateCondition with the key and a bad value" ) {
			Condition<double> condition(badValue, key);
			ConditionMaker vars({ { key, badValue } });
			condition.UpdateConditions(vars.Store(), Validate);
			THEN( "the key should not change" ) {
				CHECK( condition.Key() == key );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
			}
			THEN( "the value should be the type default (0.0)" ) {
				CHECK( condition.Value() == 0.0 );
				CHECK_FALSE( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == 0.0 );
			}
		}
		WHEN( "calling UpdateCondition with the key and a good value" ) {
			Condition<double> condition(badValue, key);
			ConditionMaker vars({ { key, goodValue } });
			condition.UpdateConditions(vars.Store(), Validate);
			THEN( "the key should not change" ) {
				CHECK( condition.Key() == key );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
			}
			THEN( "it should have the new value" ) {
				CHECK( condition.Value() == goodValue );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == goodValue );
			}
		}
	}
	GIVEN( "A condition initialized with a value that passes validation and a key" ) {
		WHEN( "calling UpdateCondition without the key" ) {
			Condition<double> condition(goodValue, key);
			ConditionMaker vars({ { otherKey, goodValue } });
			condition.UpdateConditions(vars.Store(), Validate);
			THEN( "the key should not change" ) {
				CHECK( condition.Key() == key );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
			}
			THEN( "the value should not change" ) {
				CHECK( condition.Value() == goodValue );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == goodValue );
			}
		}
		WHEN( "calling UpdateCondition with the key and a bad value" ) {
			Condition<double> condition(goodValue, key);
			ConditionMaker vars({ { key, badValue } });
			condition.UpdateConditions(vars.Store(), Validate);
			THEN( "the key should not change" ) {
				CHECK( condition.Key() == key );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
			}
			THEN( "the value should not change" ) {
				CHECK( condition.Value() == goodValue );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == goodValue );
			}
		}
		WHEN( "calling UpdateCondition with the key and another good value" ) {
			Condition<double> condition(goodValue, key);
			ConditionMaker vars({ { key, anotherGoodValue } });
			condition.UpdateConditions(vars.Store(), Validate);
			THEN( "the key should not change" ) {
				CHECK( condition.Key() == key );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
			}
			THEN( "it should have the new value" ) {
				CHECK( condition.Value() == anotherGoodValue );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == anotherGoodValue );
			}
		}
	}
}

// #endregion unit tests



} // test namespace
