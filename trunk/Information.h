/* Information.h
Michael Zahniser, 24 Oct 2013

Class representing information to be displayed in a user interface, independent
of how that information is laid out or shown.

The Interface will also have access to a list of sprites.
*/

#ifndef INFORMATION_H_INCLUDED
#define INFORMATION_H_INCLUDED

#include <map>
#include <set>
#include <string>

class Sprite;
class Radar;



class Information {
public:
	Information();
	
	void SetSprite(const std::string &name, const Sprite *sprite);
	const Sprite *GetSprite(const std::string &name) const;
	
	void SetString(const std::string &name, const std::string &value);
	const std::string &GetString(const std::string &name) const;
	
	void SetBar(const std::string &name, double value, double segments = 0.);
	double BarValue(const std::string &name) const;
	double BarSegments(const std::string &name) const;
	
	void SetRadar(const Radar &radar);
	const Radar *GetRadar() const;
	
	void SetCondition(const std::string &condition);
	bool HasCondition(const std::string &condition) const;
	
	
private:
	std::map<std::string, const Sprite *> sprites;
	std::map<std::string, std::string> strings;
	std::map<std::string, double> bars;
	std::map<std::string, double> barSegments;
	
	const Radar *radar;
	
	std::set<std::string> conditions;
};



#endif
