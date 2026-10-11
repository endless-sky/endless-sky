/* Table.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Table.h"

#include "DisplayText.h"
#include "../shader/FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "../Rectangle.h"

#include <algorithm>

using namespace std;



Table::Table()
{
	Clear();
}



// Set the column positions. If no columns are set, the Table will draw a
// list (one column of text, left aligned).
void Table::Clear()
{
	columns.clear();

	font = &FontSet::Get(14);
	rowSize = Point(0., 20.);
	center = Point(0., font->Height() / 2);
	lineSize = Point(0., 1.);
	lineOff = Point(0., font->Height() + 1);

	point = Point();
	it = columns.begin();
	color = Color(1.f, 0.f);
}



void Table::AddColumn(int x, Layout layout)
{
	columns.emplace_back(x, layout);

	// This may invalidate iterators, so:
	it = columns.begin();
}



// Set the font size. Default is 14 pixels.
void Table::SetFontSize(int size)
{
	font = &FontSet::Get(size);
	lineOff.Y() = font->Height() + 1;
	center.Y() = font->Height() / 2;
}



// Set the row height. Default is 20 pixels.
void Table::SetRowHeight(int height) noexcept
{
	rowSize.Y() = height;
}



// Set the width of the highlight area. If the underline has not been set,
// this will also set the width of the underline.
void Table::SetHighlight(int startX, int endX) noexcept
{
	rowSize.X() = endX - startX;
	center.X() = (endX + startX) / 2;

	if(!lineSize.X())
	{
		lineSize.X() = rowSize.X();
		lineOff.X() = center.X();
	}
}



// Set the X range of the underline. If the highlight has not been set, this
// will also set the width of the highlight.
void Table::SetUnderline(int startX, int endX) noexcept
{
	lineSize.X() = endX - startX;
	lineOff.X() = (endX + startX) / 2;

	if(!rowSize.X())
	{
		rowSize.X() = lineSize.X();
		center.X() = lineOff.X();
	}
}



// Begin drawing at the given position. Each time text is drawn, it fills a
// new column until all columns have been filled. Then, the Y position is
// increased based on the row height, and a new row begins.
void Table::DrawAt(const Point &point) const
{
	this->point = point + Point(0., (rowSize.Y() - font->Height()) / 2);
	it = columns.begin();
}



// Set the color for drawing text and underlines.
void Table::SetColor(const Color &color) const
{
	this->color = color;
}



// Advance to the next field without drawing anything.
void Table::Advance(int fields) const
{
	while(fields-- > 0)
	{
		if(columns.empty() || ++it == columns.end())
		{
			it = columns.begin();
			point.Y() += rowSize.Y();
		}
	}
}



// Draw a single text field, and move on to the next one.
void Table::Draw(const char *text) const
{
	Draw(text, nullptr, color);
}



void Table::Draw(const string &text) const
{
	Draw(text, nullptr, color);
}



// If a color is given, this field is drawn using that color, but the
// previously set color will be used for future fields.
void Table::Draw(const char *text, const Color &color) const
{
	Draw(text, nullptr, color);
}



void Table::Draw(const string &text, const Color &color) const
{
	Draw(text, nullptr, color);
}



void Table::Draw(double value) const
{
	Draw(Format::Number(value), nullptr, color);
}



void Table::Draw(double value, const Color &color) const
{
	Draw(Format::Number(value), nullptr, color);
}



void Table::DrawCustom(const DisplayText &text) const
{
	Draw(text.GetText(), &text.GetLayout(), color);
}



void Table::DrawCustom(const DisplayText &text, const Color &color) const
{
	Draw(text.GetText(), &text.GetLayout(), color);
}



void Table::DrawTruncatedPair(const string &left, const Color &leftColor, const string &right, const Color &rightColor,
	Truncate strategy, bool truncateRightColumn) const
{
	// Compute the width of the non-truncated string, and the margin we have for the possibly-large text.
	const auto colWidth = it->layout.width;
	const auto textWidth = font->FormattedWidth({truncateRightColumn ? left : right, {colWidth}});
	constexpr auto PAD = 5;
	const auto remainder = max(colWidth - PAD - textWidth, 0);

	auto lhs = Layout(truncateRightColumn ? colWidth : remainder, Alignment::LEFT, strategy);
	auto rhs = Layout(truncateRightColumn ? remainder : colWidth, Alignment::RIGHT, strategy);
	if(truncateRightColumn)
		lhs.truncate = Truncate::NONE;
	else
		rhs.truncate = Truncate::NONE;
	Draw(left, &lhs, leftColor);
	Draw(right, &rhs, rightColor);
}



// Draw an underline under the text for the current row.
void Table::DrawUnderline() const
{
	DrawUnderline(color);
}



void Table::DrawUnderline(const Color &color) const
{
	FillShader::Fill(point + lineOff - Point(0., 2.), lineSize, color);
}



// Highlight the current row.
void Table::DrawHighlight() const
{
	DrawHighlight(color);
}



void Table::DrawHighlight(const Color &color) const
{
	FillShader::Fill(GetRowBounds(), color);
}



// Shift the draw position down by the given amount. This usually should not
// be called in the middle of a row, or the fields will not line up.
void Table::DrawGap(int y) const
{
	point.Y() += y;
}



// Get the point that should be passed to DrawAt() to start the next row at
// the given location.
Point Table::GetPoint() const
{
	return point - Point(0., (rowSize.Y() - font->Height()) / 2);
}



// Get the center and size of the current row. This can be used to define
// what screen region constitutes a mouse click on this particular row.
Point Table::GetCenterPoint() const
{
	return point + center;
}



Point Table::GetRowSize() const
{
	return rowSize;
}



Rectangle Table::GetRowBounds() const
{
	return Rectangle(GetCenterPoint(), GetRowSize());
}



Table::Column::Column(double offset, Layout layout) noexcept
	: offset(offset), layout(layout)
{
}



void Table::Draw(const string &text, const Layout *special, const Color &color) const
{
	if(font && !columns.empty())
	{
		const auto &layout = special ? *special : it->layout;
		const double alignmentOffset = layout.align == Alignment::RIGHT ? -1.
			: layout.align == Alignment::CENTER ? -0.5 : 0.;
		auto pos = point + Point(it->offset + alignmentOffset * (layout.width >= 0 ? layout.width : font->Width(text)), 0.);
		font->Draw({text, layout}, pos, color);
	}

	Advance();
}
