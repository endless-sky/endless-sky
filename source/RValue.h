/* RValue.h
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


#include <cstdio>
#include <stdexcept>
#include <string>


#define DEBUG_RVALUE_CONDITIONS



// This class stores an rvalue and remembers where the rvalue came
// from.  Either this was a literal value, in which the key is empty()
// Or, this was from dereferencing something (ie. Dictionary,
// ConditionsStore) in which case the key is not empty()
//
// The value (V) is first in the template since (nearly?) all RValue
// classes will use a std::string key (K). Having a template key type
// allows storage of scope, wstring, etc. in the future without
// rewriting this class.
template <class V, class K=std::string>
class RValue {
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

	constexpr RValue(): value(), key() {}
	constexpr RValue(const V &value): value(value), key() {}
	constexpr RValue(const V &value, const K &key): value(value), key(key) {}
	~RValue() {}

	// Equality is handled by operator ValueType() which means only the
	// value is compared for equality.
	bool operator == (const RValue<V,K> &other) const = delete;

	// Allow construction and assignment between RValue types to
	// facilitate type conversion.

	template <class V2, class K2>
	RValue(const RValue<V2,K2> &other);

	template <class V2, class K2>
	RValue<V,K> &operator = (const RValue<V2,K2> &other);

	// Update the value from a scope that contains it
	template<class Getter>
	const ValueType &Update(const Getter &getter);

	// Accessors and mutators

	const ValueType &Value() const;
	ValueType &Value();
	const KeyType &Key() const { return key; }
	KeyType &Key() { return key; }

	// Does this RValue come from the same place as the other one?
	// If it was an lvalue, the key must be the same.
	// For literals, this means the value is the same.
	// You can think of this as an operator== that cares about the key.
	bool SameOrigin(const RValue<V,K> &o);

	// Does this originate from dereferencing something?
	bool WasLValue() const
	{
		if(!key.empty())
			printf("WasLValue: %s %f\n",key.c_str(),double(value));
		return !key.empty();
	}

	explicit operator bool() const { return static_cast<bool>(value); }

	// Allow the RValue to be treated as its value in most contexts.
	operator ValueType() const { return value; }

private:
	ValueType value;
	KeyType key;
};



// Allow construction and assignment between RValue types to
// facilitate type conversion.

template <class V, class K>
template <class V2, class K2>
RValue<V,K>::RValue(const RValue<V2,K2> &other):
	value(static_cast<ValueType>(other.Value())),
	key(static_cast<KeyType>(other.Key()))
{
}

template <class V, class K>
template <class V2, class K2>
RValue<V,K> &RValue<V,K>::operator = (const RValue<V2,K2> &other)
{
	value = static_cast<ValueType>(other.Value());
	key = static_cast<KeyType>(other.Key());
	return *this;
}

// Update the value from a scope that contains it
template <class V, class K>
template <class Getter>
const V &RValue<V,K>::Update(const Getter &getter)
{
	// If this was a literal, do nothing.
	if(!WasLValue())
		return value;
	auto got = getter.HasGet(key);
	// Assumes: got.first = true iff getter has key
	// got.second = value iff got.first
	if(got.first)
	{
		value = static_cast<ValueType>(got.second);
		printf("RValue %s updated to %f\n",key.c_str(),double(value));
	}
#ifdef DEBUG_RVALUE_CONDITIONS
	// If the value hasn't been initialized, use the default value
	else if(value == BadValue)
		value = ValueType();
#endif
	return value;
}

template <class V, class K>
const V &RValue<V,K>::Value() const
{
#ifdef DEBUG_RVALUE_CONDITIONS
	if(!key.empty() && value == BadValue)
		throw std::runtime_error("Found uninitialized value with key \""+key+"\"");
#endif
	return value;
}

template <class V, class K>
V &RValue<V,K>::Value()
{
#ifdef DEBUG_RVALUE_CONDITIONS
	if(!key.empty() && value == BadValue)
		throw std::runtime_error("Found uninitialized value with key \""+key+"\"");
#endif
	return value;
}

// Does this RValue come from the same place as the other one?
// If it was an lvalue, the key must be the same.
// For literals, this means the value is the same.
// You can think of this as an operator== that cares about the key.
template <class V, class K>
bool RValue<V,K>::SameOrigin(const RValue<V,K> &o)
{
	if(WasLValue())
		return key == o.Key();
	else
		return !o.WasLValue() && value == o.Value();
}


#endif
