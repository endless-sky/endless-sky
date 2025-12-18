/* PacketHandler.h
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
#include <functional>
#include <unordered_map>

class NetworkConnection;
class PacketReader;


// Packet handler dispatch system
// Routes incoming packets to registered handler functions
class PacketHandler {
public:
	// Handler function signature
	// Parameters: packet reader, connection (nullptr for client-side handlers)
	using HandlerFunc = std::function<void(PacketReader &, NetworkConnection *)>;

	PacketHandler() = default;
	~PacketHandler() = default;

	// Register a handler for a specific packet type
	void RegisterHandler(NetworkPacket::PacketType type, HandlerFunc handler);

	// Unregister a handler
	void UnregisterHandler(NetworkPacket::PacketType type);

	// Check if a handler is registered for a packet type
	bool HasHandler(NetworkPacket::PacketType type) const;

	// Dispatch a packet to its registered handler
	// Returns true if handler was found and executed, false otherwise
	bool Dispatch(const uint8_t *data, size_t size, NetworkConnection *connection = nullptr);

	// Dispatch using PacketReader (already validated)
	bool Dispatch(PacketReader &reader, NetworkConnection *connection = nullptr);

	// Get number of registered handlers
	size_t GetHandlerCount() const;

	// Clear all handlers
	void Clear();

	// Protocol version negotiation
	static bool IsProtocolCompatible(uint16_t clientVersion, uint16_t serverVersion);
	static uint16_t GetCurrentProtocolVersion();


private:
	std::unordered_map<NetworkPacket::PacketType, HandlerFunc> handlers;
};
