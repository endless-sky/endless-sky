/* TableArea.cpp
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

#include "TableArea.h"

#include "text/Format.h"
#include "GameData.h"
#include "ScrollBar.h"

#include <cassert>

using namespace std;



TableArea::TableArea()
{
	SetFontSize(14);
}



TableArea::TableArea(const Rectangle &r)
	: TableArea()
{
	TableArea::SetRect(r);
}



// Stub destructor, so that unique_ptrs are destructed in the correct scope.
TableArea::~TableArea()
{
}



void TableArea::AddColumn(int offset, Layout layout, const Color &color)
{
	table.AddColumn(offset, layout);
	colors.push_back(&color);
	Invalidate();
}



void TableArea::AddRow(const vector<string> &row)
{
	assert(row.size() <= colors.size() && ("Unable to add row of size " + to_string(row.size()) + " to table with "
		+ to_string(colors.size()) + " column(s)").c_str());
	rows.push_back(row);
	currentRowIndex = rows.size();
	Invalidate();
}



void TableArea::AddCell(const string &cell)
{
	while(rows.size() <= currentRowIndex)
		rows.push_back({});
	vector<string> &currentRow = rows[currentRowIndex];
	currentRow.push_back(cell);
	if(currentRow.size() == colors.size())
		++currentRowIndex;
	Invalidate();
}



void TableArea::AddCell(double cell)
{
	AddCell(Format::Number(cell));
}



void TableArea::AddCell(int64_t cell)
{
	AddCell(Format::Number(cell));
}



void TableArea::NextRow()
{
	++currentRowIndex;
}



void TableArea::SetFontSize(int size)
{
	table.SetFontSize(size);
	Invalidate();
}



void TableArea::DrawText(const Point &topLeft)
{
	table.DrawAt(topLeft);

	size_t columnCount = colors.size();
	for(const vector<string> &row : rows)
	{
		for(size_t i = 0; i < row.size(); ++i)
			table.Draw(row[i], *colors[i]);
		if(row.size() != columnCount)
			table.Advance();
	}
}



void TableArea::Validate(bool trailingBreak)
{
	if(!contentsIsValid || trailingBreak != scrollHeightIncludesTrailingBreak)
	{
		scroll.SetMaxValue(table.GetRowSize().Y() * rows.size());
		scrollHeightIncludesTrailingBreak = trailingBreak;
		contentsIsValid = true;
	}
}
