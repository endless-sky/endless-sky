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
TEST_CASE( "AttributeAccessor::IsMultiplier", "[AttributeAccessor][IsMultiplier]" ) {
	SECTION( "Base effect" ) {
		CHECK( !AttributeAccessor(PASSIVE, SHIELDS).IsMultiplier() );
		CHECK( !AttributeAccessor(PASSIVE, ENERGY).IsMultiplier() );
		CHECK( !AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1)).IsMultiplier() );
	}
	SECTION( "Multiplier effect" ) {
		CHECK( AttributeAccessor(PASSIVE, SHIELDS).Multiplier().IsMultiplier() );
		CHECK( AttributeAccessor(PASSIVE, ENERGY).Multiplier().IsMultiplier() );
		CHECK( AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Multiplier().IsMultiplier());
	}
	SECTION( "Relative effect" ) {
		CHECK( !AttributeAccessor(PASSIVE, SHIELDS).Relative().IsMultiplier() );
		CHECK( !AttributeAccessor(PASSIVE, ENERGY).Relative().IsMultiplier() );
		CHECK( !AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Relative().IsMultiplier());
	}
	SECTION( "Relative multiplier effect" ) {
		CHECK( AttributeAccessor(PASSIVE, SHIELDS).Multiplier().Relative().IsMultiplier() );
		CHECK( AttributeAccessor(PASSIVE, ENERGY).Multiplier().Relative().IsMultiplier() );
		CHECK( AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Multiplier().Relative().IsMultiplier());
	}
}

TEST_CASE( "AttributeAccessor::IsRelative", "[AttributeAccessor][IsRelative]" ) {
	SECTION( "Base effect" ) {
		CHECK( !AttributeAccessor(PASSIVE, SHIELDS).IsRelative() );
		CHECK( !AttributeAccessor(PASSIVE, ENERGY).IsRelative() );
		CHECK( !AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1)).IsRelative() );
	}
	SECTION( "Multiplier effect" ) {
		CHECK( !AttributeAccessor(PASSIVE, SHIELDS).Multiplier().IsRelative() );
		CHECK( !AttributeAccessor(PASSIVE, ENERGY).Multiplier().IsRelative() );
		CHECK( !AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Multiplier().IsRelative());
	}
	SECTION( "Relative effect" ) {
		CHECK( AttributeAccessor(PASSIVE, SHIELDS).Relative().IsRelative() );
		CHECK( AttributeAccessor(PASSIVE, ENERGY).Relative().IsRelative() );
		CHECK( AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Relative().IsRelative());
	}
	SECTION( "Relative multiplier effect" ) {
		CHECK( AttributeAccessor(PASSIVE, SHIELDS).Multiplier().Relative().IsRelative() );
		CHECK( AttributeAccessor(PASSIVE, ENERGY).Multiplier().Relative().IsRelative() );
		CHECK( AttributeAccessor(PASSIVE, static_cast<AttributeEffectType>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Multiplier().Relative().IsRelative());
	}
}

TEST_CASE( "AttributeAccessor::Relative", "[AttributeAccessor][Relative]" ) {
	SECTION( "Equality" ) {
		CHECK( AttributeAccessor(PASSIVE, SHIELDS).Relative() == AttributeAccessor(PASSIVE, SHIELDS).Relative().Relative() );
		CHECK( AttributeAccessor(PASSIVE, SHIELDS).Relative().Multiplier() ==
				AttributeAccessor(PASSIVE, SHIELDS).Multiplier().Relative() );
	}
}

TEST_CASE( "AttributeAccessor::Multiplier", "[AttributeAccessor][Multiplier]" ) {
	SECTION( "Equality" ) {
		CHECK( AttributeAccessor(PASSIVE, SHIELDS).Multiplier() ==
				AttributeAccessor(PASSIVE, SHIELDS).Multiplier().Multiplier() );
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
		CHECK( !AttributeAccessor(AFTERBURNING, ION).IsRequirement() );
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
