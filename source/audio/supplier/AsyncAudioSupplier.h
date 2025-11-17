/* AsyncAudioSupplier.h
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

#include "AudioSupplier.h"

#include <condition_variable>
#include <iostream>
#include <memory>
#include <thread>



/// Generic implementation for async suppliers that stream data decoded on another thread.
class AsyncAudioSupplier : public AudioSupplier {
public:
	explicit AsyncAudioSupplier(std::shared_ptr<std::iostream> data, bool looping = false);
	~AsyncAudioSupplier() override;

	// Inherited pure virtual methods
	size_t MaxChunks() const override;
	size_t AvailableChunks() const override;
	std::vector<sample_t> NextDataChunk() override;


protected:
	/// This is the entry point for the decoding thread.
	virtual void Decode() = 0;

	void AwaitBufferSpace();
	/// Adds data to the output buffer, then clears the given sample vector.
	/// If the supplier is done, pads the output buffer to a full chunk with silence.
	void AddBufferData(std::vector<sample_t> &samples);
	/// Pads the buffer to a full output chunk with silence.
	void PadBuffer();

	/// Reads file input. Returns the number of bytes read.
	/// The returned byte count is only less than the requested number if the end of the input was reached.
	/// If the supplier is looping, the next call will still read data. Otherwise, "done" is set to true.
	size_t ReadInput(char *output, size_t bytesToRead);


protected:
	bool done = false;
	const bool looping;

	std::shared_ptr<std::iostream> data;


private:
	/// The number of chunks to queue up in the buffer.
	static constexpr size_t BUFFER_CHUNK_SIZE = 3;
	/// The decoded data.
	std::vector<sample_t> buffer;

	// Sync management
	std::thread audioThread;
	std::mutex bufferMutex;
	std::condition_variable bufferCondition;
};
