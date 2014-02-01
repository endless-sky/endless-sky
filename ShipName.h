/* ShipName.h
Michael Zahniser, 5 Jan 2014

Class representing a set of rules for generating random ship names.
*/

#ifndef SHIP_NAME_INCLUDED
#define SHIP_NAME_INCLUDED

#include "DataFile.h"

#include <string>
#include <vector>



class ShipName {
public:
	void Load(const DataFile::Node &node);
	
	std::string Get() const;
	
	
private:
	std::vector<std::vector<std::vector<std::string>>> words;
};



#endif
