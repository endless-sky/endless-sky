/* Galaxy.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GALAXY_H_
#define GALAXY_H_

#include "Body.h"

#include "Point.h"
#include "Set.h"

#include <set>
#include <string>
#include <vector>

class DataNode;
class Sprite;
class System;



// This is any object that should be drawn as a backdrop to the map.
class Galaxy : public Body {
public:
	// A label that appears on the map that describes a region of the galaxy.
	class Label : public Body
	{
	public:
		Label(const DataNode &node);
		// Construct a label from a galaxy (legacy).
		Label(const Galaxy *galaxy);

		// The name of this label in the data files.
		std::string name;
	};


public:
	void Load(const DataNode &node, Set<Galaxy> &galaxies);
	
	const std::string &Name() const;
	const Point &Position() const;
	const std::set<const System *> &Systems() const;
	const std::vector<Label> &Labels() const;

	void SetName(const std::string &name);
	void SetPosition(Point pos);

	// Adds the given label to this galaxy (legacy).
	void AddLabel(Galaxy *label);

	// Adds or removes a system from a galaxy (backwards compatbility).
	void AddSystem(const System *system);
	void RemoveSystem(const System *system);


private:
	std::string name;
	std::set<const System *> systems;
	std::vector<Label> labels;

	// If this galaxy has a parent. This is only the case when this galaxy
	// is a legacy label.
	const Galaxy *parent = nullptr;
};

#endif
