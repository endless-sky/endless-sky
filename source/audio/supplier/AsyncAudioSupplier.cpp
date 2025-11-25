/* AsyncAudioSupplier.cpp
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

#include "AsyncAudioSupplier.h"

#include <cmath>
#include <thread>
#include <utility>

using namespace std;



AsyncAudioSupplier::AsyncAudioSupplier(shared_ptr<iostream> data, bool looping)
	: looping(looping), data(std::move(data))
{
	// Don't start the thread until this object is fully constructed.
	audioThread = thread(&AsyncAudioSupplier::Decode, this);
}



AsyncAudioSupplier::~AsyncAudioSupplier()
{
	// Tell the decoding thread to stop.
	{
		lock_guard<mutex> lock(bufferMutex);
		done = true;
	}
	bufferCondition.notify_all();
	audioThread.join();
}



size_t AsyncAudioSupplier::MaxChunks() const
{
	if(done && buffer.size() < OUTPUT_CHUNK)
		return 0;

	return max(static_cast<size_t>(2), AvailableChunks());
}



size_t AsyncAudioSupplier::AvailableChunks() const
{
	return buffer.size() / OUTPUT_CHUNK;
}


vector<AudioSupplier::sample_t> AsyncAudioSupplier::NextDataChunk()
{
	if(AvailableChunks())
	{
		lock_guard<mutex> lock(bufferMutex);

		vector<sample_t> temp{buffer.begin(), buffer.begin() + OUTPUT_CHUNK};
		buffer.erase(buffer.begin(), buffer.begin() + OUTPUT_CHUNK);
		bufferCondition.notify_all();
		return temp;
	}
	else
		return vector<sample_t>(OUTPUT_CHUNK);
}



void AsyncAudioSupplier::AwaitBufferSpace()
{
	unique_lock<mutex> lock(bufferMutex);
	while(!done && buffer.size() > (BUFFER_CHUNK_SIZE - 1) * OUTPUT_CHUNK)
		bufferCondition.wait(lock);
}



void AsyncAudioSupplier::AddBufferData(vector<sample_t> &samples)
{
	lock_guard<mutex> lock(bufferMutex);
	buffer.insert(buffer.end(), samples.begin(), samples.end());
	samples.clear();
	if(done)
		PadBuffer();
}



void AsyncAudioSupplier::PadBuffer()
{
	buffer.resize(OUTPUT_CHUNK * ceil(static_cast<double>(buffer.size()) / static_cast<double>(OUTPUT_CHUNK)));
}




size_t AsyncAudioSupplier::ReadInput(char *output, size_t bytesToRead)
{
	if(done)
		return 0;
	// Read a chunk of data from the file.
	data->read(output, bytesToRead);
	size_t read = data->gcount();
	// If we get the end of the file, loop around to the beginning.
	if(data->eof() && looping)
	{
		data->clear();
		data->seekg(0, iostream::beg);
	}
	else if(data->eof())
		done = true;
	return read;
}
