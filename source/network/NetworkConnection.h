/* NetworkConnection.h
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

#include "NetworkConstants.h"

#include <enet/enet.h>
#include <chrono>
#include <string>
#include <cstdint>


// Represents a single network connection (used by the server to track clients).
// Wraps an ENet peer and provides connection state management.
class NetworkConnection {
public:
	// Create a connection from an ENet peer
	explicit NetworkConnection(ENetPeer *peer);
	~NetworkConnection() = default;

	// Prevent copying (connections are unique)
	NetworkConnection(const NetworkConnection &) = delete;
	NetworkConnection &operator=(const NetworkConnection &) = delete;

	// Allow moving
	NetworkConnection(NetworkConnection &&) noexcept = default;
	NetworkConnection &operator=(NetworkConnection &&) noexcept = default;

	// Get the underlying ENet peer
	ENetPeer *GetPeer() const { return peer; }

	// Get connection state
	NetworkConstants::ConnectionState GetState() const { return state; }
	void SetState(NetworkConstants::ConnectionState newState);

	// Get connection info
	std::string GetAddress() const;
	uint16_t GetPort() const;
	uint32_t GetConnectionId() const { return connectionId; }

	// Get network statistics
	uint32_t GetRoundTripTime() const;  // RTT in milliseconds
	uint32_t GetPacketsSent() const;
	uint32_t GetPacketsLost() const;
	float GetPacketLossPercent() const;

	// Connection timing
	std::chrono::steady_clock::time_point GetConnectTime() const { return connectTime; }
	uint64_t GetConnectionDurationMs() const;

	// Send a packet on this connection
	bool SendPacket(const void *data, size_t size, NetworkConstants::Channel channel, bool reliable = true);

	// Disconnect this connection
	void Disconnect(uint32_t data = 0);
	void DisconnectNow(uint32_t data = 0);

	// Check if connection is still valid
	bool IsConnected() const;
	bool IsDisconnecting() const;


private:
	ENetPeer *peer;
	NetworkConstants::ConnectionState state;
	std::chrono::steady_clock::time_point connectTime;
	uint32_t connectionId;

	// Static counter for generating unique connection IDs
	static uint32_t nextConnectionId;
};
