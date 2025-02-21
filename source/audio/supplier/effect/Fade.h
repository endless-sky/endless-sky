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

#include "../AudioDataSupplier.h"

#include <memory>
#include <vector>

/// Cross-fades any number of sources into a single source.
/// There is usually a "primary" source that is not being faded,
/// which then gets replaced by another source. That's when they are
/// cross-faded.
class Fade : public AudioDataSupplier {
public:
	Fade() = default;

	void AddSource(std::unique_ptr<AudioDataSupplier> source, size_t fade = MAX_FADE);

	// Inherited pure virtual methods
	ALsizei MaxChunkCount() const override;
	int AvailableChunks() const override;
	bool IsSynchronous() const override;
	bool NextChunk(ALuint buffer) override;
	std::vector<int16_t> NextDataChunk() override;
	ALuint AwaitNextChunk() override;
	void ReturnBuffer(ALuint buffer) override;


private:
	/// Cross-fades two sources. The faded result is stored in the fadeIn input.
	void CrossFade(const std::vector<int16_t> &fadeOut, std::vector<int16_t> &fadeIn, size_t &fade);


private:
	static constexpr size_t MAX_FADE = 65536;

	std::vector<std::pair<std::unique_ptr<AudioDataSupplier>, size_t>> fadeProgress;
	std::unique_ptr<AudioDataSupplier> primarySource;
};
