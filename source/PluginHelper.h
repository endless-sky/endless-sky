/* PluginHelper.h
Copyright (c) 2023 by RisingLeaf(https://github.com/RisingLeaf)

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <string>



// This namespace holds functions for installing and updating plugins.
namespace PluginHelper
{
	// Download any file.
	bool Download(std::string url, std::string location);
	// Extract a plugin from a zip file.
	// Could be used for other zips as well, but is tailored to plugins.
	bool ExtractZIP(std::string filename, std::string destination, std::string expectedName);
}
