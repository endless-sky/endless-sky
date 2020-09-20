/* Table.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TABLE_H_
#define TABLE_H_

#include "Color.h"
#include "Font.h"
#include "Point.h"

#include <string>
#include <vector>

class Font;
class Rectangle;



// Helper class for drawing text formatted in a table, where each column of the
// table is aligned left, right, or centered. This also handles spacing in
// between table rows, underlines, selection highlights, etc.
class Table {
public:
	Table();
	
	// Set the column positions. If no columns are set, the Table will draw a
	// list (one column of text, left aligned).
	void Clear();
	void AddColumn(int x, const Font::Layout &layout);
	
	// Set the font size. Default is 14 pixels.
	void SetFontSize(int size);
	// Set the row height. Default is 20 pixels.
	void SetRowHeight(int height);
	// Set the width of the highlight area. If the underline has not been set,
	// this will also set the width of the underline.
	void SetHighlight(int startX, int endX);
	// Set the X range of the underline. If the highlight has not been set, this
	// will also set the width of the highlight.
	void SetUnderline(int startX, int endX);
	
	// Begin drawing at the given position. Each time text is drawn, it fills a
	// new column until all columns have been filled. Then, the Y position is
	// increased based on the row height, and a new row begins.
	void DrawAt(const Point &point) const;
	
	// Set the color for drawing text and underlines.
	void SetColor(const Color &color) const;
	
	// Advance to the next field without drawing anything.
	void Advance(int fields = 1) const;
	
	// Draw a single text field, and move on to the next one.
	void Draw(const std::string &text, const Font::Layout *special = nullptr) const;
	// If a color is given, this field is drawn using that color, but the
	// previously set color will be used for future fields.
	void Draw(const std::string &text, const Color &color, const Font::Layout *special = nullptr) const;
	void Draw(double value, const Font::Layout *special = nullptr) const;
	void Draw(double value, const Color &color, const Font::Layout *special = nullptr) const;
	
	// Draw a left-aligned column and a right-aligned,
	// and truncate the left or right column adaptively.
	void DrawOppositeTruncRight(int width, const std::string &left, const Color &leftColor,
		const std::string &right, const Color &rightColor, Font::Truncate trunc);
	void DrawOppositeTruncLeft(int width, const std::string &left, const Color &leftColor,
		const std::string &right, const Color &rightColor, Font::Truncate trunc);
	
	// Draw an underline under the text for the current row.
	void DrawUnderline() const;
	void DrawUnderline(const Color &color) const;
	
	// Highlight the current row.
	void DrawHighlight() const;
	void DrawHighlight(const Color &color) const;
	
	// Shift the draw position down by the given amount. This usually should not
	// be called in the middle of a row, or the fields will not line up.
	void DrawGap(int y) const;
	
	// Get the point that should be passed to DrawAt() to start the next row at
	// the given location.
	Point GetPoint();
	
	// Get the center and size of the current row. This can be used to define
	// what screen region constitutes a mouse click on this particular row.
	Point GetCenterPoint() const;
	Point GetRowSize() const;
	Rectangle GetRowBounds() const;
	
	
private:
	// Starting position for drawing a column is:
	// point + Point(offset + alignAdjust * font.Width(text), 0.)
	// So, "alignAdjust" will either be 0, -.5, or -1.
	class Column {
	public:
		Column();
		Column(double offset, const Font::Layout &layout);
		
		double offset;
		Font::Layout layout;
	};
	
	
private:
	mutable Point point;
	mutable std::vector<Column>::const_iterator it;
	mutable Color color;
	
	const Font *font;
	Point rowSize;
	Point center;
	Point lineSize;
	Point lineOff;
	
	std::vector<Column> columns;
};



#endif
