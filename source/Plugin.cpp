/* Plugin.cpp
Copyright (c) 2022 by Sam Gleske (samrocketman on GitHub)

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Plugin.h"

#include "Logger.h"

using namespace std;



// Checks if there are any dependencies of any kind.
bool Plugin::PluginDependencies::IsEmpty() const
{
	return required.empty() && optional.empty() && conflicted.empty();
}



// Checks if there are any duplicate dependencies. E.g. the same dependency in both required and conflicted.
bool Plugin::PluginDependencies::IsValid() const
{
	// We will check every dependency before returning to allow the
	// plugin developer to see all errors and not just the first.
	bool isValid = true;

	string dependencyCollisions;

	// Required dependencies will already be valid due to sets not
	// allowing duplicate values. Therefore we only need to check optional
	// and conflicts.

	// Check and log collisions between optional and required dependencies.
	for(const string &dependency : optional)
	{
		if(required.contains(dependency))
			dependencyCollisions += dependency + ", ";
	}
	if(!dependencyCollisions.empty())
	{
		dependencyCollisions.pop_back();
		dependencyCollisions.pop_back();
		Logger::Log("Dependencies named " + dependencyCollisions
			+ " were found in both the required dependencies list and the optional dependencies list.",
			Logger::Level::WARNING);
		dependencyCollisions.clear();
	}

	// Check and log collisions between conflicted and required dependencies.
	for(const string &dependency : conflicted)
	{
		if(required.contains(dependency))
		{
			isValid = false;
			dependencyCollisions += dependency + ", ";
		}
	}
	if(!dependencyCollisions.empty())
	{
		dependencyCollisions.pop_back();
		dependencyCollisions.pop_back();
		Logger::Log("Dependencies named " + dependencyCollisions
			+ " were found in both the conflicting dependencies list and the required dependencies list.",
			Logger::Level::WARNING);
		dependencyCollisions.clear();
	}

	// Check and log collisions between optional and conflicted dependencies.
	for(const string &dependency : conflicted)
	{
		if(optional.contains(dependency))
		{
			isValid = false;
			dependencyCollisions += dependency + ", ";
		}
	}
	if(!dependencyCollisions.empty())
	{
		dependencyCollisions.pop_back();
		dependencyCollisions.pop_back();
		Logger::Log("Dependencies named " + dependencyCollisions
			+ " were found in both the optional dependencies list and the conflicting dependencies list.",
			Logger::Level::WARNING);
		dependencyCollisions.clear();
	}

	return isValid;
}



// Constructs a description of the plugin from its name, tags, dependencies, etc.
string Plugin::CreateDescription() const
{
	string text;
	if(!version.empty())
		text += "Version: " + version + '\n';
	if(!authors.empty())
	{
		text += "Authors: ";
		for(const string &author : authors)
			text += author + ", ";
		text.pop_back();
		text.pop_back();
		text += '\n';
	}
	if(!tags.empty())
	{
		text += "Tags: ";
		for(const string &tag : tags)
			text += tag + ", ";
		text.pop_back();
		text.pop_back();
		text += '\n';
	}
	if(!dependencies.IsEmpty())
	{
		text += "Dependencies:\n";
		if(!dependencies.gameVersion.empty())
			text += "  Game Version: " + dependencies.gameVersion + '\n';
		if(!dependencies.required.empty())
		{
			text += "  Requires:\n";
			for(const string &dependency : dependencies.required)
				text += "  - " + dependency + '\n';
		}
		if(!dependencies.optional.empty())
		{
			text += "  Optional:\n";
			for(const string &dependency : dependencies.optional)
				text += "  - " + dependency + '\n';
		}
		if(!dependencies.conflicted.empty())
		{
			text += "  Conflicts:\n";
			for(const string &dependency : dependencies.conflicted)
				text += "  - " + dependency + '\n';
		}
		text += '\n';
	}
	if(!aboutText.empty())
		text += aboutText;

	return text;
}



// Checks whether this plugin is valid, i.e. whether it exists.
bool Plugin::IsValid() const
{
	return !name.empty();
}
