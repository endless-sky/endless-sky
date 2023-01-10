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

#include <string>


// A class that represents an event that triggers randomly, but only a
// limited number of these events can be active at once. This is
// identified by the id, which is randomly generated if one is not
// provided.  When the event first becomes possible (like entering a
// system) then the event should immediately trigger initialCount
// number of times.
template <typename T>
class LimitedEvents: public RandomEvent<T> {
public:
	static const int NO_LIMIT = -1;


	LimitedEvents(const T *event, int period = RandomEvent<T>::DEFAULT_PERIOD);
	LimitedEvents(const T *event, int period, const std::string &id, int limit = NO_LIMIT,
		int initial = 0, unsigned flags = 0);

	constexpr bool HasLimit() const noexcept;
	constexpr int Limit() const noexcept;
	int &Limit() noexcept;
	void RemoveLimit() noexcept;
	constexpr int InitialCount() const noexcept;
	int &InitialCount() noexcept;
	constexpr const std::string &Id() const noexcept;
	std::string &Id() noexcept;

	constexpr unsigned GetFlags(unsigned mask) const noexcept;
	constexpr unsigned GetFlags() const noexcept;
	unsigned &GetFlags() noexcept;

private:
	std::string id;
	int limit = NO_LIMIT;
	int initialCount = 0;
	unsigned flags = 0;
};



template <typename T>
LimitedEvents<T>::LimitedEvents(const T *event, int period)
	: RandomEvent<T>(event, period)
{
}

template <typename T>
LimitedEvents<T>::LimitedEvents(const T *event, int period, const std::string &id,
			int limit, int initial, unsigned flags)
	: RandomEvent<T>(event, period), id(id), limit(limit), initialCount(initial), flags(flags)
{
}

template <typename T>
constexpr bool LimitedEvents<T>::HasLimit() const noexcept
{
	return limit >= 0;
}

template <typename T>
constexpr int LimitedEvents<T>::Limit() const noexcept
{
	return limit;
}

template <typename T>
int &LimitedEvents<T>::Limit() noexcept
{
	return limit;
}

template <typename T>
void LimitedEvents<T>::RemoveLimit() noexcept
{
	limit = NO_LIMIT;
}

template <typename T>
constexpr int LimitedEvents<T>::InitialCount() const noexcept
{
	return initialCount;
}

template <typename T>
int &LimitedEvents<T>::InitialCount() noexcept
{
	return initialCount;
}

template <typename T>
constexpr const std::string &LimitedEvents<T>::Id() const noexcept
{
	return id;
}

template <typename T>
std::string &LimitedEvents<T>::Id() noexcept
{
	return id;
}


template <typename T>
constexpr unsigned LimitedEvents<T>::GetFlags(unsigned mask) const noexcept
{
	return flags & mask;
}


template <typename T>
constexpr unsigned LimitedEvents<T>::GetFlags() const noexcept
{
	return flags;
}


template <typename T>
unsigned &LimitedEvents<T>::GetFlags() noexcept
{
	return flags;
}

#endif
