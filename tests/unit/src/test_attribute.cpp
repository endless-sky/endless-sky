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
#include "../../../source/attribute/Attribute.h"

// ... and any system includes needed for the test file.
#include <map>
namespace { // test namespace
// #region mock data
// #endregion mock data



// #region unit tests
TEST_CASE( "AttributeAccess::IsMultiplier", "[AttributeAccess][IsMultiplier]" ) {
	SECTION( "Base effect" ) {
		CHECK( !AttributeAccess(PASSIVE, SHIELDS).IsMultiplier() );
		CHECK( !AttributeAccess(PASSIVE, ENERGY).IsMultiplier() );
		CHECK( !AttributeAccess(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1)).IsMultiplier() );
	}
	SECTION( "Multiplier effect" ) {
		CHECK( AttributeAccess(PASSIVE, SHIELDS).Multiplier().IsMultiplier() );
		CHECK( AttributeAccess(PASSIVE, ENERGY).Multiplier().IsMultiplier() );
		CHECK( AttributeAccess(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Multiplier().IsMultiplier());
	}
	SECTION( "Relative effect" ) {
		CHECK( !AttributeAccess(PASSIVE, SHIELDS).Relative().IsMultiplier() );
		CHECK( !AttributeAccess(PASSIVE, ENERGY).Relative().IsMultiplier() );
		CHECK( !AttributeAccess(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Relative().IsMultiplier());
	}
	SECTION( "Relative multiplier effect" ) {
		CHECK( AttributeAccess(PASSIVE, SHIELDS).Multiplier().Relative().IsMultiplier() );
		CHECK( AttributeAccess(PASSIVE, ENERGY).Multiplier().Relative().IsMultiplier() );
		CHECK( AttributeAccess(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Multiplier().Relative().IsMultiplier());
	}
}

TEST_CASE( "AttributeAccess::IsRelative", "[AttributeAccess][IsRelative]" ) {
	SECTION( "Base effect" ) {
		CHECK( !AttributeAccess(PASSIVE, SHIELDS).IsRelative() );
		CHECK( !AttributeAccess(PASSIVE, ENERGY).IsRelative() );
		CHECK( !AttributeAccess(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1)).IsRelative() );
	}
	SECTION( "Multiplier effect" ) {
		CHECK( !AttributeAccess(PASSIVE, SHIELDS).Multiplier().IsRelative() );
		CHECK( !AttributeAccess(PASSIVE, ENERGY).Multiplier().IsRelative() );
		CHECK( !AttributeAccess(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Multiplier().IsRelative());
	}
	SECTION( "Relative effect" ) {
		CHECK( AttributeAccess(PASSIVE, SHIELDS).Relative().IsRelative() );
		CHECK( AttributeAccess(PASSIVE, ENERGY).Relative().IsRelative() );
		CHECK( AttributeAccess(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Relative().IsRelative());
	}
	SECTION( "Relative multiplier effect" ) {
		CHECK( AttributeAccess(PASSIVE, SHIELDS).Multiplier().Relative().IsRelative() );
		CHECK( AttributeAccess(PASSIVE, ENERGY).Multiplier().Relative().IsRelative() );
		CHECK( AttributeAccess(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Multiplier().Relative().IsRelative());
	}
}

TEST_CASE( "AttributeAccess::Relative", "[AttributeAccess][Relative]" ) {
	SECTION( "Equality" ) {
		CHECK( AttributeAccess(PASSIVE, SHIELDS).Relative() == AttributeAccess(PASSIVE, SHIELDS).Relative().Relative() );
		CHECK( AttributeAccess(PASSIVE, SHIELDS).Relative().Multiplier() ==
				AttributeAccess(PASSIVE, SHIELDS).Multiplier().Relative() );
	}
}

TEST_CASE( "AttributeAccess::Multiplier", "[AttributeAccess][Multiplier]" ) {
	SECTION( "Equality" ) {
		CHECK( AttributeAccess(PASSIVE, SHIELDS).Multiplier() ==
				AttributeAccess(PASSIVE, SHIELDS).Multiplier().Multiplier() );
	}
}

TEST_CASE( "AttributeAccess::IsRequirement", "[AttributeAccess][IsRequirement]" ) {
	SECTION( "Passive effects" ) {
		CHECK( !AttributeAccess(PASSIVE, SHIELDS).IsRequirement() );
		CHECK( !AttributeAccess(PASSIVE, ENERGY).IsRequirement() );
	}
	SECTION( "Matching action and effect" ) {
		CHECK( !AttributeAccess(SHIELD_GENERATION, SHIELDS).IsRequirement() );
	}
	SECTION( "Requirements" ) {
		CHECK( AttributeAccess(SHIELD_GENERATION, ENERGY).IsRequirement() );
		CHECK( AttributeAccess(THRUSTING, FUEL).IsRequirement() );
		CHECK( AttributeAccess(THRUSTING, HULL).IsRequirement() );
		CHECK( AttributeAccess(ACTIVE_COOL, ENERGY).IsRequirement() );
	}
	SECTION( "Other effects" ) {
		CHECK( !AttributeAccess(SHIELD_GENERATION, HEAT).IsRequirement() );
		CHECK( !AttributeAccess(THRUSTING, COOLING).IsRequirement() );
		CHECK( !AttributeAccess(AFTERBURNING, ION).IsRequirement() );
	}
}

TEST_CASE( "Attribute::GetLegacyName", "[Attribute][GetLegacyName]" ) {
	SECTION( "Legacy names" ) {
		CHECK( Attribute::GetLegacyName({DAMAGE, SCRAMBLE}) == "scrambling damage" );
		CHECK( Attribute::GetLegacyName({RESISTANCE, ION, HEAT}) == "ion resistance heat" );
		CHECK( Attribute::GetLegacyName({THRUSTING, THRUST}) == "thrust" );
	}
}
// #endregion unit tests



} // test namespace
