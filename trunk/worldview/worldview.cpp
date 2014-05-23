/* worldview
Program to view all planet graphics and descriptions.
*/

#include "DataFile.h"

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <limits>
#include <algorithm>
#include <fstream>

using namespace std;

namespace {
	double minX = numeric_limits<double>::infinity();
	double minY = numeric_limits<double>::infinity();
	double maxX = -numeric_limits<double>::infinity();
	double maxY = -numeric_limits<double>::infinity();
	
	map<string, int> uses;
}

class Commodity {
public:
	string name;
	int low;
	int high;
};

static const vector<Commodity> commodities = {
	{"Food", 100, 600},
	{"Clothing", 140, 440},
	{"Metal", 190, 590},
	{"Plastic", 240, 540},
	{"Equipment", 330, 730},
	{"Medical", 430, 930},
	{"Industrial", 520, 920},
	{"Electronics", 590, 890},
	{"Heavy Metals", 610, 1310},
	{"Luxury Goods", 920, 1520}
};

class System {
public:
	void Load(const DataFile::Node &node);
	
	const DataFile::Node *root;
	double x;
	double y;
	string government;
	map<string, double> trade;
	vector<string> stars;
	vector<pair<string, string>> planets;
	vector<string> links;
};

class Planet {
public:
	void Load(const DataFile::Node &node);
	
	string landscape;
	string description;
	string spaceport;
	vector<string> shipyard;
	vector<string> outfitter;
};

double MaxDistance(const DataFile::Node &node, double d = 1.);
void Draw(const DataFile::Node &node, double x, double y, double scale, const string &name);



int main(int argc, char *argv[])
{
	if(argc < 2)
		return 1;
	
	DataFile file(argv[1]);
	
	map<string, System> systems;
	map<string, Planet> planets;
	for(const DataFile::Node &node : file)
	{
		if(node.Token(0) == "system" && node.Size() >= 2)
			systems[node.Token(1)].Load(node);
		else if(node.Token(0) == "planet" && node.Size() >= 2)
			planets[node.Token(1)].Load(node);
	}
	
	// Draw all systems:
	ofstream mapFile("map.svg");
	mapFile << "<svg width=\"240\" height=\"240\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n";
	double radius = 120.;
	double scale = 2. * (radius - 1.) / max(maxX - minX, maxY - minY);
	double centerX = (minX + maxX) / 2.;
	double centerY = (minY + maxY) / 2.;
	for(const pair<string, System> &it : systems)
	{
		const System &system = it.second;
		
		double x1 = (system.x - centerX) * scale + radius;
		double y1 = (system.y - centerY) * scale + radius;
		for(const string &link : system.links)
		{
			// Only draw links in one direction.
			if(link <= it.first)
				continue;
			
			map<string, System>::iterator lit = systems.find(link);
			if(lit == systems.end())
				continue;
			
			double x2 = (lit->second.x - centerX) * scale + radius;
			double y2 = (lit->second.y - centerY) * scale + radius;
			
			mapFile << "\n<line x1=\"" << x1 << "\" y1=\"" << y1
				<< "\" x2=\"" << x2 << "\" y2=\"" << y2
				<< "\" stroke=\"#333\" stroke-width=\"1.4\"/>";
		}
	}
	mapFile << "\n</svg>\n";
	mapFile.close();
	
	cout << "<html><head><title>World Viewer</title>" << endl;
	cout << "<style>p {margin-top: 0.2em; margin-bottom: 0.2em; line-height: 150%;}</style></head>" << endl;
	cout << "<body style=\"background-color:black; color:white;\"><table>" << endl;
	for(const pair<string, System> &it : systems)
	{
		size_t count = it.second.planets.size();
		if(!count)
			continue;
		
		cout << "<tr><td align=\"center\" valign=\"top\" rowspan=\"" << count
			<< "\">" << it.first;
		for(const string &star : it.second.stars)
			cout << "<br/><img src=\"../images/" << star << ".png\">";
		cout << "<p>Government: " << it.second.government << "</p>";
		
		// Draw system location:
		double x = (it.second.x - centerX) * scale + radius;
		double y = (it.second.y - centerY) * scale + radius;
		
		cout << "<svg width=\"240\" height=\"240\">";
		cout << "<image x=\"0\" y=\"0\" width=\"240\" height=\"240\" xlink:href=\"map.svg\"/>";
		cout << "<circle cx=\"" << x << "\" cy=\"" << y
			<< "\" r=\"2\" fill=\"#FC3\" stroke=\"none\"/>";
		cout << "</svg><br/>\n";
		
		cout << "<table>";
		for(const Commodity &commodity : commodities)
		{
			auto cit = it.second.trade.find(commodity.name);
			if(cit == it.second.trade.end())
				continue;
			
			int price = cit->second;
			int third = (commodity.high - commodity.low) / 3;
			string color = (price < commodity.low + third) ? "#6699FF"
				: (price > commodity.high - third) ? "#FF6666" : "white";
			cout << "<tr style=\"color:" << color << ";\"><td align=\"left\">"
				<< commodity.name << "</td><td>" << price << "</td></tr>";
		}
		cout << "</table><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p></td>" << endl;
		
		bool first = true;
		for(const pair<string, string> &planet : it.second.planets)
		{
			if(!first)
				cout << "<tr>";
			cout << "<td valign=\"top\" align=\"center\">" << planet.first;
			cout << "<br/><img src=\"../images/" << planet.second << ".png\">\n";
			
			const Planet &data = planets[planet.first];
			cout << "<p style=\"color:#666\">(" << uses[planet.second] << " / "
				<< uses[data.landscape] << " uses.</p>";
			
			// Draw the star system, with this planet highlighted.
			double distance = MaxDistance(*it.second.root);
			double scale = min(.03, 116. / distance);
	
			cout << "<svg width=\"240\" height=\"240\">";
			Draw(*it.second.root, 120., 120., scale, planet.first);
			cout << "</svg>\n";
			
			if(!data.shipyard.empty())
			{
				cout << "<p>Shipyard:</p>" << endl;
				for(const string &name : data.shipyard)
					cout << "<p>" << name << "</p>";
				cout << "<p>&nbsp;</p>";
			}
			if(!data.outfitter.empty())
			{
				cout << "<p>Outfitter:</p>" << endl;
				for(const string &name : data.outfitter)
					cout << "<p>" << name << "</p>";
				cout << "<p>&nbsp;</p>";
			}
			cout << "</td>" << endl;
			cout << "<td width=\"720\"><img src=\"../images/" 
				<< data.landscape
				<< ".jpg\">"
				<< data.description << "<hr/>";
			if(data.spaceport.empty())
				cout << "<p>YOU CANNOT REFUEL HERE.</p>";
			else
				cout << data.spaceport;
			cout << "<p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p>";
			cout << "</td>";
			if(!first)
				cout << "</tr>";
			cout << endl;
			
			first = false;
		}
		cout << "</tr>" << endl;
	}
	cout << "</table></body></html>" << endl;
}



void System::Load(const DataFile::Node &node)
{
	if(node.Token(0) == "system")
		root = &node;
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "object")
		{
			if(child.Size() >= 2)
			{
				pair<string, string> planet;
				planet.first = child.Token(1);
				for(const DataFile::Node &grand : child)
					if(grand.Token(0) == "sprite" && grand.Size() >= 2)
						planet.second = grand.Token(1);
				planets.push_back(planet);
			}
			// Recurse into 			
			Load(child);
		}
		else if(child.Token(0) == "sprite" && child.Size() >= 2)
		{
			++uses[child.Token(1)];
			if(!child.Token(1).compare(0, 5, "star/", 0, 5))
				stars.push_back(child.Token(1));
		}
		else if(child.Token(0) == "government" && child.Size() >= 2)
			government = child.Token(1);
		else if(child.Token(0) == "link" && child.Size() >= 2)
			links.push_back(child.Token(1));
		else if(child.Token(0) == "trade" && child.Size() >= 3)
			trade[child.Token(1)] = child.Value(2);
		else if(child.Token(0) == "pos" && child.Size() >= 3)
		{
			x = child.Value(1);
			minX = min(minX, x);
			maxX = max(maxX, x);
			
			y = child.Value(2);
			minY = min(minY, y);
			maxY = max(maxY, y);
		}
	}
}



void Planet::Load(const DataFile::Node &node)
{
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "landscape" && child.Size() >= 2)
		{
			++uses[child.Token(1)];
			landscape = child.Token(1);
		}
		else if(child.Token(0) == "shipyard" && child.Size() >= 2)
			shipyard.push_back(child.Token(1));
		else if(child.Token(0) == "outfitter" && child.Size() >= 2)
			outfitter.push_back(child.Token(1));
		else if(child.Token(0) == "description" || child.Token(0) == "spaceport" && child.Size() >= 2)
		{
			string text = "<p>" + child.Token(1) + "</p>";
			while(true)
			{
				size_t pos = text.find('\t');
				if(pos == string::npos)
					break;
				text.replace(pos, 1, "&nbsp;&nbsp;&nbsp;&nbsp;");
			}
			(child.Token(0) == "description" ? description : spaceport) += text;
		}
	}
}



double MaxDistance(const DataFile::Node &node, double d)
{
	double maximum = d;
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "object")
		{
			double thisD = 0.;
			for(const DataFile::Node &grand : child)
				if(grand.Token(0) == "distance")
					thisD = grand.Value(1);
			thisD = MaxDistance(child, d + thisD);
			if(thisD > maximum)
				maximum = thisD;
		}
	}
	return maximum;
}



void Draw(const DataFile::Node &node, double x, double y, double scale, const string &name)
{
	if(node.Token(0) == "object" && node.Size() >= 2 && node.Token(1) == name)
	{
		cout << "<circle cx=\"" << x << "\" cy=\"" << y
			<< "\" r=\"2\" fill=\"#39F\" stroke=\"none\"/>";
	}
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "object")
		{
			double distance = 0.;
			double period = 0.;
			double offset = 0.;
			for(const DataFile::Node &grand : child)
			{
				if(grand.Token(0) == "distance")
					distance = grand.Value(1);
				else if(grand.Token(0) == "period")
					period = grand.Value(1);
				else if(grand.Token(0) == "offset")
					offset = grand.Value(1);
			}
			
			distance *= scale;
			distance += 1.;
			cout << "<circle cx=\"" << x << "\" cy=\"" << y << "\" r=\"" << distance
				<< "\" stroke=\"#333\" stroke-width=\"1.4\" fill=\"none\"/>";
			
			double angle = (offset + 100000. / period) * 0.017453293;
			Draw(child, x + distance * sin(angle), y + distance * cos(angle), scale, name);
		}
	}
}
