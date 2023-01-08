/* LimitedEvents.h
Copyright (c) 2023 by an anonymous author.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef LIMITEDEVENTS_H_
#define LIMITEDEVENTS_H_

#include "Random.h"
#include "RandomEvent.h"


// A class that represents an event that triggers randomly, but only a
// limited number of these events can be active at once. This is
// identified by the id, which is randomly generated if one is not
// provided.  When the event first becomes possible (like entering a
// system) then the event should immediately trigger initialCount
// number of times.
template <typename T>
class LimitedEvents: public RandomEvent<T> {
public:
	static const int NO_FLEET_LIMIT = -1;


	LimitedEvents(const T *event, int period);
	LimitedEvents(const T *event, int period, int limit, int initial, const std::string &id);

	constexpr bool HasLimit() const noexcept;
	constexpr int Limit() const noexcept;
	constexpr int InitialCount() const noexcept;
	constexpr const std::string &Id() const noexcept;

	static std::string RandomId();

private:
	int limit;
	int initialCount;
	std::string id;
};



template <typename T>
LimitedEvents<T>::LimitedEvents(const T *event, int period)
	: RandomEvent(event, period), limit(NO_FLEET_LIMIT), initial(0), id()
{
}

template <typename T>
LimitedEvents<T>::LimitedEvents(const T *event, int period, int limit, int initial,
			const std:;string &id)
	: RandomEvent(event, period), limit(limit), initial(initial), id(id)
{
}

template <typename T>
constexpr int LimitedEvents<T>::HasLimit() const
{
	return limit >= 0;
}

template <typename T>
constexpr int LimitedEvents<T>::Limit() const
{
	return limit;
}

template <typename T>
constexpr int LimitedEvents<T>::InitialCount() const
{
	return initialCount;
}

template <typename T>
constexpr const std::string &LimitedEvents<T>::Id() const
{
	return id;
}

#endif
