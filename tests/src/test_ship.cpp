/* test_ship.cpp
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/Ship.h"

#include "datanode-factory.h"
#include "../../source/GameData.h"
#include "../../source/GameObjects.h"
#include "../../source/Government.h"

// ... and any system includes needed for the test file.
#include <memory>
#include <string>
#include <type_traits>

namespace { // test namespace

// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.

// #endregion mock data



// #region unit tests
SCENARIO( "Creating a Ship::Bay instance", "[ship][bay]" ) {
	// No default constructor.
	REQUIRE_FALSE( std::is_default_constructible<Ship::Bay>::value );
	
	GIVEN( "a reference position" ) {
		auto bay = Ship::Bay(20., 40., "Fighter");
		THEN( "the position is scaled by 50%" ) {
			CHECK( bay.point.X() == Approx(10.) );
			CHECK( bay.point.Y() == Approx(20.) );
		}
	}
	GIVEN( "a category string" ) {
		std::string value = "any string value";
		auto bay = Ship::Bay(0., 0., value);
		THEN( "the category is stored" ) {
			CHECK( bay.category == value );
		}
	}
}

SCENARIO( "A Ship::Bay instance is being copied", "[ship][bay]") {
	auto source = Ship::Bay(-10., 10., "Fighter");
	GIVEN( "the bay is occupied" ) {
		auto occupant = std::make_shared<Ship>();
		REQUIRE( occupant );
		source.ship = occupant;
		REQUIRE( source.ship );
		WHEN( "the copy is made via ctor" ) {
			Ship::Bay copy(source);
			THEN( "the copy has the correct attributes" ) {
				CHECK( copy.point.X() == source.point.X() );
				CHECK( copy.point.Y() == source.point.Y() );
				CHECK( copy.category == source.category );
				CHECK( copy.side == source.side );
				CHECK( copy.facing.Degrees() == source.facing.Degrees() );
				CHECK( copy.launchEffects == source.launchEffects );
			}
			THEN( "the copy is unoccupied" ) {
				CHECK_FALSE( copy.ship );
			}
			THEN( "the source is still occupied" ) {
				CHECK( source.ship == occupant );
			}
		}
		WHEN( "the copy is made via assignment" ) {
			Ship::Bay copy = source;
			THEN( "the copy has the correct attributes" ) {
				CHECK( copy.point.X() == source.point.X() );
				CHECK( copy.point.Y() == source.point.Y() );
				CHECK( copy.category == source.category );
				CHECK( copy.side == source.side );
				CHECK( copy.facing.Degrees() == source.facing.Degrees() );
				CHECK( copy.launchEffects == source.launchEffects );
			}
			THEN( "the copy is unoccupied" ) {
				CHECK_FALSE( copy.ship );
			}
			THEN( "the source is still occupied" ) {
				CHECK( source.ship == occupant );
			}
		}
	}
}

SCENARIO( "Creating a Ship" , "[ship][Creation]" ) {
	Ship ship;
	GameObjects objects;
	GameData::SetObjects(objects);
	GIVEN( "When created" ) {
		THEN( "it has the correct default properties" ){
			CHECK_FALSE( ship.IsValid() );
			CHECK( ship.Name().empty() );
			CHECK( ship.ModelName().empty() );
			CHECK( ship.PluralModelName().empty() );
			CHECK( ship.VariantName().empty() );
			CHECK( ship.Noun() == "ship" );
			CHECK( ship.Description().empty() );
			CHECK( ship.Attributes().Attributes().empty() );
			CHECK( ship.BaseAttributes().Attributes().empty() );
			CHECK_FALSE( ship.Position() );
			CHECK_FALSE( ship.Thumbnail() );
			CHECK_FALSE( ship.GetGovernment() );
			CHECK_FALSE( ship.Cost() );
			CHECK_FALSE( ship.Mass() );
			CHECK( ship.Outfits().empty() );
			CHECK( ship.Weapons().empty() );
			CHECK( ship.EnginePoints().empty() );
		}
	}
	GIVEN( "When loading a ship from a DataNode" ) {
		objects.Load(AsDataFile(R"(
outfit "Jump Drive"
outfit "Cool Engines"
)"));
		ship.Load(AsDataNode(R"(
ship TestShip
	plural "TestShip Plural"
	noun test
	thumbnail some/sprite
	attributes
		cost 80000
		mass 12345
		shields 100000
		hull 45000
		drag 0.3
		"outfit capacity" 45
	outfits
		"Jump Drive"
		"Cool Engines"
	turret 0 45
	engine -10 10
	description "A test ship"
	description "cool"
)"));
		ship.FinishLoading(true);

		THEN( "it has the correct properties" ) {
			CHECK( ship.IsValid() );
			CHECK( ship.Name().empty() );
			CHECK( ship.ModelName() == "TestShip" );
			CHECK( ship.PluralModelName() == "TestShip Plural" );
			CHECK( ship.VariantName() == "TestShip" );
			CHECK( ship.Noun() == "test" );
			CHECK( ship.Description() == "A test ship\ncool\n" );
			CHECK_FALSE( ship.Position() );
			CHECK( ship.Thumbnail()->Name() == "some/sprite" );
			CHECK_FALSE( ship.GetGovernment() );
			CHECK( ship.Cost() == 80000 );
			CHECK( ship.Mass() == 12345 );
			CHECK( ship.Attributes().Get("drag") == Approx(0.3) );
			CHECK( ship.Attributes().Get("shields") == Approx(100000.) );
			CHECK( ship.Attributes().Get("hull") == Approx(45000.) );
			CHECK( ship.Attributes().Get("outfit capacity") == Approx(45.) );
			CHECK( ship.BaseAttributes().Get("shields") == Approx(100000.) );
			CHECK( ship.BaseAttributes().Get("hull") == Approx(45000.) );
			CHECK( ship.BaseAttributes().Get("outfit capacity") == Approx(45.) );
			REQUIRE( ship.Outfits().size() == 2 );
			CHECK( ship.Outfits().count(objects.outfits.Get("Jump Drive")) );
			CHECK( ship.Outfits().count(objects.outfits.Get("Cool Engines")) );
			CHECK( ship.Weapons().size() == 1 );
			CHECK( ship.Weapons()[0].GetPoint().X() == Approx(0.) );
			CHECK( ship.Weapons()[0].GetPoint().Y() == Approx(45. / 2.) );
			REQUIRE( ship.EnginePoints().size() == 1 );
			CHECK( ship.EnginePoints()[0].X() == Approx(-10. / 2.) );
			CHECK( ship.EnginePoints()[0].Y() == Approx(10. / 2.) );
		}
	}
}



// Test code goes here. Preferably, use scenario-driven language making use of the SCENARIO, GIVEN,
// WHEN, and THEN macros. (There will be cases where the more traditional TEST_CASE and SECTION macros
// are better suited to declaration of the public API.)

// When writing assertions, prefer the CHECK and CHECK_FALSE macros when probing the scenario, and prefer
// the REQUIRE / REQUIRE_FALSE macros for fundamental / "validity" assertions. If a CHECK fails, the rest
// of the block's statements will still be evaluated, but a REQUIRE failure will exit the current block.

// #endregion unit tests



} // test namespace
