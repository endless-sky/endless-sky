/* EscortDisplay.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ESCORT_DISPLAY_H_
#define ESCORT_DISPLAY_H_

#include <cstdint>
#include <string>
#include <vector>

class Ship;
class Sprite;



// This class stores a list of escorts, sorted according to their value and
// stacked if necessary to get them to all fit on screen.
class EscortDisplay {
public:
	void Clear();
	void Add(const Ship &ship, bool isHere);
	
	// The display starts in the lower left corner of the screen and takes up
	// all but the top 450 pixels of the screen.
	void Draw() const;
	
	
private:
	class Icon {
	public:
		Icon(const Ship &ship, bool isHere);
		
		// Sorting operator.
		bool operator<(const Icon &other) const;
		
		int Height() const;
		void Merge(const Icon &other);
		
		const Sprite *sprite;
		bool isHere;
		bool isReadyToJump;
		int stackSize;
		int64_t cost;
		std::string system;
		std::vector<double> low;
		std::vector<double> high;
	};
	
	
private:
	void MergeStacks() const;
	
	
private:
	mutable std::vector<Icon> icons;
};



#endif
