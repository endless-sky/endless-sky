/* com
$ com <low> <high> <bin count>+

Distributions between 2 and 5 bins.
Some higher base.
Some more even, some more lopsided.

Basic principle: 1 jump * 1 T = 100 c.
Jobs should be based on that, too.

Need:
	2 even - food
	3 even - clothing
	3 skewed - metal
	4 even - plastic
	4 skewed - medical
	4 even high - industrial
	4 skewed high - equipment
	5 even - electronics
	5 skewed - luxury goods
	5 even high - rare earth
	5 skewed high - heavy metals

Food	2	100
Clothing
Metal	3
Plastic
Industrial
Equipment
Medical
Electronics
Luxury
Heavy Metal
Rare Earth
*/

#include "DataFile.h"
#include "Star.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <set>
#include <algorithm>
#include <fstream>

using namespace std;

class Value {
public:
	int minBin;
	int maxBin;
	int bin;
};

void PrintUsage();



int main(int argc, char *argv[])
{
	// Check that an input and output were specified.
	if(argc != 3)
	{
		PrintUsage();
		return 1;
	}
	// Return a different result each time.
	srand(time(NULL));
	
	// Load the starmap that we will be modifying.
	DataFile data(argv[1]);
	map<string, Star> stars;
	list<DataFile::Node> unrecognized;
	
	for(const DataFile::Node &node : data)
	{
		if(node.Size() >= 2 && node.Token(0) == "system")
			stars[node.Token(1)] = Star(node);
		else if(node.Size())
			unrecognized.push_back(node);
	}
	
	vector<string> starNames;
	int starTotal = 0;
	for(const auto &it : stars)
	{
		starNames.push_back(it.first);
		++starTotal;
	}
	
	// Load the commands.
	vector<int> binQuota;
	int binTotal = 0;
	double baseValue;
	string commodity;
	
	DataFile commands(cin);
	for(const DataFile::Node &node : commands)
	{
		if(node.Token(0) == "name")
		{
			assert(node.Size() >= 2);
			commodity = node.Token(1);
		}
		else if(node.Token(0) == "base")
		{
			assert(node.Size() >= 2);
			baseValue = node.Value(1);
		}
		else if(node.Token(0) == "bins")
		{
			assert(node.Size() >= 2);
			for(int i = 1; i < node.Size(); ++i)
			{
				binQuota.push_back(node.Value(i));
				binTotal += binQuota.back();
			}
		}
	}
	
	// Print the parameters we're working with.
	cerr << "Assigning " << binTotal << " levels to " << starTotal << " stars." << endl;
	if(starTotal > binTotal)
	{
		cerr << "Error: the number of stars is more than the total of the bin quotas." << endl;
		return 1;
	}
	
	// Look for an arrangement that works.
	int highBin = binQuota.size();
	map<string, Value> values;
	while(true)
	{
		// We have not assigned any values yet. So, we have our full quota
		// remaining, and each star can be assigned to any bin.
		vector<int> bin = binQuota;
		for(auto &star : values)
		{
			star.second.minBin = 0;
			star.second.maxBin = highBin;
		}
		
		// Keep track of which stars haven't been assigned values yet.
		vector<string> unassigned = starNames;
		while(unassigned.size())
		{
			// Pick a random star to assign a value to.
			int i = rand() % unassigned.size();
			string name = unassigned[i];
			unassigned[i] = unassigned.back();
			unassigned.pop_back();
			
			// Find out how many items left in our quota could be assigned to
			// this particular star.
			int possibilities = 0;
			for(int i = values[name].minBin; i < values[name].maxBin; ++i)
				possibilities += bin[i];
			if(!possibilities)
				break;
			
			// Pick a random one of those items to assign to it.
			int index = rand() % possibilities;
			int choice = values[name].minBin;
			while(true)
			{
				index -= bin[choice];
				if(index < 0)
					break;
				++choice;
			}
			--bin[choice];
			
			// Record our choice.
			values[name].bin = choice;
			int minBin = choice;
			int maxBin = choice + 1;
			
			// Starting from this star, trace outwards system by system. Each
			// neighboring system must be within 1 of this star's level; each
			// system neighboring those, within 2, and so on.
			vector<string> source = {name};
			set<string> done = {name};
			while(minBin > 0 || maxBin < highBin)
			{
				// Widen the min and max unless they are at their widest.
				if(minBin > 0)
					--minBin;
				if(maxBin < highBin)
					++maxBin;
				
				vector<string> next;
				
				// Update the min and max for each unvisited neighbor.
				for(const string &sourceName : source)
					for(const string &name : stars[sourceName].Links())
					{
						if(done.find(name) != done.end())
							continue;
						done.insert(name);
						
						values[name].minBin = max(values[name].minBin, minBin);
						values[name].maxBin = min(values[name].maxBin, maxBin);
						next.push_back(name);
					}
				
				// Now, visit neighbors of those neighbors.
				next.swap(source);
			}
		}
		// If there were not any stars that we could not assign values to, we
		// are done. Especially if the constraints are rather tight, it may
		// take quite a few iterations to find an acceptable solution. Or, it
		// may be outright impossible, in qhich case the program hangs.
		if(!unassigned.size())
			break;
	}
	
	// Assign each star system a value based on its bin.
	map<string, int> rough;
	for(auto &it : stars)
		rough[it.first] = baseValue + (rand() % 100) + 100 * values[it.first].bin;
	
	/*for(auto &it : stars)
		it.second.SetTrade(commodity,
			baseValue + (rand() % 100) + 100 * values[it.first].bin);*/
	for(auto &it : stars)
	{
		int count = 0;
		int sum = 0;
		for(const string &link : it.second.Links())
		{
			sum += rough[link];
			++count;
		}
		sum += count * rough[it.first];
		sum = (sum + count) / (2 * count);
		it.second.SetTrade(commodity, sum);
	}
	
	// Write out an SVG showing the levels we assigned.
	cout << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1000px\" height=\"1200px\">\n";
	cout << "<rect width=\"1000\" height=\"1200\" fill=\"white\"/>\n";
	// First, draw the connections between the stars.
	for(const auto &it : stars)
	{
		int x = it.second.X() + 950;
		int y = it.second.Y() + 600;
		for(const string &name : it.second.Links())
			cout << "<line x1=\"" << x << "\" y1=\"" << y
				<< "\" x2=\"" << stars[name].X() + 950
				<< "\" y2=\"" << stars[name].Y() + 600 << "\" stroke=\"#CCCCCC\" />\n";
	}
	// Then, draw the stars, color-coded.
	double scale = 50. * highBin;
	double mean = baseValue + scale;
	for(const auto &it : stars)
	{
		static const char hex[16] = {
			'0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
		double level = (it.second.Trade(commodity) - mean) / scale;
		level = min(1., max(-1., level));
		int red = (level > 0) ? 255 : static_cast<int>(255.99 * (1. + level));
		int blue = (level < 0) ? 255 : static_cast<int>(255.99 * (1. - level));
		int green = min(red, blue);
		
		int x = it.second.X() + 950;
		int y = it.second.Y() + 600;
		cout << "<circle cx=\"" << x << "\" cy=\"" << y
			<< "\" r=\"5\" stroke=\"black\" fill=\"#";
		cout.put(hex[red / 16]);
		cout.put(hex[red % 16]);
		cout.put(hex[green / 16]);
		cout.put(hex[green % 16]);
		cout.put(hex[blue / 16]);
		cout.put(hex[blue % 16]);
		cout << "\" />\n";
		
		cout << "<text x=\"" << x + 8 << "\" y=\"" << y + 4
			<< "\" font-family=\"Ubuntu\" font-size=\"10\" fill=\"black\">"
			<< it.first << "</text>\n";
	}
	
	cout << "<path style=\"fill:none;stroke:#0000ff;stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\""
		<< " d=\"M 906.35593,301.27119 C 607.25611,484.28311 396.30678,1011.9828 973.72882,1028.3898\" />\n";
	cout << "<path style=\"fill:none;stroke:#00c800;stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\""
		<< " d=\"M 609.62712,789.40678 C 69.485592,1082.0502 343.51084,769.43623 18.525424,535.16949\" />\n";
	cout << "<path style=\"fill:none;stroke:#8cc88c;stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\""
		<< " d=\"M -6.3559322,544.0678 C 131.10352,583.39281 222.1381,915.04122 294.91526,963.55932 c 114.04828,76.03218 160.64636,61.21148 233.8983,199.57628\" />\n";
	cout << "<path style=\"fill:none;stroke:#a0a0ff;stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\""
		<< " d=\"m 944.49153,683.89831 c -378.6202,-37.19077 -441.72604,323.92679 5.08474,319.06779\" />\n";
	cout << "<path style=\"fill:none;stroke:#b4b400;stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\""
		<< " d=\"M 286.01695,141.10169 C 2.2718848,154.0468 219.57354,537.81252 388.98305,274.57627 439.58115,195.95463 346.29778,138.35154 286.01695,141.10169 z\" />\n";
	
	cout << "</svg>\n";
	
	// Write out the data file.
	ofstream out(argv[2]);
	for(const auto &it : stars)
		it.second.Write(out);
	for(const auto &it : unrecognized)
		it.Write(out);
	
	return 0;
}



void PrintUsage()
{
	cerr << endl;
	cerr << "Usage: $ commerce <in> <out>" << endl;
	cerr << endl;
	cerr << "Followed by the following commands from STDIN, in any order:" << endl;
	cerr << "name <name of commodity>" << endl;
	cerr << "base <lowest possible price>" << endl;
	cerr << "bins <bin size>+" << endl;
	cerr << endl;
}
