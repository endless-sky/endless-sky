/* ScrollVar.h
Copyright (c) 2023 by thewierdnut

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SCROLLVAR_H_INCLUDED
#define SCROLLVAR_H_INCLUDED

#include "Animate.h"



// This class will allow you to set a scroll value, and provide an animation
// interpolation between the old value and the new value, and will clamp
// the value to a suitable range.
// The value returned by the Animate methods will be negative, as it is meant
// to be added as an offset to the draw position.
template <typename T>
class ScrollVar: public Animate<T>
{
public:
	ScrollVar(const T &maxVal = T{}, const T &displaySize = T{});

	// Set the maximum size of the scroll content.
	void SetMaxValue(const T &value);
	// Set the size of the displayable scroll area.
	void SetDisplaySize(const T &size);
	// Returns true if scroll buttons are needed.
	bool Scrollable() const;
	// Returns true if the value is at the minimum.
	bool ScrollAtMin() const;
	// Returns true if the value is at the maximum.
	bool ScrollAtMax() const;
	// Modifies the scroll value by dy, then clamps it to a suitable range.
	void Scroll(const T &dy, int steps = 5);

	// Sets the scroll value directly, then clamps it to a suitable range.
	virtual void Set(const T &current, int steps = 5) override;


private:
	// Makes sure the animation value stays in range.
	void Clamp(int steps);

	T maxVal;
	T displaySize;
};



template <typename T>
ScrollVar<T>::ScrollVar(const T &maxVal, const T &displaySize):
	maxVal{maxVal},
	displaySize{displaySize}
{

}



template <typename T>
void ScrollVar<T>::SetMaxValue(const T &value)
{
	maxVal = value;
	Clamp(0);
}



template <typename T>
void ScrollVar<T>::SetDisplaySize(const T &size)
{
	displaySize = size;
	Clamp(0);
}



template <typename T>
bool ScrollVar<T>::Scrollable() const
{
	return maxVal > displaySize;
}



template <typename T>
bool ScrollVar<T>::ScrollAtMin() const
{
	return this->Value() >= T{};
}



template <typename T>
bool ScrollVar<T>::ScrollAtMax() const
{
	return this->Value() <= displaySize - maxVal;
}



template <typename T>
void ScrollVar<T>::Scroll(const T &dy, int steps)
{
	Set(this->Value() + dy, steps);
}



template <typename T>
void ScrollVar<T>::Set(const T &current, int steps)
{
	Animate<T>::Set(current, steps);
	Clamp(steps);
}



template <typename T>
void ScrollVar<T>::Clamp(int steps)
{
	int minScroll = displaySize - maxVal;
	if(this->Value() > T{} || maxVal < displaySize)
		Animate<T>::Set(T{}, steps);
	else if(this->Value() < minScroll)
		Animate<T>::Set(minScroll, steps);
}



#endif
