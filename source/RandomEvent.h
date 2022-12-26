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

#ifndef RANDOMEVENT_H_
#define RANDOMEVENT_H_


#include "RValue.h"



template <class T>
struct PeriodTypeHasConditions
{
	static bool HasConditions(const T &t) { return false; }
};

template <class V, class K>
struct PeriodTypeHasConditions<RValue<V, K>>
{
	static bool HasConditions(const RValue<V, K> &t) { return t.WasLValue(); }
};


// A class representing an event that triggers randomly
// in a given internal, for example fleets or hazards.
template <typename T, typename P = int>
class RandomEvent {
public:
	typedef T EventType;
	typedef P PeriodType;

	constexpr RandomEvent(const T *event, const P &period, bool overrideMinimum = false) noexcept;

	constexpr const T *Get() const noexcept;
	constexpr const P &Period() const noexcept;

	// Update the period from a ConditionSet if needed:
	template<typename Getter>
	void UpdateConditions(const Getter &getter);

	bool HasConditions() const { return PeriodTypeHasConditions<P>::HasConditions(period); }

private:
	const T *event;
	P period;
};


template <typename T, typename P>
constexpr RandomEvent<T,P>::RandomEvent(const T *event, const PeriodType &period, bool overrideMinimum) noexcept
	: event(event), period(overrideMinimum ? period : (period > 0 ? period : PeriodType(200)))
{}

template <typename T, typename P>
constexpr const T *RandomEvent<T,P>::Get() const noexcept
{
	return event;
}

template <typename T, typename P>
constexpr const P &RandomEvent<T,P>::Period() const noexcept
{
	return period;
}

// Update the period from a ConditionSet if needed.
template <typename T, typename P>
template <typename Getter>
void RandomEvent<T,P>::UpdateConditions(const Getter &getter)
{
	period.UpdateConditions(getter);
}



#endif
