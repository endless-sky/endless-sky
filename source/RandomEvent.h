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

#include <memory>



// A class representing an event that triggers randomly
// in a given internal, for example fleets or hazards.
template <typename T>
class RandomEvent {
public:
	constexpr RandomEvent(const T *event, int period) noexcept;
	RandomEvent(const RandomEvent &);
	RandomEvent &operator = (const RandomEvent &r);

	constexpr const T *Get() const noexcept;
	constexpr int Period() const noexcept;
	bool CanTrigger(const ConditionsStore &tester) const;
	void AddConditions(const DataNode &node);

private:
	const T *event;
	int period;
	std::shared_ptr<ConditionSet> conditions;
};



template <typename T>
constexpr RandomEvent<T>::RandomEvent(const T *event, int period) noexcept
	: event(event), period(period > 0 ? period : 200)
{
}

template<typename T>
RandomEvent<T>::RandomEvent(const RandomEvent &r)
	: event(r.event), period(r.period), conditions()
{
	if(r.conditions)
		conditions = std::make_shared<ConditionSet>(*r.conditions);
}

template<typename T>
RandomEvent<T> &RandomEvent<T>::operator = (const RandomEvent &r)
{
	event = r.event;
	period = r.period;
	if(r.conditions)
		conditions = std::make_shared<ConditionSet>(*r.conditions);
	else
		conditions = nullptr;
	return *this;
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
	return !conditions || conditions->Test(tester);
}

template <typename T>
void RandomEvent<T>::AddConditions(const DataNode &node)
{
	if(!conditions)
		conditions = std::make_shared<ConditionSet>(node);
	else
		conditions->Load(node);
}
