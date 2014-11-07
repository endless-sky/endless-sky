/* ClickZone.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CLICK_ZONE_H_
#define CLICK_ZONE_H_

#include "Point.h"



template <class Type>
class ClickZone {
public:
	ClickZone(Point center, Point size, Type value = 0);
	
	bool Contains(Point point) const;
	Type Value() const;
	
	
private:
	Point center;
	Point size;
	
	Type value;
};



template <class Type>
ClickZone<Type>::ClickZone(Point center, Point size, Type value)
	: center(center), size(size * .5), value(value)
{
}


	
template <class Type>
bool ClickZone<Type>::Contains(Point point) const
{
	point -= center;
	
	return (fabs(point.X()) < size.X() && fabs(point.Y()) < size.Y());
}



template <class Type>
Type ClickZone<Type>::Value() const
{
	return value;
}



#endif
