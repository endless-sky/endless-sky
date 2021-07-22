#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/ConditionsStore.h"

// ... and any system includes needed for the test file.
#include <map>
#include <string>



namespace { // test namespace

// #region mock data

class MockConditionsProvider : public ConditionsProvider
{
protected:
	virtual int64_t GetConditionImpl(const std::string &name) const override;
	virtual bool HasConditionImpl(const std::string &name) const override;
	virtual bool SetConditionImpl(const std::string &name, int64_t value) override;
	virtual bool EraseConditionImpl(const std::string &name) override;


public:
	bool readOnly = false;
	std::map<std::string, int64_t> values;
};

int64_t MockConditionsProvider::GetConditionImpl(const std::string &name) const
{
	auto it = values.find(name);
	if(it != values.end())
		return it->second;
	return 0;
}

bool MockConditionsProvider::HasConditionImpl(const std::string &name) const
{
	auto it = values.find(name);
	return it != values.end();
}

bool MockConditionsProvider::SetConditionImpl(const std::string &name, int64_t value)
{
	if(readOnly)
		return false;
	values[name] = value;
	return true;
}

bool MockConditionsProvider::EraseConditionImpl(const std::string &name)
{
	if(readOnly)
		return false;
	values.erase(name);
	return true;
}
// #endregion mock data



// #region unit tests
SCENARIO( "Creating a ConditionsStore" , "[ConditionsStore][Creation]" ) {
	GIVEN( "no arguments" ) {
		const auto store = ConditionsStore();
		THEN( "no conditions are stored" ) {
			REQUIRE( store.GetPrimaryConditions().empty() );
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
		store.SetProviderNamed("named1", &mockProvNamed);
		store.SetProviderPrefixed("prefixA: ", &mockProvPrefixA);

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

			mockProvNamed.readOnly = true;
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

			mockProvPrefixA.readOnly = true;
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
			store.SetProviderPrefixed("prefix: ", &mockProvPrefix);
			auto mockProvPrefixB = MockConditionsProvider();
			store.SetProviderPrefixed("prefixB: ", &mockProvPrefixB);
			THEN( "derived prefixed conditions should be set properly" ) {
				REQUIRE( store.GetPrimaryConditions().size() == 1 );
				REQUIRE( store.AddCondition("prefixA: test", -30) == true );
				REQUIRE( store.GetPrimaryConditions().size() == 1 );
				REQUIRE( mockProvPrefixA.values["prefixA: test"] == -30 );
				REQUIRE( store.GetCondition("prefixA: test") == -30 );
				REQUIRE( store.GetCondition("myFirstVar") == 10 );

				mockProvPrefixA.readOnly = true;
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

				mockProvPrefixA.readOnly = false;
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
