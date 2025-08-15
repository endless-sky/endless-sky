/* Mp3Supplier.cpp
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

#include "Mp3Supplier.h"

#include <mad.h>

#include <cmath>
#include <cstring>
#include <thread>
#include <utility>

using namespace std;



Mp3Supplier::Mp3Supplier(shared_ptr<iostream> data, bool looping)
	: data(std::move(data)), looping(looping)
{
	// Don't start the thread until this object is fully constructed.
	thread = std::thread(&Mp3Supplier::Decode, this);
}



Mp3Supplier::~Mp3Supplier()
{
	// Tell the decoding thread to stop.
	{
		lock_guard<mutex> lock(decodeMutex);
		done = true;
	}
	condition.notify_all();
	thread.join();
}



size_t Mp3Supplier::MaxChunks() const
{
	if(done && buffer.size() < OUTPUT_CHUNK)
		return 0;

	return max(static_cast<size_t>(2), AvailableChunks());
}



size_t Mp3Supplier::AvailableChunks() const
{
	return buffer.size() / OUTPUT_CHUNK;
}



vector<AudioSupplier::sample_t> Mp3Supplier::NextDataChunk()
{
	if(AvailableChunks())
	{
		lock_guard<mutex> lock(decodeMutex);

		vector<sample_t> temp{buffer.begin(), buffer.begin() + OUTPUT_CHUNK};
		buffer.erase(buffer.begin(), buffer.begin() + OUTPUT_CHUNK);
		condition.notify_all();
		return temp;
	}
	else
		return vector<sample_t>(OUTPUT_CHUNK);
}



void Mp3Supplier::Decode()
{
	// This vector will store the input from the file.
	vector<unsigned char> input(INPUT_CHUNK, 0);
	// Objects for MP3 decoding:
	mad_stream stream;
	mad_frame frame;
	mad_synth synth;

	// Initialize the decoder.
	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);

	// Loop until we are done.
	while(true)
	{
		// If the "next" buffer has filled up, wait until it is retrieved.
		// Generally try to queue up two chunks worth of samples in it, just
		// in case NextChunk() gets called twice in rapid succession.
		unique_lock<mutex> lock(decodeMutex);
		while(!done && buffer.size() >= BUFFER_CHUNK_SIZE * OUTPUT_CHUNK)
			condition.wait(lock);
		// Check if we're done.
		if(done)
			break;

		// The lock can be freed until we start filling the output buffer.
		lock.unlock();

		// See if any input data is left undecoded in the stream. Typically
		// this is because the last block of input contained a fraction of a
		// full MP3 frame.
		size_t remainder = 0;
		if(stream.next_frame && stream.next_frame < stream.bufend)
			remainder = stream.bufend - stream.next_frame;
		if(remainder)
			memcpy(input.data(), stream.bufend - remainder, remainder);

		// Now, read a chunk of data from the file.
		data->read(reinterpret_cast<char *>(input.data() + remainder), INPUT_CHUNK - remainder);
		// If you get the end of the file, loop around to the beginning.
		size_t read = data->gcount();
		if(data->eof() && looping)
		{
			data->clear();
			data->seekg(0, iostream::beg);
		}
		else if(data->eof())
		{
			done = true;
			// Make sure there are full chunks of data available
			buffer.resize(OUTPUT_CHUNK * ceil(static_cast<double>(buffer.size()) / static_cast<double>(OUTPUT_CHUNK)));
		}

		// If there is nothing to decode, return to the top of this loop.
		if(!(read + remainder))
			continue;

		// Hand the input to the stream decoder.
		mad_stream_buffer(&stream, &input.front(), read + remainder);

		// Loop through the decoded result for that input block.
		while(true)
		{
			// Decode the next frame, and check if there is an error.
			if(mad_frame_decode(&frame, &stream))
			{
				// For recoverable errors, keep going.
				if(MAD_RECOVERABLE(stream.error))
					continue;
				else
					break;
			}
			// Convert the decoded audio into a PCM signal.
			mad_synth_frame(&synth, &frame);

			// If the source is mono, read both output channels from the left input.
			// Otherwise, read two separate input channels.
			mad_fixed_t *channels[2] = {
					synth.pcm.samples[0],
					synth.pcm.samples[synth.pcm.channels > 1]
			};

			// For this part, we need access to the output buffer.
			lock.lock();
			if(done)
				break;

			// We'll alternate what channel we read from each time through the loop.
			bool channel = false;
			for(unsigned i = 0; i < 2 * synth.pcm.length; ++i)
			{
				// Read the next sample from the next channel.
				mad_fixed_t sample = *channels[channel]++;
				channel = !channel;

				// Clip and scale the sample to 16 bits.
				sample += (1L << (MAD_F_FRACBITS - 16));
				sample = max(-MAD_F_ONE, min(MAD_F_ONE - 1, sample));
				buffer.emplace_back(sample >> (MAD_F_FRACBITS + 1 - 16));
			}
			// Now, the buffer can be used by others. In theory, the
			// NextChunk() function could take what's in that buffer while
			// we are right in the middle of this decoding cycle.
			lock.unlock();
		}
	}

	// Clean up.
	mad_synth_finish(&synth);
	mad_frame_finish(&frame);
	mad_stream_finish(&stream);
}
