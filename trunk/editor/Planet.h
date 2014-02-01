/* Planet.h
Michael Zahniser, 29 Dec 2013

Class representing a planet that you can land on.
*/

#ifndef PLANET_H_INCLUDED
#define PLANET_H_INCLUDED

#include "DataFile.h"

#include <string>
#include <list>
#include <ostream>



class Planet {
public:
	void Load(const DataFile::Node &node);
	void Write(std::ostream &out) const;
	
	std::string name;
	std::string landscape;
	std::string description;
	
	std::list<DataFile::Node> unrecognized;
};



#endif
