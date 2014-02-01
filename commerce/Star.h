/* Star.h
Michael Zahniser, 15 Oct 2013

Representation of each star system in the commerce model.
*/

#ifndef STAR_H
#define STAR_H

#include "DataFile.h"

#include <list>
#include <map>
#include <ostream>
#include <string>
#include <vector>



class Star {
public:
	Star();
	Star(const DataFile::Node &node);
	
	const std::string &Name() const;
	double X() const;
	double Y() const;
	
	const std::vector<std::string> &Links() const;
	
	double Trade(const std::string &commodity) const;
	void SetTrade(const std::string &commodity, double value);
	
	void Write(std::ostream &out) const;
	
	
private:
	std::string name;
	double x;
	double y;
	std::vector<std::string> links;
	
	std::map<std::string, double> trade;
	
	std::list<DataFile::Node> unrecognized;
};



#endif
