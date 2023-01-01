/* Condition.h
Copyright (c) 2022 by an anonymous author.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CONDITION_H_
#define CONDITION_H_


#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>

#include "ConditionsStore.h"



// This class stores either:
//   1. A condition that is in a ConditionsStore
//   2. A literal value (key is empty)
//
// The ValueType (V in the template) is first in the template since
// (nearly?)  all Condition classes will use a std::string key (K). It
// should be an arithmetic type, like double, int64_t, int, or
// unsigned. A bool should work too, but that's untested.

template < class V >
class Condition {
	template <class V2>
	friend class Condition;
public:
	using ValueType = V;
	using KeyType = std::string;
	using StoreType = ConditionsStore;

	static_assert(std::is_arithmetic<ValueType>::value, "Condition value type must be arithmetic.");
	static_assert(std::is_class<KeyType>::value, "Condition key type must be a class.");
	static_assert(&KeyType::empty, "Condition key must have an empty function.");

	Condition() : value(), key() {}
	explicit Condition(const V &value) : value(value), key() {}

	// Initialize the condition with the specified value.
	// Record what store and key to use, but do not query the store for a value.
	Condition(const V &value, std::shared_ptr<ConditionsStore> store, const KeyType &key);

	// Get the initial value from the store. Use V() if the store returned nothing.
	Condition(std::shared_ptr<ConditionsStore> store, const KeyType &key);

	template <class V2>
	Condition(const Condition<V2> &other);

	template <class V2>
	Condition &operator=(const Condition<V2> &other);

	// This commented-out operator may look convenient, but it causes
	// everything to break because any Condition=Condition assignment uses
	// this operator instead of the Condition=Condition implementation,
	// preventing explicit conversion of Conditions. A lot of code would
	// be simpler if this was working.
	//
	// template <class T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
	// Condition &operator=(const T &other);

	// Send the latest value to the ConditionsStore
	const ValueType &SendValue();

	// Read the value from the ConditionsStore
	const ValueType &UpdateConditions();

	// Read the value from the ConditionsStore, pass it through a
	// filter, and use the new result. The filter arguments are:
	// 1. The ConditionsStore's value, as a ConditionsStore::ValueType
	// 2. The current value in the Condition, as a Condition::ValueType
	// The filter should return the new value as a Condition::ValueType
	template <class Filterer>
	const ValueType &UpdateConditions(Filterer validator);

	// Pass the current value through a filter. The filter
	// receives the current value as a ValueType and returns the
	// new value as a ValueType
	template <class Filterer>
	const ValueType &Filter(Filterer filter);

	// Accessors and mutators

	const ValueType &Value() const { return value; }
	ValueType &Value() { return value; }
	const KeyType &Key() const { return key; }
	KeyType &Key() { return key; }

	std::shared_ptr<const ConditionsStore> Store() const { return store; }
	std::shared_ptr<ConditionsStore> Store() { return store; }

	// Does this Condition come from the same place as the other one?
	// If it was a condition, the key must be the same (value doesn't matter)
	// If it was a literal (no key) then the value must be the same.
	// If one is literal and the other is conditional, the result is false.
	bool SameOrigin(const Condition &o);

	// Does this originate from a condition?
	bool HasConditions() const { return store && !key.empty(); }

	// Does this originate from a literal value (ie. 5.071)?
	bool IsLiteral() const { return !store || key.empty(); }

	// Floating-point values are false if they're within half the
	// type's precision of 0 while any other types are passed
	// through static_cast<bool>
	explicit operator bool() const;

	// Allow the Condition to be treated as its value in most contexts.
	operator ValueType() const { return value; }



private:
	std::shared_ptr<ConditionsStore::ConditionEntry> GetEntry();
	std::shared_ptr<ConditionsStore::ConditionEntry> EnsureEntry();

	// The value of the condition, as this object sees it
	ValueType value;

	// The key of the condition; empty if this is literal
	KeyType key;

	// A shared_ptr to a ConditionsStore for this condition; empty if this is literal
	std::shared_ptr<ConditionsStore> store;

	// A weak_ptr to a ConditionEntry with the data. May be empty or expired.
	std::weak_ptr<ConditionsStore::ConditionEntry> entry;
};


template <class V>
Condition<V>::Condition(const V &value, std::shared_ptr<ConditionsStore> store, const KeyType &key) :
	value(value), key(key), store(store)
{
}


template <class V>
Condition<V>::Condition(std::shared_ptr<ConditionsStore> store, const KeyType &key) :
	value(), key(key), store(store)
{
	UpdateConditions();
}


// Allow construction and assignment between Condition types to
// facilitate type conversion.
template <class V>
template <class V2>
Condition<V>::Condition(const Condition<V2> &other):
	value(static_cast<ValueType>(other.Value())),
	key(other.Key()),
	store(const_cast<Condition<V2>&>(other).Store()),
	entry(const_cast<Condition<V2>&>(other).entry)
{
}


// Allow construction and assignment between Condition types to
// facilitate type conversion.
template <class V>
template <class V2>
Condition<V> &Condition<V>::operator=(const Condition<V2> &other)
{
	value = static_cast<ValueType>(other.Value());
	key = other.Key();
	store = const_cast<Condition<V2>&>(other).Store();
	entry = const_cast<Condition<V2>&>(other).entry;
	return *this;
}


// This commented-out operator may look convenient, but it causes
// everything to break because any Condition=Condition assignment uses
// this operator instead of the Condition=Condition implementation,
// preventing explicit conversion of Conditions. A lot of code would
// be simpler if this was working.
//
// template <class V>
// template <class T, typename std::enable_if<std::is_arithmetic<T>::value>::type*>
// Condition<V> &Condition<V>::operator=(const T &other)
// {
// 	value = static_cast<ValueType>(other);
// 	return *this;
// }



// Update the value from a scope that contains it
template <class V>
const V &Condition<V>::SendValue()
{
	// If this was a literal, there is nothing to update.
	if(!HasConditions())
		return value;

	std::shared_ptr<ConditionsStore::ConditionEntry> ce = EnsureEntry();

	if(!ce)
		return value;
	else if(ce->provider)
		*ce = value;
	else
		ce->value = value;
	return value;
}


template <class V>
template <class Filterer>
const V &Condition<V>::Filter(Filterer filter)
{
	return value = filter(value);
}


// Read the value from the ConditionsStore, pass it through a
// filter, and use the new result
template <class V>
template <class Filterer>
const V &Condition<V>::UpdateConditions(Filterer filter)
{
	if(!HasConditions())
		return value;

	std::shared_ptr<ConditionsStore::ConditionEntry> ce = GetEntry();

	if(!ce)
		return value;

	V newValue = ce->provider ? store->Get(key) : ce->value;
	value = filter(newValue, value);

	return value;
}


// Read the value from the ConditionsStore
template <class V>
const V &Condition<V>::UpdateConditions()
{
	// If this was a literal, there is nothing to update.
	if(!HasConditions())
		return value;

	std::shared_ptr<ConditionsStore::ConditionEntry> ce = GetEntry();

	if(ce)
		value = ce->provider ? store->Get(key) : ce->value;

	return value;
}


template <class V>
bool Condition<V>::SameOrigin(const Condition<V> &o)
{
	if(HasConditions())
		return key == o.Key() && store == o.Store();
	else if(o.HasConditions())
		return false;
	else
		return value == o.Value();
}


template < class T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr >
bool NotNearZero(T number)
{
	// Use about half the precision of the type when comparing it to zero.
	static const T epsilon = sqrtl(std::numeric_limits<T>::epsilon()*2);
	// Inf and -Inf are NotNearZero but NaN isn't. This is because
	// it is not a number, so it can't be near a number. The
	// consequence is that Condition(NaN) is false in a bool context.
	return number > epsilon || number < -epsilon;
}


template < class T, typename std::enable_if<!std::is_floating_point<T>::value>::type* = nullptr >
bool NotNearZero(T number)
{
	return static_cast<bool>(number);
}


template <class V>
Condition<V>::operator bool() const
{
	return NotNearZero(value);
}


template <class V>
typename std::shared_ptr<ConditionsStore::ConditionEntry> Condition<V>::GetEntry()
{
	// Note: assumes store and key are available
	try {
		return std::shared_ptr<ConditionsStore::ConditionEntry>(entry);
	}
	catch(const std::bad_weak_ptr &bwp)
	{
		std::shared_ptr<ConditionsStore::ConditionEntry> got = store->GetEntry(key);
		entry = got;
		return got;
	}
}


template <class V>
typename std::shared_ptr<ConditionsStore::ConditionEntry> Condition<V>::EnsureEntry()
{
	// Note: assumes store and key are available
	try {
		return std::shared_ptr<ConditionsStore::ConditionEntry>(entry);
	}
	catch(const std::bad_weak_ptr &bwp)
	{
		std::shared_ptr<ConditionsStore::ConditionEntry> got = store->EnsureEntry(key);
		entry = got;
		return got;
	}
}

#endif
