/* ActionResult.h
Copyright (c) 2026 by MereDecade

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

/// <summary>
/// Record of the state of an action after use 
/// (such as whether it was triggered and whether it will block other actions.)
/// </summary>
class ActionResult {
public:
	static const int NONE = 0;
	static const int TRIGGERED = 1;
	static const int BLOCKING = 2;
	static const int BLOCKED = 4;
};
