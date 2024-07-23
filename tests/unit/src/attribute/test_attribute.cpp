/* test_attribute.cpp
Copyright (c) 2023 by tibetiroka

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
#include "../../../../source/attribute/Attribute.h"

// ... and any system includes needed for the test file.
#include <map>
namespace { // test namespace
// #region mock data
// #endregion mock data



// #region unit tests
TEST_CASE( "AttributeAccessor::HasModifier", "[AttributeAccessor][HasModifier]" ) {
	SECTION( "Base effect" ) {
		CHECK( !AttributeAccessor(PASSIVE, SHIELDS).HasModifier(Modifier::MULTIPLIER) );
		CHECK( !AttributeAccessor(PASSIVE, ENERGY).HasModifier(Modifier::MULTIPLIER) );
		CHECK( !AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.HasModifier(Modifier::MULTIPLIER));
		CHECK( !AttributeAccessor(PASSIVE, SHIELDS).HasModifier(Modifier::RELATIVE) );
		CHECK( !AttributeAccessor(PASSIVE, ENERGY).HasModifier(Modifier::RELATIVE) );
		CHECK( !AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.HasModifier(Modifier::RELATIVE));
	}
	SECTION( "Multiplier effect" ) {
		CHECK( AttributeAccessor(PASSIVE, SHIELDS, Modifier::MULTIPLIER).HasModifier(Modifier::MULTIPLIER) );
		CHECK( AttributeAccessor(PASSIVE, ENERGY, Modifier::MULTIPLIER).HasModifier(Modifier::MULTIPLIER) );
		CHECK( AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1), Modifier::MULTIPLIER)
				.HasModifier(Modifier::MULTIPLIER));
		CHECK( !AttributeAccessor(PASSIVE, SHIELDS, Modifier::MULTIPLIER).HasModifier(Modifier::RELATIVE) );
		CHECK( !AttributeAccessor(PASSIVE, ENERGY, Modifier::MULTIPLIER).HasModifier(Modifier::RELATIVE) );
		CHECK( !AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1), Modifier::MULTIPLIER)
				.HasModifier(Modifier::RELATIVE));
	}
	SECTION( "Relative effect" ) {
		CHECK( !AttributeAccessor(PASSIVE, SHIELDS, Modifier::RELATIVE).HasModifier(Modifier::MULTIPLIER) );
		CHECK( !AttributeAccessor(PASSIVE, ENERGY, Modifier::RELATIVE).HasModifier(Modifier::MULTIPLIER) );
		CHECK( !AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1), Modifier::RELATIVE)
				.HasModifier(Modifier::MULTIPLIER));
		CHECK( AttributeAccessor(PASSIVE, SHIELDS, Modifier::RELATIVE).HasModifier(Modifier::RELATIVE) );
		CHECK( AttributeAccessor(PASSIVE, ENERGY, Modifier::RELATIVE).HasModifier(Modifier::RELATIVE) );
		CHECK( AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1), Modifier::RELATIVE)
				.HasModifier(Modifier::RELATIVE));
	}
}

TEST_CASE( "AttributeAccessor::WithModifier", "[AttributeAccessor][WithModifier]" ) {
	SECTION( "Equality" ) {
		CHECK( AttributeAccessor(PASSIVE, SHIELDS, Modifier::RELATIVE) ==
				AttributeAccessor(PASSIVE, SHIELDS, Modifier::RELATIVE).WithModifier(Modifier::RELATIVE));
		CHECK( AttributeAccessor(PASSIVE, SHIELDS, Modifier::RELATIVE).WithModifier(Modifier::MULTIPLIER) ==
				AttributeAccessor(PASSIVE, SHIELDS, Modifier::MULTIPLIER));
		CHECK( AttributeAccessor(PASSIVE, SHIELDS, Modifier::MULTIPLIER) ==
				AttributeAccessor(PASSIVE, SHIELDS, Modifier::MULTIPLIER).WithModifier(Modifier::MULTIPLIER) );
	}
}

TEST_CASE( "AttributeAccessor::IsRequirement", "[AttributeAccessor][IsRequirement]" ) {
	SECTION( "Passive effects" ) {
		CHECK( !AttributeAccessor(PASSIVE, SHIELDS).IsRequirement() );
		CHECK( !AttributeAccessor(PASSIVE, ENERGY).IsRequirement() );
	}
	SECTION( "Matching action and effect" ) {
		CHECK( !AttributeAccessor(SHIELD_GENERATION, SHIELDS).IsRequirement() );
	}
	SECTION( "Requirements" ) {
		CHECK( AttributeAccessor(SHIELD_GENERATION, ENERGY).IsRequirement() );
		CHECK( AttributeAccessor(THRUSTING, FUEL).IsRequirement() );
		CHECK( AttributeAccessor(THRUSTING, HULL).IsRequirement() );
		CHECK( AttributeAccessor(ACTIVE_COOL, ENERGY).IsRequirement() );
	}
	SECTION( "Other effects" ) {
		CHECK( !AttributeAccessor(SHIELD_GENERATION, HEAT).IsRequirement() );
		CHECK( !AttributeAccessor(THRUSTING, COOLING).IsRequirement() );
		CHECK( !AttributeAccessor(AFTERBURNING, ENERGY, Modifier::OVER_TIME).IsRequirement() );
		CHECK( !AttributeAccessor(ACTIVE_COOL, ENERGY, Modifier::OVER_TIME).IsRequirement() );
	}
}

TEST_CASE( "Attribute::GetLegacyName", "[Attribute][GetLegacyName]" ) {
	SECTION( "Legacy names" ) {
		CHECK( Attribute::GetLegacyName({DAMAGE, JAM, Modifier::OVER_TIME}) == "scrambling damage" );
		CHECK( Attribute::GetLegacyName({RESISTANCE, AttributeAccessor::WithModifier(ENERGY, Modifier::OVER_TIME), HEAT}) ==
				"ion resistance heat" );
		CHECK( Attribute::GetLegacyName({THRUSTING, THRUST}) == "thrust" );
	}
}
// #endregion unit tests



} // test namespace
