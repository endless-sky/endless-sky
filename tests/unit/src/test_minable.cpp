/* test_minable.cpp
Copyright (c) 2026 by Endless Sky contributors

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

#include "../../../source/Minable.h"

#include "datanode-factory.h"
#include "../../../source/Flotsam.h"
#include "../../../source/Random.h"
#include "../../../source/Visual.h"

#include <list>
#include <memory>
#include <vector>

namespace
{ // test namespace

	// #region unit tests
	SCENARIO( "Destroying yottrite asteroids", "[minable][yottrite]" )
	{
		// Testing the addition of minimum payload drop,
		// and that a minable with a minimum of 1 would not yield 0 drops.
		GIVEN( "the yottrite payload" )
		{
			// Dummy data based on the yottrite payload in harvesting.txt.
			const DataNode definition = AsDataNode(R"(minable "yottrite"
	payload "Yottrite" 1 5
		"toughness" 50000
)");

			Minable sample;
			sample.Load(definition, nullptr);
			REQUIRE( sample.GetPayload().size() == 1 );

			WHEN( "1000 asteroids are destroyed" )
			{
				Random::Seed(20260929);
				bool anyZeroDrops = false;
				for(int i = 1; i <= 1000; i++)
				{
					Minable asteroid = sample;
					asteroid.Place(0., 1000.);
					asteroid.Kill();

					std::vector<Visual> visuals;
					std::list<std::shared_ptr<Flotsam>> flotsam;
					asteroid.Move(visuals, flotsam);
					if(flotsam.empty())
					{
						anyZeroDrops = true;
						break;
					}
				}

				THEN( "none drop zero yottrite" )
				{
					CHECK_FALSE( anyZeroDrops );
				}
			}
		}
	}
	// #endregion unit tests

} // test namespace
