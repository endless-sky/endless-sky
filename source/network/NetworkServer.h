/* NetworkServer.h
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
#include "NetworkConnection.h"

#include <vector>
#include <memory>
#include <functional>


// Server-side network manager. Handles accepting client connections,
// receiving client packets, and broadcasting state updates.
class NetworkServer : public NetworkManager {
public:
	// Callback types
	using OnClientConnectedCallback = std::function<void(NetworkConnection &)>;
	using OnClientDisconnectedCallback = std::function<void(NetworkConnection &)>;
	using OnPacketReceivedCallback = std::function<void(NetworkConnection &, const uint8_t *, size_t)>;

	NetworkServer();
	~NetworkServer() override;

	// Start the server on the specified port
	bool Start(uint16_t port = NetworkConstants::DEFAULT_SERVER_PORT);

	// Stop the server and disconnect all clients
	void Stop();

	// Process network events (call once per frame)
	void Update() override;

	// Shutdown (alias for Stop)
	void Shutdown() override { Stop(); }

	// Check if server is running
	bool IsRunning() const { return IsActive(); }

	// Get connected clients
	const std::vector<std::unique_ptr<NetworkConnection>> &GetConnections() const { return connections; }
	size_t GetClientCount() const { return connections.size(); }

	// Send packet to specific client
	bool SendToClient(NetworkConnection &connection, const void *data, size_t size,
		NetworkConstants::Channel channel = NetworkConstants::Channel::RELIABLE_ORDERED, bool reliable = true);

	// Send packet to all connected clients
	void BroadcastToAll(const void *data, size_t size,
		NetworkConstants::Channel channel = NetworkConstants::Channel::RELIABLE_ORDERED, bool reliable = true);

	// Send packet to all clients except one
	void BroadcastToAllExcept(NetworkConnection &except, const void *data, size_t size,
		NetworkConstants::Channel channel = NetworkConstants::Channel::RELIABLE_ORDERED, bool reliable = true);

	// Disconnect a specific client
	void DisconnectClient(NetworkConnection &connection, uint32_t data = 0);

	// Set event callbacks
	void SetOnClientConnected(OnClientConnectedCallback callback) { onClientConnected = callback; }
	void SetOnClientDisconnected(OnClientDisconnectedCallback callback) { onClientDisconnected = callback; }
	void SetOnPacketReceived(OnPacketReceivedCallback callback) { onPacketReceived = callback; }


private:
	// Handle ENet events
	void HandleConnectEvent(ENetEvent &event);
	void HandleDisconnectEvent(ENetEvent &event);
	void HandleReceiveEvent(ENetEvent &event);

	// Find connection by peer
	NetworkConnection *FindConnection(ENetPeer *peer);

	// Connected clients
	std::vector<std::unique_ptr<NetworkConnection>> connections;

	// Server port
	uint16_t port;

	// Event callbacks
	OnClientConnectedCallback onClientConnected;
	OnClientDisconnectedCallback onClientDisconnected;
	OnPacketReceivedCallback onPacketReceived;
};
