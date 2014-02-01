/* Outfit.cpp
Michael Zahniser, 28 Oct 2013

Function definitions for the Outfit class.
*/

#include "Outfit.h"

#include "SpriteSet.h"

using namespace std;



Outfit::Outfit()
	: cost(0), ammo(nullptr)
{
}



void Outfit::Load(const DataFile::Node &node, const Set<Outfit> &outfits, const Set<Effect> &effects)
{
	if(node.Size() >= 2)
		name = node.Token(1);
	category = "Other";
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "cost" && child.Size() >= 2)
			cost = child.Value(1);
		else if(child.Token(0) == "category" && child.Size() >= 2)
			category = child.Token(1);
		else if(child.Token(0) == "flare sprite" && child.Size() >= 2)
			flare.Load(child);
		else if(child.Token(0) == "weapon")
		{
			for(const DataFile::Node &grand : child)
			{
				if(grand.Token(0) == "sprite" && grand.Size() >= 2)
					weaponSprite.Load(grand);
				else if(grand.Token(0) == "ammo" && grand.Size() >= 2)
					ammo = outfits.Get(grand.Token(1));
				else if(grand.Token(0) == "hit effect" && grand.Size() >= 2)
				{
					int count = (grand.Size() >= 3) ? grand.Value(2) : 1;
					hitEffects[effects.Get(grand.Token(1))] += count;
				}
				else if(grand.Token(0) == "die effect" && grand.Size() >= 2)
				{
					int count = (grand.Size() >= 3) ? grand.Value(2) : 1;
					dieEffects[effects.Get(grand.Token(1))] += count;
				}
				else if(grand.Token(0) == "submunition" && grand.Size() >= 2)
				{
					int count = (grand.Size() >= 3) ? grand.Value(2) : 1;
					submunitions[outfits.Get(grand.Token(1))] += count;
				}
				else if(grand.Size() >= 2)
					weapon[grand.Token(0)] = grand.Value(1);
			}
			weapon["range"] = weapon["lifetime"] * (
				weapon["velocity"] + .5 * weapon["acceleration"] * weapon["lifetime"]);
		}
		else if(child.Size() >= 2)
			attributes[child.Token(0)] = child.Value(1);
	}
}



const string &Outfit::Name() const
{
	return name;
}



const string &Outfit::Category() const
{
	return category;
}



int Outfit::Cost() const
{
	return cost;
}



double Outfit::Get(const string &attribute) const
{
	auto it = attributes.find(attribute);
	return (it == attributes.end()) ? 0. : it->second;
}



const map<string, double> &Outfit::Attributes() const
{
	return attributes;
}



// Determine whether the given number of instances of the given outfit can
// be added to a ship with the attributes represented by this instance. If
// not, return the maximum number that can be added.
int Outfit::CanAdd(const Outfit &other, int count) const
{
	for(const auto &at : other.attributes)
	{
		double value = Get(at.first);
		if(value + at.second * count < 0.)
			count = value / -at.second;
	}
	
	return count;
}



// For tracking a combination of outfits in a ship: add the given number of
// instances of the given outfit to this outfit.
void Outfit::Add(const Outfit &other, int count)
{
	cost += other.cost * count;
	
	for(const auto &at : other.attributes)
		attributes[at.first] += at.second * count;
	
	if(other.flare.GetSprite())
		flare = other.flare;
}



// Modify this outfit's attributes.
void Outfit::Add(const string &attribute, double value)
{
	attributes[attribute] += value;
}


	
// Get this outfit's engine flare sprite, if any.
const Animation &Outfit::FlareSprite() const
{
	return flare;
}



bool Outfit::IsWeapon() const
{
	return !weapon.empty();
}



// Get the weapon provided by this outfit, if any.
const Animation &Outfit::WeaponSprite() const
{
	return weaponSprite;
}



const Outfit *Outfit::Ammo() const
{
	return ammo;
}



double Outfit::WeaponGet(const string &attribute) const
{
	auto it = weapon.find(attribute);
	return (it == weapon.end()) ? 0. : it->second;
}



// Handle a weapon impacting something or reaching its end of life.
const map<const Effect *, int> Outfit::HitEffects() const
{
	return hitEffects;
}



const map<const Effect *, int> Outfit::DieEffects() const
{
	return dieEffects;
}



const map<const Outfit *, int> Outfit::Submunitions() const
{
	return submunitions;
}
