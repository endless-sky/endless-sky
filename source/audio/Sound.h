/* Sound.h
Copyright (c) 2014 by Michael Zahniser

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

#include "supplier/AudioSupplier.h"

#include <filesystem>
#include <memory>
#include <string>



// This is a sound that can be played. The sound's file name will determine
// whether it is looping (ends in '~') or not.
class Sound {
public:
	bool Load(const std::filesystem::path &path, const std::string &name);

	const std::string &Name() const;

	const std::vector<AudioSupplier::sample_t> &Buffer() const;
	const std::vector<AudioSupplier::sample_t> &Buffer3x() const;
	bool IsLooping() const;

	std::unique_ptr<AudioSupplier> CreateSupplier() const;


private:
	std::string name;
	std::vector<AudioSupplier::sample_t> buffer;
	std::vector<AudioSupplier::sample_t> buffer3x;
	bool isLooped = false;
};
