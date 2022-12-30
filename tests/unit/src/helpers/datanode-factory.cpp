/* datanode-factory.cpp
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

#include "datanode-factory.h"

#include "../../../../source/ConditionsStore.h"
#include "../../../../source/DataFile.h"
#include "../../../../source/DataNode.h"

#include <memory>
#include <sstream>
#include <string>
#include <vector>



// Method to convert text input into consumable DataNodes.
std::vector<DataNode> AsDataNodes(std::string text, std::shared_ptr<ConditionsStore> vars)
{
	std::stringstream in;
	in.str(text);
	const auto file = DataFile(in, vars);

	return std::vector<DataNode>{std::begin(file), std::end(file)};
}
// Convert the text to a list of nodes, and return the first node.
const DataNode AsDataNode(std::string text, std::shared_ptr<ConditionsStore> vars)
{
	auto nodes = AsDataNodes(std::move(text), vars);
	return nodes.empty() ? DataNode(vars) : *nodes.begin();
}
