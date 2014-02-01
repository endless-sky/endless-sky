/* Trade.h
Michael Zahniser, 15 Dec 2013

Class representing all the commodities that are available to trade.
*/

#ifndef TRADE_H_INCLUDED
#define TRADE_H_INCLUDED

#include "DataFile.h"

#include <vector>
#include <string>



class Trade {
public:
	class Commodity {
	public:
		std::string name;
		int low;
		int high;
	};
	
	
public:
	void Load(const DataFile::Node &node);
	
	const std::vector<Commodity> &Commodities() const;
	
	
private:
	std::vector<Commodity> commodities;
};



#endif
