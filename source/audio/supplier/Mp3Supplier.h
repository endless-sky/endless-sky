/* Mp3Supplier.h
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

#include "AudioDataSupplier.h"

#include <condition_variable>
#include <iostream>
#include <memory>
#include <thread>



/// Streams audio from an MP3 file. This is an async data supplier.
class Mp3Supplier : public AudioDataSupplier {
public:
	explicit Mp3Supplier(std::shared_ptr<std::iostream> data, bool looping = false);
	~Mp3Supplier();

	// Inherited pure virtual methods
	ALsizei MaxChunkCount() const override;
	int AvailableChunks() const override;
	bool IsSynchronous() const override;
	bool NextChunk(ALuint buffer) override;
	std::vector<int16_t> NextDataChunk() override;
	ALuint AwaitNextChunk() override;
	void ReturnBuffer(ALuint buffer) override;


private:
	/// This is the entry point for the decoding thread.
	void Decode();

private:
	/// The number of chunks to queue up in the buffer
	static constexpr size_t BUFFER_CHUNK_SIZE = 2;
	/// The decoded data
	std::vector<int16_t> buffer;

	/// The MP3 input stream
	std::shared_ptr<std::iostream> data;

	bool done = false;
	bool looping;

	std::thread thread;
	std::mutex decodeMutex;
	std::condition_variable condition;
};
