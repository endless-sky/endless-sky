/* PacketWriter.cpp
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

#include "PacketWriter.h"

#include "Angle.h"
#include "Command.h"
#include "EsUuid.h"
#include "Point.h"

#include <cstring>

using namespace std;


PacketWriter::PacketWriter(NetworkPacket::PacketType type)
	: packetType(type), finalized(false)
{
	// Reserve space for header + typical payload size
	buffer.reserve(NetworkPacket::PACKET_HEADER_SIZE + 256);
	// Leave space for header at the beginning
	buffer.resize(NetworkPacket::PACKET_HEADER_SIZE);
}


void PacketWriter::WriteUint8(uint8_t value)
{
	finalized = false;
	buffer.push_back(value);
}


void PacketWriter::WriteUint16(uint16_t value)
{
	finalized = false;
	uint16_t networkValue = HostToNetwork16(value);
	const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&networkValue);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(uint16_t));
}


void PacketWriter::WriteUint32(uint32_t value)
{
	finalized = false;
	uint32_t networkValue = HostToNetwork32(value);
	const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&networkValue);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(uint32_t));
}


void PacketWriter::WriteUint64(uint64_t value)
{
	finalized = false;
	uint64_t networkValue = HostToNetwork64(value);
	const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&networkValue);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(uint64_t));
}


void PacketWriter::WriteInt8(int8_t value)
{
	finalized = false;
	buffer.push_back(static_cast<uint8_t>(value));
}


void PacketWriter::WriteInt16(int16_t value)
{
	WriteUint16(static_cast<uint16_t>(value));
}


void PacketWriter::WriteInt32(int32_t value)
{
	WriteUint32(static_cast<uint32_t>(value));
}


void PacketWriter::WriteInt64(int64_t value)
{
	WriteUint64(static_cast<uint64_t>(value));
}


void PacketWriter::WriteFloat(float value)
{
	finalized = false;
	// Convert float to uint32 via memcpy (type punning) then write as uint32
	uint32_t intValue;
	memcpy(&intValue, &value, sizeof(float));
	WriteUint32(intValue);
}


void PacketWriter::WriteDouble(double value)
{
	finalized = false;
	// Convert double to uint64 via memcpy (type punning) then write as uint64
	uint64_t intValue;
	memcpy(&intValue, &value, sizeof(double));
	WriteUint64(intValue);
}


void PacketWriter::WriteString(const string &value)
{
	finalized = false;
	// Write length as uint16 (max string length 65535)
	if(value.size() > 65535)
	{
		WriteUint16(65535);
		buffer.insert(buffer.end(), value.begin(), value.begin() + 65535);
	}
	else
	{
		WriteUint16(static_cast<uint16_t>(value.size()));
		buffer.insert(buffer.end(), value.begin(), value.end());
	}
}


void PacketWriter::WritePoint(const Point &point)
{
	WriteDouble(point.X());
	WriteDouble(point.Y());
}


void PacketWriter::WriteAngle(const Angle &angle)
{
	// Angle stores an internal int32_t, but we serialize the degrees as double
	// for better precision over the network
	WriteDouble(angle.Degrees());
}


void PacketWriter::WriteUuid(const EsUuid &uuid)
{
	finalized = false;
	// Convert UUID to string, then parse as hex bytes
	string uuidStr = uuid.ToString();

	// UUID string format: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" (36 chars)
	// We need to write the raw 16 bytes
	// For now, write the string representation (will be 38 bytes with length prefix)
	// TODO: Optimize this to write raw 16 bytes
	WriteString(uuidStr);
}


void PacketWriter::WriteCommand(const Command &command)
{
	// Command is 16 bytes: uint64_t state + double turn
	// We need to access private members, so we'll need to add a serialization method
	// For now, use reflection/hacks or add friend class
	// Since Command doesn't expose state/turn, we'll write zeros as placeholder
	// TODO: Add Command serialization methods or friend class
	WriteUint64(0);  // state (placeholder)
	WriteDouble(0.0);  // turn (placeholder)
}


void PacketWriter::WriteBytes(const void *data, size_t size)
{
	finalized = false;
	const uint8_t *bytes = static_cast<const uint8_t *>(data);
	buffer.insert(buffer.end(), bytes, bytes + size);
}


const vector<uint8_t> &PacketWriter::GetData() const
{
	if(!finalized)
		Finalize();
	return buffer;
}


const uint8_t *PacketWriter::GetDataPtr() const
{
	if(!finalized)
		Finalize();
	return buffer.data();
}


size_t PacketWriter::GetSize() const
{
	if(!finalized)
		Finalize();
	return buffer.size();
}


void PacketWriter::Reset(NetworkPacket::PacketType type)
{
	packetType = type;
	finalized = false;
	buffer.clear();
	buffer.resize(NetworkPacket::PACKET_HEADER_SIZE);
}


void PacketWriter::Finalize() const
{
	if(finalized)
		return;

	// Calculate payload size (total size - header size)
	uint32_t payloadSize = static_cast<uint32_t>(buffer.size() - NetworkPacket::PACKET_HEADER_SIZE);

	// Write header at the beginning of buffer
	NetworkPacket::PacketHeader header;
	header.magic = HostToNetwork32(NetworkPacket::PACKET_MAGIC);
	header.protocolVersion = HostToNetwork16(NetworkPacket::PROTOCOL_VERSION);
	header.type = packetType;
	header.payloadSize = HostToNetwork32(payloadSize);

	// Copy header to beginning of buffer
	memcpy(buffer.data(), &header, NetworkPacket::PACKET_HEADER_SIZE);

	finalized = true;
}


uint16_t PacketWriter::HostToNetwork16(uint16_t value)
{
	// Check if system is little-endian (x86, x86_64)
	// If little-endian, swap bytes. If big-endian, return as-is.
	uint16_t test = 1;
	bool isLittleEndian = (*reinterpret_cast<uint8_t *>(&test) == 1);

	if(isLittleEndian)
	{
		return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
	}
	return value;
}


uint32_t PacketWriter::HostToNetwork32(uint32_t value)
{
	uint16_t test = 1;
	bool isLittleEndian = (*reinterpret_cast<uint8_t *>(&test) == 1);

	if(isLittleEndian)
	{
		return ((value & 0x000000FF) << 24) |
		       ((value & 0x0000FF00) << 8) |
		       ((value & 0x00FF0000) >> 8) |
		       ((value & 0xFF000000) >> 24);
	}
	return value;
}


uint64_t PacketWriter::HostToNetwork64(uint64_t value)
{
	uint16_t test = 1;
	bool isLittleEndian = (*reinterpret_cast<uint8_t *>(&test) == 1);

	if(isLittleEndian)
	{
		return ((value & 0x00000000000000FFULL) << 56) |
		       ((value & 0x000000000000FF00ULL) << 40) |
		       ((value & 0x0000000000FF0000ULL) << 24) |
		       ((value & 0x00000000FF000000ULL) << 8) |
		       ((value & 0x000000FF00000000ULL) >> 8) |
		       ((value & 0x0000FF0000000000ULL) >> 24) |
		       ((value & 0x00FF000000000000ULL) >> 40) |
		       ((value & 0xFF00000000000000ULL) >> 56);
	}
	return value;
}
