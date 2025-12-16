/* Swizzle.h
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

#pragma once

#include "Color.h"

#include <array>
#include <string>

class DataNode;



class Swizzle {
public:
	Swizzle() = default;

	void Load(const DataNode &node);
	bool IsLoaded() const;

	const std::string &Name() const;

	bool IsIdentity() const;
	bool OverrideMask() const;
	const float *MatrixPtr() const;

	Color Apply(const Color &to) const;

	static const Swizzle *None();


private:
	explicit Swizzle(bool identity, bool loaded, bool overrideMask, std::array<float, 16> matrix);


private:
	static constexpr inline std::array<float, 16> IDENTITY_MATRIX = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	std::string name;
	// Special case for when a swizzle does not actually need to be calculated.
	bool identity = true;
	bool loaded = false;
	bool overrideMask = false;

	std::array<float, 16> matrix = IDENTITY_MATRIX;
};
