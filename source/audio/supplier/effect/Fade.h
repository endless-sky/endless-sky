/* Fade.h
Copyright (c) 2025 by tibetiroka

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

#include "../AudioSupplier.h"

#include <memory>
#include <tuple>
#include <vector>



/// Cross-fades any number of sources into a single source.
/// There is usually a "primary" source that is not being faded,
/// which then gets replaced by another source. That's when they are
/// cross-faded.
class Fade : public AudioSupplier {
public:
	/// The fade duration. Smaller values mean faster fade.
	/// The total number of faded samples is MAX_FADE / fadePerFrame.
	static constexpr size_t MAX_FADE = 65536;
public:
	Fade() = default;

	// Adds a new primary source, and fades out the previous primary source at the specified rate.
	void AddSource(std::unique_ptr<AudioSupplier> source, size_t fadePerFrame = 1);

	void Set3x(bool is3x) override;

	// Inherited pure virtual methods
	size_t MaxChunks() const override;
	size_t AvailableChunks() const override;
	std::vector<sample_t> NextDataChunk() override;


private:
	/// Cross-fades two sources. The faded result is stored in the fadeIn input.
	static void CrossFade(const std::vector<sample_t> &fadeOut, std::vector<sample_t> &fadeIn, size_t &fade,
		size_t fadePerFrame);


private:
	/// The fading sources, with their current fade values, and how much they fade per frame.
	std::vector<std::tuple<std::unique_ptr<AudioSupplier>, size_t, size_t>> fadeProgress;
	/// The primary source; this one is not faded out by itself, but can be cross-faded with the other sources.
	std::unique_ptr<AudioSupplier> primarySource;
};
