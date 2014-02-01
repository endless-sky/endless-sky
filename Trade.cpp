/* Trade.cpp
Michael Zahniser, 15 Dec 2013

Function definitions for the Trade class.
*/

#include "Trade.h"

#include <cassert>

using namespace std;



void Trade::Load(const DataFile::Node &node)
{
	assert(node.Token(0) == "trade");
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "commodity" && child.Size() >= 4)
		{
			commodities.push_back(Commodity());
			commodities.back().name = child.Token(1);
			commodities.back().low = child.Value(2);
			commodities.back().high = child.Value(3);
		}
	}
}



const vector<Trade::Commodity> &Trade::Commodities() const
{
	return commodities;
}
