/* Trigger.h
Copyright (c) 2024 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef TRIGGER_H_
#define TRIGGER_H_

#include "TriggerType.h"

#include <algorithm>
#include <functional>
#include <optional>



struct Plugin;

// A class storing various callback functions.
template <TriggerType type, class ReturnType, class ...ParameterTypes>
class Trigger {
public:
	using CallbackType = std::function<void(ParameterTypes...)>;
	using CallReturnType = std::conditional_t<std::is_same_v<void, ReturnType>, bool, std::optional<ReturnType>>;
	using ReplacementCallbackType = std::function<CallReturnType(ParameterTypes...)>;

	// Add and remove callbacks.
	static void Register(const Plugin *plugin, CallbackType function);
	static void Register(const Plugin *plugin, ReplacementCallbackType function);
	static void Unregister(const CallbackType function);
	static void Unregister(const ReplacementCallbackType function);
	static void UnregisterAll(const Plugin *plugin);

	// Execute callbacks.
	// Regular callbacks are always executed, and in the order they were registered in.
	// Replacement callbacks are executed in reverse order, until the first one that returns a value.
	static CallReturnType Call(ParameterTypes... params);


protected:
	static std::vector<std::pair<const Plugin *, CallbackType>> callbacks;
	static std::vector<std::pair<const Plugin *, ReplacementCallbackType>> replacementCallbacks;
};



// We have to define the variable here because it's a static field of a class template
template<TriggerType type, class ReturnType, class ...ParameterTypes>
std::vector<std::pair<const Plugin *, typename Trigger<type, ReturnType, ParameterTypes...>::CallbackType>>
Trigger<type, ReturnType, ParameterTypes...>::callbacks = {};

template<TriggerType type, class ReturnType, class ...ParameterTypes>
std::vector<std::pair<const Plugin *, typename Trigger<type, ReturnType, ParameterTypes...>::ReplacementCallbackType>>
Trigger<type, ReturnType, ParameterTypes...>::replacementCallbacks = {};



// Add and remove callbacks.
template<TriggerType type, class ReturnType, class ...ParameterTypes>
void Trigger<type, ReturnType, ParameterTypes...>::Register(const Plugin *plugin, const CallbackType function)
{
	callbacks.emplace_back(plugin, std::move(function));
}



template<TriggerType type, class ReturnType, class ...ParameterTypes>
void Trigger<type, ReturnType, ParameterTypes...>::Register(const Plugin *plugin, const ReplacementCallbackType func)
{
	replacementCallbacks.emplace_back(plugin, std::move(func));
}



template <TriggerType type, class ReturnType, class ...ParameterTypes>
void Trigger<type, ReturnType, ParameterTypes...>::Unregister(const CallbackType function)
{
	std::remove_if(callbacks.begin(), callbacks.end(), [=](const auto &pair){pair.second == function;});
}



template <TriggerType type, class ReturnType, class ...ParameterTypes>
void Trigger<type, ReturnType, ParameterTypes...>::Unregister(const ReplacementCallbackType function)
{
	std::remove_if(replacementCallbacks.begin(), replacementCallbacks.end(),
			[=](const auto &pair){pair.second == function;});
}



template <TriggerType type, class ReturnType, class ...ParameterTypes>
void Trigger<type, ReturnType, ParameterTypes...>::UnregisterAll(const Plugin *plugin)
{
	std::remove_if(callbacks.begin(), callbacks.end(), [=](const auto &pair){pair.first == plugin;});
	std::remove_if(replacementCallbacks.begin(), replacementCallbacks.end(), [=](const auto &pair){pair.first == plugin;});
}



// Execute callbacks.
// Regular callbacks are always executed, and in the order they were registered in.
// Replacement callbacks are executed in reverse order, until the first one that returns a value.
template <TriggerType type, class ReturnType, class ...ParameterTypes>
Trigger<type, ReturnType, ParameterTypes...>::CallReturnType
Trigger<type, ReturnType, ParameterTypes...>::Call(ParameterTypes... params)
{
	for(const auto &pair : callbacks)
		pair.second(params...);
	for(auto it = replacementCallbacks.crbegin(); it != replacementCallbacks.crend(); ++it)
		if constexpr(std::is_same_v<CallReturnType, bool>)
		{
			if(it->second(params...))
				return true;
		}
		else
		{
			const auto value = it->second(params...);
			if(value.has_value())
				return value;
		}
	return {};
}

#endif
