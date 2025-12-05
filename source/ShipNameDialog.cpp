/* ShipNameDialog.cpp
Copyright (c) 2024 by Endless Sky contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ShipNameDialog.h"

#include "GameData.h"
#include "Phrase.h"

using namespace std;



bool ShipNameDialog::RandomName(const string &)
{
	// TODO: This always chooses human names, even for alien ships. Add a method
	// of setting the phrase based off of ship and/or purchase location.
	input = GameData::Phrases().Get("civilian")->Get();
	// False means to keep the dialog box open.
	return false;
}
