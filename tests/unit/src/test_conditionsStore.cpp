/* test_conditionsStore.cpp
Copyright (c) 2022-2025 by petervdmeer

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
#include "../../../source/ConditionsStore.h"

// ... and any system includes needed for the test file.
#include <map>
#include <string>



namespace { // test namespace

// #region mock data

void verifyName(const std::string &name1, const std::string &name2)
{
	if(name1 != name2)
		throw std::runtime_error("Names \"" + name1 + "\" and \"" + name2 + "\" do not match");
}

std::string verifyAndStripPrefix(const std::string &prefix, const std::string &inputString)
{
	if(!inputString.starts_with(prefix))
		throw std::runtime_error("String \"" + inputString + "\" does not start with prefix \"" + prefix + "\"");
	return inputString.substr(prefix.size());
}

int64_t getFromMapOrZero(const std::map<std::string, int64_t> &values, const std::string &inputString)
{
	auto it = values.find(inputString);
	if(it == values.end())
		return 0;
	return it->second;
}

class MockConditionsProvider {
public:
	void SetROPrefixProvider(ConditionsStore &store, const std::string &prefix)
	{
		store[prefix].ProvidePrefixed([this, prefix](const ConditionEntry &ce) {
			verifyAndStripPrefix(prefix, ce.Name());
			return getFromMapOrZero(values, ce.Name());
		}, [prefix](ConditionEntry &ce, int64_t value) {
			verifyAndStripPrefix(prefix, ce.Name());
			return false;
		});
	}
	void SetRWPrefixProvider(ConditionsStore &store, const std::string &prefix)
	{
		store[prefix].ProvidePrefixed([this, prefix](const ConditionEntry &ce) {
			verifyAndStripPrefix(prefix, ce.Name());
			return getFromMapOrZero(values, ce.Name());
		}, [this, prefix](ConditionEntry &ce, int64_t value) {
			verifyAndStripPrefix(prefix, ce.Name());
			values[ce.Name()] = value;
			return true;
		});
	}
	void SetRONamedProvider(ConditionsStore &store, const std::string &named)
	{
		store[named].ProvideNamed([this, named](const ConditionEntry &ce) {
			verifyName(named, ce.Name());
			return getFromMapOrZero(values, ce.Name());
		}, [named](ConditionEntry &ce, int64_t value) {
			verifyName(named, ce.Name());
			return false;
		});
	}
	void SetRWNamedProvider(ConditionsStore &store, const std::string &named)
	{
		store[named].ProvideNamed([this, named](const ConditionEntry &ce) {
			verifyName(named, ce.Name());
			return getFromMapOrZero(values, ce.Name());
		}, [this, named](ConditionEntry &ce, int64_t value) {
			verifyName(named, ce.Name());
			values[ce.Name()] = value;
			return true;
		});
	}

public:
	std::map<std::string, int64_t> values;
};


// #endregion mock data



// #region unit tests
// TODO: add tests for prefixed Get cornercases (where the prefix-entry exists, but the exact match doesn't).
SCENARIO( "Creating a ConditionsStore", "[ConditionsStore][Creation]" )
{
	GIVEN( "A ConditionStore" )
	{
		WHEN( "it is just default initialized" )
		{
			const auto store = ConditionsStore();
			THEN( "the store is empty" )
			{
				REQUIRE( store.PrimariesSize() == 0 );
			}
		}
		WHEN( "initialized using an initializer list" )
		{
			const auto store = ConditionsStore{ { "hello world", 100 }, { "goodbye world", 404 } };
			THEN( "given primary conditions are in the Store" )
			{
				REQUIRE( store.Get("hello world") == 100 );
				REQUIRE( store.Get("goodbye world") == 404 );
				REQUIRE( store.PrimariesSize() == 2 );
				// Also check for possible ill-effects from PrimariesSize() itself.
				REQUIRE( store.PrimariesSize() == 2 );
			}
			THEN( "not given conditions return the default value" )
			{
				REQUIRE( 0 == store.Get("ungreeted world") );
				REQUIRE( store.PrimariesSize() == 2 );
				REQUIRE( 0 == store.Get("hi world") );
				REQUIRE( store.PrimariesSize() == 2 );
				// Check that requesting a non-given condition twice also doesn't result in bad results
				// (for example due to caching).
				REQUIRE( 0 == store.Get("hi world") );
				REQUIRE( store.PrimariesSize() == 2 );
			}
		}
		WHEN( "initialized using an initializer map" )
		{
			const std::map<std::string, int64_t> initmap{ { "hello world", 100 }, { "goodbye world", 404 } };
			const auto store = ConditionsStore(initmap);
			THEN( "given primary conditions are in the Store" )
			{
				REQUIRE( store.Get("hello world") == 100 );
				REQUIRE( store.Get("goodbye world") == 404 );
				REQUIRE( store.PrimariesSize() == 2 );
			}
			THEN( "not given conditions return the default value" )
			{
				REQUIRE( store.Get("ungreeted world") == 0 );
				REQUIRE( store.PrimariesSize() == 2 );
				REQUIRE( store.Get("ungreeted world") == 0 );
				REQUIRE( store.PrimariesSize() == 2 );
				REQUIRE( 0 == store.Get("hi world") );
				REQUIRE( store.PrimariesSize() == 2 );
				REQUIRE( store.Get("hi world") == 0 );
				REQUIRE( store.PrimariesSize() == 2 );
			}
		}
		WHEN( "initialized with a long initializer list" )
		{
			const auto store = ConditionsStore{ { "a", 1 }, { "b", 2 }, { "d", 4 }, { "c", 3 }, { "g", 7 },
				{ "f", 6 }, { "e", 5 } };
			THEN( "all items need to be stored" )
			{
				REQUIRE( store.PrimariesSize() == 7 );
				REQUIRE( store.Get("a") == 1 );
				REQUIRE( store.Get("b") == 2 );
				REQUIRE( store.Get("c") == 3 );
				REQUIRE( store.Get("d") == 4 );
				REQUIRE( store.Get("e") == 5 );
				REQUIRE( store.Get("f") == 6 );
				REQUIRE( store.Get("g") == 7 );
				REQUIRE( store.PrimariesSize() == 7 );
			}
		}
	}
}

SCENARIO( "Setting conditions", "[ConditionStore][ConditionSetting]" )
{
	GIVEN( "An empty conditionsStore" )
	{
		auto store = ConditionsStore();
		REQUIRE( store.PrimariesSize() == 0 );
		WHEN( "a condition is set" )
		{
			store.Set("myFirstVar", 10);
			THEN( "stored condition is present and can be retrieved" )
			{
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.PrimariesSize() == 1 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store["myFirstVar"] == 10 );
			}
			THEN( "the condition can be set to another value" )
			{
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.PrimariesSize() == 1 );
				store.Set("myFirstVar", 2000);
				REQUIRE( store.Get("myFirstVar") == 2000 );
			}

			THEN( "the condition can be set to zero" )
			{
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.PrimariesSize() == 1 );
				store.Set("myFirstVar", 0);
				REQUIRE( store.Get("myFirstVar") == 0 );
			}
		}
		WHEN( "non-existing conditions are queried" )
		{
			THEN( "defaults are returned and queried conditions are not stored" )
			{
				REQUIRE( store.Get("mySecondVar") == 0 );
				REQUIRE( store.PrimariesSize() == 0 );
				REQUIRE_FALSE( store.Get("mySecondVar") );
				REQUIRE( store.Get("mySecondVar") == 0 );
				REQUIRE( store.PrimariesSize() == 0 );
			}
			THEN( "they get created when accessed through square brackets" )
			{
				REQUIRE( store["mySecondVar"] == 0 );
				REQUIRE( store.PrimariesSize() == 1 );
			}
		}
	}
}

SCENARIO( "Adding and removing on condition values", "[ConditionStore][ConditionArithmetic]" )
{
	GIVEN( "A conditionsStore with 1 condition" )
	{
		auto store = ConditionsStore{ { "myFirstVar", 10 } };
		REQUIRE( store.Get("myFirstVar") == 10 );
		REQUIRE( store.PrimariesSize() == 1 );
		WHEN( "adding to the existing primary condition" )
		{
			store.Add("myFirstVar", 10);
			THEN( "the condition gets the new value" )
			{
				REQUIRE( store.Get("myFirstVar") == 20 );
				store.Add("myFirstVar", -15);
				REQUIRE( store.Get("myFirstVar") == 5 );
				store.Add("myFirstVar", -15);
				REQUIRE( store.Get("myFirstVar") == -10 );
				REQUIRE( store["myFirstVar"] == -10 );
				++store["myFirstVar"];
				REQUIRE( store.Get("myFirstVar") == -9 );
				++store["myFirstVar"];
				REQUIRE( store["myFirstVar"] == -8 );
				REQUIRE( -8 == store["myFirstVar"] );
				auto &&se = store["myFirstVar"];
				++se;
				REQUIRE( store.Get("myFirstVar") == -7 );
				--se;
				REQUIRE( store.Get("myFirstVar") == -8 );
				se += 20;
				REQUIRE( store.Get("myFirstVar") == 12 );
				se -= 5;
				REQUIRE( store.Get("myFirstVar") == 7 );
			}
		}
		WHEN( "adding to another non-existing (primary) condition sets the new value" )
		{
			store.Add("mySecondVar", -30);
			THEN( "the new condition exists with the new value" )
			{
				REQUIRE( store.Get("mySecondVar") == -30 );
				REQUIRE( store.PrimariesSize() == 2 );
				REQUIRE( store.Get("mySecondVar") );
				store.Add("mySecondVar", 60);
				REQUIRE( store.Get("mySecondVar") == 30 );
				REQUIRE( store.PrimariesSize() == 2 );
			}
		}
	}
}

SCENARIO( "Providing derived conditions", "[ConditionStore][DerivedConditions]" )
{
	GIVEN( "A conditionsStore, a prefixed provider and a named provider" )
	{
		auto mockProvPrefixA = MockConditionsProvider();
		auto mockProvNamed = MockConditionsProvider();
		auto store = ConditionsStore{ { "myFirstVar", 10 } };
		mockProvNamed.SetRWNamedProvider(store, "named1");
		mockProvPrefixA.SetRWPrefixProvider(store, "prefixA: ");
		store.Add("named1", -30);
		REQUIRE( mockProvNamed.values.size() == 1 );
		REQUIRE( mockProvNamed.values["named1"] == -30 );
		REQUIRE( store.PrimariesSize() == 1 );
		REQUIRE( mockProvPrefixA.values.size() == 0 );
		store.Add("prefixA: test", -30);
		REQUIRE( store.PrimariesSize() == 1 );
		REQUIRE( mockProvPrefixA.values.size() == 1 );
		REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
		REQUIRE( mockProvNamed.values.size() == 1 );
		REQUIRE( store.PrimariesSize() == 1 );
		WHEN( "requesting the name of the prefixed entry, without prefix" )
		{
			THEN( "the substring should be returned that is unique" )
			{
				REQUIRE( store["prefixA: test"].NameWithoutPrefix() == "test" );
			}
		}
		WHEN( "adding to an existing primary condition" )
		{
			THEN( "the add should work as-is" )
			{
				REQUIRE( store.Get("myFirstVar") == 10 );
				store.Add("myFirstVar", 10);
				REQUIRE( store.Get("myFirstVar") == 20 );
				store.Add("myFirstVar", -15);
				REQUIRE( store.Get("myFirstVar") == 5 );
				store.Add("myFirstVar", -15);
				REQUIRE( store.Get("myFirstVar") == -10 );
				auto &&sa = store["myFirstVar"];
				sa += 15;
				REQUIRE( store.Get("myFirstVar") == 5 );
				REQUIRE( sa == 5 );
				sa -= 4;
				REQUIRE( store.Get("myFirstVar") == 1 );
				REQUIRE( store["myFirstVar"] == 1 );
			}
		}
		WHEN( "adding to an non-existing primary condition" )
		{
			THEN( "such condition should be set properly" )
			{
				store.Add("mySecondVar", -30);
				REQUIRE( store.Get("mySecondVar") == -30 );
				REQUIRE( store.PrimariesSize() == 2 );
				REQUIRE( store.Get("mySecondVar") );
				store.Add("mySecondVar", 60);
				REQUIRE( store.Get("mySecondVar") == 30 );
				REQUIRE( store.PrimariesSize() == 2 );
			}
		}
		WHEN( "adding on a named derived condition" )
		{
			store.Add("named1", -30);
			THEN( "effects of adding should be on the named condition" )
			{
				REQUIRE( store.PrimariesSize() == 1 );
				REQUIRE( mockProvNamed.values["named1"] == -60 );
				store.Add("named1", -20);
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( mockProvNamed.values["named1"] == -80 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( store.Get("named1") == -80 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.Get("mySecondVar") == 0 );
				REQUIRE( store["named1"] == -80 );
				REQUIRE( store["myFirstVar"] == 10 );
				REQUIRE( store.PrimariesSize() == 1 );
				--store["named1"];
				++store["myFirstVar"];
				REQUIRE( store.Get("named1") == -81 );
				REQUIRE( store.Get("myFirstVar") == 11 );
				REQUIRE( store.Get("mySecondVar") == 0 );
			}
			THEN( "readonly providers should reject the add and don't change values" )
			{
				mockProvNamed.SetRONamedProvider(store, "named1");
				store.Add("named1", -20);
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( mockProvNamed.values["named1"] == -60 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( store.Get("named1") == -60 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.Get("mySecondVar") == 0 );
				--store["named1"];
				REQUIRE( store.Get("named1") == -60 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.Get("mySecondVar") == 0 );
				store["named1"] -= 50;
				REQUIRE( store.Get("named1") == -60 );
				REQUIRE( store.Get("myFirstVar") == 10 );
			}
			THEN( "readonly providers should not perform any set actions" )
			{
				mockProvNamed.SetRONamedProvider(store, "named1");
				store.Set("named1", 0);
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( mockProvNamed.values["named1"] == -60 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				store.Set("named1", 40);
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( mockProvNamed.values["named1"] == -60 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( store.Get("named1") == -60 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.Get("named1") );
			}
			THEN( "not given conditions (that almost match the named condition) should not exist" )
			{
				REQUIRE_FALSE( store.Get("named") );
				REQUIRE_FALSE( store.Get("named11") );
			}
		}
		WHEN( "adding on a prefixed derived condition" )
		{
			store.Add("prefixA: test", -30);
			THEN( "derived prefixed conditions should be set properly" )
			{
				REQUIRE( store.PrimariesSize() == 1 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( mockProvPrefixA.values["prefixA: test"] == -60 );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( store.Get("prefixA: test") == -60 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.Get("mySecondVar") == 0 );
				REQUIRE( store["myFirstVar"] == 10 );
				REQUIRE( store["prefixA: test"] == -60 );
				store["myFirstVar"] += 2;
				store["prefixA: test"] -= 10;
				REQUIRE( store.Get("prefixA: test") == -70 );
				REQUIRE( store.Get("myFirstVar") == 12 );
				REQUIRE( store.Get("mySecondVar") == 0 );
				REQUIRE( store["myFirstVar"] == 12 );
				REQUIRE( store["prefixA: test"] == -70 );
				REQUIRE( store.PrimariesSize() == 1 );
			}
			THEN( "read-only prefixed provider should reject further updates" )
			{
				mockProvPrefixA.SetROPrefixProvider(store, "prefixA: ");
				store.Add("prefixA: test", -20);
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( mockProvPrefixA.values["prefixA: test"] == -60 );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( store.Get("prefixA: test") == -60 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				store["prefixA: test"] -= 20;
				REQUIRE( store.Get("prefixA: test") == -60 );
				REQUIRE( store.Get("myFirstVar") == 10 );
			}
			THEN( "prefixed values from within provider should be available" )
			{
				mockProvPrefixA.values["prefixA: "] = 22;
				mockProvPrefixA.values["prefixA:"] = 21;
				REQUIRE( store.Get("prefixA: test") );
				REQUIRE_FALSE( store.Get("prefixA: t") );
				REQUIRE( store.Get("prefixA: ") );
				REQUIRE_FALSE( store.Get("prefixA:") );
				REQUIRE( store.Get("prefixA: ") == 22 );
				REQUIRE( store.Get("prefixA:") == 0 );
				REQUIRE( store.Get("prefixA: test") == -60 );
				REQUIRE( store["prefixA: test"] == -60 );
				REQUIRE( store["prefixA: "] == 22 );
			}
			AND_GIVEN( "more derived condition providers are added" )
			{
				auto mockProvPrefix = MockConditionsProvider();
				mockProvPrefix.SetRWPrefixProvider(store, "prefix: ");
				auto mockProvPrefixB = MockConditionsProvider();
				mockProvPrefixB.SetRWPrefixProvider(store, "prefixB: ");
				THEN( "derived prefixed conditions should be set properly" )
				{
					REQUIRE( store.PrimariesSize() == 1 );
					store.Add("prefixA: test", 30);
					REQUIRE( store.PrimariesSize() == 1 );
					REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
					REQUIRE( store.Get("prefixA: test") == -30 );
					REQUIRE( store.Get("myFirstVar") == 10 );
					mockProvPrefixA.SetROPrefixProvider(store, "prefixA: ");
					store.Add("prefixA: test", -20);
					REQUIRE( mockProvPrefixA.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
					REQUIRE( store.Get("prefixA: test") == -30 );
					REQUIRE( store.Get("myFirstVar") == 10 );
					store.Set("prefixA: test", 0);
					REQUIRE( mockProvPrefixA.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
					REQUIRE( store.Get("prefixA: test") == -30 );
					REQUIRE( store.Get("myFirstVar") == 10 );
					REQUIRE( store.Get("prefixA: test") );
					REQUIRE_FALSE( store.Get("prefixA: t") );
					REQUIRE_FALSE( store.Get("prefixA: ") );
					REQUIRE_FALSE( store.Get("prefixA:") );
					mockProvPrefixA.values["prefixA: "] = 22;
					mockProvPrefixA.values["prefixA:"] = 21;
					REQUIRE( store.Get("prefixA: test") );
					REQUIRE_FALSE( store.Get("prefixA: t") );
					REQUIRE( store.Get("prefixA: ") );
					REQUIRE_FALSE( store.Get("prefixA:") );
					REQUIRE( mockProvPrefix.values.size() == 0 );
					REQUIRE( mockProvPrefixA.values.size() == 3 );
					REQUIRE( mockProvPrefixB.values.size() == 0 );
					mockProvPrefixA.SetRWPrefixProvider(store, "prefixA: ");
					store.Set("prefix: beginning", 42);
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 3 );
					REQUIRE( mockProvPrefixB.values.size() == 0 );
					store.Set("prefixB: ending", 142);
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 3 );
					REQUIRE( mockProvPrefixB.values.size() == 1 );
					store.Set("prefixA: middle", 40);
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 4 );
					REQUIRE( mockProvPrefixB.values.size() == 1 );
					store.Set("prefixA: middle2", 90);
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 5 );
					REQUIRE( mockProvPrefixB.values.size() == 1 );
					REQUIRE( store.Get("prefix: beginning") == 42 );
					REQUIRE( store.Get("prefixB: ending") == 142 );
					REQUIRE( store.Get("prefixA: ") == 22 );
					REQUIRE( store.Get("prefixA:") == 0 );
					REQUIRE( store.Get("prefixA: middle") == 40 );
					REQUIRE( store.Get("prefixA: middle2") == 90 );
					REQUIRE( store.Get("prefixA: test") == -30 );
					REQUIRE( store.Get("myFirstVar") == 10 );
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 5 );
					REQUIRE( mockProvPrefixB.values.size() == 1 );
					REQUIRE( store.PrimariesSize() == 1 );
				}
			}
		}
	}
}


SCENARIO( "Providing multiple derived conditions", "[ConditionStore][DerivedMultiple]" )
{
	GIVEN( "A conditionsStore and a prefixed provider" )
	{
		auto store = ConditionsStore();
		auto mockProvPrefixShips = MockConditionsProvider();
		mockProvPrefixShips.SetRWPrefixProvider(store, "ships: ");
		WHEN( "adding variables with similar names" )
		{
			store.Add("ships: A", 20);
			REQUIRE( mockProvPrefixShips.values.size() == 1 );
			REQUIRE( mockProvPrefixShips.values["ships: A"] == 20 );
			store.Add("ships: AB", 30);
			REQUIRE( mockProvPrefixShips.values.size() == 2 );
			REQUIRE( mockProvPrefixShips.values["ships: AB"] == 30 );
			store.Add("ships: C", 40);
			REQUIRE( mockProvPrefixShips.values.size() == 3 );
			REQUIRE( mockProvPrefixShips.values["ships: C"] == 40 );
			THEN( "the values should be retrieved as set" )
			{
				REQUIRE( store.Get("ships: AB") == 30 );
				REQUIRE( store["ships: AB"] == 30 );
				REQUIRE( store.Get("ships: C") == 40 );
				REQUIRE( store["ships: C"] == 40 );
				REQUIRE( store.Get("ships: A") == 20 );
				REQUIRE( store["ships: A"] == 20 );
				REQUIRE( mockProvPrefixShips.values.size() == 3 );
			}
		}
		WHEN( "adding a prefixed provider that is in the subset of the prefixed provider" )
		{
			auto mockProvPrefixShipsA = MockConditionsProvider();
			// Just check that we don't crash.
			mockProvPrefixShipsA.SetRWPrefixProvider(store, "ships: A:");
		}
		WHEN( "adding a named provider that is in the subset of the prefixed provider" )
		{
			auto mockProvPrefixShipsA = MockConditionsProvider();
			// Just check that we don't crash.
			mockProvPrefixShipsA.SetRWNamedProvider(store, "ships: A:");
		}
		WHEN( "adding a prefixed provider that is in the superset of the prefixed provider" )
		{
			auto mockProvPrefixShi = MockConditionsProvider();
			// Just check that we don't crash.
			mockProvPrefixShi.SetRWPrefixProvider(store, "shi");
		}
	}
	GIVEN( "A pre-existing condition in a store" )
	{
		auto store = ConditionsStore();
		store["ships: A"] = 40;
		WHEN(" a prefixed provider gets added which has the condition in range")
		{
			auto mockProvPrefixShips = MockConditionsProvider();
			// Just check that we don't crash.
			mockProvPrefixShips.SetRWPrefixProvider(store, "ships: ");
			THEN( "Adding a sub-prefix-condition should not cause a crash")
			{
				auto mockProvPrefixShipsLarge = MockConditionsProvider();
				// Just check that we don't crash.
				mockProvPrefixShipsLarge.SetRWPrefixProvider(store, "ships: Large: ");
			}
		}
	}
}


// #endregion unit tests



} // test namespace
