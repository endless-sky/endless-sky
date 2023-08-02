/* test_attribute.cpp
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
		CHECK( a.GetMinimum(Attribute(SHIELD_GENERATION, SHIELDS).Multiplier()) == -1. );
		CHECK( a.GetMinimum(Attribute(THRUSTING, ENERGY).Relative().Multiplier()) == -1. );
	}
	SECTION( "Protection" ) {
		CHECK( a.GetMinimum(Attribute(PROTECTION, SCRAMBLE)) == -0.99 );
		CHECK( a.GetMinimum(Attribute(PROTECTION, SCRAMBLE, ENERGY)) == std::numeric_limits<double>::lowest() );
	}
	SECTION( "Others" ) {
		CHECK( a.GetMinimum(Attribute(THRUSTING, SCRAMBLE)) == std::numeric_limits<double>::lowest() );
	}
}

TEST_CASE( "AttributeStore::Set", "[AttributeStore][Set]" ) {
	AttributeStore a;
	a.Set("solar heat", 0.);
	SECTION( "Empty when only contains 0" ) {
		CHECK( a.empty() );
	}
	a.Set(Attribute(PROTECTION, SCRAMBLE), -2.);
	SECTION( "Respecting minimum values" ) {
		CHECK( a.Get(Attribute(PROTECTION, SCRAMBLE)) == -0.99 );
	}
	SECTION( "Updates legacy values" ) {
		CHECK( a.Get("scramble protection") == a.Get(Attribute(PROTECTION, SCRAMBLE)) );
	}
	SECTION( "Not empty when contains data" ) {
		CHECK( !a.empty() );
	}
}

TEST_CASE( "AttributeStore::Load", "[AttributeStore][Load]" ) {
	AttributeStore store;
	DataNode node = AsDataNode("parent\n\tattribute 1\n\tthrust 100\n\t\tenergy 20\n"
			"\t\theat 10\n\t\"other attribute\" 1\n\t\"shield generation\"\n\t\tshields 30\n\t\tenergy 50\n"
			"\tturn 500\n\t\tshields 100\n\tresistance\n\t\tscramble 100\n\t\t\tenergy 20\n"
			"\t\"slowing resistance\" 30\n\t\theat 40");
	for(const DataNode &child : node)
		store.Load(child);
	SECTION( "Check loaded attributes" ) {
		CHECK( !store.empty() );
		CHECK( !store.IsPresent("some attribute") );
		CHECK( store.IsPresent("attribute") );
		CHECK( store.Get("attribute") == 1. );
		CHECK( store.Get("thrust") == 100. );
		CHECK( store.Get(Attribute(THRUSTING, THRUST)) == 100. );
		CHECK( store.Get("thrusting energy") == 20. );
		CHECK( store.Get(Attribute(THRUSTING, ENERGY)) == 20. );
		CHECK( store.Get("thrusting heat") == 10. );
		CHECK( store.Get(Attribute(THRUSTING, HEAT)) == 10. );
		CHECK( store.IsPresent("other attribute") );
		CHECK( store.Get("other attribute") == 1. );
		CHECK( store.Get("shield generation") == 30. );
		CHECK( store.Get(Attribute(SHIELD_GENERATION, SHIELDS)) == 30. );
		CHECK( store.Get("shield energy") == 50. );
		CHECK( store.Get(Attribute(SHIELD_GENERATION, ENERGY)) == 50. );
		CHECK( store.Get("turn") == 500. );
		CHECK( store.Get(Attribute(TURNING, TURN)) == 500. );
		CHECK( store.Get("turning shields") == 100. );
		CHECK( store.Get(Attribute(TURNING, SHIELDS)) == 100. );
		CHECK( store.Get("scramble resistance") == 100. );
		CHECK( store.Get(Attribute(RESISTANCE, SCRAMBLE)) == 100. );
		CHECK( store.Get("scramble resistance energy") == 20. );
		CHECK( store.Get(Attribute(RESISTANCE, SCRAMBLE, ENERGY)) == 20. );
		CHECK( store.Get("slowing resistance") == 30. );
		CHECK( store.Get(Attribute(RESISTANCE, SLOWING)) == 30. );
		CHECK( store.Get("slowing resistance heat") == 40. );
		CHECK( store.Get(Attribute(RESISTANCE, SLOWING, HEAT)) == 40. );
	}
}
// #endregion unit tests



} // test namespace
