/* RenderState.cpp
Copyright (c) 2021 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "RenderState.h"

#include "Body.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "text/Format.h"

#include <algorithm>
#include <cassert>
#include <sstream>

using namespace std;



RenderState RenderState::current;
RenderState RenderState::interpolated;



// Interpolates the given previous state with the given alpha and returns the new state.
void RenderState::Interpolate(const RenderState &previous, double alpha)
{
	// Clear the previous entries.
	interpolated.bodies.clear();
	interpolated.planetLabels.clear();
	interpolated.batchData.clear();
	interpolated.asteroids.clear();
	interpolated.crosshairs.clear();
	interpolated.overlays.clear();

	for(const auto &it : current.bodies)
	{
		auto prevInformation = previous.bodies.find(it.first);
		if(prevInformation == previous.bodies.end())
		{
			// The current body appeared in the current frame.
			interpolated.bodies.emplace(it);
			continue;
		}

		// Since the body existed in the previous physics frame, we need to interpolate
		// the two bodies.
		const auto &prev = prevInformation->second;
		Information info;

		info.position[0] = it.second.position[0] * alpha + prev.position[0] * (1. - alpha);
		info.position[1] = it.second.position[1] * alpha + prev.position[1] * (1. - alpha);
		info.transform[0] = it.second.transform[0] * alpha + prev.transform[0] * (1. - alpha);
		info.transform[1] = it.second.transform[1] * alpha + prev.transform[1] * (1. - alpha);
		info.transform[2] = it.second.transform[2] * alpha + prev.transform[2] * (1. - alpha);
		info.transform[3] = it.second.transform[3] * alpha + prev.transform[3] * (1. - alpha);
		info.blur[0] = it.second.blur[0] * alpha + prev.blur[0] * (1. - alpha);
		info.blur[1] = it.second.blur[1] * alpha + prev.blur[1] * (1. - alpha);
		info.frame = it.second.frame * alpha + prev.frame * (1. - alpha);
		interpolated.bodies.emplace(it.first, std::move(info));
	}
	for(const auto &label : current.planetLabels)
	{
		auto prev = previous.planetLabels.find(label.first);
		if(prev == previous.planetLabels.end())
		{
			interpolated.planetLabels.emplace(label);
			continue;
		}
		interpolated.planetLabels.emplace(label.first, label.second * alpha + prev->second * (1. - alpha));
	}
	interpolated.starFieldCenter = current.starFieldCenter * alpha + previous.starFieldCenter * (1. - alpha);
	interpolated.centerVelocity = current.centerVelocity * alpha + previous.centerVelocity * (1. - alpha);
	for(const auto &it : current.batchData)
	{
		auto prev = previous.batchData.find(it.first);
		if(prev == previous.batchData.end())
		{
			interpolated.batchData.emplace(it);
			continue;
		}

		for(const auto &data : it.second)
		{
			const auto &body = data.object;
			assert(body && "invalid reference to body");
			auto prevBody = find_if(prev->second.begin(), prev->second.end(), [&body](const Data &data)
					{
						return data.object == body;
					});
			if(prevBody == prev->second.end())
			{
				// This body didn't exist in the previous frame so we just add it.
				interpolated.batchData[it.first].emplace_back(data);
				continue;
			}

			interpolated.batchData[it.first].emplace_back();
			auto &newData = interpolated.batchData[it.first].back();
			newData.object = body;
			for(size_t i = 0; i < newData.vertices.size(); ++i)
				newData.vertices[i] = data.vertices[i] * alpha + prevBody->vertices[i] * (1. - alpha);
		}
	}
	for(const auto &it : current.asteroids)
	{
		auto prev = previous.asteroids.find(it.first);
		if(prev == previous.asteroids.end())
		{
			interpolated.asteroids[it.first] = Point();
			continue;
		}

		interpolated.asteroids[it.first] = it.second * alpha + prev->second * (1. - alpha);
		interpolated.asteroids[it.first] -= prev->second;
	}
	for(const auto &it : current.crosshairs)
	{
		auto prev = previous.crosshairs.find(it.first);
		if(prev == previous.crosshairs.end())
		{
			interpolated.crosshairs.emplace(it);
			continue;
		}

		interpolated.crosshairs[it.first] = it.second * alpha + prev->second * (1. - alpha);
	}
	for(const auto &it : current.overlays)
	{
		auto prev = previous.overlays.find(it.first);
		if(prev == previous.overlays.end())
		{
			interpolated.overlays.emplace(it);
			continue;
		}

		interpolated.overlays[it.first] = it.second * alpha + prev->second * (1. - alpha);
	}
}
