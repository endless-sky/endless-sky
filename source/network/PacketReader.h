/* PacketReader.h
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

#include "Packet.h"

#include <cstdint>
#include <string>

class Angle;
class Command;
class EsUuid;
class Point;


// Binary packet deserialization for network reception
// Handles endianness conversion (reads from network byte order - big endian)
class PacketReader {
public:
	// Create a packet reader from received data (including header)
	PacketReader(const uint8_t *data, size_t size);

	// Validate packet header (returns true if valid)
	bool IsValid() const;

	// Get packet type and metadata
	NetworkPacket::PacketType GetPacketType() const { return header.type; }
	uint16_t GetProtocolVersion() const;
	uint32_t GetPayloadSize() const;

	// Check if there's enough data to read
	bool CanRead(size_t bytes) const;

	// Read primitive types (automatically converts from network byte order)
	uint8_t ReadUint8();
	uint16_t ReadUint16();
	uint32_t ReadUint32();
	uint64_t ReadUint64();

	int8_t ReadInt8();
	int16_t ReadInt16();
	int32_t ReadInt32();
	int64_t ReadInt64();

	float ReadFloat();
	double ReadDouble();

	// Read string (length-prefixed: uint16_t length + string data)
	std::string ReadString();

	// Read game types
	Point ReadPoint();        // 16 bytes (2 doubles)
	Angle ReadAngle();        // 8 bytes (double, converted to Angle)
	EsUuid ReadUuid();        // Variable (string representation)
	Command ReadCommand();    // 16 bytes (uint64_t + double)

	// Read raw bytes (for custom data)
	void ReadBytes(void *data, size_t size);

	// Get current read position and remaining bytes
	size_t GetPosition() const { return position; }
	size_t GetRemainingBytes() const;

	// Reset read position to start of payload
	void Reset();

	// Check if an error occurred during reading
	bool HasError() const { return error; }


private:
	// Endianness conversion helpers
	static uint16_t NetworkToHost16(uint16_t value);
	static uint32_t NetworkToHost32(uint32_t value);
	static uint64_t NetworkToHost64(uint64_t value);


private:
	const uint8_t *data;
	size_t dataSize;
	size_t position;
	bool error;
	NetworkPacket::PacketHeader header;
};
