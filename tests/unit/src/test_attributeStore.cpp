/* test_attributeStore.cpp
Copyright (c) 2023 by tibetiroka

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
#include "../../../source/AttributeStore.h"

// Include a helper for creating data nodes
#include "datanode-factory.h"

// ... and any system includes needed for the test file.
#include <limits>

namespace { // test namespace
// #region mock data
// #endregion mock data



// #region unit tests
TEST_CASE( "AttributeStore::GetMinimum", "[AttributeStore][GetMinimum]" ) {
	AttributeStore a;
	SECTION( "Unspecified text attributes" ) {
		CHECK( a.GetMinimum("random string") == 0. );
		CHECK( a.GetMinimum("solar heat") == 0. );
		CHECK( a.GetMinimum("unplunderable") == 0. );
	}
	SECTION( "Specified text attributes" ) {
		CHECK( a.GetMinimum("hull threshold") == std::numeric_limits<double>::lowest() );
		CHECK( a.GetMinimum("crew equivalent") == std::numeric_limits<double>::lowest() );
		CHECK( a.GetMinimum("fuel consumption") == std::numeric_limits<double>::lowest() );
	}
	SECTION( "Multipliers" ) {
		CHECK( a.GetMinimum(AttributeAccess(SHIELD_GENERATION, SHIELDS).Multiplier()) == -1. );
		CHECK( a.GetMinimum(AttributeAccess(THRUSTING, ENERGY).Relative().Multiplier()) == -1. );
	}
	SECTION( "Protection" ) {
		CHECK( a.GetMinimum(AttributeAccess(PROTECTION, SCRAMBLE)) == -0.99 );
		CHECK( a.GetMinimum(AttributeAccess(PROTECTION, SCRAMBLE, ENERGY)) == std::numeric_limits<double>::lowest() );
	}
	SECTION( "Others" ) {
		CHECK( a.GetMinimum(AttributeAccess(THRUSTING, SCRAMBLE)) == std::numeric_limits<double>::lowest() );
	}
}

TEST_CASE( "AttributeStore::Set", "[AttributeStore][Set]" ) {
	AttributeStore a;
	a.Set("solar heat", 0.);
	SECTION( "Empty when only contains 0" ) {
		CHECK( a.empty() );
	}
	a.Set(AttributeAccess(PROTECTION, SCRAMBLE), -2.);
	SECTION( "Respecting minimum values" ) {
		CHECK( a.Get(AttributeAccess(PROTECTION, SCRAMBLE)) == -0.99 );
	}
	SECTION( "Doesn't update legacy values" ) {
		CHECK( a.Get("scramble protection") == 0. );
	}
	SECTION( "Not empty when contains data" ) {
		CHECK( !a.empty() );
	}
}

TEST_CASE( "AttributeStore::Load", "[AttributeStore][Load]" ) {
	AttributeStore store;
	DataNode node = AsDataNode("parent\n"
							"	attribute 1\n"
							"	thrust 100\n"
							"		energy 20\n"
							"		heat 10\n"
							"	turn 500\n"
							"		shields 100\n"
							"	\"scramble resistance\" 100\n"
							"		energy 20\n"
							"	\"other attribute\" 1\n"
							"	\"another attribute\" 0\n"
							"	\"shield generation\" 30\n"
							"		\"energy\" 50\n"
							"	\"slowing resistance\" 30\n"
							"		heat 40\n"
							"		energy 20");
	for(const DataNode &child : node)
		store.Load(child);
	SECTION( "Check loaded attributes" ) {
		CHECK( !store.empty() );
		CHECK( !store.IsPresent("some attribute") );
		CHECK( store.IsPresent("attribute") );
		CHECK( store.Get("attribute") == 1. );
		CHECK( store.Get("thrust") == 0. );
		CHECK( store.IsPresent({THRUSTING, THRUST}) );
		CHECK( store.Get({THRUSTING, THRUST}) == 100. );
		CHECK( store.Get("thrusting energy") == 0. );
		CHECK( store.Get({THRUSTING, ENERGY}) == 20. );
		CHECK( store.Get("thrusting heat") == 0. );
		CHECK( store.Get({THRUSTING, HEAT}) == 10. );
		CHECK( store.IsPresent("other attribute") );
		CHECK( store.Get("other attribute") == 1. );
		CHECK( !store.IsPresent("another attribute") );
		CHECK( store.Get("another attribute") == 0. );
		CHECK( store.Get("shield generation") == 0. );
		CHECK( store.Get({SHIELD_GENERATION, SHIELDS}) == 30. );
		CHECK( store.Get("shield energy") == 0. );
		CHECK( store.Get({SHIELD_GENERATION, ENERGY}) == 50. );
		CHECK( store.Get("turn") == 0. );
		CHECK( store.Get({TURNING, TURN}) == 500. );
		CHECK( store.Get("turning shields") == 0. );
		CHECK( store.Get({TURNING, SHIELDS}) == 100. );
		CHECK( store.Get("scramble resistance") == 0. );
		CHECK( store.Get({RESISTANCE, SCRAMBLE}) == 100. );
		CHECK( store.Get("scramble resistance energy") == 0. );
		CHECK( store.Get({RESISTANCE, SCRAMBLE, ENERGY}) == 20. );
		CHECK( store.Get("slowing resistance") == 0. );
		CHECK( store.Get({RESISTANCE, SLOWING}) == 30. );
		CHECK( store.Get("slowing resistance heat") == 0. );
		CHECK( store.Get({RESISTANCE, SLOWING, HEAT}) == 40. );
	}
}

	TEST_CASE( "AttributeStore::Save", "[AttributeStore][Save]" ) {
		AttributeStore store;
		DataNode node = AsDataNode("parent\n"
								"	attribute 1\n"
								"	thrust 100\n"
								"		energy 20\n"
								"		heat 10\n"
								"	turn 500\n"
								"		shields 100\n"
								"	\"scramble resistance\" 100\n"
								"		energy 20\n"
								"	\"other attribute\" 1\n"
								"	\"another attribute\" 0\n"
								"	\"shield generation\" 30\n"
								"	\"shield energy\" 50\n"
								"	\"slowing resistance\" 30\n"
								"		heat 40\n"
								"		energy 20");
		for(const DataNode &child : node)
			store.Load(child);
		DataWriter writer;
		store.Save(writer);
		SECTION( "Check saved data" ) {
			std::string data = writer.SaveToString();
			std::string expected =
R"(attribute 1
"other attribute" 1
"shield generation" 30
	energy 50
thrust 100
	energy 20
	heat 10
turn 500
	shields 100
"scramble resistance" 100
	energy 20
"slowing resistance" 30
	energy 20
	heat 40
)";
			CHECK( data == expected );
		}
	}
// #endregion unit tests



} // test namespace
