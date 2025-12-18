/* NetworkConstants.h
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
#include <cstddef>


// Network configuration constants for Endless Sky multiplayer.
namespace NetworkConstants {

	// ===== Server Configuration =====

	// Default server port for multiplayer games
	constexpr uint16_t DEFAULT_SERVER_PORT = 12345;

	// Maximum number of concurrent clients the server can handle
	constexpr size_t MAX_CLIENTS = 32;

	// Maximum number of channels per connection
	// Channel 0: Reliable ordered (connection, chat, missions)
	// Channel 1: Unreliable sequenced (positions, commands)
	// Channel 2: Reliable unordered (projectiles, effects)
	constexpr size_t CHANNEL_COUNT = 3;


	// ===== Network Performance =====

	// Bandwidth limits (0 = unlimited)
	// These can be tuned based on network conditions
	constexpr uint32_t INCOMING_BANDWIDTH = 0;  // bytes/second (0 = no limit)
	constexpr uint32_t OUTGOING_BANDWIDTH = 0;  // bytes/second (0 = no limit)

	// Recommended bandwidth for good performance
	constexpr uint32_t RECOMMENDED_UPLOAD = 57600;    // ~56 KB/s
	constexpr uint32_t RECOMMENDED_DOWNLOAD = 115200; // ~112 KB/s


	// ===== Timeouts =====

	// Connection timeout in milliseconds
	constexpr uint32_t CONNECTION_TIMEOUT_MS = 5000;  // 5 seconds

	// Disconnection timeout in milliseconds
	constexpr uint32_t DISCONNECTION_TIMEOUT_MS = 3000;  // 3 seconds

	// How long to wait for events when polling
	constexpr uint32_t EVENT_POLL_TIMEOUT_MS = 100;  // 100ms

	// Server tick timeout (how long to process events each frame)
	constexpr uint32_t SERVER_TICK_TIMEOUT_MS = 16;  // ~60 FPS

	// Client tick timeout
	constexpr uint32_t CLIENT_TICK_TIMEOUT_MS = 16;  // ~60 FPS

	// Ping interval (milliseconds between ping packets)
	constexpr uint32_t PING_INTERVAL_MS = 1000;  // 1 second


	// ===== Protocol Configuration =====

	// Protocol version - increment when making breaking changes
	constexpr uint8_t PROTOCOL_VERSION = 1;

	// Maximum packet size (bytes)
	// ENet's default maximum is 32MB, but we'll use a more reasonable limit
	constexpr size_t MAX_PACKET_SIZE = 1024 * 1024;  // 1 MB

	// Maximum message size for chat/text
	constexpr size_t MAX_CHAT_MESSAGE_SIZE = 512;


	// ===== Channel Configuration =====

	// Channel IDs
	enum class Channel : uint8_t {
		RELIABLE_ORDERED = 0,      // Connection, chat, missions (guaranteed order)
		UNRELIABLE_SEQUENCED = 1,  // Positions, commands (latest only)
		RELIABLE_UNORDERED = 2     // Projectiles, effects (guaranteed but any order)
	};


	// ===== Update Frequencies =====

	// Game simulation tick rate (both server and client)
	constexpr uint32_t SIMULATION_TICK_RATE = 60;  // 60 FPS

	// Server broadcasts state updates at this rate
	constexpr uint32_t SERVER_UPDATE_RATE = 20;  // 20 Hz (every 3 frames)

	// Client sends input commands at this rate
	constexpr uint32_t CLIENT_COMMAND_RATE = 60;  // 60 Hz (every frame)


	// ===== Connection States =====

	enum class ConnectionState : uint8_t {
		DISCONNECTED = 0,  // Not connected
		CONNECTING = 1,    // Connection in progress
		CONNECTED = 2,     // Successfully connected
		DISCONNECTING = 3, // Disconnection in progress
		TIMED_OUT = 4,     // Connection lost
		FAILED = 5         // Connection failed
	};


	// ===== Network Quality Thresholds =====

	// Latency thresholds (milliseconds)
	constexpr uint32_t LATENCY_EXCELLENT = 50;   // < 50ms: Excellent
	constexpr uint32_t LATENCY_GOOD = 100;       // < 100ms: Good
	constexpr uint32_t LATENCY_FAIR = 150;       // < 150ms: Fair
	constexpr uint32_t LATENCY_POOR = 200;       // < 200ms: Poor
	// > 200ms: Unplayable

	// Packet loss thresholds (percentage)
	constexpr float PACKET_LOSS_GOOD = 1.0f;      // < 1%: Good
	constexpr float PACKET_LOSS_FAIR = 3.0f;      // < 3%: Fair
	constexpr float PACKET_LOSS_POOR = 5.0f;      // < 5%: Poor
	// > 5%: Unplayable


	// ===== Buffer Sizes =====

	// Command buffer size (how many frames of commands to buffer)
	constexpr size_t COMMAND_BUFFER_SIZE = 120;  // 2 seconds at 60 FPS

	// Snapshot buffer size (how many server snapshots to keep)
	constexpr size_t SNAPSHOT_BUFFER_SIZE = 60;  // 3 seconds at 20 Hz

	// Prediction buffer size (for client-side prediction)
	constexpr size_t PREDICTION_BUFFER_SIZE = 120;  // 2 seconds at 60 FPS


	// ===== Magic Numbers =====

	// Magic number for packet validation (helps detect protocol mismatches)
	constexpr uint32_t PACKET_MAGIC = 0x45534D50;  // "ESMP" (Endless Sky MultiPlayer)

	// Magic number for save file validation
	constexpr uint32_t SAVE_MAGIC = 0x45534D53;  // "ESMS" (Endless Sky Multiplayer Save)


	// ===== Interpolation & Prediction =====

	// How many milliseconds to interpolate behind (for smooth movement)
	constexpr uint32_t INTERPOLATION_DELAY_MS = 100;  // 100ms behind server

	// Maximum extrapolation time (milliseconds)
	constexpr uint32_t MAX_EXTRAPOLATION_MS = 200;  // Don't predict beyond 200ms

	// Reconciliation threshold (position error in pixels before correction)
	constexpr double RECONCILIATION_THRESHOLD = 10.0;  // 10 pixels

	// Reconciliation smoothing factor (0-1, higher = faster correction)
	constexpr double RECONCILIATION_SMOOTHING = 0.1;  // 10% correction per frame


	// ===== Server Capacity =====

	// Recommended maximum entities per server
	constexpr size_t MAX_SHIPS_PER_SERVER = 1000;
	constexpr size_t MAX_PROJECTILES_PER_SERVER = 5000;
	constexpr size_t MAX_EFFECTS_PER_SERVER = 2000;


	// ===== Debug & Logging =====

	// Enable verbose network logging (disable for release)
	#ifdef DEBUG
	constexpr bool NETWORK_VERBOSE_LOGGING = true;
	#else
	constexpr bool NETWORK_VERBOSE_LOGGING = false;
	#endif

	// Log network statistics interval (milliseconds)
	constexpr uint32_t STATS_LOG_INTERVAL_MS = 5000;  // Every 5 seconds

} // namespace NetworkConstants
