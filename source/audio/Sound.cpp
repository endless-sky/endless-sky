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

#include "../Files.h"
#include "../Logger.h"
#include "supplier/WavSupplier.h"

#include <cstdint>

using namespace std;

namespace {
	// Read a WAV header, and return the size of the data, in bytes. If the file
	// is an unsupported format (anything but little-endian 16-bit PCM at 44100 HZ),
	// this will return 0.
	uint32_t ReadHeader(shared_ptr<iostream> &in, uint32_t frequency);
	uint32_t Read4(const shared_ptr<iostream> &in);
	uint16_t Read2(const shared_ptr<iostream> &in);
}



bool Sound::Load(const filesystem::path &path, const string &name)
{
	if(path.extension() != ".wav")
		return false;
	this->name = name;

	isLooped = path.stem().string().ends_with('~');
	bool isFast = isLooped ? path.stem().string().ends_with("@3x~") : path.stem().string().ends_with("@3x");
	vector<AudioSupplier::sample_t> &buf = isFast ? buffer3x : buffer;

	shared_ptr<iostream> in = Files::Open(path);
	if(!in)
		return false;
	uint32_t bytes = ReadHeader(in, AudioSupplier::SAMPLE_RATE);
	if(!bytes)
	{
		Logger::Log("WAV file uses an unsupported format. Only 44100Hz little-endian 16-bit PCM is supported.",
			Logger::Level::WARNING);
		return false;
	}

	// Read 16-bit mono from the file.
	vector<char> data(bytes);
	in->read(data.data(), bytes);

	// Store 16-bit stereo buffer.
	buf.resize(2 * data.size() / sizeof(AudioSupplier::sample_t));
	for(size_t i = 0; i < buf.size() / 2; ++i)
	{
		buf[2 * i] = reinterpret_cast<AudioSupplier::sample_t *>(data.data())[i];
		buf[2 * i + 1] = reinterpret_cast<AudioSupplier::sample_t *>(data.data())[i];
	}
	return true;
}



const string &Sound::Name() const
{
	return name;
}



const vector<AudioSupplier::sample_t> &Sound::Buffer() const
{
	return buffer.empty() ? buffer3x : buffer;
}



const vector<AudioSupplier::sample_t> &Sound::Buffer3x() const
{
	return buffer3x.empty() ? buffer : buffer3x;
}



bool Sound::IsLooping() const
{
	return isLooped;
}



unique_ptr<AudioSupplier> Sound::CreateSupplier() const
{
	return unique_ptr<AudioSupplier>{new WavSupplier{*this, false, IsLooping()}};
}



namespace {
	// Read a WAV header, and return the size of the data, in bytes. If the file
	// is an unsupported format (anything but little-endian 16-bit PCM at 44100 HZ),
	// this will return 0.
	uint32_t ReadHeader(shared_ptr<iostream> &in, uint32_t frequency)
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
				uint32_t fileFrequency = Read4(in);
				uint32_t byteRate = Read4(in);
				uint32_t blockAlign = Read2(in);
				uint32_t bitsPerSample = Read2(in);

				// Skip any further bytes in this chunk.
				if(subchunkSize > 16)
					in->seekg(subchunkSize - 16, ios::cur);

				if(audioFormat != 1)
					return 0;
				if(numChannels != 1)
					return 0;
				if(bitsPerSample != 16)
					return 0;
				if(fileFrequency != frequency)
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
				in->seekg(subchunkSize, ios::cur);
		}
	}



	uint32_t Read4(const shared_ptr<iostream> &in)
	{
		unsigned char data[4];
		in->read(reinterpret_cast<char *>(data), 4);
		if(in->gcount() != 4)
			return 0;
		uint32_t result = 0;
		for(int i = 0; i < 4; ++i)
			result |= static_cast<uint32_t>(data[i]) << (i * 8);
		return result;
	}



	uint16_t Read2(const shared_ptr<iostream> &in)
	{
		unsigned char data[2];
		in->read(reinterpret_cast<char *>(data), 2);
		if(in->gcount() != 2)
			return 0;
		uint16_t result = 0;
		for(int i = 0; i < 2; ++i)
			result |= static_cast<uint16_t>(data[i]) << (i * 8);
		return result;
	}
}
