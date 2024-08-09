/* datanode-factory.h
Copyright (c) 2021 by Benjamin Hauch

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

#include "../../../source/DataNode.h"

#include <string>
#include <vector>



// Method to convert text input into consumable DataNodes.
std::vector<DataNode> AsDataNodes(const std::string &text);
// Convert the text to a list of nodes, and return the first node.
const DataNode AsDataNode(std::string text);
