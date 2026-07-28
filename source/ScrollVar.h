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

#pragma once

#include "Animate.h"



// This class allows you to set a scroll value and provides animation
// interpolation between the old and new values, while clamping the value to a
// suitable range. The Animate methods will return negative values, as they are
// meant to be added as an offset to the draw position.
template<typename T>
class ScrollVar : public Animate<T> {
public:
	ScrollVar() = default;
	ScrollVar(const T &maxVal, const T &displaySize);

	// Set the maximum size of the scroll content.
	void SetMaxValue(const T &value);
	// Get the maximum size of the scroll content.
	const T &MaxValue() const;
	// Set the size of the displayable scroll area.
	void SetDisplaySize(const T &size);
	// Get the size of the displayable scroll area.
	const T &DisplaySize() const;
	// Returns true if scroll buttons are needed.
	bool Scrollable() const;
	// Returns true if the value is at the minimum.
	bool IsScrollAtMin() const;
	// Returns true if the value is at the maximum.
	bool IsScrollAtMax() const;
	// Modifies the scroll value by dy, then clamps it to a suitable range.
	void Scroll(const T &dy, int steps = 5);

	double AnimatedScrollFraction() const;
	double ScrollFraction() const;

	// Sets the scroll value directly, then clamps it to a suitable range.
	virtual void Set(const T &current, int steps = 5) override;

	// Shortcut mathematical operators for convenience.
	ScrollVar &operator=(const T &v);


private:
	// Makes sure the animation value stays in range.
	void Clamp(int steps);

	T maxVal{};
	T displaySize{};
};



template<typename T>
ScrollVar<T>::ScrollVar(const T &maxVal, const T &displaySize)
	: maxVal{maxVal}, displaySize{displaySize}
{
}



template<typename T>
void ScrollVar<T>::SetMaxValue(const T &value)
{
	maxVal = value;
	Clamp(0);
}



template<typename T>
const T &ScrollVar<T>::MaxValue() const
{
	return maxVal;
}



template<typename T>
void ScrollVar<T>::SetDisplaySize(const T &size)
{
	displaySize = size;
	Clamp(0);
}



template<typename T>
const T &ScrollVar<T>::DisplaySize() const
{
	return displaySize;
}




template<typename T>
bool ScrollVar<T>::Scrollable() const
{
	return maxVal > displaySize;
}



template<typename T>
bool ScrollVar<T>::IsScrollAtMin() const
{
	return this->Value() <= T{};
}



template<typename T>
bool ScrollVar<T>::IsScrollAtMax() const
{
	return this->Value() >= maxVal - displaySize;
}



template<typename T>
void ScrollVar<T>::Scroll(const T &dy, int steps)
{
	Set(this->Value() + dy, steps);
}



template<typename T>
double ScrollVar<T>::AnimatedScrollFraction() const
{
	return static_cast<double>(Animate<T>::AnimatedValue()) / static_cast<double>(maxVal - displaySize);
}



template<typename T>
double ScrollVar<T>::ScrollFraction() const
{
	return static_cast<double>(Animate<T>::Value()) / static_cast<double>(maxVal - displaySize);
}



template<typename T>
void ScrollVar<T>::Set(const T &current, int steps)
{
	Animate<T>::Set(current, steps);
	Clamp(steps);
}



template<typename T>
void ScrollVar<T>::Clamp(int steps)
{
	int maxScroll = maxVal - displaySize;
	if(this->Value() < T{} || maxVal < displaySize)
		Animate<T>::Set(T{}, steps);
	else if(this->Value() > maxScroll)
		Animate<T>::Set(maxScroll, steps);
}



template<typename T>
ScrollVar<T> &ScrollVar<T>::operator=(const T &v)
{
	Set(v);
	return *this;
}
