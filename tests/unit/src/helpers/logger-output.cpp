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
		// There is one bar after the timestamp, and another after the message level.
		size_t pos = line.find('|');
		if(pos != std::string::npos)
		{
			pos = line.find('|', pos);
			if(pos != std::string::npos)
			{
				// pos is the index of the second bar, after which there's a space, so add 2.
				if(line.size() > pos + 2)
					line = line.substr(pos + 2);
				else
					line.clear();
			}
		}
		formattedOutput += line + '\n';
	}
	return formattedOutput;
}
