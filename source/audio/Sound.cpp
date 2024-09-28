/* Sound.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Sound.h"

#include "../File.h"

#include <SDL2/SDL_rwops.h>
#include "../Files.h"

#include <AL/al.h>

#include <cstdint>
#include <cstdio>
#include <vector>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#define MINIMP3_ONLY_MP3
//#define MINIMP3_ONLY_SIMD
//#define MINIMP3_NO_SIMD
#define MINIMP3_NONSTANDARD_BUT_LOGICAL
//#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#pragma GCC diagnostic pop

using namespace std;

namespace {
	// Read a WAV header, and return the size of the data, in bytes. If the file
	// is an unsupported format (anything but little-endian 16-bit PCM at 44100 HZ),
	// this will return 0.
	uint32_t ReadHeader(File &in, uint32_t &frequency);
	uint32_t Read4(File &in);
	uint16_t Read2(File &in);

	// Read an mp3 file
	bool ReadMP3(File& in, vector<char>& data, uint32_t &frequency);
}



bool Sound::Load(const string &path, const string &name)
{
	if(path.length() < 5)
		return false;
	this->name = name;

	isLooped = path[path.length() - 5] == '~';

	uint32_t frequency = 0;
	vector<char> data;

	if(path.compare(path.length() - 4, 4, ".wav") == 0)
	{
		File in(path);
		if(!in)
			return false;
		uint32_t bytes = ReadHeader(in, frequency);
		if(!bytes)
			return false;

		data.resize(bytes);
		if(SDL_RWread(in, &data[0], 1, bytes) != bytes)
			return false;
	}
	else if(path.compare(path.length() - 4, 4, ".mp3") == 0)
	{
		File in(path);
		if(!in)
			return false;
		if(!ReadMP3(in, data, frequency))
			return false;
	}
	else
	{
		return false;
	}

	if(!buffer)
		alGenBuffers(1, &buffer);
	alBufferData(buffer, AL_FORMAT_MONO16, &data.front(), data.size(), frequency);

	return true;
}



const string &Sound::Name() const
{
	return name;
}



unsigned Sound::Buffer() const
{
	return buffer;
}



bool Sound::IsLooping() const
{
	return isLooped;
}



namespace {
	// Read a WAV header, and return the size of the data, in bytes. If the file
	// is an unsupported format (anything but little-endian 16-bit PCM at 44100 HZ),
	// this will return 0.
	uint32_t ReadHeader(File &in, uint32_t &frequency)
	{
		uint32_t chunkID = Read4(in);
		if(chunkID != 0x46464952) // "RIFF" in big endian.
			return 0;

		// Ignore the "chunk size".
		Read4(in);
		uint32_t format = Read4(in);
		if(format != 0x45564157) // "WAVE"
			return 0;

		bool foundHeader = false;
		while(true)
		{
			uint32_t subchunkID = Read4(in);
			uint32_t subchunkSize = Read4(in);

			if(subchunkID == 0x20746d66) // "fmt "
			{
				foundHeader = true;
				if(subchunkSize < 16)
					return 0;

				uint16_t audioFormat = Read2(in);
				uint16_t numChannels = Read2(in);
				frequency = Read4(in);
				uint32_t byteRate = Read4(in);
				uint32_t blockAlign = Read2(in);
				uint32_t bitsPerSample = Read2(in);

				// Skip any further bytes in this chunk.
				if(subchunkSize > 16)
					SDL_RWseek(in, subchunkSize - 16, RW_SEEK_CUR);

				if(audioFormat != 1)
					return 0;
				if(numChannels != 1)
					return 0;
				if(bitsPerSample != 16)
					return 0;
				if(byteRate != frequency * numChannels * bitsPerSample / 8)
					return 0;
				if(blockAlign != numChannels * bitsPerSample / 8)
					return 0;
			}
			else if(subchunkID == 0x61746164) // "data"
			{
				if(!foundHeader)
					return 0;
				return subchunkSize;
			}
			else
				SDL_RWseek(in, subchunkSize, RW_SEEK_CUR);
		}
	}



	uint32_t Read4(File &in)
	{
		unsigned char data[4];
		if(SDL_RWread(in, data, 1, 4) != 4)
			return 0;
		uint32_t result = 0;
		for(int i = 0; i < 4; ++i)
			result |= static_cast<uint32_t>(data[i]) << (i * 8);
		return result;
	}



	uint16_t Read2(File &in)
	{
		unsigned char data[2];
		if(SDL_RWread(in, data, 1, 2) != 2)
			return 0;
		uint16_t result = 0;
		for(int i = 0; i < 2; ++i)
			result |= static_cast<uint16_t>(data[i]) << (i * 8);
		return result;
	}



	bool ReadMP3(File& in, vector<char>& data, uint32_t& frequency)
	{
		mp3dec_t mp3d;
		mp3dec_init(&mp3d);
		
		auto raw_data = Files::Read(in);
		const uint8_t* p = reinterpret_cast<const uint8_t*>(raw_data.data());
		const uint8_t* pend = p + raw_data.size();

		size_t size = 0;
		frequency = 0;
		while(p < pend)
		{
			data.resize(size + MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(int16_t));
			int16_t* next = reinterpret_cast<int16_t*>(data.data() + size);
			mp3dec_frame_info_t info;
			size_t sample_count = mp3dec_decode_frame(&mp3d, p, pend-p, next, &info);

			if(info.frame_bytes <= 0) // insufficient data... but we gave it the
				break;                 // whole buffer.
			size += sample_count * 2;
			if(frequency == 0)
				frequency = static_cast<uint32_t>(info.hz);
			else if(frequency != static_cast<uint32_t>(info.hz))
				return false;          // file is not fixed frequency.
			else if(info.channels != 1)
				return false;          // only mono is supported

			p += info.frame_bytes;
		}
		data.resize(size);
		return !data.empty();
	}
}
