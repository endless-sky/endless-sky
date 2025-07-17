/* Music.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Music.h"

#include "../Files.h"
#include "../text/Format.h"
#include "supplier/Mp3Supplier.h"


#include <map>

using namespace std;

namespace {
	map<string, filesystem::path> paths;
}



void Music::Init(const vector<filesystem::path> &sources)
{
	for(const auto &source : sources)
	{
		// Find all the sound files that this resource source provides.
		filesystem::path root = source / "sounds";
		vector<filesystem::path> files = Files::RecursiveList(root);

		for(const auto &path : files)
		{
			// Sanity check on the path length.
			if(Format::LowerCase(path.extension().string()) != ".mp3")
				continue;

			string name = (path.parent_path() / path.stem()).lexically_relative(root).generic_string();
			paths[name] = path;
		}
	}
}



unique_ptr<AudioSupplier> Music::CreateSupplier(const string &name, bool looping)
{
	if(paths.contains(name))
		return unique_ptr<AudioSupplier>{
				new Mp3Supplier{Files::Open(paths[name]), looping}};
	return {};
}
