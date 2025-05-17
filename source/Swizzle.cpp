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

#include <algorithm>
#include <utility>

using namespace std;

namespace {
	constexpr int STRIDE = 4;
}



void Swizzle::Load(const DataNode &node)
{
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);

		// The corresponding row of the matrix for each channel name.
		static const array<pair<string, int>, 4> channels = {{
			{"red", 0},
			{"green", 1},
			{"blue", 2},
			{"alpha", 3}
		}};
		auto channel = find_if(channels.cbegin(), channels.cend(), [key](const auto &kv)
		{
			return kv.first == key;
		});
		if(channel != channels.cend())
		{
			// Fill in the row of the matrix for the channel.
			// We subtract one to account for the name being in the node.
			int channelStartIndex = channel->second * STRIDE;
			int elementNum = min(child.Size() - 1, 4);
			for(int i = 0; i < elementNum; i++)
				matrix[channelStartIndex + i] = child.Value(i + 1);
		}
		else if(key == "override")
			overrideMask = true;
		else
			child.PrintTrace("Unrecognized attribute in swizzle definition:");
	}

	// Special-case flag for when applying a swizzle would do nothing at all.
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
	array<float, 4> newColor = {0, 0, 0, 0};

	// Manual matrix multiply into newColor.
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			newColor[i] += colorPtr[j] * matrix[i * STRIDE + j];

	return Color(newColor[0], newColor[1], newColor[2], newColor[3]);
}



const Swizzle *Swizzle::None()
{
	static const Swizzle IDENTITY_SWIZZLE(
		true,
		true,
		true,
		IDENTITY_MATRIX
	);

	return &IDENTITY_SWIZZLE;
}



Swizzle::Swizzle(bool identity, bool loaded, bool overrideMask, array<float, 16> matrix)
	: identity(identity), loaded(loaded), overrideMask(overrideMask), matrix(matrix)
{
}
