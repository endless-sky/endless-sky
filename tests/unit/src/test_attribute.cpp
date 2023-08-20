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
TEST_CASE( "Attribute::IsMultiplier", "[Attribute][IsMultiplier]" ) {
	SECTION( "Base effect" ) {
		CHECK( !Attribute(PASSIVE, SHIELDS).IsMultiplier() );
		CHECK( !Attribute(PASSIVE, ENERGY).IsMultiplier() );
		CHECK( !Attribute(PASSIVE, static_cast<AttributeEffect>(ATTRIBUTE_EFFECT_COUNT - 1)).IsMultiplier() );
	}
	SECTION( "Multiplier effect" ) {
		CHECK( Attribute(PASSIVE, SHIELDS).Multiplier().IsMultiplier() );
		CHECK( Attribute(PASSIVE, ENERGY).Multiplier().IsMultiplier() );
		CHECK( Attribute(PASSIVE, static_cast<AttributeEffect>(ATTRIBUTE_EFFECT_COUNT - 1)).Multiplier().IsMultiplier() );
	}
	SECTION( "Relative effect" ) {
		CHECK( !Attribute(PASSIVE, SHIELDS).Relative().IsMultiplier() );
		CHECK( !Attribute(PASSIVE, ENERGY).Relative().IsMultiplier() );
		CHECK( !Attribute(PASSIVE, static_cast<AttributeEffect>(ATTRIBUTE_EFFECT_COUNT - 1)).Relative().IsMultiplier() );
	}
	SECTION( "Relative multiplier effect" ) {
		CHECK( Attribute(PASSIVE, SHIELDS).Multiplier().Relative().IsMultiplier() );
		CHECK( Attribute(PASSIVE, ENERGY).Multiplier().Relative().IsMultiplier() );
		CHECK( Attribute(PASSIVE, static_cast<AttributeEffect>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Multiplier().Relative().IsMultiplier());
	}
}

TEST_CASE( "IsRelative", "[Attribute][IsRelative]" ) {
	SECTION( "Base effect" ) {
		CHECK( !Attribute(PASSIVE, SHIELDS).IsRelative() );
		CHECK( !Attribute(PASSIVE, ENERGY).IsRelative() );
		CHECK( !Attribute(PASSIVE, static_cast<AttributeEffect>(ATTRIBUTE_EFFECT_COUNT - 1)).IsRelative() );
	}
	SECTION( "Multiplier effect" ) {
		CHECK( !Attribute(PASSIVE, SHIELDS).Multiplier().IsRelative() );
		CHECK( !Attribute(PASSIVE, ENERGY).Multiplier().IsRelative() );
		CHECK( !Attribute(PASSIVE, static_cast<AttributeEffect>(ATTRIBUTE_EFFECT_COUNT - 1)).Multiplier().IsRelative() );
	}
	SECTION( "Relative effect" ) {
		CHECK( Attribute(PASSIVE, SHIELDS).Relative().IsRelative() );
		CHECK( Attribute(PASSIVE, ENERGY).Relative().IsRelative() );
		CHECK( Attribute(PASSIVE, static_cast<AttributeEffect>(ATTRIBUTE_EFFECT_COUNT - 1)).Relative().IsRelative() );
	}
	SECTION( "Relative multiplier effect" ) {
		CHECK( Attribute(PASSIVE, SHIELDS).Multiplier().Relative().IsRelative() );
		CHECK( Attribute(PASSIVE, ENERGY).Multiplier().Relative().IsRelative() );
		CHECK( Attribute(PASSIVE, static_cast<AttributeEffect>(ATTRIBUTE_EFFECT_COUNT - 1))
				.Multiplier().Relative().IsRelative());
	}
}

TEST_CASE( "Attribute::Relative", "[Attribute][Relative]" ) {
	SECTION( "Equality" ) {
		CHECK( Attribute(PASSIVE, SHIELDS).Relative() == Attribute(PASSIVE, SHIELDS).Relative().Relative() );
		CHECK( Attribute(PASSIVE, SHIELDS).Relative().Multiplier() == Attribute(PASSIVE, SHIELDS).Multiplier().Relative() );
	}
}

TEST_CASE( "Attribute::Multiplier", "[Attribute][Multiplier]" ) {
	SECTION( "Equality" ) {
		CHECK( Attribute(PASSIVE, SHIELDS).Multiplier() == Attribute(PASSIVE, SHIELDS).Multiplier().Multiplier() );
	}
}

TEST_CASE( "Attribute::IsRequirement", "[Attribute][IsRequirement]" ) {
	SECTION( "Passive effects" ) {
		CHECK( !Attribute(PASSIVE, SHIELDS).IsRequirement() );
		CHECK( !Attribute(PASSIVE, ENERGY).IsRequirement() );
		CHECK( !Attribute(static_cast<AttributeCategory>(-1), FUEL).IsRequirement() );
	}
	SECTION( "Matching action and effect" ) {
		CHECK( !Attribute(SHIELD_GENERATION, SHIELDS).IsRequirement() );
	}
	SECTION( "Requirements" ) {
		CHECK( Attribute(SHIELD_GENERATION, ENERGY).IsRequirement() );
		CHECK( Attribute(THRUSTING, FUEL).IsRequirement() );
		CHECK( Attribute(THRUSTING, HULL).IsRequirement() );
		CHECK( Attribute(ACTIVE_COOL, ENERGY).IsRequirement() );
		CHECK( Attribute(SHIELD_GENERATION, DELAY).IsRequirement() );
	}
	SECTION( "Other effects" ) {
		CHECK( !Attribute(SHIELD_GENERATION, HEAT).IsRequirement() );
		CHECK( !Attribute(THRUSTING, COOLING).IsRequirement() );
		CHECK( !Attribute(AFTERBURNING, ION).IsRequirement() );
	}
}
std::map<Attribute, std::string> legacy = {
		{Attribute(PASSIVE, ENERGY), "energy capacity"},
		{Attribute(PASSIVE, SHIELDS), "shields"},
		{Attribute(SHIELD_GENERATION, static_cast<AttributeEffect>(-1)), "shield generation"},
		{Attribute(SHIELD_GENERATION, SHIELDS), "shield generation"},
		{Attribute(SHIELD_GENERATION, ENERGY), "shield energy"},
		{Attribute(SHIELD_GENERATION, HEAT), "shield heat"},
		{Attribute(SHIELD_GENERATION, FUEL), "shield fuel"},
		{Attribute(SHIELD_GENERATION, DELAY), "shield delay"},
		{Attribute(SHIELD_GENERATION, DEPLETED_DELAY), "depleted shield delay"},
		{Attribute(PASSIVE, HULL), "hull"},
		{Attribute(HULL_REPAIR, static_cast<AttributeEffect>(-1)), "hull repair rate"},
		{Attribute(HULL_REPAIR, HULL), "hull repair rate"},
		{Attribute(HULL_REPAIR, ENERGY), "hull energy"},
		{Attribute(HULL_REPAIR, HEAT), "hull heat"},
		{Attribute(HULL_REPAIR, FUEL), "hull fuel"},
		{Attribute(HULL_REPAIR, DELAY), "repair delay"},
		{Attribute(HULL_REPAIR, DEPLETED_DELAY), "disabled repair delay"},
		{Attribute(SHIELD_GENERATION, SHIELDS).Multiplier(), "shield generation multiplier"},
		{Attribute(SHIELD_GENERATION, ENERGY).Multiplier(), "shield energy multiplier"},
		{Attribute(SHIELD_GENERATION, HEAT).Multiplier(), "shield heat multiplier"},
		{Attribute(SHIELD_GENERATION, FUEL).Multiplier(), "shield fuel multiplier"},
		{Attribute(HULL_REPAIR, HULL).Multiplier(), "hull repair multiplier"},
		{Attribute(HULL_REPAIR, ENERGY).Multiplier(), "hull energy multiplier"},
		{Attribute(HULL_REPAIR, HEAT).Multiplier(), "hull heat multiplier"},
		{Attribute(HULL_REPAIR, FUEL).Multiplier(), "hull fuel multiplier"},
		{Attribute(PASSIVE, RAMSCOOP), "ramscoop"},
		{Attribute(PASSIVE, FUEL), "fuel capacity"},
		{Attribute(THRUSTING, static_cast<AttributeEffect>(-1)), "thrust"},
		{Attribute(THRUSTING, THRUST), "thrust"},
		{Attribute(THRUSTING, ENERGY), "thrusting energy"},
		{Attribute(THRUSTING, HEAT), "thrusting heat"},
		{Attribute(THRUSTING, SHIELDS), "thrusting shields"},
		{Attribute(THRUSTING, HULL), "thrusting hull"},
		{Attribute(THRUSTING, FUEL), "thrusting fuel"},
		{Attribute(THRUSTING, DISCHARGE), "thrusting discharge"},
		{Attribute(THRUSTING, CORROSION), "thrusting corrosion"},
		{Attribute(THRUSTING, ION), "thrusting ion"},
		{Attribute(THRUSTING, SCRAMBLE), "thrusting scramble"},
		{Attribute(THRUSTING, LEAK), "thrusting leakage"},
		{Attribute(THRUSTING, BURN), "thrusting burn"},
		{Attribute(THRUSTING, SLOWING), "thrusting slowing"},
		{Attribute(THRUSTING, DISRUPTION), "thrusting disruption"},
		{Attribute(TURNING, static_cast<AttributeEffect>(-1)), "turn"},
		{Attribute(TURNING, TURN), "turn"},
		{Attribute(TURNING, ENERGY), "turning energy"},
		{Attribute(TURNING, HEAT), "turning heat"},
		{Attribute(TURNING, SHIELDS), "turning shields"},
		{Attribute(TURNING, HULL), "turning hull"},
		{Attribute(TURNING, FUEL), "turning fuel"},
		{Attribute(TURNING, DISCHARGE), "turning discharge"},
		{Attribute(TURNING, CORROSION), "turning corrosion"},
		{Attribute(TURNING, ION), "turning ion"},
		{Attribute(TURNING, SCRAMBLE), "turning scramble"},
		{Attribute(TURNING, LEAK), "turning leakage"},
		{Attribute(TURNING, BURN), "turning burn"},
		{Attribute(TURNING, SLOWING), "turning slowing"},
		{Attribute(TURNING, DISRUPTION), "turning disruption"},
		{Attribute(REVERSE_THRUSTING, static_cast<AttributeEffect>(-1)), "reverse thrust"},
		{Attribute(REVERSE_THRUSTING, REVERSE_THRUST), "reverse thrust"},
		{Attribute(REVERSE_THRUSTING, ENERGY), "reverse thrusting energy"},
		{Attribute(REVERSE_THRUSTING, HEAT), "reverse thrusting heat"},
		{Attribute(REVERSE_THRUSTING, SHIELDS), "reverse thrusting shields"},
		{Attribute(REVERSE_THRUSTING, HULL), "reverse thrusting hull"},
		{Attribute(REVERSE_THRUSTING, FUEL), "reverse thrusting fuel"},
		{Attribute(REVERSE_THRUSTING, DISCHARGE), "reverse thrusting discharge"},
		{Attribute(REVERSE_THRUSTING, CORROSION), "reverse thrusting corrosion"},
		{Attribute(REVERSE_THRUSTING, ION), "reverse thrusting ion"},
		{Attribute(REVERSE_THRUSTING, SCRAMBLE), "reverse thrusting scramble"},
		{Attribute(REVERSE_THRUSTING, LEAK), "reverse thrusting leakage"},
		{Attribute(REVERSE_THRUSTING, BURN), "reverse thrusting burn"},
		{Attribute(REVERSE_THRUSTING, SLOWING), "reverse thrusting slowing"},
		{Attribute(REVERSE_THRUSTING, DISRUPTION), "reverse thrusting disruption"},
		{Attribute(AFTERBURNING, static_cast<AttributeEffect>(-1)), "afterburner thrust"},
		{Attribute(AFTERBURNING, THRUST), "afterburner thrust"},
		{Attribute(AFTERBURNING, ENERGY), "afterburner energy"},
		{Attribute(AFTERBURNING, HEAT), "afterburner heat"},
		{Attribute(AFTERBURNING, SHIELDS), "afterburner shields"},
		{Attribute(AFTERBURNING, HULL), "afterburner hull"},
		{Attribute(AFTERBURNING, FUEL), "afterburner fuel"},
		{Attribute(AFTERBURNING, DISCHARGE), "afterburner discharge"},
		{Attribute(AFTERBURNING, CORROSION), "afterburner corrosion"},
		{Attribute(AFTERBURNING, ION), "afterburner ion"},
		{Attribute(AFTERBURNING, SCRAMBLE), "afterburner scramble"},
		{Attribute(AFTERBURNING, LEAK), "afterburner leakage"},
		{Attribute(AFTERBURNING, BURN), "afterburner burn"},
		{Attribute(AFTERBURNING, SLOWING), "afterburner slowing"},
		{Attribute(AFTERBURNING, DISRUPTION), "afterburner disruption"},
		{Attribute(COOL, static_cast<AttributeEffect>(-1)), "cooling"},
		{Attribute(COOL, COOLING), "cooling"},
		{Attribute(ACTIVE_COOL, static_cast<AttributeEffect>(-1)), "active cooling"},
		{Attribute(ACTIVE_COOL, ACTIVE_COOLING), "active cooling"},
		{Attribute(ACTIVE_COOL, ENERGY), "cooling energy"},
		{Attribute(PASSIVE, HEAT), "heat capacity"},
		{Attribute(CLOAKING, static_cast<AttributeEffect>(-1)), "cloak"},
		{Attribute(CLOAKING, CLOAK), "cloak"},
		{Attribute(CLOAKING, ENERGY), "cloaking energy"},
		{Attribute(CLOAKING, FUEL), "cloaking fuel"},
		{Attribute(CLOAKING, HEAT), "cloaking heat"},
		{Attribute(RESISTANCE, DISRUPTION), "disruption resistance"},
		{Attribute(RESISTANCE, DISRUPTION, ENERGY), "disruption resistance energy"},
		{Attribute(RESISTANCE, DISRUPTION, HEAT), "disruption resistance heat"},
		{Attribute(RESISTANCE, DISRUPTION, FUEL), "disruption resistance fuel"},
		{Attribute(RESISTANCE, ION), "ion resistance"},
		{Attribute(RESISTANCE, ION, ENERGY), "ion resistance energy"},
		{Attribute(RESISTANCE, ION, HEAT), "ion resistance heat"},
		{Attribute(RESISTANCE, ION, FUEL), "ion resistance fuel"},
		{Attribute(RESISTANCE, SCRAMBLE), "scramble resistance"},
		{Attribute(RESISTANCE, SCRAMBLE, ENERGY), "scramble resistance energy"},
		{Attribute(RESISTANCE, SCRAMBLE, HEAT), "scramble resistance heat"},
		{Attribute(RESISTANCE, SCRAMBLE, FUEL), "scramble resistance fuel"},
		{Attribute(RESISTANCE, SLOWING), "slowing resistance"},
		{Attribute(RESISTANCE, SLOWING, ENERGY), "slowing resistance energy"},
		{Attribute(RESISTANCE, SLOWING, HEAT), "slowing resistance heat"},
		{Attribute(RESISTANCE, SLOWING, FUEL), "slowing resistance fuel"},
		{Attribute(RESISTANCE, DISCHARGE), "discharge resistance"},
		{Attribute(RESISTANCE, DISCHARGE, ENERGY), "discharge resistance energy"},
		{Attribute(RESISTANCE, DISCHARGE, HEAT), "discharge resistance heat"},
		{Attribute(RESISTANCE, DISCHARGE, FUEL), "discharge resistance fuel"},
		{Attribute(RESISTANCE, CORROSION), "corrosion resistance"},
		{Attribute(RESISTANCE, CORROSION, ENERGY), "corrosion resistance energy"},
		{Attribute(RESISTANCE, CORROSION, HEAT), "corrosion resistance heat"},
		{Attribute(RESISTANCE, CORROSION, FUEL), "corrosion resistance fuel"},
		{Attribute(RESISTANCE, LEAK), "leak resistance"},
		{Attribute(RESISTANCE, LEAK, ENERGY), "leak resistance energy"},
		{Attribute(RESISTANCE, LEAK, HEAT), "leak resistance heat"},
		{Attribute(RESISTANCE, LEAK, FUEL), "leak resistance fuel"},
		{Attribute(RESISTANCE, BURN), "burn resistance"},
		{Attribute(RESISTANCE, BURN, ENERGY), "burn resistance energy"},
		{Attribute(RESISTANCE, BURN, HEAT), "burn resistance heat"},
		{Attribute(RESISTANCE, BURN, FUEL), "burn resistance fuel"},
		{Attribute(RESISTANCE, PIERCING), "piercing resistance"},
		{Attribute(PROTECTION, DISRUPTION), "disruption protection"},
		{Attribute(PROTECTION, ENERGY), "energy protection"},
		{Attribute(PROTECTION, FORCE), "force protection"},
		{Attribute(PROTECTION, FUEL), "fuel protection"},
		{Attribute(PROTECTION, HEAT), "heat protection"},
		{Attribute(PROTECTION, HULL), "hull protection"},
		{Attribute(PROTECTION, ION), "ion protection"},
		{Attribute(PROTECTION, SCRAMBLE), "scramble protection"},
		{Attribute(PROTECTION, PIERCING), "piercing protection"},
		{Attribute(PROTECTION, SHIELDS), "shield protection"},
		{Attribute(PROTECTION, SLOWING), "slowing protection"},
		{Attribute(PROTECTION, DISCHARGE), "discharge protection"},
		{Attribute(PROTECTION, CORROSION), "corrosion protection"},
		{Attribute(PROTECTION, LEAK), "leak protection"},
		{Attribute(PROTECTION, BURN), "burn protection"},
		{Attribute(FIRING, ENERGY), "firing energy"},
		{Attribute(FIRING, FORCE), "firing force"},
		{Attribute(FIRING, FUEL), "firing fuel"},
		{Attribute(FIRING, HEAT), "firing heat"},
		{Attribute(FIRING, HULL), "firing hull"},
		{Attribute(FIRING, SHIELDS), "firing shields"},
		{Attribute(FIRING, ION), "firing ion"},
		{Attribute(FIRING, SCRAMBLE), "firing scramble"},
		{Attribute(FIRING, SLOWING), "firing slowing"},
		{Attribute(FIRING, DISRUPTION), "firing disruption"},
		{Attribute(FIRING, DISCHARGE), "firing discharge"},
		{Attribute(FIRING, CORROSION), "firing corrosion"},
		{Attribute(FIRING, LEAK), "firing leak"},
		{Attribute(FIRING, BURN), "firing burn"},
		{Attribute(FIRING, ENERGY).Relative(), "relative firing energy"},
		{Attribute(FIRING, FUEL).Relative(), "relative firing fuel"},
		{Attribute(FIRING, HEAT).Relative(), "relative firing heat"},
		{Attribute(FIRING, HULL).Relative(), "relative firing hull"},
		{Attribute(FIRING, SHIELDS).Relative(), "relative firing shields"},
		{Attribute(DAMAGE, FORCE), "hit force"},
		{Attribute(DAMAGE, PIERCING), "piercing"},
		{Attribute(DAMAGE, SHIELDS), "shield damage"},
		{Attribute(DAMAGE, HULL), "hull damage"},
		{Attribute(DAMAGE, DISABLED), "disabled damage"},
		{Attribute(DAMAGE, MINABLE), "minable damage"},
		{Attribute(DAMAGE, HEAT), "heat damage"},
		{Attribute(DAMAGE, FUEL), "fuel damage"},
		{Attribute(DAMAGE, ENERGY), "energy damage"},
		{Attribute(DAMAGE, SHIELDS).Relative(), "relative shield damage"},
		{Attribute(DAMAGE, HULL).Relative(), "relative hull damage"},
		{Attribute(DAMAGE, DISABLED).Relative(), "relative disabled damage"},
		{Attribute(DAMAGE, MINABLE).Relative(), "relative minable damage"},
		{Attribute(DAMAGE, HEAT).Relative(), "relative heat damage"},
		{Attribute(DAMAGE, FUEL).Relative(), "relative fuel damage"},
		{Attribute(DAMAGE, ENERGY).Relative(), "relative energy damage"},
		{Attribute(DAMAGE, ION), "ion damage"},
		{Attribute(DAMAGE, SCRAMBLE), "scrambling damage"},
		{Attribute(DAMAGE, DISRUPTION), "disruption damage"},
		{Attribute(DAMAGE, SLOWING), "slowing damage"},
		{Attribute(DAMAGE, DISCHARGE), "discharge damage"},
		{Attribute(DAMAGE, CORROSION), "corrosion damage"},
		{Attribute(DAMAGE, LEAK), "leak damage"},
		{Attribute(DAMAGE, BURN), "burn damage"},
};

TEST_CASE( "Attribute::GetLegacyName", "[Attribute][GetLegacyName]" ) {
	SECTION( "All existing names" ) {
		for(auto &it : legacy)
			CHECK( it.first.GetLegacyName() == it.second );
	}
	SECTION( "-1 and passive" ) {
		for(int i = 0; i < ATTRIBUTE_EFFECT_COUNT; i++)
		{
			AttributeEffect effect = static_cast<AttributeEffect>(i);
			CHECK( Attribute(PASSIVE, effect).GetLegacyName() ==
					Attribute(static_cast<AttributeCategory>(-1), effect).GetLegacyName() );
		}
	}
}

TEST_CASE( "Attribute::Parse", "[Attribute][Parse]" ) {
	SECTION( "All legacy attributes" ) {
		for(auto it : legacy)
			CHECK( *Attribute::Parse(it.second) == it.first );
	}
}

TEST_CASE( "Attribute::IsSupported", "[Attribute][IsSupported]" ) {
	SECTION( "Legacy attributes" ) {
		for(auto &it : legacy)
			CHECK( it.first.IsSupported() );
	}
	SECTION( "Other attributes" ) {
		for(int i = -1; i < ATTRIBUTE_CATEGORY_COUNT; i++)
		{
			for(int j = -1; j < ATTRIBUTE_EFFECT_COUNT * 4; j++)
			{
				for(int k = -1; k < ATTRIBUTE_EFFECT_COUNT; k++)
				{
					Attribute a(static_cast<AttributeCategory>(i), static_cast<AttributeEffect>(j),
							static_cast<AttributeEffect>(k));
					if(!legacy.count(a))
						CHECK( !a.IsSupported() );
				}
			}
		}
	}
}
// #endregion unit tests



} // test namespace
