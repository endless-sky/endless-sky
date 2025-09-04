/* SilenceSupplier.cpp
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

#include "SilenceSupplier.h"

#include <cmath>

using namespace std;



SilenceSupplier::SilenceSupplier(double seconds) : seconds(seconds)
{
}



size_t SilenceSupplier::MaxChunks() const
{
	return ceil(seconds / (static_cast<double>(OUTPUT_CHUNK) / SAMPLE_RATE / 2.)) - consumedBuffers;
}



size_t SilenceSupplier::AvailableChunks() const
{
	return MaxChunks();
}



vector<AudioSupplier::sample_t> SilenceSupplier::NextDataChunk()
{
	++consumedBuffers;
	return vector<sample_t>(OUTPUT_CHUNK);
}
