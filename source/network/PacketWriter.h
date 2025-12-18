/* PacketWriter.h
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
#include <vector>

class Angle;
class Command;
class EsUuid;
class Point;


// Binary packet serialization for network transmission
// Handles endianness conversion (writes in network byte order - big endian)
class PacketWriter {
public:
	// Create a packet writer with packet type
	explicit PacketWriter(NetworkPacket::PacketType type);

	// Write primitive types (automatically converts to network byte order)
	void WriteUint8(uint8_t value);
	void WriteUint16(uint16_t value);
	void WriteUint32(uint32_t value);
	void WriteUint64(uint64_t value);

	void WriteInt8(int8_t value);
	void WriteInt16(int16_t value);
	void WriteInt32(int32_t value);
	void WriteInt64(int64_t value);

	void WriteFloat(float value);
	void WriteDouble(double value);

	// Write string (length-prefixed: uint16_t length + string data)
	void WriteString(const std::string &value);

	// Write game types
	void WritePoint(const Point &point);        // 16 bytes (2 doubles)
	void WriteAngle(const Angle &angle);        // 4 bytes (int32_t)
	void WriteUuid(const EsUuid &uuid);         // 16 bytes
	void WriteCommand(const Command &command);  // 16 bytes (uint64_t + double)

	// Write raw bytes (for custom data)
	void WriteBytes(const void *data, size_t size);

	// Get the final packet data (includes header + payload)
	const std::vector<uint8_t> &GetData() const;
	const uint8_t *GetDataPtr() const;
	size_t GetSize() const;

	// Clear the buffer and reset to new packet type
	void Reset(NetworkPacket::PacketType type);

	// Get the packet type
	NetworkPacket::PacketType GetPacketType() const { return packetType; }


private:
	// Finalize packet (writes header at beginning)
	void Finalize() const;

	// Endianness conversion helpers
	static uint16_t HostToNetwork16(uint16_t value);
	static uint32_t HostToNetwork32(uint32_t value);
	static uint64_t HostToNetwork64(uint64_t value);


private:
	NetworkPacket::PacketType packetType;
	mutable std::vector<uint8_t> buffer;
	mutable bool finalized;
};
