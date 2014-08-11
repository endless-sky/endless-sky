/* Sound.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Sound.h"

#ifndef __APPLE__
#include <AL/alut.h>
#else
#include <OpenAL/al.h>
#include <fstream>
#include <vector>
#endif

using namespace std;

#ifdef __APPLE__
namespace {
	// Read a WAV header, and return the size of the data, in bytes. If the file
	// is an unsupported format (anything but little-endian 16-bit PCM at 44100 HZ),
	// this will return 0.
	uint32_t ReadHeader(istream &in, uint32_t &frequency);
	uint32_t Read4(istream &in);
	uint16_t Read2(istream &in);
}
#endif



void Sound::Load(const string &path)
{
	if(path.length() < 5 || path.compare(path.length() - 4, 4, ".wav"))
		return;
	
	isLooped = path[path.length() - 5] == '~';
#ifndef __APPLE__
	buffer = alutCreateBufferFromFile(path.c_str());
#else
	ifstream in(path, ios::in | ios::binary);
	uint32_t frequency = 0;
	uint32_t bytes = ReadHeader(in, frequency);
	if(bytes)
	{
		vector<char> data(bytes);
		in.read(&data.front(), bytes);
		
		alGenBuffers(1, &buffer);
		alBufferData(buffer, AL_FORMAT_MONO16, &data.front(), bytes, frequency);
	}
#endif
}



unsigned Sound::Buffer() const
{
	return buffer;
}



bool Sound::IsLooping() const
{
	return isLooped;
}



#ifdef __APPLE__
namespace {
	// Read a WAV header, and return the size of the data, in bytes. If the file
	// is an unsupported format (anything but little-endian 16-bit PCM at 44100 HZ),
	// this will return 0.
	uint32_t ReadHeader(istream &in, uint32_t &frequency)
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
					in.ignore(subchunkSize - 16);
				
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
				in.ignore(subchunkSize);
		}
	}
	
	
	
	uint32_t Read4(istream &in)
	{
		char data[4];
		in.read(data, 4);
		uint32_t result = 0;
		for(int i = 0; i < 4; ++i)
			result |= static_cast<uint32_t>(static_cast<unsigned char>(data[i])) << (i * 8);
		return result;
	}
	
	
	
	uint16_t Read2(istream &in)
	{
		char data[2];
		in.read(data, 2);
		uint16_t result = 0;
		for(int i = 0; i < 2; ++i)
			result |= static_cast<uint16_t>(static_cast<unsigned char>(data[i])) << (i * 8);
		return result;
	}
}
#endif
