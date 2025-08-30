/* Mp3Supplier.h
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



/// Streams audio from an MP3 file.
class Mp3Supplier : public AsyncAudioSupplier {
public:
	explicit Mp3Supplier(std::shared_ptr<std::iostream> data, bool looping = false);


private:
	/// This is the entry point for the decoding thread.
	void Decode() override;
};
