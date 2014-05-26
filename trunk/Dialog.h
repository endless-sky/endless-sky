/* Dialog.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DIALOG_H_
#define DIALOG_H_

#include "Panel.h"

#include "Point.h"
#include "WrappedText.h"

#include <string>



class Dialog : public Panel {
public:
	// Dialog that has no callback (information only). In this form, there is
	// only an "ok" button, not a "cancel" button.
	Dialog(const std::string &text);
	virtual ~Dialog() = default;
	
	// Three different kinds of dialogs can be constructed: requesting numerical
	// input, requesting text input, or not requesting any input at all. In any
	// case, the callback is called only if the user selects "ok", not "cancel."
template <class T>
	Dialog(T &t, void (T::*fun)(int), const std::string &text);
	
template <class T>
	Dialog(T &t, void (T::*fun)(const std::string &), const std::string &text);
	
template <class T>
	Dialog(T &t, void (T::*fun)(), const std::string &text);
	
	// Draw this panel.
	virtual void Draw() const;
	
	
protected:
	// The use can click "ok" or "cancel", or use the tab key to toggle which
	// button is highlighted and the enter key to select it.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	
	
private:
	// Common code from all three constructors:
	void Init(const std::string &message, bool canCancel = true);
	void DoCallback() const;
	
	
private:
	WrappedText text;
	int height;
	
	std::function<void(int)> intFun;
	std::function<void(const std::string &)> stringFun;
	std::function<void()> voidFun;
	
	bool canCancel;
	bool okIsActive;
	
	std::string input;
	
	mutable Point okPos;
	mutable Point cancelPos;
};



template <class T>
Dialog::Dialog(T &t, void (T::*fun)(int), const std::string &text)
	: intFun(std::bind(fun, t, std::placeholders::_1))
{
	Init(text);
}



template <class T>
Dialog::Dialog(T &t, void (T::*fun)(const std::string &), const std::string &text)
	: stringFun(std::bind(fun, t, std::placeholders::_1))
{
	Init(text);
}



template <class T>
Dialog::Dialog(T &t, void (T::*fun)(), const std::string &text)
	: voidFun(std::bind(fun, t))
{
	Init(text);
}



#endif
