/* ShipNameDialog.h
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

#ifndef NAME_DIALOG_H_
#define NAME_DIALOG_H_

#include "Dialog.h"

#include "Point.h"

#include <string>



// A special version of Dialog for naming ships.
// Contains a text entry field and an additional button, "Random",
// which populates the text entry field with a randomly selected name
// from the "civilian" phrase.
class ShipNameDialog : public Dialog {
public:
	template <class T>
	ShipNameDialog(T *panel, void (T::*fun)(const std::string &), const std::string &message, std::string initialValue = "")
		: Dialog(panel, fun, message, initialValue) {}

	virtual void Draw() override;


protected:
	virtual bool Click(int x, int y, int clicks) override;


private:
	Point randomPos;
};



#endif
