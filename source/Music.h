/* Music.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MUSIC_H_
#define MUSIC_H_

#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <vector>



// The Music class streams mp3 audio from a file and delivers it to the program
// on "block" at a time, so it never needs to hold the entire decoded file in
// memory. Each block is 16-bit stereo, 44100 Hz. If no file is specified, or if
// the decoding thread is not done yet, it returns silence rather than blocking,
// so the game won't freeze if the music stops for some reason.
class Music {
public:
	static void Init(const std::vector<std::string> &sources);
	
	
public:
	Music();
	~Music();
	
	void SetSource(const std::string &name = "");
	const std::vector<int16_t> &NextChunk();
	
	
private:
	// This is the entry point for the decoding thread.
	void Decode();
	
	
private:
	// Buffers for storing the decoded audio sample. The "silence" buffer holds
	// a block of silence to be returned if nothing was read from the file.
	std::vector<int16_t> silence;
	std::vector<int16_t> next;
	std::vector<int16_t> current;
	
	std::string previousPath;
	// This pointer holds the file for as long as it is owned by the main
	// thread. When the decode thread takes possession of it, it sets this
	// pointer to null.
	FILE *nextFile = nullptr;
	bool hasNewFile = false;
	bool done = false;
	
#ifndef ES_NO_THREADS
	std::thread thread;
	std::mutex decodeMutex;
	std::condition_variable condition;
#endif // ES_NO_THREADS
};



#endif
