/* Confusion.h
Copyright (c) 2026 by Anarchist2

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

#include <string>

class DataNode;
class DataWriter;

class Confusion {
public:
	Confusion() = default;
	// Construct and Load() at the same time.
	Confusion(const DataNode &node);

	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	const std::string &Name() const;

	bool IsDefined() const;

	// Randomize the initial confusion period of a ship.
	void RandomizePeriod();

	// Get and update the current aiming offset of a ship.
	const double CurrentConfusion() const;
	void UpdateConfusion(bool isFocusing);


private:
	bool isDefined = false;
	std::string name;

	double confusionMultiplier = 10.;
	double period = 240.;
	double focusMultiplier = .1;
	double gainFocusTime = 600.;
	double loseFocusTime = 120.;

	// Variables used for tracking a ship's current confusion.
	int tick = 0;
	double focusPercentage = 0.;
	double confusion;
};
