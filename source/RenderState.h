/* RenderState.h
Copyright (c) 2021 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef RENDERSTATE_H_
#define RENDERSTATE_H_

#include "Point.h"

#include <array>
#include <unordered_map>
#include <vector>

class Body;
class Sprite;
class StellarObject;



// Class representing a logic state updated by Engine.
class RenderState {
public:
	// Stores information needed for state interpolation.
	// Not every information needed to draw an object needs to be interpolated.
	class Information
	{
	public:
		std::array<float, 2> position = {};
		std::array<float, 4> transform = {};
		std::array<float, 2> blur = {};
		float frame = 0.;
	};

	// The states used by the physics loop. The first entry is the current state.
	static RenderState states[2];
	// The interpolated render state of the current graphics frame.
	static RenderState interpolated;


public:
	// Maps a body to its state.
	std::unordered_map<const Body *, Information> bodies;

	// The planet labels positions.
	std::unordered_map<const StellarObject *, Point> planetLabels;

	// The center of the starfield background.
	Point starFieldCenter;

	// The center velocity.
	Point centerVelocity;

	// Each sprite consists of six vertices (four vertices to form a quad and
	// two dummy vertices to mark the break in between them). Each of those
	// vertices has five attributes: (x, y) position in pixels, (s, t) texture
	// coordinates, and the index of the sprite frame.
	class Data
	{
	public:
		unsigned object;
		std::array<float, 6 * 5> vertices;
	};
	std::unordered_map<const Sprite *, std::vector<Data>> batchData;

	// The position of the base asteroids.
	std::unordered_map<const Body *, Point> asteroids;

	// The target crosshair centers.
	std::unordered_map<const Body *, Point> crosshairs;

	// The status overlays.
	std::unordered_map<const Body *, Point> overlays;

public:
	RenderState() noexcept = default;
	// A render state should not be able to be copied (for performance reasons).
	RenderState(RenderState &&) noexcept = default;
	RenderState &operator=(RenderState &&) noexcept = default;

	// Interpolates the given previous state with the given alpha and returns the new state.
	RenderState Interpolate(const RenderState &previous, double alpha) const;
};



#endif
