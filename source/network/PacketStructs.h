/* PacketStructs.h
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

#include <cstdint>
#include <string>


// Packed network data structures
// These structures are serialized/deserialized using PacketWriter/PacketReader
// All multi-byte values use network byte order (big-endian)

namespace NetworkPacket {

// Example packed structures for common packet types
// These are NOT sent as raw structs (endianness issues)
// but define the layout that PacketWriter/PacketReader use

// Ship state packet - full ship state update
// Sent from server to clients at 20Hz for visible ships
// Total payload: ~89 bytes (excluding string fields)
struct ShipStatePacket {
	uint8_t uuidBytes[16];       // 16 bytes - Ship UUID
	double positionX;            // 8 bytes - Position X
	double positionY;            // 8 bytes - Position Y
	double velocityX;            // 8 bytes - Velocity X
	double velocityY;            // 8 bytes - Velocity Y
	int32_t facing;              // 4 bytes - Angle (int32_t)
	float shields;               // 4 bytes - Shields (0-1 normalized)
	float hull;                  // 4 bytes - Hull (0-1 normalized)
	float energy;                // 4 bytes - Energy (0-1 normalized)
	float fuel;                  // 4 bytes - Fuel (0-1 normalized)
	uint16_t flags;              // 2 bytes - Status flags (thrusting, firing, etc.)
	// Variable length fields:
	// - shipName (string, length-prefixed)
	// - modelName (string, length-prefixed)
};

// Ship command packet - player input commands
// Sent from client to server at 60Hz
// Total payload: 25 bytes
struct ShipCommandPacket {
	uint8_t uuidBytes[16];       // 16 bytes - Ship UUID
	uint64_t commandState;       // 8 bytes - Command bitmask
	double commandTurn;          // 8 bytes - Turn amount (-1 to 1)
	uint32_t sequenceNumber;     // 4 bytes - For reconciliation
	// Total: 36 bytes
};

// Connect request packet - initial client connection
// Sent from client to server
struct ConnectRequestPacket {
	uint16_t clientProtocolVersion;  // 2 bytes
	// Variable length fields:
	// - playerName (string, length-prefixed)
	// - clientVersion (string, length-prefixed)
};

// Connect accept packet - server accepts connection
// Sent from server to client
struct ConnectAcceptPacket {
	uint8_t playerUuidBytes[16]; // 16 bytes - Assigned player UUID
	uint32_t serverTickRate;     // 4 bytes - Server simulation tick rate
	uint32_t worldStateRate;     // 4 bytes - World state broadcast rate
	// Variable length fields:
	// - serverName (string, length-prefixed)
	// - welcomeMessage (string, length-prefixed)
};

// Connect reject packet - server rejects connection
// Sent from server to client
struct ConnectRejectPacket {
	uint8_t reasonCode;          // 1 byte - Rejection reason code
	// Variable length fields:
	// - reasonMessage (string, length-prefixed)
};

// Ping/Pong packet - latency measurement
// Bidirectional
struct PingPongPacket {
	uint64_t timestamp;          // 8 bytes - Timestamp (milliseconds since epoch)
	uint32_t sequenceNumber;     // 4 bytes - Sequence number
	// Total: 12 bytes
};

// Chat message packet
// Bidirectional
struct ChatPacket {
	uint8_t senderUuidBytes[16]; // 16 bytes - Sender player UUID (0 for server)
	uint64_t timestamp;          // 8 bytes - Message timestamp
	// Variable length fields:
	// - senderName (string, length-prefixed)
	// - message (string, length-prefixed)
};

// Projectile spawn packet - new projectile created
// Sent from server to clients
struct ProjectileSpawnPacket {
	uint8_t projectileUuidBytes[16]; // 16 bytes - Projectile UUID
	uint8_t sourceShipUuidBytes[16]; // 16 bytes - Source ship UUID
	double positionX;                // 8 bytes
	double positionY;                // 8 bytes
	double velocityX;                // 8 bytes
	double velocityY;                // 8 bytes
	int32_t facing;                  // 4 bytes
	float damage;                    // 4 bytes
	// Variable length fields:
	// - weaponName (string, length-prefixed)
};

// Ship destroyed packet - ship was destroyed
// Sent from server to clients
struct ShipDestroyedPacket {
	uint8_t shipUuidBytes[16];       // 16 bytes - Destroyed ship UUID
	uint8_t killerUuidBytes[16];     // 16 bytes - Killer UUID (0 if environment)
	uint8_t destructionType;         // 1 byte - Type of destruction
};

// Effect spawn packet - visual effect spawned
// Sent from server to clients
struct EffectSpawnPacket {
	double positionX;                // 8 bytes
	double positionY;                // 8 bytes
	double velocityX;                // 8 bytes
	double velocityY;                // 8 bytes
	// Variable length fields:
	// - effectName (string, length-prefixed)
};

// Player join packet - new player joined
// Sent from server to clients
struct PlayerJoinPacket {
	uint8_t playerUuidBytes[16];     // 16 bytes - Player UUID
	// Variable length fields:
	// - playerName (string, length-prefixed)
};

// Player leave packet - player left
// Sent from server to clients
struct PlayerLeavePacket {
	uint8_t playerUuidBytes[16];     // 16 bytes - Player UUID
	uint8_t reasonCode;              // 1 byte - Leave reason
	// Variable length fields:
	// - reasonMessage (string, length-prefixed)
};

// Disconnect packet - graceful disconnect
// Bidirectional
struct DisconnectPacket {
	uint8_t reasonCode;              // 1 byte - Disconnect reason
	// Variable length fields:
	// - reasonMessage (string, length-prefixed)
};


// Status flag bits for ShipStatePacket
namespace ShipFlags {
	constexpr uint16_t THRUSTING      = 0x0001;
	constexpr uint16_t REVERSE        = 0x0002;
	constexpr uint16_t TURNING_LEFT   = 0x0004;
	constexpr uint16_t TURNING_RIGHT  = 0x0008;
	constexpr uint16_t FIRING_PRIMARY = 0x0010;
	constexpr uint16_t FIRING_SECONDARY = 0x0020;
	constexpr uint16_t AFTERBURNER    = 0x0040;
	constexpr uint16_t CLOAKED        = 0x0080;
	constexpr uint16_t HYPERSPACING   = 0x0100;
	constexpr uint16_t LANDING        = 0x0200;
	constexpr uint16_t DISABLED       = 0x0400;
	constexpr uint16_t OVERHEATED     = 0x0800;
}

// Rejection reason codes for ConnectRejectPacket
enum class RejectReason : uint8_t {
	SERVER_FULL = 0,
	INCOMPATIBLE_VERSION = 1,
	BANNED = 2,
	INVALID_NAME = 3,
	INTERNAL_ERROR = 4,
	MAINTENANCE = 5,
};

// Leave reason codes for PlayerLeavePacket
enum class LeaveReason : uint8_t {
	DISCONNECT = 0,
	TIMEOUT = 1,
	KICKED = 2,
	BANNED = 3,
	ERROR = 4,
};

// Disconnect reason codes for DisconnectPacket
enum class DisconnectReason : uint8_t {
	USER_QUIT = 0,
	KICKED = 1,
	TIMEOUT = 2,
	PROTOCOL_ERROR = 3,
	INTERNAL_ERROR = 4,
};

// Destruction type codes for ShipDestroyedPacket
enum class DestructionType : uint8_t {
	COMBAT = 0,
	COLLISION = 1,
	SELF_DESTRUCT = 2,
	ENVIRONMENTAL = 3,
};


} // namespace NetworkPacket
