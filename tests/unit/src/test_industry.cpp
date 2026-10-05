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

#include <map>
#include <string>
#include <utility>



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

const std::string DEPOT = R"(facility "Depot"
	warehouse 50
	uninhabited
)";

Set<Facility> MakeFacilities()
{
	Set<Facility> facilities;
	facilities.Get("Mining Outpost")->Load(AsDataNode(OUTPOST));
	facilities.Get("Machine Works")->Load(AsDataNode(WORKS));
	facilities.Get("Depot")->Load(AsDataNode(DEPOT));
	return facilities;
}

// Markets and distances for testing. Planets are one jump apart unless set
// otherwise, and have no market unless given a price.
class MockWorld : public Industry::World {
public:
	virtual int Price(const std::string &planet, const std::string &commodity) const override
	{
		auto it = prices.find({planet, commodity});
		return it == prices.end() ? 0 : it->second;
	}
	virtual void Trade(const std::string &planet, const std::string &commodity, int tons) override
	{
		traded[{planet, commodity}] += tons;
	}
	virtual int Jumps(const std::string &from, const std::string &to) const override
	{
		if(from == to)
			return 0;
		auto it = jumps.find({from, to});
		return it == jumps.end() ? 1 : it->second;
	}

	std::map<std::pair<std::string, std::string>, int> prices;
	std::map<std::pair<std::string, std::string>, int> traded;
	std::map<std::pair<std::string, std::string>, int> jumps;
};
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

SCENARIO( "Loading a story facility", "[Facility]" ) {
	GIVEN( "a facility for uninhabited worlds that must be unlocked" ) {
		Facility facility;
		facility.Load(AsDataNode(R"(facility "Survey Drill"
	output "Core Samples" 1
	uninhabited
	requires "expanded: survey drill"
	flavor "The drill on <planet> is fine."
	flavor `It found a "spike".`
)"));
		THEN( "it can be built on any uninhabited world, and nowhere else" ) {
			CHECK( facility.CanBuildOn("Rock", {}, false) );
			CHECK_FALSE( facility.CanBuildOn("Earth", {}, true) );
		}
		THEN( "its requirement and flavor are read" ) {
			CHECK( facility.Requirement() == "expanded: survey drill" );
			REQUIRE( facility.Flavor().size() == 2 );
			CHECK( facility.Flavor()[1] == "It found a \"spike\"." );
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
			MockWorld world;
			world.prices[{"New Greenland", "Metal"}] = 300;
			const Industry::DayReport report = industry.AdvanceDay(1000, &world);
			THEN( "the day's output is sold, and the market is told" ) {
				CHECK( report.sales == 600 );
				CHECK( holding.Stock("Metal") == 0 );
				CHECK( world.traded[{"New Greenland", "Metal"}] == -2 );
			}
		}
		WHEN( "auto-sell is on but there is no market" ) {
			holding.autoSell = true;
			MockWorld world;
			const Industry::DayReport report = industry.AdvanceDay(1000, &world);
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

SCENARIO( "Using a warehouse", "[Industry]" ) {
	const Set<Facility> facilities = MakeFacilities();
	const Facility &depot = *facilities.Find("Depot");
	GIVEN( "a planet with no warehouse" ) {
		Industry industry;
		THEN( "nothing can be stored there" ) {
			CHECK( industry.WarehouseCapacity("Rock") == 0 );
			CHECK( industry.Store("Rock", "Metal", 10) == 0 );
		}
	}
	GIVEN( "two depots on a planet" ) {
		Industry industry;
		industry.Build(depot, "Rock");
		industry.Build(depot, "Rock");
		THEN( "the warehouse holds any mix of goods up to its capacity" ) {
			CHECK( industry.WarehouseCapacity("Rock") == 100 );
			CHECK( industry.Store("Rock", "Metal", 70) == 70 );
			CHECK( industry.Store("Rock", "Food", 70) == 30 );
			CHECK( industry.WarehouseUsed("Rock") == 100 );
			CHECK( industry.Retrieve("Rock", "Metal", 100) == 70 );
			CHECK( industry.WarehouseUsed("Rock") == 30 );
		}
	}
}

SCENARIO( "Running freight routes", "[Industry]" ) {
	const Set<Facility> facilities = MakeFacilities();
	const Facility &outpost = *facilities.Find("Mining Outpost");
	const Facility &works = *facilities.Find("Machine Works");
	const Facility &depot = *facilities.Find("Depot");
	MockWorld world;
	Industry industry;
	industry.Build(outpost, "Rock");
	industry.Build(works, "Earth");
	Industry::Route route;
	route.commodity = "Metal";
	route.from = "Rock";
	route.to = "Earth";
	route.tons = 10;

	GIVEN( "a route from a mine to a factory one jump away" ) {
		industry.AddRoute(route);
		const Industry::DayReport report = industry.AdvanceDay(10000, &world);
		THEN( "the day's output is delivered, and freight is paid per ton and jump" ) {
			CHECK( industry.Find(outpost, "Rock")->Stock("Metal") == 0 );
			CHECK( industry.Find(works, "Earth")->Stock("Metal") == 2 );
			CHECK( report.freight == 2 * (Industry::FREIGHT_BASE + Industry::FREIGHT_PER_JUMP) );
			CHECK( industry.Routes()[0].lastMoved == 2 );
		}
	}
	GIVEN( "a route to a place that is out of reach" ) {
		world.jumps[{"Rock", "Earth"}] = -1;
		industry.AddRoute(route);
		const Industry::DayReport report = industry.AdvanceDay(10000, &world);
		THEN( "nothing moves" ) {
			CHECK( industry.Find(outpost, "Rock")->Stock("Metal") == 2 );
			CHECK( report.freight == 0 );
			CHECK( industry.Routes()[0].lastMoved == 0 );
		}
	}
	GIVEN( "a route that buys on the source market and sells at the destination" ) {
		world.prices[{"Market", "Food"}] = 100;
		world.prices[{"Earth", "Food"}] = 300;
		route.commodity = "Food";
		route.from = "Market";
		route.buy = true;
		route.sell = true;
		industry.AddRoute(route);
		const Industry::DayReport report = industry.AdvanceDay(100000, &world);
		THEN( "the full amount is bought, shipped and sold, and both markets are told" ) {
			CHECK( report.purchases == 1000 );
			CHECK( report.sales == 3000 );
			CHECK( world.traded[{"Market", "Food"}] == 10 );
			CHECK( world.traded[{"Earth", "Food"}] == -10 );
		}
	}
	GIVEN( "a route to a warehouse with little room left" ) {
		industry.Build(depot, "Depot Rock");
		industry.Store("Depot Rock", "Food", 49);
		route.to = "Depot Rock";
		industry.AddRoute(route);
		industry.AdvanceDay(10000, &world);
		THEN( "only what fits is moved" ) {
			CHECK( industry.Warehouse("Depot Rock").at("Metal") == 1 );
			CHECK( industry.Find(outpost, "Rock")->Stock("Metal") == 1 );
		}
	}
	GIVEN( "a route the player cannot afford" ) {
		world.prices[{"Market", "Food"}] = 100;
		route.commodity = "Food";
		route.from = "Market";
		route.to = "Earth";
		route.buy = true;
		route.sell = false;
		industry.Build(depot, "Earth");
		industry.AddRoute(route);
		// After upkeep (100 for the mine, 50 for the factory), 350 credits are left:
		// enough for 3 tons at 100 to buy plus 15 for freight.
		const Industry::DayReport report = industry.AdvanceDay(500, &world);
		THEN( "only what can be paid for is moved" ) {
			CHECK( report.upkeep == 150 );
			CHECK( industry.Routes()[0].lastMoved == 3 );
			CHECK( report.purchases == 300 );
			CHECK( report.freight == 45 );
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
		industry.Find(outpost, "New Greenland")->AddReport("1 Jan 3014", "Quiet day.");
		industry.Build(*facilities.Find("Depot"), "Rock");
		industry.Store("Rock", "Food", 12);
		Industry::Route route;
		route.commodity = "Metal";
		route.from = "New Greenland";
		route.to = "Earth";
		route.tons = 25;
		route.sell = true;
		industry.AddRoute(route);
		DataWriter writer;
		industry.Save(writer);
		WHEN( "it is saved and loaded again" ) {
			Industry loaded;
			loaded.Load(AsDataNode(writer.SaveToString()), facilities);
			THEN( "everything is restored" ) {
				REQUIRE( loaded.Holdings().size() == 3 );
				const Industry::Holding *mine = loaded.Find(outpost, "New Greenland");
				REQUIRE( mine );
				CHECK( mine->Stock("Metal") == 2 );
				CHECK( mine->autoSell );
				CHECK( mine->count == 1 );
				REQUIRE( mine->reports.size() == 1 );
				CHECK( mine->reports[0].first == "1 Jan 3014" );
				CHECK( mine->reports[0].second == "Quiet day." );
				const Industry::Holding *factory = loaded.Find(works, "Earth");
				REQUIRE( factory );
				CHECK( factory->count == 2 );
				CHECK( factory->Stock("Plastic") == 3 );
				CHECK_FALSE( factory->autoSell );
				CHECK( loaded.Warehouse("Rock").at("Food") == 12 );
				REQUIRE( loaded.Routes().size() == 1 );
				CHECK( loaded.Routes()[0].commodity == "Metal" );
				CHECK( loaded.Routes()[0].from == "New Greenland" );
				CHECK( loaded.Routes()[0].to == "Earth" );
				CHECK( loaded.Routes()[0].tons == 25 );
				CHECK_FALSE( loaded.Routes()[0].buy );
				CHECK( loaded.Routes()[0].sell );
			}
		}
	}
	GIVEN( "a facility with many reports" ) {
		Industry industry;
		industry.Build(outpost, "Earth");
		Industry::Holding &holding = *industry.Find(outpost, "Earth");
		for(int i = 0; i < 8; ++i)
			holding.AddReport(std::to_string(i), "report");
		THEN( "only the five most recent are kept" ) {
			REQUIRE( holding.reports.size() == 5 );
			CHECK( holding.reports.front().first == "3" );
			CHECK( holding.reports.back().first == "7" );
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
