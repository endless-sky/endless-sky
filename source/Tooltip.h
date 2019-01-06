/* Tooltip.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TOOLTIP_H_

#define TOOLTIP_H_

#include <string>
#include <vector>

class Outfit;

class Tooltip {
public:
	Tooltip();
	Tooltip(std::string label);

	const std::string& Text() const;
	const bool HasExtras() const;
	const std::string ExtrasForOutfit(const Outfit& outfit) const;

	void AddText(std::string newText);
	void AddComparison(std::string attributeName);
	void SetSummary(bool shouldAddSummary);

private:	
	std::string text;
	std::vector<std::string> compareTo;
	bool addSummary;
	const std::string label;
};

#endif // TOOLTIP_H_
