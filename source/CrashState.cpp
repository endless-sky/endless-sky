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
#include "CrashState.h"
#include "Files.h"
#include <string>

namespace CrashState
{

static State g_prev_state = INVALID;

static State Get()
{
   std::string contents = Files::Read(Files::Config() + "/state.txt");
   if (!contents.empty())
   {
      try
      {
         int ret = std::stoi(contents);
         return static_cast<State>(ret);
      }
      catch (...) {}
   }
   // Garbage in the state file.
   return CrashState::INVALID;
}

void Init() { g_prev_state = Get(); Set(INITIAL); }

void Set(State s)
{
   Files::Write(Files::Config() + "/state.txt", std::to_string(static_cast<int>(s)));
}

State Previous() { return g_prev_state; }



}