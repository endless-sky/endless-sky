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

#ifndef RVALUE_H_
#define RVALUE_H_


#include <cmath>
#include <limits>
#include <string>
#include <type_traits>



// This class stores either:
//   1. A condition's value and key
//   2. A literal value (key is empty)
//
// The ValueType (V in the template) is first in the template since
// (nearly?)  all Condition classes will use a std::string key (K). It
// should be an arithmetic type, like double, int64_t, int, or
// unsigned. A bool should work too, but that's untested.
//
// The KeyType (K in the template) is a template parameter so we can
// allow storage of scope, wstring, etc. in the future without
// rewriting this class.
//
// Condition assumes its KeyType has an empty() function, and that the
// default constructor produces an empty() KeyType. Also, the Getter
// must have a HasGet method that receives the KeyType. An std::string
// satisfies all of these requirements, and it is the default.

template < class V, class K = std::string >
class Condition {
public:
	typedef V ValueType;
	typedef K KeyType;

	static_assert(std::is_arithmetic<ValueType>::value, "Condition value type must be arithmetic.");
	static_assert(std::is_class<KeyType>::value, "Condition key type must be a class.");
	static_assert(&KeyType::empty, "Condition key must have an empty function.");

	constexpr Condition() : value(), key() {}
	explicit constexpr Condition(const V &value) : value(value), key() {}
	constexpr Condition(const V &value, const K &key) : value(value), key(key) {}
	~Condition() {}

	template <class V2, class K2>
	Condition(const Condition<V2, K2> &other);

	template <class V2, class K2>
	Condition<V, K> &operator = (const Condition<V2, K2> &other);

	template <class T, typename std::enable_if<std::is_arithmetic<T>::value, T>::type* Check = nullptr>
	Condition<V, K> &operator = (const T &t) { value = static_cast<ValueType>(t); return *this; }

	// Update the value from a scope that contains it
	template<class Getter>
	const ValueType &UpdateConditions(const Getter &getter);

	// Update the value from a scope that contains it, but use
	// ValueType() if Validator(weight) is false.
	template<class Getter, class Validator>
	const ValueType &UpdateConditions(const Getter &getter, Validator validator);

	// Accessors and mutators

	const ValueType &Value() const { return value; }
	ValueType &Value() { return value; }
	const KeyType &Key() const { return key; }
	KeyType &Key() { return key; }

	// Does this Condition come from the same place as the other one?
	// If it was a condition, the key must be the same (value doesn't matter)
	// If it was a literal (no key) then the value must be the same.
	// If one is literal and the other is conditional, the result is false.
	bool SameOrigin(const Condition<V, K> &o);

	// Does this originate from a condition?
	bool HasConditions() const { return !key.empty(); }

	// Does this originate from a literal value (ie. 5.071)?
	bool IsLiteral() const { return key.empty(); }

	// Floating-point values are false if they're within half the
	// type's precision of 0 while any other types are passed
	// through static_cast<bool>
	explicit operator bool() const;

	// Allow the Condition to be treated as its value in most contexts.
	operator ValueType() const { return value; }



private:
	ValueType value;
	KeyType key;
};


// Allow construction and assignment between Condition types to
// facilitate type conversion.
template <class V, class K>
template <class V2, class K2>
Condition<V, K>::Condition(const Condition<V2, K2> &other):
	value(static_cast<ValueType>(other.Value())),
	key(static_cast<KeyType>(other.Key()))
{
}


// Allow construction and assignment between Condition types to
// facilitate type conversion.
template <class V, class K>
template <class V2, class K2>
Condition<V, K> &Condition<V, K>::operator = (const Condition<V2, K2> &other)
{
	value = static_cast<ValueType>(other.Value());
	key = static_cast<KeyType>(other.Key());
	return *this;
}


// Update the value from a scope that contains it
template <class V, class K>
template <class Getter>
const V &Condition<V, K>::UpdateConditions(const Getter &getter)
{
	// If this was a literal, there is nothing to update.
	if(HasConditions())
	{
		auto got = getter.HasGet(key);
		// Assumes: got.first = true iff getter has key
		// got.second = value iff got.first
		if(got.first)
			value = static_cast<ValueType>(got.second);
	}
	return value;
}


// Update the value from a scope that contains it, if the new value passes a validator.
template <class V, class K>
template <class Getter, class Validator>
const V &Condition<V, K>::UpdateConditions(const Getter &getter, Validator validator)
{
	// If this was a literal, there is nothing to update.
	if(HasConditions())
	{
		auto got = getter.HasGet(key);
		// Assumes: got.first = true iff getter has key
		// got.second = value iff got.first
		if(got.first && validator(got.second))
		{
			value = static_cast<ValueType>(got.second);
			return value;
		}
	}
	if(!validator(value))
		value = ValueType();
	return value;
}


template <class V, class K>
bool Condition<V, K>::SameOrigin(const Condition<V, K> &o)
{
	if(HasConditions())
		return key == o.Key();
	else if(o.HasConditions())
		return false;
	else
		return value == o.Value();
}


template < class T, typename std::enable_if<std::is_floating_point<T>::value, T>::type* Check = nullptr >
bool NotNearZero(T number)
{
	// Use about half the precision of the type when comparing it to zero.
	static const T epsilon = sqrtl(std::numeric_limits<T>::epsilon()*2);
	// Inf and -Inf are NotNearZero but NaN isn't. This is because
	// it is not a number, so it can't be near a number. The
	// consequence is that Condition(NaN) is false in a bool context.
	return number > epsilon || number < -epsilon;
}


template < class T, typename std::enable_if<!std::is_floating_point<T>::value, T>::type* Check = nullptr >
bool NotNearZero(T number)
{
	return static_cast<bool>(number);
}


template <class V, class K>
Condition<V, K>::operator bool() const {
	return NotNearZero(value);
}

#endif
