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
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>



// This class stores an rvalue and remembers where the rvalue came
// from.  Either this was a literal value, in which the key is empty()
// Or, this was from dereferencing something (ie. Dictionary,
// ConditionsStore) in which case the key is not empty()
//
// The value (V) is first in the template since (nearly?) all Condition
// classes will use a std::string key (K). Having a template key type
// allows storage of scope, wstring, etc. in the future without
// rewriting this class.
template < class V, class K = std::string >
class Condition {
public:
	typedef V ValueType;
	typedef K KeyType;

	// The BadValue represents a key whose value was unavailable
	// when it was loaded. This is never set by default. The
	// caller has to explicitly request it.

#ifdef DEBUG_RVALUE_CONDITIONS
	static constexpr ValueType BadValue = ValueType(0xDeadBeef);
#else
	static constexpr ValueType BadValue = ValueType();
#endif

	constexpr Condition();
	explicit constexpr Condition(const V &value);
	constexpr Condition(const V &value, const K &key);
	~Condition();

	template <class V2, class K2>
	Condition(const Condition<V2,K2> &other);

	template <class V2, class K2>
	Condition<V,K> &operator = (const Condition<V2,K2> &other);

	// Update the value from a scope that contains it
	template<class Getter>
	const ValueType &UpdateConditions(const Getter &getter);

	// Update the value from a scope that contains it, but use
	// ValueType() if Validator(weight) is false.
	template<class Getter, class Validator>
	const ValueType &UpdateConditions(const Getter &getter, Validator validator);

	// Accessors and mutators

	const ValueType &Value() const;
	ValueType &Value();
	const KeyType &Key() const { return key; }
	KeyType &Key() { return key; }

	// Does this Condition come from the same place as the other one?
	// If it was an lvalue, the key must be the same.
	// For literals, this means the value is the same.
	// You can think of this as an operator== that cares about the key.
	bool SameOrigin(const Condition<V,K> &o);

	// Does this originate from a condition?
	bool HasConditions() const { return !key.empty(); }

	// Does this originate from a literal value (ie. 5.071)?
	bool IsLiteral() const { return key.empty(); }

	explicit operator bool() const;

	// Allow the Condition to be treated as its value in most contexts.
	operator ValueType() const { return value; }

private:
	ValueType value;
	KeyType key;
};



template <class V, class K>
constexpr Condition<V,K>::Condition():
	value(),
	key()
{
}



template <class V, class K>
constexpr Condition<V,K>::Condition(const V &value):
	value(value),
	key()
{
}



template <class V, class K>
constexpr Condition<V,K>::Condition(const V &value, const K &key):
	value(value),
	key(key)
{
}



template <class V, class K>
Condition<V,K>::~Condition()
{
}



// Allow construction and assignment between Condition types to
// facilitate type conversion.
template <class V, class K>
template <class V2, class K2>
Condition<V,K>::Condition(const Condition<V2,K2> &other):
	value(static_cast<ValueType>(other.Value())),
	key(static_cast<KeyType>(other.Key()))
{
}



// Allow construction and assignment between Condition types to
// facilitate type conversion.
template <class V, class K>
template <class V2, class K2>
Condition<V,K> &Condition<V,K>::operator = (const Condition<V2,K2> &other)
{
	value = static_cast<ValueType>(other.Value());
	key = static_cast<KeyType>(other.Key());
	return *this;
}



// Update the value from a scope that contains it
template <class V, class K>
template <class Getter>
const V &Condition<V,K>::UpdateConditions(const Getter &getter)
{
	// If this was a literal, do nothing.
	if(IsLiteral())
		return value;
	auto got = getter.HasGet(key);
	// Assumes: got.first = true iff getter has key
	// got.second = value iff got.first
	if(got.first)
		value = static_cast<ValueType>(got.second);
#ifdef DEBUG_RVALUE_CONDITIONS
	// If the value hasn't been initialized, use the default value
	else if(value == BadValue)
		value = ValueType();
#endif
	return value;
}



// Update the value from a scope that contains it, if the new value passes a validator.
template <class V, class K>
template <class Getter, class Validator>
const V &Condition<V,K>::UpdateConditions(const Getter &getter, Validator validator)
{
	if(HasConditions())
	{
		auto got = getter.HasGet(key);
		// Assumes: got.first = true iff getter has key
		// got.second = value iff got.first
		if(got.first and validator(got.second))
			value = static_cast<ValueType>(got.second);
#ifdef DEBUG_RVALUE_CONDITIONS
		// If the value hasn't been initialized, use the default value
		else if(value == BadValue)
			value = ValueType();
#endif
	}
	return value;
}



template <class V, class K>
const V &Condition<V,K>::Value() const
{
#ifdef DEBUG_RVALUE_CONDITIONS
	if(!key.empty() && value == BadValue)
		throw std::runtime_error("Found uninitialized value with key \"" + key + "\"");
#endif
	return value;
}



template <class V, class K>
V &Condition<V,K>::Value()
{
#ifdef DEBUG_RVALUE_CONDITIONS
	if(!key.empty() && value == BadValue)
		throw std::runtime_error("Found uninitialized value with key \"" + key + "\"");
#endif
	return value;
}



// Does this Condition come from the same place as the other one?
// If it was an lvalue, the key must be the same.
// For literals, this means the value is the same.
// You can think of this as an operator== that cares about the key.
template <class V, class K>
bool Condition<V,K>::SameOrigin(const Condition<V,K> &o)
{
	if(HasConditions())
		return key == o.Key();
	else
		return value == o.Value();
}

template < class T, typename std::enable_if<std::is_floating_point<T>::value, T>::type* Check = nullptr >
bool NotNearZero(T whut)
{
	// Use about half the precision of the type when comparing it to zero.
	static const T epsilon = sqrtl(std::numeric_limits<T>::epsilon*2);
	// Inf and -Inf are NotNearZero but NaN isn't. This is because
	// it is not a number, so it can't be near a number. The
	// consequence is that Condition(NaN) is false in a bool context.
	return whut > epsilon || whut < -epsilon;
}

template < class T, typename std::enable_if<!std::is_floating_point<T>::value, T>::type* Check = nullptr >
bool NotNearZero(T whut)
{
	return static_cast<bool>(whut);
}

template <class V, class K>
Condition<V,K>::operator bool() const {
	return NotNearZero(value);
}

#endif
