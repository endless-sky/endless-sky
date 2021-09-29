/* test_distancemap.cpp
Copyright (c) 2021 by quyykk

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

#include "datanode-factory.h"

#include "../../source/GameData.h"
#include "../../source/GameObjects.h"

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
	GameObjects objects;
	GameData::SetObjects(objects);

	GIVEN( "A simple DistanceMap" ) {
		objects.Load(AsDataFile(R"(
system One
	pos -10 0
	link Two
system Two
	pos 0 0
	link One
	link Three
system Three
	pos 10 0
	link Two
)"));
		const auto &one = objects.systems.Get("One");
		const auto &two = objects.systems.Get("Two");
		const auto &three = objects.systems.Get("Three");
		const DistanceMap map(one);
		THEN( "it has the mapped the correct routes" ) {
			CHECK(map.End() == one);
			CHECK(map.HasRoute(one));
			CHECK(map.HasRoute(two));
			CHECK(map.HasRoute(three));
		}
		THEN( "it knows the distances to its routes" ) {
			CHECK(map.Days(one) == 0);
			CHECK(map.Days(two) == 1);
			CHECK(map.Days(three) == 2);
		}
		THEN( "it can determine the next system along the route" ) {
			CHECK_FALSE(map.Route(one));
			CHECK(map.Route(two) == one);
			CHECK(map.Route(three) == two);
		}
		THEN( "it knows the fuel required to its destination" ) {
			CHECK(map.RequiredFuel(one, one) == 0);
			CHECK(map.RequiredFuel(one, two) == 100);
			CHECK(map.RequiredFuel(one, three) == 200);
			CHECK(map.RequiredFuel(two, one) == 100);
			CHECK(map.RequiredFuel(three, one) == 200);
		}
		THEN( "it knows the fuel required to a stopover" ) {
			CHECK(map.RequiredFuel(two, three) == 100);
			CHECK(map.RequiredFuel(three, two) == 100);
		}
		THEN( "it can return all of its systems" ) {
			auto mapSystems = map.Systems();
			size_t count = 0;
			for(auto system : {one, two, three})
			{
				CHECK(mapSystems.count(system));
				++count;
			}
			CHECK(mapSystems.size() == count);
		}
	}
}
// #endregion unit tests



} // test namespace
