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

#include <vector>

class Point;
class Rectangle;



// A ScrollArea that renders text using the Table class.
class TableArea : public ScrollArea
{
public:
	TableArea();
	explicit TableArea(const Rectangle &r);
	virtual ~TableArea() override;

	void AddColumn(int offset, Layout layout, const Color &color);
	void AddRow(const std::vector<std::string> &row);
	void AddCell(const std::string &cell);
	void AddCell(double cell);
	void AddCell(int64_t cell);
	void NextRow();

	void SetFontSize(int size);

	virtual void Validate(bool trailingBreak) override;


protected:
	virtual void DrawText(const Point &topLeft) override;


private:
	std::vector<std::vector<std::string>> rows;
	std::vector<const Color *> colors;
	size_t currentRowIndex = 0;

	Table table;
};
