/* OptionalPreference.h
Copyright (c) 2025 by TomGoodIdea

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

#include "DataNode.h"
#include "DataWriter.h"

#include <optional>



// A preference that may have a numerical value, or be in a default no-value state.
template<class T>
class OptionalPreference {
public:
	OptionalPreference(T minValue, T step, int numberOfSteps, std::string (&stringFun)(T));

	void Load(const DataNode &node, T minLimit = std::numeric_limits<T>::lowest(),
		T maxLimit = std::numeric_limits<T>::max());
	void Save(DataWriter &out, const std::string &name) const;

	void Toggle(bool backwards = false);
	const std::optional<T> &Get() const;
	std::string Setting() const;


private:
	std::optional<T> value;
	// The index doesn't have any meaningful information for the value.
	// It's only used for the settings UI that doesn't allow arbitrary numerical input.
	int currentIndex = 0;
	// The constraints controlling changing the value via the settings UI.
	const T minValue{};
	const T step{};
	const int maxIndex = 0;
	// The function used to construct a string from the value.
	std::string (&stringFun)(T);
};



template<class T>
OptionalPreference<T>::OptionalPreference(T minValue, T step, int numberOfSteps, std::string (&stringFun)(T))
	: minValue{minValue}, step{step}, maxIndex{numberOfSteps - 1}, stringFun{stringFun}
{
}



template<class T>
void OptionalPreference<T>::Load(const DataNode &node, T minLimit, T maxLimit)
{
	// The first element is used to check if the value has been set,
	// while the second one contains the value.
	if(node.Size() < 3 || !node.BoolValue(1))
		return;

	value = std::clamp<T>(node.Value(2), minLimit, maxLimit);
	// Choose the index corresponding to the step closest to the actual value.
	currentIndex = (*value - minValue) / step;
}



template<class T>
void OptionalPreference<T>::Save(DataWriter &out, const std::string &name) const
{
	out.Write(name, value.has_value(), value.value_or(0));
}



template<class T>
void OptionalPreference<T>::Toggle(bool backwards)
{
	if(value.has_value())
	{
		if(backwards ? --currentIndex > 0 : ++currentIndex <= maxIndex)
			// Change the value to the next full step.
			*value = minValue + step * currentIndex;
		else
		{
			// Reset to "default".
			currentIndex = 0;
			value.reset();
		}
	}
	else
	{
		if(backwards)
		{
			currentIndex = maxIndex;
			value = minValue + step * currentIndex;
		}
		else
			value = minValue;
	}
}



template<class T>
const std::optional<T> &OptionalPreference<T>::Get() const
{
	return value;
}



template<class T>
std::string OptionalPreference<T>::Setting() const
{
	return value.has_value() ? stringFun(*value) : "default";
}
