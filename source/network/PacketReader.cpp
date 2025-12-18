/* PacketReader.cpp
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

#include "PacketReader.h"

#include "Angle.h"
#include "Command.h"
#include "EsUuid.h"
#include "Point.h"

#include <cstring>

using namespace std;


PacketReader::PacketReader(const uint8_t *data, size_t size)
	: data(data), dataSize(size), position(0), error(false)
{
	// Read and validate header
	if(size < NetworkPacket::PACKET_HEADER_SIZE)
	{
		error = true;
		memset(&header, 0, sizeof(header));
		return;
	}

	// Read header
	memcpy(&header, data, NetworkPacket::PACKET_HEADER_SIZE);

	// Convert header fields from network byte order
	header.magic = NetworkToHost32(header.magic);
	header.protocolVersion = NetworkToHost16(header.protocolVersion);
	// header.type is already 1 byte, no conversion needed
	header.payloadSize = NetworkToHost32(header.payloadSize);

	// Move position past header
	position = NetworkPacket::PACKET_HEADER_SIZE;

	// Validate header
	if(header.magic != NetworkPacket::PACKET_MAGIC)
		error = true;

	// Check payload size matches
	if(position + header.payloadSize != dataSize)
		error = true;
}


bool PacketReader::IsValid() const
{
	return !error;
}


uint16_t PacketReader::GetProtocolVersion() const
{
	return header.protocolVersion;
}


uint32_t PacketReader::GetPayloadSize() const
{
	return header.payloadSize;
}


bool PacketReader::CanRead(size_t bytes) const
{
	return !error && (position + bytes <= dataSize);
}


uint8_t PacketReader::ReadUint8()
{
	if(!CanRead(1))
	{
		error = true;
		return 0;
	}
	return data[position++];
}


uint16_t PacketReader::ReadUint16()
{
	if(!CanRead(sizeof(uint16_t)))
	{
		error = true;
		return 0;
	}

	uint16_t value;
	memcpy(&value, data + position, sizeof(uint16_t));
	position += sizeof(uint16_t);
	return NetworkToHost16(value);
}


uint32_t PacketReader::ReadUint32()
{
	if(!CanRead(sizeof(uint32_t)))
	{
		error = true;
		return 0;
	}

	uint32_t value;
	memcpy(&value, data + position, sizeof(uint32_t));
	position += sizeof(uint32_t);
	return NetworkToHost32(value);
}


uint64_t PacketReader::ReadUint64()
{
	if(!CanRead(sizeof(uint64_t)))
	{
		error = true;
		return 0;
	}

	uint64_t value;
	memcpy(&value, data + position, sizeof(uint64_t));
	position += sizeof(uint64_t);
	return NetworkToHost64(value);
}


int8_t PacketReader::ReadInt8()
{
	return static_cast<int8_t>(ReadUint8());
}


int16_t PacketReader::ReadInt16()
{
	return static_cast<int16_t>(ReadUint16());
}


int32_t PacketReader::ReadInt32()
{
	return static_cast<int32_t>(ReadUint32());
}


int64_t PacketReader::ReadInt64()
{
	return static_cast<int64_t>(ReadUint64());
}


float PacketReader::ReadFloat()
{
	uint32_t intValue = ReadUint32();
	float value;
	memcpy(&value, &intValue, sizeof(float));
	return value;
}


double PacketReader::ReadDouble()
{
	uint64_t intValue = ReadUint64();
	double value;
	memcpy(&value, &intValue, sizeof(double));
	return value;
}


string PacketReader::ReadString()
{
	// Read length
	uint16_t length = ReadUint16();
	if(error || !CanRead(length))
	{
		error = true;
		return "";
	}

	// Read string data
	string result(reinterpret_cast<const char *>(data + position), length);
	position += length;
	return result;
}


Point PacketReader::ReadPoint()
{
	double x = ReadDouble();
	double y = ReadDouble();
	return Point(x, y);
}


Angle PacketReader::ReadAngle()
{
	// Read as double and convert to Angle
	double degrees = ReadDouble();
	return Angle(degrees);
}


EsUuid PacketReader::ReadUuid()
{
	// Read string representation and convert to EsUuid
	string uuidStr = ReadString();
	return EsUuid::FromString(uuidStr);
}


Command PacketReader::ReadCommand()
{
	// Read uint64_t state and double turn
	// Since Command doesn't expose constructors for these, we'll need to
	// add friend class or serialization methods
	// For now, read the values but return empty command
	uint64_t state = ReadUint64();
	double turn = ReadDouble();

	// TODO: Construct Command from state and turn
	// This requires adding serialization support to Command class
	return Command();
}


void PacketReader::ReadBytes(void *dest, size_t size)
{
	if(!CanRead(size))
	{
		error = true;
		return;
	}

	memcpy(dest, data + position, size);
	position += size;
}


size_t PacketReader::GetRemainingBytes() const
{
	if(error || position >= dataSize)
		return 0;
	return dataSize - position;
}


void PacketReader::Reset()
{
	position = NetworkPacket::PACKET_HEADER_SIZE;
	error = false;
}


uint16_t PacketReader::NetworkToHost16(uint16_t value)
{
	// Check if system is little-endian
	uint16_t test = 1;
	bool isLittleEndian = (*reinterpret_cast<uint8_t *>(&test) == 1);

	if(isLittleEndian)
	{
		return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
	}
	return value;
}


uint32_t PacketReader::NetworkToHost32(uint32_t value)
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


uint64_t PacketReader::NetworkToHost64(uint64_t value)
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
