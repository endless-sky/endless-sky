/* TextTooltip.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TEXT_TOOLTIP_H_
#define TEXT_TOOLTIP_H_

#include "ClickZone.h"
#include "Point.h"
#include "WrappedText.h"

#include <string>
#include <vector>


// Class that a will display a text tooltip after hovering a label for 60 frames.
// It can auto scale the width.
class TextTooltip {
public:
	TextTooltip();
	
	void Draw();
	void Clear();
	
	const Point &HoverPoint() const;
	void SetHoverPoint(const Point &point);
	
	const std::string &Label() const;
	void SetLabel(const std::string &label);
	
	std::vector<ClickZone<std::string>> &Zones();
	void CheckZones();
	
	WrappedText &Text();
	
	int HoverCount() const;
	
	
private:
	int hoverCount = 0;
	Point hoverPoint;
	std::string hoverLabel;
	std::vector<ClickZone<std::string>> hoverZones;
	WrappedText tooltipText;
};



#endif
