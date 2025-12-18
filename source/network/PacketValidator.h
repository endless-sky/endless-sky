/* PacketValidator.h
 * Copyright (c) 2025 by Endless Sky Development Team
 *
 * Endless Sky is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <cstddef>


// Packet validation utilities
// Provides CRC32 checksums for data integrity verification
class PacketValidator {
public:
	// Compute CRC32 checksum for data
	static uint32_t ComputeCRC32(const uint8_t *data, size_t size);

	// Verify CRC32 checksum
	static bool VerifyCRC32(const uint8_t *data, size_t size, uint32_t expectedCRC);

	// Compute CRC32 for packet (excluding any embedded checksum field)
	// This is a convenience wrapper for packet data
	static uint32_t ComputePacketCRC(const uint8_t *packetData, size_t packetSize);


private:
	// CRC32 lookup table (IEEE 802.3 polynomial)
	static const uint32_t crc32Table[256];

	// Initialize CRC32 table (called once)
	static void InitializeCRC32Table();
	static bool tableInitialized;
};
