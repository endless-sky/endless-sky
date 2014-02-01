/* SpriteSet.cpp
Michael Zahniser, 24 Oct 2013

Function definitions for the SpriteSet class.
*/

#include "SpriteSet.h"

#include "Sprite.h"

#include <map>

using namespace std;

namespace {
	map<string, Sprite> sprites;
}



const Sprite *SpriteSet::Get(const string &name)
{
	return &sprites[name];
}



Sprite *SpriteSet::Modify(const string &name)
{
	return &sprites[name];
}
