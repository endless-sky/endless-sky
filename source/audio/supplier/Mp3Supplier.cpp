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

#include <array>
#include <cmath>
#include <cstring>
#include <utility>

using namespace std;



Mp3Supplier::Mp3Supplier(shared_ptr<iostream> data, bool looping)
	: AsyncAudioSupplier(std::move(data), looping)
{
}



void Mp3Supplier::Decode()
{
	// This vector will store the input from the file.
	array<unsigned char, INPUT_CHUNK> input{};
	vector<sample_t> samples;
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
		AwaitBufferSpace();
		// Check if we're done.
		if(done)
		{
			PadBuffer();
			break;
		}

		// See if any input data is left undecoded in the stream. Typically
		// this is because the last block of input contained a fraction of a
		// full MP3 frame.
		size_t remainder = 0;
		if(stream.next_frame && stream.next_frame < stream.bufend)
			remainder = stream.bufend - stream.next_frame;
		if(remainder)
			memcpy(input.data(), stream.next_frame, remainder);

		// Now, read a chunk of data from the file.
		size_t read = ReadInput(reinterpret_cast<char *>(input.data() + remainder), INPUT_CHUNK - remainder);

		// If there is nothing to decode, return to the top of this loop.
		if(!(read + remainder))
		{
			if(done)
			{
				AddBufferData(samples);
				break;
			}
			continue;
		}

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
				samples.emplace_back(sample >> (MAD_F_FRACBITS + 1 - 16));
			}
		}
		AddBufferData(samples);
	}

	// Clean up.
	mad_synth_finish(&synth);
	mad_frame_finish(&frame);
	mad_stream_finish(&stream);
}
