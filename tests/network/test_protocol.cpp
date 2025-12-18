/* test_protocol.cpp
 * Copyright (c) 2025 by Endless Sky Development Team
 *
 * Comprehensive tests for protocol handler and validation
 */

#include "../../source/network/PacketHandler.h"
#include "../../source/network/PacketValidator.h"
#include "../../source/network/PacketWriter.h"
#include "../../source/network/PacketReader.h"
#include "../../source/network/Packet.h"

#include <iostream>
#include <cstring>

using namespace std;
using namespace NetworkPacket;

// Stub for NetworkConnection (not needed for these tests)
class NetworkConnection {};

// Stubs for Random and Logger (needed by Angle and EsUuid)
namespace Random {
	void Seed(uint64_t seed) {}
	uint64_t Int() { return 0; }
	uint32_t Int(uint32_t max) { return max > 0 ? 0 : 0; }
}

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


// Test 1: Handler registration
bool TestHandlerRegistration()
{
	PacketHandler handler;

	// Initially empty
	if(handler.GetHandlerCount() != 0)
		return false;

	// Register handler
	bool handlerCalled = false;
	handler.RegisterHandler(PacketType::PING, [&](PacketReader &reader, NetworkConnection *conn) {
		handlerCalled = true;
	});

	// Should have 1 handler
	if(handler.GetHandlerCount() != 1)
		return false;

	// Should have handler for PING
	if(!handler.HasHandler(PacketType::PING))
		return false;

	// Should not have handler for PONG
	if(handler.HasHandler(PacketType::PONG))
		return false;

	return true;
}


// Test 2: Handler dispatch
bool TestHandlerDispatch()
{
	PacketHandler handler;

	// Register handler
	bool handlerCalled = false;
	uint32_t receivedValue = 0;

	handler.RegisterHandler(PacketType::PING, [&](PacketReader &reader, NetworkConnection *conn) {
		handlerCalled = true;
		receivedValue = reader.ReadUint32();
	});

	// Create packet
	PacketWriter writer(PacketType::PING);
	writer.WriteUint32(12345);

	// Dispatch
	bool dispatched = handler.Dispatch(writer.GetDataPtr(), writer.GetSize());

	return dispatched && handlerCalled && (receivedValue == 12345);
}


// Test 3: Multiple handlers
bool TestMultipleHandlers()
{
	PacketHandler handler;

	int pingCount = 0;
	int pongCount = 0;

	handler.RegisterHandler(PacketType::PING, [&](PacketReader &r, NetworkConnection *c) {
		pingCount++;
	});

	handler.RegisterHandler(PacketType::PONG, [&](PacketReader &r, NetworkConnection *c) {
		pongCount++;
	});

	// Send PING
	PacketWriter ping(PacketType::PING);
	handler.Dispatch(ping.GetDataPtr(), ping.GetSize());

	// Send PONG
	PacketWriter pong(PacketType::PONG);
	handler.Dispatch(pong.GetDataPtr(), pong.GetSize());

	// Send PING again
	handler.Dispatch(ping.GetDataPtr(), ping.GetSize());

	return (pingCount == 2) && (pongCount == 1) && (handler.GetHandlerCount() == 2);
}


// Test 4: Unregister handler
bool TestUnregisterHandler()
{
	PacketHandler handler;

	bool handlerCalled = false;
	handler.RegisterHandler(PacketType::PING, [&](PacketReader &r, NetworkConnection *c) {
		handlerCalled = true;
	});

	// Should have handler
	if(!handler.HasHandler(PacketType::PING))
		return false;

	// Unregister
	handler.UnregisterHandler(PacketType::PING);

	// Should not have handler
	if(handler.HasHandler(PacketType::PING))
		return false;

	// Try to dispatch - should fail
	PacketWriter writer(PacketType::PING);
	bool dispatched = handler.Dispatch(writer.GetDataPtr(), writer.GetSize());

	return !dispatched && !handlerCalled;
}


// Test 5: Handler not found
bool TestHandlerNotFound()
{
	PacketHandler handler;

	// No handlers registered
	PacketWriter writer(PacketType::PING);
	bool dispatched = handler.Dispatch(writer.GetDataPtr(), writer.GetSize());

	return !dispatched;  // Should fail to dispatch
}


// Test 6: Clear all handlers
bool TestClearHandlers()
{
	PacketHandler handler;

	handler.RegisterHandler(PacketType::PING, [](PacketReader &r, NetworkConnection *c) {});
	handler.RegisterHandler(PacketType::PONG, [](PacketReader &r, NetworkConnection *c) {});
	handler.RegisterHandler(PacketType::CONNECT_REQUEST, [](PacketReader &r, NetworkConnection *c) {});

	if(handler.GetHandlerCount() != 3)
		return false;

	handler.Clear();

	return (handler.GetHandlerCount() == 0);
}


// Test 7: CRC32 computation
bool TestCRC32Computation()
{
	const char *testData = "Hello, World!";
	size_t size = strlen(testData);

	uint32_t crc = PacketValidator::ComputeCRC32(
		reinterpret_cast<const uint8_t *>(testData),
		size
	);

	// CRC32 of "Hello, World!" is 0xEC4AC3D0 (IEEE 802.3)
	return (crc == 0xEC4AC3D0);
}


// Test 8: CRC32 verification
bool TestCRC32Verification()
{
	const char *testData = "Test Data";
	size_t size = strlen(testData);

	uint32_t crc = PacketValidator::ComputeCRC32(
		reinterpret_cast<const uint8_t *>(testData),
		size
	);

	// Verify with correct CRC
	bool validCorrect = PacketValidator::VerifyCRC32(
		reinterpret_cast<const uint8_t *>(testData),
		size,
		crc
	);

	// Verify with wrong CRC
	bool validWrong = PacketValidator::VerifyCRC32(
		reinterpret_cast<const uint8_t *>(testData),
		size,
		crc + 1
	);

	return validCorrect && !validWrong;
}


// Test 9: CRC32 empty data
bool TestCRC32EmptyData()
{
	uint32_t crc = PacketValidator::ComputeCRC32(nullptr, 0);

	// CRC32 of empty data is 0x00000000
	return (crc == 0x00000000);
}


// Test 10: Packet CRC
bool TestPacketCRC()
{
	PacketWriter writer(PacketType::PING);
	writer.WriteUint32(12345);

	uint32_t crc = PacketValidator::ComputePacketCRC(
		writer.GetDataPtr(),
		writer.GetSize()
	);

	// CRC should be non-zero for valid packet
	if(crc == 0)
		return false;

	// Verify CRC
	bool valid = PacketValidator::VerifyCRC32(
		writer.GetDataPtr(),
		writer.GetSize(),
		crc
	);

	return valid;
}


// Test 11: Protocol version compatibility
bool TestProtocolVersionCompatibility()
{
	// Same version - should be compatible
	bool sameVersion = PacketHandler::IsProtocolCompatible(1, 1);

	// Different versions - currently not compatible
	bool diffVersion = PacketHandler::IsProtocolCompatible(1, 2);

	return sameVersion && !diffVersion;
}


// Test 12: Get current protocol version
bool TestGetProtocolVersion()
{
	uint16_t version = PacketHandler::GetCurrentProtocolVersion();

	return (version == PROTOCOL_VERSION) && (version == 1);
}


// Test 13: Invalid packet dispatch
bool TestInvalidPacketDispatch()
{
	PacketHandler handler;

	handler.RegisterHandler(PacketType::PING, [](PacketReader &r, NetworkConnection *c) {});

	// Create invalid packet (wrong magic number)
	uint8_t invalidData[20];
	memset(invalidData, 0, 20);

	bool dispatched = handler.Dispatch(invalidData, 20);

	return !dispatched;  // Should fail due to invalid packet
}


// Test 14: Large packet CRC
bool TestLargePacketCRC()
{
	PacketWriter writer(PacketType::SERVER_WORLD_STATE);

	// Write 1000 values
	for(int i = 0; i < 1000; ++i)
		writer.WriteUint32(i);

	uint32_t crc = PacketValidator::ComputePacketCRC(
		writer.GetDataPtr(),
		writer.GetSize()
	);

	bool valid = PacketValidator::VerifyCRC32(
		writer.GetDataPtr(),
		writer.GetSize(),
		crc
	);

	return valid;
}


// Test 15: Handler with PacketReader
bool TestHandlerWithPacketReader()
{
	PacketHandler handler;

	bool correctType = false;
	uint32_t receivedValue = 0;

	handler.RegisterHandler(PacketType::CLIENT_COMMAND, [&](PacketReader &reader, NetworkConnection *conn) {
		correctType = (reader.GetPacketType() == PacketType::CLIENT_COMMAND);
		receivedValue = reader.ReadUint32();
	});

	// Create packet
	PacketWriter writer(PacketType::CLIENT_COMMAND);
	writer.WriteUint32(99999);

	// Create reader and dispatch
	PacketReader reader(writer.GetDataPtr(), writer.GetSize());
	bool dispatched = handler.Dispatch(reader);

	return dispatched && correctType && (receivedValue == 99999);
}


int main()
{
	cout << "=== Protocol Handler and Validation Tests ===" << endl;
	cout << endl;

	// Handler tests
	ReportTest("Test 1: Handler Registration", TestHandlerRegistration());
	ReportTest("Test 2: Handler Dispatch", TestHandlerDispatch());
	ReportTest("Test 3: Multiple Handlers", TestMultipleHandlers());
	ReportTest("Test 4: Unregister Handler", TestUnregisterHandler());
	ReportTest("Test 5: Handler Not Found", TestHandlerNotFound());
	ReportTest("Test 6: Clear All Handlers", TestClearHandlers());

	// CRC32 tests
	ReportTest("Test 7: CRC32 Computation", TestCRC32Computation());
	ReportTest("Test 8: CRC32 Verification", TestCRC32Verification());
	ReportTest("Test 9: CRC32 Empty Data", TestCRC32EmptyData());
	ReportTest("Test 10: Packet CRC", TestPacketCRC());

	// Protocol tests
	ReportTest("Test 11: Protocol Version Compatibility", TestProtocolVersionCompatibility());
	ReportTest("Test 12: Get Protocol Version", TestGetProtocolVersion());
	ReportTest("Test 13: Invalid Packet Dispatch", TestInvalidPacketDispatch());
	ReportTest("Test 14: Large Packet CRC", TestLargePacketCRC());
	ReportTest("Test 15: Handler with PacketReader", TestHandlerWithPacketReader());

	cout << endl;
	cout << "=== Test Results ===" << endl;
	cout << "Tests Run: " << testsRun << endl;
	cout << "Tests Passed: " << testsPassed << endl;
	cout << "Tests Failed: " << (testsRun - testsPassed) << endl;

	return (testsPassed == testsRun) ? 0 : 1;
}
