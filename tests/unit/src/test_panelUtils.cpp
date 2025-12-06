/* test_panelUtils.cpp
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

#include "../../../source/PanelUtils.h"
#include "../../../source/Command.h"

#include <SDL2/SDL.h>

namespace { // test namespace

SCENARIO("HandleZoomKey recognizes zoom keys", "[PanelUtils]") {
	GIVEN("No active command") {
		Command empty;

		THEN("Minus key is recognized") {
			REQUIRE(PanelUtils::HandleZoomKey(SDLK_MINUS, empty, false));
		}

		THEN("Keypad minus key is recognized") {
			REQUIRE(PanelUtils::HandleZoomKey(SDLK_KP_MINUS, empty, false));
		}

		THEN("Plus key is recognized") {
			REQUIRE(PanelUtils::HandleZoomKey(SDLK_PLUS, empty, false));
		}

		THEN("Keypad plus key is recognized") {
			REQUIRE(PanelUtils::HandleZoomKey(SDLK_KP_PLUS, empty, false));
		}

		THEN("Equals key is recognized (alternate plus)") {
			REQUIRE(PanelUtils::HandleZoomKey(SDLK_EQUALS, empty, false));
		}

		THEN("Other keys are not recognized") {
			REQUIRE_FALSE(PanelUtils::HandleZoomKey(SDLK_a, empty, false));
			REQUIRE_FALSE(PanelUtils::HandleZoomKey(SDLK_SPACE, empty, false));
			REQUIRE_FALSE(PanelUtils::HandleZoomKey(SDLK_RETURN, empty, false));
		}
	}
}

SCENARIO("HandleZoomKey respects checkCommand parameter", "[PanelUtils]") {
	GIVEN("An empty command") {
		Command empty;

		WHEN("checkCommand is false") {
			THEN("Zoom keys are handled") {
				REQUIRE(PanelUtils::HandleZoomKey(SDLK_MINUS, empty, false));
			}
		}

		WHEN("checkCommand is true and command is empty") {
			THEN("Zoom keys are handled") {
				REQUIRE(PanelUtils::HandleZoomKey(SDLK_MINUS, empty, true));
			}
		}
	}
}

SCENARIO("HandleZoomScroll recognizes scroll direction", "[PanelUtils]") {
	THEN("Negative dy (scroll down) returns true") {
		REQUIRE(PanelUtils::HandleZoomScroll(-1.));
	}

	THEN("Positive dy (scroll up) returns true") {
		REQUIRE(PanelUtils::HandleZoomScroll(1.));
	}

	THEN("Zero dy returns false") {
		REQUIRE_FALSE(PanelUtils::HandleZoomScroll(0.));
	}

	THEN("Small negative values are handled") {
		REQUIRE(PanelUtils::HandleZoomScroll(-0.1));
	}

	THEN("Small positive values are handled") {
		REQUIRE(PanelUtils::HandleZoomScroll(0.1));
	}
}

} // test namespace
