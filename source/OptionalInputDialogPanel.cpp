/* OptionalInputDialogPanel.cpp
Copyright (c) 2026 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "OptionalInputDialogPanel.h"

using namespace std;



OptionalInputDialogPanel::OptionalInputDialogPanel(DialogInit &init,
	function<void(std::optional<int>)> intFun, function<void(std::optional<double>)> doubleFun)
	: DialogPanel(init), optionalIntFun(std::move(intFun)), optionalDoubleFun(std::move(doubleFun))
{
	if(optionalIntFun)
	{
		this->intFun = [this](int value) -> void {
			optionalIntFun(value);
		};
	}
	if(optionalDoubleFun)
	{
		this->doubleFun = [this](double value) -> void {
			optionalDoubleFun(value);
		};
	}
	buttonThree = FunctionButton(this, "Unset", 'u', &OptionalInputDialogPanel::Unset);
	numButtons = 3;
	DialogPanel::Resize();
}



bool OptionalInputDialogPanel::Unset(const string &)
{
	if(optionalIntFun)
		optionalIntFun(std::nullopt);
	if(optionalDoubleFun)
		optionalDoubleFun(std::nullopt);
	// True means to close the dialog box.
	return true;
}
