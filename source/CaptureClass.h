/* CaptureClass.h
Copyright (c) 2023 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CAPTURE_CLASS_H_
#define CAPTURE_CLASS_H_

#include <string>

class DataNode;
class DataWriter;



// A capture class represents the success rate of an outfit in being used to capture
// a ship, or the ship's success rate in defending against a capture attempt. The
// outfit and ship must share a capture class of the same name for the outfit to be
// used on the ship. Capture attempts can also result in the ship self destructing or
// locking down, or breaking the outfit used.
class CaptureClass {
public:
	CaptureClass(const DataNode &node);
	void Save(DataWriter &out) const;

	const std::string &Name() const;
	double SuccessChance() const;
	double SelfDestructChance() const;
	double LockDownChance() const;
	double BreakOnSuccessChance() const;
	double BreakOnFailureChance() const;


private:
	std::string name;
	double successChance = 0.;
	double selfDestructChance = 0.;
	double lockDownChance = 0.;
	double breakOnSuccessChance = 0.;
	double breakOnFailureChance = 0.;
};



#endif
