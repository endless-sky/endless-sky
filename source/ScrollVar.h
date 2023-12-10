/* ScrollVar.h
Copyright (c) 2023 by Rian Shelley

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
template <typename VarT>
class ScrollVar: public Animate<VarT>
{
public:
	ScrollVar(const VarT & maxVal = VarT{}, const VarT & displaySize = VarT{}) :
		m_maxVal{maxVal},
		m_displaySize{displaySize}
	{}

	// Set the maximum size of the scroll content.
	void SetMaxValue(const VarT & value) { m_maxVal = value; Clamp(0); }
	// Set the size of the displayable scroll area.
	void SetDisplaySize(const VarT & size) { m_displaySize = size; Clamp(0); }
	// Returns true if scroll buttons are needed.
	bool Scrollable() const { return m_maxVal > m_displaySize; }
	// Returns true if the value is at the minimum.
	bool ScrollAtMin() const { return this->Value() >= VarT{}; }
	// Returns true if the value is at the maximum.
	bool ScrollAtMax() const { return this->Value() <= m_displaySize - m_maxVal; }
	// Modifies the scroll value by dy, then clamps it to a suitable range
	void Scroll(const VarT & dy, int steps = 5) { Set(this->Value() + dy, steps); }

	// Sets the scroll value directly, then clamps it to a suitable range
	virtual void Set(const VarT & current, int steps = 5) override
	{
		Animate<VarT>::Set(current, steps);
		Clamp(steps);
	}

private:
	// Makes sure the animation value stays in range.
	void Clamp(int steps)
	{
		int minScroll = m_displaySize - m_maxVal;
		if(this->Value() > VarT{} || m_maxVal < m_displaySize)
			Animate<VarT>::Set(VarT{}, steps);
		else if(this->Value() < minScroll)
			Animate<VarT>::Set(minScroll, steps);
	}

	VarT m_maxVal;
	VarT m_displaySize;
};

#endif
