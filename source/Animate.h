/* Animate.h
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



// Smoothly change a variable from one value to another. Used to smooth out
// scrolling and panning.
template<typename T>
class Animate {
public:
	virtual ~Animate() = default;
	// Set the next target value of this variable, linearly interpolated along
	// the given number of frames.
	virtual void Set(const T &current, int steps = 5);

	// Reset the pending number of frames to zero. This makes the interpolated
	// value jump straight to the target value.
	void EndAnimation();
	// Compute the next interpolated value. This needs to be called once per frame.
	void Step();

	// Returns the interpolated value.
	const T &AnimatedValue() const;
	// Returns the actual value.
	const T &Value() const;
	// Synonym for Value().
	operator const T &() const;
	// Returns true if there are no more animation steps pending.
	bool IsAnimationDone() const;

	// Shortcut mathematical operators for convenience.
	Animate &operator=(const T &v);
	Animate &operator+=(const T &v);
	Animate &operator-=(const T &v);


private:
	int steps = 0;
	T current{};
	T target{};

};



template<typename T>
void Animate<T>::Set(const T &current, int steps)
{
	this->steps = steps;
	this->target = current;
}



template<typename T>
void Animate<T>::EndAnimation()
{
	steps = 0;
}



template<typename T>
void Animate<T>::Step()
{
	if(steps <= 0)
		current = target;
	else
	{
		T delta = (target - current) / steps;
		current += delta;
		--steps;
	}
}



template<typename T>
const T &Animate<T>::AnimatedValue() const
{
	return current;
}



template<typename T>
const T &Animate<T>::Value() const
{
	return target;
}



template<typename T>
Animate<T>::operator const T &() const
{
	return Value();
}



template<typename T>
bool Animate<T>::IsAnimationDone() const
{
	return steps == 0;
}



template<typename T>
Animate<T> &Animate<T>::operator=(const T &v)
{
	Set(v);
	return *this;
}



template<typename T>
Animate<T> &Animate<T>::operator+=(const T &v)
{
	Set(target + v);
	return *this;
}



template<typename T>
Animate<T> &Animate<T>::operator-=(const T &v)
{
	Set(target - v);
	return *this;
}
