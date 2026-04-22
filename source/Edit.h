/* Edit.h
Copyright (c) 2026 by thewierdnut

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

#include "Panel.h"

#include "Caret.h"
#include "Color.h"
#include "Rectangle.h"

#include <functional>
#include <string>



// Implements an edit control.
class Edit : public Panel {
public:
	enum ALIGN { LEFT, CENTER, RIGHT };


public:
	Edit();
	virtual ~Edit() = default;

	void SetFontSize(int f);
	int FontSize() const { return fontSize; }
	void SetPosition(const Rectangle &position);
	const Rectangle &Position() const { return position; }
	void SetEnabled(bool e) { isEditable = e; }
	bool Enabled() const { return isEditable; }

	const std::string& Text() const;
	void SetText(const std::string& s);
	void Clear();

	virtual void Draw() override;

	void SetAlign(ALIGN a) { alignment = a; }
	ALIGN Align() const { return alignment; }
	void SetPadding(int p);
	void SetLeftPadding(int p);
	void SetRightPadding(int p);
	void SetTopPadding(int p);
	void SetBottomPadding(int p);
	int Padding() const { return leftPadding; }
	int LeftPadding() const { return leftPadding; }
	int RightPadding() const { return rightPadding; }
	int TopPadding() const { return topPadding; }
	int BottomPadding() const { return bottomPadding; }

	void SetVisible(bool v) { visible = v; }
	bool Visible() const { return visible; }
	void SetBgColor(const Color &color) { bgColor = color; }
	const Color &BgColor() const { return bgColor; }

	typedef std::function<bool(std::string &)> ChangedCallback;
	void SetCallback(ChangedCallback cb) { changedCallback = cb; }


protected:
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y, MouseButton button) override;
	virtual bool OnFocus(bool f) override;
	virtual bool TextInput(const std::string &s) override;

	void MoveCaret(size_t pos);
	void UpdateCaret(size_t pos);
	void UpdateText(const std::string &text, size_t caretpos);

	void Cut();
	void Copy();
	void Paste();
	void Undo();
	void Redo();

	// Given an offset into the string, return the position where caret or
	// highlight would go.
	Point AlignedOffset(size_t offset);
	void UpdateHighlightRect(size_t o1, size_t o2);

	void ComputeTextBounds();


private:
	Rectangle position;
	Rectangle textBounds;
	Color bgColor;

	std::vector<std::pair<std::string, size_t>> textHistory;
	size_t historyPos = 0;

	Caret caret;
	size_t caretPos = 0;

	Rectangle highlight{};
	size_t highlightPos = -1;

	bool isActive = true;
	bool isEditable = true;
	bool isHover = false;

	int fontSize = 18;
	ALIGN alignment = LEFT;
	int leftPadding = 5;
	int rightPadding = 5;
	int topPadding = 5;
	int bottomPadding = 5;

	bool visible = true;

	Point dragPos{};

	ChangedCallback changedCallback;
};
