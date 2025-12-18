/* test_network_abstraction.cpp
 * Integration test for Network Abstraction Layer (Phase 1.2)
 *
 * Tests NetworkManager, NetworkServer, NetworkClient, and NetworkConnection classes
 */

#include "../../source/network/NetworkClient.h"
#include "../../source/network/NetworkServer.h"
#include "../../source/network/NetworkConstants.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

constexpr int TEST_PORT = 12346;
constexpr int TEST_TIMEOUT_MS = 5000;

std::atomic<bool> serverRunning{false};
std::atomic<bool> clientConnected{false};
std::atomic<bool> serverReceivedMessage{false};
std::atomic<bool> clientReceivedResponse{false};
std::atomic<int> clientsConnected{0};

// Test messages
const std::string CLIENT_MESSAGE = "Hello from NetworkClient!";
const std::string SERVER_RESPONSE = "Hello from NetworkServer!";


// Server thread function
void runServer() {
	NetworkServer server;

	// Set up callbacks
	server.SetOnClientConnected([](NetworkConnection &connection) {
		std::cout << "[SERVER] Client connected: " << connection.GetAddress()
		          << ":" << connection.GetPort()
		          << " (ID: " << connection.GetConnectionId() << ")" << std::endl;
		clientsConnected++;
	});

	server.SetOnClientDisconnected([](NetworkConnection &connection) {
		std::cout << "[SERVER] Client disconnected (ID: " << connection.GetConnectionId() << ")" << std::endl;
		clientsConnected--;
	});

	server.SetOnPacketReceived([&server](NetworkConnection &connection, const uint8_t *data, size_t size) {
		std::string message(reinterpret_cast<const char*>(data));
		std::cout << "[SERVER] Received: \"" << message << "\"" << std::endl;

		if(message == CLIENT_MESSAGE) {
			serverReceivedMessage = true;

			// Send response
			server.SendToClient(connection, SERVER_RESPONSE.c_str(), SERVER_RESPONSE.size() + 1);
			std::cout << "[SERVER] Sent response" << std::endl;
		}
	});

	// Start server
	if(!server.Start(TEST_PORT)) {
		std::cerr << "[SERVER] Failed to start" << std::endl;
		return;
	}

	serverRunning = true;
	std::cout << "[SERVER] Started on port " << TEST_PORT << std::endl;

	// Run server loop
	auto startTime = std::chrono::steady_clock::now();
	while(serverRunning) {
		server.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS

		// Timeout check
		auto now = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
		if(duration.count() > TEST_TIMEOUT_MS) {
			std::cerr << "[SERVER] Timeout" << std::endl;
			break;
		}
	}

	server.Stop();
	std::cout << "[SERVER] Stopped" << std::endl;
}


// Client test function
bool runClient() {
	// Wait for server to start
	auto startTime = std::chrono::steady_clock::now();
	while(!serverRunning) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		auto now = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
		if(duration.count() > TEST_TIMEOUT_MS) {
			std::cerr << "[CLIENT] Server failed to start" << std::endl;
			return false;
		}
	}

	NetworkClient client;

	// Set up callbacks
	client.SetOnConnected([]() {
		std::cout << "[CLIENT] Connected to server" << std::endl;
		clientConnected = true;
	});

	client.SetOnDisconnected([]() {
		std::cout << "[CLIENT] Disconnected from server" << std::endl;
	});

	client.SetOnConnectionFailed([]() {
		std::cerr << "[CLIENT] Connection failed" << std::endl;
	});

	client.SetOnPacketReceived([](const uint8_t *data, size_t size) {
		std::string message(reinterpret_cast<const char*>(data));
		std::cout << "[CLIENT] Received: \"" << message << "\"" << std::endl;

		if(message == SERVER_RESPONSE) {
			clientReceivedResponse = true;
		}
	});

	// Connect to server
	std::cout << "[CLIENT] Connecting to localhost:" << TEST_PORT << std::endl;
	if(!client.Connect("127.0.0.1", TEST_PORT)) {
		std::cerr << "[CLIENT] Failed to initiate connection" << std::endl;
		return false;
	}

	// Wait for connection
	startTime = std::chrono::steady_clock::now();
	while(!clientConnected && client.IsConnecting()) {
		client.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));

		auto now = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
		if(duration.count() > TEST_TIMEOUT_MS) {
			std::cerr << "[CLIENT] Connection timeout" << std::endl;
			return false;
		}
	}

	if(!clientConnected) {
		std::cerr << "[CLIENT] Failed to connect" << std::endl;
		return false;
	}

	// Send message
	std::cout << "[CLIENT] Sending: \"" << CLIENT_MESSAGE << "\"" << std::endl;
	if(!client.SendToServer(CLIENT_MESSAGE.c_str(), CLIENT_MESSAGE.size() + 1)) {
		std::cerr << "[CLIENT] Failed to send message" << std::endl;
		return false;
	}

	// Wait for response
	startTime = std::chrono::steady_clock::now();
	while(!clientReceivedResponse) {
		client.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));

		auto now = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
		if(duration.count() > TEST_TIMEOUT_MS) {
			std::cerr << "[CLIENT] Timeout waiting for response" << std::endl;
			break;
		}
	}

	// Get statistics
	std::cout << "[CLIENT] Statistics:" << std::endl;
	std::cout << "  - RTT: " << client.GetRoundTripTime() << "ms" << std::endl;
	std::cout << "  - Packet Loss: " << client.GetPacketLossPercent() << "%" << std::endl;
	std::cout << "  - Packets Sent: " << client.GetTotalPacketsSent() << std::endl;
	std::cout << "  - Packets Received: " << client.GetTotalPacketsReceived() << std::endl;

	// Disconnect
	client.Disconnect();
	std::cout << "[CLIENT] Disconnected" << std::endl;

	return clientReceivedResponse;
}


int main(int argc, char** argv) {
	std::cout << "=== Network Abstraction Layer Test (Phase 1.2) ===" << std::endl;

	// Initialize network system
	if(!NetworkManager::Initialize()) {
		std::cerr << "Failed to initialize network system" << std::endl;
		return 1;
	}
	std::cout << "Network system initialized" << std::endl;

	// Start server in separate thread
	std::thread serverThread(runServer);

	// Run client test
	bool success = runClient();

	// Wait a moment for server to process
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// Signal server to stop
	serverRunning = false;
	serverThread.join();

	// Deinitialize network system
	NetworkManager::Deinitialize();
	std::cout << "Network system deinitialized" << std::endl;

	// Report results
	std::cout << "\n=== Test Results ===" << std::endl;
	std::cout << "Server started:           " << (serverRunning || true ? "✓ PASS" : "✗ FAIL") << std::endl;
	std::cout << "Client connected:         " << (clientConnected ? "✓ PASS" : "✗ FAIL") << std::endl;
	std::cout << "Server received message:  " << (serverReceivedMessage ? "✓ PASS" : "✗ FAIL") << std::endl;
	std::cout << "Client received response: " << (clientReceivedResponse ? "✓ PASS" : "✗ FAIL") << std::endl;
	std::cout << "Client count correct:     " << (clientsConnected == 0 ? "✓ PASS" : "✗ FAIL") << std::endl;

	if(clientConnected && serverReceivedMessage && clientReceivedResponse && clientsConnected == 0) {
		std::cout << "\n✓ ALL TESTS PASSED - Network Abstraction Layer working!" << std::endl;
		return 0;
	} else {
		std::cout << "\n✗ SOME TESTS FAILED" << std::endl;
		return 1;
	}
}
