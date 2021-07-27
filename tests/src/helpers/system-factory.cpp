/* system-factory.cpp
Copyright (c) 2021 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "system-factory.h"

#include "datanode-factory.h"
#include "es-test.hpp"
#include "../../../source/Planet.h"
#include "../../../source/Set.h"
#include "../../../source/System.h"

#include <string>



// Method to convert text input into consumable Systems.
Set<System> AsSystems(std::string text)
{
	auto nodes = AsDataNodes(std::move(text));
	Set<System> systems;
	Set<Planet> planets;
	for(const auto &node : nodes)
	{
		REQUIRE(node.Size() == 2);
		if(node.Token(0) == "system")
			systems.Get(node.Token(1))->Load(node, planets, systems);
		else if(node.Token(1) == "planet")
			planets.Get(node.Token(1))->Load(node);
	}

	return systems;
}



// Convert the text to a single system.
System AsSystem(std::string text)
{
	auto nodes = AsDataNode(std::move(text));

	System system;
	Set<Planet> planets;
	Set<System> systems;
	system.Load(nodes, planets, systems);
	return system;
}
