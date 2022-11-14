/* Music.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MUSIC_H_
#define MUSIC_H_

#include <mad.h>

#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <future>
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
	// How many bytes to read from the file at a time:
	static constexpr size_t INPUT_CHUNK = 65536;
	// How many samples to put in each output block. Because the output is in
	// stereo, the duration of the sample is half this amount:
	static constexpr size_t OUTPUT_CHUNK = INPUT_CHUNK / 2;

	using Buffer = std::vector<int16_t>;


public:
	static void Init(const std::vector<std::string> &sources);


public:
	~Music();

	// Set the source of music. If the path is empty, this music will be silent.
	void SetSource(const std::string &name = "");
	// Get the name of the current music source playing.
	const std::string &GetSource() const;
	// Get the next audio buffer to play.
	const Buffer &NextChunk();


private:
	// Starts a task that decodes a single frame from the currently loaded file.
	void DecodeFrame();


private:
	// Buffers for storing the decoded audio sample. The "silence" buffer holds
	// a block of silence to be returned if nothing was read from the file.
	Buffer silence = Buffer(OUTPUT_CHUNK, 0);
	Buffer next;
	Buffer current;

	std::string currentSource;
	std::string previousPath;

	// This pointer holds the file that is currently being decoded.
	FILE *file = nullptr;
	std::future<void> decoder;

	mad_stream stream;
	mad_frame frame;
	mad_synth synth;
};



#endif
