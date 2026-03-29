/* GameVersion.cpp
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

#include "GameVersion.h"

using namespace std;



string GameVersion::ToString() const
{
	return to_string(numbers[0]) + '.'
		+ to_string(numbers[1]) + '.'
		+ to_string(numbers[2]) + '.'
		+ to_string(numbers[3])
		+ (fullRelease ? "" : "-alpha");
}
