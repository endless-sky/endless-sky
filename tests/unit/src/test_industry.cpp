/* test_industry.cpp
Copyright (c) 2026 by Endless Sky Expanded contributors

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
#include "../../../source/Industry.h"

// Include a helper for creating well-formed DataNodes.
#include "datanode-factory.h"

// ... and any system includes needed for the test file.
#include "../../../source/DataWriter.h"
#include "../../../source/Facility.h"
#include "../../../source/Set.h"

#include <string>



namespace { // test namespace

// #region mock data
const std::string OUTPOST = R"(facility "Mining Outpost"
	description "A mine."
	cost 150000
	output "Metal" 2
	storage 5
	planet "New Greenland" "Earth"
)";

Set<Facility> MakeFacilities()
{
	Set<Facility> facilities;
	facilities.Get("Mining Outpost")->Load(AsDataNode(OUTPOST));
	return facilities;
}
// #endregion mock data



// #region unit tests
SCENARIO( "Loading a facility type", "[Facility]" ) {
	GIVEN( "a facility definition" ) {
		Facility facility;
		REQUIRE_FALSE( facility.IsDefined() );
		facility.Load(AsDataNode(OUTPOST));
		THEN( "all of its attributes are read" ) {
			CHECK( facility.IsDefined() );
			CHECK( facility.TrueName() == "Mining Outpost" );
			CHECK( facility.Description() == "A mine." );
			CHECK( facility.Cost() == 150000 );
			CHECK( facility.Output() == "Metal" );
			CHECK( facility.OutputPerDay() == 2 );
			CHECK( facility.Storage() == 5 );
		}
		THEN( "it can only be built on the listed planets" ) {
			CHECK( facility.CanBuildOn("New Greenland") );
			CHECK( facility.CanBuildOn("Earth") );
			CHECK_FALSE( facility.CanBuildOn("Mars") );
		}
	}
}

SCENARIO( "Running a facility", "[Industry]" ) {
	const Set<Facility> facilities = MakeFacilities();
	const Facility &outpost = *facilities.Find("Mining Outpost");
	GIVEN( "a newly built facility" ) {
		Industry industry;
		industry.Build(outpost, "New Greenland");
		REQUIRE( industry.Holdings().size() == 1 );
		REQUIRE( industry.Holdings().front().stockpile == 0 );

		WHEN( "the same facility is built again on the same planet" ) {
			industry.Build(outpost, "New Greenland");
			THEN( "nothing changes" ) {
				CHECK( industry.Holdings().size() == 1 );
			}
		}
		WHEN( "a day passes" ) {
			industry.AdvanceDay();
			THEN( "it produces its daily output" ) {
				CHECK( industry.Find(outpost, "New Greenland")->stockpile == 2 );
			}
		}
		WHEN( "many days pass" ) {
			for(int i = 0; i < 10; ++i)
				industry.AdvanceDay();
			THEN( "the stockpile stops at the storage limit" ) {
				CHECK( industry.Find(outpost, "New Greenland")->stockpile == 5 );
			}
		}
		WHEN( "goods are collected with limited cargo space" ) {
			industry.AdvanceDay();
			industry.AdvanceDay();
			Industry::Holding &holding = *industry.Find(outpost, "New Greenland");
			int taken = Industry::Collect(holding, 3);
			THEN( "only what fits is taken" ) {
				CHECK( taken == 3 );
				CHECK( holding.stockpile == 1 );
			}
			AND_WHEN( "the rest is collected with plenty of space" ) {
				taken = Industry::Collect(holding, 100);
				THEN( "the stockpile is emptied" ) {
					CHECK( taken == 1 );
					CHECK( holding.stockpile == 0 );
				}
			}
		}
	}
}

SCENARIO( "Saving and loading facilities", "[Industry]" ) {
	const Set<Facility> facilities = MakeFacilities();
	const Facility &outpost = *facilities.Find("Mining Outpost");
	GIVEN( "an industry with a partly filled facility" ) {
		Industry industry;
		industry.Build(outpost, "New Greenland");
		industry.AdvanceDay();
		DataWriter writer;
		industry.Save(writer);
		WHEN( "it is saved and loaded again" ) {
			Industry loaded;
			loaded.Load(AsDataNode(writer.SaveToString()), facilities);
			THEN( "the facility and its stockpile are restored" ) {
				REQUIRE( loaded.Holdings().size() == 1 );
				const Industry::Holding *holding = loaded.Find(outpost, "New Greenland");
				REQUIRE( holding );
				CHECK( holding->stockpile == 2 );
			}
		}
	}
	GIVEN( "an empty industry" ) {
		Industry industry;
		DataWriter writer;
		industry.Save(writer);
		THEN( "nothing is written to the save file" ) {
			CHECK( writer.SaveToString().empty() );
		}
	}
	GIVEN( "a saved facility whose type no longer exists" ) {
		Industry industry;
		industry.Load(AsDataNode("industry\n\tfacility \"Gone\"\n\t\tplanet Earth\n"), facilities);
		THEN( "it is skipped" ) {
			CHECK( industry.Holdings().empty() );
		}
	}
}
// #endregion unit tests



} // test namespace
