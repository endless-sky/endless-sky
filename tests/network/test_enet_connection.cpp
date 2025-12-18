/* test_enet_connection.cpp
 * Proof-of-concept test for ENet integration
 *
 * This test validates that ENet library is properly integrated and can:
 * 1. Initialize ENet
 * 2. Create a server
 * 3. Create a client and connect to server
 * 4. Send/receive packets
 * 5. Clean up gracefully
 */

#include <enet/enet.h>
#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>

constexpr int TEST_PORT = 12345;
constexpr int MAX_CLIENTS = 32;
constexpr int CHANNEL_COUNT = 2;
constexpr int TIMEOUT_MS = 5000;

std::atomic<bool> serverRunning{false};
std::atomic<bool> testPassed{false};
std::atomic<bool> clientConnected{false};
std::atomic<bool> messageReceived{false};

// Server thread function
void runServer() {
	ENetAddress address;
	ENetHost* server;

	// Bind to localhost
	address.host = ENET_HOST_ANY;
	address.port = TEST_PORT;

	// Create server
	server = enet_host_create(&address, MAX_CLIENTS, CHANNEL_COUNT, 0, 0);
	if (server == nullptr) {
		std::cerr << "[SERVER] Failed to create server" << std::endl;
		return;
	}

	std::cout << "[SERVER] Started on port " << TEST_PORT << std::endl;
	serverRunning = true;

	ENetEvent event;
	int connectionCount = 0;
	bool receivedMessage = false;

	// Server event loop
	while (serverRunning && connectionCount < 2) {
		while (enet_host_service(server, &event, 100) > 0) {
			switch (event.type) {
				case ENET_EVENT_TYPE_CONNECT:
					std::cout << "[SERVER] Client connected from "
						<< (event.peer->address.host & 0xFF) << "."
						<< ((event.peer->address.host >> 8) & 0xFF) << "."
						<< ((event.peer->address.host >> 16) & 0xFF) << "."
						<< ((event.peer->address.host >> 24) & 0xFF)
						<< ":" << event.peer->address.port << std::endl;
					connectionCount++;
					clientConnected = true;
					break;

				case ENET_EVENT_TYPE_RECEIVE: {
					std::string message(reinterpret_cast<char*>(event.packet->data));
					std::cout << "[SERVER] Received: \"" << message << "\"" << std::endl;

					if (message == "Hello from client!") {
						// Send response back
						const char* response = "Hello from server!";
						ENetPacket* packet = enet_packet_create(response, strlen(response) + 1, ENET_PACKET_FLAG_RELIABLE);
						enet_peer_send(event.peer, 0, packet);
						enet_host_flush(server);
						std::cout << "[SERVER] Sent response" << std::endl;
						receivedMessage = true;
					}

					enet_packet_destroy(event.packet);
					break;
				}

				case ENET_EVENT_TYPE_DISCONNECT:
					std::cout << "[SERVER] Client disconnected" << std::endl;
					event.peer->data = nullptr;
					connectionCount++;
					break;

				case ENET_EVENT_TYPE_NONE:
					break;
			}
		}
	}

	if (receivedMessage) {
		std::cout << "[SERVER] Test successful!" << std::endl;
		testPassed = true;
	}

	// Clean up
	enet_host_destroy(server);
	std::cout << "[SERVER] Shut down" << std::endl;
}

// Client test function
bool runClient() {
	std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Wait for server to start

	ENetHost* client;
	ENetAddress address;
	ENetPeer* peer;

	// Create client
	client = enet_host_create(nullptr, 1, CHANNEL_COUNT, 0, 0);
	if (client == nullptr) {
		std::cerr << "[CLIENT] Failed to create client" << std::endl;
		return false;
	}

	// Set up server address
	enet_address_set_host(&address, "127.0.0.1");
	address.port = TEST_PORT;

	// Connect to server
	std::cout << "[CLIENT] Connecting to localhost:" << TEST_PORT << std::endl;
	peer = enet_host_connect(client, &address, CHANNEL_COUNT, 0);
	if (peer == nullptr) {
		std::cerr << "[CLIENT] No available peers for connection" << std::endl;
		enet_host_destroy(client);
		return false;
	}

	ENetEvent event;
	bool connected = false;
	bool receivedResponse = false;

	// Wait for connection
	if (enet_host_service(client, &event, TIMEOUT_MS) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
		std::cout << "[CLIENT] Connected to server" << std::endl;
		connected = true;
	} else {
		std::cerr << "[CLIENT] Connection failed or timed out" << std::endl;
		enet_peer_reset(peer);
		enet_host_destroy(client);
		return false;
	}

	// Send test message
	const char* message = "Hello from client!";
	ENetPacket* packet = enet_packet_create(message, strlen(message) + 1, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, 0, packet);
	enet_host_flush(client);
	std::cout << "[CLIENT] Sent: \"" << message << "\"" << std::endl;

	// Wait for response
	auto startTime = std::chrono::steady_clock::now();
	while (!receivedResponse) {
		while (enet_host_service(client, &event, 100) > 0) {
			if (event.type == ENET_EVENT_TYPE_RECEIVE) {
				std::string response(reinterpret_cast<char*>(event.packet->data));
				std::cout << "[CLIENT] Received: \"" << response << "\"" << std::endl;

				if (response == "Hello from server!") {
					receivedResponse = true;
					messageReceived = true;
				}

				enet_packet_destroy(event.packet);
			}
		}

		// Timeout check
		auto now = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() > TIMEOUT_MS) {
			std::cerr << "[CLIENT] Timeout waiting for response" << std::endl;
			break;
		}
	}

	// Disconnect gracefully
	enet_peer_disconnect(peer, 0);

	// Wait for disconnect acknowledgment
	bool disconnected = false;
	while (enet_host_service(client, &event, 3000) > 0) {
		if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
			std::cout << "[CLIENT] Disconnected" << std::endl;
			disconnected = true;
			break;
		}
	}

	if (!disconnected) {
		enet_peer_reset(peer);
	}

	// Clean up
	enet_host_destroy(client);
	std::cout << "[CLIENT] Shut down" << std::endl;

	return receivedResponse;
}

int main(int argc, char** argv) {
	std::cout << "=== ENet Integration Test ===" << std::endl;
	std::cout << "ENet Version: " << ENET_VERSION_MAJOR << "."
	          << ENET_VERSION_MINOR << "." << ENET_VERSION_PATCH << std::endl;

	// Initialize ENet
	if (enet_initialize() != 0) {
		std::cerr << "Failed to initialize ENet" << std::endl;
		return 1;
	}
	std::cout << "ENet initialized successfully" << std::endl;

	// Start server in separate thread
	std::thread serverThread(runServer);

	// Wait for server to start
	auto startTime = std::chrono::steady_clock::now();
	while (!serverRunning) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		auto now = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count() > 5) {
			std::cerr << "Server failed to start within timeout" << std::endl;
			enet_deinitialize();
			return 1;
		}
	}

	// Run client test
	bool clientSuccess = runClient();

	// Wait a moment for server to process
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// Signal server to shut down
	serverRunning = false;
	serverThread.join();

	// Clean up ENet
	enet_deinitialize();
	std::cout << "ENet deinitialized" << std::endl;

	// Report results
	std::cout << "\n=== Test Results ===" << std::endl;
	std::cout << "Client connected:   " << (clientConnected ? "✓ PASS" : "✗ FAIL") << std::endl;
	std::cout << "Message sent:       " << (clientSuccess ? "✓ PASS" : "✗ FAIL") << std::endl;
	std::cout << "Response received:  " << (messageReceived ? "✓ PASS" : "✗ FAIL") << std::endl;
	std::cout << "Server processed:   " << (testPassed ? "✓ PASS" : "✗ FAIL") << std::endl;

	if (clientConnected && clientSuccess && messageReceived && testPassed) {
		std::cout << "\n✓ ALL TESTS PASSED - ENet integration successful!" << std::endl;
		return 0;
	} else {
		std::cout << "\n✗ SOME TESTS FAILED" << std::endl;
		return 1;
	}
}
