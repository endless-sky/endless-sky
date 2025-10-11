/* FlacSupplier.cpp
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

#include "FlacSupplier.h"

#include "../../Logger.h"

#include <utility>

using namespace std;



FlacSupplier::FlacSupplier(shared_ptr<iostream> data, bool looping)
	: AsyncAudioSupplier(std::move(data), looping)
{
}



FLAC__StreamDecoderWriteStatus FlacSupplier::write_callback(const FLAC__Frame *frame, const FLAC__int32 *const buffer[])
{
	const size_t channels = frame->header.channels;
	const size_t blocksize = frame->header.blocksize;

	AwaitBufferSpace();
	static vector<sample_t> samples;
	for(size_t i = 0; i < blocksize; ++i)
		for(size_t ch = 0; ch < channels; ++ch)
			samples.push_back(static_cast<sample_t>(buffer[ch][i]));

	AddBufferData(samples);
	/// Allow looping back to the beginning of the file on the next read.
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}



void FlacSupplier::metadata_callback(const FLAC__StreamMetadata *metadata)
{
	if(metadata->data.stream_info.channels != 2)
	{
		Logger::Log("FLAC channel count should be two, but is " + to_string(metadata->data.stream_info.channels)
			+ ". The audio may be corrupt.", Logger::Level::WARNING);
		done = true;
	}
	if(metadata->data.stream_info.bits_per_sample != 16)
	{
		Logger::Log("FLAC should use 16-bit samples, but is " + to_string(metadata->data.stream_info.bits_per_sample)
			+ "-bit instead. The audio may be corrupt.", Logger::Level::WARNING);
		done = true;
	}
	if(metadata->data.stream_info.sample_rate != SAMPLE_RATE)
	{
		Logger::Log("FLAC should use " + to_string(SAMPLE_RATE) + " sample rate, but is "
			+ to_string(metadata->data.stream_info.sample_rate) + ". The audio may be corrupt.",
			Logger::Level::WARNING);
		done = true;
	}
}



void FlacSupplier::error_callback(FLAC__StreamDecoderErrorStatus status)
{
	Logger::Log("FLAC error " + string(FLAC__StreamDecoderErrorStatusString[status]), Logger::Level::WARNING);
	done = true;
	PadBuffer();
}



FLAC__StreamDecoderReadStatus FlacSupplier::read_callback(FLAC__byte buffer[], size_t *bytes)
{
	size_t requestedBytes = *bytes;
	size_t readBytes = ReadInput(reinterpret_cast<char*>(buffer), requestedBytes);
	*bytes = readBytes;
	if(readBytes != requestedBytes)
		lastReadWasEof = true;

	return done ? FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM : FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}



FLAC__StreamDecoderSeekStatus FlacSupplier::seek_callback(FLAC__uint64 absolute_byte_offset)
{
	return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
}



FLAC__StreamDecoderTellStatus FlacSupplier::tell_callback(FLAC__uint64 *absolute_byte_offset)
{
	return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
}



FLAC__StreamDecoderLengthStatus FlacSupplier::length_callback(FLAC__uint64 *stream_length)
{
	return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
}



bool FlacSupplier::eof_callback()
{
	return done || lastReadWasEof;
}



void FlacSupplier::Decode()
{
	init();
	do {
		lastReadWasEof = false;
		process_until_end_of_stream();
		reset();
	} while(!done && lastReadWasEof);
	finish();
}
