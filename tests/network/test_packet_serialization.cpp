/* test_packet_serialization.cpp
 * Copyright (c) 2025 by Endless Sky Development Team
 *
 * Comprehensive unit tests for binary packet serialization/deserialization
 */

#include "../../source/network/PacketWriter.h"
#include "../../source/network/PacketReader.h"
#include "../../source/network/Packet.h"
#include "../../source/Angle.h"
#include "../../source/EsUuid.h"
#include "../../source/Point.h"

#include <iostream>
#include <cstring>
#include <cmath>
#include <stdexcept>

using namespace std;
using namespace NetworkPacket;

// Stub Random class for Angle and EsUuid
namespace Random {
	void Seed(uint64_t seed) {}
	uint64_t Int() { return 0; }
	uint32_t Int(uint32_t max) { return max > 0 ? 0 : 0; }
}

// Stub Logger class for EsUuid
namespace Logger {
	enum class Level { ERROR };
	void Log(const std::string &message, Level level = Level::ERROR) {}
}


// Test result tracking
int testsRun = 0;
int testsPassed = 0;

void ReportTest(const string &name, bool passed)
{
	testsRun++;
	if(passed)
	{
		testsPassed++;
		cout << "[PASS] " << name << endl;
	}
	else
	{
		cout << "[FAIL] " << name << endl;
	}
}


// Test 1: Basic packet header validation
bool TestPacketHeader()
{
	PacketWriter writer(PacketType::PING);
	writer.WriteUint32(12345);

	const uint8_t *data = writer.GetDataPtr();
	size_t size = writer.GetSize();

	// Create reader
	PacketReader reader(data, size);

	// Validate header
	bool valid = reader.IsValid();
	bool correctType = (reader.GetPacketType() == PacketType::PING);
	bool correctVersion = (reader.GetProtocolVersion() == PROTOCOL_VERSION);

	return valid && correctType && correctVersion;
}


// Test 2: Primitive type serialization (uint8, uint16, uint32, uint64)
bool TestPrimitiveTypes()
{
	PacketWriter writer(PacketType::CLIENT_COMMAND);

	// Write various primitive types
	writer.WriteUint8(42);
	writer.WriteUint16(1234);
	writer.WriteUint32(567890);
	writer.WriteUint64(9876543210ULL);

	writer.WriteInt8(-42);
	writer.WriteInt16(-1234);
	writer.WriteInt32(-567890);
	writer.WriteInt64(-9876543210LL);

	// Read them back
	PacketReader reader(writer.GetDataPtr(), writer.GetSize());

	bool u8Match = (reader.ReadUint8() == 42);
	bool u16Match = (reader.ReadUint16() == 1234);
	bool u32Match = (reader.ReadUint32() == 567890);
	bool u64Match = (reader.ReadUint64() == 9876543210ULL);

	bool i8Match = (reader.ReadInt8() == -42);
	bool i16Match = (reader.ReadInt16() == -1234);
	bool i32Match = (reader.ReadInt32() == -567890);
	bool i64Match = (reader.ReadInt64() == -9876543210LL);

	return u8Match && u16Match && u32Match && u64Match &&
	       i8Match && i16Match && i32Match && i64Match;
}


// Test 3: Floating point serialization (float, double)
bool TestFloatingPoint()
{
	PacketWriter writer(PacketType::SERVER_SHIP_UPDATE);

	float testFloat = 3.14159f;
	double testDouble = 2.718281828459045;

	writer.WriteFloat(testFloat);
	writer.WriteDouble(testDouble);

	PacketReader reader(writer.GetDataPtr(), writer.GetSize());

	float readFloat = reader.ReadFloat();
	double readDouble = reader.ReadDouble();

	// Check with epsilon for floating point comparison
	bool floatMatch = (fabs(readFloat - testFloat) < 0.00001f);
	bool doubleMatch = (fabs(readDouble - testDouble) < 0.000000001);

	return floatMatch && doubleMatch;
}


// Test 4: String serialization (length-prefixed)
bool TestStrings()
{
	PacketWriter writer(PacketType::CLIENT_CHAT);

	string shortStr = "Hello";
	string longStr = "This is a much longer string for testing purposes!";
	string emptyStr = "";

	writer.WriteString(shortStr);
	writer.WriteString(longStr);
	writer.WriteString(emptyStr);

	PacketReader reader(writer.GetDataPtr(), writer.GetSize());

	string readShort = reader.ReadString();
	string readLong = reader.ReadString();
	string readEmpty = reader.ReadString();

	return (readShort == shortStr) && (readLong == longStr) && (readEmpty == emptyStr);
}


// Test 5: Point serialization (2 doubles)
bool TestPoint()
{
	PacketWriter writer(PacketType::SERVER_SHIP_UPDATE);

	Point testPoint(123.456, -789.012);
	writer.WritePoint(testPoint);

	PacketReader reader(writer.GetDataPtr(), writer.GetSize());
	Point readPoint = reader.ReadPoint();

	bool xMatch = (fabs(readPoint.X() - testPoint.X()) < 0.000001);
	bool yMatch = (fabs(readPoint.Y() - testPoint.Y()) < 0.000001);

	return xMatch && yMatch;
}


// Test 6: Angle serialization
bool TestAngle()
{
	PacketWriter writer(PacketType::SERVER_SHIP_UPDATE);

	Angle testAngle(45.0);
	writer.WriteAngle(testAngle);

	PacketReader reader(writer.GetDataPtr(), writer.GetSize());
	Angle readAngle = reader.ReadAngle();

	// Compare degrees
	bool match = (fabs(readAngle.Degrees() - testAngle.Degrees()) < 0.000001);

	return match;
}


// Test 7: UUID serialization
bool TestUuid()
{
	PacketWriter writer(PacketType::SERVER_PLAYER_JOIN);

	// Create a UUID from string
	EsUuid testUuid = EsUuid::FromString("550e8400-e29b-41d4-a716-446655440000");
	writer.WriteUuid(testUuid);

	PacketReader reader(writer.GetDataPtr(), writer.GetSize());
	EsUuid readUuid = reader.ReadUuid();

	// Compare string representations
	return (readUuid.ToString() == testUuid.ToString());
}


// Test 8: Round-trip test (write and read back)
bool TestRoundTrip()
{
	PacketWriter writer(PacketType::SERVER_WORLD_STATE);

	// Write complex data
	writer.WriteUint32(12345);
	writer.WriteString("Test Message");
	writer.WriteFloat(1.23f);
	writer.WriteDouble(4.56789);
	Point testPoint(100.0, 200.0);
	writer.WritePoint(testPoint);

	// Read it all back
	PacketReader reader(writer.GetDataPtr(), writer.GetSize());

	uint32_t readUint = reader.ReadUint32();
	string readStr = reader.ReadString();
	float readFloat = reader.ReadFloat();
	double readDouble = reader.ReadDouble();
	Point readPoint = reader.ReadPoint();

	bool u32Match = (readUint == 12345);
	bool strMatch = (readStr == "Test Message");
	bool floatMatch = (fabs(readFloat - 1.23f) < 0.00001f);
	bool doubleMatch = (fabs(readDouble - 4.56789) < 0.000001);
	bool pointMatch = (fabs(readPoint.X() - 100.0) < 0.000001) &&
	                  (fabs(readPoint.Y() - 200.0) < 0.000001);

	return u32Match && strMatch && floatMatch && doubleMatch && pointMatch;
}


// Test 9: Buffer overflow protection
bool TestBufferOverflow()
{
	PacketWriter writer(PacketType::PING);
	writer.WriteUint32(42);

	PacketReader reader(writer.GetDataPtr(), writer.GetSize());

	// Read the uint32
	reader.ReadUint32();

	// Try to read beyond buffer
	uint32_t overflow = reader.ReadUint32();

	// Should have error flag set
	return reader.HasError();
}


// Test 10: Endianness handling
bool TestEndianness()
{
	PacketWriter writer(PacketType::PONG);

	// Write a known value
	uint32_t testValue = 0x12345678;
	writer.WriteUint32(testValue);

	const uint8_t *data = writer.GetDataPtr();

	// Check that bytes are in network byte order (big-endian)
	// Byte at offset PACKET_HEADER_SIZE should be 0x12 (most significant byte)
	bool correctOrder = (data[PACKET_HEADER_SIZE] == 0x12);

	// Also verify round-trip works
	PacketReader reader(data, writer.GetSize());
	uint32_t readValue = reader.ReadUint32();
	bool roundTrip = (readValue == testValue);

	return correctOrder && roundTrip;
}


// Test 11: Multiple packets
bool TestMultiplePackets()
{
	// Write first packet
	PacketWriter writer1(PacketType::CLIENT_COMMAND);
	writer1.WriteUint32(111);

	// Write second packet
	PacketWriter writer2(PacketType::SERVER_SHIP_UPDATE);
	writer2.WriteUint32(222);

	// Read both
	PacketReader reader1(writer1.GetDataPtr(), writer1.GetSize());
	PacketReader reader2(writer2.GetDataPtr(), writer2.GetSize());

	bool type1Match = (reader1.GetPacketType() == PacketType::CLIENT_COMMAND);
	bool type2Match = (reader2.GetPacketType() == PacketType::SERVER_SHIP_UPDATE);
	bool value1Match = (reader1.ReadUint32() == 111);
	bool value2Match = (reader2.ReadUint32() == 222);

	return type1Match && type2Match && value1Match && value2Match;
}


// Test 12: Empty packet (only header)
bool TestEmptyPacket()
{
	PacketWriter writer(PacketType::DISCONNECT);
	// Don't write any payload

	PacketReader reader(writer.GetDataPtr(), writer.GetSize());

	bool valid = reader.IsValid();
	bool correctType = (reader.GetPacketType() == PacketType::DISCONNECT);
	bool emptyPayload = (reader.GetPayloadSize() == 0);

	return valid && correctType && emptyPayload;
}


// Test 13: Large packet
bool TestLargePacket()
{
	PacketWriter writer(PacketType::SERVER_WORLD_STATE);

	// Write many values
	for(int i = 0; i < 1000; ++i)
		writer.WriteUint32(i);

	PacketReader reader(writer.GetDataPtr(), writer.GetSize());

	// Read them back
	bool allMatch = true;
	for(int i = 0; i < 1000; ++i)
	{
		uint32_t value = reader.ReadUint32();
		if(value != static_cast<uint32_t>(i))
		{
			allMatch = false;
			break;
		}
	}

	return allMatch && !reader.HasError();
}


// Test 14: Reset functionality
bool TestReset()
{
	PacketWriter writer(PacketType::PING);
	writer.WriteUint32(123);

	// Reset to new packet type
	writer.Reset(PacketType::PONG);
	writer.WriteUint32(456);

	PacketReader reader(writer.GetDataPtr(), writer.GetSize());

	bool correctType = (reader.GetPacketType() == PacketType::PONG);
	bool correctValue = (reader.ReadUint32() == 456);

	return correctType && correctValue;
}


// Test 15: Invalid packet detection
bool TestInvalidPacket()
{
	// Create buffer with invalid magic number
	uint8_t fakeData[20];
	memset(fakeData, 0, 20);

	PacketReader reader(fakeData, 20);

	// Should detect invalid packet
	return !reader.IsValid();
}


int main()
{
	cout << "=== Packet Serialization Tests ===" << endl;
	cout << endl;

	ReportTest("Test 1: Packet Header Validation", TestPacketHeader());
	ReportTest("Test 2: Primitive Types", TestPrimitiveTypes());
	ReportTest("Test 3: Floating Point", TestFloatingPoint());
	ReportTest("Test 4: Strings", TestStrings());
	ReportTest("Test 5: Point", TestPoint());
	ReportTest("Test 6: Angle", TestAngle());
	ReportTest("Test 7: UUID", TestUuid());
	ReportTest("Test 8: Round Trip", TestRoundTrip());
	ReportTest("Test 9: Buffer Overflow Protection", TestBufferOverflow());
	ReportTest("Test 10: Endianness Handling", TestEndianness());
	ReportTest("Test 11: Multiple Packets", TestMultiplePackets());
	ReportTest("Test 12: Empty Packet", TestEmptyPacket());
	ReportTest("Test 13: Large Packet", TestLargePacket());
	ReportTest("Test 14: Reset Functionality", TestReset());
	ReportTest("Test 15: Invalid Packet Detection", TestInvalidPacket());

	cout << endl;
	cout << "=== Test Results ===" << endl;
	cout << "Tests Run: " << testsRun << endl;
	cout << "Tests Passed: " << testsPassed << endl;
	cout << "Tests Failed: " << (testsRun - testsPassed) << endl;

	return (testsPassed == testsRun) ? 0 : 1;
}
