/* test_cameraController.cpp
Copyright (c) 2024 by the Endless Sky developers

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

#include "../../../source/CameraController.h"
#include "../../../source/FollowShipCamera.h"
#include "../../../source/FreeCamera.h"
#include "../../../source/OrbitPlanetCamera.h"
#include "../../../source/Point.h"

namespace { // test namespace

SCENARIO("FreeCamera basic movement", "[CameraController]") {
	GIVEN("A free camera at origin") {
		FreeCamera camera;

		THEN("Initial position is at origin") {
			REQUIRE(camera.GetTarget().X() == 0.);
			REQUIRE(camera.GetTarget().Y() == 0.);
		}

		THEN("Initial velocity is zero") {
			REQUIRE(camera.GetVelocity().X() == 0.);
			REQUIRE(camera.GetVelocity().Y() == 0.);
		}

		WHEN("Movement input is applied") {
			camera.SetMovement(1., 0.);
			camera.Step();

			THEN("Camera moves in that direction") {
				REQUIRE(camera.GetTarget().X() > 0.);
			}

			THEN("Velocity reflects movement") {
				REQUIRE(camera.GetVelocity().X() > 0.);
			}
		}
	}
}

SCENARIO("FreeCamera mode name", "[CameraController]") {
	GIVEN("A free camera") {
		FreeCamera camera;

		THEN("Mode name is 'Free Camera'") {
			REQUIRE(camera.ModeName() == "Free Camera");
		}
	}
}

SCENARIO("FollowShipCamera without ships", "[CameraController]") {
	GIVEN("A follow camera with no ships") {
		FollowShipCamera camera;

		THEN("Target is at origin") {
			REQUIRE(camera.GetTarget().X() == 0.);
			REQUIRE(camera.GetTarget().Y() == 0.);
		}

		THEN("Velocity is zero") {
			REQUIRE(camera.GetVelocity().X() == 0.);
			REQUIRE(camera.GetVelocity().Y() == 0.);
		}

		THEN("No observed ship") {
			REQUIRE(camera.GetObservedShip() == nullptr);
		}

		THEN("Mode name is 'Follow Ship'") {
			REQUIRE(camera.ModeName() == "Follow Ship");
		}
	}
}

SCENARIO("OrbitPlanetCamera without objects", "[CameraController]") {
	GIVEN("An orbit camera with no stellar objects") {
		OrbitPlanetCamera camera;

		THEN("Target is at origin") {
			REQUIRE(camera.GetTarget().X() == 0.);
			REQUIRE(camera.GetTarget().Y() == 0.);
		}

		THEN("Mode name is 'Orbit Planet'") {
			REQUIRE(camera.ModeName() == "Orbit Planet");
		}
	}
}

SCENARIO("FreeCamera position can be set directly", "[CameraController]") {
	GIVEN("A free camera") {
		FreeCamera camera;

		WHEN("Position is set to a specific location") {
			Point newPos(100., 200.);
			camera.SetPosition(newPos);

			THEN("GetTarget returns that position") {
				REQUIRE(camera.GetTarget().X() == 100.);
				REQUIRE(camera.GetTarget().Y() == 200.);
			}
		}
	}
}

SCENARIO("CycleTarget virtual method works on base class pointer", "[CameraController]") {
	GIVEN("A FollowShipCamera accessed through base pointer") {
		std::unique_ptr<CameraController> camera = std::make_unique<FollowShipCamera>();

		THEN("CycleTarget can be called without crash") {
			// Should not crash even with no ships
			camera->CycleTarget();
			REQUIRE(true);
		}
	}

	GIVEN("An OrbitPlanetCamera accessed through base pointer") {
		std::unique_ptr<CameraController> camera = std::make_unique<OrbitPlanetCamera>();

		THEN("CycleTarget can be called without crash") {
			// Should not crash even with no objects
			camera->CycleTarget();
			REQUIRE(true);
		}
	}
}

SCENARIO("SetMovement virtual method works on base class pointer", "[CameraController]") {
	GIVEN("A FreeCamera accessed through base pointer") {
		std::unique_ptr<CameraController> camera = std::make_unique<FreeCamera>();

		WHEN("SetMovement is called through base pointer") {
			camera->SetMovement(1., 1.);
			camera->Step();

			THEN("Camera moves") {
				REQUIRE(camera->GetTarget().X() > 0.);
				REQUIRE(camera->GetTarget().Y() > 0.);
			}
		}
	}

	GIVEN("A FollowShipCamera accessed through base pointer") {
		std::unique_ptr<CameraController> camera = std::make_unique<FollowShipCamera>();

		THEN("SetMovement can be called without crash (no-op)") {
			camera->SetMovement(1., 1.);
			REQUIRE(true);
		}
	}
}

} // test namespace
