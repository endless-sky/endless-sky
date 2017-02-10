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
#include "Rectangle.h"

#include <cmath>



// This is a simple template class defining a rectangular region in the UI that
// may take action if it is clicked on. The region stores a single data object
// that identifies it or identifies the action to take.
template <class Type>
class ClickZone : public Rectangle {
public:
	// Constructor. The "dimensions" are the full width and height of the zone.
	explicit ClickZone(const Rectangle &rect, Type value = 0);
	ClickZone(Point center, Point dimensions, Type value = 0);
	
	// Retrieve the value associated with this zone.
	Type Value() const;
	
	
private:
	Type value;
};



template <class Type>
ClickZone<Type>::ClickZone(Point center, Point dimensions, Type value)
	: Rectangle(center, dimensions), value(value)
{
}



template <class Type>
ClickZone<Type>::ClickZone(const Rectangle &rect, Type value)
	: Rectangle(rect), value(value)
{
}



template <class Type>
Type ClickZone<Type>::Value() const
{
	return value;
}



#endif
