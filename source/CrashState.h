/*
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CRASH_STATE_H
#define CRASH_STATE_H


/**
 * Flush the game state to persistent storage, so that we know what we were
 * doing if the game crashes.
 */
namespace CrashState
{
   enum State { INVALID, INITIAL, PREFERENCES, OPENGL, DATA, LOADED, EXITED };

   void Init();
   void Set(State state);
   State Previous();

   inline bool HasCrashed() { return Previous() != INVALID &&
                                     Previous() != LOADED &&
                                     Previous() != EXITED; }
}



#endif