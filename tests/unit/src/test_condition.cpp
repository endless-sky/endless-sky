/* test_condition.cpp
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
const double value = static_cast<double>(intValue);
const double otherValue = 2.0;
const ConditionsStore::ValueType thirdValueInt = 99;
const double thirdValue = static_cast<double>(thirdValueInt);
const double fourthValue = 12345;
const double tinyValue = 1e-30;
const std::string key = "key";
const std::string otherKey = "otherKey";
const char *nonEmptyCString = "nonEmptyCString";
const char *anotherCString = "anotherCString";


// For validation checks
const double badValue = -1.0;
const double otherBadValue = -2.0;
const double goodValue = 1.0;
const double otherGoodValue = 2.0;
double FilterCondition(ConditionsStore::ValueType newValue, double oldValue)
{
	if(newValue >= 0)
		return static_cast<double>(newValue);
	else
		return oldValue;
}
double FilterValue(double oldValue)
{
	return oldValue >= 0 ? oldValue : 0;
}

// #endregion mock data

// IMPLEMENTATION NOTE: These unit tests assume the default KeyType of
// a Condition is std::string. This is because the code assumes that
// as well.

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
		CHECK( std::is_nothrow_move_constructible<T>::value );
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
		CHECK( std::is_nothrow_move_assignable<T>::value );
	}
}

SCENARIO( "Creating a Condition with the default KeyType" , "[Condition][KeyType]" ) {
	GIVEN( "a default-constructed Condition<double>::KeyType" ) {
		WHEN( "its empty() method is called" ) {
			Condition<double>::KeyType key;
			THEN( "the result should be true" ) {
				CHECK( key.empty() );
				CHECK_FALSE( !key.empty() );
			}
		}
	}
	GIVEN( "a Condition<double>::KeyType constructed from a c string (const char *) to a non-empty string" ) {
		Condition<double>::KeyType key(nonEmptyCString);
		WHEN( "its empty() method is called" ) {
			THEN( "the result should be false" ) {
				CHECK_FALSE( key.empty() );
				CHECK( !key.empty() );
			}
		}
		WHEN( "compared to c strings" ) {
			THEN( "it should be equal to the c string provide to it" ) {
				CHECK( key == nonEmptyCString );
				CHECK_FALSE( key != nonEmptyCString );
			}
			THEN( "it should not be equal to a different c string" ) {
				CHECK_FALSE( key == anotherCString );
				CHECK( key != anotherCString );
			}
		}
	}
}

SCENARIO( "Creating a Condition" , "[Condition][Creation]" ) {
	GIVEN( "a Condition<double>" ) {
		WHEN( "it is default initialized" ) {
			Condition<double> condition;
			THEN( "the contents should be empty" ) {
				CHECK( condition.Value() == double() );
				CHECK( condition.Key().empty() );
				CHECK( condition.IsLiteral() );
				CHECK_FALSE( condition.HasConditions() );
				CHECK_FALSE( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == double());
				CHECK( static_cast<double>(condition) == condition.Value());
			}
			THEN( "it should have the same origin as itself" ) {
				CHECK( condition.SameOrigin(condition) );
			}
			AND_WHEN( "UpdateConditions is called" ) {
				condition.UpdateConditions();
				THEN( "the key and value should not change" ) {
					CHECK( condition.Value() == double() );
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
				ConditionMaker vars;
				Condition<double> named(value, vars.Store(), key);
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
			AND_WHEN( "SameOrigin() is called with a literal Condition with the same value" ) {
				Condition<double> same(value);
				THEN( "the result should be true" ) {
					CHECK( same.IsLiteral() );
					CHECK( condition.SameOrigin(same) );
					CHECK( same.SameOrigin(condition) );
				}
			}
			AND_WHEN( "SameOrigin() is called with a literal Condition with a different value" ) {
				Condition<double> other(otherValue);
				THEN( "the result should be false" ) {
					CHECK( other.IsLiteral() );
					CHECK_FALSE( condition.SameOrigin(other) );
					CHECK_FALSE( other.SameOrigin(condition) );
				}
			}
			AND_WHEN( "UpdateConditions is called" ) {
				condition.UpdateConditions();
				THEN( "the key and value should not change" ) {
					CHECK( condition.Value() == value );
					CHECK( condition.IsLiteral() );
					CHECK_FALSE( condition.HasConditions() );
					CHECK( condition.Key().empty() );
				}
			}
		}
		WHEN( "it is initialized with a key, value, and empty store" ) {
			ConditionMaker vars;
			Condition<double> condition(value, vars.Store(), key);
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
				Condition<double> other(otherValue, vars.Store(), key);
				THEN( "the result should be true" ) {
					CHECK( other.Value() == otherValue );
					CHECK( condition.SameOrigin(other) );
					CHECK( other.SameOrigin(condition) );
				}
			}
			AND_WHEN( "SameOrigin() is called with a Condition that has a different name but the same value" ) {
				Condition<double> other(condition.Value(), vars.Store(), "notkey");
				THEN( "the result should be true" ) {
					CHECK( other.Key() == "notkey" );
					CHECK_FALSE( condition.SameOrigin(other) );
					CHECK_FALSE( other.SameOrigin(condition) );
				}
			}
			AND_WHEN( "UpdateConditions is called" ) {
				condition.UpdateConditions();
				THEN( "the key and value should not change" ) {
					CHECK( condition.Value() == value );
					CHECK_FALSE( condition.IsLiteral() );
					CHECK( condition.HasConditions() );
					CHECK( condition.Key() == key );
				}
			}
			AND_WHEN( "the key is added to the store and UpdateConditions is called" ) {
				(*vars.Store())[key] = otherValue;
				condition.UpdateConditions();
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
		WHEN( "it is initialized with a key, value, and a store that has a DataProvider "
				"for that key, but a different value for it" ) {
			ConditionMaker vars;
			std::string prefix = "prefix ";
			std::string fullKey = prefix + key;
			double providerValue = otherValue;
			vars.AddProviderNamed(fullKey, fullKey, providerValue);
			Condition<double> condition(value, vars.Store(), fullKey);
			Condition<double> fromProvider(vars.Store(), fullKey);
			THEN( "the condition should have its initial value" ) {
				CHECK( condition.Value() == value );
				CHECK( condition.Key() == fullKey );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == value);
				CHECK( static_cast<double>(condition) == condition.Value());
			}
			THEN( "it should have the same origin as itself" ) {
				CHECK( condition.SameOrigin(condition) );
			}
			AND_WHEN( "the DataProvider's value for that key is changed" ) {
				vars.Store()->Set(fullKey, thirdValueInt);
				fromProvider.UpdateConditions();
				AND_WHEN( "a condition who had a key from that provider has its UpdateConditions() is called" ) {
					THEN( "the key should not change" ) {
						CHECK( fromProvider.Key() == fullKey );
						CHECK_FALSE( fromProvider.IsLiteral() );
						CHECK( fromProvider.HasConditions() );
					}
					THEN( "the value should change" ) {
						CHECK( fromProvider.Value() == thirdValue );
					}
				}
				AND_WHEN( "a condition connected to the provider, but with a different initial"
						" value, does not have its UpdateConditions() called" ) {
					THEN( "the key should not change" ) {
						CHECK( condition.Key() == fullKey );
						CHECK_FALSE( condition.IsLiteral() );
						CHECK( condition.HasConditions() );
					}
					THEN( "the value should not change" ) {
						CHECK( condition.Value() == value );
					}
				}
			}
			WHEN( "SameOrigin() is called with a Condition that has the DataProvider's value" ) {
				THEN( "the result should be true" ) {
					CHECK( condition.SameOrigin(fromProvider) );
					CHECK( fromProvider.SameOrigin(condition) );
				}
			}
			AND_WHEN( "SameOrigin() is called with a Condition that has a different name but the same value" ) {
				Condition<double> other(condition.Value(), vars.Store(), "notkey");
				THEN( "the result should be false" ) {
					CHECK( other.Key() == "notkey" );
					CHECK_FALSE( condition.SameOrigin(other) );
					CHECK_FALSE( other.SameOrigin(condition) );
				}
			}
			AND_WHEN( "UpdateConditions is called" ) {
				condition.UpdateConditions();
				THEN( "the key should not change" ) {
					CHECK( condition.Key() == fullKey );
					CHECK_FALSE( condition.IsLiteral() );
					CHECK( condition.HasConditions() );
				}
				THEN( "the value should match the DataProvider's value" ) {
					CHECK_FALSE( condition.Value() == value );
					CHECK( condition.Value() == providerValue );
				}
			}
		}
		WHEN( "it is initialized with a key and a store with that key" ) {
			ConditionMaker vars({ {key, value} });
			Condition<double> condition(vars.Store(), key);
			THEN( "it should have that key and value" ) {
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
				Condition<double> other(otherValue, vars.Store(), key);
				THEN( "the result should be true" ) {
					CHECK( other.Value() == otherValue );
					CHECK( condition.SameOrigin(other) );
					CHECK( other.SameOrigin(condition) );
				}
			}
			AND_WHEN( "SameOrigin() is called with a Condition that has a different name but the same value" ) {
				Condition<double> other(condition.Value(), vars.Store(), "notkey");
				THEN( "the result should be true" ) {
					CHECK( other.Key() == "notkey" );
					CHECK_FALSE( condition.SameOrigin(other) );
					CHECK_FALSE( other.SameOrigin(condition) );
				}
			}
			AND_WHEN( "UpdateConditions is called" ) {
				condition.UpdateConditions();
				THEN( "the key should not change" ) {
					CHECK( condition.Key() == key );
					CHECK_FALSE( condition.IsLiteral() );
					CHECK( condition.HasConditions() );
				}
				THEN( "the value should not change" ) {
					CHECK( condition == value );
					CHECK( condition.Value() == value );
				}
			}
			AND_WHEN( "the value in the store is updated and UpdateConditions is called" ) {
				vars.Store()->Set(key, thirdValueInt);
				condition.UpdateConditions();
				THEN( "the key should not change" ) {
					CHECK( condition.Key() == key );
					CHECK_FALSE( condition.IsLiteral() );
					CHECK( condition.HasConditions() );
				}
				THEN( "the value should change to the new value" ) {
					CHECK( condition == thirdValue );
					CHECK( condition.Value() == thirdValue );
				}
			}
			AND_WHEN( "a new value is assigned to the Condition" ) {
				condition.Value() = fourthValue;
				THEN( "the key should not change" ) {
					CHECK( condition.Key() == key );
					CHECK_FALSE( condition.IsLiteral() );
					CHECK( condition.HasConditions() );
				}
				THEN( "the condition's value should change" ) {
					CHECK( condition == fourthValue );
					CHECK( condition.Value() == fourthValue );
				}
				AND_WHEN( "SendValue is called" ) {
					condition.SendValue();
					THEN( "the store's value should change" ) {
						CHECK( vars.Store()->Get(key) ==
							static_cast<ConditionsStore::ValueType>(fourthValue) );
					}
				}
			}
			AND_WHEN( "UpdateConditions is called again" ) {
				condition.UpdateConditions();
				THEN( "the key should not change" ) {
					CHECK( condition.Key() == key );
					CHECK_FALSE( condition.IsLiteral() );
					CHECK( condition.HasConditions() );
				}
				THEN( "the value should not change" ) {
					CHECK( condition == value );
					CHECK( condition.Value() == value );
				}
			}
		}
		WHEN( "it is copy constructed from another Condition<double>" ) {
			ConditionMaker vars({ {otherKey, otherValue} });
			Condition<double> condition(value, vars.Store(), key);
			Condition<double> copy(condition);
			THEN( "it should have the same key and value" ) {
				CHECK( copy.Value() == value );
				CHECK( copy.Key() == key );
				CHECK_FALSE( copy.IsLiteral() );
				CHECK( copy.HasConditions() );
				CHECK( !copy.IsLiteral() );
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
			AND_WHEN( "UpdateConditions is called on the copy without that key" ) {
				copy.UpdateConditions();
				THEN( "the key and value should not change" ) {
					CHECK( copy.Value() == value );
					CHECK_FALSE( copy.IsLiteral() );
					CHECK( copy.HasConditions() );
					CHECK( copy.Key() == key );
					CHECK( condition.Value() == value );
					CHECK_FALSE( condition.IsLiteral() );
					CHECK( condition.HasConditions() );
					CHECK( condition.Key() == key );
				}
			}
			AND_WHEN( "the key is added to the store, and UpdateConditions is called on the copy" ) {
				vars.Store()->Set(key, thirdValueInt);
				copy.UpdateConditions();
				THEN( "the copy's key should not change" ) {
					CHECK( copy.Key() == key );
					CHECK_FALSE( copy.IsLiteral() );
					CHECK( copy.HasConditions() );
				}
				THEN( "the copy's value should change" ) {
					CHECK( copy.Value() == thirdValue );
				}
				THEN( "the original's key should not change" ) {
					CHECK( condition.Key() == key );
					CHECK_FALSE( condition.IsLiteral() );
					CHECK( condition.HasConditions() );
				}
				THEN( "the original's value should not change" ) {
					CHECK( condition.Value() == value );
				}
			}
		}
		WHEN( "it is copy constructed from a Condition<int>" ) {
			ConditionMaker vars;
			Condition<int> condition(intValue, vars.Store(), key);
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
			AND_WHEN( "UpdateConditions is called without that key" ) {
				vars.Store()->Set(otherKey, otherValue);
				copy.UpdateConditions();
				THEN( "the key should not change" ) {
					CHECK_FALSE( copy.IsLiteral() );
					CHECK( copy.HasConditions() );
					CHECK( copy.Key() == key );
				}
				THEN( "value should not change" ) {
					CHECK( copy.Value() == static_cast<double>(intValue) );
				}
			}
			AND_WHEN( "UpdateConditions is called with that key" ) {
				vars.Store()->Set(key, otherValue);
				copy.UpdateConditions();
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

SCENARIO( "Filtering in UpdateCondition" , "[Condition][Filtering]" ) {
	GIVEN( "A condition initialized without a key and with a value that fails the filter" ) {
		ConditionMaker vars({ { otherKey, goodValue } });
		Condition<double> condition(badValue);
		WHEN( "calling UpdateCondition without the key" ) {
			condition.UpdateConditions(FilterCondition);
			THEN( "it should not gain a key" ) {
				CHECK( condition.Key().empty() );
				CHECK( condition.IsLiteral() );
				CHECK_FALSE( condition.HasConditions() );
			}
			THEN( "the value should not be changed" ) {
				CHECK( condition.Value() == badValue );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == badValue );
			}
		}
		WHEN( "calling the filter" ) {
			condition.Filter(FilterValue);
			THEN( "it should not gain a key" ) {
				CHECK( condition.Key().empty() );
				CHECK( condition.IsLiteral() );
				CHECK_FALSE( condition.HasConditions() );
			}
			THEN( "the value should be 0" ) {
				CHECK( condition.Value() == 0 );
				CHECK_FALSE( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == 0 );
			}
		}
	}
	GIVEN( "A condition initialized with a value that fails validation and a key" ) {
		WHEN( "calling UpdateCondition without the key" ) {
			ConditionMaker vars({ { otherKey, goodValue } });
			Condition<double> condition(badValue, vars.Store(), key);
			condition.UpdateConditions(FilterCondition);
			THEN( "the key should not change" ) {
				CHECK( condition.Key() == key );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
			}
			THEN( "the value should not change" ) {
				CHECK( condition.Value() == badValue );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == badValue );
			}
		}
		WHEN( "calling UpdateCondition with the a store that has the key and a bad value on a "
				"condition that has the key and another bad value" ) {
			ConditionMaker vars({ { key, badValue } });
			Condition<double> condition(otherBadValue, vars.Store(), key);
			condition.UpdateConditions(FilterCondition);
			THEN( "the key should not change" ) {
				CHECK( condition.Key() == key );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
			}
			THEN( "the value should not change" ) {
				CHECK( condition.Value() == otherBadValue );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == otherBadValue );
			}
		}
		WHEN( "calling UpdateCondition with the key and a good value on a condition that has that key and a bad value" ) {
			ConditionMaker vars({ { key, goodValue } });
			Condition<double> condition(badValue, vars.Store(), key);
			condition.UpdateConditions(FilterCondition);
			THEN( "the key should not change" ) {
				CHECK( condition.Key() == key );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
			}
			THEN( "the condition should have the new value" ) {
				CHECK( condition.Value() == goodValue );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == goodValue );
			}
		}
	}
	GIVEN( "A condition initialized with a value that passes validation and a key" ) {
		WHEN( "calling UpdateCondition without the key" ) {
			ConditionMaker vars({ { otherKey, otherGoodValue } });
			Condition<double> condition(goodValue, vars.Store(), key);
			condition.UpdateConditions(FilterCondition);
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
			ConditionMaker vars({ { key, badValue } });
			Condition<double> condition(goodValue, vars.Store(), key);
			condition.UpdateConditions(FilterCondition);
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
			ConditionMaker vars({ { key, otherGoodValue } });
			Condition<double> condition(goodValue, vars.Store(), key);
			condition.UpdateConditions(FilterCondition);
			THEN( "the key should not change" ) {
				CHECK( condition.Key() == key );
				CHECK_FALSE( condition.IsLiteral() );
				CHECK( condition.HasConditions() );
			}
			THEN( "it should have the new value" ) {
				CHECK( condition.Value() == otherGoodValue );
				CHECK( static_cast<bool>(condition) );
				CHECK( static_cast<double>(condition) == otherGoodValue );
			}
		}
	}
}

// #endregion unit tests



} // test namespace
