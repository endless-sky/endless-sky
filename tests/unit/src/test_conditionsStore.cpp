/* test_conditionsStore.cpp
Copyright (c) 2022 by petervdmeer

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
	if(inputString.size() < prefix.size() || (0 != inputString.compare(0, prefix.size(), prefix)))
		throw std::runtime_error("String \"" + inputString + "\" does not start with prefix \"" + prefix + "\"");
	return inputString.substr(prefix.size());
}

bool isInMap(const std::map<std::string, int64_t> &values, const std::string &inputString)
{
	return values.find(inputString) != values.end();
}

int64_t getFromMapOrZero(const std::map<std::string, int64_t> &values, const std::string &inputString)
{
	auto it = values.find(inputString);
	if(it == values.end())
		return 0;
	return it->second;
}

int primarySize(const ConditionsStore &store)
{
	int size = 0;
	auto it = store.PrimariesBegin();
	while(it != store.PrimariesEnd())
	{
		++it;
		++size;
	}
	return size;
}


class MockConditionsProvider {
public:
	void SetROPrefixProvider(ConditionsStore &store, const std::string &prefix)
	{
		auto &&conditionsProvider = store.GetProviderPrefixed(prefix);
		conditionsProvider.SetHasFunction([this, prefix](const std::string &name)
		{
			verifyAndStripPrefix(prefix, name);
			return isInMap(values, name);
		});
		conditionsProvider.SetSetFunction([prefix](const std::string &name, int64_t value)
		{
			verifyAndStripPrefix(prefix, name);
			return false;
		});
		conditionsProvider.SetEraseFunction([prefix](const std::string &name)
		{
			verifyAndStripPrefix(prefix, name);
			return false;
		});
		conditionsProvider.SetGetFunction([this, prefix](const std::string &name)
		{
			verifyAndStripPrefix(prefix, name);
			return getFromMapOrZero(values, name);
		});
	}
	void SetRWPrefixProvider(ConditionsStore &store, const std::string &prefix)
	{
		auto &&conditionsProvider = store.GetProviderPrefixed(prefix);
		conditionsProvider.SetHasFunction([this, prefix](const std::string &name)
		{
			verifyAndStripPrefix(prefix, name);
			return isInMap(values, name);
		});
		conditionsProvider.SetSetFunction([this, prefix](const std::string &name, int64_t value)
		{
			verifyAndStripPrefix(prefix, name);
			values[name] = value;
			return true;
		});
		conditionsProvider.SetEraseFunction([this, prefix](const std::string &name)
		{
			verifyAndStripPrefix(prefix, name);
			values.erase(name);
			return true;
		});
		conditionsProvider.SetGetFunction([this, prefix](const std::string &name)
		{
			verifyAndStripPrefix(prefix, name);
			return getFromMapOrZero(values, name);
		});
	}
	void SetRONamedProvider(ConditionsStore &store, const std::string &named)
	{
		auto &&conditionsProvider = store.GetProviderNamed(named);
		conditionsProvider.SetHasFunction([this, named](const std::string &name)
		{
			verifyName(named, name);
			return isInMap(values, name);
		});
		conditionsProvider.SetSetFunction([named](const std::string &name, int64_t value)
		{
			verifyName(named, name);
			return false;
		});
		conditionsProvider.SetEraseFunction([named](const std::string &name)
		{
			verifyName(named, name);
			return false;
		});
		conditionsProvider.SetGetFunction([this, named](const std::string &name)
		{
			verifyName(named, name);
			return getFromMapOrZero(values, name);
		});
	}
	void SetRWNamedProvider(ConditionsStore &store, const std::string &named)
	{
		auto &&conditionsProvider = store.GetProviderNamed(named);
		conditionsProvider.SetHasFunction([this, named](const std::string &name)
		{
			verifyName(named, name);
			return isInMap(values, name);
		});
		conditionsProvider.SetSetFunction([this, named](const std::string &name, int64_t value)
		{
			verifyName(named, name);
			values[name] = value;
			return true;
		});
		conditionsProvider.SetEraseFunction([this, named](const std::string &name)
		{
			verifyName(named, name);
			values.erase(name);
			return true;
		});
		conditionsProvider.SetGetFunction([this, named](const std::string &name)
		{
			verifyName(named, name);
			return getFromMapOrZero(values, name);
		});
	}

public:
	std::map<std::string, int64_t> values;
};


// #endregion mock data



// #region unit tests
SCENARIO( "Creating a ConditionsStore", "[ConditionsStore][Creation]" )
{
	GIVEN( "A ConditionStore" )
	{
		WHEN( "it is just default initalized" )
		{
			const auto store = ConditionsStore();
			THEN( "the store is empty" )
			{
				REQUIRE( primarySize(store) == 0 );
			}
			THEN( "two begin iterators should be equal" )
			{
				REQUIRE( store.PrimariesBegin() == store.PrimariesBegin() );
			}
			THEN( "two end iterators should be equal" )
			{
				REQUIRE( store.PrimariesEnd() == store.PrimariesEnd() );
			}
			THEN( "the begin and end iterators should be equal" )
			{
				REQUIRE( store.PrimariesBegin() == store.PrimariesEnd() );
				REQUIRE( store.PrimariesEnd() == store.PrimariesBegin() );
				auto it = store.PrimariesBegin();
				REQUIRE( it == store.PrimariesEnd() );
				REQUIRE( store.PrimariesEnd() == it );
				it = store.PrimariesEnd();
				REQUIRE( it == store.PrimariesBegin() );
				REQUIRE( store.PrimariesBegin() == it );
			}
		}
		WHEN( "initialized using an initalizer list" )
		{
			const auto store = ConditionsStore{ { "hello world", 100 }, { "goodbye world", 404 } };
			THEN( "given primary conditions are in the Store" )
			{
				REQUIRE( store.Get("hello world") == 100 );
				REQUIRE( store.Get("goodbye world") == 404 );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( primarySize(store) == 2 );
			}
			THEN( "not given conditions return the default value" )
			{
				REQUIRE( 0 == store.Get("ungreeted world") );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.Get("ungreeted world") == 0 );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( 0 == store.Get("hi world") );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.Get("hi world") == 0 );
				REQUIRE( primarySize(store) == 2 );
			}
			THEN( "two iterators to the same position should be equal" )
			{
				REQUIRE( store.PrimariesBegin() == store.PrimariesBegin() );
				REQUIRE( store.PrimariesEnd() == store.PrimariesEnd() );
			}
			THEN( "iterating over the conditions should return all initial values" )
			{
				auto it = store.PrimariesBegin();
				REQUIRE( it != store.PrimariesEnd() );
				REQUIRE( it->first == "goodbye world" );
				REQUIRE( it->second == 404 );
				++it;
				REQUIRE( it->first == "hello world" );
				REQUIRE( it->second == 100 );
				it++;
				REQUIRE( it == store.PrimariesEnd() );
			}
			THEN( "doing lower_bound finds should return values above the bound" )
			{
				auto it = store.PrimariesLowerBound("ha");
				REQUIRE( it != store.PrimariesEnd() );
				REQUIRE( it->first == "hello world" );
				REQUIRE( it->second == 100 );
				++it;
				REQUIRE( it == store.PrimariesEnd() );
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
				REQUIRE( primarySize(store) == 2 );
			}
			THEN( "not given conditions return the default value" )
			{
				REQUIRE( store.Get("ungreeted world") == 0 );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.Get("ungreeted world") == 0 );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( 0 == store.Get("hi world") );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.Get("hi world") == 0 );
				REQUIRE( primarySize(store) == 2 );
			}
		}
		WHEN( "initialized with a long initializer list" )
		{
			const auto store = ConditionsStore{ { "a", 1 }, { "b", 2 }, { "d", 4 }, { "c", 3 }, { "g", 7 },
				{ "f", 6 }, { "e", 5 } };
			THEN( "iterating from the beginning should pass all conditions and reach the end" )
			{
				auto it = store.PrimariesBegin();
				REQUIRE( (it->first == "a" && it->second == 1) );
				++it;
				REQUIRE( (it->first == "b" && it->second == 2) );
				it++;
				REQUIRE( (it->first == "c" && it->second == 3) );
				REQUIRE( (++it)->first == "d" );
				REQUIRE( (it->first == "d" && it->second == 4) );
				REQUIRE( (it++)->first == "d" );
				REQUIRE( (it->first == "e" && it->second == 5) );
				++it;
				REQUIRE( (it->first == "f" && it->second == 6) );
				it++;
				REQUIRE( (it->first == "g" && it->second == 7) );
				it++;
				REQUIRE( it == store.PrimariesEnd() );
			}
		}
	}
}

SCENARIO( "Setting and erasing conditions", "[ConditionStore][ConditionSetting]" )
{
	GIVEN( "An empty conditionsStore" )
	{
		auto store = ConditionsStore();
		REQUIRE( primarySize(store) == 0 );
		WHEN( "a condition is set" )
		{
			REQUIRE( store.Set("myFirstVar", 10) == true );
			THEN( "stored condition is present and can be retrieved" )
			{
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.Has("myFirstVar") == true );
				REQUIRE( primarySize(store) == 1 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store["myFirstVar"] == 10 );
			}
			THEN( "erasing the condition will make it disappear again" )
			{
				REQUIRE( store.Has("myFirstVar") == true );
				REQUIRE( primarySize(store) == 1 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( primarySize(store) == 1 );
				REQUIRE( store.Erase("myFirstVar") == true );
				REQUIRE( store.Has("myFirstVar") == false );
				REQUIRE( primarySize(store) == 0 );
				REQUIRE( store.Get("myFirstVar") == 0 );
				REQUIRE( store.Has("myFirstVar") == false );
				REQUIRE( primarySize(store) == 0 );
			}
		}
		WHEN( "non-existing conditions are queried" )
		{
			THEN( "defaults are returned and queried conditions are not stored" )
			{
				REQUIRE( store.Get("mySecondVar") == 0 );
				REQUIRE( primarySize(store) == 0 );
				REQUIRE( store.Has("mySecondVar") == false );
				REQUIRE( store.Get("mySecondVar") == 0 );
				REQUIRE( primarySize(store) == 0 );
			}
			THEN( "they get created when accessed through square brackets" )
			{
				REQUIRE( store["mySecondVar"] == 0 );
				REQUIRE( primarySize(store) == 1 );
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
		REQUIRE( primarySize(store) == 1 );
		WHEN( "adding to the existing primary condition" )
		{
			REQUIRE( store.Add("myFirstVar", 10) == true );
			THEN( "the condition gets the new value" )
			{
				REQUIRE( store.Get("myFirstVar") == 20 );
				REQUIRE( store.Add("myFirstVar", -15) == true );
				REQUIRE( store.Get("myFirstVar") == 5 );
				REQUIRE( store.Add("myFirstVar", -15) == true );
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
			REQUIRE( store.Add("mySecondVar", -30) == true );
			THEN( "the new condition exists with the new value" )
			{
				REQUIRE( store.Get("mySecondVar") == -30 );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.Has("mySecondVar") == true );
				REQUIRE( store.Add("mySecondVar", 60) == true );
				REQUIRE( store.Get("mySecondVar") == 30 );
				REQUIRE( primarySize(store) == 2 );
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
		REQUIRE( store.Add("named1", -30) == true );
		REQUIRE( mockProvNamed.values.size() == 1 );
		REQUIRE( mockProvNamed.values["named1"] == -30 );
		REQUIRE( primarySize(store) == 1 );
		REQUIRE( mockProvPrefixA.values.size() == 0 );
		REQUIRE( store.Add("prefixA: test", -30) == true );
		REQUIRE( primarySize(store) == 1 );
		REQUIRE( mockProvPrefixA.values.size() == 1 );
		REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
		REQUIRE( mockProvNamed.values.size() == 1 );
		REQUIRE( primarySize(store) == 1 );
		WHEN( "adding to an existing primary condition" )
		{
			THEN( "the add should work as-is" )
			{
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.Add("myFirstVar", 10) == true );
				REQUIRE( store.Get("myFirstVar") == 20 );
				REQUIRE( store.Add("myFirstVar", -15) == true );
				REQUIRE( store.Get("myFirstVar") == 5 );
				REQUIRE( store.Add("myFirstVar", -15) == true );
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
				REQUIRE( store.Add("mySecondVar", -30) == true );
				REQUIRE( store.Get("mySecondVar") == -30 );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.Has("mySecondVar") == true );
				REQUIRE( store.Add("mySecondVar", 60) == true );
				REQUIRE( store.Get("mySecondVar") == 30 );
				REQUIRE( primarySize(store) == 2 );
			}
		}
		WHEN( "iterating over primary conditions" )
		{
			auto it = store.PrimariesBegin();
			THEN( "only the stored primary condition should be returned" )
			{
				REQUIRE( it->first == "myFirstVar" );
				REQUIRE( it->second == 10 );
				++it;
				REQUIRE( it == store.PrimariesEnd() );
			}
			THEN( "primary-lowerBound should only search in primary conditions" )
			{
				it = store.PrimariesLowerBound("n");
				REQUIRE( it == store.PrimariesEnd() );
				it = store.PrimariesLowerBound("l");
				REQUIRE( it != store.PrimariesEnd() );
				REQUIRE( it->first == "myFirstVar" );
				REQUIRE( it->second == 10 );
				++it;
				REQUIRE( it == store.PrimariesEnd() );
			}
		}
		WHEN( "adding on a named derived condition" )
		{
			REQUIRE( store.Add("named1", -30) == true );
			THEN( "effects of adding should be on the named condition" )
			{
				REQUIRE( primarySize(store) == 1 );
				REQUIRE( mockProvNamed.values["named1"] == -60 );
				REQUIRE( store.Add("named1", -20) == true );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( mockProvNamed.values["named1"] == -80 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( store.Get("named1") == -80 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.Get("mySecondVar") == 0 );
				REQUIRE( store["named1"] == -80 );
				REQUIRE( store["myFirstVar"] == 10 );
				REQUIRE( primarySize(store) == 1 );
				--store["named1"];
				++store["myFirstVar"];
				REQUIRE( store.Get("named1") == -81 );
				REQUIRE( store.Get("myFirstVar") == 11 );
				REQUIRE( store.Get("mySecondVar") == 0 );
			}
			THEN( "readonly providers should reject the add and don't change values" )
			{
				mockProvNamed.SetRONamedProvider(store, "named1");
				REQUIRE( store.Add("named1", -20) == false );
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
			THEN( "readonly providers should not perform erase actions" )
			{
				mockProvNamed.SetRONamedProvider(store, "named1");
				REQUIRE( store.Erase("named1") == false );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( mockProvNamed.values["named1"] == -60 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( store.Get("named1") == -60 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.Has("named1") == true );
			}
			THEN( "not given conditions (that almost match the named condition) should not exist" )
			{
				REQUIRE( store.Has("named") == false );
				REQUIRE( store.Has("named11") == false );
			}
		}
		WHEN( "adding on a prefixed derived condition" )
		{
			REQUIRE( store.Add("prefixA: test", -30) == true );
			THEN( "derived prefixed conditions should be set properly" )
			{
				REQUIRE( primarySize(store) == 1 );
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
				REQUIRE( primarySize(store) == 1 );
			}
			THEN( "read-only prefixed provider should reject further updates" )
			{
				mockProvPrefixA.SetROPrefixProvider(store, "prefixA: ");
				REQUIRE( store.Add("prefixA: test", -20) == false );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( mockProvPrefixA.values["prefixA: test"] == -60 );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( store.Get("prefixA: test") == -60 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				store["prefixA: test"] -= 20;
				REQUIRE( store.Get("prefixA: test") == -60 );
				REQUIRE( store.Get("myFirstVar") == 10 );
			}
			THEN( "read-only prefixed provider should reject erase" )
			{
				mockProvPrefixA.SetROPrefixProvider(store, "prefixA: ");
				REQUIRE( store.Erase("prefixA: test") == false );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( mockProvPrefixA.values["prefixA: test"] == -60 );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( store.Get("prefixA: test") == -60 );
				REQUIRE( store.Get("myFirstVar") == 10 );
				REQUIRE( store.Has("prefixA: test") == true );
				REQUIRE( store.Has("prefixA: t") == false );
				REQUIRE( store.Has("prefixA: ") == false );
				REQUIRE( store.Has("prefixA:") == false );
			}
			THEN( "prefixed values from within provider should be available" )
			{
				mockProvPrefixA.values["prefixA: "] = 22;
				mockProvPrefixA.values["prefixA:"] = 21;
				REQUIRE( store.Has("prefixA: test") == true );
				REQUIRE( store.Has("prefixA: t") == false );
				REQUIRE( store.Has("prefixA: ") == true );
				REQUIRE( store.Has("prefixA:") == false );
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
					REQUIRE( primarySize(store) == 1 );
					REQUIRE( store.Add("prefixA: test", 30) == true );
					REQUIRE( primarySize(store) == 1 );
					REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
					REQUIRE( store.Get("prefixA: test") == -30 );
					REQUIRE( store.Get("myFirstVar") == 10 );
					mockProvPrefixA.SetROPrefixProvider(store, "prefixA: ");
					REQUIRE( store.Add("prefixA: test", -20) == false );
					REQUIRE( mockProvPrefixA.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
					REQUIRE( store.Get("prefixA: test") == -30 );
					REQUIRE( store.Get("myFirstVar") == 10 );
					REQUIRE( store.Erase("prefixA: test") == false );
					REQUIRE( mockProvPrefixA.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
					REQUIRE( store.Get("prefixA: test") == -30 );
					REQUIRE( store.Get("myFirstVar") == 10 );
					REQUIRE( store.Has("prefixA: test") == true );
					REQUIRE( store.Has("prefixA: t") == false );
					REQUIRE( store.Has("prefixA: ") == false );
					REQUIRE( store.Has("prefixA:") == false );
					mockProvPrefixA.values["prefixA: "] = 22;
					mockProvPrefixA.values["prefixA:"] = 21;
					REQUIRE( store.Has("prefixA: test") == true );
					REQUIRE( store.Has("prefixA: t") == false );
					REQUIRE( store.Has("prefixA: ") == true );
					REQUIRE( store.Has("prefixA:") == false );
					REQUIRE( mockProvPrefix.values.size() == 0 );
					REQUIRE( mockProvPrefixA.values.size() == 3 );
					REQUIRE( mockProvPrefixB.values.size() == 0 );
					mockProvPrefixA.SetRWPrefixProvider(store, "prefixA: ");
					REQUIRE( store.Set("prefix: beginning", 42) == true );
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 3 );
					REQUIRE( mockProvPrefixB.values.size() == 0 );
					REQUIRE( store.Set("prefixB: ending", 142) == true );
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 3 );
					REQUIRE( mockProvPrefixB.values.size() == 1 );
					REQUIRE( store.Set("prefixA: middle", 40) == true );
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 4 );
					REQUIRE( mockProvPrefixB.values.size() == 1 );
					REQUIRE( store.Set("prefixA: middle2", 90) == true );
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
					REQUIRE( primarySize(store) == 1 );
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
			REQUIRE( store.Add("ships: A", 20) == true );
			REQUIRE( mockProvPrefixShips.values.size() == 1 );
			REQUIRE( mockProvPrefixShips.values["ships: A"] == 20 );
			REQUIRE( store.Add("ships: AB", 30) == true );
			REQUIRE( mockProvPrefixShips.values.size() == 2 );
			REQUIRE( mockProvPrefixShips.values["ships: AB"] == 30 );
			REQUIRE( store.Add("ships: C", 40) == true );
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
	}
}
// #endregion unit tests



} // test namespace
