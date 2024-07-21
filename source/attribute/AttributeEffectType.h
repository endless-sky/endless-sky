/* AttributeEffectType.h
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

#ifndef ATTRIBUTE_EFFECT_TYPE_H_
#define ATTRIBUTE_EFFECT_TYPE_H_

// The different effects of an attribute. Each one has multiple variants,
// each offset by ATTRIBUTE_EFFECT_COUNT. The variants are:
// base (not offset), multiplier, relative, and relative multiplier.
enum AttributeEffectType : int {
	SHIELDS,
	HULL,
	THRUST,
	REVERSE_THRUST,
	TURN,
	ACTIVE_COOLING,
	RAMSCOOP,
	CLOAK,
	COOLING,
	FORCE,
	ENERGY,
	FUEL,
	HEAT,
	DISCHARGE,
	CORROSION,
	LEAK,
	BURN,
	ION,
	SCRAMBLE,
	SLOWING,
	DISRUPTION,
	DISABLED,
	MINABLE,
	PIERCING,
	ATTRIBUTE_EFFECT_COUNT
};

#endif
