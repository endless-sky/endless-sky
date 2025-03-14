/* Swizzle.cpp
Copyright (c) 2025 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Swizzle.h"

#include "Color.h"
#include "DataNode.h"

#include <array>
#include <string>
#include <utility>

using namespace std;



Swizzle::Swizzle()
{
}



void Swizzle::Load(const DataNode &node)
{
	if(node.Size() < 2)
		node.PrintTrace("swizzle without a name");
	else
		name = node.Token(1);
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);

		static const array<pair<string, int>, 4> channels = {{
			{"red", 0},
			{"green", 1},
			{"blue", 2},
			{"alpha", 3}
		}};
		bool wasChannel = false;
		for(auto channel : channels)
			if(key == channel.first)
			{
				for(int i = 0; i < child.Size() - 1; i++)
					matrix[channel.second * STRIDE + i] = child.Value(i + 1);
				wasChannel = true;
				continue;
			}
		if(wasChannel)
		{
		}
		else if(key == "override")
			overrideMask = true;
		else
			child.PrintTrace("Unrecognised attribute in swizzle definition");
	}

	identity = matrix == IDENTITY_MATRIX;
	loaded = true;
}



bool Swizzle::IsLoaded() const
{
	return loaded;
}



const string &Swizzle::Name() const
{
	return name;
}



bool Swizzle::IsIdentity() const
{
	return identity;
}



bool Swizzle::OverrideMask() const
{
	return overrideMask;
}



const float *Swizzle::MatrixPtr() const
{
	return matrix.data();
}



Color Swizzle::Apply(const Color &to) const
{
	const float *colorPtr = to.Get();
	array<double, 4> newColor = {0, 0, 0, 0};
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			newColor[i] += colorPtr[j] * matrix[i * STRIDE + j];

	return Color(newColor[0], newColor[1], newColor[2], newColor[3]);
}



Swizzle *Swizzle::None()
{
	static Swizzle IDENTITY_SWIZZLE = Swizzle {
		true,
		true,
		true,
		IDENTITY_MATRIX
	};

	return &IDENTITY_SWIZZLE;
}



Swizzle::Swizzle(bool identity, bool loaded, bool overrideMask, array<float, 16> matrix)
	: identity(identity), loaded(loaded), overrideMask(overrideMask), matrix(matrix)
{
}
