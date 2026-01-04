/* ShipNameDialogPanel.h
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

#pragma once

#include "DialogPanel.h"

#include <string>



// A special version of Dialog for naming ships.
// Contains a text entry field and an additional button, "Random",
// which populates the text entry field with a randomly selected name
// from the "civilian" phrase.
class ShipNameDialogPanel : public DialogPanel {
public:
	template<class T>
	ShipNameDialogPanel(T *panel, DialogPanel::FunctionButton buttonOne, const std::string &message,
		std::string initialValue = "");


private:
	bool RandomName(const std::string &);
};



template<class T>
ShipNameDialogPanel::ShipNameDialogPanel(T *panel, DialogPanel::FunctionButton buttonOne, const std::string &message,
	std::string initialValue)
	: DialogPanel(panel, message, initialValue, buttonOne,
		DialogPanel::FunctionButton(this, "Random", 'r', &ShipNameDialogPanel::RandomName),
		[](const std::string &) { return true; })
{
}
