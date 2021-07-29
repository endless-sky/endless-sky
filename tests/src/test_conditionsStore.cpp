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


class MockConditionsProvider
{
public:
	struct ConditionsStore::DerivedProvider GetROPrefixProvider(const std::string &prefix)
	{
		struct ConditionsStore::DerivedProvider conditionsProvider;
		conditionsProvider.hasFun = [this, prefix] (const std::string &name){ verifyAndStripPrefix(prefix, name); return isInMap(values, name); };
		conditionsProvider.setFun = [this, prefix] (const std::string &name, int64_t value){ verifyAndStripPrefix(prefix, name); return false; };
		conditionsProvider.eraseFun = [this, prefix] (const std::string &name){ verifyAndStripPrefix(prefix, name); return false; };
		conditionsProvider.getFun = [this, prefix] (const std::string &name){ verifyAndStripPrefix(prefix, name); return getFromMapOrZero(values, name); };
		return conditionsProvider;
	}
	struct ConditionsStore::DerivedProvider GetRWPrefixProvider(const std::string &prefix)
	{
		struct ConditionsStore::DerivedProvider conditionsProvider;
		conditionsProvider.hasFun = [this, prefix] (const std::string &name){ verifyAndStripPrefix(prefix, name); return isInMap(values, name); };
		conditionsProvider.setFun = [this, prefix] (const std::string &name, int64_t value){ verifyAndStripPrefix(prefix, name); values[name] = value; return true; };
		conditionsProvider.eraseFun = [this, prefix] (const std::string &name){ verifyAndStripPrefix(prefix, name); values.erase(name); return true; };
		conditionsProvider.getFun = [this, prefix] (const std::string &name){ verifyAndStripPrefix(prefix, name); return getFromMapOrZero(values, name); };
		return conditionsProvider;
	}
	struct ConditionsStore::DerivedProvider GetRONamedProvider(const std::string &named)
	{
		struct ConditionsStore::DerivedProvider conditionsProvider;
		conditionsProvider.hasFun = [this, named] (const std::string &name){ verifyName(named, name); return isInMap(values, name); };
		conditionsProvider.setFun = [this, named] (const std::string &name, int64_t value){ verifyName(named, name); return false; };
		conditionsProvider.eraseFun = [this, named] (const std::string &name){ verifyName(named, name); return false; };
		conditionsProvider.getFun = [this, named] (const std::string &name){ verifyName(named, name); return getFromMapOrZero(values, name); };
		return conditionsProvider;
	}
	struct ConditionsStore::DerivedProvider GetRWNamedProvider(const std::string &named)
	{
		struct ConditionsStore::DerivedProvider conditionsProvider;
		conditionsProvider.hasFun = [this, named] (const std::string &name){ verifyName(named, name); return isInMap(values, name); };
		conditionsProvider.setFun = [this, named] (const std::string &name, int64_t value){ verifyName(named, name); values[name] = value; return true; };
		conditionsProvider.eraseFun = [this, named] (const std::string &name){ verifyName(named, name); values.erase(name); return true; };
		conditionsProvider.getFun = [this, named] (const std::string &name){ verifyName(named, name); return getFromMapOrZero(values, name); };
		return conditionsProvider;
	}

public:
	std::map<std::string, int64_t> values;
};


// #endregion mock data



// #region unit tests
SCENARIO( "Creating a ConditionsStore" , "[ConditionsStore][Creation]" ) {
	GIVEN( "no arguments" ) {
		const auto store = ConditionsStore();
		THEN( "no conditions are stored" ) {
			REQUIRE( store.GetPrimaryConditions().empty() );
		}
		THEN( "the begin and end iterator should be equal" ){
			REQUIRE( store.PrimariesBegin() == store.PrimariesBegin() );
			REQUIRE( store.PrimariesEnd() == store.PrimariesEnd() );
			REQUIRE( store.PrimariesBegin() == store.PrimariesEnd() );
			REQUIRE( store.PrimariesEnd() == store.PrimariesBegin() );
			auto it = store.PrimariesBegin();
			REQUIRE( it == store.PrimariesEnd() );
			REQUIRE( store.PrimariesEnd() == it );
		}
	}
	GIVEN( "an initializer list" ) {
		const auto store = ConditionsStore{{"hello world", 100}, {"goodbye world", 404}};
		
		THEN( "given primary conditions are in the Store" ) {
			REQUIRE( store.GetCondition("hello world") == 100 );
			REQUIRE( store.GetCondition("goodbye world") == 404 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
		}
		THEN( "not given conditions return the default value" ) {
			REQUIRE( 0 == store.GetCondition("ungreeted world") );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
			REQUIRE( store.GetCondition("ungreeted world") == 0 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );

			REQUIRE( 0 == store.GetCondition("hi world") );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
			REQUIRE( store.GetCondition("hi world") == 0 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
		}
		THEN( "two iterators to the same position should be equal"){
			REQUIRE( store.PrimariesBegin() == store.PrimariesBegin() );
			REQUIRE( store.PrimariesEnd() == store.PrimariesEnd() );
		}
		THEN( "iterating over the conditions should return both initial values") {
			auto it = store.PrimariesBegin();
			REQUIRE( it != store.PrimariesEnd());
			REQUIRE( it.first() == "goodbye world");
			REQUIRE( it.second() == 404 );
			++it;
			REQUIRE( it.first() == "hello world");
			REQUIRE( it.second() == 100 );
			it++;
			REQUIRE( it == store.PrimariesEnd());
		}
		THEN( "doing lower_bound finds should return values above the bound") {
			auto it = store.PrimariesLowerBound("ha");
			REQUIRE( it != store.PrimariesEnd());
			REQUIRE( it.first() == "hello world");
			REQUIRE( it.second() == 100 );
			++it;
			REQUIRE( it == store.PrimariesEnd());
		}
	}
	GIVEN( "an initializer map" ) {
		const std::map<std::string, int64_t> initmap {{"hello world", 100}, {"goodbye world", 404}};
		const auto store = ConditionsStore(initmap);
		
		THEN( "given primary conditions are in the Store" ) {
			REQUIRE( store.GetCondition("hello world") == 100 );
			REQUIRE( store.GetCondition("goodbye world") == 404 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
		}
		THEN( "not given conditions return the default value" ) {
			REQUIRE( store.GetCondition("ungreeted world") == 0 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
			REQUIRE( store.GetCondition("ungreeted world") == 0 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );

			REQUIRE( 0 == store.GetCondition("hi world") );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
			REQUIRE( store.GetCondition("hi world") == 0 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
		}
	}
	GIVEN( "a long initializer list" ) {
		const auto store = ConditionsStore{{"a", 1}, {"b", 2}, {"d", 4}, {"c", 3}, {"g", 7}, {"f", 6}, {"e", 5}};
		auto it = store.PrimariesBegin();
		THEN( "iterating should pass all conditions and reach the end")
		{
			REQUIRE( (it.first() == "a" && it.second() == 1) );
			++it;
			REQUIRE( (it.first() == "b" && it.second() == 2) );
			it++;
			REQUIRE( (it.first() == "c" && it.second() == 3) );
			REQUIRE( (++it).first() == "d" );
			REQUIRE( (it.first() == "d" && it.second() == 4) );
			REQUIRE( (it++).first() == "d" );
			REQUIRE( (it.first() == "e" && it.second() == 5) );
			++it;
			REQUIRE( (it.first() == "f" && it.second() == 6) );
			it++;
			REQUIRE( (it.first() == "g" && it.second() == 7) );
			it++;
			REQUIRE( it == store.PrimariesEnd());
		}
	}
}

SCENARIO( "Setting and erasing conditions", "[ConditionStore][ConditionSetting]" ) {
	GIVEN( "an empty starting store" ) {
		auto store = ConditionsStore();
		THEN( "stored values can be retrieved" ) {
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
			REQUIRE( store.SetCondition("myFirstVar", 10) == true );
			REQUIRE( store.GetCondition("myFirstVar") == 10 );
			REQUIRE( store.HasCondition("myFirstVar") == true );
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( store.GetCondition("myFirstVar") == 10 );
		}
		THEN( "defaults are returned for unstored values and are not stored" ) {
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
			REQUIRE( store.GetCondition("mySecondVar") == 0 );
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
			REQUIRE( store.HasCondition("mySecondVar") == false );
			REQUIRE( store.GetCondition("mySecondVar") == 0 );
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
		}
		THEN( "erased conditions are indeed removed" ) {
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
			REQUIRE( store.SetCondition("myFirstVar", 10) == true );
			REQUIRE( store.HasCondition("myFirstVar") == true );
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( store.GetCondition("myFirstVar") == 10 );
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( store.EraseCondition("myFirstVar") == true );
			REQUIRE( store.HasCondition("myFirstVar") == false );
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
			REQUIRE( store.GetCondition("myFirstVar") == 0 );
			REQUIRE( store.HasCondition("myFirstVar") == false );
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
		}
	}
}

SCENARIO( "Adding and removing on condition values", "[ConditionStore][ConditionArithmetic]" ) {
	GIVEN( "an starting store with 1 condition" ) {
		auto store = ConditionsStore{{"myFirstVar", 10}};
		THEN( "adding to an existing primary condition gives the new value" ) {
			REQUIRE( store.GetCondition("myFirstVar") == 10 );
			REQUIRE( store.AddCondition("myFirstVar", 10) == true );
			REQUIRE( store.GetCondition("myFirstVar") == 20 );
			REQUIRE( store.AddCondition("myFirstVar", -15) == true );
			REQUIRE( store.GetCondition("myFirstVar") == 5 );
			REQUIRE( store.AddCondition("myFirstVar", -15) == true );
			REQUIRE( store.GetCondition("myFirstVar") == -10 );
		}
		THEN( "adding to an non-existing primary condition sets the new value" ) {
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( store.AddCondition("mySecondVar", -30) == true );
			REQUIRE( store.GetCondition("mySecondVar") == -30 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
			REQUIRE( store.HasCondition("mySecondVar") == true );
			REQUIRE( store.AddCondition("mySecondVar", 60) == true );
			REQUIRE( store.GetCondition("mySecondVar") == 30 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
		}
	}
}

SCENARIO( "Providing derived conditions", "[ConditionStore][DerivedConditions]" ) {
	GIVEN( "an starting store with 1 condition" ) {
		auto mockProvPrefixA = MockConditionsProvider();
		auto mockProvNamed = MockConditionsProvider();
		auto store = ConditionsStore{{"myFirstVar", 10}};
		store.SetProviderNamed("named1", mockProvNamed.GetRWNamedProvider("named1"));
		store.SetProviderPrefixed("prefixA: ", mockProvPrefixA.GetRWPrefixProvider("prefixA: "));

		THEN( "adding to an existing primary condition gives the new value" ) {
			REQUIRE( store.GetCondition("myFirstVar") == 10 );
			REQUIRE( store.AddCondition("myFirstVar", 10) == true );
			REQUIRE( store.GetCondition("myFirstVar") == 20 );
			REQUIRE( store.AddCondition("myFirstVar", -15) == true );
			REQUIRE( store.GetCondition("myFirstVar") == 5 );
			REQUIRE( store.AddCondition("myFirstVar", -15) == true );
			REQUIRE( store.GetCondition("myFirstVar") == -10 );
		}
		THEN( "adding to an non-existing primary condition sets the new value" ) {
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( store.AddCondition("mySecondVar", -30) == true );
			REQUIRE( store.GetCondition("mySecondVar") == -30 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
			REQUIRE( store.HasCondition("mySecondVar") == true );
			REQUIRE( store.AddCondition("mySecondVar", 60) == true );
			REQUIRE( store.GetCondition("mySecondVar") == 30 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
		}
		THEN( "derived named conditions should be set properly" ) {
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( store.AddCondition("named1", -30) == true );
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( mockProvNamed.values["named1"] == -30 );
			REQUIRE( mockProvNamed.values.size() == 1 );
			REQUIRE( mockProvPrefixA.values.size() == 0 );
			REQUIRE( store.GetCondition("named1") == -30 );
			REQUIRE( store.GetCondition("myFirstVar") == 10 );
			REQUIRE( store.GetCondition("mySecondVar") == 0 );

			store.SetProviderNamed("named1", mockProvNamed.GetRONamedProvider("named1"));

			REQUIRE( store.AddCondition("named1", -20) == false );
			REQUIRE( mockProvNamed.values["named1"] == -30 );
			REQUIRE( mockProvNamed.values.size() == 1 );
			REQUIRE( mockProvPrefixA.values.size() == 0 );
			REQUIRE( store.GetCondition("named1") == -30 );
			REQUIRE( store.GetCondition("myFirstVar") == 10 );
			REQUIRE( store.GetCondition("mySecondVar") == 0 );

			REQUIRE( store.EraseCondition("named1") == false );
			REQUIRE( mockProvNamed.values["named1"] == -30 );
			REQUIRE( mockProvNamed.values.size() == 1 );
			REQUIRE( mockProvPrefixA.values.size() == 0 );
			REQUIRE( store.GetCondition("named1") == -30 );
			REQUIRE( store.GetCondition("myFirstVar") == 10 );

			REQUIRE( store.HasCondition("named1") == true );
			REQUIRE( store.HasCondition("named") == false );
		}
		THEN( "derived prefixed conditions should be set properly" ) {
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( store.AddCondition("prefixA: test", -30) == true );
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
			REQUIRE( mockProvPrefixA.values.size() == 1 );
			REQUIRE( mockProvNamed.values.size() == 0 );
			REQUIRE( store.GetCondition("prefixA: test") == -30 );
			REQUIRE( store.GetCondition("myFirstVar") == 10 );
			REQUIRE( store.GetCondition("mySecondVar") == 0 );

			store.SetProviderPrefixed("prefixA: ", mockProvPrefixA.GetROPrefixProvider("prefixA: "));
			REQUIRE( store.AddCondition("prefixA: test", -20) == false );
			REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
			REQUIRE( mockProvPrefixA.values.size() == 1 );
			REQUIRE( mockProvNamed.values.size() == 0 );
			REQUIRE( store.GetCondition("prefixA: test") == -30 );
			REQUIRE( store.GetCondition("myFirstVar") == 10 );

			REQUIRE( store.EraseCondition("prefixA: test") == false );
			REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
			REQUIRE( mockProvPrefixA.values.size() == 1 );
			REQUIRE( mockProvNamed.values.size() == 0 );
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
		}
		AND_GIVEN( "more derived conditions are added" )
		{
			auto mockProvPrefix = MockConditionsProvider();
			store.SetProviderPrefixed("prefix: ", mockProvPrefix.GetRWPrefixProvider("prefix: "));
			auto mockProvPrefixB = MockConditionsProvider();
			store.SetProviderPrefixed("prefixB: ", mockProvPrefixB.GetRWPrefixProvider("prefixB: "));
			THEN( "derived prefixed conditions should be set properly" ) {
				REQUIRE( store.GetPrimaryConditions().size() == 1 );
				REQUIRE( store.AddCondition("prefixA: test", -30) == true );
				REQUIRE( store.GetPrimaryConditions().size() == 1 );
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
				REQUIRE( store.GetPrimaryConditions().size() == 1 );
			}
		}
	}
}



// #endregion unit tests



} // test namespace
