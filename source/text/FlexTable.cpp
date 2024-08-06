/* FlexTable.cpp
Copyright (c) 2024 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FlexTable.h"

#include "../FillShader.h"
#include "Font.h"
#include "../GameData.h"

using namespace std;



FlexTable::Cell::Cell(FlexTable::Column *column, const string &text) : column(column),
		wrapWidth(column->table->AverageColumnWidth()), highlightColor(GameData::Colors().Get("faint")),
		underlineColor(GameData::Colors().Get("medium")), textColor(GameData::Colors().Get("medium")),
		highlightedTextColor(nullptr), font(&FontSet::Get(14))
{
	SetText(text);
}



void FlexTable::Cell::SetText(const string &input)
{
	text.assign(input);
	column->table->Invalidate();
}



const std::string &FlexTable::Cell::Text() const
{
	return text;
}



// The minimum width of the cell, based on the previous wrap width.
// This is not an absolute minimum; rather, it indicates how much of the previously configured space is actually used.
int FlexTable::Cell::MinWidth() const
{
	if(SpansRow())
		return column->table->width;
	return wrappedText.LongestLineWidth() ? wrappedText.LongestLineWidth() - 2 : 0.;
}



// The width and height of the cell, including padding.
int FlexTable::Cell::Width() const
{
	return wrapWidth;
}



int FlexTable::Cell::Height() const
{
	if(text.empty())
		return topGap + bottomGap;
	return wrappedText.Height() + topGap + bottomGap;
}



// Set the font. Default is the size 14 font.
void FlexTable::Cell::SetFont(const Font *font)
{
	this->font = font;
	column->table->Invalidate();
}



const Font *FlexTable::Cell::GetFont() const
{
	return font;
}



// Extra height reserved at the top or bottom of the cell.
// Gaps are synchronized between all cells in a row.
void FlexTable::Cell::SetTopGap(int gap)
{
	int row = GetRow();
	for(auto &column : column->table->columns)
		column.Row(row).topGap = gap;
}



void FlexTable::Cell::SetBottomGap(int gap)
{
	int row = GetRow();
	for(auto &column : column->table->columns)
		column.Row(row).bottomGap = gap;
}



int FlexTable::Cell::GetTopGap() const
{
	return topGap;
}



int FlexTable::Cell::GetBottomGap() const
{
	return bottomGap;
}



// Configures cell decorations. These affect the entire cell, not just the space occupied by text.
void FlexTable::Cell::SetHighlight(bool highlight)
{
	this->highlight = highlight;
}



void FlexTable::Cell::SetUnderline(bool underline)
{
	this->underline = underline;
}



void FlexTable::Cell::SetTextColor(const Color &textColor)
{
	this->textColor = &textColor;
}



// Sets the colors for highlights.
// Setting highlight or underline-related colors automatically enables highlighting/underlining.
void FlexTable::Cell::SetHighlightedTextColor(const Color &textColor)
{
	this->highlightedTextColor = &textColor;
	SetHighlight();
}



void FlexTable::Cell::SetHighlightColor(const Color &highlightColor)
{
	this->highlightColor = &highlightColor;
	SetHighlight();
}



void FlexTable::Cell::SetUnderlineColor(const Color &underlineColor)
{
	this->underlineColor = &underlineColor;
	SetUnderline();
}



int FlexTable::Cell::GetRow() const
{
	for(size_t i = 0; i < column->cells.size(); i++)
		if(&(column->cells[i]) == this)
			return i;
	return -1;
}



// Makes the cell span the entire row. This should only be called within the first column.
void FlexTable::Cell::SpanRow(bool span)
{
	spansRow = span;
	if(span)
		SetWrapWidth(column->table->Width());
}



bool FlexTable::Cell::SpansRow() const
{
	return spansRow;
}



// The optimal width of the column (the most space the text inside can take up).
int FlexTable::Cell::OptimalFlexWidth() const
{
	if(spansRow)
		return column->table->width;
	// WrappedText's width includes 2 pixels of padding on the left side.
	WrappedText text(*font);
	text.SetWrapWidth(column->table->width + 2);
	text.Wrap(Text());
	return text.LongestLineWidth() - 2;
}



void FlexTable::Cell::SetWrapWidth(int width)
{
	wrapWidth = width;
	column->table->Invalidate();
}



void FlexTable::Cell::UpdateLayout()
{
	wrappedText.SetFont(*font);
	wrappedText.SetAlignment(column->alignment);
	wrappedText.SetTruncate(column->truncate);
	wrappedText.SetWrapWidth(wrapWidth + 2); // WrappedText expects 2 pixels of padding
	wrappedText.Wrap(text);
}



// Draws the cell centered on the given point.
void FlexTable::Cell::Draw(const Point &position) const
{
	double gapWidth = (column->DecoratesGap() && !spansRow && column != &column->table->GetColumn(-1)) ?
			column->table->columnSpacing : 0.;
	if(highlight && highlightColor)
	{
		const Point &center = position + Point(Width() / 2. + gapWidth / 2., topGap + wrappedText.Height() / 2.);
		FillShader::Fill(center, Point(Width() + gapWidth, wrappedText.Height()), *highlightColor);
	}
	if(underline && underlineColor)
	{
		const Point &center = position + Point(Width() / 2. + gapWidth / 2., topGap + wrappedText.Height() - 1);
		FillShader::Fill(center, Point(Width() + gapWidth, 1), *underlineColor);
	}
	// WrappedText has 2 pixels of padding on the left side, so move the text's location to compensate for it.
	// The text is printed between the top and bottom gaps.
	const Point &textPos = position + Point(-2., topGap);
	wrappedText.Draw(textPos, (highlight && highlightedTextColor) ? *highlightedTextColor : *textColor);
}



FlexTable::Column::Column(FlexTable *table) : table(table)
{
}



// Gets the cell in the specified row.
const FlexTable::Cell &FlexTable::Column::Row(int row) const
{
	return cells[row];
}



FlexTable::Cell &FlexTable::Column::Row(int row)
{
	table->ClampRow(row);
	return cells[row];
}



int FlexTable::Column::RowCount() const
{
	return cells.size();
}



FlexTable::Cell &FlexTable::Column::AddRow(const string &text)
{
	Cell &cell = cells.emplace_back(this);
	cell.SetText(text);
	return cell;
}



// Gets the width of the column (the width of the widest cell in the column).
// This ignores cells that span across columns.
int FlexTable::Column::Width() const
{
	int width = 0;
	for(const auto &cell : cells)
		if(!cell.SpansRow())
			width = max(width, cell.Width());
	return width;
}



// The width required for this column to best fit in the table,
// accounting for the amount of text contained in each cell.
int FlexTable::Column::OptimalFlexWidth() const
{
	int maxWidth = 0;
	for(const auto &cell : cells)
		if(!cell.SpansRow())
			maxWidth = max(maxWidth, cell.OptimalFlexWidth());
	return maxWidth;
}



void FlexTable::Column::SetAlignment(Alignment alignment)
{
	this->alignment = alignment;
	table->Invalidate();
}



void FlexTable::Column::SetTruncate(Truncate truncate)
{
	this->truncate = truncate;
	table->Invalidate();
}



// A flexing column can extend beyond the width of the text it contains.
void FlexTable::Column::SetFlex(bool flex)
{
	canFlex = flex;
}



bool FlexTable::Column::CanFlex() const
{
	return canFlex;
}



// Sets whether cells decorate the spacing after the column.
void FlexTable::Column::SetDecorateGap(bool decorate)
{
	decorateGap = decorate;
}



bool FlexTable::Column::DecoratesGap() const
{
	return decorateGap;
}



// Fits each cell to the given width by setting their padding values.
// This also sets the column's width to this value.
void FlexTable::Column::FitToWidth(int width)
{
	for(auto &cell : cells)
		if(!cell.SpansRow())
			cell.SetWrapWidth(width);
	UpdateLayout();
}



// Sets the wrap width of each cell to the column's minimum width.
void FlexTable::Column::Pack()
{
	int target = 0;
	for(auto &cell : cells)
		if(!cell.SpansRow())
			target = max(target, cell.MinWidth());
	FitToWidth(target);
}



void FlexTable::Column::UpdateLayout()
{
	for(auto &cell : cells)
		cell.UpdateLayout();
}



// Removes all cells from this column.
void FlexTable::Column::Clear()
{
	cells.clear();
}



FlexTable::FlexTable(int width, int columns) : width(width), columns(columns, Column{this})
{
}



FlexTable::FlexTable(const FlexTable &other) noexcept : width(other.width), rowSpacing(other.rowSpacing),
	columnSpacing(other.columnSpacing), flexStrategy(other.flexStrategy), rowCount(other.rowCount)
{
	columns = other.columns;

	for(auto &column : columns)
	{
		column.table = this;
		for(auto &cell : column.cells)
			cell.column = &column;
	}

	Invalidate();
}



FlexTable &FlexTable::operator=(const FlexTable &other)
{
	width = other.width;
	rowSpacing = other.rowSpacing;
	columnSpacing = other.columnSpacing;
	flexStrategy = other.flexStrategy;
	columns = other.columns;
	rowCount = other.rowCount;

	for(auto &column : columns)
	{
		column.table = this;
		for(auto &cell : column.cells)
			cell.column = &column;
	}

	Invalidate();
	return *this;
}



// Removes all rows from the table, but preserves columns.
void FlexTable::Clear()
{
	for(auto &column : columns)
		column.Clear();
	rowCount = 0;
}



void FlexTable::AddColumn()
{
	columns.emplace_back(this);
	Clear();
}



int FlexTable::Columns() const
{
	return columns.size();
}



const FlexTable::Column &FlexTable::GetColumn(int index) const
{
	return columns[index];
}



FlexTable::Column &FlexTable::GetColumn(int index)
{
	ClampColumn(index);
	return columns[index];
}



int FlexTable::Rows() const
{
	return rowCount;
}



int FlexTable::Width() const
{
	return width;
}



void FlexTable::SetWidth(int width)
{
	this->width = width;
	for(auto &column : columns)
		for(auto &cell : column.cells)
			cell.SetWrapWidth(cell.SpansRow() ? width : AverageColumnWidth());
	Invalidate();
}



// Gets the height of the table until the bottom of the specified row.
int FlexTable::Height(int untilRow)
{
	if(Rows() == 0 || Columns() == 0)
		return 0;

	ClampRow(untilRow);

	UpdateLayout();
	int height = -rowSpacing;
	for(int row = 0; row <= untilRow; ++row)
	{
		int rowHeight = 0;
		for(const auto &column : columns)
			rowHeight = max(rowHeight, column.Width() > 0 ? column.Row(row).Height() : 0);
		height += rowHeight + rowSpacing;
	}
	return height;
}



// Gets the mouse hitbox of a row. This hitbox includes gaps and paddings, but not the spacing between rows.
// The anchor point must be the same as in Draw().
Rectangle FlexTable::GetRowHitbox(int targetRow, const Point &anchor)
{
	ClampRow(targetRow);

	UpdateLayout();

	int startY = targetRow == 0 ? 0 : Height(targetRow - 1);
	int endY = Height(targetRow);
	int topGap = GetCell(targetRow, 0).GetTopGap();
	int bottomGap = GetCell(targetRow, 0).GetBottomGap();

	startY += topGap;
	endY -= bottomGap;
	// Rectangle expects a center point.
	Point offset = Point(width / 2., (endY - startY) / 2.);
	Rectangle hitbox(Point(0., startY) + anchor + offset, Point(width, endY - startY - 1));

	return hitbox;
}



Rectangle FlexTable::GetCellHitbox(int row, int column, const Point &anchor)
{
	ClampRow(row);
	ClampColumn(column);

	UpdateLayout();

	Cell &cell = GetCell(row, column);
	Rectangle rowBox = GetRowHitbox(row, anchor);

	int startX = 0;
	for(int i = 0; i < column; i++)
		startX += columns[i].Width() + columnSpacing;

	Point offset = Point((startX - rowBox.Width() / 2.) + cell.Width() / 2., 0.);
	return Rectangle(rowBox.Center() + offset, Point(cell.Width(), rowBox.Height()));
}



const FlexTable::Cell &FlexTable::GetCell(int row, int column) const
{
	ClampRow(row);
	ClampColumn(column);

	return GetColumn(column).Row(row);
}



FlexTable::Cell &FlexTable::GetCell(int row, int column)
{
	// const_cast is safe here, because we are in a non-const object calling the const function
	return const_cast<FlexTable::Cell &>(const_cast<const FlexTable *>(this)->GetCell(row, column));
}



// Fills a row with individual cells. Any extra arguments are discarded,
// and missing ones are substituted with empty cells.
// Returns a pointer to the first cell in the row, or nullptr if there are no columns in the table.
FlexTable::Cell *FlexTable::FillRow(const std::initializer_list<std::string> &cellTexts)
{
	if(columns.empty())
		return nullptr;
	rowCount++;

	size_t column = 0;
	for(const std::string &text : cellTexts)
	{
		if(column == columns.size())
			break;
		columns[column++].AddRow(text);
	}
	while(column != columns.size())
		columns[column++].AddRow("");

	Invalidate();
	return &columns[0].cells.back();
}



FlexTable::Cell *FlexTable::FillRow(const std::initializer_list<std::pair<std::string, const Color &>> &cellTexts)
{
	if(columns.empty())
		return nullptr;
	rowCount++;

	size_t column = 0;
	for(const auto &[text, color] : cellTexts)
	{
		if(column == columns.size())
			break;
		columns[column++].AddRow(text).SetTextColor(color);
	}
	while(column != columns.size())
		columns[column++].AddRow("");

	Invalidate();
	return &columns[0].cells.back();
}



// Fills a row with a single cell. This cell spans across every column.
// Returns a pointer to the cell, or nullptr if there are no columns in the table.
FlexTable::Cell *FlexTable::FillUnifiedRow(const std::string &text, const Color &color)
{
	if(columns.empty())
		return nullptr;
	rowCount++;

	auto it = columns.begin();
	Cell &first = it->AddRow(text);
	first.SetTextColor(color);
	first.SpanRow();

	it++;
	for(; it != columns.end(); it++)
		it->AddRow("");

	Invalidate();
	return &columns[0].cells.back();
}



// Removes the last n rows of the table.
void FlexTable::PopRow(int amount)
{
	for(auto &column : columns)
		for(int i = 0; i < amount; ++i)
			column.cells.pop_back();
	rowCount -= amount;
	Invalidate();
}



// Configures the number of empty pixels between each table row (not text rows within a single cell).
void FlexTable::SetRowSpacing(int spacing)
{
	rowSpacing = spacing;
}



int FlexTable::GetRowSpacing() const
{
	return rowSpacing;
}



// Configures the number of empty pixels between each table column.
// This is ignored around empty columns.
void FlexTable::SetColumnSpacing(int spacing)
{
	columnSpacing = spacing;
	Invalidate();
}



int FlexTable::GetColumnSpacing() const
{
	return columnSpacing;
}



// Highlights the row containing the specific point, and turns off highlighting for every other cell.
// Returns the index of the row containing the point, or -1 if it's not in the table.
// If allowFirstRow is false, the first row is ignored.
// The anchor point must be the same as in Draw().
int FlexTable::SetRowHighlight(const Point &point, const Point &anchor, bool allowFirstRow)
{
	int foundRow = -1;
	for(int row = !allowFirstRow; row < Rows(); ++row)
	{
		bool contains = GetRowHitbox(row, anchor).Contains(point);
		if(contains)
			foundRow = row;
		for(auto &column : columns)
			column.Row(row).SetHighlight(contains);
	}
	return foundRow;
}



// Draws the table, and returns a point under the table for other draw operations.
// The anchor point is the center of the top edge of the table.
Point FlexTable::Draw(const Point &position)
{
	if(columns.empty())
		return position;

	UpdateLayout();

	Point rowBegin = position;
	for(int row = 0; row < Rows(); ++row)
	{
		int rowHeight = 0;
		Point cellBegin{rowBegin};
		for(const auto &column : columns)
		{
			// Skip empty columns
			if(column.Width() == 0 && !column.Row(row).SpansRow())
				continue;

			const Cell &cell = column.Row(row);
			rowHeight = max(rowHeight, cell.Height());
			cell.Draw(cellBegin);
			cellBegin.X() += cell.Width() + columnSpacing;

			if(cell.SpansRow())
				break;
		}
		rowBegin.Y() += rowSpacing + rowHeight;
	}
	return rowBegin;
}



FlexTable::FlexStrategy FlexTable::GetFlexStrategy() const
{
	return flexStrategy;
}



void FlexTable::SetFlexStrategy(FlexTable::FlexStrategy strategy)
{
	flexStrategy = strategy;
}



// The width of an average column in the table, without spacing.
int FlexTable::AverageColumnWidth() const
{
	return (width - (columns.size() - 1) * columnSpacing) / columns.size();
}



// Marks that the layout of the table has to be recomputed.
void FlexTable::Invalidate()
{
	valid = false;
}



void FlexTable::UpdateLayout()
{
	// Only update the layout if it changed.
	if(valid)
		return;
	if(columns.empty())
		return;
	for(auto &column : columns)
		column.UpdateLayout();

	// The individual strategy is simple, so let's check for that first.
	if(flexStrategy == FlexStrategy::INDIVIDUAL)
	{
		// Calculate the available space, minus any spacing.
		int availableWidth = width + columnSpacing;
		for(auto &column : columns)
			if(column.Width())
				availableWidth -= columnSpacing;

		// Flex each row.
		for(int row = 0; row < Rows(); ++row)
		{
			int currentWidth = 0;
			bool spansRow = false;
			for(auto &column : columns)
				if(column.Width())
				{
					Cell &cell = column.Row(row);
					cell.SetWrapWidth(cell.MinWidth());
					if(cell.SpansRow())
					{
						spansRow = true;
						break;
					}
					currentWidth += cell.Width();
				}
			if(spansRow) // This row is used by a single cell, so we don't need to distribute space.
				continue;
			// Now that we know how much space is available, we can distribute it.
			for(auto &column : columns)
			{
				if(!column.CanFlex())
					continue;

				Cell &cell = column.Row(row);
				int extra = cell.OptimalFlexWidth() - cell.Width();
				if(extra <= availableWidth - currentWidth)
				{
					cell.SetWrapWidth(cell.OptimalFlexWidth());
					currentWidth += extra;
				}
				else
				{
					cell.SetWrapWidth(cell.Width() + (availableWidth - currentWidth));
					currentWidth = availableWidth;
					break;
				}
			}
			// Distribute any leftover space.
			Cell &first = columns.front().Row(row);
			first.SetWrapWidth(first.Width() + (availableWidth - currentWidth));
		}
		// Update all layouts.
		for(auto &column : columns)
			column.UpdateLayout();
		valid = true;
		return;
	}

	// Otherwise, perform all column flexing operations.

	// Calculate the size of each column.
	// Since some columns might be smaller than the initially used average width,
	// others can be expanded to fit their contents better.
	// Empty columns are treated as if they weren't in the table at all (but not empty cells!).
	int overallWidth = -columnSpacing;
	vector<Column *> flexColumns; // columns that can to grow
	for(auto &column : columns)
	{
		column.Pack();
		int width = column.Width();
		if(width)
		{
			overallWidth += width + columnSpacing;
			if(column.CanFlex())
				flexColumns.emplace_back(&column);
		}
	}

	if(flexColumns.empty()) // No columns can expand, so skip the rest of the computations.
		return;

	// Calculate what total width the columns are requesting
	int freeWidth = width - overallWidth;
	int requestedWidth = 0;
	for(const auto &column : flexColumns)
	{
		int optimal = column->OptimalFlexWidth();
		requestedWidth += optimal - column->Width();
	}
	if(requestedWidth <= freeWidth)
	{
		// All columns fit in the table without line breaks, so give them their optimal width.
		for(const auto column : flexColumns)
			column->FitToWidth(column->OptimalFlexWidth());
		// Then make them fill up the rest of the empty space in the table.
		freeWidth -= requestedWidth;
		switch(flexStrategy)
		{
			case FlexStrategy::INDIVIDUAL: // can't happen
				break;
			case FlexStrategy::FIRST:
				flexColumns.front()->FitToWidth(flexColumns.front()->Width() + freeWidth);
				break;
			case FlexStrategy::LAST:
				flexColumns.back()->FitToWidth(flexColumns.back()->Width() + freeWidth);
				break;
			case FlexStrategy::EVEN:
				int perColumn = freeWidth / flexColumns.size();
				int remainder = freeWidth % flexColumns.size();
				//
				for(auto column : flexColumns)
					column->FitToWidth(column->Width() + perColumn);
				flexColumns.front()->FitToWidth(flexColumns.front()->Width() + remainder);
				break;
		}
	}
	else
	{
		// We don't have enough space to fit everything, so start handing out the free space while it lasts.
		if(flexStrategy == FlexStrategy::FIRST || flexStrategy == FlexStrategy::LAST)
		{
			bool first = flexStrategy == FlexStrategy::FIRST;
			for(size_t i = (first ? 0 : flexColumns.size() - 1); i != (first ? flexColumns.size() : -1); i++)
			{
				Column *column = flexColumns[i];
				int requested = column->OptimalFlexWidth() - column->Width();
				if(requested >= freeWidth)
				{
					column->FitToWidth(column->Width() + freeWidth);
					break;
				}
				else
				{
					column->FitToWidth(column->Width() + requested);
					freeWidth -= requested;
				}
			}
		}
		else // FlexStrategy::EVEN
		{
			// Hand out free space to all columns who still need it.
			// Some may need less space than what would is distributed, so reuse their extra space as well.
			while(freeWidth && freeWidth >= static_cast<int>(flexColumns.size()))
			{
				int perColumn = freeWidth / flexColumns.size();
				vector<Column *> remaining;
				remaining.reserve(flexColumns.size());

				for(auto column : flexColumns)
				{
					// Check if the column accepts all of the extra width.
					int expected = column->OptimalFlexWidth() - column->Width();
					if(expected > perColumn)
					{
						column->FitToWidth(column->Width() + perColumn);
						freeWidth -= perColumn;
						remaining.emplace_back(column);
					}
					else
					{
						column->FitToWidth(column->OptimalFlexWidth());
						freeWidth -= expected;
					}
				}
				flexColumns = remaining;
			}
			// Distribute the remainder. (There are fewer pixels left than columns,
			// since anything greater would have already been distributed.)
			for(int i = 0; i < freeWidth; ++i)
				flexColumns[i]->FitToWidth(flexColumns[i]->Width() + 1);

		}
		for(auto &column : columns)
			column.UpdateLayout();
		// We filled up the entire width of the table; nothing more can be done.
	}
	valid = true;
}



// Finds the row that corresponds to the given offset, supporting negative values and wrap-arounds.
void FlexTable::ClampRow(int &row) const
{
	if(Rows() == 0)
		row = 0;
	else
	{
		row %= Rows();
		if(row < 0)
			row += Rows();
	}
}



void FlexTable::ClampColumn(int &column) const
{
	if(Columns() == 0)
		column = 0;
	else
	{
		column %= Columns();
		if(column < 0)
			column += Columns();
	}
}
