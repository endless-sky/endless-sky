/* NetworkClient.h
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

#include "NetworkManager.h"
#include "NetworkConstants.h"

#include <enet/enet.h>
#include <string>
#include <functional>
#include <chrono>


// Client-side network manager. Handles connecting to a server,
// sending commands, and receiving state updates.
class NetworkClient : public NetworkManager {
public:
	// Callback types
	using OnConnectedCallback = std::function<void()>;
	using OnDisconnectedCallback = std::function<void()>;
	using OnConnectionFailedCallback = std::function<void()>;
	using OnPacketReceivedCallback = std::function<void(const uint8_t *, size_t)>;

	NetworkClient();
	~NetworkClient() override;

	// Connect to a server
	bool Connect(const std::string &hostname, uint16_t port = NetworkConstants::DEFAULT_SERVER_PORT);

	// Disconnect from server
	void Disconnect();

	// Process network events (call once per frame)
	void Update() override;

	// Shutdown (alias for Disconnect)
	void Shutdown() override { Disconnect(); }

	// Check connection state
	bool IsConnected() const;
	bool IsConnecting() const;
	NetworkConstants::ConnectionState GetState() const { return state; }

	// Send packet to server
	bool SendToServer(const void *data, size_t size,
		NetworkConstants::Channel channel = NetworkConstants::Channel::RELIABLE_ORDERED, bool reliable = true);

	// Get connection info
	std::string GetServerAddress() const { return serverAddress; }
	uint16_t GetServerPort() const { return serverPort; }
	uint32_t GetRoundTripTime() const;  // Ping in milliseconds
	float GetPacketLossPercent() const;

	// Set event callbacks
	void SetOnConnected(OnConnectedCallback callback) { onConnected = callback; }
	void SetOnDisconnected(OnDisconnectedCallback callback) { onDisconnected = callback; }
	void SetOnConnectionFailed(OnConnectionFailedCallback callback) { onConnectionFailed = callback; }
	void SetOnPacketReceived(OnPacketReceivedCallback callback) { onPacketReceived = callback; }


private:
	// Handle ENet events
	void HandleConnectEvent(ENetEvent &event);
	void HandleDisconnectEvent(ENetEvent &event);
	void HandleReceiveEvent(ENetEvent &event);

	// Update connection state
	void UpdateConnectionState();

	// Server peer
	ENetPeer *serverPeer;

	// Connection state
	NetworkConstants::ConnectionState state;

	// Server info
	std::string serverAddress;
	uint16_t serverPort;

	// Connection timing
	std::chrono::steady_clock::time_point connectionStartTime;

	// Event callbacks
	OnConnectedCallback onConnected;
	OnDisconnectedCallback onDisconnected;
	OnConnectionFailedCallback onConnectionFailed;
	OnPacketReceivedCallback onPacketReceived;
};
