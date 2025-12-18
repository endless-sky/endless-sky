/* Packet.h
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

#include <cstddef>
#include <cstdint>


// Packet type definitions for network protocol
namespace NetworkPacket {

// Protocol version for backwards compatibility
constexpr uint16_t PROTOCOL_VERSION = 1;

// Magic number for packet validation (ASCII "ESMP" = Endless Sky MultiPlayer)
constexpr uint32_t PACKET_MAGIC = 0x45534D50;


// Packet type enum - defines all network message types
enum class PacketType : uint8_t {
	// Connection packets (1-9)
	CONNECT_REQUEST = 1,
	CONNECT_ACCEPT = 2,
	CONNECT_REJECT = 3,
	DISCONNECT = 4,
	PING = 5,
	PONG = 6,

	// Client → Server packets (10-19)
	CLIENT_COMMAND = 10,          // Ship commands (60Hz)
	CLIENT_CHAT = 11,
	CLIENT_READY = 12,

	// Server → Client packets (20-29)
	SERVER_WELCOME = 20,          // Initial connection data
	SERVER_WORLD_STATE = 21,      // Full state snapshot (20Hz)
	SERVER_SHIP_UPDATE = 22,      // Individual ship update
	SERVER_PROJECTILE_SPAWN = 23,
	SERVER_SHIP_DESTROYED = 24,
	SERVER_EFFECT_SPAWN = 25,
	SERVER_CHAT = 26,
	SERVER_PLAYER_JOIN = 27,
	SERVER_PLAYER_LEAVE = 28,

	// Synchronization packets (30-39)
	FULL_SYNC_REQUEST = 30,
	FULL_SYNC_RESPONSE = 31,
};


// Packet header - prepended to all packets
// Total size: 11 bytes
// Use packed attribute to prevent padding
#pragma pack(push, 1)
struct PacketHeader {
	uint32_t magic;           // 4 bytes - PACKET_MAGIC for validation
	uint16_t protocolVersion; // 2 bytes - PROTOCOL_VERSION
	PacketType type;          // 1 byte - packet type
	uint32_t payloadSize;     // 4 bytes - size of payload (not including header)
};
#pragma pack(pop)

// Size of packet header
constexpr size_t PACKET_HEADER_SIZE = sizeof(PacketHeader);
static_assert(PACKET_HEADER_SIZE == 11, "PacketHeader must be 11 bytes");


} // namespace NetworkPacket
