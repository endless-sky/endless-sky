/* Triggers.h
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

#ifndef TRIGGERS_H_
#define TRIGGERS_H_

#include "Trigger.h"



// Define all used trigger types here. This avoids having to fill out the template every time.
struct Plugin;

// Plugin::Load
using LoadPluginTrigger = Trigger<TriggerType::LOAD_PLUGIN, Plugin *, const std::string &>;
using PluginLoadedTrigger = Trigger<TriggerType::PLUGIN_LOADED, void, Plugin *>;

#endif
