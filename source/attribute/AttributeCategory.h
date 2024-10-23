/* AttributeCategory.h
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

// Defines groupings for attributes. Many of these are situations when outfit effects are applied to a ship.
// The first couple entries (until CLOAKING) define the same effect as the first couple entries of AttributeEffect,
// and can be used as a shortcut in the data format.
// Categories are also generated programmatically. For any category C and effect E,
// C + ATTRIBUTE_CATEGORY_COUNT * (E + 1)
// denotes a special category that is combined with the effect. This is primarily used for RESISTANCE, where
// categories such as "burn resistance" are created this way.
enum AttributeCategory : int {
	// These categories have matching default effects.
	SHIELD_GENERATION,
	HULL_REPAIR,
	THRUSTING,
	REVERSE_THRUSTING,
	TURNING,
	ACTIVE_COOL,
	RAMSCOOPING,
	CLOAKING,
	// These don't have default effects.
	AFTERBURNING,
	FIRING,
	PROTECTION,
	RESISTANCE,
	DAMAGE,
	PASSIVE,
	ATTRIBUTE_CATEGORY_COUNT
};
