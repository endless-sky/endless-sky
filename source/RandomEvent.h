/* RandomEvent.h
Copyright (c) 2021 by Michael Zahniser

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

#include "ConditionSet.h"
#include "ConditionsStore.h"
#include "DataNode.h"



// A class representing an event that triggers randomly
// in a given internal, for example fleets or hazards.
template <typename T>
class RandomEvent {
public:
	constexpr RandomEvent(const T *event, int period, const DataNode &node) noexcept;

	constexpr const T *Get() const noexcept;
	constexpr int Period() const noexcept;
	bool CanTrigger(const ConditionsStore &tester) const;
	void AddConditions(const DataNode &node);

private:
	const T *event;
	int period;
	ConditionSet conditions;
};



template <typename T>
constexpr RandomEvent<T>::RandomEvent(const T *event, int period, const DataNode &node) noexcept
	: event(event), period(period > 0 ? period : 200)
{
	for(auto &child : node)
		if(child.Size() == 2 && child.Token(0) == "to" && child.Token(1) == "spawn")
			conditions.Load(child);
		// TODO: else with an error-message?
}

template <typename T>
constexpr const T *RandomEvent<T>::Get() const noexcept
{
	return event;
}

template <typename T>
constexpr int RandomEvent<T>::Period() const noexcept
{
	return period;
}

template <typename T>
bool RandomEvent<T>::CanTrigger(const ConditionsStore &tester) const
{
	return conditions.IsEmpty() || conditions.Test(tester);
}
