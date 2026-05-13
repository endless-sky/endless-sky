/* TableArea.h
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

#pragma once

#include "ScrollArea.h"

#include "Color.h"
#include "text/Table.h"

#include <utility>
#include <vector>

class Point;
class Rectangle;



// Represents a rect on the screen that needs to display text. The text can be
// larger than the display area, in which case the class will allow the text
// to scroll in response to use input.
class TableArea : public ScrollArea
{
public:
	TableArea();
	explicit TableArea(const Rectangle &r);
	virtual ~TableArea() override;

	void SetRect(const Rectangle &r) override;

	void AddRows(const std::vector<std::pair<std::string, int64_t>> &list);
	void AddRow(const std::string &name, int64_t value);
	void AddRow(const std::string &name);
	void SetColor(const Color &c);
	void SetFontSize(int size);
	void SetDrawValues(bool drawValues);


protected:
	virtual void DrawText(const Point &topLeft) override;

	virtual void Validate(bool trailingBreak) override;


private:
	std::vector<std::pair<std::string, int64_t>> rows;
	bool drawValues = true;
	Table table;
	Color color;
};
