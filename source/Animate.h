/* Animate.h
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

#ifndef ANIMATE_H_INCLUDED
#define ANIMATE_H_INCLUDED

// Smoothly change a variable from one value to another. Used to smooth out
// scrolling and panning.
template <typename VarT>
class Animate
{
public:
	virtual ~Animate() = default;
	// Set the next target value of this variable, linearly interpolated using
	// steps frames.
	virtual void Set(const VarT & current, int steps = 5)
	{
		m_steps = steps;
		m_target = current;
	}
	// Reset the pending number of frames to zero. This makes the interpolated
	// value jump straight to the target value.
	void ResetAnimation() { m_steps = 0; }
	// Compute the next interpolated value. This needs called once per frame.
	void Step()
	{
		if(m_steps <= 0)
			m_current = m_target;
		else
		{
			VarT delta = (m_target - m_current) / m_steps;
			m_current += delta;
			--m_steps;
		}
	}

	// Returns the interpolated value.
	const VarT & AnimatedValue() const { return m_current; }
	// Returns the actual value.
	const VarT & Value() const { return m_target; }
	// Synonym for Value()
	operator const VarT &() const { return Value(); }

	// Shortcut mathmatical operators for convenience
	Animate & operator=(const VarT & v) { Set(v); return *this; }
	Animate & operator+=(const VarT & v) { Set(m_target + v); return *this; }
	Animate & operator-=(const VarT & v) { Set(m_target - v); return *this; }

private:
	int m_steps = 0;
	VarT m_current{};
	VarT m_target{};
};

#endif
