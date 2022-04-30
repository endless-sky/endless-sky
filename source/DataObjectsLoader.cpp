/* DataObjectsLoader.cpp
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "DataObjectsLoader.h"

#include "DataFile.h"
#include "DataNode.h"
#include "Files.h"

using namespace std;



DataObjectsLoader::DataObjectsLoader(UniverseObjects &universe, InterfaceObjects &interfaces) :
	universe(universe), interfaces(interfaces)
{
}



future<void> DataObjectsLoader::Load(const vector<string> &sources, bool debugMode)
{
	progress = 0.;

	// We need to copy any variables used for loading to avoid a race condition.
	// 'this' is not copied, so 'this' shouldn't be accessed after calling this
	// function (except for calling GetProgress which is safe due to the atomic).
	return async(launch::async, [this, sources, debugMode]() noexcept -> void
		{
			vector<string> files;
			for(const string &source : sources)
			{
				// Iterate through the paths starting with the last directory given. That
				// is, things in folders near the start of the path have the ability to
				// override things in folders later in the path.
				auto list = Files::RecursiveList(source + "data/");
				files.reserve(files.size() + list.size());
				files.insert(files.end(),
						make_move_iterator(list.begin()),
						make_move_iterator(list.end()));
			}

			const double step = 1. / (static_cast<int>(files.size()) + 1);
			for(const auto &path : files)
			{
				LoadFile(path, debugMode);

				// Increment the atomic progress by one step.
				// We use acquire + release to prevent any reordering.
				auto val = progress.load(memory_order_acquire);
				progress.store(val + step, memory_order_release);
			}
			FinishLoading();
			progress = 1.;
		});
}



double DataObjectsLoader::GetProgress() const
{
	return progress.load(memory_order_acquire);
}



void DataObjectsLoader::FinishLoading()
{
	universe.FinishLoading();
}



void DataObjectsLoader::LoadFile(const string &path, bool debugMode)
{
	// This is an ordinary file. Check to see if it is an image.
	if(path.length() < 4 || path.compare(path.length() - 4, 4, ".txt"))
		return;

	DataFile data(path);
	if(debugMode)
		Files::LogError("Parsing: " + path);

	for(const DataNode &node : data)
		if(!(universe.LoadNode(node, path) || interfaces.LoadNode(node)))
			node.PrintTrace("Skipping unrecognized root object:");
}
