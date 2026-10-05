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
	upkeep 100
	output "Metal" 2
	storage 5
	planet "New Greenland" "Earth"
	attributes mining uninhabited
)";

const std::string WORKS = R"(facility "Machine Works"
	cost 300000
	upkeep 50
	input "Metal" 2
	input "Plastic" 1
	output "Equipment" 4
	storage 10
)";

Set<Facility> MakeFacilities()
{
	Set<Facility> facilities;
	facilities.Get("Mining Outpost")->Load(AsDataNode(OUTPOST));
	facilities.Get("Machine Works")->Load(AsDataNode(WORKS));
	return facilities;
}
// #endregion mock data



// #region unit tests
SCENARIO( "Loading a facility type", "[Facility]" ) {
	GIVEN( "a facility definition" ) {
		Facility facility;
		REQUIRE_FALSE( facility.IsDefined() );
		facility.Load(AsDataNode(WORKS));
		THEN( "all of its attributes are read" ) {
			CHECK( facility.IsDefined() );
			CHECK( facility.TrueName() == "Machine Works" );
			CHECK( facility.Cost() == 300000 );
			CHECK( facility.Upkeep() == 50 );
			CHECK( facility.Storage() == 10 );
			REQUIRE( facility.Inputs().size() == 2 );
			CHECK( facility.Inputs()[0] == std::make_pair(std::string("Metal"), 2) );
			CHECK( facility.Inputs()[1] == std::make_pair(std::string("Plastic"), 1) );
			REQUIRE( facility.Outputs().size() == 1 );
			CHECK( facility.Outputs()[0] == std::make_pair(std::string("Equipment"), 4) );
		}
	}
	GIVEN( "a facility limited to some planets and attributes" ) {
		Facility facility;
		facility.Load(AsDataNode(OUTPOST));
		THEN( "it can be built on the listed planets" ) {
			CHECK( facility.CanBuildOn("New Greenland", {}) );
			CHECK( facility.CanBuildOn("Earth", {"urban"}) );
		}
		THEN( "it can be built on planets with any listed attribute" ) {
			CHECK( facility.CanBuildOn("Mars", {"desert", "mining"}) );
			CHECK( facility.CanBuildOn("Rock", {"uninhabited"}) );
		}
		THEN( "it cannot be built anywhere else" ) {
			CHECK_FALSE( facility.CanBuildOn("Mars", {"desert"}) );
			CHECK_FALSE( facility.CanBuildOn("Mars", {}) );
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
		Industry::Holding &holding = *industry.Find(outpost, "New Greenland");
		REQUIRE( holding.count == 1 );
		REQUIRE( holding.Stock("Metal") == 0 );
		REQUIRE( holding.status == Industry::Status::STARTING );

		WHEN( "a day passes" ) {
			const Industry::DayReport report = industry.AdvanceDay(1000);
			THEN( "it pays upkeep and produces its daily output" ) {
				CHECK( report.upkeep == 100 );
				CHECK( report.sales == 0 );
				CHECK( holding.Stock("Metal") == 2 );
				CHECK( holding.status == Industry::Status::RUNNING );
			}
		}
		WHEN( "the player cannot afford the upkeep" ) {
			const Industry::DayReport report = industry.AdvanceDay(99);
			THEN( "nothing is paid and nothing is produced" ) {
				CHECK( report.upkeep == 0 );
				CHECK( holding.Stock("Metal") == 0 );
				CHECK( holding.status == Industry::Status::NO_CREDITS );
			}
		}
		WHEN( "many days pass" ) {
			for(int i = 0; i < 10; ++i)
				industry.AdvanceDay(1000);
			THEN( "the stock stops at the storage limit" ) {
				CHECK( holding.Stock("Metal") == 5 );
				CHECK( holding.status == Industry::Status::STORAGE_FULL );
			}
		}
		WHEN( "the facility is built again on the same planet" ) {
			industry.Build(outpost, "New Greenland");
			industry.AdvanceDay(1000);
			THEN( "it is expanded: production, upkeep and storage all double" ) {
				CHECK( industry.Holdings().size() == 1 );
				CHECK( holding.count == 2 );
				CHECK( holding.Stock("Metal") == 4 );
				CHECK( holding.Capacity() == 10 );
			}
		}
		WHEN( "goods are collected with limited cargo space" ) {
			industry.AdvanceDay(1000);
			industry.AdvanceDay(1000);
			int taken = Industry::Collect(holding, "Metal", 3);
			THEN( "only what fits is taken" ) {
				CHECK( taken == 3 );
				CHECK( holding.Stock("Metal") == 1 );
			}
			AND_WHEN( "the rest is collected with plenty of space" ) {
				taken = Industry::Collect(holding, "Metal", 100);
				THEN( "the stock is emptied" ) {
					CHECK( taken == 1 );
					CHECK( holding.Stock("Metal") == 0 );
				}
			}
		}
		WHEN( "auto-sell is on and the output can be sold" ) {
			holding.autoSell = true;
			int soldTons = 0;
			const Industry::DayReport report = industry.AdvanceDay(1000,
				[&soldTons](const Industry::Holding &, const std::string &commodity, int tons) -> int64_t {
					soldTons += tons;
					return commodity == "Metal" ? tons * 300 : 0;
				});
			THEN( "the day's output is sold" ) {
				CHECK( soldTons == 2 );
				CHECK( report.sales == 600 );
				CHECK( holding.Stock("Metal") == 0 );
			}
		}
		WHEN( "auto-sell is on but there is no market" ) {
			holding.autoSell = true;
			const Industry::DayReport report = industry.AdvanceDay(1000,
				[](const Industry::Holding &, const std::string &, int) -> int64_t { return 0; });
			THEN( "the output is kept" ) {
				CHECK( report.sales == 0 );
				CHECK( holding.Stock("Metal") == 2 );
			}
		}
	}
}

SCENARIO( "Running a production chain", "[Industry]" ) {
	const Set<Facility> facilities = MakeFacilities();
	const Facility &works = *facilities.Find("Machine Works");
	GIVEN( "a factory with no inputs" ) {
		Industry industry;
		industry.Build(works, "Earth");
		Industry::Holding &holding = *industry.Find(works, "Earth");
		WHEN( "a day passes" ) {
			industry.AdvanceDay(1000);
			THEN( "it sits idle" ) {
				CHECK( holding.Stock("Equipment") == 0 );
				CHECK( holding.status == Industry::Status::NO_INPUTS );
			}
		}
		WHEN( "it is supplied with more than it can store" ) {
			const int metal = Industry::Supply(holding, "Metal", 25);
			const int plastic = Industry::Supply(holding, "Plastic", 1);
			THEN( "only what fits is accepted" ) {
				CHECK( metal == 10 );
				CHECK( plastic == 1 );
			}
			AND_WHEN( "days pass" ) {
				industry.AdvanceDay(1000);
				industry.AdvanceDay(1000);
				THEN( "it runs only while every input is available" ) {
					CHECK( holding.Stock("Metal") == 8 );
					CHECK( holding.Stock("Plastic") == 0 );
					CHECK( holding.Stock("Equipment") == 4 );
					CHECK( holding.status == Industry::Status::NO_INPUTS );
				}
			}
		}
	}
}

SCENARIO( "Saving and loading facilities", "[Industry]" ) {
	const Set<Facility> facilities = MakeFacilities();
	const Facility &outpost = *facilities.Find("Mining Outpost");
	const Facility &works = *facilities.Find("Machine Works");
	GIVEN( "an industry with stocked and expanded facilities" ) {
		Industry industry;
		industry.Build(outpost, "New Greenland");
		industry.Build(works, "Earth");
		industry.Build(works, "Earth");
		industry.AdvanceDay(1000);
		Industry::Supply(*industry.Find(works, "Earth"), "Plastic", 3);
		industry.Find(outpost, "New Greenland")->autoSell = true;
		DataWriter writer;
		industry.Save(writer);
		WHEN( "it is saved and loaded again" ) {
			Industry loaded;
			loaded.Load(AsDataNode(writer.SaveToString()), facilities);
			THEN( "everything is restored" ) {
				REQUIRE( loaded.Holdings().size() == 2 );
				const Industry::Holding *mine = loaded.Find(outpost, "New Greenland");
				REQUIRE( mine );
				CHECK( mine->Stock("Metal") == 2 );
				CHECK( mine->autoSell );
				CHECK( mine->count == 1 );
				const Industry::Holding *factory = loaded.Find(works, "Earth");
				REQUIRE( factory );
				CHECK( factory->count == 2 );
				CHECK( factory->Stock("Plastic") == 3 );
				CHECK_FALSE( factory->autoSell );
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
	GIVEN( "a save from before production chains" ) {
		Industry industry;
		industry.Load(AsDataNode("industry\n\tfacility \"Mining Outpost\"\n\t\tplanet Earth\n\t\tstockpile 4\n"),
			facilities);
		THEN( "the old stockpile becomes stock of the facility's output" ) {
			REQUIRE( industry.Holdings().size() == 1 );
			CHECK( industry.Holdings().front().Stock("Metal") == 4 );
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
