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

#include "text/FontSet.h"
#include "GameData.h"
#include "ScrollBar.h"

using namespace std;



TableArea::TableArea()
{
	SetFontSize(14);
	SetColor(*GameData::Colors().Get("medium"));
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



void TableArea::SetRect(const Rectangle &r)
{
	ScrollArea::SetRect(r);
	int width = r.Width();
	table.AddColumn(0, {width, Alignment::LEFT});
	table.AddColumn(width, {width, Alignment::RIGHT});
}



void TableArea::AddRows(const vector<pair<string, int64_t>> &list)
{
	rows.insert(rows.end(), list.begin(), list.end());
}



void TableArea::AddRow(const string &name, int64_t value)
{
	rows.push_back({name, value});
	Invalidate();
}



void TableArea::AddRow(const string &name)
{
	rows.push_back({name, 1});
	Invalidate();
}



void TableArea::SetFontSize(int size)
{
	table.SetFontSize(size);
	Invalidate();
}



void TableArea::SetColor(const Color &c)
{
	color = c;
	Invalidate();
}



void TableArea::SetDrawValues(bool drawValues)
{
	this->drawValues = drawValues;
}



void TableArea::DrawText(const Point &topLeft)
{
	table.DrawAt(topLeft);
	for(const auto &[name, value] : rows)
	{
		table.Draw(name, color);
		if(drawValues)
			table.Draw(value);
		else
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
