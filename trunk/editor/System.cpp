/* System.cpp
Michael Zahniser, 26 Dec 2013

Function definitions for the System class.
*/

#include "System.h"

#include "DirIt.h"
#include "Sprite.h"
#include "SpriteSet.h"

#include <algorithm>
#include <cassert>
#include <vector>
#include <cmath>
#include <set>

using namespace std;

namespace {
	vector<string> stars;
	vector<string> moons;
	vector<string> planets;
	vector<string> giants;
	
	map<string, int> tradeMin;
	map<string, int> tradeMax;
}



void System::Init()
{
	{
		DirIt it("../images/star/");
		while(true)
		{
			string path = it.Next();
			if(path.empty())
				break;
			if(path.length() < 4 || path.substr(path.length() - 4) != ".png")
				continue;
			
			path = path.substr(10);
			SpriteSet::Get(path);
			stars.push_back(path);
		}
	}
	vector<string> allPlanets;
	{
		DirIt it("../images/planet/");
		while(true)
		{
			string path = it.Next();
			if(path.empty())
				break;
			if(path.length() < 4 || path.substr(path.length() - 4) != ".png")
				continue;
			
			path = path.substr(10);
			SpriteSet::Get(path);
			allPlanets.push_back(path);
		}
	}
	SpriteSet::Finish();
	for(const string &path : allPlanets)
	{
		const Sprite *sprite = SpriteSet::Get(path);
		if(!sprite->Width())
			continue;
		
		if(sprite->Width() <= 110)
			moons.push_back(path);
		if(sprite->Width() >= 100 && sprite->Width() <= 250)
			planets.push_back(path);
		else
			giants.push_back(path);
	}
}



int System::TradeMin(const std::string &name)
{
	if(tradeMin.find(name) == tradeMin.end())
		return 0;
	
	return tradeMin[name];
}



int System::TradeMax(const std::string &name)
{
	if(tradeMax.find(name) == tradeMax.end())
		return 0;
	
	return tradeMax[name];
}



void System::Load(const DataFile::Node &node, const Set<System> &systems)
{
	assert(node.Size() >= 2 && node.Token(0) == "system");
	*this = System();
	name = node.Token(1);
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "pos")
		{
			assert(child.Size() >= 3);
			pos = Point(child.Value(1), child.Value(2));
		}
		else if(child.Token(0) == "link")
		{
			assert(child.Size() >= 2);
			links.push_back(systems.Get(child.Token(1)));
		}
		else if(child.Token(0) == "habitable")
			habitable = child.Value(1);
		else if(child.Token(0) == "trade")
		{
			trade[child.Token(1)] = child.Value(2);
			auto it = tradeMin.find(child.Token(1));
			if(it == tradeMin.end())
			{
				tradeMin[child.Token(1)] = child.Value(2);
				tradeMax[child.Token(1)] = child.Value(2);
			}
			else
			{
				tradeMin[child.Token(1)] = min(tradeMin[child.Token(1)],
					static_cast<int>(child.Value(2)));
				tradeMax[child.Token(1)] = max(tradeMax[child.Token(1)],
					static_cast<int>(child.Value(2)));
			}
			
			// Since we can't edit trade values here, save them in a block:
			unrecognized.push_back(child);
		}
		else if(child.Token(0) == "asteroids")
		{
			asteroids.push_back(Asteroids());
			asteroids.back().name = child.Token(1);
			asteroids.back().count = child.Value(2);
			asteroids.back().energy = child.Value(3);
		}
		
		else if(child.Token(0) == "object")
			LoadObject(root, child);
		else
			unrecognized.push_back(child);
	}
	
	if(asteroids.empty())
	{
		// Pick the total number of asteroids. Bias towards small numbers, with
		// a few systems with many more.
		int fullTotal = (rand() % 21) * (rand() % 21) + 1;
		double energy = (rand() % 21 + 10) * (rand() % 21 + 10) * .01;
		const string suffix[2] = {" rock", " metal"}; 
		const string prefix[3] = {"small", "medium", "large"};
		
		int total[2] = {rand() % fullTotal, 0};
		total[1] = fullTotal - total[0];
		
		for(int i = 0; i < 2; ++i)
		{
			if(!total[i])
				continue;
			
			int count[3] = {0, rand() % total[i], 0};
			int remaining = total[i] - count[1];
			if(remaining)
			{
				count[0] = rand() % remaining;
				count[2] = remaining - count[0];
			}
			
			for(int j = 0; j < 3; ++j)
				if(count[j])
				{
					asteroids.push_back(Asteroids());
					asteroids.back().name = prefix[j] + suffix[i];
					asteroids.back().count = count[j];
					asteroids.back().energy = energy * (rand() % 101 + 50) * .01;
				}
		}
	}
	
	if(root.sprite.empty() && root.children.empty())
	{
		if(name == "Sol")
			Sol();
		else
			Randomize();
	}
}



void System::LoadObject(Object &parent, const DataFile::Node &node)
{
	parent.children.push_back(Object());
	Object &object = parent.children.back();
	if(node.Size() >= 2)
		object.planet = node.Token(1);
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "sprite")
			object.sprite = child.Token(1);
		else if(child.Token(0) == "distance")
			object.distance = child.Value(1);
		else if(child.Token(0) == "period")
			object.period = child.Value(1);
		else if(child.Token(0) == "offset")
			object.offset = child.Value(1);
		else if(child.Token(0) == "object")
			LoadObject(object, child);
	}
}



int System::Trade(const std::string &name) const
{
	auto it = trade.find(name);
	if(it == trade.end())
		return 0;
	return it->second;
}



float System::TradeRange(const std::string &name) const
{
	int tmin = TradeMin(name);
	int tmax = TradeMax(name);
	if(!tmin || !tmax)
		return 0.f;
	
	float trange = (tmax - tmin) * .5f;
	float tmid = tmin + trange;
	
	return (Trade(name) - tmid) / trange;
}



System::Object::Object(double d, double p, const string &sprite, double o)
	: sprite(sprite), distance(d), period(p), offset(o)
{
}



void System::Randomize()
{
	bool feasible = false;
	while(!feasible)
	{
		root = System::Object();
		static const string starTypes[] = {
			"star/b5.png",
			"star/a0.png",
			"star/a5.png",
			"star/f0.png",
			"star/f5.png",
			"star/g0.png",
			"star/g5.png",
			"star/k0.png",
			"star/k5.png",
			"star/m0.png",
			"star/m4.png",
			"star/m8.png"};
		static const int probability[] = {
			1,
			1,
			2,
			3,
			8,
			12,
			14,
			17,
			16,
			12,
			9,
			5};
	
		// Decide whether this will be a binary star system.
		double mass = 0.;
		double occupied = 0.;
		if(rand() % 3)
		{
			// Single star system.
			int index = 0;
			for(int p = rand() % 100; p > 0; p -= probability[index])
				++index;
		
			const Sprite *sprite = SpriteSet::Get(starTypes[index]);
			double radius = (sprite->Width() - 30.) * .5;
			mass = radius * radius * radius;
			occupied = radius;
		
			root.children.push_back(System::Object(0., 10., starTypes[index]));
		}
		else
		{
			int firstIndex = 0;
			for(int p = rand() % 100; p > 0; p -= probability[firstIndex])
				++firstIndex;
			int secondIndex = 0;
			for(int p = rand() % 100; p > 0; p -= probability[secondIndex])
				++secondIndex;
		
			const Sprite *first = SpriteSet::Get(starTypes[firstIndex]);
			const Sprite *second = SpriteSet::Get(starTypes[secondIndex]);
		
			double firstR = (first->Width() - 30.) * .5;
			double secondR = (second->Width() - 30.) * .5;
		
			double firstM = firstR * firstR * firstR;
			double secondM = secondR * secondR * secondR;
			mass = firstM + secondM;
		
			double distance = firstR + secondR + (rand() % 40) + 40.;
			// m1 * d1 = m2 * d2
			// d1 + d2 = d;
			// m1 * d1 = m2 * (d - d1)
			// m1 * d1 = m2 * d - m2 * d1
			// (m1 + m2) * d1 = m2 * d
			double firstD = (secondM * distance) / mass;
			double secondD = (firstM * distance) / mass;
			occupied = max(firstD + firstR, secondD + secondR) + 50.;
		
			double period = sqrt((distance * distance * distance) / (mass * .25));
			if(firstD < secondD)
				root.children.push_back(System::Object(
					firstD, period, starTypes[firstIndex]));
			root.children.push_back(System::Object(
				secondD, period, starTypes[secondIndex], 180.));
			if(firstD >= secondD)
				root.children.push_back(System::Object(
					firstD, period, starTypes[firstIndex]));
		}
	
		// Earth orbits at a distance of 1000 from a star with mass 27000. So, if
		// the earth is at the center of its habitable zone:
		// 1000 = 30^3 / k; k = 30^3 / 1000.
		habitable = mass / 25.;
	
		// Note: say Earth orbits a g0 with mass 27,000. Its period must be 365.25.
		// Say I want it to orbit at a distance of 1000. What should the mass
		// multiplier be to get the proper orbital period?
		// sqrt(d^3 / (k * m)) = 365.25
		// d^3 / (k * m) = 365.25^2
		// k = d^3 / (365.25^2 * m) = .2776
		// Distance must be at least 90 + 2*60 + 2*60 + (50 + 30) = 410
	
		// Put this much space in between planets.
		int spacer = 100;
	
		set<string> used;
		while(occupied < 2000.)
		{
			int space = rand() % spacer;
			occupied += (space * space) * .01 + spacer / 2;
			spacer = (spacer * 3) / 2;
		
			// Pick a random planet. The farther out we are, the more likely it is
			// to be a gas giant.
			string spriteName;
			bool isTerrestrial = rand() % 2000 > occupied;
			do {
				if(isTerrestrial)
					spriteName = planets[rand() % planets.size()];
				else
					spriteName = giants[rand() % giants.size()];
			} while(used.find(spriteName) != used.end());
			used.insert(spriteName);
			
			const Sprite *sprite = SpriteSet::Get(spriteName);
		
			// Generate the planet and its moons.
			// Earth's moon orbits every 27.3 days. The mass of my earth sprite will
			// be 90^3. I'd ideally like the moon to be about 200 pixels away from
			// the center of the earth, so:
			// k = (d^3) / (27.3^2 * 90^3) =  
			System::Object planet(0., 0., spriteName);
			int moonCount = rand() % (isTerrestrial ? (rand() % 2 + 1) : (rand() % 3 + 3));
			// Don't let tiny planets have moons.
			if(sprite->Width() < 150)
				moonCount = 0;
		
			double radius = sprite->Width() * .5;
			double planetMass = radius * radius * radius * .015;
			int moonSpace = 50;
			for(int i = 0; i < moonCount; ++i)
			{
				radius += rand() % moonSpace;
				moonSpace += 20;
			
				string moonSpriteName;
				do {
					moonSpriteName = moons[rand() % moons.size()];
				} while(used.find(moonSpriteName) != used.end());
				used.insert(moonSpriteName);
			
				const Sprite *moonSprite = SpriteSet::Get(moonSpriteName);
				radius += .5 * moonSprite->Width();
				double period = sqrt((radius * radius * radius) / planetMass);
				planet.children.push_back(System::Object(radius, period, moonSpriteName));
				radius += .5 * moonSprite->Width();
			}
		
			occupied += radius;
			planet.period = sqrt((occupied * occupied * occupied) / (mass * .25));
			planet.distance = occupied;
			root.children.push_back(planet);
			occupied += radius;
		}
		
		int inhab = 0;
		int outhab = 0;
		for(const Object &object : root.children)
		{
			if(object.sprite.length() < 6 || object.sprite.substr(0, 6) != "planet")
				continue;
			
			string prefix = object.sprite.substr(7, 5);
			bool def = (prefix == "fores" || prefix == "ocean" || prefix == "earth");
			bool maybe = (prefix == "cloud");
			
			if(object.distance >= .5 * habitable && object.distance <= 2. * habitable)
				inhab += def + maybe;
			else
				outhab += def;
		}
		feasible = inhab && !outhab;
	}
}



void System::Sol()
{
	// Generate our own solar system, modified slightly to make it more compact
	// and to match the orbital parameters used elsewhere.
	root.children.push_back(System::Object(0., 25.38, "star/g0.png"));
	double mass = 27000.;
	habitable = mass / 25.;
	double massScale = sqrt(mass * .25);
	
	//double pM = 87.97;
	// To give Venus more space:
	double pM = 60.;
	// period = sqrt(d^3 / (m * .25)) = d^1.5 / sqrt(m * .25)
	// (p * massScale)^(2/3) = d
	double dM = pow(pM * massScale, 2. / 3.);
	root.children.push_back(System::Object(dM, pM, "planet/mercury.png"));
	
	//double pV = 224.7;
	// To avoid colliding with the moon:
	double pV = 150.;
	double dV = pow(pV * massScale, 2. / 3.);
	root.children.push_back(System::Object(dV, pV, "planet/venus.png"));
	
	double pE = 365.25;
	double dE = pow(pE * massScale, 2. / 3.);
	root.children.push_back(System::Object(dE, pE, "planet/earth.png"));
	
	double mE = pow(SpriteSet::Get("planet/earth.png")->Width() * .5, 3.) * .015;
	double pEm = 27.3;
	double dEm = pow(pEm * sqrt(mE), 2. / 3.);
	root.children.back().children.push_back(System::Object(dEm, pEm, "planet/luna.png"));
	
	double pA = 687.0;
	double dA = pow(pA * massScale, 2. / 3.);
	root.children.push_back(System::Object(dA, pA, "planet/mars.png"));
	
	//double pJ = 4333.;
	// Moving Jupiter in unrealistically close:
	double pJ = 2000.;
	double dJ = pow(pJ * massScale, 2. / 3.);
	root.children.push_back(System::Object(dJ, pJ, "planet/jupiter.png"));
	
	double mJ = pow(SpriteSet::Get("planet/jupiter.png")->Width() * .5, 3.) * .015;
	double msJ = sqrt(mJ);
	
	// Incorrectly scaled so they won't overlap the planet:
	double pJi = 1.769 * 6;
	double pJe = 3.551 * 6;
	double pJg = 7.155 * 6;
	// To compress this orbit:
	double pJc = 16.69 * 4;
	
	double dJi = pow(pJi * msJ, 2. / 3.);
	double dJe = pow(pJe * msJ, 2. / 3.);
	double dJg = pow(pJg * msJ, 2. / 3.);
	double dJc = pow(pJc * msJ, 2. / 3.);
	
	root.children.back().children.push_back(System::Object(dJi, pJi, "planet/io.png"));
	root.children.back().children.push_back(System::Object(dJe, pJe, "planet/europa.png"));
	root.children.back().children.push_back(System::Object(dJg, pJg, "planet/ganymede.png"));
	root.children.back().children.push_back(System::Object(dJc, pJc, "planet/callisto.png"));
}



void System::ToggleLink(System *link)
{
	if(link == this)
		return;
	
	auto it = find(links.begin(), links.end(), link);
	auto otherIt = find(link->links.begin(), link->links.end(), this);
	if(it == links.end())
	{
		links.push_back(link);
		if(otherIt == link->links.end())
			link->links.push_back(this);
	}
	else
	{
		links.erase(it);
		if(otherIt != link->links.end())
			link->links.erase(otherIt);
	}
}



void System::Write(ostream &out) const
{
	out << "system \"" << name << "\"\n";
	out << "\tpos " << pos.X() << " " << pos.Y() << "\n";
	out << "\thabitable " << habitable << "\n";
	for(const System *link : links)
		out << "\tlink \"" << link->name << "\"\n";
	
	for(const Asteroids &a : asteroids)
		out << "\tasteroids \"" << a.name << "\" " << a.count << " " << a.energy << "\n";
	
	// Now, write everything we did not recognize.
	for(const DataFile::Node &node : unrecognized)
		node.Write(out);
	
	// Finally, write the stellar objects.
	for(const Object &object : root.children)
		Write(out, object, 1);
	
	// Add an extra line break just for readability.
	out << "\n";
}



void System::Write(ostream &out, const Object &object, int depth) const
{
	string tabs(depth, '\t');
	
	out << tabs << "object";
	if(!object.planet.empty())
		out << " \"" << object.planet << "\"";
	out << "\n";
	
	if(!object.sprite.empty())
		out << tabs << "\tsprite \"" << object.sprite << "\"\n";
	
	if(object.distance)
		out << tabs << "\tdistance " << object.distance << "\n";
	
	if(object.period)
		out << tabs << "\tperiod " << object.period << "\n";
	
	if(object.offset)
		out << tabs << "\toffset " << object.offset << "\n";
	
	for(const Object &child : object.children)
		Write(out, child, depth + 1);
}
