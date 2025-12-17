/* logger-output.cpp
Copyright (c) 2025 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "logger-output.h"

#include <sstream>



std::string IgnoreLogHeaders(std::string output)
{
	std::string formattedOutput;
	std::istringstream stream{output};
	std::string line;
	while(std::getline(stream, line))
	{
		// Completely skip the session markers.
		if(line.find("Logger session") != std::string::npos)
			continue;
		size_t pos = line.find("| W |");
		if(pos == std::string::npos)
			pos = line.find("| I |");
		if(pos == std::string::npos)
			pos = line.find("| E |");
		if(pos != std::string::npos)
			line = line.substr(pos + 6);
		formattedOutput += line + '\n';
	}
	return formattedOutput;
}
