/* FlacSupplier.h
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

#pragma once

#include "AsyncAudioSupplier.h"

#include <FLAC++/decoder.h>



/// Streams audio from a FLAC file.
class FlacSupplier : protected FLAC::Decoder::Stream, public AsyncAudioSupplier {
// Maintenance note: the order of the parents is important. AsyncAudioSupplier must be destructed first,
// otherwise the FLAC::Decoder::Stream may free the resources with the decoder thread running.
public:
	explicit FlacSupplier(std::shared_ptr<std::iostream> data, bool looping = false);


private:
	/// A flac frame has been processed; forward the samples to the output buffer.
	FLAC__StreamDecoderWriteStatus write_callback(const FLAC__Frame *frame, const FLAC__int32 *const buffer[]) override;
	/// Metadata was read from the file.
	void metadata_callback(const FLAC__StreamMetadata *metadata) override;
	void error_callback(FLAC__StreamDecoderErrorStatus status) override;

	/// Read bytes of input into the buffer.
	FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes) override;

	/// Seeking is not supported, and shouldn't be called in runtime.
	FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset) override;
	FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset) override;
	FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length) override;
	bool eof_callback() override;

	/// This is the entry point for the decoding thread.
	void Decode() override;


private:
	/// If the last read reached the end of the file, we may have to loop back by resetting the decoder.
	bool lastReadWasEof = false;
};
