/* Person.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "LocationFilter.h"
#include "Personality.h"
#include "Phrase.h"

#include <list>
#include <memory>

class ConditionsStore;
class DataNode;
class FormationPattern;
class Government;
class Ship;
class System;



// A unique individual who may appear at random times in the game.
class Person {
public:
	void Load(const DataNode &node, const ConditionsStore *playerConditions,
		const std::set<const System *> *visitedSystems, const std::set<const Planet *> *visitedPlanets);
	// Finish loading all the ships in this person specification.
	void FinishLoading();
	bool IsLoaded() const;
	// Prevent this person from being spawned in any system.
	void NeverSpawn();

	// Find out how often this person should appear in the given system. If this
	// person is dead or already active, this will return zero.
	int Frequency(const System *system) const;

	// Get the person's characteristics. The ship object is persistent, i.e. it
	// will be recycled every time this person appears.
	const std::list<std::shared_ptr<Ship>> &Ships() const;
	const Government *GetGovernment() const;
	const Personality &GetPersonality() const;
	const Phrase &GetHail() const;
	// Check if a person has been destroyed or captured.
	bool IsDestroyed() const;
	// Mark this person as destroyed.
	void Destroy();
	// Mark this person as no longer destroyed.
	void Restore();
	// Check if a person is already placed somewhere.
	bool IsPlaced() const;
	// Mark this person as being no longer "placed" somewhere.
	void ClearPlacement();


private:
	bool isLoaded = false;
	LocationFilter location;
	int frequency = 100;

	std::list<std::shared_ptr<Ship>> ships;
	const FormationPattern *formationPattern = nullptr;
	const Government *government = nullptr;
	Personality personality;
	Phrase hail;
};
