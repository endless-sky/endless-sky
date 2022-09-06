/* DataObjectsLoader.h
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef DATA_OBJECTS_LOADER_H_
#define DATA_OBJECTS_LOADER_H_

#include "InterfaceObjects.h"
#include "UniverseObjects.h"

#include <future>



// This class contains the loader for serialized game data. The serialized
// game data contains various types of intermixed data (interfaces, outfits,
// ships, missions) so the loader does need to load to different Object stores.
class DataObjectsLoader{
public:
	DataObjectsLoader(UniverseObjects &universe, InterfaceObjects &interfaces);

	// Load game objects from the given directories of definitions.
	std::future<void> Load(const std::vector<std::string> &sources, bool debugMode = false);
	// Determine the fraction of data files read from disk.
	double GetProgress() const;
	// Resolve every game object dependency.
	void FinishLoading();


private:
	void LoadFile(const std::string &path, bool debugMode = false);


private:
	// A value in [0, 1] representing how many source files have been processed for content.
	std::atomic<double> progress;


private:
	// References to the objects we are loading into;
	UniverseObjects &universe;
	InterfaceObjects &interfaces;
};

#endif
