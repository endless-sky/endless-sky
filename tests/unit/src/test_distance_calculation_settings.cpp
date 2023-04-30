/* test_distance_calculation_settings.cpp
Copyright (c) 2023 by warp-core

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
#include "../../../source/DistanceCalculationSettings.h"

// Include a helper for creating well-formed DataNodes.
#include "datanode-factory.h"

namespace { // test namespace
// #region mock data
// #endregion mock data



// #region unit tests
TEST_CASE( "DistanceCalculationSettings Basics", "[DistanceCalculationSettings]" ) {
	using T = DistanceCalculationSettings;
	SECTION( "Class Traits" ) {
		CHECK_FALSE( std::is_trivial<T>::value );
		CHECK( std::is_standard_layout<T>::value );
		CHECK( std::is_nothrow_destructible<T>::value );
		CHECK( std::is_trivially_destructible<T>::value );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible<T>::value );
		CHECK_FALSE( std::is_trivially_default_constructible<T>::value );
		CHECK( std::is_nothrow_default_constructible<T>::value );
		CHECK( std::is_copy_constructible<T>::value );
		CHECK( std::is_trivially_copy_constructible<T>::value );
		CHECK( std::is_nothrow_copy_constructible<T>::value );
		CHECK( std::is_move_constructible<T>::value );
		CHECK( std::is_trivially_move_constructible<T>::value );
		CHECK( std::is_nothrow_move_constructible<T>::value );
	}
	SECTION( "Copy Traits" ) {
		CHECK( std::is_copy_assignable<T>::value );
		CHECK( std::is_trivially_copyable<T>::value );
		CHECK( std::is_trivially_copy_assignable<T>::value );
		CHECK( std::is_nothrow_copy_assignable<T>::value );
	}
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable<T>::value );
		CHECK( std::is_trivially_move_assignable<T>::value );
		CHECK( std::is_nothrow_move_assignable<T>::value );
	}
}

SCENARIO( "A wormmhole strategy and boolean must be stored.", "[DistanceCalculationSettings]" ) {
	GIVEN( "No initial values" ) {
		DistanceCalculationSettings a;
		WHEN( "the settings are created" ) {
			THEN( "it represents (NONE, false)" ) {
				CHECK( a.WormholeStrat() == WormholeStrategy::NONE );
				CHECK( a.AssumesJumpDrive() == false );
			}
		}
	}
}

const DataNode defaultNode = AsDataNode("node\n\t\"no wormholes\"");
const DataNode jdNode = AsDataNode("node\n\t\"assumes jump drive\"");
const DataNode unrestrictedWormholesNode = AsDataNode("node\n\t\"only unrestricted wormholes\"");
const DataNode unrestrictedWormholesJDNode =
		AsDataNode("node\n\t\"only unrestricted wormholes\"\n\t\"assumes jump drive\"");
const DataNode allWormholesNode = AsDataNode("node\n\t\"all wormholes\"");
const DataNode allWormholesJDNode = AsDataNode("node\n\t\"all wormholes\"\n\t\"assumes jump drive\"");

TEST_CASE( "DistanceCalculationSettings::Load", "[DistanceCalculationSettings::Load]") {
	using T = DistanceCalculationSettings;
	SECTION( "No wormholes, no jump drive") {
		T settings(defaultNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::NONE );
		CHECK( settings.AssumesJumpDrive() == false );
	}
	SECTION( "No wormholes, jump drive") {
		T settings(jdNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::NONE );
		CHECK( settings.AssumesJumpDrive() == true );
	}
	SECTION( "Unrestricted wormholes, no jump drive") {
		T settings(unrestrictedWormholesNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::ONLY_UNRESTRICTED );
		CHECK( settings.AssumesJumpDrive() == false );
	}
	SECTION( "Unrestricted wormholes, jump drive") {
		T settings(unrestrictedWormholesJDNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::ONLY_UNRESTRICTED );
		CHECK( settings.AssumesJumpDrive() == true );
	}
	SECTION( "All wormholes, no jump drive") {
		T settings(allWormholesNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::ALL );
		CHECK( settings.AssumesJumpDrive() == false );
	}
	SECTION( "All wormholes, jump drive") {
		T settings(allWormholesJDNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::ALL );
		CHECK( settings.AssumesJumpDrive() == true );
	}
}

TEST_CASE( "Copying DistanceCalculationSettings", "[DistanceCalculationSettings]") {
	using T = DistanceCalculationSettings;
	SECTION( "No wormholes, no jump drive") {
		T settings(defaultNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::NONE );
		CHECK( settings.AssumesJumpDrive() == false );
		T copiedSettings = settings;
		CHECK( copiedSettings.WormholeStrat() == WormholeStrategy::NONE );
		CHECK( copiedSettings.AssumesJumpDrive() == false );
	}
	SECTION( "No wormholes, jump drive") {
		T settings(jdNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::NONE );
		CHECK( settings.AssumesJumpDrive() == true );
		T copiedSettings = settings;
		CHECK( copiedSettings.WormholeStrat() == WormholeStrategy::NONE );
		CHECK( copiedSettings.AssumesJumpDrive() == true );
	}
	SECTION( "Unrestricted wormholes, no jump drive") {
		T settings(unrestrictedWormholesNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::ONLY_UNRESTRICTED );
		CHECK( settings.AssumesJumpDrive() == false );
		T copiedSettings = settings;
		CHECK( copiedSettings.WormholeStrat() == WormholeStrategy::ONLY_UNRESTRICTED );
		CHECK( copiedSettings.AssumesJumpDrive() == false );
	}
	SECTION( "Unrestricted wormholes, jump drive") {
		T settings(unrestrictedWormholesJDNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::ONLY_UNRESTRICTED );
		CHECK( settings.AssumesJumpDrive() == true );
		T copiedSettings = settings;
		CHECK( copiedSettings.WormholeStrat() == WormholeStrategy::ONLY_UNRESTRICTED );
		CHECK( copiedSettings.AssumesJumpDrive() == true );
	}
	SECTION( "All wormholes, no jump drive") {
		T settings(allWormholesNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::ALL );
		CHECK( settings.AssumesJumpDrive() == false );
		T copiedSettings = settings;
		CHECK( copiedSettings.WormholeStrat() == WormholeStrategy::ALL );
		CHECK( copiedSettings.AssumesJumpDrive() == false );
	}
	SECTION( "All wormholes, jump drive") {
		T settings(allWormholesJDNode);
		CHECK( settings.WormholeStrat() == WormholeStrategy::ALL );
		CHECK( settings.AssumesJumpDrive() == true );
		T copiedSettings = settings;
		CHECK( copiedSettings.WormholeStrat() == WormholeStrategy::ALL );
		CHECK( copiedSettings.AssumesJumpDrive() == true );
	}
}
// #endregion unit tests



} // test namespace
