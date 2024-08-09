/* Modifier.h
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

#pragma once

// Defines various alternative effect behaviours. NONE is used for regular effects,
// RELATIVE specifies scaling relative to some target values, MULTIPLIER multiplies
// regular effects, and OVER_TIME affects the target over a period, instead of instantly.
// AttributeEffectType values are offset based on their modifier. Each effect has exactly
// one modifier.
enum class Modifier {
	NONE,
	MULTIPLIER,
	RELATIVE,
	OVER_TIME,
	MODIFIER_COUNT
};
