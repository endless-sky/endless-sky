/* StarType.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef STAR_H
#define STAR_H

#include <string>

class DataNode;



// Class that represents a type of star. It holds the name of the star's sprite,
// and the values for how much stellar wind and light that type of star gives emits. 
class StarType {
public:
	// Constructor for new star object
	//Star(std::string name, double stellarWind, double luminosity);
	StarType();
		
	// Set this Star to hold the given values.
	void Load(DataNode node);
		
	std::string GetName() const;
		
	double GetStellarWind() const;
		
	double GetLuminosity() const;


private:
	// Represents the name of a type of star. Must correspond to name of 
	// sprite in "endless-sky/images/star/"
	std::string name;
		
	// Values that represent the stellar wind and luminosity that a star
	// of this type emits.
	double stellarWind;
	double luminosity;
};



#endif
