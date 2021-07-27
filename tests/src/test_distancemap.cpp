/* test_distancemap.cpp
Copyright (c) 2020 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/DistanceMap.h"

// Include a helper for creating well-formed DistanceMaps.
#include "system-factory.h"

// ... and any system includes needed for the test file.
#include <algorithm>
#include <string>
#include <vector>

namespace { // test namespace
// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.


// #endregion mock data



// #region unit tests
SCENARIO( "DistanceMap can find the shortest path between systems.", "[DistanceMap]") {
	const Set<System> systems = AsSystems(
#include "../data/systems.txt"
	);
	const auto &system1 = systems.Get("System1");
	const auto &system2 = systems.Get("System2");
	const auto &system3 = systems.Get("System3");
	const auto &system4 = systems.Get("System4");
	const auto &system5 = systems.Get("System5");
	const auto &system6 = systems.Get("System6");
	const auto &system7 = systems.Get("System7");
	const auto &system8 = systems.Get("System8");
	const auto &system9 = systems.Get("System9");
	const auto &system10 = systems.Get("System10");
	const auto &system11 = systems.Get("System11");
	const auto &system12 = systems.Get("System12");
	const auto &system13 = systems.Get("System13");
	const auto &system14 = systems.Get("System14");
	const auto &system15 = systems.Get("System15");
	const auto &system16 = systems.Get("System16");
	const auto &system17 = systems.Get("System17");
	const auto &system18 = systems.Get("System18");
	const auto &system19 = systems.Get("System19");
	const auto &system20 = systems.Get("System20");
	const auto &system21 = systems.Get("System21");
	const auto &system22 = systems.Get("System22");
	const auto &system23 = systems.Get("System23");
	const auto &system24 = systems.Get("System24");
	const auto &system25 = systems.Get("System25");
	const auto &system26 = systems.Get("System26");
	const auto &system27 = systems.Get("System27");
	const auto &system28 = systems.Get("System28");

	GIVEN( "A simple DistanceMap" ) {
		const DistanceMap map(system1, -1, 3);
		THEN( "it has the mapped the correct routes" ) {
			CHECK(map.End() == system1);
			CHECK(map.HasRoute(system1));
			CHECK(map.HasRoute(system2));
			CHECK(map.HasRoute(system3));
			CHECK(map.HasRoute(system4));
			CHECK(map.HasRoute(system5));
			CHECK(map.HasRoute(system6));
			CHECK(map.HasRoute(system7));
			CHECK(map.HasRoute(system8));
			CHECK(map.HasRoute(system9));
			CHECK_FALSE(map.HasRoute(system10));
			CHECK(map.HasRoute(system11));
			CHECK(map.HasRoute(system12));
			CHECK_FALSE(map.HasRoute(system13));
			CHECK_FALSE(map.HasRoute(system14));
			CHECK_FALSE(map.HasRoute(system15));
			CHECK_FALSE(map.HasRoute(system16));
			CHECK_FALSE(map.HasRoute(system17));
			CHECK_FALSE(map.HasRoute(system18));
			CHECK(map.HasRoute(system19));
			CHECK(map.HasRoute(system20));
			CHECK_FALSE(map.HasRoute(system21));
			CHECK(map.HasRoute(system22));
			CHECK(map.HasRoute(system23));
			CHECK_FALSE(map.HasRoute(system24));
			CHECK_FALSE(map.HasRoute(system25));
			CHECK_FALSE(map.HasRoute(system26));
			CHECK_FALSE(map.HasRoute(system27));
			CHECK_FALSE(map.HasRoute(system28));
		}
		THEN( "it knows the distances to its routes" ) {
			CHECK(map.Days(system1) == 0);
			CHECK(map.Days(system2) == 1);
			CHECK(map.Days(system3) == 1);
			CHECK(map.Days(system4) == 1);
			CHECK(map.Days(system5) == 2);
			CHECK(map.Days(system6) == 3);
			CHECK(map.Days(system7) == 2);
			CHECK(map.Days(system8) == 2);
			CHECK(map.Days(system9) == 3);
			CHECK(map.Days(system10) == -1);
			CHECK(map.Days(system11) == 2);
			CHECK(map.Days(system12) == 3);
			CHECK(map.Days(system13) == -1);
			CHECK(map.Days(system14) == -1);
			CHECK(map.Days(system15) == -1);
			CHECK(map.Days(system16) == -1);
			CHECK(map.Days(system17) == -1);
			CHECK(map.Days(system18) == -1);
			CHECK(map.Days(system19) == 2);
			CHECK(map.Days(system20) == 3);
			CHECK(map.Days(system21) == -1);
			CHECK(map.Days(system22) == 3);
			CHECK(map.Days(system23) == 3);
			CHECK(map.Days(system24) == -1);
			CHECK(map.Days(system25) == -1);
			CHECK(map.Days(system26) == -1);
			CHECK(map.Days(system27) == -1);
			CHECK(map.Days(system28) == -1);
		}
		THEN( "it can determine the next system along the route" ) {
			CHECK_FALSE(map.Route(system1));
			CHECK(map.Route(system2) == system1);
			CHECK(map.Route(system3) == system1);
			CHECK(map.Route(system4) == system1);
			CHECK(map.Route(system5) == system2);
			CHECK(map.Route(system6) == system5);
			CHECK(map.Route(system7) == system4);
			CHECK(map.Route(system8) == system4);
			CHECK(map.Route(system9) == system8);
			CHECK_FALSE(map.Route(system10));
			CHECK(map.Route(system11) == system2);
			CHECK(map.Route(system12) == system5);
			CHECK_FALSE(map.Route(system13));
			CHECK_FALSE(map.Route(system14));
			CHECK_FALSE(map.Route(system15));
			CHECK_FALSE(map.Route(system16));
			CHECK_FALSE(map.Route(system17));
			CHECK_FALSE(map.Route(system18));
			CHECK(map.Route(system19) == system3);
			CHECK(map.Route(system20) == system7);
			CHECK_FALSE(map.Route(system21));
			CHECK(map.Route(system22) == system11);
			CHECK(map.Route(system23) == system11);
			CHECK_FALSE(map.Route(system24));
			CHECK_FALSE(map.Route(system25));
			CHECK_FALSE(map.Route(system26));
			CHECK_FALSE(map.Route(system27));
			CHECK_FALSE(map.Route(system28));
		}
		THEN( "it knows the fuel required to its destination" ) {
			CHECK(map.RequiredFuel(system1, system1) == 0);
			CHECK(map.RequiredFuel(system2, system1) == 100);
			CHECK(map.RequiredFuel(system3, system1) == 100);
			CHECK(map.RequiredFuel(system4, system1) == 100);
			CHECK(map.RequiredFuel(system5, system1) == 200);
			CHECK(map.RequiredFuel(system6, system1) == 300);
			CHECK(map.RequiredFuel(system7, system1) == 200);
			CHECK(map.RequiredFuel(system8, system1) == 200);
			CHECK(map.RequiredFuel(system9, system1) == 300);
			CHECK(map.RequiredFuel(system10, system1) == -1);
			CHECK(map.RequiredFuel(system11, system1) == 200);
			CHECK(map.RequiredFuel(system12, system1) == 300);
			CHECK(map.RequiredFuel(system13, system1) == -1);
			CHECK(map.RequiredFuel(system14, system1) == -1);
			CHECK(map.RequiredFuel(system15, system1) == -1);
			CHECK(map.RequiredFuel(system16, system1) == -1);
			CHECK(map.RequiredFuel(system17, system1) == -1);
			CHECK(map.RequiredFuel(system18, system1) == -1);
			CHECK(map.RequiredFuel(system19, system1) == 200);
			CHECK(map.RequiredFuel(system20, system1) == 300);
			CHECK(map.RequiredFuel(system21, system1) == -1);
			CHECK(map.RequiredFuel(system22, system1) == 300);
			CHECK(map.RequiredFuel(system23, system1) == 300);
			CHECK(map.RequiredFuel(system24, system1) == -1);
			CHECK(map.RequiredFuel(system25, system1) == -1);
			CHECK(map.RequiredFuel(system26, system1) == -1);
			CHECK(map.RequiredFuel(system27, system1) == -1);
			CHECK(map.RequiredFuel(system28, system1) == -1);
		}
		THEN( "it knows the fuel required to a stopover" ) {
			CHECK(map.RequiredFuel(system5, system2) == 100);
			CHECK(map.RequiredFuel(system6, system5) == 100);
			CHECK(map.RequiredFuel(system7, system4) == 100);
			CHECK(map.RequiredFuel(system8, system4) == 100);
			CHECK(map.RequiredFuel(system9, system8) == 100);
			CHECK(map.RequiredFuel(system11, system2) == 100);
			CHECK(map.RequiredFuel(system12, system5) == 100);
			CHECK(map.RequiredFuel(system19, system3) == 100);
			CHECK(map.RequiredFuel(system20, system7) == 100);
			CHECK(map.RequiredFuel(system22, system11) == 100);
			CHECK(map.RequiredFuel(system23, system11) == 100);
		}
		THEN( "it can return all of its systems" ) {
			std::vector<const char *> systems{
				"System1", "System2", "System3", "System4", "System5",
				"System6", "System7", "System8", "System9", "System11",
				"System12", "System19", "System20", "System22", "System23"};

			auto mapSystems = map.Systems();
			CHECK(mapSystems.size() == systems.size());
			for(auto system : mapSystems)
				CHECK(std::find(systems.begin(), systems.end(), system->Name()) != systems.end());
		}
	}
}
// #endregion unit tests



} // test namespace
