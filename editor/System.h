/* System.h
Michael Zahniser, 26 Dec 2013

Class representing a star system.
*/

#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED

#include "DataFile.h"
#include "Point.h"
#include "Set.h"

#include <list>
#include <ostream>
#include <string>
#include <vector>
#include <map>

class Planet;



class System {
public:
	static void Init();
	static int TradeMin(const std::string &name);
	static int TradeMax(const std::string &name);
	
	class Object {
	public:
		Object(double d = 0., double p = 10., const std::string &sprite = "", double o = 0.);
		
		void AddStation();
		
		std::string sprite;
		
		// Orbital parameters:
		double distance;
		double period;
		double offset;
		
		// If this object if a visitable planet...
		std::string planet;
		
		// These objects orbit around this one.
		std::list<Object> children;
	};
	
	class Asteroids {
	public:
		std::string name;
		int count;
		double energy;
	};
	
	
public:
	void Load(const DataFile::Node &node, const Set<System> &systems);
	void LoadObject(Object &parent, const DataFile::Node &node);
	
	int Trade(const std::string &name) const;
	float TradeRange(const std::string &name) const;
	
	void Randomize();
	void Sol();
	
	void ToggleLink(System *link);
	
	void Write(std::ostream &out) const;
	void Write(std::ostream &out, const Object &object, int depth) const;
	
	
public:
	std::string name;
	std::string government;
	Point pos;
	
	Object root;
	double habitable;
	
	std::vector<const System *> links;
	std::map<std::string, int> trade;
	std::vector<Asteroids> asteroids;
	
	std::list<DataFile::Node> unrecognized;
};



#endif
