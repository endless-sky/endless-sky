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

State g_prev_state = INVALID;
State g_cur_state = INVALID;
bool g_is_test = false;

void Init(bool test)
{
   g_is_test = test;
   g_prev_state = CrashState::INVALID;
   std::string contents = Files::Read(Files::Config() / "crash_state.txt");
   if (!contents.empty())
   {
      try
      {
         int ret = std::stoi(contents);
         g_prev_state = static_cast<State>(ret);
      }
      catch (...) {}
   }

   Set(INITIAL);
}

void Set(State s)
{
   g_cur_state = s;
   Files::Write(Files::Config() / "crash_state.txt", std::to_string(static_cast<int>(s)));
}

State Get()
{
   return g_cur_state;
}

State Previous() { return g_prev_state; }

bool HasCrashed()
{
   if(g_is_test)
      return false; // don't do crash logic for unit tests
   return Previous() != INVALID &&
          Previous() != LOADED &&
          Previous() != EXITED;
}

}