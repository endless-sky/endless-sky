/* ConditionContext.h
Copyright (c) 2014-2025 by Michael Zahniser and others

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

class Ship;

/// Make available information specific to a single ConditionSet evaluation
/// Intended to be read-only and constant
class ConditionContext {
public:
	ConditionContext() = default;

	/// The ship whom initiated the currently evaluated condition.
	/// May be a nullptr if not applicable
    virtual const Ship * getHailingShip() const;
};

const ConditionContext DEFAULT_CONDITION_CONTEXT = ConditionContext();

/// Information specific for when a ship is hailing the player
class ConditionContextHailing: public ConditionContext {
public:
	ConditionContextHailing(const Ship &hailingShip);

	virtual const Ship * getHailingShip() const override;

private:
	const Ship &hailingShip;
};
