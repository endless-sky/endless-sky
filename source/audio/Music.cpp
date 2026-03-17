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
#include "supplier/FlacSupplier.h"
#include "../text/Format.h"
#include "supplier/Mp3Supplier.h"


#include <map>

using namespace std;

namespace {
	enum class MusicFileType {
		MP3, FLAC
	};

	map<string, pair<filesystem::path, MusicFileType>> paths;
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
			string name = (path.parent_path() / path.stem()).lexically_relative(root).generic_string();
			string extension = Format::LowerCase(path.extension().string());
			if(extension == ".mp3")
				paths[name] = {path, MusicFileType::MP3};
			else if(extension == ".flac")
				paths[name] = {path, MusicFileType::FLAC};
		}
	}
}



unique_ptr<AudioSupplier> Music::CreateSupplier(const string &name, bool looping)
{
	if(paths.contains(name))
		switch(paths[name].second)
		{
			case MusicFileType::MP3:
				return unique_ptr<AudioSupplier>{
					new Mp3Supplier{Files::Open(paths[name].first), looping}};
			case MusicFileType::FLAC:
				return unique_ptr<AudioSupplier>{
					new FlacSupplier{Files::Open(paths[name].first), looping}};
		}
	return {};
}
