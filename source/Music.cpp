/* Music.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Music.h"

#include "Files.h"

#include <mad.h>

#include <algorithm>
#include <cstring>
#include <map>

using namespace std;

namespace {
	// How many bytes to read from the file at a time:
	const size_t INPUT_CHUNK = 65536;
	// How many samples to put in each output block. Because the output is in
	// stereo, the duration of the sample is half this amount:
	const size_t OUTPUT_CHUNK = 32768;
	
	map<string, string> paths;
}



void Music::Init(const vector<string> &sources)
{
	for(const string &source : sources)
	{
		// Find all the sound files that this resource source provides.
		string root = source + "sounds/";
		vector<string> files = Files::RecursiveList(root);
		
		for(const string &path : files)
		{
			// Sanity check on the path length.
			if(path.length() < root.length() + 4)
				continue;
			string ext = path.substr(path.length() - 4);
			if(ext != ".mp3" && ext != ".MP3")
				continue;
			
			string name = path.substr(root.length(), path.length() - root.length() - 4);
			paths[name] = path;
		}
	}
}



// Music constructor, which starts the decoding thread. Initially, the thread
// has no file to read, so it will sleep until a file is specified.
Music::Music()
	: silence(OUTPUT_CHUNK, 0)
{
	// Don't start the thread until this object is fully constructed.
	thread = std::thread(&Music::Decode, this);
}



// Destructor, which waits for the thread to stop.
Music::~Music()
{
	// Tell the decoding thread to stop.
	{
		unique_lock<mutex> lock(decodeMutex);
		done = true;
	}
	condition.notify_all();
	thread.join();
	
	// If the decode thread has not yet taken possession of the next file, it is
	// our job to close it.
	if(nextFile)
		fclose(nextFile);
}



// Set the source of music. If the path is empty, this music will be silent.
void Music::SetSource(const string &name)
{
	// Find a file that provides this music.
	auto it = paths.find(name);
	string path = (it == paths.end() ? "" : it->second);
	
	// Do nothing if this is the same file we're playing.
	if(path == previousPath)
		return;
	previousPath = path;
	
	// Inform the decoding thread that it should switch to decoding a new file.
	unique_lock<mutex> lock(decodeMutex);
	if(path.empty())
		nextFile = nullptr;
	else
		nextFile = Files::Open(path);
	hasNewFile = true;
	
	// Also clear any decoded data left over from the previous file.
	next.clear();
	
	// Notify the decoding thread that it can start.
	lock.unlock();
	condition.notify_all();
}



// Get the next audio buffer to play.
const vector<int16_t> &Music::NextChunk()
{
	// Check whether the "next" buffer is ready.
	unique_lock<mutex> lock(decodeMutex);
	if(next.size() < OUTPUT_CHUNK)
		return silence;
	
	// If the next buffer is ready, move a chunk of data into the output buffer.
	// All output buffers need to be the same size so that we can fade between
	// two different sources.
	current.clear();
	current.insert(current.begin(), next.begin(), next.begin() + OUTPUT_CHUNK);
	next.erase(next.begin(), next.begin() + OUTPUT_CHUNK);
	
	// Once the lock is unlocked, notify the decoding thread to continue.
	lock.unlock();
	condition.notify_all();
	
	// Return the buffer.
	return current;
	
}



// Entry point for the decoding thread.
void Music::Decode()
{
	// This vector will store the input from the file.
	vector<unsigned char> input(INPUT_CHUNK, 0);
	// Objects for MP3 decoding:
	mad_stream stream;
	mad_frame frame;
	mad_synth synth;
	// Loop until the thread is told to quit.
	while(true)
	{
		// First, wait until a new file has been specified or we're done.
		FILE *file = nullptr;
		while(!file)
		{
			unique_lock<mutex> lock(decodeMutex);
			while(!done && !hasNewFile)
				condition.wait(lock);
			
			// If the "done" variable has been set, exit this thread.
			if(done)
				return;
			
			// The new file now belongs to us, and it's our job to close it.
			file = nextFile;
			nextFile = nullptr;
			hasNewFile = false;
		}
		
		// Now, we have a file to read. Initialize the decoder.
		mad_stream_init(&stream);
		mad_frame_init(&frame);
		mad_synth_init(&synth);
		
		// Loop until we are asked to switch files.
		while(true)
		{
			// If the "next" buffer has filled up, wait until it is retrieved.
			// Generally try to queue up two chunks worth of samples in it, just
			// in case NextChunk() gets called twice in rapid succession.
			unique_lock<mutex> lock(decodeMutex);
			while(!done && next.size() >= 2 * OUTPUT_CHUNK)
				condition.wait(lock);
			// Check if we're done or if we need to switch files.
			if(done || hasNewFile)
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
				memcpy(&input.front(), stream.next_frame, remainder);
			
			// Now, read a chunk of data from the file.
			size_t read = fread(&input.front() + remainder, 1, INPUT_CHUNK - remainder, file);
			// If you get the end of the file, loop around to the beginning.
			if(!read || feof(file))
				rewind(file);
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
				if(done || hasNewFile)
					break;
	
				// We'll alternate what channel we read from each time through the loop.
				int channel = 0;
				for(unsigned i = 0; i < 2 * synth.pcm.length; ++i)
				{
					// Read the next sample from the next channel.
					mad_fixed_t sample = *channels[channel]++;
					channel = !channel;
		
					// Clip and scale the sample to 16 bits.
					sample += (1L << (MAD_F_FRACBITS - 16));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
					sample = max(-MAD_F_ONE, min(MAD_F_ONE - 1, sample));
#pragma GCC diagnostic pop
					next.push_back(sample >> (MAD_F_FRACBITS + 1 - 16));
				}
				// Now, the "next" buffer can be used by others. In theory, the
				// NextChunk() function could take what's in that buffer while
				// we are right in the middle of this decoding cycle.
				lock.unlock();
			}
		}
		
		// Clean up.
		mad_synth_finish(&synth);
		mad_frame_finish(&frame);
		mad_stream_finish(&stream);
		fclose(file);
	}
}
