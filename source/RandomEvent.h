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

#include <type_traits>




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
	void UpdateConditions();

	static int MinimumPeriod() { return minimumPeriod; }
	bool HasConditions() const;

private:
	static constexpr int minimumPeriod = 200;
	const T *event;
	bool overrideMinimum;
	P period;
};

template <class T, typename std::enable_if<std::is_class<T>::value>::type* = nullptr >
bool PeriodTypeHasConditions(const T &t)
{
	return t.HasConditions();
}

template <class T, typename std::enable_if<!std::is_class<T>::value>::type* = nullptr >
bool PeriodTypeHasConditions(const T &t)
{
	return false;
}

template < class T, class U,
	typename std::enable_if<!std::is_class<T>::value>::type* = nullptr,
	typename std::enable_if<std::is_arithmetic<U>::value>::type* = nullptr
>
T InitialPeriod(const T &period, U minimumValue, bool overrideMinimum)
{
	T result(period);
	if(result > 0)
		return result;
	else if(overrideMinimum)
		result = T(0);
	else
		result = T(minimumValue);
	return result;
}

template < class T, class U,
	typename std::enable_if<std::is_class<T>::value>::type* = nullptr,
	typename std::enable_if<std::is_arithmetic<U>::value>::type* = nullptr
>
T InitialPeriod(const T &period, U minimumValue, bool overrideMinimum)
{
	T result(period);
	if(result > 0)
		return result;
	else if(overrideMinimum)
		result.Value() = 0;
	else
		result.Value() = minimumValue;
	return result;
}

template <typename T, typename P>
constexpr RandomEvent<T, P>::RandomEvent(const T *event, const PeriodType &period, bool overrideMinimum) noexcept
	: event(event), overrideMinimum(overrideMinimum),
	period(InitialPeriod(period, minimumPeriod, overrideMinimum))
{}

template <typename T, typename P>
constexpr const T *RandomEvent<T, P>::Get() const noexcept
{
	return event;
}

template <typename T, typename P>
constexpr const P &RandomEvent<T, P>::Period() const noexcept
{
	return period;
}

// Update the period from a ConditionSet if needed.
template <typename T, typename P>
void RandomEvent<T, P>::UpdateConditions()
{
	period.UpdateConditions();
	if(period <= 0)
		period.Value() = overrideMinimum ? 0 : minimumPeriod;
}

template <typename T, typename P>
bool RandomEvent<T, P>::HasConditions() const
{
	return PeriodTypeHasConditions<P>(period);
}


#endif
