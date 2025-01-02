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
// identified by the category, which is randomly generated if one is not
// provided.  When the event first becomes possible (like entering a
// system) then the event should immediately trigger initialCount
// number of times.
template <typename T>
class LimitedEvents: public RandomEvent<T> {
public:
	static const int NO_LIMIT = -1;


	LimitedEvents(const T *event, int period = RandomEvent<T>::DEFAULT_PERIOD, const DataNode &node = DataNode());
	LimitedEvents(const T *event, int period, const std::string &category, int limit = NO_LIMIT,
		int initial = 0, unsigned flags = 0, int nonDisabledLimit = NO_LIMIT);

	// Maximum number of events that can be active at once
	constexpr bool HasLimit() const noexcept;
	constexpr int Limit() const noexcept;
	int &Limit() noexcept;
	void RemoveLimit() noexcept;

	// For events that involve a T that can be disabled (like ships or fleets),
	// this limit excludes events with disabled T
	constexpr bool HasNonDisabledLimit() const noexcept;
	constexpr int NonDisabledLimit() const noexcept;
	int &NonDisabledLimit() noexcept;
	void RemoveNonDisabledLimit() noexcept;

	// The number of events to spawn initially, such as when entering a system.
	constexpr int InitialCount() const noexcept;
	int &InitialCount() noexcept;

	constexpr const std::string &Category() const noexcept;
	std::string &Category() noexcept;

	constexpr unsigned GetFlags(unsigned mask) const noexcept;
	constexpr unsigned GetFlags() const noexcept;
	unsigned &GetFlags() noexcept;

private:
	std::string category;
	int limit = NO_LIMIT;
	int initialCount = 0;
	unsigned flags = 0;
	int nonDisabledLimit = NO_LIMIT;
};



template <typename T>
LimitedEvents<T>::LimitedEvents(const T *event, int period, const DataNode &node)
	: RandomEvent<T>(event, period, node)
{
}

template <typename T>
LimitedEvents<T>::LimitedEvents(const T *event, int period, const std::string &category,
			int limit, int initial, unsigned flags, int nonDisabledLimit)
	: RandomEvent<T>(event, period), category(category), limit(limit),
		initialCount(initial), flags(flags), nonDisabledLimit(nonDisabledLimit)
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
constexpr bool LimitedEvents<T>::HasNonDisabledLimit() const noexcept
{
	return nonDisabledLimit >= 0;
}

template <typename T>
constexpr int LimitedEvents<T>::NonDisabledLimit() const noexcept
{
	return nonDisabledLimit;
}

template <typename T>
int &LimitedEvents<T>::NonDisabledLimit() noexcept
{
	return nonDisabledLimit;
}

template <typename T>
void LimitedEvents<T>::RemoveNonDisabledLimit() noexcept
{
	nonDisabledLimit = NO_LIMIT;
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
constexpr const std::string &LimitedEvents<T>::Category() const noexcept
{
	return category;
}

template <typename T>
std::string &LimitedEvents<T>::Category() noexcept
{
	return category;
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
