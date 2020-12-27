/* Table.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Table.h"

#include "DisplayText.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "Rectangle.h"

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



void Table::AddColumn(int x, const DisplayText::Layout &layout)
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
void Table::SetRowHeight(int height)
{
	rowSize.Y() = height;
}



// Set the width of the highlight area. If the underline has not been set,
// this will also set the width of the underline.
void Table::SetHighlight(int startX, int endX)
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
void Table::SetUnderline(int startX, int endX)
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



// if a DisplayText or a Layout is given, this field drawn using that layout,
// but the previously set color will be used for future fields.
void Table::Draw(const DisplayText &text) const
{
	Draw(text.GetText(), &text.GetLayout(), color);
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



void Table::Draw(const DisplayText &text, const Color &color) const
{
	Draw(text.GetText(), &text.GetLayout(), color);
}



void Table::Draw(double value) const
{
	Draw(Format::Number(value), nullptr, color);
}



void Table::Draw(double value, const DisplayText::Layout &layout) const
{
	Draw(Format::Number(value), &layout, color);
}



void Table::Draw(double value, const Color &color) const
{
	Draw(Format::Number(value), nullptr, color);
}



void Table::Draw(double value, const Color &color, const DisplayText::Layout &layout) const
{
	Draw(Format::Number(value), &layout, color);
}



void Table::DrawOppositeTruncRight(int width, const string &left, const Color &leftColor,
	const string &right, const Color &rightColor, DisplayText::Truncate trunc)
{
	const DisplayText::Layout layoutLeft{-1};
	const int leftWidth = font->Width({left, layoutLeft});
	Draw({left, layoutLeft}, leftColor);
	const DisplayText::Layout layoutRight{width - leftWidth, DisplayText::Align::RIGHT, trunc};
	Draw({right, layoutRight}, rightColor);
}



void Table::DrawOppositeTruncLeft(int width, const string &left, const Color &leftColor,
	const string &right, const Color &rightColor, DisplayText::Truncate trunc)
{
	const DisplayText::Layout layoutRight{width, DisplayText::Align::RIGHT};
	const int rightWidth = font->Width({right, layoutRight});
	const DisplayText::Layout layoutLeft{width - rightWidth, trunc};
	Draw({left, layoutLeft}, leftColor);
	Draw({right, layoutRight}, rightColor);
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
	FillShader::Fill(GetCenterPoint(), GetRowSize(), color);
}



// Shift the draw position down by the given amount. This usually should not
// be called in the middle of a row, or the fields will not line up.
void Table::DrawGap(int y) const
{
	point.Y() += y;
}


	
// Get the point that should be passed to DrawAt() to start the next row at
// the given location.
Point Table::GetPoint()
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



Table::Column::Column(double offset, const DisplayText::Layout &layout)
	: offset(offset), layout(layout)
{
}



void Table::Draw(const string &text, const DisplayText::Layout *special, const Color &color) const
{
	if(font)
	{
		Point pos = point;
		const auto &layout = special ? *special : it != columns.end() ? it->layout : DisplayText::Layout{};
		const double alignAdjust = layout.align == DisplayText::Align::CENTER
			? -.5 : layout.align == DisplayText::Align::RIGHT ? -1. : 0.;
		if(it != columns.end())
		{
			if(layout.width >= 0)
				pos += Point(it->offset + alignAdjust * layout.width, 0.);
			else
			{
				// This column has a virtually infinite width.
				const int width = font->Width(text);
				pos += Point(it->offset + alignAdjust * width, 0.);
			}
		}
		font->Draw({text, layout}, pos, color);
	}
	
	Advance();
}
