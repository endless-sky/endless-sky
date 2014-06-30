/* Personality.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PERSONALITY_H_
#define PERSONALITY_H_

#include "DataFile.h"

#include "Point.h"



// Class defining an AI "personality": what actions it takes, and how skilled
// and aggressive it is in combat.
class Personality {
public:
	Personality();
	
	void Load(const DataFile::Node &node);
	
	bool IsPacifist() const;
	bool IsForbearing() const;
	bool IsTimid() const;
	bool Disables() const;
	bool Plunders() const;
	bool IsHeroic() const;
	
	const Point &Confusion() const;
	
	
private:
	int flags;
	double confusionMultiplier;
	mutable Point confusion;
};



#endif
