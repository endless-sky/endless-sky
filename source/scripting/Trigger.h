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
// Each callback can be registered as a regular callback for extending functionality, or as a replace callback.
// Call() executes all regular callbacks, and the last registered replacement callback.
template <TriggerType type, class CallbackType>
class TriggerBase {
public:
	// Add and remove callbacks.
	static void Register(const Plugin *plugin, const CallbackType function);
	static void Unregister(const CallbackType function);
	static void UnregisterAll(const Plugin *plugin);


protected:
	static std::vector<std::pair<const Plugin *, CallbackType>> callbacks;
};



// We have to define the variable here because it's a static field of a class template
template<TriggerType type, class CallbackType>
std::vector<std::pair<const Plugin *, CallbackType>> TriggerBase<type, CallbackType>::callbacks = {};



// Add and remove callbacks.
template <TriggerType type, class CallbackType>
void TriggerBase<type, CallbackType>::Register(const Plugin *plugin, const CallbackType function)
{
	callbacks.emplace_back(plugin, function);
}



template <TriggerType type, class CallbackType>
void TriggerBase<type, CallbackType>::Unregister(const CallbackType function)
{
	std::remove_if(callbacks.begin(), callbacks.end(), [=](const auto &pair){pair.second == function;});
}



template <TriggerType type, class CallbackType>
void TriggerBase<type, CallbackType>::UnregisterAll(const Plugin *plugin)
{
	std::remove_if(callbacks.begin(), callbacks.end(), [=](const auto &pair){pair.first == plugin;});
}



// Alias our two trigger variants.
template <TriggerType type, class ...ParameterTypes>
class Trigger : public TriggerBase<type, std::function<void(ParameterTypes...)>> {
public:
	// Call every registered callback.
	static inline void Call(ParameterTypes... parameters)
	{
		for(const auto &callback : Trigger::callbacks)
			callback.second(parameters...);
	}
};



template <TriggerType type, class ReturnValue, class ...ParameterTypes>
class ReplacementTrigger : public TriggerBase<type, std::function<ReturnValue(ParameterTypes...)>> {
public:
	// Call the last registered callback.
	static inline std::conditional<std::is_same_v<ReturnValue, void>, bool, std::optional<ReturnValue>>
	Call(ParameterTypes... parameters)
	{
		if(ReplacementTrigger::callbacks.empty())
			return {};
		else
			if constexpr(std::is_same_v<ReturnValue, void>)
			{
				ReplacementTrigger::callbacks.back().second(parameters...);
				return true;
			}
			else
				return ReplacementTrigger::callbacks.back().second(parameters...);
	}
};


#endif