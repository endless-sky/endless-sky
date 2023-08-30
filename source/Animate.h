/* Animate.h
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/
#ifndef ANIMATE_H_INCLUDED
#define ANIMATE_H_INCLUDED

// Smoothly change a variable from one value to another. Used to smooth out
// scrolling and panning.
template <typename VarT>
class Animate
{
public:
	void Set(const VarT& current, int steps)
	{
		m_steps = steps;
		m_current = current;
	}
	void Reset() { m_steps = 0; }
	void Step(const VarT& target)
	{
		if(m_steps <= 0)
			m_current = target;
		else
		{
			VarT delta = (target - m_current) / m_steps;
			m_current += delta;
			--m_steps;
		}
	}
	const VarT& Get() const { return m_current; }
	operator const VarT&() const { return Get(); }

private:
	int m_steps = 0;
	VarT m_current{};
};

#endif