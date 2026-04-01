/* Dropdown.h
Copyright (c) 2023 by thewierdnut

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


#include "Color.h"
#include "Edit.h"
#include "Panel.h"
#include "Rectangle.h"

#include <functional>
#include <string>
#include <vector>



// Implements a dropdown control.
class Dropdown : public Edit
{
public:
	Dropdown();
	virtual ~Dropdown() = default;

	void SetOptions(const std::vector<std::string> &options);
	void SetText(const std::string &s);
	void SetSelectedIndex(int idx);
	int GetSelectedIndex() const { return selectedIndex; }

	virtual void Draw() override;

	void ShowDropIcon(bool s) { showDropIcon = s; }

	void SetTypeable(bool t);
	void SetEnabled(bool e);
	bool Enabled() const { return enabled; }


protected:
	void DoDropdown(const Point &pos);

private:
	int IdxFromPoint(int x, int y) const;

	std::vector<std::string> options;
	int selectedIndex = -1;

	bool showDropIcon = false;
	bool enabled = true;

	class DroppedPanel;
};
