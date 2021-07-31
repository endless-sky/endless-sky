#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/ConditionsStore.h"

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
	while (it != store.PrimariesEnd())
	{
		++it;
		++size;
	}
	return size;
}


class MockConditionsProvider
{
public:
	struct ConditionsStore::DerivedProvider GetROPrefixProvider(const std::string &prefix)
	{
		struct ConditionsStore::DerivedProvider conditionsProvider;
		conditionsProvider.hasFun = [this, prefix] (const std::string &name) { verifyAndStripPrefix(prefix, name); return isInMap(values, name); };
		conditionsProvider.setFun = [this, prefix] (const std::string &name, int64_t value) { verifyAndStripPrefix(prefix, name); return false; };
		conditionsProvider.eraseFun = [this, prefix] (const std::string &name) { verifyAndStripPrefix(prefix, name); return false; };
		conditionsProvider.getFun = [this, prefix] (const std::string &name) { verifyAndStripPrefix(prefix, name); return getFromMapOrZero(values, name); };
		return conditionsProvider;
	}
	struct ConditionsStore::DerivedProvider GetRWPrefixProvider(const std::string &prefix)
	{
		struct ConditionsStore::DerivedProvider conditionsProvider;
		conditionsProvider.hasFun = [this, prefix] (const std::string &name) { verifyAndStripPrefix(prefix, name); return isInMap(values, name); };
		conditionsProvider.setFun = [this, prefix] (const std::string &name, int64_t value) { verifyAndStripPrefix(prefix, name); values[name] = value; return true; };
		conditionsProvider.eraseFun = [this, prefix] (const std::string &name) { verifyAndStripPrefix(prefix, name); values.erase(name); return true; };
		conditionsProvider.getFun = [this, prefix] (const std::string &name) { verifyAndStripPrefix(prefix, name); return getFromMapOrZero(values, name); };
		return conditionsProvider;
	}
	struct ConditionsStore::DerivedProvider GetRONamedProvider(const std::string &named)
	{
		struct ConditionsStore::DerivedProvider conditionsProvider;
		conditionsProvider.hasFun = [this, named] (const std::string &name) { verifyName(named, name); return isInMap(values, name); };
		conditionsProvider.setFun = [this, named] (const std::string &name, int64_t value) { verifyName(named, name); return false; };
		conditionsProvider.eraseFun = [this, named] (const std::string &name) { verifyName(named, name); return false; };
		conditionsProvider.getFun = [this, named] (const std::string &name) { verifyName(named, name); return getFromMapOrZero(values, name); };
		return conditionsProvider;
	}
	struct ConditionsStore::DerivedProvider GetRWNamedProvider(const std::string &named)
	{
		struct ConditionsStore::DerivedProvider conditionsProvider;
		conditionsProvider.hasFun = [this, named] (const std::string &name) { verifyName(named, name); return isInMap(values, name); };
		conditionsProvider.setFun = [this, named] (const std::string &name, int64_t value) { verifyName(named, name); values[name] = value; return true; };
		conditionsProvider.eraseFun = [this, named] (const std::string &name) { verifyName(named, name); values.erase(name); return true; };
		conditionsProvider.getFun = [this, named] (const std::string &name) { verifyName(named, name); return getFromMapOrZero(values, name); };
		return conditionsProvider;
	}

public:
	std::map<std::string, int64_t> values;
};


// #endregion mock data



// #region unit tests
SCENARIO( "Creating a ConditionsStore" , "[ConditionsStore][Creation]" ) {
	GIVEN( "A ConditionStore" ) {
		WHEN( "it is just default initalized" ) {
			const auto store = ConditionsStore();
			THEN( "the store is empty" ) {
				REQUIRE( primarySize(store) == 0 );
			}
			THEN( "two begin iterators should be equal" ) {
				REQUIRE( store.PrimariesBegin() == store.PrimariesBegin() );
			}
			THEN( "two end iterators should be equal" ) {
				REQUIRE( store.PrimariesEnd() == store.PrimariesEnd() );
			}
			THEN( "the begin and end iterators should be equal" ) {
				REQUIRE( store.PrimariesBegin() == store.PrimariesEnd() );
				REQUIRE( store.PrimariesEnd() == store.PrimariesBegin() );
				auto it = store.PrimariesBegin();
				REQUIRE( it == store.PrimariesEnd() );
				REQUIRE( store.PrimariesEnd() == it );
				it == store.PrimariesEnd();
				REQUIRE( it == store.PrimariesBegin() );
				REQUIRE( store.PrimariesBegin() == it );
			}
		}
		WHEN( "initialized using an initalizer list" ) {
			const auto store = ConditionsStore{{"hello world", 100}, {"goodbye world", 404}};
			THEN( "given primary conditions are in the Store" ) {
				REQUIRE( store.GetCondition("hello world") == 100 );
				REQUIRE( store.GetCondition("goodbye world") == 404 );
				REQUIRE( primarySize(store) == 2 );
			}
			THEN( "not given conditions return the default value" ) {
				REQUIRE( 0 == store.GetCondition("ungreeted world") );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.GetCondition("ungreeted world") == 0 );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( 0 == store.GetCondition("hi world") );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.GetCondition("hi world") == 0 );
				REQUIRE( primarySize(store) == 2 );
			}
			THEN( "two iterators to the same position should be equal") {
				REQUIRE( store.PrimariesBegin() == store.PrimariesBegin() );
				REQUIRE( store.PrimariesEnd() == store.PrimariesEnd() );
			}
			THEN( "iterating over the conditions should return all initial values") {
				auto it = store.PrimariesBegin();
				REQUIRE( it != store.PrimariesEnd());
				REQUIRE( it->first == "goodbye world");
				REQUIRE( it->second == 404 );
				++it;
				REQUIRE( it->first == "hello world");
				REQUIRE( it->second == 100 );
				it++;
				REQUIRE( it == store.PrimariesEnd());
			}
			THEN( "doing lower_bound finds should return values above the bound") {
				auto it = store.PrimariesLowerBound("ha");
				REQUIRE( it != store.PrimariesEnd());
				REQUIRE( it->first == "hello world");
				REQUIRE( it->second == 100 );
				++it;
				REQUIRE( it == store.PrimariesEnd());
			}
		}
		WHEN( "initialized using an initializer map" ) {
			const std::map<std::string, int64_t> initmap {{"hello world", 100}, {"goodbye world", 404}};
			const auto store = ConditionsStore(initmap);
			THEN( "given primary conditions are in the Store" ) {
				REQUIRE( store.GetCondition("hello world") == 100 );
				REQUIRE( store.GetCondition("goodbye world") == 404 );
				REQUIRE( primarySize(store) == 2 );
			}
			THEN( "not given conditions return the default value" ) {
				REQUIRE( store.GetCondition("ungreeted world") == 0 );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.GetCondition("ungreeted world") == 0 );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( 0 == store.GetCondition("hi world") );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.GetCondition("hi world") == 0 );
				REQUIRE( primarySize(store) == 2 );
			}
		}
		WHEN( "initialized with a long initializer list" ) {
			const auto store = ConditionsStore{{"a", 1}, {"b", 2}, {"d", 4}, {"c", 3}, {"g", 7}, {"f", 6}, {"e", 5}};
			THEN( "iterating from the beginning should pass all conditions and reach the end")
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
				REQUIRE( it == store.PrimariesEnd());
			}
		}
	}
}

SCENARIO( "Setting and erasing conditions", "[ConditionStore][ConditionSetting]" ) {
	GIVEN( "An empty conditionsStore" ) {
		auto store = ConditionsStore();
		REQUIRE( primarySize(store) == 0 );
		WHEN( "a condition is set" ) {
			REQUIRE( store.SetCondition("myFirstVar", 10) == true );
			THEN( "stored condition is present and can be retrieved" ) {
				REQUIRE( store.GetCondition("myFirstVar") == 10 );
				REQUIRE( store.HasCondition("myFirstVar") == true );
				REQUIRE( primarySize(store) == 1 );
				REQUIRE( store.GetCondition("myFirstVar") == 10 );
			}
			THEN( "erasing the condition will make it disappear again" ) {
				REQUIRE( store.HasCondition("myFirstVar") == true );
				REQUIRE( primarySize(store) == 1 );
				REQUIRE( store.GetCondition("myFirstVar") == 10 );
				REQUIRE( primarySize(store) == 1 );
				REQUIRE( store.EraseCondition("myFirstVar") == true );
				REQUIRE( store.HasCondition("myFirstVar") == false );
				REQUIRE( primarySize(store) == 0 );
				REQUIRE( store.GetCondition("myFirstVar") == 0 );
				REQUIRE( store.HasCondition("myFirstVar") == false );
				REQUIRE( primarySize(store) == 0 );
			}
		}
		WHEN( "non-existing conditions are queried" ) {
			THEN( "defaults are returned and queried conditions are not stored" ) {
				REQUIRE( store.GetCondition("mySecondVar") == 0 );
				REQUIRE( primarySize(store) == 0 );
				REQUIRE( store.HasCondition("mySecondVar") == false );
				REQUIRE( store.GetCondition("mySecondVar") == 0 );
				REQUIRE( primarySize(store) == 0 );
			}
		}
	}
}

SCENARIO( "Adding and removing on condition values", "[ConditionStore][ConditionArithmetic]" ) {
	GIVEN( "A conditionsStore with 1 condition" ) {
		auto store = ConditionsStore{{"myFirstVar", 10}};
		REQUIRE( store.GetCondition("myFirstVar") == 10 );
		REQUIRE( primarySize(store) == 1 );
		WHEN( "adding to the existing primary condition" ) {
			REQUIRE( store.AddCondition("myFirstVar", 10) == true );
			THEN( "the condition gets the new value" ) {
				REQUIRE( store.GetCondition("myFirstVar") == 20 );
				REQUIRE( store.AddCondition("myFirstVar", -15) == true );
				REQUIRE( store.GetCondition("myFirstVar") == 5 );
				REQUIRE( store.AddCondition("myFirstVar", -15) == true );
				REQUIRE( store.GetCondition("myFirstVar") == -10 );
			}
		}
		WHEN( "adding to another non-existing (primary) condition sets the new value" ) {
			REQUIRE( store.AddCondition("mySecondVar", -30) == true );
			THEN( "the new condition exists with the new value" ) {
				REQUIRE( store.GetCondition("mySecondVar") == -30 );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.HasCondition("mySecondVar") == true );
				REQUIRE( store.AddCondition("mySecondVar", 60) == true );
				REQUIRE( store.GetCondition("mySecondVar") == 30 );
				REQUIRE( primarySize(store) == 2 );
			}
		}
	}
}

SCENARIO( "Providing derived conditions", "[ConditionStore][DerivedConditions]" ) {
	GIVEN( "A conditionsStore, a prefixed provider and a named provider" ) {
		auto mockProvPrefixA = MockConditionsProvider();
		auto mockProvNamed = MockConditionsProvider();
		auto store = ConditionsStore{{"myFirstVar", 10}};
		store.SetProviderNamed("named1", mockProvNamed.GetRWNamedProvider("named1"));
		store.SetProviderPrefixed("prefixA: ", mockProvPrefixA.GetRWPrefixProvider("prefixA: "));
		REQUIRE( store.AddCondition("named1", -30) == true );
		REQUIRE( mockProvNamed.values["named1"] == -30 );
		REQUIRE( mockProvNamed.values.size() == 1 );
		REQUIRE( primarySize(store) == 1 );
		REQUIRE( mockProvPrefixA.values.size() == 0 );
		REQUIRE( store.AddCondition("prefixA: test", -30) == true );
		REQUIRE( primarySize(store) == 1 );
		REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
		REQUIRE( mockProvPrefixA.values.size() == 1 );
		REQUIRE( mockProvNamed.values.size() == 1 );
		REQUIRE( primarySize(store) == 1 );
		WHEN( "adding to an existing primary condition" ) {
			THEN( "the add should work as-is" ) {
				REQUIRE( store.GetCondition("myFirstVar") == 10 );
				REQUIRE( store.AddCondition("myFirstVar", 10) == true );
				REQUIRE( store.GetCondition("myFirstVar") == 20 );
				REQUIRE( store.AddCondition("myFirstVar", -15) == true );
				REQUIRE( store.GetCondition("myFirstVar") == 5 );
				REQUIRE( store.AddCondition("myFirstVar", -15) == true );
				REQUIRE( store.GetCondition("myFirstVar") == -10 );
			}
		}
		WHEN( "adding to an non-existing primary condition" ) {
			THEN( "such condition should be set properly" ) {
				REQUIRE( store.AddCondition("mySecondVar", -30) == true );
				REQUIRE( store.GetCondition("mySecondVar") == -30 );
				REQUIRE( primarySize(store) == 2 );
				REQUIRE( store.HasCondition("mySecondVar") == true );
				REQUIRE( store.AddCondition("mySecondVar", 60) == true );
				REQUIRE( store.GetCondition("mySecondVar") == 30 );
				REQUIRE( primarySize(store) == 2 );
			}
		}
		WHEN( "iterating over primary conditions" ) {
			auto it = store.PrimariesBegin();
			THEN( "only the stored primary condition should be returned" ) {
				REQUIRE( it->first == "myFirstVar" );
				REQUIRE( it->second == 10 );
				++it;
				REQUIRE( it == store.PrimariesEnd() );
			}
			THEN( "primary-lowerBound should only search in primary conditions" ) {
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
		WHEN( "adding on a named derived condition" ) {
			REQUIRE( store.AddCondition("named1", -30) == true );
			THEN( "effects of adding should be on the named condition" ) {
				REQUIRE( primarySize(store) == 1 );
				REQUIRE( mockProvNamed.values["named1"] == -60 );
				REQUIRE( store.AddCondition("named1", -20) == true );
				REQUIRE( mockProvNamed.values["named1"] == -80 );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( store.GetCondition("named1") == -80 );
				REQUIRE( store.GetCondition("myFirstVar") == 10 );
				REQUIRE( store.GetCondition("mySecondVar") == 0 );
			}
			THEN( "readonly providers should reject the add and don't change values" ) {
				store.SetProviderNamed("named1", mockProvNamed.GetRONamedProvider("named1"));
				REQUIRE( store.AddCondition("named1", -20) == false );
				REQUIRE( mockProvNamed.values["named1"] == -60 );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( store.GetCondition("named1") == -60 );
				REQUIRE( store.GetCondition("myFirstVar") == 10 );
				REQUIRE( store.GetCondition("mySecondVar") == 0 );
			}
			THEN( "readonly providers should not perform erase actions" ) {
				store.SetProviderNamed("named1", mockProvNamed.GetRONamedProvider("named1"));
				REQUIRE( store.EraseCondition("named1") == false );
				REQUIRE( mockProvNamed.values["named1"] == -60 );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( store.GetCondition("named1") == -60 );
				REQUIRE( store.GetCondition("myFirstVar") == 10 );
				REQUIRE( store.HasCondition("named1") == true );
			}
			THEN( "not given conditions (that almost match the named condition) should not exist" ) {
				REQUIRE( store.HasCondition("named") == false );
				REQUIRE( store.HasCondition("named11") == false );
			}
		}
		WHEN( "adding on a prefixed derived condition" ) {
			REQUIRE( store.AddCondition("prefixA: test", -30) == true );
			THEN( "derived prefixed conditions should be set properly" ) {
				REQUIRE( primarySize(store) == 1 );
				REQUIRE( mockProvPrefixA.values["prefixA: test"] == -60 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( store.GetCondition("prefixA: test") == -60 );
				REQUIRE( store.GetCondition("myFirstVar") == 10 );
				REQUIRE( store.GetCondition("mySecondVar") == 0 );
			}
			THEN( "read-only prefixed provider should reject further updates" ) {
				store.SetProviderPrefixed("prefixA: ", mockProvPrefixA.GetROPrefixProvider("prefixA: "));
				REQUIRE( store.AddCondition("prefixA: test", -20) == false );
				REQUIRE( mockProvPrefixA.values["prefixA: test"] == -60 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( store.GetCondition("prefixA: test") == -60 );
				REQUIRE( store.GetCondition("myFirstVar") == 10 );
			}
			THEN( "read-only prefixed provider should reject erase" ) {
				store.SetProviderPrefixed("prefixA: ", mockProvPrefixA.GetROPrefixProvider("prefixA: "));
				REQUIRE( store.EraseCondition("prefixA: test") == false );
				REQUIRE( mockProvPrefixA.values["prefixA: test"] == -60 );
				REQUIRE( mockProvPrefixA.values.size() == 1 );
				REQUIRE( mockProvNamed.values.size() == 1 );
				REQUIRE( store.GetCondition("prefixA: test") == -60 );
				REQUIRE( store.GetCondition("myFirstVar") == 10 );
				REQUIRE( store.HasCondition("prefixA: test") == true );
				REQUIRE( store.HasCondition("prefixA: t") == false );
				REQUIRE( store.HasCondition("prefixA: ") == false );
				REQUIRE( store.HasCondition("prefixA:") == false );
			}
			THEN( "prefixed values from within provider should be available" ) {
				mockProvPrefixA.values["prefixA: "] = 22;
				mockProvPrefixA.values["prefixA:"] = 21;
				REQUIRE( store.HasCondition("prefixA: test") == true );
				REQUIRE( store.HasCondition("prefixA: t") == false );
				REQUIRE( store.HasCondition("prefixA: ") == true );
				REQUIRE( store.HasCondition("prefixA:") == false );
				REQUIRE( store.GetCondition("prefixA: ") == 22 );
				REQUIRE( store.GetCondition("prefixA:") == 0 );
				REQUIRE( store.GetCondition("prefixA: test") == -60 );
			}
			AND_GIVEN( "more derived condition providers are added" ) {
				auto mockProvPrefix = MockConditionsProvider();
				store.SetProviderPrefixed("prefix: ", mockProvPrefix.GetRWPrefixProvider("prefix: "));
				auto mockProvPrefixB = MockConditionsProvider();
				store.SetProviderPrefixed("prefixB: ", mockProvPrefixB.GetRWPrefixProvider("prefixB: "));
				THEN( "derived prefixed conditions should be set properly" ) {
					REQUIRE( primarySize(store) == 1 );
					REQUIRE( store.AddCondition("prefixA: test", 30) == true );
					REQUIRE( primarySize(store) == 1 );
					REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
					REQUIRE( store.GetCondition("prefixA: test") == -30 );
					REQUIRE( store.GetCondition("myFirstVar") == 10 );
					store.SetProviderPrefixed("prefixA: ", mockProvPrefixA.GetROPrefixProvider("prefixA: "));
					REQUIRE( store.AddCondition("prefixA: test", -20) == false );
					REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
					REQUIRE( mockProvPrefixA.values.size() == 1 );
					REQUIRE( store.GetCondition("prefixA: test") == -30 );
					REQUIRE( store.GetCondition("myFirstVar") == 10 );
					REQUIRE( store.EraseCondition("prefixA: test") == false );
					REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
					REQUIRE( mockProvPrefixA.values.size() == 1 );
					REQUIRE( store.GetCondition("prefixA: test") == -30 );
					REQUIRE( store.GetCondition("myFirstVar") == 10 );
					REQUIRE( store.HasCondition("prefixA: test") == true );
					REQUIRE( store.HasCondition("prefixA: t") == false );
					REQUIRE( store.HasCondition("prefixA: ") == false );
					REQUIRE( store.HasCondition("prefixA:") == false );
					mockProvPrefixA.values["prefixA: "] = 22;
					mockProvPrefixA.values["prefixA:"] = 21;
					REQUIRE( store.HasCondition("prefixA: test") == true );
					REQUIRE( store.HasCondition("prefixA: t") == false );
					REQUIRE( store.HasCondition("prefixA: ") == true );
					REQUIRE( store.HasCondition("prefixA:") == false );
					REQUIRE( mockProvPrefix.values.size() == 0 );
					REQUIRE( mockProvPrefixA.values.size() == 3 );
					REQUIRE( mockProvPrefixB.values.size() == 0 );
					store.SetProviderPrefixed("prefixA: ", mockProvPrefixA.GetRWPrefixProvider("prefixA: "));
					REQUIRE( store.SetCondition("prefix: beginning", 42) == true );
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 3 );
					REQUIRE( mockProvPrefixB.values.size() == 0 );
					REQUIRE( store.SetCondition("prefixB: ending", 142) == true );
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 3 );
					REQUIRE( mockProvPrefixB.values.size() == 1 );
					REQUIRE( store.SetCondition("prefixA: middle", 40) == true );
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 4 );
					REQUIRE( mockProvPrefixB.values.size() == 1 );
					REQUIRE( store.SetCondition("prefixA: middle2", 90) == true );
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 5 );
					REQUIRE( mockProvPrefixB.values.size() == 1 );
					REQUIRE( store.GetCondition("prefix: beginning") == 42 );
					REQUIRE( store.GetCondition("prefixB: ending") == 142 );
					REQUIRE( store.GetCondition("prefixA: ") == 22 );
					REQUIRE( store.GetCondition("prefixA:") == 0 );
					REQUIRE( store.GetCondition("prefixA: middle") == 40 );
					REQUIRE( store.GetCondition("prefixA: middle2") == 90 );
					REQUIRE( store.GetCondition("prefixA: test") == -30 );
					REQUIRE( store.GetCondition("myFirstVar") == 10 );
					REQUIRE( mockProvPrefix.values.size() == 1 );
					REQUIRE( mockProvPrefixA.values.size() == 5 );
					REQUIRE( mockProvPrefixB.values.size() == 1 );
					REQUIRE( primarySize(store) == 1 );
				}
			}
		}
	}
}



// #endregion unit tests



} // test namespace
